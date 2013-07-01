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
#include "linkhelper.h"

#include <propvarutil.h>

#include <iostream>

using namespace Microsoft::WRL;


LinkHelper::LinkHelper(const std::wstring &shortcutPath, const std::wstring &appID):
    m_shortcutPath(shortcutPath),
    m_appID(appID)
{
    if(shortcutPath.length()<=0)
    {

        wchar_t buffer[MAX_PATH];
        DWORD charWritten = GetEnvironmentVariable(L"APPDATA",buffer , MAX_PATH);
        HRESULT hr = charWritten > 0 ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr))
        {
            m_shortcutPath = buffer;
            m_shortcutPath.append(L"\\Microsoft\\Windows\\Start Menu\\Programs\\SnoreToast.lnk");
        }

    }
}



HRESULT LinkHelper::tryCreateShortcut()
{
    HRESULT hr = S_OK;
    DWORD attributes = GetFileAttributes(m_shortcutPath.c_str());
    bool fileExists = attributes < 0xFFFFFFF;

    if (!fileExists)
    {
        hr = installShortcut();
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

HRESULT LinkHelper::setPropertyForExisitingShortcut()
{

    HRESULT hr = S_OK;
    ComPtr<IShellLink> shellLink;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

    if (SUCCEEDED(hr))
    {
        ComPtr<IPersistFile> persistFile;
        hr = shellLink.As(&persistFile);
        if (SUCCEEDED(hr))
        {
            hr = persistFile.Get()->Load(m_shortcutPath.c_str(), STGM_READWRITE);
            if (SUCCEEDED(hr))
            {
                hr = setPropertyForShortcut(shellLink,persistFile.Get());
            }
        }
    }
    return hr;
}

HRESULT LinkHelper::setPropertyForShortcut(ComPtr<IShellLink> shellLink,IPersistFile *persistFile)
{
    HRESULT hr = S_OK;
    ComPtr<IPropertyStore> propertyStore;

    hr = shellLink.As(&propertyStore);
    if (SUCCEEDED(hr))
    {
        PROPVARIANT appIdPropVar;
        hr = InitPropVariantFromString(m_appID.c_str(), &appIdPropVar);
        if (SUCCEEDED(hr))
        {
            hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
            if (SUCCEEDED(hr))
            {
                hr = propertyStore->Commit();
                if (SUCCEEDED(hr))
                {
                    hr = persistFile->Save(m_shortcutPath.c_str(), TRUE);
                }
            }
            PropVariantClear(&appIdPropVar);
        }
    }
    return hr;
}


// Install the shortcut
HRESULT LinkHelper::installShortcut()
{
    wchar_t exePath[MAX_PATH];

    DWORD charWritten = GetModuleFileNameEx(GetCurrentProcess(), nullptr, exePath, ARRAYSIZE(exePath));

    HRESULT hr = charWritten > 0 ? S_OK : E_FAIL;

    if (SUCCEEDED(hr))
    {
        ComPtr<IShellLink> shellLink;
        hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

        if (SUCCEEDED(hr))
        {
            hr = shellLink->SetPath(exePath);
            if (SUCCEEDED(hr))
            {
                hr = shellLink->SetArguments(L"");
                if (SUCCEEDED(hr))
                {
                    ComPtr<IPersistFile> persistFile;
                    hr = shellLink.As(&persistFile);
                    if (SUCCEEDED(hr))
                    {
                        hr = setPropertyForShortcut(shellLink,persistFile.Get());
                    }
                }
            }
        }
    }
    return hr;
}

