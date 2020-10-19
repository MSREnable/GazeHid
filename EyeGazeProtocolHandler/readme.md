# EyeGazeProtocolHandler

The [GazeDevicePreview.RequestCalibrationAsync()](https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.preview.gazedevicepreview.requestcalibrationasync?view=winrt-19041)
method call is able to start calibration by making a request to the `eyegaze:calibrate` protocol
scheme. This example is a UWP implementation of a simple application which registers as a 
protocol handler for the `eyegaze:` scheme and is able to interpret the argument accordingly.

Note that launching this application directly is handled differently than when
it has been launched from URI activation. This behavior can be useful depending on
your situation.