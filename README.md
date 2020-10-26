# Eye Tracker HID Reference Implementation

This is a reference implementation to match the [HID specification](https://www.usb.org/sites/default/files/hut1_2.pdf#page=183) for eye trackers. The code 
presents both a DMF/VHF based implementation as well as a UMDF2 VHIDMINI based implementation. 
There are also a number of sample driver implementations as well as test tools to assist in 
understanding and further development.

## Driver Implementations

[EyeGazeIoctl](/Documentation/EyeGazeIoctl.md) - This implementation uses modern frameworks to 
produce a driver which can be sent data from a User Mode application. This implementation allows
for data to easily be provided from a number of sources.

[swdevice/vhidmini](/Documentation/swdevice_vhidmini.md) - This implementation is based on the [VHIDMINI2 
sample](https://github.com/microsoft/Windows-driver-samples/tree/master/hid/vhidmini2). It provides a
number of sample driver implementations for various eye tracker manufacturers.

## Technical Notes Beyond Basic Driver Implementation

*eyegaze:calibrate* - Many eye trackers require a calibration routine to function properly. The *GazeDevicePreview* API 
provides a [GazeDevicePreview.RequestCalibrationAsync Method](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazedevicepreview.requestcalibrationasync?view=winrt-19041)
for this purpose. When the client calls this device, *RequestCalibrationAsync()* will make a request to the URI *eyegaze:calibrate*. 
The OEM can use this request to launch their calibration application. Note that the HID specification provides for "TRACKER_STATUS_CONFIGURING", 
such that during calibration it is advisable to change the status. Once calibration is complete, a similar status change back
to TRACKER_STATUS_READY would be expected. *EyeGazeProtocolHandler* is a simple example of how to have an application register for this
protocol scheme. A simple way to test is to open a browser and put *eyegaze:calibrate* into the URI box - the browser (Edge, Chrome) will
try to launch the associated application.

*[GazeInputSourcePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazeinputsourcepreview?view=winrt-19041)/[GazeDevicePreview](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazedevicepreview?view=winrt-19041)* - 
Windows has lower level APIs available for accessing the raw X/Y coordinates from the Eye Tracker. 

*[Gaze Interaction Library](https://aka.ms/gazeinteractiondocs)* - This library provides a higher level api for Eye Trackers. Specifically,
the API allows a developer to eye gaze enable controls such as buttons. When a user looks at a control then the API will provide
various events that can be used for both animation and activation. A common use case is to eye gaze enable a button to show a series of 
animations during gaze enter, fixation, dwell, and activation. The API uses filtering to stabalize the signal. Rather than 
dealing with X/Y coordinates directly, the *Gaze Interation Library* allows the developer to focus on control activation events
such as *OnClick* similar to a typical application.

## Data Flow Diagram

Many eye tracking systems are based around commodity IR cameras such as those available
from [FLIR (formerly Point Grey)](https://www.flir.com). The IR camera feed is passed to
the camera driver before being processed by custom image processing software. The image 
processing software is generally in the form of a windows service and provides an API for applications
to consume. While each eye gaze system is unique, all provide a stream of X/Y coordinates
to represent eye gaze data.

The drivers in this repository represent two methods for communicating with the custom API.
- The [EyeGazeIoctl](/Documentation/EyeGazeIoctl.md) exposes IOCTLs, which are essentially function
calls. A seperate process, perhaps part of the same service providing the custom API, can hook into
the driver and send IOCTLs.
- The [swdevice/vhidmini](/Documentation/swdevice_vhidmini.md) solution has UMDF drivers which communicate 
directly with the custom API. This method is more self-contained, but brings with it the challenges
of UMDF driver development.

Once the gaze data has been provided to the appropriate driver, Windows will then
take that data and plumb it through the various internal structures to the consuming applications.

Beyond supplying the gaze data stream, the other major function provided by the custom API 
is the ability to calibrate the eye tracking system. The windows APIs utilize the `eyegaze:calibrate`
protocol scheme to launch a calibration experience.

![Gaze Data Flow](/Documentation/assets/images/Gaze_Data_Flow.png)

## Driver State Flow Diagram

The diagram below presents a general state flow of the driver during its lifetime. When initially
booted the driver should initially start in a state that requires various setup data. Once the screen
and user data have been provided it can transition to the *Ready* state.

At various times in the lifecycle of the driver it may be necessary to 
transition to other states. One such example is if the user requests recalibration. In that case, the
calibration experience should signal a state change to the driver when starting calibration
as well as when it is complete.

![Driver State Flow](/Documentation/assets/images/Driver_State_Flow.png)

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

## Debugging Resources

Debugging drivers can be be difficult. However, through the process of developing these drivers we found a number 
of tools and strategies which helped. Please see the [debugging resources](/Documentation/debugging_resources.md) document
for more information.

## Test Signing Drivers

To use the drivers without the need for a kernel debugger, you can utilize [Test Signing](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/installing-a-test-signed-driver-package-on-the-test-computer). The 
documentation for test signing is extensive and includes important information regarding the path to releasing a driver. For simplicity, the steps
below are provided for reference. The instructions below are used for the *EyeGazeIoctl* driver, but can be modified for use with the other drivers
in this repository.

To utilize test signing, you will need to enable it on your system. From an administrator command prompt run the following command and then restart. Note
that you may need to [disable Secure Boot](https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/disabling-secure-boot) in order for the command to work.
```
Bcdedit.exe -set TESTSIGNING ON
```

Next we need to sign the driver. From a Visual Studio Developer Command prompt, run the following commands from the build output directory of the `EyeGazeIoctl` driver in `Dmf/DmfSamples`:
```
makecert -r -pe -ss PrivateCertStore -n CN=GazeHID_Test -eku 1.3.6.1.5.5.7.3.3 GazeHID_Test.cer
certmgr /add GazeHID_Test.cer /s /r localMachine root
stampinf -f .\EyeGazeIoctl.inf -d 10/23/2020 -v 1.0.0000.0
inf2cat /driver:. /os:Vista_x64
SignTool sign /v /s PrivateCertStore /n GazeHID_Test /t http://timestamp.digicert.com EyeGazeIoctl.sys
Signtool sign /v /fd sha256 /s PrivateCertStore /n GazeHID_Test /t http://timestamp.digicert.com /a EyeGazeIoctl.cat
Signtool verify /pa /v EyeGazeIoctl.cat
```
It is expected that you will get an error indicating `SignTool Error: A certificate chain processed, but terminated in a root certificate which is not trusted by the trust provider.` This
is due to the certificate being self-signed. The error can be ignored for testing purposes.

Finally, we need to install the driver. Visual Studio Developer Command prompt, run the following commands from the build output directory of the `EyeGazeIoctl` driver in `Dmf/DmfSamples`:
```
."C:\Program Files (x86)\Windows Kits\10\Tools\x64\devcon.exe" install EyeGazeIoctl.inf root\EyeGazeIoctl
```

If successful, you should see `Drivers installed successfully` and can find the driver in device manager under *Sample Device->EyeGazeIoctl Device*.

## Updating VID/PID

The sample drivers utilize a sample Vendor ID of `0xFEED` and Product ID of `0xDEED`. These values *MUST* be updated as a part of any release
process. 
- If using the *EyeGazeIoctl* driver, please update `dmf\DmfSamples\EyeGazeIoctl\DmfInterface.c`.
- If using the *VHIDMINI2* drivers, please update `GazeHid\drivers\driver.h` 



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
