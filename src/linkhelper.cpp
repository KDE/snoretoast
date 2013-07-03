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
#include <sstream>

using namespace Microsoft::WRL;



HRESULT LinkHelper::tryCreateShortcut(const std::wstring &shortcutPath,const std::wstring &exePath, const std::wstring &appID)
{
    HRESULT hr = S_OK;

    std::wstringstream path;
    std::wstring lnkName;
    wchar_t buffer[MAX_PATH];

    if (GetEnvironmentVariable(L"APPDATA",buffer , MAX_PATH)>0)
    {
        path << buffer
                     << L"\\Microsoft\\Windows\\Start Menu\\Programs\\";

    }

    lnkName = shortcutPath;

    if(shortcutPath.rfind(L".lnk") == std::wstring::npos)
    {
        lnkName.append(L".lnk");
    }
	hr = mkdirs(path.str(),lnkName);
    if(SUCCEEDED(hr))
    {
        path << lnkName;

        DWORD attributes = GetFileAttributes(path.str().c_str());
        bool fileExists = attributes < 0xFFFFFFF;

        if (!fileExists)
        {
            hr = installShortcut(path.str(),exePath,appID);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;
}

HRESULT LinkHelper::tryCreateShortcut(const std::wstring &appID)
{
    wchar_t buffer[MAX_PATH];
    if(GetModuleFileNameEx(GetCurrentProcess(), nullptr, buffer,MAX_PATH)>0)
    {
        return tryCreateShortcut(L"SnoreToast.lnk",buffer,appID);
    }
    return E_FAIL;
}




// Install the shortcut
HRESULT LinkHelper::installShortcut(const std::wstring &shortcutPath,const std::wstring &exePath, const std::wstring &appID)
{
    std::wcout << L"Installing shortcut: " << shortcutPath << L" " << exePath << L" " << appID << std::endl;
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        ComPtr<IShellLink> shellLink;
        hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

        if (SUCCEEDED(hr))
        {
            hr = shellLink->SetPath(exePath.c_str());
            if (SUCCEEDED(hr))
            {
                hr = shellLink->SetArguments(L"");
                if (SUCCEEDED(hr))
                {
                    ComPtr<IPropertyStore> propertyStore;

                    hr = shellLink.As(&propertyStore);
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT appIdPropVar;
                        hr = InitPropVariantFromString(appID.c_str(), &appIdPropVar);
                        if (SUCCEEDED(hr))
                        {
                            hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                            if (SUCCEEDED(hr))
                            {
                                hr = propertyStore->Commit();
                                if (SUCCEEDED(hr))
                                {
                                    ComPtr<IPersistFile> persistFile;
                                    hr = shellLink.As(&persistFile);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = persistFile->Save(shortcutPath.c_str(), TRUE);
                                    }
                                }
                            }
                            PropVariantClear(&appIdPropVar);
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT LinkHelper::mkdirs(const std::wstring &basepath,const std::wstring &dirs)
{
    HRESULT hr = S_OK;
    size_t pos;
    size_t oldPos = 0;
    size_t last_pos = dirs.rfind(L"\\");
    if(last_pos == std::wstring::npos)
    {
        return hr;
    }
    while(SUCCEEDED(hr) && (pos = dirs.find(L"\\",oldPos)) <= last_pos)
    {
        std::wcout << L"mkdir" << basepath << dirs.substr(0,pos) <<std::endl;
        hr = _wmkdir((basepath + dirs.substr(0,pos)).c_str()) != ENOENT?S_OK:E_FAIL;
		if(oldPos == pos)
		{
			break;
		}
        oldPos = pos;
    }
    return hr;
}

