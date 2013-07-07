/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2013  Patrick von Reth <vonreth@kde.org>


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

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <SDKDDKVer.h>

// Windows Header Files:
#include <Windows.h>
#include <sal.h>
#include <Psapi.h>
#include <strsafe.h>
#include <ObjBase.h>
#include <ShObjIdl.h>
#include <functiondiscoverykeys.h>
#include <guiddef.h>
#include <ShlGuid.h>



#include <wrl\client.h>
#include <wrl\implements.h>
#include <windows.ui.notifications.h>

#include "StringReferenceWrapper.h"

#include <string>

class ToastEventHandler;

class SnoreToasts
{
public:

    enum USER_ACTION
    {
        Failed = -1,
        Success,
        Hidden,
        Dismissed,
        Timeout
    };
    SnoreToasts(const std::wstring &appID);
    ~SnoreToasts();

    void displayToast(const std::wstring &title, const std::wstring &body, const std::wstring &image, bool wait);
    USER_ACTION userAction();

    void setSound(const std::wstring &soundFile);
    void setSilent(bool silent);




private:
    HRESULT createToast();
    HRESULT setImage();
    HRESULT setSound();
    HRESULT setTextValues();
    HRESULT setEventHandler(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast);
    HRESULT setNodeValueString(const HSTRING &onputString, ABI::Windows::Data::Xml::Dom::IXmlNode *node );
    HRESULT addAttribute(const std::wstring &name,ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap *attributeMap);

	void printXML();


    std::wstring m_appID;

    std::wstring m_title;
    std::wstring m_body;
    std::wstring m_image;
    std::wstring m_sound;
    bool m_silent;
    bool m_wait;

    SnoreToasts::USER_ACTION m_action;

    ABI::Windows::Data::Xml::Dom::IXmlDocument *m_toastXml;
    ABI::Windows::UI::Notifications::IToastNotificationManagerStatics *m_toastManager;

    Microsoft::WRL::ComPtr<ToastEventHandler> m_eventHanlder;



};
