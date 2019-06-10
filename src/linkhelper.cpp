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
#include "linkhelper.h"
#include "toasteventhandler.h"
#include "utils.h"

#include <propvarutil.h>
#include <comdef.h>

#include <iostream>
#include <sstream>

// compat with older sdk
#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_AppUserModel_ToastActivatorCLSID = {
    { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 26
};
#define INIT_PKEY_AppUserModel_ToastActivatorCLSID                                                 \
    {                                                                                              \
        { 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 }, 26         \
    }
#endif //#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID

HRESULT LinkHelper::tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                      const std::filesystem::path &exePath,
                                      const std::wstring &appID, const std::wstring &callbackUUID)
{
    std::filesystem::path path = shortcutPath;
    if (path.is_relative()) {
        path = startmenuPath() / path;
    }
    // make sure the extension is set
    path.replace_extension(L".lnk");

    if (std::filesystem::exists(path)) {
        tLog << L"Path: " << path << L" already exists, skip creation of shortcut";
        return S_OK;
    }
    if (!std::filesystem::exists(path.parent_path())
        && !std::filesystem::create_directories(path.parent_path())) {
        tLog << L"Failed to create dir: " << path.parent_path();
        return S_FALSE;
    }
    return installShortcut(path, exePath, appID, callbackUUID);
}

HRESULT LinkHelper::tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                      const std::wstring &appID, const std::wstring &callbackUUID)
{
    return tryCreateShortcut(shortcutPath, Utils::selfLocate(), appID, callbackUUID);
}

// Install the shortcut
HRESULT LinkHelper::installShortcut(const std::filesystem::path &shortcutPath,
                                    const std::filesystem::path &exePath, const std::wstring &appID,
                                    const std::wstring &callbackUUID)
{
    std::wcout << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID
               << std::endl;
    tLog << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID << L" "
         << callbackUUID;
    if (!callbackUUID.empty()) {
        /**
         * Add CToastNotificationActivationCallback to registry
         * Required to use the CToastNotificationActivationCallback for buttons and textbox
         * interactions. windows.ui.notifications does not support user interaction from cpp
         */
        const std::wstring locPath = Utils::selfLocate().wstring();
        const std::wstring url = [&callbackUUID] {
            std::wstringstream url;
            url << L"SOFTWARE\\Classes\\CLSID\\" << callbackUUID << L"\\LocalServer32";
            return url.str();
        }();
        tLog << url;
        ST_RETURN_ON_ERROR(HRESULT_FROM_WIN32(
                ::RegSetKeyValueW(HKEY_CURRENT_USER, url.c_str(), nullptr, REG_SZ, locPath.c_str(),
                                  static_cast<DWORD>(locPath.size() * sizeof(wchar_t)))));
    }

    ComPtr<IShellLink> shellLink;
    ST_RETURN_ON_ERROR(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&shellLink)));
    ST_RETURN_ON_ERROR(shellLink->SetPath(exePath.c_str()));
    ST_RETURN_ON_ERROR(shellLink->SetArguments(L""));

    ComPtr<IPropertyStore> propertyStore;
    ST_RETURN_ON_ERROR(shellLink.As(&propertyStore));

    PROPVARIANT appIdPropVar;
    ST_RETURN_ON_ERROR(InitPropVariantFromString(appID.c_str(), &appIdPropVar));
    ST_RETURN_ON_ERROR(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
    PropVariantClear(&appIdPropVar);

    if (!callbackUUID.empty()) {
        GUID guid;
        ST_RETURN_ON_ERROR(CLSIDFromString(callbackUUID.c_str(), &guid));

        tLog << guid.Data1;
        PROPVARIANT toastActivatorPropVar = {};
        toastActivatorPropVar.vt = VT_CLSID;
        toastActivatorPropVar.puuid = &guid;
        ST_RETURN_ON_ERROR(propertyStore->SetValue(PKEY_AppUserModel_ToastActivatorCLSID,
                                                   toastActivatorPropVar));
    }
    ST_RETURN_ON_ERROR(propertyStore->Commit());

    ComPtr<IPersistFile> persistFile;
    ST_RETURN_ON_ERROR(shellLink.As(&persistFile));
    return persistFile->Save(shortcutPath.c_str(), true);
}

std::filesystem::path LinkHelper::startmenuPath()
{
    wchar_t buffer[MAX_PATH];
    std::wstringstream path;

    if (GetEnvironmentVariable(L"APPDATA", buffer, MAX_PATH) > 0) {
        path << buffer << L"\\Microsoft\\Windows\\Start Menu\\Programs\\";
    }
    return path.str();
}
