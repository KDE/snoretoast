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
#pragma once
#include "snoretoasts.h"

typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *>
        DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *,
        ABI::Windows::UI::Notifications::ToastDismissedEventArgs *>
        DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification *,
        ABI::Windows::UI::Notifications::ToastFailedEventArgs *>
        DesktopToastFailedEventHandler;

class ToastEventHandler : public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler,
                                                            DesktopToastDismissedEventHandler,
                                                            DesktopToastFailedEventHandler>
{

public:
    explicit ToastEventHandler::ToastEventHandler(const SnoreToasts &toast);
    ~ToastEventHandler();

    HANDLE event();
    SnoreToastActions::Actions &userAction();

    // DesktopToastActivatedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ IInspectable *args);

    // DesktopToastDismissedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e);

    // DesktopToastFailedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender,
                          _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e);

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_ref); }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG l = InterlockedDecrement(&m_ref);
        if (l == 0) {
            delete this;
        }
        return l;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<IUnknown *>(static_cast<DesktopToastActivatedEventHandler *>(this));
        } else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler))) {
            *ppv = static_cast<DesktopToastActivatedEventHandler *>(this);
        } else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler))) {
            *ppv = static_cast<DesktopToastDismissedEventHandler *>(this);
        } else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler))) {
            *ppv = static_cast<DesktopToastFailedEventHandler *>(this);
        } else {
            *ppv = nullptr;
        }

        if (*ppv) {
            reinterpret_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    ULONG m_ref;
    SnoreToastActions::Actions m_userAction;
    HANDLE m_event;
    const SnoreToasts &m_toast;
};
