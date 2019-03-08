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
#include "utils.h"

#include <sstream>
#include <iostream>
#include <wchar.h>
#include <algorithm>
#include <assert.h>

using namespace ABI::Windows::UI::Notifications;

ToastEventHandler::ToastEventHandler(const std::wstring &id) :
    m_ref(1),
    m_userAction(SnoreToasts::Hidden)
{
    std::wstringstream eventName;
    eventName << L"ToastEvent" << id;
    m_event = CreateEventW(nullptr, true, false, eventName.str().c_str());
}

ToastEventHandler::~ToastEventHandler()
{
    CloseHandle(m_event);
}

HANDLE ToastEventHandler::event()
{
    return m_event;
}

SnoreToasts::USER_ACTION &ToastEventHandler::userAction()
{
    return m_userAction;
}

// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /*sender*/, _In_ IInspectable *args)
{
    IToastActivatedEventArgs *buttonReply = nullptr;
    args->QueryInterface(&buttonReply);
    if (buttonReply == nullptr)
    {
        std::wcerr << L"args is not a IToastActivatedEventArgs" << std::endl;
    }
    else
    {
        HSTRING args;
        buttonReply->get_Arguments(&args);
        PCWSTR _str = WindowsGetStringRawBuffer(args, nullptr);
        const std::wstring str = _str;

        const auto data = Utils::splitData(CToastNotificationActivationCallback::data());
        const auto action = data.at(L"action");

        if (action == Actions::Reply)
        {
            tLog << L"The user entered a text.";
            std::wcout << data.at(L"text") << std::endl;
            m_userAction = SnoreToasts::TextEntered;
        }
        else if (action == Actions::Clicked)
        {
            tLog << L"The user clicked on the toast.";
            m_userAction = SnoreToasts::Success;
        }
        else
        {
            tLog << L"The user clicked on a toast button.";
            std::wcout << str << std::endl;
            m_userAction = SnoreToasts::ButtonPressed;
        }
    }
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */, _In_ IToastDismissedEventArgs *e)
{
    ToastDismissalReason tdr;
    HRESULT hr = e->get_Reason(&tdr);
    if (SUCCEEDED(hr)) {
        switch (tdr) {
        case ToastDismissalReason_ApplicationHidden:
            tLog << L"The application hid the toast using ToastNotifier.hide()";
            m_userAction = SnoreToasts::Hidden;
            break;
        case ToastDismissalReason_UserCanceled:
            tLog << L"The user dismissed this toast";
            m_userAction = SnoreToasts::Dismissed;
            break;
        case ToastDismissalReason_TimedOut:
            tLog << L"The toast has timed out";
            m_userAction = SnoreToasts::TimedOut;
            break;
        }
    }
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */, _In_ IToastFailedEventArgs * /* e */)
{
    std::wcerr << L"The toast encountered an error." << std::endl;
    std::wcerr << L"Please make sure that the app id is set correctly." << std::endl;
    std::wcerr << L"Command Line: " << GetCommandLineW() << std::endl;
    m_userAction = SnoreToasts::Failed;
    SetEvent(m_event);
    return S_OK;
}

CToastNotificationActivationCallback::CToastNotificationActivationCallback()
{
}

HANDLE CToastNotificationActivationCallback::m_event = INVALID_HANDLE_VALUE;
std::wstring CToastNotificationActivationCallback::m_data = {};

HRESULT CToastNotificationActivationCallback::Activate(LPCWSTR appUserModelId, LPCWSTR invokedArgs,
                                                       const NOTIFICATION_USER_INPUT_DATA* data, ULONG count)
{
    if (invokedArgs == nullptr)
    {
        return S_OK;
    }
    tLog << "CToastNotificationActivationCallback::Activate: " << appUserModelId << " : " << invokedArgs << " : " << data;
    const auto dataMap = Utils::splitData(invokedArgs);
    if (dataMap.at(L"action") == Actions::Reply)
    {
        assert(count);
        std::wstringstream sMsg;
        sMsg << invokedArgs << L"text=";
        for (ULONG i=0; i<count; ++i)
        {
            std::wstring tmp = data[i].Value;
            // printing \r to stdcout is kind of problematic :D
            std::replace(tmp.begin(), tmp.end(), L'\r', L'\n');
            sMsg << tmp;
        }
        m_data = sMsg.str();
    }
    else
    {
        m_data = invokedArgs;
    }

    const auto pipe = dataMap.find(L"pipe");
    if (pipe != dataMap.cend())
    {
        Utils::writePipe(pipe->second, m_data);
    }

    tLog << m_data;
    if (m_event != INVALID_HANDLE_VALUE)
    {
        SetEvent(m_event);
    }
    return S_OK;
}

std::wstring CToastNotificationActivationCallback::waitForActivation()
{
    if (m_event == INVALID_HANDLE_VALUE)
    {
        std::wstringstream eventName;
        eventName << L"ToastActivationEvent" << GetCurrentProcessId();
        m_event = CreateEventW(nullptr, true, false, eventName.str().c_str());
    }
    else
    {
        ResetEvent(m_event);
    }
    WaitForSingleObject(m_event, INFINITE);
    return m_data;
}


std::wstring CToastNotificationActivationCallback::data()
{
    return m_data;
}
