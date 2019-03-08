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
