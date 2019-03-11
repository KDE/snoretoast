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
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY PKEY_AppUserModel_ToastActivatorCLSID = { { 0x9F4C2855, 0x9F79, 0x4B39,{ 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 26 };
#define INIT_PKEY_AppUserModel_ToastActivatorCLSID { { 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 }, 26 }
#endif //#ifndef INIT_PKEY_AppUserModel_ToastActivatorCLSID

HRESULT LinkHelper::tryCreateShortcut(const std::filesystem::path &shortcutPath, const std::filesystem::path &exePath, const std::wstring &appID)
{
    if (!std::filesystem::path(shortcutPath).is_relative())
    {
        std::wcerr << L"The shortcut path must be relative" << std::endl;
        return S_FALSE;
    }
	const std::filesystem::path path = (startmenuPath() / L"SnoreToast" / SnoreToasts::version() / shortcutPath).replace_extension(L".lnk");

	if (std::filesystem::exists(path))
    {
        tLog << L"Path: " << path << L" already exists, skip creation of shortcut";
        return S_OK;
    }
    if (!std::filesystem::exists(path.parent_path()) && !std::filesystem::create_directories(path.parent_path()))
    {
        tLog << L"Failed to create dir: " << path.parent_path();
        return S_FALSE;
    }
    return installShortcut(path, exePath, appID);
}

HRESULT LinkHelper::tryCreateShortcut(const std::wstring &appID)
{
    return tryCreateShortcut(L"SnoreToast", Utils::selfLocate().c_str(), appID);
}

// Install the shortcut
HRESULT LinkHelper::installShortcut(const std::filesystem::path &shortcutPath, const std::filesystem::path &exePath, const std::wstring &appID)
{
    std::wcout << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID << std::endl;
    tLog << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID;
    /**
     * Add CToastNotificationActivationCallback to registry
     * Required to use the CToastNotificationActivationCallback for buttons and textbox interactions.
     * windows.ui.notifications does not support user interaction from cpp
     */;
	const std::wstring locPath = Utils::selfLocate().wstring();
    HRESULT hr = HRESULT_FROM_WIN32(::RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\" TOAST_UUID  L"\\LocalServer32", nullptr, REG_SZ, locPath.c_str(), static_cast<DWORD>(locPath.size() * sizeof(wchar_t))));

    if (SUCCEEDED(hr)) {
        ComPtr<IShellLink> shellLink;
        hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

        if (SUCCEEDED(hr)) {
            hr = shellLink->SetPath(exePath.c_str());
            if (SUCCEEDED(hr)) {
                hr = shellLink->SetArguments(L"");
                if (SUCCEEDED(hr)) {
                    ComPtr<IPropertyStore> propertyStore;

                    hr = shellLink.As(&propertyStore);
                    if (SUCCEEDED(hr)) {
                        PROPVARIANT appIdPropVar;
                        hr = InitPropVariantFromString(appID.c_str(), &appIdPropVar);
                        if (SUCCEEDED(hr)) {
                            hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                            if (SUCCEEDED(hr)) {
                                PropVariantClear(&appIdPropVar);
                                PROPVARIANT toastActivatorPropVar;
                                toastActivatorPropVar.vt = VT_CLSID;
                                toastActivatorPropVar.puuid = const_cast<CLSID*>(&__uuidof(CToastNotificationActivationCallback));

                                hr = propertyStore->SetValue(PKEY_AppUserModel_ToastActivatorCLSID, toastActivatorPropVar);
                                if (SUCCEEDED(hr)) {
                                    hr = propertyStore->Commit();
                                    if (SUCCEEDED(hr)) {
                                        ComPtr<IPersistFile> persistFile;
                                        hr = shellLink.As(&persistFile);
                                        if (SUCCEEDED(hr)) {
                                            hr = persistFile->Save(shortcutPath.c_str(), TRUE);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (FAILED(hr)) {
        std::wcerr << "Failed to install shortcut " << shortcutPath << "  error: " << _com_error(hr).ErrorMessage() << std::endl;
    }
    return hr;
}

std::filesystem::path LinkHelper::startmenuPath()
{
    wchar_t buffer[MAX_PATH];
    std::wstringstream path;

    if (GetEnvironmentVariable(L"APPDATA", buffer , MAX_PATH) > 0) {
        path << buffer
             << L"\\Microsoft\\Windows\\Start Menu\\Programs\\";

    }
    return path.str();
}

