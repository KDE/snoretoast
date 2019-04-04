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

class LIBSNORETOAST_EXPORT LinkHelper
{
public:
    static HRESULT tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                     const std::filesystem::path &exePath,
                                     const std::wstring &appID,
                                     const std::wstring &callbackUUID = {});

    static HRESULT tryCreateShortcut(const std::filesystem::path &shortcutPath,
                                     const std::wstring &appID,
                                     const std::wstring &callbackUUID = {});

private:
    static HRESULT installShortcut(const std::filesystem::path &shortcutPath,
                                   const std::filesystem::path &exePath, const std::wstring &appID,
                                   const std::wstring &callbackUUID = {});

    static std::filesystem::path startmenuPath();
};
