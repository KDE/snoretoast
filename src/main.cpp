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
#include "snoretoasts.h"
#include "config.h"

#include "toasteventhandler.h"

#include "snoretoastactioncenterintegration.h"

#include "linkhelper.h"
#include "utils.h"

#include <cmrc/cmrc.hpp>

#include <appmodel.h>
#include <shellapi.h>
#include <roapi.h>

#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

CMRC_DECLARE(SnoreToastResource);

std::wstring getAppId(const std::wstring &pid, const std::wstring &fallbackAppID)
{
    if (pid.empty()) {
        return fallbackAppID;
    }
    const int _pid = std::stoi(pid);
    const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, _pid);
    if (!process) {
        tLog << "Failed to retreive appid for " << _pid
             << " Failed to retrive process hanlde: " << Utils::formatWinError(GetLastError());
        return fallbackAppID;
    }
    uint32_t size = 0;
    long rc = GetApplicationUserModelId(process, &size, nullptr);
    if (rc != ERROR_INSUFFICIENT_BUFFER) {
        if (rc == APPMODEL_ERROR_NO_APPLICATION) {
            tLog << "Failed to retreive appid for " << _pid << " Process is a desktop application";
        } else {
            tLog << "Failed to retreive appid for " << _pid
                 << " Error: " << Utils::formatWinError(rc);
        }
        CloseHandle(process);
        return fallbackAppID;
    }
    std::wstring out(size, 0);
    rc = GetApplicationUserModelId(process, &size, out.data());
    CloseHandle(process);
    if (rc != ERROR_SUCCESS) {
        tLog << "Failed to retreive appid for " << _pid << " Error: " << Utils::formatWinError(rc);
        return fallbackAppID;
    }
    // strip 0
    out.resize(out.size() - 1);
    tLog << "AppId from pid" << out;
    return out;
}

void help(const std::wstring &error)
{
    if (!error.empty()) {
        std::wcerr << error << std::endl;
    } else {
        std::wcerr << L"Welcome to SnoreToast " << SnoreToasts::version() << "." << std::endl
                   << L"A command line application capable of creating Windows Toast notifications."
                   << std::endl;
    }
    const auto filesystem = cmrc::SnoreToastResource::get_filesystem();
    const auto help = filesystem.open("help.txt");
    std::wcerr << help.begin() << std::endl;
}

void version()
{
    std::wcerr << L"SnoreToast version " << SnoreToasts::version() << std::endl
               << L"ToastActivatorCLSID: " << SNORETOAST_CALLBACK_GUID << std::endl
               << L"Copyright (C) 2022  Hannah von Reth <vonreth@kde.org>" << std::endl
               << L"SnoreToast is free software: you can redistribute it and/or modify" << std::endl
               << L"it under the terms of the GNU Lesser General Public License as published by"
               << std::endl
               << L"the Free Software Foundation, either version 3 of the License, or" << std::endl
               << L"(at your option) any later version." << std::endl;
}

std::filesystem::path getIcon()
{
    auto image = std::filesystem::temp_directory_path() / "snoretoast" / SnoreToasts::version()
            / "logo.png";
    if (!std::filesystem::exists(image)) {
        std::filesystem::create_directories(image.parent_path());
        const auto filesystem = cmrc::SnoreToastResource::get_filesystem();
        const auto img = filesystem.open("256-256-snoretoast.png");
        std::ofstream out(image, std::ios::binary);
        out.write(const_cast<char *>(img.begin()), img.size());
        out.close();
    }
    return image;
}

SnoreToastActions::Actions parse(std::vector<wchar_t *> args)
{
    HRESULT hr = S_OK;

    std::wstring appID;
    std::wstring pid;
    std::filesystem::path pipe;
    std::filesystem::path application;
    std::wstring title;
    std::wstring body;
    std::filesystem::path image;
    std::wstring id;
    std::wstring sound(L"Notification.Default");
    std::wstring buttons;
    Duration duration = Duration::Short;
    bool silent = false;
    bool closeNotify = false;
    bool isTextBoxEnabled = false;

    auto nextArg = [&](std::vector<wchar_t *>::const_iterator &it,
                       const std::wstring &helpText) -> std::wstring {
        if (it != args.cend()) {
            return *it++;
        } else {
            help(helpText);
            exit(static_cast<int>(SnoreToastActions::Actions::Error));
        }
    };

    auto it = args.begin() + 1;
    while (it != args.end()) {
        std::wstring arg(nextArg(it, L""));
        std::transform(arg.begin(), arg.end(), arg.begin(),
                       [](int i) -> int { return ::tolower(i); });
        if (arg == L"-m") {
            body = nextArg(it,
                           L"Missing argument to -m.\n"
                           L"Supply argument as -m \"message string\"");
        } else if (arg == L"-t") {
            title = nextArg(it,
                            L"Missing argument to -t.\n"
                            L"Supply argument as -t \"bold title string\"");
        } else if (arg == L"-p") {
            image = nextArg(it, L"Missing argument to -p. Supply argument as -p \"image path\"");
        } else if (arg == L"-s") {
            sound = nextArg(it,
                            L"Missing argument to -s.\n"
                            L"Supply argument as -s \"sound name\"");
        } else if (arg == L"-d") {
            std::wstring _d = nextArg(it,
                                      L"Missing argument to -d.\n"
                                      L"Supply argument as -d (short |long)");
            if (_d == L"short") {
                duration = Duration::Short;
            } else if (_d == L"long") {
                duration = Duration::Long;
            } else {
                help(_d + L" is not a valid");
                return SnoreToastActions::Actions::Error;
            }
        } else if (arg == L"-id") {
            id = nextArg(it,
                         L"Missing argument to -id.\n"
                         L"Supply argument as -id \"id\"");
        } else if (arg == L"-silent") {
            silent = true;
        } else if (arg == L"-appid") {
            appID = nextArg(it,
                            L"Missing argument to -appID.\n"
                            L"Supply argument as -appID \"Your.APP.ID\"");
        } else if (arg == L"-pid") {
            pid = nextArg(it,
                          L"Missing argument to -pid.\n"
                          L"Supply argument as -pid \"pid\"");
        } else if (arg == L"-pipename") {
            pipe = nextArg(it,
                           L"Missing argument to -pipeName.\n"
                           L"Supply argument as -pipeName \"\\.\\pipe\\foo\\\"");
        } else if (arg == L"-application") {
            application = nextArg(it,
                                  L"Missing argument to -application.\n"
                                  L"Supply argument as -application \"C:\\foo.exe\"");
        } else if (arg == L"-b") {
            buttons = nextArg(it,
                              L"Missing argument to -b.\n"
                              L"Supply argument for buttons as -b \"button1;button2\"");
        } else if (arg == L"-tb") {
            isTextBoxEnabled = true;
        } else if (arg == L"-install") {
            const std::wstring shortcut(
                    nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\""));

            const std::wstring exe(
                    nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\""));

            appID = nextArg(it,
                            L"Missing argument to -install.\n"
                            L"Supply argument as -install \"path to your shortcut\" \"path to the "
                            L"application the shortcut should point to\" \"App.ID\"");

            return SUCCEEDED(LinkHelper::tryCreateShortcut(
                           shortcut, exe, appID, SnoreToastActionCenterIntegration::uuid()))
                    ? SnoreToastActions::Actions::Clicked
                    : SnoreToastActions::Actions::Error;
        } else if (arg == L"-close") {
            id = nextArg(it,
                         L"Missing agument to -close"
                         L"Supply argument as -close \"id\"");
            closeNotify = true;
        } else if (arg == L"-v") {
            version();
            return SnoreToastActions::Actions::Clicked;
        } else if (arg == L"-h") {
            help(L"");
            return SnoreToastActions::Actions::Clicked;
        } else {
            std::wstringstream ws;
            ws << L"Unknown argument: " << arg << std::endl;
            help(ws.str());
            return SnoreToastActions::Actions::Error;
        }
    }

    appID = getAppId(pid, appID);
    if (appID.empty()) {
        std::wstringstream _appID;
        _appID << L"Snore.DesktopToasts." << SnoreToasts::version();
        appID = _appID.str();
        hr = LinkHelper::tryCreateShortcut(std::filesystem::path(L"SnoreToast")
                                                   / SnoreToasts::version() / L"SnoreToast",
                                           appID, SnoreToastActionCenterIntegration::uuid());
        if (!SUCCEEDED(hr)) {
            return SnoreToastActions::Actions::Error;
        }
    }
    if (closeNotify) {
        if (!id.empty()) {
            SnoreToasts app(appID);
            app.setId(id);
            if (app.closeNotification()) {
                return SnoreToastActions::Actions::Clicked;
            }
        } else {
            help(L"Close only works if an -id id was provided.");
        }
    } else {
        hr = (title.length() > 0 && body.length() > 0) ? S_OK : E_FAIL;
        if (SUCCEEDED(hr)) {
            if (isTextBoxEnabled) {
                if (pipe.empty()) {
                    std::wcerr << L"TextBox notifications only work if a pipe for the result "
                                  L"was provided"
                               << std::endl;
                    return SnoreToastActions::Actions::Error;
                }
            }
            if (image.empty()) {
                image = getIcon();
            }
            SnoreToasts app(appID);
            app.setPipeName(pipe);
            app.setApplication(application);
            app.setSilent(silent);
            app.setSound(sound);
            app.setId(id);
            app.setButtons(buttons);
            app.setTextBoxEnabled(isTextBoxEnabled);
            app.setDuration(duration);
            app.displayToast(title, body, image);
            return app.userAction();
        } else {
            help(L"");
            return SnoreToastActions::Actions::Clicked;
        }
    }

    return SnoreToastActions::Actions::Error;
}

SnoreToastActions::Actions handleEmbedded()
{
    SnoreToasts::waitForCallbackActivation();
    return SnoreToastActions::Actions::Clicked;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t *, int)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // atache to the parent console output, if its an interactive terminal
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            FILE *dummy;
            _wfreopen_s(&dummy, L"CONOUT$", L"w", stdout);
            setvbuf(stdout, nullptr, _IONBF, 0);

            _wfreopen_s(&dummy, L"CONOUT$", L"w", stderr);
            setvbuf(stderr, nullptr, _IONBF, 0);
            std::ios::sync_with_stdio();
        }
    }

    const auto commandLine = GetCommandLineW();
    int argc;
    wchar_t **argv = CommandLineToArgvW(commandLine, &argc);

    tLog << commandLine;

    SnoreToastActions::Actions action = SnoreToastActions::Actions::Clicked;
    HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        if (std::wstring(commandLine).find(L"-Embedding") != std::wstring::npos) {
            action = handleEmbedded();
        } else {
            action = parse(std::vector<wchar_t *>(argv, argv + argc));
        }
        Windows::Foundation::Uninitialize();
    }

    return static_cast<int>(action);
}
