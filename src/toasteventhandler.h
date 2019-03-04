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
#include "wrl.h"

#define TOAST_UUID "383803B6-AFDA-4220-BFC3-0DBF810106BF"

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

//Define INotificationActivationCallback for older versions of the Windows SDK
#include <ntverp.h>

typedef struct NOTIFICATION_USER_INPUT_DATA
{
  LPCWSTR Key;
  LPCWSTR Value;
}  NOTIFICATION_USER_INPUT_DATA;

MIDL_INTERFACE("53E31837-6600-4A81-9395-75CFFE746F94")
INotificationActivationCallback : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Activate(__RPC__in_string LPCWSTR appUserModelId, __RPC__in_opt_string LPCWSTR invokedArgs,
                                               __RPC__in_ecount_full_opt(count) const NOTIFICATION_USER_INPUT_DATA *data, ULONG count) = 0;
};


//The COM server which implements the callback notifcation from Action Center
class DECLSPEC_UUID(TOAST_UUID)
  CToastNotificationActivationCallback : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, INotificationActivationCallback>
{
public:
//Constructors / Destructors
  CToastNotificationActivationCallback()
  {
  }

  virtual HRESULT STDMETHODCALLTYPE Activate(__RPC__in_string LPCWSTR appUserModelId, __RPC__in_opt_string LPCWSTR invokedArgs,
                                             __RPC__in_ecount_full_opt(count) const NOTIFICATION_USER_INPUT_DATA* data, ULONG count) override;
};

CoCreatableClass(CToastNotificationActivationCallback);

class ToastEventHandler :
    public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{

public:
    ToastEventHandler::ToastEventHandler(const std::wstring &id);
    ~ToastEventHandler();

    HANDLE event();
    SnoreToasts::USER_ACTION &userAction();

    // DesktopToastActivatedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable *args);

    // DesktopToastDismissedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e);

    // DesktopToastFailedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e);

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_ref);
    }

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
    SnoreToasts::USER_ACTION m_userAction;
    HANDLE m_event;
};
