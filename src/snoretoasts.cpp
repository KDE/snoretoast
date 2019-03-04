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

#include <sstream>
#include <iostream>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

SnoreToasts::SnoreToasts(const std::wstring &appID) :
    m_appID(appID),
    m_action(Success),
    m_sound(L"Notification.Default")
{
}

SnoreToasts::~SnoreToasts()
{
    LinkHelper::unregisterActivator();
}

void SnoreToasts::displayToast(const std::wstring &title, const std::wstring &body, const std::wstring &image, bool wait)
{
    HRESULT hr = S_OK;
    m_title = title;
    m_body = body;
    m_image = image;
    m_wait = wait;

    hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &m_toastManager);
    if (SUCCEEDED(hr)) {
        if (m_image.length() > 0) {
            hr = m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText02, &m_toastXml);
        } else {
            hr = m_toastManager->GetTemplateContent(ToastTemplateType_ToastText02, &m_toastXml);
        }

        if (SUCCEEDED(hr)) {
            ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> rootList;
            hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"toast").Get(), &rootList);

            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> root;
                hr = rootList->Item(0, &root);
                if (m_textbox){
                    ComPtr<IXmlNamedNodeMap> rootAttributes;

                    hr = root->get_Attributes(&rootAttributes);
                    if (SUCCEEDED(hr)) {
                        hr = addAttribute(L"launch", rootAttributes.Get(), L"action=openThread&amp;threadId=92185");
                    }
                }
                //Adding buttons
                if (!m_buttons.empty())
                {
                    setButtons(root);
                }
                else if (m_textbox)
                {
                    setTextBox(root);
                }

                if (SUCCEEDED(hr)) {

                    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> audioElement;
                    hr = m_toastXml->CreateElement(StringReferenceWrapper(L"audio").Get(), &audioElement);
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
    }
    //    printXML();

    if (SUCCEEDED(hr)) {
        if (m_image.length() > 0) {
            hr = setImage();
        }
        if (SUCCEEDED(hr)) {
            hr = setSound();
            if (SUCCEEDED(hr)) {
                hr = setTextValues();
                if (SUCCEEDED(hr)) {
                    //printXML();
                    hr = createToast();
                }
            }
        }
    }
    //printXML();
    m_action = SUCCEEDED(hr) ? Success : Failed;
}

SnoreToasts::USER_ACTION SnoreToasts::userAction()
{
    if (m_eventHanlder.Get() != NULL) {
        HANDLE event = m_eventHanlder.Get()->event();
        WaitForSingleObject(event, INFINITE);
        m_action = m_eventHanlder.Get()->userAction();
        if (m_action == SnoreToasts::Hidden) {
            m_notifier->Hide(m_notification.Get());
            std::wcerr << L"The application hid the toast using ToastNotifier.hide()" << std::endl;
        }
        CloseHandle(event);
    }
    return m_action;
}

bool SnoreToasts::closeNotification()
{
    std::wstringstream eventName;
    eventName << L"ToastEvent"
              << m_id;

    HANDLE event = OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName.str().c_str());
    if (event) {
        SetEvent(event);
        return true;
    }
    std::wcerr << "Notification " << m_id << " does not exist" << std::endl;
    return false;
}

void SnoreToasts::setSound(const std::wstring &soundFile)
{
    m_sound = soundFile;
}

void SnoreToasts::setSilent(bool silent)
{
    m_silent = silent;
}

void SnoreToasts::setId(const std::wstring &id)
{
    m_id = id;
}

void SnoreToasts::setButtons(const std::wstring &buttons)
{
    m_buttons = buttons;
}

void SnoreToasts::setTextBoxEnabled(bool textBoxEnabled)
{
    m_textbox = textBoxEnabled;
}

// Set the value of the "src" attribute of the "image" node
HRESULT SnoreToasts::setImage()
{
    HRESULT hr = S_OK;

    ComPtr<IXmlNodeList> nodeList;
    hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
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
                    hr = setNodeValueString(StringReferenceWrapper(m_image).Get(), srcAttribute.Get());
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
    hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"audio").Get(), &nodeList);
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
                    if (m_sound.find(L"ms-winsoundevent:") == std::wstring::npos) {
                        sound = L"ms-winsoundevent:";
                        sound.append(m_sound);
                    } else {
                        sound = m_sound;
                    }

                    hr = setNodeValueString(StringReferenceWrapper(sound).Get(), srcAttribute.Get());
                    if (SUCCEEDED(hr)) {
                        hr = attributes->GetNamedItem(StringReferenceWrapper(L"silent").Get(), &srcAttribute);
                        if (SUCCEEDED(hr)) {
                            hr = setNodeValueString(StringReferenceWrapper(m_silent ? L"true" : L"false").Get(), srcAttribute.Get());
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
        hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
        if (SUCCEEDED(hr)) {
            //create the title
            ComPtr<IXmlNode> textNode;
            hr = nodeList->Item(0, &textNode);
            if (SUCCEEDED(hr)) {
                hr = setNodeValueString(StringReferenceWrapper(m_title).Get(), textNode.Get());
                if (SUCCEEDED(hr)) {

                    hr = nodeList->Item(1, &textNode);
                    if (SUCCEEDED(hr)) {
                        hr = setNodeValueString(StringReferenceWrapper(m_body).Get(), textNode.Get());
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
    HRESULT hr = m_toastXml->CreateElement(StringReferenceWrapper(L"actions").Get(), &actionsElement);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> actionsNodeTmp;
        hr = actionsElement.As(&actionsNodeTmp);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionsNode;
            root->AppendChild(actionsNodeTmp.Get(), &actionsNode);

            std::wstring buttonText;
            std::wstringstream wss(m_buttons);
            while(std::getline(wss, buttonText, L';'))
            {
                hr &= createNewActionButton(actionsNode, buttonText);
            }
        }
    }

    return hr;
}

HRESULT SnoreToasts::setTextBox(ComPtr<IXmlNode> root)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> actionsElement;
    HRESULT hr = m_toastXml->CreateElement(StringReferenceWrapper(L"actions").Get(), &actionsElement);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> actionsNodeTmp;
        hr = actionsElement.As(&actionsNodeTmp);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> actionsNode;
            root->AppendChild(actionsNodeTmp.Get(), &actionsNode);

            ComPtr<ABI::Windows::Data::Xml::Dom::IXmlElement> inputElement;
            HRESULT hr = m_toastXml->CreateElement(StringReferenceWrapper(L"input").Get(), &inputElement);
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
                        hr &= addAttribute(L"placeHolderContent", inputAttributes.Get(), L"Type a reply");
                    }
                }
                ComPtr<IXmlElement> actionElement;
                HRESULT hr = m_toastXml->CreateElement(StringReferenceWrapper(L"action").Get(), &actionElement);
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
                            std::wstringstream id;
                            id << "action=reply&amp;processId=" << GetCurrentProcessId();
                            hr &= addAttribute(L"arguments", actionAttributes.Get(), id.str());
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
    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(m_id));

    HRESULT hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
    if (SUCCEEDED(hr)) {
        hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
        if (SUCCEEDED(hr)) {
            hr = toast->add_Failed(eventHandler.Get(), &failedToken);
            if (SUCCEEDED(hr)) {
                m_eventHanlder = eventHandler;
            }
        }
    }
    return hr;
}

HRESULT SnoreToasts::setNodeValueString(const HSTRING &inputString, IXmlNode *node)
{
    ComPtr<IXmlText> inputText;

    HRESULT hr = m_toastXml->CreateTextNode(inputString, &inputText);
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
    HRESULT hr = m_toastXml->CreateAttribute(StringReferenceWrapper(name).Get(), &srcAttribute);

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

HRESULT SnoreToasts::addAttribute(const std::wstring &name, IXmlNamedNodeMap *attributeMap, const std::wstring &value)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlAttribute> srcAttribute;
    HRESULT hr = m_toastXml->CreateAttribute(StringReferenceWrapper(name).Get(), &srcAttribute);

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
    HRESULT hr = m_toastXml->CreateElement(StringReferenceWrapper(L"action").Get(), &actionElement);
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
                hr &= addAttribute(L"arguments", actionAttributes.Get(), value);
                hr &= addAttribute(L"activationType", actionAttributes.Get(), L"foreground");
            }
        }
    }

    return hr;
}

void SnoreToasts::printXML()
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer> s;
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> ss(m_toastXml);
    ss.As(&s);
    HSTRING string;
    s->GetXml(&string);
    PCWSTR str = WindowsGetStringRawBuffer(string, NULL);
    std::wcerr << L"------------------------" << std::endl
               << L"SnoreToast " << version() << std::endl
               << L"------------------------" << std::endl
               << m_appID << std::endl
               << L"------------------------" << std::endl
               << str << std::endl
               << L"------------------------" << std::endl;
}

// Create and display the toast
HRESULT SnoreToasts::createToast()
{
    HRESULT hr = m_toastManager->CreateToastNotifierWithId(StringReferenceWrapper(m_appID).Get(), &m_notifier);
    if (SUCCEEDED(hr)) {
        if (SUCCEEDED(hr)) {
            ComPtr<IToastNotificationFactory> factory;
            hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory);
            if (SUCCEEDED(hr)) {
                hr = factory->CreateToastNotification(m_toastXml.Get(), &m_notification);
                if (SUCCEEDED(hr)) {
                    if (m_wait) {
                        NotificationSetting setting = NotificationSetting_Enabled;
                        m_notifier->get_Setting(&setting);
                        if (setting == NotificationSetting_Enabled) {
                            hr = setEventHandler(m_notification);
                        } else {
                            std::wcerr << L"Notifications are disabled" << std::endl
                                       << L"Reason: ";

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
                            }
                            std::wcerr << L"Please make sure that the app id is set correctly." << std::endl;
                            std::wcerr << L"Command Line: " << GetCommandLineW() << std::endl;
                        }
                    }
                    if (SUCCEEDED(hr)) {
                        hr = m_notifier->Show(m_notification.Get());
                    }
                }
            }
        }
    }
    return hr;
}

std::wstring SnoreToasts::version()
{
    return L"0.5.99";
}
