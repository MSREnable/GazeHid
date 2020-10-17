
using Windows.UI.Xaml.Controls;

namespace EyeGazeProtocolHandler
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            if (!string.IsNullOrEmpty(UriData.AbsoluteUri))
            {
                MessageBlock.Text = $"Activated via protocol handler, uri: {UriData.AbsoluteUri}";
            }
        }
    }

    // TODO Actually do calibration
    // 1. Transition the driver Configuraiton into USER_CALIBRATION_NEEDED state
    // 2. Run your calibration
    // 3. Transition the driver Configuration into READY state
    // 4. Close the calibration app

    public static class UriData
    {
        public static string AbsoluteUri { get; set; }
    }
}
