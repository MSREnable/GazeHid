#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <conio.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include "common.h"
#include "Tracker.h"

GUID                        m_hidguid;
HANDLE                      m_file = INVALID_HANDLE_VALUE;
PHIDP_PREPARSED_DATA        m_pPpd = NULL;
HIDD_ATTRIBUTES             m_Attr; // Device attributes

HIDP_CAPS                   m_Caps; // The Capabilities of this hid device.
PHIDP_VALUE_CAPS            m_pInputValueCaps = NULL;
PHIDP_VALUE_CAPS            m_pOutputValueCaps = NULL;
PHIDP_VALUE_CAPS            m_pFeatureValueCaps = NULL;
PHIDP_BUTTON_CAPS           m_pInputButtonCaps = NULL;
PHIDP_BUTTON_CAPS           m_pOutputButtonCaps = NULL;

WCHAR                       m_pvhidminiDeviceString[MAXIMUM_STRING_LENGTH] = { 0 };
WCHAR                       m_pProductString[MAXIMUM_STRING_LENGTH] = { 0 };
WCHAR                       m_pSerialNumberString[MAXIMUM_STRING_LENGTH] = { 0 };
WCHAR                       m_pManufacturerString[MAXIMUM_STRING_LENGTH] = { 0 };
WCHAR                       m_pPhysicalDescriptorString[MAXIMUM_STRING_LENGTH] = { 0 };

PHIDP_LINK_COLLECTION_NODE  m_pLinkCollectionNodes = NULL;

LONG                       m_lScreenWidth = 0;
LONG                       m_lScreenHeight = 0;

GAZE_REPORT                 m_gazeReport;
CAPABILITIES_REPORT         m_capabilitiesReport;
CONFIGURATION_REPORT        m_configurationReport;
TRACKER_STATUS_REPORT       m_trackerStatusReport;

//
// Function prototypes
//
BOOLEAN
GetFeatureCapabilities(
);

VOID
PrintFeatureCapabilities(
);

BOOLEAN
GetFeatureConfiguration(
);

VOID
PrintFeatureConfiguration(
);

BOOLEAN
GetFeatureTrackerStatus(
);

VOID
PrintFeatureTrackerStatus(
);

BOOLEAN
SetFeatureTrackerControl(
    uint8_t modeRequest
);

BOOLEAN
CheckIfOurDevice(
    PHANDLE pHandle
);

BOOLEAN
ReadInputData(
);

BOOLEAN
GetIndexedString(
);

BOOLEAN
GetWellKnownStrings(
);

VOID
PrintWellKnownStrings(
);

VOID
PrintValueCaps(
);

VOID
PrintValueCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT valueCapsLength,
    PHIDP_VALUE_CAPS* ppValueCaps
);

BOOLEAN
GetValueCaps(
    HIDP_REPORT_TYPE reportType,
    USHORT length,
    PHIDP_VALUE_CAPS* ppValueCaps
);

VOID
PrintButtonCaps(
);

VOID
PrintButtonCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT length,
    PHIDP_BUTTON_CAPS *ppButtonCaps
);

BOOLEAN
GetButtonCaps(
    HIDP_REPORT_TYPE reportType,
    USHORT length,
    PHIDP_BUTTON_CAPS *ppButtonCaps
);

BOOLEAN
GetLinkCollectionNodes(
);

BOOLEAN
TraverseLinkCollectionNodes(
    ULONG currentNodeIdx
);

VOID
PrintLinkCollectionNodes(
);

VOID
PrintLinkCollectionNode(
    int currentNodeIdx
);

VOID
PrintLinkCollectionNodeChildren(
    int currentNodeIdx
);

USHORT
GetLinkCollectionIndex
(
    HIDP_REPORT_TYPE reportType,
    USAGE reportID,
    USAGE usage
);

BOOLEAN
FindMatchingDevice(
    _In_ LPGUID Guid
);

int8_t
CodeToExponent(
    BYTE b
);

CHAR*
UnitsToString(
    ULONG units
);

CHAR*
IsLogicalMinMaxSigned(
    LONG logicalMin,
    LONG logicalMax
);

CHAR*
GetUsageString(
    USAGE usage
);

CHAR*
GetCollectionTypeString(
    int collectionType
);

CHAR*
GetReportTypeString(
    HIDP_REPORT_TYPE reportType
);

CHAR*
GetUsagePageString(
    USAGE usagePage
);

CHAR*
GetStatusResultString(
    NTSTATUS status
);

CHAR*
GetTrackerQualityString(
    uint8_t trackerQuality
);

CHAR*
GetConfigurationStatusString(
    uint8_t configurationStatus
);

VOID
GetDesktopResolution(
    PLONG plHorizontal,
    PLONG plVertical
);

VOID
Dispose(
);

VOID
PrintBuffer(
    PCHAR pbBuffer,
    USHORT cbBuffer
);