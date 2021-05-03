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

ToastEventHandler::ToastEventHandler(const SnoreToasts &toast)
    : m_ref(1), m_userAction(SnoreToastActions::Actions::Hidden), m_toast(toast)
{
    std::wstringstream eventName;
    eventName << L"ToastEvent" << m_toast.id();
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

SnoreToastActions::Actions &ToastEventHandler::userAction()
{
    return m_userAction;
}

// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /*sender*/,
                                         _In_ IInspectable *args)
{
    IToastActivatedEventArgs *buttonReply = nullptr;
    args->QueryInterface(&buttonReply);
    if (buttonReply == nullptr) {
        std::wcerr << L"args is not a IToastActivatedEventArgs" << std::endl;
    } else {
        HSTRING args;
        buttonReply->get_Arguments(&args);
        std::wstring data = WindowsGetStringRawBuffer(args, nullptr);
        tLog << data;
        const auto dataMap = Utils::splitData(data);
        const auto action = SnoreToastActions::getAction(dataMap.at(L"action"));
        assert(dataMap.at(L"notificationId") == m_toast.id());

        if (action == SnoreToastActions::Actions::TextEntered) {
            // The text is only passed to the named pipe
            tLog << L"The user entered a text.";
            m_userAction = SnoreToastActions::Actions::TextEntered;
        } else if (action == SnoreToastActions::Actions::Clicked) {
            tLog << L"The user clicked on the toast.";
            m_userAction = SnoreToastActions::Actions::Clicked;
        } else {
            tLog << L"The user clicked on a toast button.";
            std::wcout << dataMap.at(L"button") << std::endl;
            m_userAction = SnoreToastActions::Actions::ButtonClicked;
        }
        if (!m_toast.pipeName().empty()) {
            if (m_userAction == SnoreToastActions::Actions::ButtonClicked) {
                Utils::writePipe(m_toast.pipeName(), m_toast.formatAction(m_userAction, { { L"button", dataMap.at(L"button") } }));
            } else {
                Utils::writePipe(m_toast.pipeName(), m_toast.formatAction(m_userAction));
            }
        }
    }
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */,
                                         _In_ IToastDismissedEventArgs *e)
{
    ToastDismissalReason tdr;
    HRESULT hr = e->get_Reason(&tdr);
    if (SUCCEEDED(hr)) {
        switch (tdr) {
        case ToastDismissalReason_ApplicationHidden:
            tLog << L"The application hid the toast using ToastNotifier.hide()";
            m_userAction = SnoreToastActions::Actions::Hidden;
            break;
        case ToastDismissalReason_UserCanceled:
            tLog << L"The user dismissed this toast";
            m_userAction = SnoreToastActions::Actions::Dismissed;
            break;
        case ToastDismissalReason_TimedOut:
            tLog << L"The toast has timed out";
            m_userAction = SnoreToastActions::Actions::Timedout;
            break;
        }
    }
    if (!m_toast.pipeName().empty()) {
        Utils::writePipe(m_toast.pipeName(), m_toast.formatAction(m_userAction));
    }
    SetEvent(m_event);
    return S_OK;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification * /* sender */,
                                         _In_ IToastFailedEventArgs * /* e */)
{
    std::wcerr << L"The toast encountered an error." << std::endl;
    std::wcerr << L"Please make sure that the app id is set correctly." << std::endl;
    std::wcerr << L"Command Line: " << GetCommandLineW() << std::endl;
    m_userAction = SnoreToastActions::Actions::Error;
    SetEvent(m_event);
    return S_OK;
}
