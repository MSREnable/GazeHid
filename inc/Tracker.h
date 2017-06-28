#pragma once

#define HID_USAGE_PAGE_EYE_HEAD_TRACKER             (0x0012)
#define HID_USAGE_PAGE_NAME_EYE_HEAD_TRACKER        "Eye and Head Trackers"

#define HID_USAGE_EYE_TRACKER                       (0x0001)
#define HID_USAGE_HEAD_TRACKER                      (0x0002)
//RESERVED                                          0x0003-0x000F

// HID_REPORT_ID List
#define HID_USAGE_TRACKING_DATA                     (0x0010)
#define HID_USAGE_CAPABILITIES                      (0x0011)
#define HID_USAGE_CONFIGURATION                     (0x0012)
#define HID_USAGE_TRACKER_STATUS                    (0x0013)
#define HID_USAGE_TRACKER_CONTROL                   (0x0014)
//RESERVED                                          0x0015-0x001F

// HID_USAGE_TRACKING_DATA - Input Collection
#define HID_USAGE_TIMESTAMP                         (0x0020)
#define HID_USAGE_POSITION_X                        (0x0021) 
#define HID_USAGE_POSITION_Y                        (0x0022)
#define HID_USAGE_POSITION_Z                        (0x0023)
#define HID_USAGE_GAZE_LOCATION                     (0x0024)
#define HID_USAGE_LEFT_EYE_POSITION                 (0x0025)
#define HID_USAGE_RIGHT_EYE_POSITION                (0x0026)
#define HID_USAGE_HEAD_POSITION                     (0x0027)
//RESERVED                                          0x0028-0x00FF

// HID_USAGE_CAPABILITIES - Feature Collection 
#define HID_USAGE_TRACKER_QUALITY                   (0x0100)
#define HID_USAGE_GAZE_LOCATION_ORIGIN              (0x0101)
#define HID_USAGE_EYE_POSITION_ORIGIN               (0x0102)
#define HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY        (0x0103)
#define HID_USAGE_MINIMUM_TRACKING_DISTANCE         (0x0104)
#define HID_USAGE_OPTIMUM_TRACKING_DISTANCE         (0x0105)
#define HID_USAGE_MAXIMUM_TRACKING_DISTANCE         (0x0106)
#define HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH        (0x0107)
#define HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT       (0x0108)
#define HID_USAGE_SCREEN_PLANE_CURVATURE            (0x0109)
#define HID_USAGE_SCREEN_PLANE_TOP_LEFT             (0x010A)
#define HID_USAGE_SCREEN_PLANE_TOP_RIGHT            (0x010B)
#define HID_USAGE_SCREEN_PLANE_BOTTOM_LEFT          (0x010C)
#define HID_USAGE_TRACKER_COORDINATES               (0x010D)
//RESERVED                                          0x0010E-0x01FF

// HID_USAGE_CONFIGURATION - Feature Collection 
#define HID_USAGE_DISPLAY_MANUFACTURER_ID           (0x0200)
#define HID_USAGE_DISPLAY_PRODUCT_ID                (0x0201)
#define HID_USAGE_DISPLAY_SERIAL_NUMBER             (0x0202)
#define HID_USAGE_DISPLAY_MANUFACTURER_DATE         (0x0203)
//RESERVED                                          0x0204-0x02FF

// HID_USAGE_TRACKER_STATUS - Feature Collection 
#define HID_USAGE_DEVICE_STATUS                     (0x0300)
#define HID_USAGE_CONFIGURATION_STATUS              (0x0301)
#define HID_USAGE_SAMPLING_FREQUENCY                (0x0302)
//RESERVED                                          0x0303-0x03FF

// HID_USAGE_TRACKER_CONTROL - Feature Collection 
#define HID_USAGE_MODE_REQUEST                      (0x0400)

// TODO: API Validator has to be turned off for the driver. This needs to be fixed.


#include <pshpack1.h>

typedef struct _POINT2D
{
    float    X;
    float    Y;
} POINT2D, *PPOINT2D;

typedef struct _POINT3D
{
    uint32_t    X;
    uint32_t    Y;
    uint32_t    Z;
} POINT3D, *PPOINT3D;

typedef struct _GAZE_REPORT
{
    uint8_t        ReportId;
    uint8_t        Reserved[3];
    uint64_t    TimeStamp;
    POINT2D     GazePoint;
    POINT3D     LeftEyePosition;
    POINT3D     RightEyePosition;
} GAZE_REPORT, *PGAZE_REPORT;

typedef struct _CAPABILITIES_REPORT
{
    uint8_t            ReportId;
    uint8_t            TrackerQuality;
    uint8_t            GazeLocationOrigin;
    uint8_t            EyePositionOrigin;
    uint16_t        MaxFramesPerSecond; // Maximum Sampling Frequency
    uint16_t        MinimumTrackingDistance;
    uint16_t        OptimumTrackingDistance;
    uint16_t        MaximumTrackingDistance;
    uint16_t        MaximumScreenPlaneWidth;
    uint16_t        MaximumScreenPlaneHeight;
    uint32_t        ScreenPlaneCurvature;
    POINT3D         ScreenPlaneTopLeft;
    POINT3D         ScreenPlaneTopRight;
    POINT3D         ScreenPlaneBottomLeft;
} CAPABILITIES_REPORT, *PCAPABILITIES_REPORT;

typedef struct _CONFIGURATION_REPORT
{
    uint8_t         ReportId;
    uint8_t         Reserved;
    uint16_t        DisplayManufacturerId;
    uint16_t        DisplayProductId;
    uint32_t        DisplaySerialNumber;
    uint16_t        DisplayManufacturerDate;
} CONFIGURATION_REPORT, *PCONFIGURATION_REPORT;

typedef struct _TRACKER_STATUS_REPORT
{
    uint8_t         ReportId;
    uint8_t         Reserved;
    uint8_t         TrackerStatus;
    uint8_t         ConfigurationStatus;
    uint16_t        FramesPerSecond;
} TRACKER_STATUS_REPORT, *PTRACKER_STATUS_REPORT;

typedef struct _TRACKER_CONTROL_REPORT
{
    uint8_t         ReportId;
    uint8_t         ModeRequest;
} TRACKER_CONTROL_REPORT, *PTRACKER_CONTROL_REPORT;

#include <poppack.h>
