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

        HRESULT hr = GetActivationFactory(
                StringRef(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
                &m_toastManager);
        if (!SUCCEEDED(hr)) {
            std::wcerr << L"SnoreToasts: Failed to register com Factory, please make sure you "
                          L"correctly initialised with RO_INIT_MULTITHREADED"
                       << std::endl;
            m_action = SnoreToastActions::Actions::Error;
        }
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
    ComPtr<IToastNotificationManagerStatics> m_toastManager;
    ComPtr<IToastNotifier> m_notifier;
    ComPtr<IToastNotification> m_notification;

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

    ComPtr<IToastNotificationHistory> getHistory()
    {
        ComPtr<IToastNotificationManagerStatics2> toastStatics2;
        if (Utils::checkResult(m_toastManager.As(&toastStatics2))) {
            ComPtr<IToastNotificationHistory> nativeHistory;
            Utils::checkResult(toastStatics2->get_History(&nativeHistory));
            return nativeHistory;
        }
        return {};
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

HRESULT SnoreToasts::displayToast(const std::wstring &title, const std::wstring &body,
                                  const std::filesystem::path &image, bool wait)
{
    // asume that we fail
    d->m_action = SnoreToastActions::Actions::Error;

    d->m_title = title;
    d->m_body = body;
    d->m_image = image;
    d->m_wait = wait;

    if (!d->m_image.empty()) {
        ReturnOnErrorHr(d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText02,
                                                              &d->m_toastXml));
    } else {
        ReturnOnErrorHr(d->m_toastManager->GetTemplateContent(ToastTemplateType_ToastText02,
                                                              &d->m_toastXml));
    }
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> rootList;
    ReturnOnErrorHr(d->m_toastXml->GetElementsByTagName(StringRef(L"toast").Get(), &rootList));

    ComPtr<IXmlNode> root;
    ReturnOnErrorHr(rootList->Item(0, &root));
    ComPtr<IXmlNamedNodeMap> rootAttributes;
    ReturnOnErrorHr(root->get_Attributes(&rootAttributes));

    const auto data = formatAction(SnoreToastActions::Actions::Clicked);
    ReturnOnErrorHr(addAttribute(L"launch", rootAttributes.Get(), data));
    // Adding buttons
    if (!d->m_buttons.empty()) {
        setButtons(root);
    } else if (d->m_textbox) {
        setTextBox(root);
    }
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> audioElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"audio").Get(), &audioElement));

    ComPtr<IXmlNode> audioNodeTmp;
    ReturnOnErrorHr(audioElement.As(&audioNodeTmp));

    ComPtr<IXmlNode> audioNode;
    ReturnOnErrorHr(root->AppendChild(audioNodeTmp.Get(), &audioNode));

    ComPtr<IXmlNamedNodeMap> attributes;
    ReturnOnErrorHr(audioNode->get_Attributes(&attributes));
    ReturnOnErrorHr(addAttribute(L"src", attributes.Get()));
    ReturnOnErrorHr(addAttribute(L"silent", attributes.Get()));
    //    printXML();

    if (!d->m_image.empty()) {
        ReturnOnErrorHr(setImage());
    }
    ReturnOnErrorHr(setSound());

    ReturnOnErrorHr(setTextValues());

    printXML();
    ReturnOnErrorHr(createToast());
    d->m_action = SnoreToastActions::Actions::Clicked;
    return S_OK;
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

    if (auto history = d->getHistory()) {
        if (Utils::checkResult(history->RemoveGroupedTagWithId(StringRef(d->m_id).Get(),
                                                               StringRef(L"SnoreToast").Get(),
                                                               StringRef(d->m_appID).Get()))) {
            return true;
        }
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
    ComPtr<IXmlNodeList> nodeList;
    ReturnOnErrorHr(d->m_toastXml->GetElementsByTagName(StringRef(L"image").Get(), &nodeList));

    ComPtr<IXmlNode> imageNode;
    ReturnOnErrorHr(nodeList->Item(0, &imageNode));

    ComPtr<IXmlNamedNodeMap> attributes;
    ReturnOnErrorHr(imageNode->get_Attributes(&attributes));

    ComPtr<IXmlNode> srcAttribute;
    ReturnOnErrorHr(attributes->GetNamedItem(StringRef(L"src").Get(), &srcAttribute));
    return setNodeValueString(StringRef(d->m_image).Get(), srcAttribute.Get());
}

HRESULT SnoreToasts::setSound()
{
    ComPtr<IXmlNodeList> nodeList;
    ReturnOnErrorHr(d->m_toastXml->GetElementsByTagName(StringRef(L"audio").Get(), &nodeList));

    ComPtr<IXmlNode> audioNode;
    ReturnOnErrorHr(nodeList->Item(0, &audioNode));

    ComPtr<IXmlNamedNodeMap> attributes;

    ReturnOnErrorHr(audioNode->get_Attributes(&attributes));
    ComPtr<IXmlNode> srcAttribute;

    ReturnOnErrorHr(attributes->GetNamedItem(StringRef(L"src").Get(), &srcAttribute));
    std::wstring sound;
    if (d->m_sound.find(L"ms-winsoundevent:") == std::wstring::npos) {
        sound = L"ms-winsoundevent:";
        sound.append(d->m_sound);
    } else {
        sound = d->m_sound;
    }

    ReturnOnErrorHr(setNodeValueString(StringRef(sound).Get(), srcAttribute.Get()));
    ReturnOnErrorHr(attributes->GetNamedItem(StringRef(L"silent").Get(), &srcAttribute));
    return setNodeValueString(StringRef(d->m_silent ? L"true" : L"false").Get(),
                              srcAttribute.Get());
}

// Set the values of each of the text nodes
HRESULT SnoreToasts::setTextValues()
{
    ComPtr<IXmlNodeList> nodeList;
    ReturnOnErrorHr(d->m_toastXml->GetElementsByTagName(StringRef(L"text").Get(), &nodeList));
    // create the title
    ComPtr<IXmlNode> textNode;
    ReturnOnErrorHr(nodeList->Item(0, &textNode));
    ReturnOnErrorHr(setNodeValueString(StringRef(d->m_title).Get(), textNode.Get()));
    ReturnOnErrorHr(nodeList->Item(1, &textNode));
    return setNodeValueString(StringRef(d->m_body).Get(), textNode.Get());
}

HRESULT SnoreToasts::setButtons(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"actions").Get(), &actionsElement));

    ComPtr<IXmlNode> actionsNodeTmp;
    ReturnOnErrorHr(actionsElement.As(&actionsNodeTmp));

    ComPtr<IXmlNode> actionsNode;
    ReturnOnErrorHr(root->AppendChild(actionsNodeTmp.Get(), &actionsNode));

    std::wstring buttonText;
    std::wstringstream wss(d->m_buttons);
    while (std::getline(wss, buttonText, L';')) {
        ReturnOnErrorHr(createNewActionButton(actionsNode, buttonText));
    }
    return S_OK;
}

HRESULT SnoreToasts::setTextBox(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"actions").Get(), &actionsElement));

    ComPtr<IXmlNode> actionsNodeTmp;
    ReturnOnErrorHr(actionsElement.As(&actionsNodeTmp));

    ComPtr<IXmlNode> actionsNode;
    ReturnOnErrorHr(root->AppendChild(actionsNodeTmp.Get(), &actionsNode));

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> inputElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"input").Get(), &inputElement));

    ComPtr<IXmlNode> inputNodeTmp;
    ReturnOnErrorHr(inputElement.As(&inputNodeTmp));

    ComPtr<IXmlNode> inputNode;
    ReturnOnErrorHr(actionsNode->AppendChild(inputNodeTmp.Get(), &inputNode));

    ComPtr<IXmlNamedNodeMap> inputAttributes;
    ReturnOnErrorHr(inputNode->get_Attributes(&inputAttributes));

    ReturnOnErrorHr(addAttribute(L"id", inputAttributes.Get(), L"textBox"));
    ReturnOnErrorHr(addAttribute(L"type", inputAttributes.Get(), L"text"));
    ReturnOnErrorHr(addAttribute(L"placeHolderContent", inputAttributes.Get(), L"Type a reply"));

    ComPtr<IXmlElement> actionElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"action").Get(), &actionElement));

    ComPtr<IXmlNode> actionNodeTmp;
    ReturnOnErrorHr(actionElement.As(&actionNodeTmp));

    ComPtr<IXmlNode> actionNode;
    ReturnOnErrorHr(actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode));

    ComPtr<IXmlNamedNodeMap> actionAttributes;
    ReturnOnErrorHr(actionNode->get_Attributes(&actionAttributes));

    ReturnOnErrorHr(addAttribute(L"content", actionAttributes.Get(), L"Send"));

    const auto data = formatAction(SnoreToastActions::Actions::ButtonClicked);

    ReturnOnErrorHr(addAttribute(L"arguments", actionAttributes.Get(), data));
    return addAttribute(L"hint-inputId", actionAttributes.Get(), L"textBox");
}

HRESULT SnoreToasts::setEventHandler(ComPtr<IToastNotification> toast)
{
    // Register the event handlers
    EventRegistrationToken activatedToken, dismissedToken, failedToken;
    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(*this));

    ReturnOnErrorHr(toast->add_Activated(eventHandler.Get(), &activatedToken));
    ReturnOnErrorHr(toast->add_Dismissed(eventHandler.Get(), &dismissedToken));
    ReturnOnErrorHr(toast->add_Failed(eventHandler.Get(), &failedToken));
    d->m_eventHanlder = eventHandler;
    return S_OK;
}

HRESULT SnoreToasts::setNodeValueString(const HSTRING &inputString, IXmlNode *node)
{
    ComPtr<IXmlText> inputText;
    ReturnOnErrorHr(d->m_toastXml->CreateTextNode(inputString, &inputText));

    ComPtr<IXmlNode> inputTextNode;
    ReturnOnErrorHr(inputText.As(&inputTextNode));

    ComPtr<IXmlNode> pAppendedChild;
    return node->AppendChild(inputTextNode.Get(), &pAppendedChild);
}

HRESULT SnoreToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    HRESULT hr = d->m_toastXml->CreateAttribute(StringRef(name).Get(), &srcAttribute);

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
    ReturnOnErrorHr(d->m_toastXml->CreateAttribute(StringRef(name).Get(), &srcAttribute));

    ComPtr<IXmlNode> node;
    ReturnOnErrorHr(srcAttribute.As(&node));

    ComPtr<IXmlNode> pNode;
    ReturnOnErrorHr(attributeMap->SetNamedItem(node.Get(), &pNode));
    return setNodeValueString(StringRef(value).Get(), node.Get());
}

HRESULT SnoreToasts::createNewActionButton(ComPtr<IXmlNode> actionsNode, const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionElement;
    ReturnOnErrorHr(d->m_toastXml->CreateElement(StringRef(L"action").Get(), &actionElement));
    ComPtr<IXmlNode> actionNodeTmp;
    ReturnOnErrorHr(actionElement.As(&actionNodeTmp));

    ComPtr<IXmlNode> actionNode;
    ReturnOnErrorHr(actionsNode->AppendChild(actionNodeTmp.Get(), &actionNode));

    ComPtr<IXmlNamedNodeMap> actionAttributes;
    ReturnOnErrorHr(actionNode->get_Attributes(&actionAttributes));

    ReturnOnErrorHr(addAttribute(L"content", actionAttributes.Get(), value));

    const auto data =
            formatAction(SnoreToastActions::Actions::ButtonClicked, { { L"button", value } });
    ReturnOnErrorHr(addAttribute(L"arguments", actionAttributes.Get(), data));
    return addAttribute(L"activationType", actionAttributes.Get(), L"foreground");
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
    ReturnOnErrorHr(d->m_toastManager->CreateToastNotifierWithId(
            HStringReference(d->m_appID.data()).Get(), &d->m_notifier));

    ComPtr<IToastNotificationFactory> factory;
    ReturnOnErrorHr(GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
            &factory));
    ReturnOnErrorHr(factory->CreateToastNotification(d->m_toastXml.Get(), &d->m_notification));

    ComPtr<Notifications::IToastNotification2> toastV2;
    if (SUCCEEDED(d->m_notification.As(&toastV2))) {
        ReturnOnErrorHr(toastV2->put_Tag(HStringReference(d->m_id.data()).Get()));
        ReturnOnErrorHr(toastV2->put_Group(HStringReference(L"SnoreToast").Get()));
    }

    std::wstring error;
    NotificationSetting setting;
    ReturnOnErrorHr(d->m_notifier->get_Setting(&setting));

    switch (setting) {
    case NotificationSetting_DisabledForApplication:
        error = L"DisabledForApplication";
        break;
    case NotificationSetting_DisabledForUser:
        error = L"DisabledForUser";
        break;
    case NotificationSetting_DisabledByGroupPolicy:
        error = L"DisabledByGroupPolicy";
        break;
    case NotificationSetting_DisabledByManifest:
        error = L"DisabledByManifest";
        break;
    case NotificationSetting_Enabled:
        if (d->m_wait) {
            ReturnOnErrorHr(setEventHandler(d->m_notification));
        }
        break;
    }
    if (!error.empty()) {
        std::wstringstream err;
        err << L"Notifications are disabled\n"
            << L"Reason: " << error << L"Please make sure that the app id is set correctly.\n"
            << L"Command Line: " << GetCommandLineW();
        tLog << err.str();
        std::wcerr << err.str() << std::endl;
        return S_FALSE;
    }
    return d->m_notifier->Show(d->m_notification.Get());
}

std::wstring SnoreToasts::version()
{
    static std::wstring ver = [] {
        std::wstringstream st;
        st << SNORETOAST_VERSION_MAJOR << L"." << SNORETOAST_VERSION_MINOR << L"."
           << SNORETOAST_VERSION_PATCH;
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
