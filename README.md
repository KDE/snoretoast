Snoretoast
==========
A command line application which is capable of creating Windows Toast notifications on Windows 8 or later.
If SnoreToast is used without the parameter --appID an default appID is used and a shortcut to SnoreToast.exe is created in the startmenu, notifications created that way will be asigned to SnoreToast.

If you wan't to brand your notifications you need to create the application startmenu entry with `snoretoast.exe --install "MyApp\MyApp.lnk" "C:\myApp.exe" "My.APP_ID"`.
This appID then needs to be passed to snoretoast.exe with the `--appID`` parameter.

# Releases and Binaries
Releases and binaries can be found [here](https://binary-factory.kde.org/job/SnoreToast_Nightly_win64/).

# Installing icons with nsis
```
!include LogicLib.nsh
!include WordFunc.nsh

Function SnoreWinVer
    ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
    ${VersionCompare} "6.2" $R0 $R0
    ${If} $R0 == 1
        Push "NotWin8"
    ${Else}
        Push "AtLeastWin8"
    ${EndIf}
FunctionEnd

!macro SnoreShortcut path exe appID
    Call SnoreWinVer
    Pop $0
    ${If} $0 == "AtLeastWin8"
        nsExec::ExecToLog '"${SnoreToastExe}" -install "${path}" "${exe}" "${appID}"'
    ${Else}
        CreateShortCut "${path}" "${exe}"
    ${EndIf}
!macroend

```


----------------------------------------------------------
```
Welcome to SnoreToast 0.5.99.
A command line application which is capable of creating Windows Toast notifications.

---- Usage ----
SnoreToast [Options]

---- Options ----
[-t] <title string>     | Displayed on the first line of the toast.
[-m] <message string>   | Displayed on the remaining lines, wrapped.
[-b] <button1;button2 string>| Displayed on the bottom line, can list multiple buttons separated by ;
[-tb]                   | Displayed a textbox on the bottom line, only if buttons are not presented.
[-p] <image URI>        | Display toast with an image, local files only.
[-id] <id>              | sets the id for a notification to be able to close it later.
[-s] <sound URI>        | Sets the sound of the notifications, for possible values see http://msdn.microsoft.com/en-us/library/windows/apps/hh761492.aspx.
[-silent]               | Don't play a sound file when showing the notifications.
[-appID] <App.ID>       | Don't create a shortcut but use the provided app id.
[-pipeName] <\.\pipe\pipeName\> | Provide a name pipe which is used for callbacks.
[-application] <C:\foo.exe>     | Provide a application that might be started if the pipe does not exist.
-close <id>             | Closes a currently displayed notification.

-install <name> <application> <appID>| Creates a shortcut <name> in the start menu which point to the executable <application>, appID used for the notifications.

-v                      | Print the version and copying information.
-h                      | Print these instructions. Same as no args.
Exit Status     :  Exit Code
Failed          : -1

Success         :  0
Hidden          :  1
Dismissed       :  2
TimedOut        :  3
ButtonPressed   :  4
TextEntered     :  5

---- Image Notes ----
Images must be .png with:
        maximum dimensions of 1024x1024
        size <= 200kb
These limitations are due to the Toast notification system.
```
