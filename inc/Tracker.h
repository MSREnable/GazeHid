#pragma once

#pragma region Eye Tracker HID Usages
// From HUTRR74 - https://www.usb.org/sites/default/files/hutrr74_-_usage_page_for_head_and_eye_trackers_0.pdf

#define HID_USAGE_PAGE_EYE_HEAD_TRACKER             (0x0012)
#define HID_USAGE_PAGE_NAME_EYE_HEAD_TRACKER        "Eye and Head Trackers"

#define HID_USAGE_UNDEFINED                         (0x0000)        // Type
#define HID_USAGE_EYE_TRACKER                       (0x0001)        // CA
#define HID_USAGE_HEAD_TRACKER                      (0x0002)        // CA
//RESERVED                                          0x0003-0x000F

// HID_REPORT_ID List
#define HID_USAGE_TRACKING_DATA                     (0x10)        // CP
#define HID_USAGE_CAPABILITIES                      (0x11)        // CL
#define HID_USAGE_CONFIGURATION                     (0x12)        // CL
#define HID_USAGE_TRACKER_STATUS                    (0x13)        // CL
#define HID_USAGE_TRACKER_CONTROL                   (0x14)        // CL
//RESERVED                                          0x0015-0x001F

// HID_USAGE_TRACKING_DATA - Input Collection
#define HID_USAGE_TIMESTAMP                         (0x0020)        // DV
#define HID_USAGE_POSITION_X                        (0x0021)        // DV 
#define HID_USAGE_POSITION_Y                        (0x0022)        // DV
#define HID_USAGE_POSITION_Z                        (0x0023)        // DV
#define HID_USAGE_GAZE_LOCATION                     (0x0024)        // CP
#define HID_USAGE_LEFT_EYE_POSITION                 (0x0025)        // CP
#define HID_USAGE_RIGHT_EYE_POSITION                (0x0026)        // CP
#define HID_USAGE_HEAD_POSITION                     (0x0027)        // CP
#define HID_USAGE_HEAD_DIRECTION                    (0x0028)        // CP
#define HID_USAGE_ROTATION_ABOUT_X_AXIS             (0x0029)        // DV
#define HID_USAGE_ROTATION_ABOUT_Y_AXIS             (0x002A)        // DV
#define HID_USAGE_ROTATION_ABOUT_Z_AXIS             (0x002B)        // DV
//RESERVED                                          0x002C-0x00FF

// HID_USAGE_CAPABILITIES - Feature Collection 
#define HID_USAGE_TRACKER_QUALITY                   (0x0100)        // SV
#define HID_USAGE_MINIMUM_TRACKING_DISTANCE         (0x0101)        // SV
#define HID_USAGE_OPTIMUM_TRACKING_DISTANCE         (0x0102)        // SV
#define HID_USAGE_MAXIMUM_TRACKING_DISTANCE         (0x0103)        // SV
#define HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH        (0x0104)        // SV
#define HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT       (0x0105)        // SV
//RESERVED                                          0x00106-0x01FF

// HID_USAGE_CONFIGURATION - Feature Collection 
#define HID_USAGE_DISPLAY_MANUFACTURER_ID           (0x0200)        // SV
#define HID_USAGE_DISPLAY_PRODUCT_ID                (0x0201)        // SV
#define HID_USAGE_DISPLAY_SERIAL_NUMBER             (0x0202)        // SV
#define HID_USAGE_DISPLAY_MANUFACTURER_DATE         (0x0203)        // SV
#define HID_USAGE_CALIBRATED_SCREEN_WIDTH           (0x0204)        // SV
#define HID_USAGE_CALIBRATED_SCREEN_HEIGHT          (0x0205)        // SV
//RESERVED                                          0x0204-0x02FF

// HID_USAGE_TRACKER_STATUS - Feature Collection 
#define HID_USAGE_SAMPLING_FREQUENCY                (0x0300)        // DV
#define HID_USAGE_CONFIGURATION_STATUS              (0x0301)        // DV
//RESERVED                                          0x0302-0x03FF

// HID_USAGE_TRACKER_CONTROL - Feature Collection 
#define HID_USAGE_MODE_REQUEST                      (0x0400)        // DV
#pragma endregion Eye Tracker HID Usages

#pragma region Eye Tracker HID Usage Constant Definitions
// TODO: API Validator has to be turned off for the driver. This needs to be fixed.

// Tracker Quality
#define TRACKER_QUALITY_RESERVED                    0
#define TRACKER_QUALITY_FINE_GAZE                   1

// Tracker Status
#define TRACKER_STATUS_RESERVED                     0
#define TRACKER_STATUS_READY                        1
#define TRACKER_STATUS_CONFIGURING                  2
#define TRACKER_STATUS_SCREEN_SETUP_NEEDED          3
#define TRACKER_STATUS_USER_CALIBRATION_NEEDED      4

// Device Mode Request
#define MODE_REQUEST_ENABLE_GAZE_POINT              1
#define MODE_REQUEST_ENABLE_EYE_POSITION            2
#define MODE_REQUEST_ENABLE_HEAD_POSITION           4
#pragma endregion Eye Tracker HID Usage Constant Definitions

#include <pshpack1.h>

#pragma region HID Report Struct Definitions
typedef struct _POINT2D
{
    int32_t    X;
    int32_t    Y;
} POINT2D, *PPOINT2D;

typedef struct _POINT3D
{
    int32_t    X;
    int32_t    Y;
    int32_t    Z;
} POINT3D, *PPOINT3D;

typedef struct _GAZE_REPORT
{
    uint8_t     ReportId;
    //uint8_t     Reserved[7];
    uint64_t    TimeStamp;
    POINT2D     GazePoint;
    POINT3D     LeftEyePosition;
    POINT3D     RightEyePosition;
    //POINT3D     HeadPosition;
    //POINT3D     HeadDirection;
} GAZE_REPORT, *PGAZE_REPORT;

typedef struct _CAPABILITIES_REPORT
{
    uint8_t         ReportId;
    uint8_t         TrackerQuality;
    uint32_t        MinimumTrackingDistance;
    uint32_t        OptimumTrackingDistance;
    uint32_t        MaximumTrackingDistance;
    uint32_t        MaximumScreenPlaneWidth;
    uint32_t        MaximumScreenPlaneHeight;
} CAPABILITIES_REPORT, *PCAPABILITIES_REPORT;

typedef struct _CONFIGURATION_REPORT
{
    uint8_t         ReportId;
    //uint8_t         Reserved;
    uint16_t        DisplayManufacturerId;
    uint16_t        DisplayProductId;
    uint32_t        DisplaySerialNumber;
    uint16_t        DisplayManufacturerDate;
    int32_t         CalibratedScreenWidth;
    int32_t         CalibratedScreenHeight;
} CONFIGURATION_REPORT, *PCONFIGURATION_REPORT;

typedef struct _TRACKER_STATUS_REPORT
{
    uint8_t         ReportId;
    uint8_t         ConfigurationStatus;
    uint16_t        SamplingFrequency;
} TRACKER_STATUS_REPORT, *PTRACKER_STATUS_REPORT;

typedef struct _TRACKER_CONTROL_REPORT
{
    uint8_t         ReportId;
    uint8_t         ModeRequest;
} TRACKER_CONTROL_REPORT, *PTRACKER_CONTROL_REPORT;
#pragma endregion HID Report Struct Definitions

#include <poppack.h>
