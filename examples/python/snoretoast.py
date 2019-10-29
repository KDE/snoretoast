import ctypes
import os
import subprocess
import sys

PIPE_NAME = r"\\.\PIPE\snorepy"
APP_ID = "SoreToast.Example.Python"

# install a shortcut with a app id, this will change the displayed origin of the notification, in the notification and the action center
# for different ways how to provide such an app id, have a look at the readme
subprocess.run(["snoretoast", "-install", "Snoretoast Python Example", sys.executable, APP_ID])


bufsize = 1024
handle = ctypes.windll.kernel32.CreateNamedPipeW(PIPE_NAME, 0x00000001, 0, 1, bufsize, bufsize, 0, None)
if handle == -1:
    print("Error")
    exit(-1)
subprocess.run(["snoretoast", "-t", "Snoretoast ❤ python", "-m", "This rocks", "-b", "🎸;This;❤", "-p", os.path.join(os.path.dirname(__file__), "snoretoast-python.png"),
                 "-pipeName", PIPE_NAME, "-appID", APP_ID])

ctypes.windll.kernel32.ConnectNamedPipe(handle, None)

c_read = ctypes.c_int32(0)
buff = ctypes.create_unicode_buffer(bufsize + 1) 
ctypes.windll.kernel32.ReadFile(handle, buff, bufsize, ctypes.byref(c_read), None)
# NULL terminate it.
buff[c_read.value] = "\0"

data = dict((a,b) for a,b in [x.split("=", 1) for x in filter(None, buff.value.split(";"))])
if data["action"] == "buttonClicked":
    print("The user clicked the button: ", data["button"])
else:
    print(data["action"])


