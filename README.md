# Eye Tracker HID Reference Implementation

This is a reference implementation to match the HID specification for eye trackers. The code 
presents both a DMF/VHF based implementation as well as a UMDF2 VHIDMINI based implementation. 
There are also a number of sample driver implementations as well as test tools to assist in 
understanding and further development.

## Driver Implementations

[EyeGazeIoctl](Documentation/EyeGazeIoctl.md) - This implementation uses modern frameworks to 
produce a driver which can be sent data from a User Mode application. This implementation allows
for data to easily be provided from a number of sources.

[swdevice/vhidmini](Documentation/swdevice_vhidmini.md) - This implementation is based on the [VHIDMINI2 
sample](https://github.com/microsoft/Windows-driver-samples/tree/master/hid/vhidmini2). It provides a
number of sample driver implementations for various eye tracker manufacturers.

## Technical Notes Beyond Basic Driver Implementation

*eyegaze:calibrate* - Many eye trackers require a calibration routine to function properly. The *GazeDevicePreview* API 
provides a [GazeDevicePreview.RequestCalibrationAsync Method](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazedevicepreview.requestcalibrationasync?view=winrt-19041)
for this purpose. When the client calls this device, *RequestCalibrationAsync()* will make a request to the URI *eyegaze:calibrate*. 
The OEM can use this request to launch their calibration application. Note that the HID specification provides for "TRACKER_STATUS_CONFIGURING", 
such that during calibration it is advisable to change the status. Once calibration is complete, a similar status change back
to TRACKER_STATUS_READY would be expected.

*[GazeInputSourcePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazeinputsourcepreview?view=winrt-19041)/[GazeDevicePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazedevicepreview?view=winrt-19041)* - 
Windows has lower level APIs available for accessing the raw X/Y coordinates from the Eye Tracker. 

*[Gaze Interaction Library](https://aka.ms/gazeinteractiondocs)* - This library provides a higher level api for Eye Trackers. Specifically,
the API allows a developer to eye gaze enable controls such as buttons. When a user looks at a control then the API will provide
various events that can be used for both animation and activation. A common use case is to eye gaze enable a button to show a series of 
animations during gaze enter, fixation, dwell, and activation. The API uses filtering to stabalize the signal. Rather than 
dealing with X/Y coordinates directly, the *Gaze Interation Library* allows the developer to focus on control activation events
such as *OnClick* similar to a typical application.

## Testing Tools

*GazeTracing* - This tool uses the [GazeInputSourcePreview.GazeMoved Event](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazeinputsourcepreview.gazemoved?view=winrt-19041)
to access the gaze data from a UWP application. The implementation is similar to the *Gaze Tracing* functionality
demonstrated in the [Windows Community Toolkit Sample App](https://www.microsoft.com/en-us/p/windows-community-toolkit-sample-app/9nblggh4tlcq). 
The application will draw dots where it detects your eye gaze, and leave the last 100 datapoints on the screen.

*testvhid* - This tool uses Windows APIs to inspect the HID data structures reported by the driver.

*GazeHidTest* - UWP app which provides a basic grid of buttons and obtains data from the Hid report directly.

## Additional Testing Resources

*[Windows Community Toolkit Sample App](https://www.microsoft.com/en-us/p/windows-community-toolkit-sample-app/9nblggh4tlcq)* - The sample application
provides demonstrations of both lower level Eye Tracker data using [GazeInputSourcePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazeinputsourcepreview?view=winrt-19041) 
as well as higher level implementations using the [Gaze Interaction Library](https://aka.ms/gazeinteractiondocs)

*[Gaze Interactions and Eye Tracking in Windows Apps](https://docs.microsoft.com/en-us/windows/uwp/design/input/gaze-interactions)* - Additional documentation
regarding the use of the [GazeInputSourcePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazeinputsourcepreview?view=winrt-19041) apis.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
