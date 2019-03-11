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

#include "utils.h"
#include "snoretoasts.h"

#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>



using namespace Microsoft::WRL;

namespace {
    bool s_registered = false;
}
namespace Utils {

bool registerActivator()
{
    if (!s_registered)
    {
        s_registered = true;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::Create([] {});
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().IncrementObjectCount();
        return SUCCEEDED(Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().RegisterObjects());
    }
    return true;
}

void unregisterActivator()
{
    if (s_registered)
    {
        s_registered = false;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().UnregisterObjects();
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().DecrementObjectCount();
    }
}

std::unordered_map<std::wstring, std::wstring> splitData(const std::wstring &data)
{
    std::unordered_map<std::wstring, std::wstring> out;
    std::wstring tmp;
    std::wstringstream wss(data);
    while(std::getline(wss, tmp, L';'))
    {
        const auto pos = tmp.find(L"=");
        out[tmp.substr(0, pos)] = tmp.substr(pos + 1);
    }
    return out;
}

const std::filesystem::path &selfLocate()
{
    static const std::filesystem::path path = [] {
		std::wstring buf;
        size_t size;
        do {
			buf.resize(buf.size() + 1024);
            size = GetModuleFileNameW(nullptr, const_cast<wchar_t*>(buf.data()), static_cast<DWORD>(buf.size()));
        } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		buf.resize(size);
		return buf;
	}();
    return path;
}

bool writePipe(const std::filesystem::path &pipe, const std::wstring &data)
{
    HANDLE hPipe = CreateFile(pipe.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        const DWORD toWrite = static_cast<DWORD>(data.size() * sizeof(wchar_t));
        WriteFile(hPipe, data.c_str(), toWrite, &written, nullptr);
        const bool success = written == toWrite;
        tLog << (success ? L"Wrote: " : L"Failed to write: ") << data << " to " << pipe;
        WriteFile(hPipe, nullptr, sizeof(wchar_t), &written, nullptr);
        CloseHandle(hPipe);
        return success;
    }
	tLog << L"Failed to open pipe: " << pipe << L" data: " << data;
    return false;
}

bool startProcess(const std::filesystem::path & app)
{
	STARTUPINFO info = {};
	info.cb = sizeof(info);
	PROCESS_INFORMATION pInfo = {};
	const auto application = app.wstring();
	if (!CreateProcess(const_cast<wchar_t*>(application.c_str()), const_cast<wchar_t*>(application.c_str()), nullptr, nullptr, false, DETACHED_PROCESS | INHERIT_PARENT_AFFINITY | CREATE_NO_WINDOW, nullptr, nullptr, &info, &pInfo))
	{
		tLog << L"Failed to start: " << app;
		return false;
	}
	WaitForInputIdle(pInfo.hProcess, INFINITE);
	// wait a second until the application is initialised
	Sleep(1000);
	DWORD status;
	GetExitCodeProcess(pInfo.hProcess, &status);
	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	tLog << L"Started: " << app << L" Status: " << (status == STILL_ACTIVE ? L"STILL_ACTIVE" : std::to_wstring(status));
	return status == STILL_ACTIVE;
}

std::wstring formatData(const std::vector<std::pair<std::wstring, std::wstring> > &data)
{
    std::wstringstream out;
    for (const auto &p : data)
    {
        if (!p.second.empty())
        {
            out << p.first << L"=" << p.second << L";";
        }
    }
    return out.str();
}

}

ToastLog::ToastLog()
{
    *this << Utils::selfLocate() <<  L" v" << SnoreToasts::version() << L"\n\t";
}

ToastLog::~ToastLog()
{
	m_log << L"\n";
    OutputDebugStringW(m_log.str().c_str());
}
