/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2013-2019  Hannah von Reth <vonreth@kde.org>

    SnoreToast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SnoreToast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnoreToast.  If not, see <http://www.gnu.org/licenses/>.
    */
#include "snoretoasts.h"
#include "toasteventhandler.h"
#include "linkhelper.h"
#include "utils.h"
#include "stringreferencewrapper.h"

#include <sstream>
#include <iostream>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

class SnoreToastsPrivate
{
public:
    SnoreToastsPrivate(SnoreToasts *parent, const std::wstring &appID)
        : m_parent(parent), m_appID(appID), m_id(std::to_wstring(GetCurrentProcessId()))
    {
    }
    SnoreToasts *m_parent;

    std::wstring m_appID;
    std::filesystem::path m_pipeName;
    std::filesystem::path m_application;

    std::wstring m_title;
    std::wstring m_body;
    std::filesystem::path m_image;
    std::wstring m_sound = L"Notification.Default";
    std::wstring m_id;
    std::wstring m_buttons;
    bool m_silent = false;
    bool m_wait = false;
    bool m_textbox = false;

    SnoreToastActions::Actions m_action = SnoreToastActions::Actions::Clicked;

    ComPtr<IXmlDocument> m_toastXml;
    ComPtr<Notifications::IToastNotificationManagerStatics> m_toastManager;
    ComPtr<Notifications::IToastNotifier> m_notifier;
    ComPtr<Notifications::IToastNotification> m_notification;

    ComPtr<ToastEventHandler> m_eventHanlder;

    static HANDLE ctoastEvent()
    {
        static HANDLE _event = [] {
            std::wstringstream eventName;
            eventName << L"ToastActivationEvent" << GetCurrentProcessId();
            return CreateEvent(nullptr, true, false, eventName.str().c_str());
        }();
        return _event;
    }
};

SnoreToasts::SnoreToasts(const std::wstring &appID) : d(new SnoreToastsPrivate(this, appID))
{
    Utils::registerActivator();
}

SnoreToasts::~SnoreToasts()
{
    Utils::unregisterActivator();
    delete d;
}

void SnoreToasts::displayToast(const std::wstring &title, const std::wstring &body,
                               const std::filesystem::path &image, bool wait)
{
    HRESULT hr = S_OK;
    d->m_title = title;
    d->m_body = body;
    d->m_image = image;
    d->m_wait = wait;

    hr = GetActivationFactory(
            StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager)
                    .Get(),
            &d->m_toastManager);
    if (!SUCCEEDED(hr)) {
        std::wcerr << L"SnoreToasts: Failed to register com Factory, please make sure you "
                      L"correctly initialised with RO_INIT_MULTITHREADED"
                   << std::endl;
        d->m_action = SnoreToastActions::Actions::Error;
        return;
    }

    if (!d->m_image.empty()) {
        hr = d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText02,
                                                   &d->m_toastXml);
    } else {
        hr = d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &d->m_toastXml);
    }

    if (SUCCEEDED(hr)) {
        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> rootList;
        hr = d->m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"toast").Get(), &rootList);

        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> root;
            hr = rootList->Item(0, &root);

            ComPtr<IXmlNamedNodeMap> rootAttributes;
            hr = root->get_Attributes(&rootAttributes);
            if (SUCCEEDED(hr)) {
                const auto data = formatAction(SnoreToastActions::Actions::Clicked);
                hr = addAttribute(L"launch", rootAttributes.Get(), data);
            }
            // Adding buttons
            if (!d->m_buttons.empty()) {
                setButtons(root);
            } else if (d->m_textbox) {
                setTextBox(root);
            }
            if (SUCCEEDED(hr)) {
                ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> audioElement;
                hr = d->m_toastXml->CreateElement(StringReferenceWrapper(L"audio").Get(),
                                                  &audioElement);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> audioNodeTmp;
                    hr = audioElement.As(&audioNodeTmp);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> audioNode;
                        hr = root->AppendChild(audioNodeTmp.Get(), &audioNode);
                        if (SUCCEEDED(hr)) {

                            ComPtr<IXmlNamedNodeMap> attributes;

                            hr = audioNode->get_Attributes(&attributes);
                            if (SUCCEEDED(hr)) {
                                hr = addAttribute(L"src", attributes.Get());
                                if (SUCCEEDED(hr)) {
                                    addAttribute(L"silent", attributes.Get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //    printXML();

    if (SUCCEEDED(hr)) {
        if (!d->m_image.empty()) {
            hr = setImage();
        }
        if (SUCCEEDED(hr)) {
            hr = setSound();
            if (SUCCEEDED(hr)) {
                hr = setTextValues();
                if (SUCCEEDED(hr)) {
                    printXML();
                    hr = createToast();
                }
            }
        }
    }
    d->m_action =
            SUCCEEDED(hr) ? SnoreToastActions::Actions::Clicked : SnoreToastActions::Actions::Error;
}

SnoreToastActions::Actions SnoreToasts::userAction()
{
    if (d->m_eventHanlder.Get()) {
        HANDLE event = d->m_eventHanlder.Get()->event();
        WaitForSingleObject(event, INFINITE);
        d->m_action = d->m_eventHanlder.Get()->userAction();
        if (d->m_action == SnoreToastActions::Actions::Hidden) {
            d->m_notifier->Hide(d->m_notification.Get());
            tLog << L"The application hid the toast using ToastNotifier.hide()";
        }
        CloseHandle(event);
    }
    return d->m_action;
}

bool SnoreToasts::closeNotification()
{
    std::wstringstream eventName;
    eventName << L"ToastEvent" << d->m_id;

    HANDLE event = OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName.str().c_str());
    if (event) {
        SetEvent(event);
        return true;
    }
    tLog << "Notification " << d->m_id << " does not exist";
    return false;
}

void SnoreToasts::setSound(const std::wstring &soundFile)
{
    d->m_sound = soundFile;
}

void SnoreToasts::setSilent(bool silent)
{
    d->m_silent = silent;
}

void SnoreToasts::setId(const std::wstring &id)
{
    if (!id.empty()) {
        d->m_id = id;
    }
}

std::wstring SnoreToasts::id() const
{
    return d->m_id;
}

void SnoreToasts::setButtons(const std::wstring &buttons)
{
    d->m_buttons = buttons;
}

void SnoreToasts::setTextBoxEnabled(bool textBoxEnabled)
{
    d->m_textbox = textBoxEnabled;
}

// Set the value of the "src" attribute of the "image" node
HRESULT SnoreToasts::setImage()
{
    HRESULT hr = S_OK;

    ComPtr<IXmlNodeList> nodeList;
    hr = d->m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> imageNode;
        hr = nodeList->Item(0, &imageNode);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNamedNodeMap> attributes;

            hr = imageNode->get_Attributes(&attributes);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> srcAttribute;

                hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
                if (SUCCEEDED(hr)) {
                    hr = setNodeValueString(StringReferenceWrapper(d->m_image).Get(),
                                            srcAttribute.Get());
                }
            }
        }
    }
    return hr;
}

HRESULT SnoreToasts::setSound()
{
    HRESULT hr = S_OK;
    ComPtr<IXmlNodeList> nodeList;
    hr = d->m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"audio").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> audioNode;
        hr = nodeList->Item(0, &audioNode);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNamedNodeMap> attributes;

            hr = audioNode->get_Attributes(&attributes);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> srcAttribute;

                hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
                if (SUCCEEDED(hr)) {
                    std::wstring sound;
                    if (d->m_sound.find(L"ms-winsoundevent:") == std::wstring::npos) {
                        sound = L"ms-winsoundevent:";
                        sound.append(d->m_sound);
                    } else {
                        sound = d->m_sound;
                    }

                    hr = setNodeValueString(StringReferenceWrapper(sound).Get(),
                                            srcAttribute.Get());
                    if (SUCCEEDED(hr)) {
                        hr = attributes->GetNamedItem(StringReferenceWrapper(L"silent").Get(),
                                                      &srcAttribute);
                        if (SUCCEEDED(hr)) {
                            hr = setNodeValueString(
                                    StringReferenceWrapper(d->m_silent ? L"true" : L"false").Get(),
                                    srcAttribute.Get());
                        }
                    }
                }
            }
        }
    }
    return hr;
}

// Set the values of each of the text nodes
HRESULT SnoreToasts::setTextValues()
{
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNodeList> nodeList;
        hr = d->m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
        if (SUCCEEDED(hr)) {
            // create the title
            ComPtr<IXmlNode> textNode;
            hr = nodeList->Item(0, &textNode);
            if (SUCCEEDED(hr)) {
                hr = setNodeValueString(StringReferenceWrapper(d->m_title).Get(), textNode.Get());
                if (SUCCEEDED(hr)) {

                    hr = nodeList->Item(1, &textNode);
                    if (SUCCEEDED(hr)) {
                        hr = setNodeValueString(StringReferenceWrapper(d->m_body).Get(),
                                                textNode.Get());
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT SnoreToasts::setButtons(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    HRESULT hr =
            d->m_toastXml->CreateElement(StringReferenceWrapper(L"actions").Get(), &actionsElement);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> actionsNodeTmp;
        hr = actionsElement.As(&actionsNodeTmp);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionsNode;
            root->AppendChild(actionsNodeTmp.Get(), &actionsNode);

            std::wstring buttonText;
            std::wstringstream wss(d->m_buttons);
            while (std::getline(wss, buttonText, L';')) {
                hr &= createNewActionButton(actionsNode, buttonText);
            }
        }
    }

    return hr;
}

HRESULT SnoreToasts::setTextBox(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    HRESULT hr =
            d->m_toastXml->CreateElement(StringReferenceWrapper(L"actions").Get(), &actionsElement);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> actionsNodeTmp;
        hr = actionsElement.As(&actionsNodeTmp);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionsNode;
            root->AppendChild(actionsNodeTmp.Get(), &actionsNode);

            ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> inputElement;
            HRESULT hr = d->m_toastXml->CreateElement(StringReferenceWrapper(L"input").Get(),
                                                      &inputElement);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> inputNodeTmp;
                hr = inputElement.As(&inputNodeTmp);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> inputNode;
                    actionsNode->AppendChild(inputNodeTmp.Get(), &inputNode);
                    ComPtr<IXmlNamedNodeMap> inputAttributes;
                    hr = inputNode->get_Attributes(&inputAttributes);
                    if (SUCCEEDED(hr)) {
                        hr &= addAttribute(L"id", inputAttributes.Get(), L"textBox");
                        hr &= addAttribute(L"type", inputAttributes.Get(), L"text");
                        hr &= addAttribute(L"placeHolderContent", inputAttributes.Get(),
                                           L"Type a reply");
                    }
                }
                ComPtr<IXmlElement> actionElement;
                HRESULT hr = d->m_toastXml->CreateElement(StringReferenceWrapper(L"action").Get(),
                                                          &actionElement);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> actionNodeTmp;
                    hr = actionElement.As(&actionNodeTmp);
                    if (SUCCEEDED(hr)) {
                        ComPtr<IXmlNode> actionNode;
                        actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode);
                        ComPtr<IXmlNamedNodeMap> actionAttributes;
                        hr = actionNode->get_Attributes(&actionAttributes);
                        if (SUCCEEDED(hr)) {
                            hr &= addAttribute(L"content", actionAttributes.Get(), L"Send");
                            const auto data =
                                    formatAction(SnoreToastActions::Actions::ButtonClicked);
                            hr &= addAttribute(L"arguments", actionAttributes.Get(), data);
                            hr &= addAttribute(L"hint-inputId", actionAttributes.Get(), L"textBox");
                        }
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT SnoreToasts::setEventHandler(ComPtr<IToastNotification> toast)
{
    // Register the event handlers
    EventRegistrationToken activatedToken, dismissedToken, failedToken;
    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(*this));

    HRESULT hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
    if (SUCCEEDED(hr)) {
        hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
        if (SUCCEEDED(hr)) {
            hr = toast->add_Failed(eventHandler.Get(), &failedToken);
            if (SUCCEEDED(hr)) {
                d->m_eventHanlder = eventHandler;
            }
        }
    }
    return hr;
}

HRESULT SnoreToasts::setNodeValueString(const HSTRING &inputString, IXmlNode *node)
{
    ComPtr<IXmlText> inputText;

    HRESULT hr = d->m_toastXml->CreateTextNode(inputString, &inputText);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> inputTextNode;

        hr = inputText.As(&inputTextNode);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> pAppendedChild;
            hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
        }
    }

    return hr;
}

HRESULT SnoreToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    HRESULT hr = d->m_toastXml->CreateAttribute(StringReferenceWrapper(name).Get(), &srcAttribute);

    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = srcAttribute.As(&node);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> pNode;
            hr = attributeMap->SetNamedItem(node.Get(), &pNode);
        }
    }
    return hr;
}

HRESULT SnoreToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap,
                                  const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    HRESULT hr = d->m_toastXml->CreateAttribute(StringReferenceWrapper(name).Get(), &srcAttribute);

    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = srcAttribute.As(&node);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> pNode;
            hr = attributeMap->SetNamedItem(node.Get(), &pNode);
            hr = setNodeValueString(StringReferenceWrapper(value).Get(), node.Get());
        }
    }
    return hr;
}

HRESULT SnoreToasts::createNewActionButton(ComPtr<IXmlNode> actionsNode, const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionElement;
    HRESULT hr =
            d->m_toastXml->CreateElement(StringReferenceWrapper(L"action").Get(), &actionElement);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> actionNodeTmp;
        hr = actionElement.As(&actionNodeTmp);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionNode;
            actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode);
            ComPtr<IXmlNamedNodeMap> actionAttributes;
            hr = actionNode->get_Attributes(&actionAttributes);
            if (SUCCEEDED(hr)) {
                hr &= addAttribute(L"content", actionAttributes.Get(), value);

                const auto data = formatAction(SnoreToastActions::Actions::ButtonClicked,
                                               { { L"button", value } });
                hr &= addAttribute(L"arguments", actionAttributes.Get(), data);
                hr &= addAttribute(L"activationType", actionAttributes.Get(), L"foreground");
            }
        }
    }

    return hr;
}

void SnoreToasts::printXML()
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer> s;
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> ss(d->m_toastXml);
    ss.As(&s);
    HSTRING string;
    s->GetXml(&string);
    PCWSTR str = WindowsGetStringRawBuffer(string, nullptr);
    tLog << L"------------------------\n\t\t\t" << str << L"\n\t\t" << L"------------------------";
}

std::filesystem::path SnoreToasts::pipeName() const
{
    return d->m_pipeName;
}

void SnoreToasts::setPipeName(const std::filesystem::path &pipeName)
{
    d->m_pipeName = pipeName;
}

std::filesystem::path SnoreToasts::application() const
{
    return d->m_application;
}

void SnoreToasts::setApplication(const std::filesystem::path &application)
{
    d->m_application = application;
}

std::wstring SnoreToasts::formatAction(
        const SnoreToastActions::Actions &action,
        const std::vector<std::pair<std::wstring_view, std::wstring_view>> &extraData) const
{
    const auto pipe = d->m_pipeName.wstring();
    const auto application = d->m_application.wstring();
    std::vector<std::pair<std::wstring_view, std::wstring_view>> data = {
        { L"action", SnoreToastActions::getActionString(action) },
        { L"notificationId", std::wstring_view(d->m_id) },
        { L"pipe", std::wstring_view(pipe) },
        { L"application", std::wstring_view(application) }
    };
    data.insert(data.end(), extraData.cbegin(), extraData.cend());
    return Utils::formatData(data);
}

// Create and display the toast
HRESULT SnoreToasts::createToast()
{
    HRESULT hr = d->m_toastManager->CreateToastNotifierWithId(
            StringReferenceWrapper(d->m_appID).Get(), &d->m_notifier);
    if (SUCCEEDED(hr)) {
        if (SUCCEEDED(hr)) {
            ComPtr<IToastNotificationFactory> factory;
            hr = GetActivationFactory(
                    StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification)
                            .Get(),
                    &factory);
            if (SUCCEEDED(hr)) {
                hr = factory->CreateToastNotification(d->m_toastXml.Get(), &d->m_notification);
                if (SUCCEEDED(hr)) {
                    if (d->m_wait) {
                        NotificationSetting setting = NotificationSetting_Enabled;
                        d->m_notifier->get_Setting(&setting);
                        if (setting == NotificationSetting_Enabled) {
                            hr = setEventHandler(d->m_notification);
                        } else {
                            std::wcerr << L"Notifications are disabled" << std::endl << L"Reason: ";
                            switch (setting) {
                            case NotificationSetting_DisabledForApplication:
                                std::wcerr << L"DisabledForApplication" << std::endl;
                                break;
                            case NotificationSetting_DisabledForUser:
                                std::wcerr << L"DisabledForUser" << std::endl;
                                break;
                            case NotificationSetting_DisabledByGroupPolicy:
                                std::wcerr << L"DisabledByGroupPolicy" << std::endl;
                                break;
                            case NotificationSetting_DisabledByManifest:
                                std::wcerr << L"DisabledByManifest" << std::endl;
                                break;
                            case NotificationSetting_Enabled:
                                // unreachable
                                break;
                            }
                            std::wcerr << L"Please make sure that the app id is set correctly."
                                       << std::endl;
                            std::wcerr << L"Command Line: " << GetCommandLineW() << std::endl;
                            hr = S_FALSE;
                        }
                    }
                    if (SUCCEEDED(hr)) {
                        hr = d->m_notifier->Show(d->m_notification.Get());
                    }
                }
            }
        }
    }
    return hr;
}

std::wstring SnoreToasts::version()
{
    static std::wstring ver = []{
        std::wstringstream st;
        st << SNORETOAST_VERSION_MAJOR << L"." << SNORETOAST_VERSION_MINOR << L"." << SNORETOAST_VERSION_PATCH;
        return st.str();
    }();
    return ver;
}

HRESULT SnoreToasts::backgroundCallback(const std::wstring &appUserModelId,
                                        const std::wstring &invokedArgs, const std::wstring &msg)
{
    tLog << "CToastNotificationActivationCallback::Activate: " << appUserModelId << " : "
         << invokedArgs << " : " << msg;
    const auto dataMap = Utils::splitData(invokedArgs);
    const auto action = SnoreToastActions::getAction(dataMap.at(L"action"));
    std::wstring dataString;
    if (action == SnoreToastActions::Actions::TextEntered) {
        std::wstringstream sMsg;
        sMsg << invokedArgs << L"text=" << msg;
        ;
        dataString = sMsg.str();
    } else {
        dataString = invokedArgs;
    }
    const auto pipe = dataMap.find(L"pipe");
    if (pipe != dataMap.cend()) {
        if (!Utils::writePipe(pipe->second, dataString)) {
            const auto app = dataMap.find(L"application");
            if (app != dataMap.cend()) {
                if (Utils::startProcess(app->second)) {
                    Utils::writePipe(pipe->second, dataString, true);
                }
            }
        }
    }

    tLog << dataString;
    if (!SetEvent(SnoreToastsPrivate::ctoastEvent())) {
        tLog << "SetEvent failed" << GetLastError();
        return S_FALSE;
    }
    return S_OK;
}
void SnoreToasts::waitForCallbackActivation()
{
    Utils::registerActivator();
    WaitForSingleObject(SnoreToastsPrivate::ctoastEvent(), INFINITE);
    Utils::unregisterActivator();
}
