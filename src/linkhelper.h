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

#include "snoretoasts.h"

class LinkHelper
{
public:
    LinkHelper(const std::wstring &shortcutPath, const std::wstring &appID);
    HRESULT tryCreateShortcut();
    HRESULT installShortcut();
    HRESULT setPropertyForExisitingShortcut();
    HRESULT setPropertyForShortcut(Microsoft::WRL::ComPtr<IShellLinkW> shellLink, IPersistFile *persistFile);

private:
    std::wstring m_shortcutPath;
    std::wstring m_appID;
};
