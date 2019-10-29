import ctypes
import os
import subprocess
import sys
import time
import threading

PIPE_NAME = r"\\.\PIPE\snorepy"
APP_ID = "SnoreToast.Example.Python"
BUF_SIZE = 1024

def server():
    while True:
        handle = ctypes.windll.kernel32.CreateNamedPipeW(PIPE_NAME, 0x00000001,  0, 255, BUF_SIZE, BUF_SIZE, 0, None)
        if handle == -1:
            print("Error")
            exit(-1)
        ctypes.windll.kernel32.ConnectNamedPipe(handle, None)
        c_read = ctypes.c_int32(0)
        buff = ctypes.create_unicode_buffer(BUF_SIZE + 1)
        ctypes.windll.kernel32.ReadFile(handle, buff, BUF_SIZE, ctypes.byref(c_read), None)
        ctypes.windll.kernel32.CloseHandle(handle)
        if c_read:
            # NULL terminate it.
            buff[c_read.value] = "\0"

            dataString = buff.value
            print(dataString)
            data = dict((a,b) for a,b in [x.split("=", 1) for x in filter(None, dataString.split(";"))])
            print("Callback from:", data["notificationId"])
            if data["action"] == "buttonClicked":
                print("The user clicked the button: ", data["button"])
            else:
                print(data["action"])



def run(args):
    print(" ".join(args))
    subprocess.run(args)

serverThread = None
try:
    # install a shortcut with a app id, this will change the displayed origin of the notification, in the notification and the action center
    # for different ways how to provide such an app id, have a look at the readme
    run(["snoretoast", "-install", "Snoretoast Python Example", sys.executable, APP_ID])


    # start a server, the server will receive callbacks from the active notification and notifications interacted with in the action center
    serverThread = threading.Thread(target=server, daemon=True)
    serverThread.start()
    for i in range(10):
        run(["snoretoast", "-t", "Snoretoast ❤ python", "-m", "This rocks", "-b", "🎸;This;❤;" + str(i), "-p", os.path.join(os.path.dirname(__file__), "snoretoast-python.png"),
             "-id", str(i), "-pipeName", PIPE_NAME, "-appID", APP_ID])

    while True:
        # let the server continue but wait for a keyboard interupt
        time.sleep(10000)
finally:
    if serverThread:
        del serverThread
