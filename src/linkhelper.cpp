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
#include "linkhelper.h"
#include "toasteventhandler.h"

#include <propvarutil.h>
#include <comdef.h>

#include <iostream>
#include <sstream>
#include <regex>
#include <corecrt_wstring.h>

using namespace Microsoft::WRL;

HRESULT LinkHelper::tryCreateShortcut(const std::wstring &shortcutPath, const std::wstring &exePath, const std::wstring &appID)
{
    HRESULT hr = S_OK;
    std::wstringstream lnkName;

    if (PathIsRelative(shortcutPath.c_str()) == TRUE) {
        lnkName << startmenuPath();
    }
    lnkName << shortcutPath;

    if (shortcutPath.rfind(L".lnk") == std::wstring::npos) {
        lnkName << L".lnk";
    }
    hr = mkdirs(lnkName.str());
    if (SUCCEEDED(hr)) {

        DWORD attributes = GetFileAttributes(lnkName.str().c_str());
        bool fileExists = attributes < 0xFFFFFFF;

        if (!fileExists) {
            hr = installShortcut(lnkName.str(), exePath, appID, __uuidof(CToastNotificationActivationCallback));
        } else {
            hr = S_FALSE;
        }

    }
    registerActivator();
    return hr;
}

HRESULT LinkHelper::tryCreateShortcut(const std::wstring &appID)
{
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameEx(GetCurrentProcess(), nullptr, buffer, MAX_PATH) > 0) {
        return tryCreateShortcut(L"SnoreToast.lnk", buffer, appID);
    }
    return E_FAIL;
}

HRESULT LinkHelper::registerActivator()
{
    Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::Create([] {});
    Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().IncrementObjectCount();
    return Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().RegisterObjects();
}

void LinkHelper::unregisterActivator()
{
    Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().UnregisterObjects();
    Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().DecrementObjectCount();
}

// Install the shortcut
HRESULT LinkHelper::installShortcut(const std::wstring &shortcutPath, const std::wstring &exePath, const std::wstring &appID, GUID toastGUID)
{
    PCWSTR pszExePath = exePath.c_str();
    std::wcout << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID << std::endl;
    HRESULT hr = HRESULT_FROM_WIN32(::RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\CLSID\\{383803B6-AFDA-4220-BFC3-0DBF810106BA}\\LocalServer32", nullptr, REG_SZ, pszExePath, static_cast<DWORD>(wcslen(pszExePath)*sizeof(wchar_t))));

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
                                toastActivatorPropVar.puuid = &toastGUID;
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
        std::wcout << "Failed to install shortcut " << shortcutPath << "  error: " << _com_error(hr).ErrorMessage() << std::endl;
    }
    return hr;
}

HRESULT LinkHelper::mkdirs(const std::wstring &dirs)
{
    HRESULT hr = S_OK;
    static std::wregex seperator(L"\\\\|/");

    for (std::wsregex_iterator i = std::wsregex_iterator(dirs.begin(), dirs.end(), seperator); SUCCEEDED(hr) && i != std::wsregex_iterator(); ++i) {
        hr = _wmkdir(dirs.substr(0, i->position()).c_str()) != ENOENT ? S_OK : E_FAIL;
    }
    return hr;
}

std::wstring LinkHelper::startmenuPath()
{
    wchar_t buffer[MAX_PATH];
    std::wstringstream path;

    if (GetEnvironmentVariable(L"APPDATA", buffer , MAX_PATH) > 0) {
        path << buffer
             << L"\\Microsoft\\Windows\\Start Menu\\Programs\\";

    }
    return path.str();
}

