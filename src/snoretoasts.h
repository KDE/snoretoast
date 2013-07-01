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
    SnoreToasts(const std::wstring &title, const std::wstring &body, const std::wstring &image, bool wait);
    ~SnoreToasts();

    USER_ACTION displayToast();
    USER_ACTION userAction();

    void setShortcutPath(const std::wstring &path,const std::wstring &id, bool setupShortcut);



private:

    HRESULT initialize();


    HRESULT createToast();
    HRESULT setImageSrc();
    HRESULT SetTextValues();
    HRESULT setNodeValueString(const HSTRING onputString,
            ABI::Windows::Data::Xml::Dom::IXmlNode *node
            );

    std::wstring m_title;
    std::wstring m_body;
    std::wstring m_image;
    std::wstring m_shortcut;
    std::wstring m_appID;
    bool m_wait;
    bool m_createShortcut;
    SnoreToasts::USER_ACTION m_action;

    ABI::Windows::Data::Xml::Dom::IXmlDocument *m_toastXml;
    ABI::Windows::UI::Notifications::IToastNotificationManagerStatics *m_toastManager;



};
