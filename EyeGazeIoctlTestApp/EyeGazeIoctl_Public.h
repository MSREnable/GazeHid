/*++

    Copyright (C) Microsoft. All rights reserved.

Module Name:

    Dmf_EyeGazeIoctl_Public.h

Abstract:

    This module contains the common declarations shared by driver and user applications.

Environment:

    User and Kernel Modes

--*/

#pragma once

#pragma pack(push, 1)

#include <wtypes.h>
#include <initguid.h>

typedef struct _POINT2D
{
    LONG X;
    LONG Y;
} POINT2D, * PPOINT2D;

typedef struct _POINT3D
{
    LONG X;
    LONG Y;
    LONG Z;
} POINT3D, * PPOINT3D;

typedef struct _GAZE_DATA
{
    ULONGLONG TimeStamp;
    POINT2D GazePoint;
    POINT3D LeftEyePosition;
    POINT3D RightEyePosition;
    POINT3D HeadPosition;
    POINT3D HeadDirection;
} GAZE_DATA, * PGAZE_DATA;

typedef struct _CONFIGURATION_DATA
{
    USHORT DisplayManufacturerId;
    USHORT DisplayProductId;
    ULONG DisplaySerialNumber;
    USHORT DisplayManufacturerDate;
    ULONG CalibratedScreenWidth;
    ULONG CalibratedScreenHeight;
} CONFIGURATION_DATA, * PCONFIGURATION_DATA;

typedef struct _CAPABILITIES_DATA
{
    UCHAR ReportId;
    UCHAR TrackerQuality;
    ULONG MinimumTrackingDistance;
    ULONG OptimumTrackingDistance;
    ULONG MaximumTrackingDistance;
    ULONG MaximumScreenPlaneWidth;
    ULONG MaximumScreenPlaneHeight;
} CAPABILITIES_DATA, * PCAPABILITIES_DATA;

#pragma pack(pop)

// {2E8BE292-682B-493E-B7D8-73033613E530}
DEFINE_GUID(VirtualEyeGaze_GUID,
    0x2e8be292, 0x682b, 0x493e, 0xb7, 0xd8, 0x73, 0x3, 0x36, 0x13, 0xe5, 0x30);

#define IOCTL_EYEGAZE_GAZE_DATA             CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                                     0x100, \
                                                     METHOD_BUFFERED, \
                                                     FILE_WRITE_ACCESS)

#define IOCTL_EYEGAZE_CONFIGURATION_REPORT  CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                                     0x101, \
                                                     METHOD_BUFFERED, \
                                                     FILE_WRITE_ACCESS)

#define IOCTL_EYEGAZE_CAPABILITIES_REPORT   CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                                     0x102, \
                                                     METHOD_BUFFERED, \
                                                     FILE_WRITE_ACCESS)

#define IOCTL_EYEGAZE_CONTROL_REPORT        CTL_CODE(FILE_DEVICE_UNKNOWN,\
                                                     0x103, \
                                                     METHOD_BUFFERED, \
                                                     FILE_WRITE_ACCESS)

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
#define MODE_REQUEST_RESERVED                       0
#define MODE_REQUEST_ENABLE_GAZE_POINT              1
#define MODE_REQUEST_ENABLE_EYE_POSITION            2
#define MODE_REQUEST_ENABLE_HEAD_POSITION           4

// eof: Dmf_EyeGazeIoctl_Public.h
//
