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

#include <sstream>
#include <iostream>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

SnoreToasts::SnoreToasts(const std::wstring &appID):
    m_appID(appID),
    m_action(Success)
{
}

SnoreToasts::~SnoreToasts()
{
    m_toastManager->Release();
    m_toastXml->Release();
}



HRESULT SnoreToasts::displayToast(const std::wstring &title, const std::wstring &body, const std::wstring &image, bool wait)
{
    HRESULT hr = S_OK;
    m_title = title;
    m_body = body;
    m_image = image;
    m_wait = wait;

    hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &m_toastManager);
    if (SUCCEEDED(hr))
    {
        if(m_image.length()>0)
        {
            hr = m_toastManager->GetTemplateContent(ToastTemplateType_ToastImageAndText04, &m_toastXml);
        }
        else
        {
            hr = m_toastManager->GetTemplateContent(ToastTemplateType_ToastText04, &m_toastXml);
        }
    }

    if(SUCCEEDED(hr))
    {
        if(m_image.length()>0)
        {
            hr = setImageSrc();
        }
        if(SUCCEEDED(hr))
        {
            hr = setTextValues();
            if(SUCCEEDED(hr))
            {

                hr = createToast();
            }
        }
    }
    return SUCCEEDED(hr);
}

SnoreToasts::USER_ACTION SnoreToasts::userAction()
{
    if(m_eventHanlder.Get() != NULL)
    {
        WaitForSingleObject(m_eventHanlder.Get()->event(),INFINITE);
        m_action = m_eventHanlder.Get()->userAction();
    }
    return m_action;
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
HRESULT SnoreToasts::setTextValues()
{
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        ComPtr<IXmlNodeList> nodeList;
        hr = m_toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
        if (SUCCEEDED(hr))
        {
            //create the title
            ComPtr<IXmlNode> textNode;
            hr = nodeList->Item(0, &textNode);
            if (SUCCEEDED(hr))
            {
                hr = setNodeValueString(StringReferenceWrapper(m_title).Get(), textNode.Get());
                if (SUCCEEDED(hr))
                {

                    std::wstring lineOne;
                    std::wstring lineTwo;
                    size_t maxlength = 30;
                    if(m_body.length()>maxlength)
                    {
                        size_t pos = m_body.rfind(L" ",maxlength);
                        lineOne = m_body.substr(0,pos);
                        lineTwo = m_body.substr(pos+1);
                    }
                    else
                    {
                        lineOne = m_body;
                    }

                    hr = nodeList->Item(1, &textNode);
                    if (SUCCEEDED(hr))
                    {
                        hr = setNodeValueString(StringReferenceWrapper(lineOne).Get(), textNode.Get());
                        if (SUCCEEDED(hr) && lineTwo.length()>0)
                        {
                            hr = nodeList->Item(2, &textNode);
                            if (SUCCEEDED(hr))
                            {
                                hr = setNodeValueString(StringReferenceWrapper(lineTwo).Get(), textNode.Get());
                            }
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
    ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler());

    HRESULT hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
    if (SUCCEEDED(hr))
    {
        hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
        if (SUCCEEDED(hr))
        {
            hr = toast->add_Failed(eventHandler.Get(), &failedToken);
            if (SUCCEEDED(hr))
            {
                m_eventHanlder = eventHandler;
            }
        }
    }
    return hr;
}

HRESULT SnoreToasts::setNodeValueString(const HSTRING &inputString,  IXmlNode *node)
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
                if(m_wait)
                {
                    hr = setEventHandler(toast);
                }
                if (SUCCEEDED(hr))
                {
                    hr = notifier->Show(toast.Get());
                }
            }
        }
    }
    return hr;
}

