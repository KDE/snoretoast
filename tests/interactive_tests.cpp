#include "snoretoasts.h"
#include "utils.h"

#include "testing.h"

#include <map>
#include <optional>
#include <thread>

using namespace Testing;
using namespace std::chrono_literals;

namespace {

std::filesystem::path s_snoretoast;

class Process
{
public:
    explicit Process(const std::vector<std::wstring> &args)
    {
        std::wstring commandLine = quoteArgument(s_snoretoast.wstring());
        for (const auto &arg : args) {
            commandLine += L" " + quoteArgument(arg);
        }
        STARTUPINFOW info = {};
        info.cb = sizeof(info);
        if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, false, 0, nullptr,
                            nullptr, &info, &m_info)) {
            std::wcerr << L"Failed to start " << commandLine << L": "
                       << Utils::formatWinError(GetLastError()) << std::endl;
        }
    }

    ~Process()
    {
        if (m_info.hProcess) {
            if (WaitForSingleObject(m_info.hProcess, 0) == WAIT_TIMEOUT) {
                TerminateProcess(m_info.hProcess, 1);
            }
            CloseHandle(m_info.hProcess);
            CloseHandle(m_info.hThread);
        }
    }

    std::optional<int> exitCode(std::chrono::milliseconds timeout)
    {
        if (!m_info.hProcess
            || WaitForSingleObject(m_info.hProcess, static_cast<DWORD>(timeout.count()))
                    != WAIT_OBJECT_0) {
            return {};
        }
        DWORD code = 0;
        GetExitCodeProcess(m_info.hProcess, &code);
        return static_cast<int>(code);
    }

private:
    PROCESS_INFORMATION m_info = {};
};

std::wostream &operator<<(std::wostream &out, const std::optional<int> &code)
{
    if (code) {
        return out << *code;
    }
    return out << L"<no exit>";
}

void announce(const std::wstring &instruction)
{
    std::wcout << L"\n>>> " << instruction << std::endl;
}

void expectPipe(const std::wstring &pipe, const std::wstring &id,
                const std::map<std::wstring, std::wstring> &expected)
{
    const auto data = Utils::splitData(pipe);
    const auto value = [&data](const std::wstring &key) {
        const auto it = data.find(key);
        return it == data.end() ? std::wstring(L"<missing>") : std::wstring(it->second);
    };
    CHECK_EQ(value(L"notificationId"), id);
    for (const auto &[key, expectedValue] : expected) {
        CHECK_EQ(value(key), expectedValue);
    }
}

void scenario(const std::wstring &title, const std::wstring &instruction,
              std::vector<std::wstring> extraArgs, Action expected,
              const std::map<std::wstring, std::wstring> &expectedPipe)
{
    announce(instruction);
    PipeServer server;
    const auto id = uniqueName(L"interactive_");
    std::vector<std::wstring> args = { L"-t",  title, L"-m", instruction, L"-id",
                                       id,     L"-pipeName", server.name() };
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());
    Process process(args);
    CHECK_EQ(process.exitCode(90s), std::optional<int>(static_cast<int>(expected)));
    if (!expectedPipe.empty()) {
        expectPipe(server.receivedWithin(5s), id, expectedPipe);
    }
}

void question(const std::wstring &title, const std::wstring &question,
              std::vector<std::wstring> extraArgs, const std::wstring &answers,
              const std::wstring &expectedAnswer)
{
    extraArgs.insert(extraArgs.end(), { L"-b", answers });
    scenario(title, question, extraArgs, Action::ButtonClicked,
             { { L"action", L"buttonClicked" }, { L"button", expectedAnswer } });
}

}

TEST(clickBody)
{
    scenario(L"Click me", L"Click on the body of this notification, not on a button.", {},
             Action::Clicked, { { L"action", L"clicked" } });
}

TEST(clickButton)
{
    scenario(L"Press a button", L"Press the button labeled \"Press me\".",
             { L"-b", L"Not me;Press me" }, Action::ButtonClicked,
             { { L"action", L"buttonClicked" }, { L"button", L"Press me" } });
}

TEST(enterText)
{
    scenario(L"Reply", L"Type exactly: snore  and press Send.", { L"-tb" }, Action::TextEntered,
             { { L"action", L"textEntered" }, { L"text", L"snore" } });
}

TEST(dismiss)
{
    scenario(L"Dismiss me", L"Close this notification with its X button.", {}, Action::Dismissed,
             { { L"action", L"dismissed" } });
}

TEST(timeout)
{
    scenario(L"Wait", L"Don't touch this notification, wait until it disappears by itself.",
             { L"-d", L"short" }, Action::Timedout, { { L"action", L"timedout" } });
}

TEST(closeById)
{
    announce(L"A notification will appear and be closed automatically in 3 seconds. Don't touch it.");
    const auto id = uniqueName(L"interactive_");
    Process toast({ L"-t", L"Closing soon", L"-m", L"This notification will be closed automatically.",
                    L"-id", id });
    std::this_thread::sleep_for(3s);
    Process closer({ L"-close", id });
    CHECK_EQ(closer.exitCode(10s), std::optional<int>(static_cast<int>(Action::Clicked)));
    CHECK_EQ(toast.exitCode(10s), std::optional<int>(static_cast<int>(Action::Hidden)));
}

TEST(showsLogo)
{
    question(L"Image", L"Does this notification show the SnoreToast logo?", {}, L"Yes;No", L"Yes");
}

TEST(playsSound)
{
    question(L"Sound", L"Did you hear a notification sound?", {}, L"Yes;No", L"Yes");
}

TEST(silent)
{
    question(L"Silent", L"Did you hear a notification sound?", { L"-silent" }, L"Yes;No", L"No");
}

TEST(unicode)
{
    question(L"Ünïcödé ✓ ★ 日本語",
             L"Is the title shown as: \"Ünïcödé ✓ ★ 日本語\" (U-umlaut, n, i-umlaut, ..., a check "
             L"mark, a star and three Japanese characters)?",
             {}, L"Yes;No", L"Yes");
}

int wmain(int argc, wchar_t **argv)
{
    setupUnicodeConsole();
    s_snoretoast = Utils::selfLocate().parent_path() / L"snoretoast.exe";
    std::vector<std::wstring> filter;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--snoretoast" && i + 1 < argc) {
            s_snoretoast = argv[++i];
        } else if (arg == L"--list") {
            for (const auto &test : registry()) {
                std::wcout << test.name << std::endl;
            }
            return 0;
        } else {
            filter.push_back(arg);
        }
    }
    if (!std::filesystem::exists(s_snoretoast)) {
        std::wcerr << s_snoretoast << L" does not exist, pass --snoretoast <path>" << std::endl;
        return 1;
    }
    std::wcout << L"Interactive SnoreToast tests using " << s_snoretoast << L"\n"
               << L"Each notification tells you what to do. Make sure notifications are enabled\n"
               << L"and Do Not Disturb is off. You have 60 seconds per notification." << std::endl;
    return runTests(filter);
}
