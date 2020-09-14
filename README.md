# Eye Tracker HID Reference Implementation

This is a reference implementation to match the HID specification for eye trackers. The code 
presents both a DMF/VHF based implementation as well as a UMDF2 VHIDMINI based implementation. 
There are also a number of sample driver implementations as well as test tools to assist in 
understanding and further development.

[EyeGazeIoctl](Documentation/EyeGazeIoctl.md) - This implementation uses modern frameworks to 
produce a driver which can be sent data from a User Mode application. This implementation allows
for data to easily be provided from a number of sources.

[swdevice/vhidmini](Documentation/swdevice_vhidmini.md) - This implementation is based on the [VHIDMINI2 
sample](https://github.com/microsoft/Windows-driver-samples/tree/master/hid/vhidmini2). It provides a
number of sample driver implementations for various eye tracker manufacturers.

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
