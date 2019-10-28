import subprocess
import ctypes

PIPE_NAME = r"\\.\PIPE\snorepy"
bufsize = 1024
handle = ctypes.windll.kernel32.CreateNamedPipeW(PIPE_NAME, 0x00000001, 0, 1, bufsize, bufsize, 0, None)
if handle == -1:
    print("Error")
    exit(-1)
subprocess.run(["snoretoast", "-t", "Snore loves python♥", "-m", "This rocks", "-b", "Rock;This", "-pipeName", PIPE_NAME])

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


