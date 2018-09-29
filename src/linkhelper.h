/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2013-2015  Hannah von Reth <vonreth@kde.org>

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
#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_AppUserModel_ToastActivatorCLSID = { { 0x9F4C2855, 0x9F79, 0x4B39,{ 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 26 };
#define INIT_PKEY_AppUserModel_ToastActivatorCLSID { { 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 }, 26 }
#endif //#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID

#pragma comment(lib, "runtimeobject.lib")

class LinkHelper
{
public:
    static HRESULT tryCreateShortcut(const std::wstring &shortcutPath, const std::wstring &exePath, const std::wstring &appID);
    static HRESULT tryCreateShortcut(const std::wstring &appID);
    static HRESULT registerActivator();
    static void unregisterActivator();

private:
    static HRESULT installShortcut(const std::wstring &shortcutPath, const std::wstring &exePath, const std::wstring &appID, GUID toastGUID);
    static HRESULT mkdirs(const std::wstring &dirs);

    static std::wstring startmenuPath();

};
