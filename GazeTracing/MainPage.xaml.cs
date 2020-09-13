using System.Collections.ObjectModel;
using Windows.Devices.Input.Preview;
using Windows.Foundation;
using Windows.UI.Xaml.Controls;
using Windows.Devices.Enumeration;
using Windows.Devices.HumanInterfaceDevice;
using System;
using Windows.UI.Xaml;
using Windows.ApplicationModel;
using System.Collections.Generic;
using Windows.UI.Core;
using Windows.Storage.Streams;

namespace GazeTracing
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            InitializeComponent();

            DataContext = this;
            InitializeGazeTracing();
        }

        private void InitializeGazeTracing()
        {
            MaxGazeHistorySize = 100;

            gazeInputSourcePreview = GazeInputSourcePreview.GetForCurrentView();
            gazeInputSourcePreview.GazeMoved += GazeInputSourcePreview_GazeMoved;
        }

        private GazeInputSourcePreview gazeInputSourcePreview;

        public ObservableCollection<Point> GazeHistory { get; set; } = new ObservableCollection<Point>();

        public int MaxGazeHistorySize { get; set; }

        private void GazeInputSourcePreview_GazeMoved(GazeInputSourcePreview sender, GazeMovedPreviewEventArgs args)
        {
            var point = args.CurrentPoint.EyeGazePosition;
            if (!point.HasValue)
            {
                return;
            }

            GazeHistory.Add(point.Value);
            if (GazeHistory.Count > MaxGazeHistorySize)
            {
                GazeHistory.RemoveAt(0);
            }
        }
    }
}
