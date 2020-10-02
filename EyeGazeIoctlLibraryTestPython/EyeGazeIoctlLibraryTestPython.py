# This is a sample Python script.

# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.


from ctypes import *
import math
import time
import pandas

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    eyeGazeIoctlDll = cdll.LoadLibrary("..\\x64\\Debug\\EyeGazeIoctlLibrary.dll")
    eyeGazeIoctlDll.InitializeEyeGaze()

    user32 = windll.user32
    screensize = user32.GetSystemMetrics(0), user32.GetSystemMetrics(1)
    monitorWidthUm = eyeGazeIoctlDll.GetPrimaryMonitorWidth()
    monitorHeightUm = eyeGazeIoctlDll.GetPrimaryMonitorHeight()

    xMonitorRatio = monitorWidthUm / screensize[0]
    yMonitorRatio = monitorHeightUm / screensize[1]

    x = 0
    y = 0
    velocity = 125
    angle = 45.0

    while True:
        timestamp = c_int64(pandas.Timestamp.utcnow().to_datetime64())

        print("SendGazeReport[", x, ", ", y, ", ", timestamp, "]")
        eyeGazeIoctlDll.SendGazeReport(int(x), int(y), timestamp)

        time.sleep(0.01)

        # Generate bouncing test pattern
        x += int(velocity * math.cos(angle * math.pi / 180))
        y += int(velocity * math.sin(angle * math.pi / 180))

        if x < 0 or x > monitorWidthUm:
            angle = 180 - angle
        elif y < 0 or y > monitorHeightUm:
            angle = 360 - angle
