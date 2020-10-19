# EyeGazeIoctlLibrary

This is a DLL that provides a simplified interface for the [EyeGazeIoctl Driver](/Documentation/EyeGazeIoctl.md). The DLL
allows consumers, such as other [C++](../EyeGazeIoctlLibraryTestC/readme.md) or [Python](../EyeGazeIoctlLibraryTestPython/readme.md) applications, to 
easily send eye gaze data to the EyeGazeIoctl driver. 

The primary use case for the DLL is:

1. Call InitializeEyeGaze()
2. Call SendGazeReport(x, y, timestamp)

The DLL will initialize the EyeGazeIoctl driver with reasonable defaults, then 
accept any SendGazeReport() calls and pass them along to the driver.