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
#include "snoretoasts.h"
#include "linkhelper.h"

#include <string>
#include <iostream>

#include <roapi.h>

using namespace Windows::Foundation;


void help()
{
    std::wcout << L"Welcome to SnoreToast." << std::endl
               << L"Provide toast with a message and display it-" << std::endl
               << L"via the graphical notification system." << std::endl
               << L"This application is inspired by https://github.com/nels-o/toaster and has the same syntax in some parts" << std::endl
               << std::endl
               << L"---- Usage ----" << std::endl
               << L"toast [Options]" << std::endl
               << std::endl
               << L"---- Options ----" << std::endl
               << L"[-t] <title string>\t| Displayed on the first line of the toast." << std::endl
               << L"[-m] <message string>\t| Displayed on the remaining lines, wrapped." << std::endl
               << L"[-p] <image URI>\t| Display toast with an image" << std::endl
               << L"[-w] \t\t\t| Wait for toast to expire or activate." << std::endl
               << std::endl
               << L"The folowing arguments are only avalible in SnoreToast:" << std::endl
               << L"[-s] <sound URI> \t| Sets the sound of the notifications, for possible values see http://msdn.microsoft.com/en-us/library/windows/apps/hh761492.aspx." << std::endl
               << L"[-silent] \t\t| Don't play a sound file when showing the notifications." << std::endl
               << L"[-install] <path> <path to aplication> <APP.ID>\t| Set the path of your applications shortcut." << std::endl
               << L"[-appID] <App.ID>\t| Dpn't create a shortcut but use the provided app id." << std::endl
               << L"[-v] \t\t\t| Print the version an copying information." << std::endl
               << std::endl
               << L"?\t\t\t| Print these intructions. Same as no args." << std::endl
               << L"Exit Status\t:  Exit Code" << std::endl
               << L"Failed\t\t: -1"
               << std::endl
               << "Success\t\t:  0" << std::endl
               << "Hidden\t\t:  1" << std::endl
               << "Dismissed\t\t:  2" << std::endl
               << "Timeout\t\t:  3" << std::endl
               << std::endl
               << L"---- Image Notes ----" << std::endl
               << L"Images must be .png with:" << std::endl
               << L"\tmaximum dimensions of 1024x1024" << std::endl
               << L"\tsize <= 200kb" << std::endl
               << L"These limitations are due to the Toast notification system." << std::endl
               << L"This should go without saying, but windows style paths are required." << std::endl;
}

void version()
{
    std::wcout << L"SnoreToast version 0.1" << std::endl
               << L"Copyright (C) 2013  Patrick von Reth <vonreth@kde.org>" << std::endl
               << L"SnoreToast is free software: you can redistribute it and/or modify" << std::endl
               << L"it under the terms of the GNU Lesser General Public License as published by" << std::endl
               << L"the Free Software Foundation, either version 3 of the License, or" << std::endl
               << L"(at your option) any later version." << std::endl;

}

SnoreToasts::USER_ACTION parse(wchar_t *in[],int len)
{
    HRESULT hr = S_OK;

    std::wstring appID;
    std::wstring title;
    std::wstring body;
    std::wstring image;
    std::wstring sound(L"Notification.Default");
    bool silent = false;
    bool  wait = false;
    bool showHelp = false;



    for(int i = 0;i<len;++i)
    {
        std::wstring arg(in[i]);
        if(arg == L"-m")
        {
            if (i + 1 < len)
            {
                body = in[i + 1];
            }
            else
            {
                std::wcout << L"Missing argument to -m.\n"
                              L"Supply argument as -m \"message string\"" << std::endl;
                showHelp = true;
            }
        }
        else if(arg == L"-t")
        {
            if (i + 1 < len)
            {
                title = in[i + 1];
            }
            else
            {
                std::wcout << L"Missing argument to -t.\n"
                              L"Supply argument as -t \"bold title string\"" << std::endl;
                showHelp = true;
            }
        }
        else if(arg == L"-p")
        {
            if (i + 1 < len)
            {
                std::wstring path = in[i + 1];
                if(path.at(1) == ':' )//abspath
                {
                    image = L"file:///";
                    image.append(path);
                }
                else if(path.substr(0,4) == L"http")//url not working?
                {
                    image = path;
                }
                else
                {
                    image = _wfullpath(nullptr, in[i + 1], MAX_PATH);
                }
            }
            else
            {
                std::wcout << L"Missing argument to -p."
                              L"Supply argument as -p \"image path\"" << std::endl;
                showHelp = true;
            }
        }
        else  if(arg == L"-w")
        {
            wait = true;
        }
        else  if(arg == L"-s")
        {
            if (i + 1 < len)
            {
                sound = in[i + 1];
            }
            else
            {
                std::wcout << L"Missing argument to -s.\n"
                              L"Supply argument as -s \"sound name\" " << std::endl;
                showHelp = true;
            }
        }
        else  if(arg == L"-silent")
        {
            silent = true;
        }
        else  if(arg == L"-appID")
        {
            if (i + 1 < len)
            {
                appID = in[i + 1];
            }
            else
            {
                std::wcout << L"Missing argument to -a.\n"
                              L"Supply argument as -a Your.APP.ID" << std::endl;
                showHelp = true;
            }
        }
        else  if(arg == L"-install")
        {
            if (i + 3 < len)
            {
                std::wstring shortcut(in[i + 1]) ;
                std::wstring exe(in[i + 2]);
                appID = in[i + 3];
                return SUCCEEDED(LinkHelper::tryCreateShortcut(shortcut,exe,appID)) ? SnoreToasts::Success : SnoreToasts::Failed;
            }
            else
            {
                std::wcout << L"Missing argument to -install.\n"
                              L"Supply argument as -install \"path to your shortcut\" \"path to the aplication the shortcut should point to\" \"App.ID\"" << std::endl;
                showHelp = true;
            }
        }
        else  if(arg == L"-v")
        {
            version();
            return SnoreToasts::Success;
        }
    }
    hr = (!showHelp && title.length() > 0  && body.length() > 0)?S_OK:E_FAIL;
    if(SUCCEEDED(hr))
    {
        if(appID.length() == 0)
        {
			appID = L"Snore.DesktopToasts";
            hr = LinkHelper::tryCreateShortcut(appID);
        }
        if(SUCCEEDED(hr))
        {
            SnoreToasts app(appID);
            app.setSilent(silent);
            app.setSound(sound);
            app.displayToast(title,body,image,wait);
            return app.userAction();
        }
    }
    else
    {
        help();
        return SnoreToasts::Success;
    }


    return SnoreToasts::Failed;
}

// Main function
int main()
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    SnoreToasts::USER_ACTION action = SnoreToasts::Success;

    HRESULT hr = Initialize(RO_INIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        action = parse(argv,argc);
        Uninitialize();
    }

    return action;
}

