/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2019  Hannah von Reth <vonreth@kde.org>

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

#include <windows.h>

#include <sstream>
#include <unordered_map>

namespace Utils
{
    HRESULT registerActivator();
    void unregisterActivator();

    std::unordered_map<std::wstring, std::wstring> splitData(const std::wstring &data);

    std::wstring selfLocate();

    std::wstring formatData(const std::vector<std::pair<std::wstring, std::wstring>> &data);

    bool writePipe(const std::wstring &pipe, const std::wstring &data);
};


class ToastLog
{
public:
    ToastLog();
    ~ToastLog();

    inline ToastLog &log() { return *this;}

private:
    std::wstringstream m_log;
    template<typename T>
    friend ToastLog & operator<<(ToastLog &, const T&);
};

#define tLog ToastLog().log() << __FUNCSIG__ << ": "

template <typename T>
ToastLog &operator<< (ToastLog &log, const T &t) {
    log.m_log << t;
    return log;
}
