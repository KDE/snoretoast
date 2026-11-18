#include "snoretoasts.h"
#include "config.h"
#include "snoretoastactioncenterintegration.h"
#include "utils.h"

#include "testing.h"

#include <roapi.h>
#include <shellapi.h>

using namespace Testing;

TEST(actionStringsRoundTrip)
{
    const std::vector<std::pair<Action, std::wstring>> expected = {
        { Action::Clicked, L"clicked" },        { Action::Hidden, L"hidden" },
        { Action::Dismissed, L"dismissed" },    { Action::Timedout, L"timedout" },
        { Action::ButtonClicked, L"buttonClicked" }, { Action::TextEntered, L"textEntered" },
    };
    for (const auto &[action, str] : expected) {
        CHECK_EQ(SnoreToastActions::getActionString(action), str);
        CHECK_EQ(SnoreToastActions::getAction(str), action);
        CHECK_EQ(SnoreToastActions::getAction(std::wstring_view(str)), action);
        CHECK_EQ(SnoreToastActions::getAction(str.c_str()), action);
    }
}

TEST(actionUnknownIsError)
{
    CHECK_EQ(SnoreToastActions::getAction(std::wstring(L"")), Action::Error);
    CHECK_EQ(SnoreToastActions::getAction(std::wstring(L"foo")), Action::Error);
    CHECK_EQ(SnoreToastActions::getAction(std::wstring(L"Clicked")), Action::Error);
    CHECK_EQ(SnoreToastActions::getAction(std::wstring(L"clicked ")), Action::Error);
}

TEST(actionErrorHasNoString)
{
    bool threw = false;
    try {
        SnoreToastActions::getActionString(Action::Error);
    } catch (const std::out_of_range &) {
        threw = true;
    }
    CHECK(threw);
}

TEST(actionExitCodesAreStable)
{
    CHECK_EQ(static_cast<int>(Action::Clicked), 0);
    CHECK_EQ(static_cast<int>(Action::Hidden), 1);
    CHECK_EQ(static_cast<int>(Action::Dismissed), 2);
    CHECK_EQ(static_cast<int>(Action::Timedout), 3);
    CHECK_EQ(static_cast<int>(Action::ButtonClicked), 4);
    CHECK_EQ(static_cast<int>(Action::TextEntered), 5);
    CHECK_EQ(static_cast<int>(Action::Error), -1);
}

TEST(splitDataEmpty)
{
    CHECK(Utils::splitData(L"").empty());
    CHECK(Utils::splitData(L";").empty());
}

TEST(splitDataBasic)
{
    const auto map = Utils::splitData(L"action=clicked;notificationId=42;pipe=\\\\.\\pipe\\foo;");
    CHECK_EQ(map.size(), size_t(3));
    CHECK_EQ(map.at(L"action"), std::wstring_view(L"clicked"));
    CHECK_EQ(map.at(L"notificationId"), std::wstring_view(L"42"));
    CHECK_EQ(map.at(L"pipe"), std::wstring_view(L"\\\\.\\pipe\\foo"));
}

TEST(splitDataValueContainsEquals)
{
    const auto map = Utils::splitData(L"text=a=b;");
    CHECK_EQ(map.at(L"text"), std::wstring_view(L"a=b"));
}

TEST(splitDataEmptyValue)
{
    const auto map = Utils::splitData(L"pipe=;action=clicked;");
    CHECK_EQ(map.at(L"pipe"), std::wstring_view(L""));
    CHECK_EQ(map.at(L"action"), std::wstring_view(L"clicked"));
}

TEST(splitDataSkipsEmptyKey)
{
    const auto map = Utils::splitData(L"=foo;action=clicked;");
    CHECK_EQ(map.size(), size_t(1));
    CHECK(map.find(L"") == map.end());
}

TEST(splitDataSkipsSegmentsWithoutEquals)
{
    const auto map = Utils::splitData(L"foo;action=clicked;bar;");
    CHECK_EQ(map.size(), size_t(1));
    CHECK_EQ(map.at(L"action"), std::wstring_view(L"clicked"));
}

TEST(splitDataSkipsEmptySegments)
{
    const auto map = Utils::splitData(L";a=1;;b=2;;");
    CHECK_EQ(map.size(), size_t(2));
    CHECK_EQ(map.at(L"a"), std::wstring_view(L"1"));
    CHECK_EQ(map.at(L"b"), std::wstring_view(L"2"));
}

TEST(splitDataWithoutTrailingSeparator)
{
    const auto map = Utils::splitData(L"a=1;b=2");
    CHECK_EQ(map.size(), size_t(2));
    CHECK_EQ(map.at(L"b"), std::wstring_view(L"2"));
}

TEST(splitDataLastKeyWins)
{
    const auto map = Utils::splitData(L"a=1;a=2;");
    CHECK_EQ(map.at(L"a"), std::wstring_view(L"2"));
}

TEST(formatDataOnlyVersion)
{
    CHECK_EQ(Utils::formatData({}), versionEntry);
}

TEST(formatDataKeepsOrderAndSkipsEmpty)
{
    const auto out = Utils::formatData({ { L"action", L"clicked" },
                                         { L"pipe", L"" },
                                         { L"notificationId", L"7" } });
    CHECK_EQ(out, L"action=clicked;notificationId=7;" + versionEntry);
}

TEST(formatSplitRoundTrip)
{
    const auto formatted = Utils::formatData({ { L"action", L"buttonClicked" },
                                               { L"button", L"Yes please" },
                                               { L"application", L"C:\\foo bar\\app.exe" } });
    const auto map = Utils::splitData(formatted);
    CHECK_EQ(map.size(), size_t(4));
    CHECK_EQ(map.at(L"action"), std::wstring_view(L"buttonClicked"));
    CHECK_EQ(map.at(L"button"), std::wstring_view(L"Yes please"));
    CHECK_EQ(map.at(L"application"), std::wstring_view(L"C:\\foo bar\\app.exe"));
    CHECK_EQ(map.at(L"version"), std::wstring_view(SnoreToasts::version()));
}

TEST(versionMatchesConfig)
{
    CHECK_EQ(SnoreToasts::version(), SNORETOAST_VERSION);
    CHECK_EQ(SnoreToasts::version(),
             std::to_wstring(SNORETOAST_VERSION_MAJOR) + L"." + std::to_wstring(SNORETOAST_VERSION_MINOR)
                     + L"." + std::to_wstring(SNORETOAST_VERSION_PATCH));
}

TEST(callbackUuidIsBracedGuid)
{
    const auto uuid = SnoreToastActionCenterIntegration::uuid();
    CHECK_EQ(uuid.size(), size_t(38));
    CHECK_EQ(uuid.front(), L'{');
    CHECK_EQ(uuid.back(), L'}');
    GUID guid;
    CHECK(SUCCEEDED(CLSIDFromString(uuid.c_str(), &guid)));
    CHECK(IsEqualGUID(guid, __uuidof(SnoreToastActionCenterIntegration)));
}

TEST(formatWinErrorKnownCode)
{
    CHECK(!Utils::formatWinError(ERROR_FILE_NOT_FOUND).empty());
}

TEST(formatWinErrorUnknownCode)
{
    CHECK(Utils::formatWinError(0x2FFFFFFF).empty());
}

TEST(selfLocateIsThisExecutable)
{
    wchar_t buf[MAX_PATH * 4];
    const auto len = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    CHECK_EQ(Utils::selfLocate(), std::filesystem::path(std::wstring(buf, len)));
    CHECK_EQ(Utils::selfLocate().filename(), std::filesystem::path(L"snoretoast_tests.exe"));
    CHECK(&Utils::selfLocate() == &Utils::selfLocate());
}

TEST(writePipeWithoutServerFails)
{
    CHECK(!Utils::writePipe(uniquePipeName(), L"action=clicked;"));
}

TEST(writePipeDeliversData)
{
    PipeServer server;
    CHECK(server.valid());
    const std::wstring data = L"action=clicked;notificationId=1;";
    CHECK(Utils::writePipe(server.name(), data, true));
    CHECK_EQ(server.received(), data);
}

TEST(activateWithoutArgsIsNoop)
{
    auto callback = Microsoft::WRL::Make<SnoreToastActionCenterIntegration>();
    CHECK(callback->Activate(L"Some.App", nullptr, nullptr, 0) == S_OK);
}

TEST(activateForwardsClickToPipe)
{
    PipeServer server;
    const auto args = L"action=clicked;notificationId=3;pipe=" + server.name() + L";";
    auto callback = Microsoft::WRL::Make<SnoreToastActionCenterIntegration>();
    CHECK(callback->Activate(L"Some.App", args.c_str(), nullptr, 0) == S_OK);
    CHECK_EQ(server.received(), args);
}

TEST(activateAppendsTextReply)
{
    PipeServer server;
    const auto args = L"action=textEntered;notificationId=3;pipe=" + server.name() + L";";
    const NOTIFICATION_USER_INPUT_DATA input[] = { { L"textBox", L"hello\rworld" } };
    auto callback = Microsoft::WRL::Make<SnoreToastActionCenterIntegration>();
    CHECK(callback->Activate(L"Some.App", args.c_str(), input, 1) == S_OK);
    CHECK_EQ(server.received(), args + L"text=hello\nworld");
}

TEST(activateIgnoresTextForOtherActions)
{
    PipeServer server;
    const auto args = L"action=buttonClicked;button=Ok;pipe=" + server.name() + L";";
    const NOTIFICATION_USER_INPUT_DATA input[] = { { L"textBox", L"ignored" } };
    auto callback = Microsoft::WRL::Make<SnoreToastActionCenterIntegration>();
    CHECK(callback->Activate(L"Some.App", args.c_str(), input, 1) == S_OK);
    CHECK_EQ(server.received(), args);
}

TEST(activateWithoutPipeSucceeds)
{
    auto callback = Microsoft::WRL::Make<SnoreToastActionCenterIntegration>();
    CHECK(callback->Activate(L"Some.App", L"action=clicked;", nullptr, 0) == S_OK);
}

TEST(toastDefaults)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    CHECK_EQ(toast.id(), std::to_wstring(GetCurrentProcessId()));
    CHECK(toast.duration() == Duration::Short);
    CHECK(toast.pipeName().empty());
    CHECK(toast.application().empty());
    CHECK(toast.useFalbackMode());
}

TEST(toastSetters)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"my-id");
    CHECK_EQ(toast.id(), std::wstring(L"my-id"));
    toast.setId(L"");
    CHECK_EQ(toast.id(), std::wstring(L"my-id"));
    toast.setDuration(Duration::Long);
    CHECK(toast.duration() == Duration::Long);
    toast.setPipeName(L"\\\\.\\pipe\\foo");
    CHECK_EQ(toast.pipeName(), std::filesystem::path(L"\\\\.\\pipe\\foo"));
    toast.setApplication(L"C:\\foo.exe");
    CHECK_EQ(toast.application(), std::filesystem::path(L"C:\\foo.exe"));
}

TEST(toastFormatActionMinimal)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"42");
    CHECK_EQ(toast.formatAction(Action::Clicked),
             L"action=clicked;notificationId=42;" + versionEntry);
}

TEST(toastFormatActionFull)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setId(L"42");
    toast.setPipeName(L"\\\\.\\pipe\\foo");
    toast.setApplication(L"C:\\foo.exe");
    CHECK_EQ(toast.formatAction(Action::ButtonClicked, { { L"button", L"Yes" } }),
             L"action=buttonClicked;notificationId=42;pipe=\\\\.\\pipe\\foo;"
             L"application=C:\\foo.exe;button=Yes;"
                     + versionEntry);
}

TEST(toastFormatActionParsesBack)
{
    SnoreToasts toast(L"SnoreToast.Tests.Unregistered");
    toast.setPipeName(L"\\\\.\\pipe\\foo");
    const auto formatted = toast.formatAction(Action::TextEntered);
    const auto map = Utils::splitData(formatted);
    CHECK_EQ(SnoreToastActions::getAction(map.at(L"action")), Action::TextEntered);
    CHECK_EQ(map.at(L"notificationId"), std::wstring_view(toast.id()));
    CHECK_EQ(map.at(L"pipe"), std::wstring_view(L"\\\\.\\pipe\\foo"));
    CHECK(map.find(L"application") == map.end());
}

TEST(quoteArgumentRoundTrip)
{
    const std::vector<std::wstring> args = {
        L"plain",
        L"with space",
        L"",
        L"Press the button labeled \"Press me\".",
        L"\\\\.\\pipe\\foo",
        L"trailing\\",
        L"trailing two\\\\",
        L"backslash before quote\\\"x",
        L"Ünïcödé ✓ ★ 日本語",
    };
    std::wstring commandLine = L"app.exe";
    for (const auto &arg : args) {
        commandLine += L" " + quoteArgument(arg);
    }
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(commandLine.c_str(), &argc);
    CHECK_EQ(static_cast<size_t>(argc), args.size() + 1);
    for (size_t i = 0; i < std::min(args.size(), static_cast<size_t>(argc) - 1); ++i) {
        CHECK_EQ(std::wstring(argv[i + 1]), args[i]);
    }
    LocalFree(argv);
}

int wmain()
{
    setupUnicodeConsole();
    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED))) {
        std::wcerr << L"RoInitialize failed" << std::endl;
        return 1;
    }
    const int result = runTests();
    RoUninitialize();
    return result;
}
