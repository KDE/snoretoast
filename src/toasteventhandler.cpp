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

#include <sstream>
#include <iostream>
#include <wchar.h>
#include <algorithm>

using namespace ABI::Windows::UI::Notifications;

ToastEventHandler::ToastEventHandler(const std::wstring &id) :
    m_ref(1),
    m_userAction(SnoreToasts::Hidden)
{
    std::wstringstream eventName;
    eventName << L"ToastEvent";
    if (!id.empty()) {
        eventName << id;
    } else {
        eventName << GetCurrentProcessId();
    }
    m_event = CreateEventW(NULL, TRUE, FALSE, eventName.str().c_str());
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
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification *  sender, _In_ IInspectable * args)
{
    IToastActivatedEventArgs *buttonReply = nullptr;
    args->QueryInterface(&buttonReply);
    if (buttonReply == nullptr)
    {
        std::wcerr << L"args is not a IToastActivatedEventArgs" << std::endl;
        std::wcerr << L"The user clicked on the toast." << std::endl;
    }
    else
    {
        HSTRING args;
        buttonReply->get_Arguments(&args);
        PCWSTR str = WindowsGetStringRawBuffer(args, NULL);

        std::wcerr << L"The user clicked on a toast button." << std::endl;
        if (wcscmp(str, L"action=reply&amp;processId=") < 0)
        {
            std::wcout << str << std::endl;
            m_userAction = SnoreToasts::TextEntered;
        }
        else
        {
            std::wcerr << str << std::endl;
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
            std::wcerr << L"The application hid the toast using ToastNotifier.hide()" << std::endl;
            m_userAction = SnoreToasts::Hidden;
            break;
        case ToastDismissalReason_UserCanceled:
            std::wcerr << L"The user dismissed this toast" << std::endl;
            m_userAction = SnoreToasts::Dismissed;
            break;
        case ToastDismissalReason_TimedOut:
            std::wcerr << L"The toast has timed out" << std::endl;
            m_userAction = SnoreToasts::TimedOut;
            break;
        default:
            std::wcerr << L"Toast not activated" << std::endl;
            m_userAction = SnoreToasts::Failed;
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

HRESULT CToastNotificationActivationCallback::Activate(LPCWSTR appUserModelId, LPCWSTR invokedArgs,
                                                       const NOTIFICATION_USER_INPUT_DATA* data, ULONG count)
{
    if (invokedArgs == nullptr)
    {
        return S_OK;
    }
    std::wstringstream sMsg;
    std::wcerr << "CToastNotificationActivationCallback::Activate: " << appUserModelId << " : " << invokedArgs << " : " << data << std::endl;
    std::wcerr << "CurrentProcess: " << GetCurrentProcessId() << std::endl;
    if (count)
    {
      for (ULONG i=0; i<count; ++i)
      {
        std::wstring tmp = data[i].Value;
        // printing \r to stdcout is kind of problematic :D
        std::replace(tmp.begin(), tmp.end(), '\r', '\n');
        sMsg << tmp;
      }
      std::wcout << sMsg.str() << std::endl;
    }
    return S_OK;
}
