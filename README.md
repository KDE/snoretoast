Snoretoast
==========
A command line application which is capable of creating Windows Toast notifications on Windows 8 or later.
If SnoreToast is used without the parameter --appID an default appID is used and a shortcut to SnoreToast.exe is created in the startmenu, please read more about Application User Model IDs on http://msdn.microsoft.com/en-us/library/windows/apps/dd378459.aspx .
If your application already has a shortcut in the startmenu, which provides an appID please pass this appID, an appID is needed and is also used to determine the application icon displayed in the toast notification.

If your application does not have a shortcut in the startmenu yet please use the --install parameter, for example "snoretoast.exe --install "MyApp\MyApp.lnk" "C:\myApp.exe" "My.APP_ID" " the appID provided here has to be passed to snoretoast with the parameter --appID.

# Releases and Binaries
Releases and binaries can be found [here](http://download.kde.org/stable/snoretoast/).


# SnoreToastGui
We provide two applications SnoreToast.exe and SnoreToastGui.exe.
SnoreToastGui.exe is a GUI application and won't display a command line window when executed.
SnoreToast.exe is a standard commandline application.


----------------------------------------------------------

    Welcome to SnoreToast 0.5.1.
    A command line application which is capable of creating Windows Toast notifications.

    ---- Usage ----
    SnoreToast [Options]

    ---- Options ----
    [-t] <title string>     | Displayed on the first line of the toast.
    [-m] <message string>   | Displayed on the remaining lines, wrapped.
    [-b] <button string>    | Displayed on the bottom line, can list multiple buttons separated by ;
    [-tb]                   | Displayed a textbox on the bottom line, only if buttons are not presented.
    [-p] <image URI>        | Display toast with an image, local files only.
    [-w]                    | Wait for toast to expire or activate.
    [-id] <id>              | sets the id for a notification to be able to close it later.
    [-s] <sound URI>        | Sets the sound of the notifications, for possible values see http://msdn.microsoft.com/en-us/library/windows/apps/hh761492.aspx.
    [-silent]               | Don't play a sound file when showing the notifications.
    [-appID] <App.ID>       | Don't create a shortcut but use the provided app id.
    -close <id>             | Closes a currently displayed notification, in order to be able to close a notification the parameter -w must be used to create the notification.

    -install <path> <application> <appID>| Creates a shortcut <path> in the start menu which point to the executable <application>, appID used for the notifications.

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
    This should go without saying, but windows style paths are required.
