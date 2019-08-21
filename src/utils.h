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

#include <comdef.h>
#include <filesystem>
#include <sstream>
#include <unordered_map>

class ToastLog;

class ToastLog
{
public:
    ToastLog();
    ~ToastLog();

    inline ToastLog &log() { return *this; }

private:
    std::wstringstream m_log;
    template<typename T>
    friend ToastLog &operator<<(ToastLog &, const T &);
};

template<typename T>
ToastLog &operator<<(ToastLog &log, const T &t)
{
    log.m_log << L" " << t;
    return log;
}

template<>
inline ToastLog &operator<<(ToastLog &log, const HRESULT &hr)
{
    if (FAILED(hr)) {
        _com_error err(hr);
        log.m_log << L" Error: " << hr << L" " << err.ErrorMessage();
    }
    return log;
}

#define tLog ToastLog().log() << __FUNCSIG__ << L"\n\t\t"

namespace Utils {
bool registerActivator();
void unregisterActivator();

std::unordered_map<std::wstring_view, std::wstring_view> splitData(const std::wstring_view &data);

const std::filesystem::path &selfLocate();

std::wstring formatData(const std::vector<std::pair<std::wstring_view, std::wstring_view>> &data);

bool writePipe(const std::filesystem::path &pipe, const std::wstring &data, bool wait = false);
bool startProcess(const std::filesystem::path &app);

inline bool checkResult(const char *file, const long line, const char *func, const HRESULT &hr)
{
    if (FAILED(hr)) {
        tLog << file << line << func << L":\n\t\t\t" << hr;
        return false;
    }
    return true;
}

std::wstring formatWinError(unsigned long errorCode);
};

#define ST_CHECK_RESULT(hr) Utils::checkResult(__FILE__, __LINE__, __FUNCSIG__, hr)

#define ST_RETURN_ON_ERROR(hr)                                                                     \
    do {                                                                                           \
        HRESULT _tmp = hr;                                                                         \
        if (!ST_CHECK_RESULT(_tmp)) {                                                              \
            return _tmp;                                                                           \
        }                                                                                          \
    } while (false)
