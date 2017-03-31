using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Display;
using Windows.UI;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Devices.Enumeration;
using Windows.Devices.HumanInterfaceDevice;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace GazeHidTest
{
    class GazeData
    {
        public int Timestamp;
        public float X;
        public float Y;
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        const int HID_USAGE_PAGE_EYE_HEAD_TRACKER = 0x11;
        const int HID_USAGE_EYE_TRACKER = 0x01;

        const int HID_USAGE_TRACKING_DATA = 0x10;
        const int HID_USAGE_CAPABILITIES = 0x11;
        const int HID_USAGE_CONFIGURATION  = 0x12;
        const int HID_USAGE_TRACKER_STATUS = 0x13;
        const int HID_USAGE_TRACKER_CONTROL = 0x14;

        const int HID_USAGE_TIMESTAMP = 0x20;
        const int HID_USAGE_POSITION_X = 0x21;
        const int HID_USAGE_POSITION_Y = 0x22;
        const int HID_USAGE_POSITION_Z = 0x23;
        const int HID_USAGE_GAZE_LOCATION = 0x24;
        const int HID_USAGE_LEFT_EYE_POSITION = 0x25;
        const int HID_USAGE_RIGHT_EYE_POSITION = 0x26;
        const int HID_USAGE_HEAD_POSITION = 0x27;

        HidDevice _eyeTracker;
        SolidColorBrush _backgroundBrush;
        SolidColorBrush _foregroundBrush;
        Button _curButton;

        int _screenWidth;
        int _screenHeight;
        int _numRows;
        int _numCols;


        public MainPage()
        {
            this.InitializeComponent();
            Loaded += OnMainPageLoaded;
            KeyDown += OnMainPageKeyDown;

            _numRows = 2;
            _numCols = 2;

            _backgroundBrush = new SolidColorBrush(Colors.Gray);
            _foregroundBrush = new SolidColorBrush(Colors.Blue);
            ApplicationView.PreferredLaunchWindowingMode = ApplicationViewWindowingMode.FullScreen;
        }

        private void OnMainPageKeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Left)
            {
                _numCols = (_numCols > 1) ? _numCols-- : _numCols;
            }
            else if (e.Key == Windows.System.VirtualKey.Right)
            {
                _numCols++;
            }
            else if (e.Key == Windows.System.VirtualKey.Up)
            {
                _numRows++;
            }
            else if (e.Key == Windows.System.VirtualKey.Down)
            {
                _numRows = (_numRows > 1) ? _numRows-- : _numRows;
            }
            InitializeGrid(_numRows, _numCols);
        }

        void OnMainPageLoaded(object sender, RoutedEventArgs e)
        {
            InitializeScreenDimensions();
            InitializeGrid(_numRows, _numCols);
            InitializeHid();
        }

        void InitializeScreenDimensions()
        {
            var scale = DisplayInformation.GetForCurrentView().ResolutionScale;
            _screenWidth = (int)(this.ActualWidth * (float)scale / 100);
            _screenHeight = (int)(this.ActualHeight * (float)scale / 100);
        }

        async void InitializeHid()
        {
            string selector = HidDevice.GetDeviceSelector(HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_EYE_TRACKER);
            var devices = await DeviceInformation.FindAllAsync(selector);
            if (devices.Count <= 0)
            {
                // TODO: Show appropriate error message
                return;
            }

            _eyeTracker = await HidDevice.FromIdAsync(devices.ElementAt(0).Id, Windows.Storage.FileAccessMode.ReadWrite);
            _eyeTracker.InputReportReceived += OnGazeReportReceived;
        }

        void InitializeGrid(int rows, int cols)
        {
            AppGrid.RowDefinitions.Clear();
            AppGrid.ColumnDefinitions.Clear();
            for (int row = 0; row < rows; row++)
            {
                AppGrid.RowDefinitions.Add(new RowDefinition());
            }

            for (int col = 0; col < cols; col++)
            {
                AppGrid.ColumnDefinitions.Add(new ColumnDefinition());
            }

            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols; col++)
                {
                    var button = new Button();
                    button.Name = "b" + "_" + col + "_" + row;
                    button.Content = button.Name;
                    button.Background = _backgroundBrush;
                    button.VerticalAlignment = VerticalAlignment.Center;
                    button.HorizontalAlignment = HorizontalAlignment.Center;
                    button.Click += OnGridButtonClick;
                    button.Width = this.ActualWidth / cols;
                    button.Height = this.ActualHeight / rows;

                    Grid.SetColumn(button, col);
                    Grid.SetRow(button, row);
                    AppGrid.Children.Add(button);
                }
            }
        }

        private void OnGridButtonClick(object sender, RoutedEventArgs e)
        {
            Debug.WriteLine($"{(sender as Button).Name}");
        }
            

        void HandleGazeData(GazeData gazeData)
        {
            var screenPoint = new Point(gazeData.X * _screenWidth, gazeData.Y * _screenHeight);
            Debug.WriteLine($"{gazeData.Timestamp}: [{screenPoint}]");

            var elements = VisualTreeHelper.FindElementsInHostCoordinates(screenPoint, this);
            foreach (FrameworkElement element in elements)
            {
                var button = element as Button;
                if (button != null)
                {
                    if (_curButton != null)
                    {
                        _curButton.Background = _backgroundBrush;
                    }
                    button.Background = _foregroundBrush;
                    _curButton = button;
                    break;
                }
            }
        }

        void OnGazeReportReceived(HidDevice sender, HidInputReportReceivedEventArgs args)
        {
            var report = args.Report;
            Debug.Assert(report.Id == HID_USAGE_TRACKING_DATA);

            HidNumericControl num;

            num = report.GetNumericControl(HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_TIMESTAMP);
            var timestamp = (int)num.Value;

            num = report.GetNumericControl(HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_POSITION_X);
            var x = BitConverter.ToSingle(BitConverter.GetBytes((int)num.Value), 0);

            num = report.GetNumericControl(HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_POSITION_Y);
            var y = BitConverter.ToSingle(BitConverter.GetBytes((int)num.Value), 0);

            var gazeData = new GazeData { Timestamp = timestamp, X = x, Y = y };

            var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                HandleGazeData(gazeData);
            });

        }
    }
}
