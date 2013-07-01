/*
    VSD prints debugging messages of applications and their
    sub-processes to console and supports logging of their output.
    Copyright (C) 2013  Patrick von Reth <vonreth@kde.org>


    VSD is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VSD is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnarlNetworkBridge.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "snoretoasts.h"
#include "ToastEventHandler.h"
#include "linkhelper.h"

#include <sstream>
#include <iostream>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

SnoreToasts::SnoreToasts(const std::wstring &title, const std::wstring &body, const std::wstring &image, bool wait):
    m_title(title),
    m_body(body),
    m_image(image),
    m_appID(L"Snore.DesktopToasts"),
    m_wait(wait),
    m_action(Success),
    m_createShortcut(true)
{


}

SnoreToasts::~SnoreToasts()
{
    //    delete m_toastManager;
    //    delete m_toastXml;
}

HRESULT SnoreToasts::initialize()
{
    HRESULT hr = S_OK;

    if(m_createShortcut)
    {
        LinkHelper helper(m_shortcut,m_appID);
        if(m_shortcut.length()>0)
        {
            hr = helper.setPropertyForExisitingShortcut();
        }
        else
        {
            hr = helper.tryCreateShortcut();
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &m_toastManager);
        if (SUCCEEDED(hr))
        {
            hr = m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText04, &m_toastXml);
        }
    }
    return hr;
}



// Display the toast using classic COM. Note that is also possible to create and display the toast using the new C++ /ZW options (using handles,
// COM wrappers, etc.)
SnoreToasts::USER_ACTION SnoreToasts::displayToast()
{
    HRESULT hr = initialize();

    if(SUCCEEDED(hr))
    {
        hr = setImageSrc();
        if(SUCCEEDED(hr))
        {
            hr = SetTextValues();
            if(SUCCEEDED(hr))
            {

                hr = createToast();
            }
        }
    }
    return SUCCEEDED(hr)?userAction():Failed;
}

SnoreToasts::USER_ACTION SnoreToasts::userAction()
{
    return m_action;
}

void SnoreToasts::setShortcutPath(const std::wstring &path, const std::wstring &id, bool setupShortcut)
{
    m_shortcut = path;
    m_appID = id;
    m_createShortcut = setupShortcut;
}


// Set the value of the "src" attribute of the "image" node
HRESULT SnoreToasts::setImageSrc()
{
    HRESULT hr = S_OK;

    ComPtr<IXmlNodeList> nodeList;
    hr = m_toastXml->GetElementsByTagName( StringReferenceWrapper(L"image").Get(), &nodeList);
    if (SUCCEEDED(hr))
    {
        ComPtr<IXmlNode> imageNode;
        hr = nodeList->Item(0, &imageNode);
        if (SUCCEEDED(hr))
        {
            ComPtr<IXmlNamedNodeMap> attributes;

            hr = imageNode->get_Attributes(&attributes);
            if (SUCCEEDED(hr))
            {
                ComPtr<IXmlNode> srcAttribute;

                hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
                if (SUCCEEDED(hr))
                {
                    hr = setNodeValueString(StringReferenceWrapper(m_image).Get(), srcAttribute.Get());
                }
            }
        }
    }
    return hr;
}

// Set the values of each of the text nodes
HRESULT SnoreToasts::SetTextValues()
{
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        ComPtr<IXmlNodeList> nodeList;
        hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
        if (SUCCEEDED(hr))
        {
            ComPtr<IXmlNode> textNode;
            hr = nodeList->Item(0, &textNode);
            if (SUCCEEDED(hr))
            {
                hr = setNodeValueString(StringReferenceWrapper(m_title).Get(), textNode.Get());
            }

            hr = nodeList->Item(1, &textNode);
            if (SUCCEEDED(hr))
            {
                hr = setNodeValueString(StringReferenceWrapper(m_body).Get(), textNode.Get());
            }


        }
    }
    return hr;
}

HRESULT SnoreToasts::setNodeValueString(const HSTRING inputString,  IXmlNode *node)
{
    ComPtr<IXmlText> inputText;

    HRESULT hr = m_toastXml->CreateTextNode(inputString, &inputText);
    if (SUCCEEDED(hr))
    {
        ComPtr<IXmlNode> inputTextNode;

        hr = inputText.As(&inputTextNode);
        if (SUCCEEDED(hr))
        {
            ComPtr<IXmlNode> pAppendedChild;
            hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
        }
    }

    return hr;
}

// Create and display the toast
HRESULT SnoreToasts::createToast()
{
    ComPtr<IToastNotifier> notifier;
    HRESULT hr = m_toastManager->CreateToastNotifierWithId(StringReferenceWrapper(m_appID).Get(), &notifier);
    if (SUCCEEDED(hr))
    {
        ComPtr<IToastNotificationFactory> factory;
        hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory);
        if (SUCCEEDED(hr))
        {
            ComPtr<IToastNotification> toast;
            hr = factory->CreateToastNotification(m_toastXml, &toast);
            if (SUCCEEDED(hr))
            {
                // Register the event handlers
                EventRegistrationToken activatedToken, dismissedToken, failedToken;
                ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler());

                hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
                if (SUCCEEDED(hr))
                {
                    hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
                    if (SUCCEEDED(hr))
                    {
                        hr = toast->add_Failed(eventHandler.Get(), &failedToken);
                        if (SUCCEEDED(hr))
                        {
                            hr = notifier->Show(toast.Get());
                            if(SUCCEEDED(hr) && m_wait)
                            {
                                WaitForSingleObject(eventHandler.Get()->event(),INFINITE);
                                m_action = eventHandler.Get()->userAction();
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

