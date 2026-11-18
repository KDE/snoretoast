#pragma once

#include "snoretoasts.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <vector>

namespace Testing {

struct TestCase
{
    const char *name;
    std::function<void()> fn;
};

inline std::vector<TestCase> &registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

inline int failures = 0;

struct Register
{
    Register(const char *name, std::function<void()> fn) { registry().push_back({ name, fn }); }
};

inline std::wstring uniqueName(const std::wstring &prefix)
{
    static std::atomic<int> counter = 0;
    return prefix + std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(counter++);
}

inline std::wstring uniquePipeName()
{
    return uniqueName(L"\\\\.\\pipe\\snoretoast_test_");
}

class PipeServer
{
public:
    PipeServer() : m_name(uniquePipeName())
    {
        m_pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_INBOUND,
                                  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 4096, 0,
                                  nullptr);
        m_result = std::async(std::launch::async, [this] {
            std::wstring out;
            if (!ConnectNamedPipe(m_pipe, nullptr)) {
                const auto err = GetLastError();
                if (err != ERROR_PIPE_CONNECTED && err != ERROR_NO_DATA) {
                    return out;
                }
            }
            wchar_t buf[512];
            DWORD read = 0;
            while (ReadFile(m_pipe, buf, sizeof(buf), &read, nullptr) && read > 0) {
                out.append(buf, read / sizeof(wchar_t));
            }
            return out;
        });
    }

    ~PipeServer()
    {
        if (m_result.valid()
            && m_result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            HANDLE client = CreateFileW(m_name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                        0, nullptr);
            if (client != INVALID_HANDLE_VALUE) {
                CloseHandle(client);
            }
            m_result.wait();
        }
        CloseHandle(m_pipe);
    }

    bool valid() const { return m_pipe != INVALID_HANDLE_VALUE; }
    const std::wstring &name() const { return m_name; }
    std::wstring received() { return m_result.get(); }

private:
    std::wstring m_name;
    HANDLE m_pipe;
    std::future<std::wstring> m_result;
};

inline const std::wstring versionEntry = L"version=" + SnoreToasts::version() + L";";

}

using Action = SnoreToastActions::Actions;

inline std::wostream &operator<<(std::wostream &out, Action a)
{
    return out << static_cast<int>(a);
}

#define TEST(name)                                                                                 \
    static void name();                                                                            \
    static const Testing::Register name##_reg(#name, name);                                        \
    static void name()

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            ++Testing::failures;                                                                   \
            std::wcerr << __FILE__ << L":" << __LINE__ << L": CHECK(" #expr L") failed"            \
                       << std::endl;                                                               \
        }                                                                                          \
    } while (false)

#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        const auto &_a = (actual);                                                                 \
        const auto &_e = (expected);                                                               \
        if (!(_a == _e)) {                                                                         \
            ++Testing::failures;                                                                   \
            std::wcerr << __FILE__ << L":" << __LINE__ << L": CHECK_EQ(" #actual L", " #expected   \
                       << L") failed\n\tactual:   '" << _a << L"'\n\texpected: '" << _e << L"'"   \
                       << std::endl;                                                               \
        }                                                                                          \
    } while (false)
