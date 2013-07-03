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

using namespace ABI::Windows::UI::Notifications;

ToastEventHandler::ToastEventHandler() :
    m_ref(1),
    m_action(SnoreToasts::Success)
{
    SECURITY_ATTRIBUTES attributes;
    ZeroMemory(&attributes);
    attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    attributes.bInheritHandle = FALSE;
    m_event = CreateEventW(&attributes,TRUE,FALSE,L"SnoreToastEvent");
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
    return m_action;
}


// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IInspectable* /* args */)
{
    std::wcout << L"The user clicked on the toast." << std::endl;
    m_action = SnoreToasts::Success;
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IToastDismissedEventArgs* e)
{
    ToastDismissalReason tdr;
    HRESULT hr = e->get_Reason(&tdr);
    if (SUCCEEDED(hr))
    {
        switch (tdr)
        {
        case ToastDismissalReason_ApplicationHidden:
            std::wcout << L"The application hid the toast using ToastNotifier.hide()" << std::endl;
            m_action = SnoreToasts::Hidden;
            break;
        case ToastDismissalReason_UserCanceled:
            std::wcout << L"The user dismissed this toast" << std::endl;
            m_action = SnoreToasts::Dismissed;
            break;
        case ToastDismissalReason_TimedOut:
            std::wcout << L"The toast has timed out" << std::endl;
            m_action = SnoreToasts::Timeout;
            break;
        default:
            std::wcout << L"Toast not activated" << std::endl;
            m_action = SnoreToasts::Failed;
            break;
        }
    }
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IToastFailedEventArgs* /* e */)
{
    std::wcout << L"The toast encountered an error." << std::endl;
    m_action = SnoreToasts::Failed;
    SetEvent(m_event);
    return S_OK;
}
