/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    testvhid.c

Environment:

    user mode only

Author:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <conio.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include <wingdi.h>
#include "common.h"
#include <time.h>
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

PHIDP_LINK_COLLECTION_NODE  m_pLinkCollectionNodes = NULL;

//
// Function prototypes
//
BOOLEAN
GetFeatureCapabilities(
);

BOOLEAN
GetFeatureConfiguration(
);

BOOLEAN
GetFeatureTrackerStatus(
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

BOOLEAN
GetValueCaps(
);

BOOLEAN
GetValueCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT length,
    PHIDP_VALUE_CAPS* ppValueCaps
);

BOOLEAN
GetButtonCaps(
);

BOOLEAN
GetButtonCapsEX(
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
DumpLinkCollectionNode(
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

VOID
PrintUsageString(
    USAGE usage
);

VOID
PrintCollectionType
(
    int collectionType
);

VOID
PrintReportType
(
    HIDP_REPORT_TYPE reportType
);

VOID
PrintUsagePage
(
    USAGE usagePage
);

VOID
PrintStatusResult
(
    NTSTATUS status
);

NTSTATUS
GetUsageValue
(
    _In_ HIDP_REPORT_TYPE reportType,
    _In_ USHORT usCollectionIdx,
    _In_ USAGE usage,
    _In_ PCHAR pbBuffer,
    _In_ DWORD cbBuffer,
    _Out_ PULONG ulUsageValue
);

//
// Implementation
//
INT __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
)
{
    BOOLEAN found = FALSE;
    BOOLEAN bSuccess = FALSE;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    srand((unsigned)time(NULL));

    HidD_GetHidGuid(&m_hidguid);

    found = FindMatchingDevice(&m_hidguid);

    if (found) {
        printf("\n...sending control request to our device\n\n");

        if (m_pPpd == NULL)
        {
            printf("Error: PreParsedData not obtained!\n");
            goto cleanup;
        }

        bSuccess = GetLinkCollectionNodes();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        // Get Button and Value Caps
        bSuccess = GetValueCaps();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetButtonCaps();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        //
        // Get Strings
        //
        bSuccess = GetIndexedString();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetWellKnownStrings();
        //if (bSuccess == FALSE) {
        //    goto cleanup;
        //}

        //
        // Get/Set feature loopback
        //

        bSuccess = GetFeatureCapabilities();
        //if (bSuccess == FALSE) {
        //    goto cleanup;
        //}

        bSuccess = GetFeatureConfiguration();
        //if (bSuccess == FALSE) {
        //    goto cleanup;
        //}

        bSuccess = GetFeatureTrackerStatus();
        //if (bSuccess == FALSE) {
        //    goto cleanup;
        //}

        bSuccess = SetFeatureTrackerControl(TRUE);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        printf("\n\n\nPress any key to continue...\n");
        while (!_kbhit())
        {
        }
        _getch();

        //
        // Read/Write report loopback
        //
        bSuccess = ReadInputData();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

    }
    else {
        printf("Failure: Could not find our HID device \n");
    }

cleanup:
    if (found && bSuccess == FALSE) {
        printf("****** Failure: one or more commands to device failed *******\n");
    }

    printf("Press any key to exit...\n");
    while (!_kbhit())
    {
    }
    _getch();

    if (m_file != INVALID_HANDLE_VALUE) {
        CloseHandle(m_file);
    }

    if (m_pPpd)
    {
        HidD_FreePreparsedData(m_pPpd);
    }

    if (m_pInputValueCaps) {
        free(m_pInputValueCaps);
    }

    if (m_pOutputValueCaps) {
        free(m_pOutputValueCaps);
    }

    if (m_pFeatureValueCaps) {
        free(m_pFeatureValueCaps);
    }

    if (m_pInputButtonCaps) {
        free(m_pInputButtonCaps);
    }

    if (m_pOutputButtonCaps) {
        free(m_pOutputButtonCaps);
    }

    return (bSuccess ? 0 : 1);
}

BOOLEAN
FindMatchingDevice(
    _In_  LPGUID  InterfaceGuid
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR currentInterface;
    BOOLEAN bRet = FALSE;
    HANDLE devHandle = INVALID_HANDLE_VALUE;

    m_file = INVALID_HANDLE_VALUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto cleanup;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto cleanup;
    }

    deviceInterfaceList = (PWSTR)calloc(deviceInterfaceListLength, sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto cleanup;
    }
    //ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto cleanup;
    }

    printf("Looking for our HID device (with UP=0x%04X "
        "and Usage=0x%02X)\n\n", HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_EYE_TRACKER);

    //
    // Enumerate devices of this interface class
    //
    for (currentInterface = deviceInterfaceList;
        *currentInterface && m_file == INVALID_HANDLE_VALUE;
        currentInterface += wcslen(currentInterface) + 1) {

        wprintf(L"%s\n", currentInterface);
        devHandle = CreateFile(currentInterface,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING, // No special create flags
            0, // No special attributes
            NULL); // No template file

        if (INVALID_HANDLE_VALUE == devHandle) {
            printf("Warning: CreateFile failed: %d\n", GetLastError());
            continue;
        }

        if (CheckIfOurDevice(devHandle)) {
            printf("Device Found %ls\n", currentInterface);
            bRet = TRUE;
            m_file = devHandle;
        }
        else {
            CloseHandle(devHandle);
        }
    }

cleanup:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }
    return bRet;
}

BOOLEAN
CheckIfOurDevice(
    PHANDLE pHandle
)
{
    BOOLEAN bSuccess = TRUE;

    if (bSuccess && !HidD_GetPreparsedData(pHandle, &m_pPpd))
    {
        printf("ERROR: HidD_GetPreparsedData failed \n");
        bSuccess = FALSE;
    }

    if (bSuccess && !HidP_GetCaps(m_pPpd, &m_Caps))
    {
        printf("ERROR: HidP_GetCaps failed \n");
        bSuccess = FALSE;
    }

    if (bSuccess &&
        (m_Caps.UsagePage == HID_USAGE_PAGE_EYE_HEAD_TRACKER) &&
        (m_Caps.Usage == HID_USAGE_EYE_TRACKER))
    {
        if (!HidD_GetAttributes(pHandle, &m_Attr))
        {
            printf("ERROR: HidD_GetAttributes failed \n");
            bSuccess = FALSE;
        }
        else
        {
            printf("\nSuccess: Found my device...\n");
            printf("Device Attributes - PID: 0x%x, VID: 0x%x \n", m_Attr.ProductID, m_Attr.VendorID);
            bSuccess = TRUE;
        }
    }
    else
    {
        bSuccess = FALSE;
    }

    if (!bSuccess)
    {
        ZeroMemory(&m_Attr, sizeof(HIDD_ATTRIBUTES));
        ZeroMemory(&m_Caps, sizeof(HIDP_CAPS));
        HidD_FreePreparsedData(m_pPpd);
    }

    return bSuccess;
}

BOOLEAN
GetFeatureCapabilities(
)
{
    BOOLEAN bSuccess = TRUE;

    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_CAPABILITIES;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    printf("\nReportID: "); PrintUsageString(reportID); printf("\n");
    printf("LinkCollection: %d\n", usCollectionIdx);
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_TRACKER_QUALITY, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_GAZE_LOCATION_ORIGIN, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_EYE_POSITION_ORIGIN, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_MINIMUM_TRACKING_DISTANCE, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_OPTIMUM_TRACKING_DISTANCE, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_MAXIMUM_TRACKING_DISTANCE, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");

    free(pbBuffer);

    return bSuccess;
}

BOOLEAN
GetFeatureConfiguration(
)
{
    BOOLEAN bSuccess = TRUE;

    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_CONFIGURATION;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    printf("\nReportID: "); PrintUsageString(reportID); printf("\n");
    printf("LinkCollection: %d\n", usCollectionIdx);
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_DISPLAY_MANUFACTURER_ID, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_DISPLAY_PRODUCT_ID, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_DISPLAY_SERIAL_NUMBER, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_DISPLAY_MANUFACTURER_DATE, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_CALIBRATED_SCREEN_WIDTH, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_CALIBRATED_SCREEN_HEIGHT, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");

    free(pbBuffer);

    return bSuccess;
}

BOOLEAN
GetFeatureTrackerStatus(
)
{
    BOOLEAN bSuccess = TRUE;

    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_TRACKER_STATUS;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    printf("\nReportID: "); PrintUsageString(reportID); printf("\n");
    printf("LinkCollection: %d\n", usCollectionIdx);
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_DEVICE_STATUS, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_CONFIGURATION_STATUS, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
    GetUsageValue(HidP_Feature, usCollectionIdx, HID_USAGE_SAMPLING_FREQUENCY, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");

    free(pbBuffer);

    return bSuccess;
}

BOOLEAN
SetFeatureTrackerControl(
    uint8_t modeRequest
)
{
    BOOLEAN bSuccess = TRUE;
    BYTE buffer[2] = { 0 };

    if (modeRequest > 1)
    {
        printf("ERROR: Mode Request value invalid. Should be 0 or 1\n");
        bSuccess = FALSE;
    }
    else
    {
        buffer[0] = HID_USAGE_TRACKER_CONTROL;
        buffer[1] = modeRequest;

        bSuccess = HidD_SetFeature(m_file, &buffer, sizeof(buffer));

        if (bSuccess)
        {
            printf("HID_USAGE_TRACKER_CONTROL set to %d\n", modeRequest);
        }
        else
        {
            printf("ERROR: HidD_GetFeature failed. GLE=0x%08x\n", GetLastError());
        }
    }

    return bSuccess;
}

BOOLEAN
ReadInputData(
)
{
    BOOL bSuccess = TRUE;
    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.InputReportByteLength;
    DWORD bytesRead;
    USAGE reportID = 0;
    USHORT usCollectionIdx;

    //ULONG ulTimestamp;
    //ULONG ulPositionX;
    //ULONG ulPositionY;
    //ULONG ulPositionZ;
    ULONG ulUsageValue;

    //NTSTATUS status;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    printf("\n\nHID_USAGE_TRACKING_DATA\n");

    //
    // get input data.
    //
    while ((bSuccess) && (!_kbhit()))
    {
        reportID = 0;
        ZeroMemory(pbBuffer, cbBuffer);

        bSuccess = ReadFile(
            m_file,         // HANDLE hFile,
            pbBuffer,       // LPVOID lpBuffer,
            cbBuffer,       // DWORD nNumberOfBytesToRead,
            &bytesRead,     // LPDWORD lpNumberOfBytesRead,
            NULL            // LPOVERLAPPED lpOverlapped
        );

        if (!bSuccess)
        {
            printf("ERROR: ReadFile failed. GLE=0x%08x\n", GetLastError());
        }
        else
        {
            reportID = pbBuffer[0];
            printf("ReportID:   0x%04X ", reportID); PrintUsageString(reportID); printf("\n");

            if (reportID == HID_USAGE_TRACKING_DATA)
            {
                usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, 0);
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_TIMESTAMP, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                
                usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_GAZE_LOCATION);
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_X, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_Y, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");

                usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_LEFT_EYE_POSITION);
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_X, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_Y, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_Z, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");

                usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_RIGHT_EYE_POSITION);
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_X, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_Y, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
                GetUsageValue(HidP_Input, usCollectionIdx, HID_USAGE_POSITION_Z, pbBuffer, cbBuffer, &ulUsageValue); printf("\n");
            }
            else if (reportID == 0x0015) // Config status change
            {
                PCHAR pbBuffer2 = NULL;
                USHORT cbBuffer2 = m_Caps.FeatureReportByteLength;
                pbBuffer2 = calloc(cbBuffer2, sizeof(CHAR));

                pbBuffer2[0] = HID_USAGE_CONFIGURATION;

                HidD_GetFeature(m_file, pbBuffer2, cbBuffer2);

                printf("GetFeature complete\n");
            }
            else
            {
                printf("Unknown Input Report\n");
            }
            printf("\n");
        }
    }

    if (_kbhit())
    {
        _getch();
    }

    free(pbBuffer);

    return (BOOLEAN)bSuccess;
}


BOOLEAN
GetIndexedString(
)
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = calloc(bufferLength, sizeof(BYTE));
    if (!buffer)
    {
        printf("calloc failed\n");
        return FALSE;
    }

    //ZeroMemory(buffer, bufferLength);

    bSuccess = HidD_GetIndexedString(
        m_file,
        VHIDMINI_DEVICE_STRING_INDEX,  // IN ULONG  StringIndex,
        (PVOID)buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetIndexedString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Indexed string: %S\n", (PWSTR)buffer);
    }

    free(buffer);

    return bSuccess;
}

BOOLEAN
GetWellKnownStrings(
)
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = calloc(bufferLength, sizeof(BYTE));
    if (!buffer)
    {
        printf("calloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetProductString(
        m_file,
        (PVOID)buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetProductString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Product string: %S\n", (PWSTR)buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetSerialNumberString(
        m_file,
        (PVOID)buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetSerialNumberString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Serial number string: %S\n", (PWSTR)buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetManufacturerString(
        m_file,
        (PVOID)buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetManufacturerString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Manufacturer string: %S\n", (PWSTR)buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetPhysicalDescriptor(
        m_file,
        (PVOID)buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetPhysicalDescriptor failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Physical Descriptor string: %S\n", (PWSTR)buffer);
    }

    //exit:

    free(buffer);

    return bSuccess;
}

BOOLEAN
GetValueCaps(
)
{
    BOOLEAN bSuccess = TRUE;

    printf("ValueCaps\n");
    GetValueCapsEX(HidP_Input, m_Caps.NumberInputValueCaps, &m_pInputValueCaps);
    GetValueCapsEX(HidP_Output, m_Caps.NumberOutputValueCaps, &m_pOutputValueCaps);
    GetValueCapsEX(HidP_Feature, m_Caps.NumberFeatureValueCaps, &m_pFeatureValueCaps);
    printf("\n");

    return bSuccess;
}

BOOLEAN
GetValueCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT valueCapsLength,
    PHIDP_VALUE_CAPS* ppValueCaps
)
{
    BOOLEAN bSuccess = TRUE;

    PHIDP_VALUE_CAPS pValueCaps = NULL;

    if (valueCapsLength > 0)
    {
        *ppValueCaps = calloc(valueCapsLength, sizeof(HIDP_VALUE_CAPS));

        HidP_GetValueCaps(
            reportType,
            *ppValueCaps,
            &valueCapsLength,
            m_pPpd
        );

        pValueCaps = *ppValueCaps;

        for (int idx = 0; idx < valueCapsLength; ++idx)
        {
            printf("ReportType:           "); PrintReportType(reportType); printf("\n");
            printf("UsagePage:            0x%04X ", pValueCaps[idx].UsagePage); PrintUsagePage(pValueCaps[idx].UsagePage); printf("\n");
            printf("ReportID:             0x%04X ", pValueCaps[idx].ReportID); PrintUsageString(pValueCaps[idx].ReportID); printf("\n");
            printf("LinkCollection:     %8d\n", pValueCaps[idx].LinkCollection);
            printf("LinkUsage:            0x%04X ", pValueCaps[idx].LinkUsage); PrintUsageString(pValueCaps[idx].LinkUsage); printf("\n");
            printf("LinkUsagePage:        0x%04X ", pValueCaps[idx].LinkUsagePage); PrintUsagePage(pValueCaps[idx].LinkUsagePage); printf("\n");
            printf("BitSize:              0x%04X\n", pValueCaps[idx].BitSize);
            printf("\n");

        }
    }

    return bSuccess;
}

BOOLEAN
GetButtonCaps(
)
{
    BOOLEAN bSuccess = TRUE;

    printf("ButtonCaps\n");
    GetButtonCapsEX(HidP_Input, m_Caps.NumberInputButtonCaps, &m_pInputButtonCaps);
    GetButtonCapsEX(HidP_Output, m_Caps.NumberOutputButtonCaps, &m_pOutputButtonCaps);
    printf("\n");

    return bSuccess;
}

BOOLEAN
GetButtonCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT buttonCapsLength,
    PHIDP_BUTTON_CAPS *ppButtonCaps
)
{
    BOOLEAN bSuccess = TRUE;
    PHIDP_BUTTON_CAPS pButtonCaps = NULL;

    if (buttonCapsLength > 0)
    {
        *ppButtonCaps = calloc(buttonCapsLength, sizeof(HIDP_BUTTON_CAPS));
        //ZeroMemory(*ppButtonCaps, length * sizeof(HIDP_BUTTON_CAPS));

        HidP_GetButtonCaps(
            reportType,
            *ppButtonCaps,
            &buttonCapsLength,
            m_pPpd
        );

        pButtonCaps = *ppButtonCaps;

        for (int idx = 0; idx < buttonCapsLength; ++idx)
        {
            printf("ReportType:           "); PrintReportType(reportType); printf("\n");
            printf("UsagePage:            0x%04X ", pButtonCaps[idx].UsagePage); PrintUsagePage(pButtonCaps[idx].UsagePage); printf("\n");
            printf("ReportID:             0x%04X\n", pButtonCaps[idx].ReportID);
            printf("LinkCollection:     %8d\n", pButtonCaps[idx].LinkCollection);
            printf("LinkUsage:            0x%04X ", pButtonCaps[idx].LinkUsage); PrintUsageString(pButtonCaps[idx].LinkUsage); printf("\n");
            printf("LinkUsagePage:        0x%04X\n", pButtonCaps[idx].LinkUsagePage); PrintUsagePage(pButtonCaps[idx].LinkUsagePage); printf("\n");
            printf("\n");
        }
    }

    return bSuccess;
}

BOOLEAN
GetLinkCollectionNodes(
)
{
    printf("\nLinkCollectionNodes\n");
    ULONG ulLinkCollectionNodesLength = m_Caps.NumberLinkCollectionNodes;

    if (ulLinkCollectionNodesLength > 0)
    {
        m_pLinkCollectionNodes = calloc(ulLinkCollectionNodesLength, sizeof(HIDP_LINK_COLLECTION_NODE));
        //ZeroMemory(m_pLinkCollectionNodes, linkCollectionNodesLength * sizeof(HIDP_LINK_COLLECTION_NODE));

        HidP_GetLinkCollectionNodes(
            m_pLinkCollectionNodes,
            &ulLinkCollectionNodesLength,
            m_pPpd
        );

        for (ULONG ulCurrentNodeIdx = 0; ulCurrentNodeIdx < ulLinkCollectionNodesLength; ++ulCurrentNodeIdx)
        {
            DumpLinkCollectionNode(ulCurrentNodeIdx);
        }

        //TraverseLinkCollectionNodes(0);
    }

    return TRUE;
}

BOOLEAN
TraverseLinkCollectionNodes(
    ULONG currentNodeIdx
)
{
    DumpLinkCollectionNode(currentNodeIdx);

    if (m_pLinkCollectionNodes[currentNodeIdx].NumberOfChildren > 0)
    {
        USHORT childNodeIdx = m_pLinkCollectionNodes[currentNodeIdx].FirstChild;
        while (childNodeIdx != 0)
        {
            TraverseLinkCollectionNodes(childNodeIdx);

            childNodeIdx = m_pLinkCollectionNodes[currentNodeIdx].NextSibling;
        }
    }

    return TRUE;
}

VOID
DumpLinkCollectionNode(
    int currentNodeIdx
)
{
    printf("Index:              %8d\n", currentNodeIdx);
    printf("Usage:                0x%04X ", m_pLinkCollectionNodes[currentNodeIdx].LinkUsage); PrintUsageString(m_pLinkCollectionNodes[currentNodeIdx].LinkUsage); printf("\n");
    printf("UsagePage:            0x%04X ", m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage); PrintUsagePage(m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage); printf("\n");
    if (m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage != HID_USAGE_PAGE_EYE_HEAD_TRACKER)
    {
        printf("Error: Invalid Usage Page, should be 0x%04X\n", HID_USAGE_PAGE_EYE_HEAD_TRACKER);
    }
    printf("CollectionType:     %8d ", m_pLinkCollectionNodes[currentNodeIdx].CollectionType); PrintCollectionType(m_pLinkCollectionNodes[currentNodeIdx].CollectionType); printf("\n");
    printf("IsAlias:            %8d\n", m_pLinkCollectionNodes[currentNodeIdx].IsAlias);
    printf("Parent:             %8d\n", m_pLinkCollectionNodes[currentNodeIdx].Parent);
    printf("NumberOfChildren:   %8d ", m_pLinkCollectionNodes[currentNodeIdx].NumberOfChildren); PrintLinkCollectionNodeChildren(currentNodeIdx); printf("\n");
    printf("FirstChild:         %8d\n", m_pLinkCollectionNodes[currentNodeIdx].FirstChild);
    printf("NextSibling:        %8d\n", m_pLinkCollectionNodes[currentNodeIdx].NextSibling);
    printf("\n");
}

VOID
PrintLinkCollectionNodeChildren(
    int currentNodeIdx
)
{
    
    printf("(");

    for (int currentChildNodeIdx = m_pLinkCollectionNodes[currentNodeIdx].FirstChild; 
        currentChildNodeIdx != 0;
        currentChildNodeIdx = m_pLinkCollectionNodes[currentChildNodeIdx].NextSibling)
    {
        printf(" %d ", currentChildNodeIdx);
    }

    printf(")");
}

VOID
PrintCollectionType
(
    int collectionType
)
{
    // From HID Spec 6.2.2.6
    switch (collectionType)
    {
    case 0: printf("Physical"); break;
    case 1: printf("Application"); break;
    case 2: printf("Logical"); break;
    case 3: printf("Report"); break;
    case 4: printf("Named Array"); break;
    case 5: printf("Usage Switch"); break;
    case 6: printf("Usage Modifier"); break;
    default: printf("Unknown"); break;
    }
}

VOID
PrintUsageString(
    USAGE usage
)
{
    switch (usage)
    {
    case HID_USAGE_EYE_TRACKER: printf("HID_USAGE_EYE_TRACKER"); break;
    case HID_USAGE_HEAD_TRACKER: printf("HID_USAGE_HEAD_TRACKER"); break;

        // HID_REPORT_ID List
    case HID_USAGE_TRACKING_DATA: printf("HID_USAGE_TRACKING_DATA"); break;
    case HID_USAGE_CAPABILITIES: printf("HID_USAGE_CAPABILITIES"); break;
    case HID_USAGE_CONFIGURATION: printf("HID_USAGE_CONFIGURATION"); break;
    case HID_USAGE_TRACKER_STATUS: printf("HID_USAGE_TRACKER_STATUS"); break;
    case HID_USAGE_TRACKER_CONTROL: printf("HID_USAGE_TRACKER_CONTROL"); break;

        // HID_USAGE_TRACKING_DATA - Input Collection
    case HID_USAGE_TIMESTAMP: printf("HID_USAGE_TIMESTAMP"); break;
    case HID_USAGE_POSITION_X: printf("HID_USAGE_POSITION_X"); break;
    case HID_USAGE_POSITION_Y: printf("HID_USAGE_POSITION_Y"); break;
    case HID_USAGE_POSITION_Z: printf("HID_USAGE_POSITION_Z"); break;
    case HID_USAGE_GAZE_LOCATION: printf("HID_USAGE_GAZE_LOCATION"); break;
    case HID_USAGE_LEFT_EYE_POSITION: printf("HID_USAGE_LEFT_EYE_POSITION"); break;
    case HID_USAGE_RIGHT_EYE_POSITION: printf("HID_USAGE_RIGHT_EYE_POSITION"); break;
    case HID_USAGE_HEAD_POSITION: printf("HID_USAGE_HEAD_POSITION"); break;

        // HID_USAGE_CAPABILITIES - Feature Collection 
    case HID_USAGE_TRACKER_QUALITY: printf("HID_USAGE_TRACKER_QUALITY"); break;
    case HID_USAGE_GAZE_LOCATION_ORIGIN: printf("HID_USAGE_GAZE_LOCATION_ORIGIN"); break;
    case HID_USAGE_EYE_POSITION_ORIGIN: printf("HID_USAGE_EYE_POSITION_ORIGIN"); break;
    case HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY: printf("HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY"); break;
    case HID_USAGE_MINIMUM_TRACKING_DISTANCE: printf("HID_USAGE_MINIMUM_TRACKING_DISTANCE"); break;
    case HID_USAGE_OPTIMUM_TRACKING_DISTANCE: printf("HID_USAGE_OPTIMUM_TRACKING_DISTANCE"); break;
    case HID_USAGE_MAXIMUM_TRACKING_DISTANCE: printf("HID_USAGE_MAXIMUM_TRACKING_DISTANCE"); break;
    case HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH: printf("HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH"); break;
    case HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT: printf("HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT"); break;

        // HID_USAGE_CONFIGURATION - Feature Collection 
    case HID_USAGE_DISPLAY_MANUFACTURER_ID: printf("HID_USAGE_DISPLAY_MANUFACTURER_ID"); break;
    case HID_USAGE_DISPLAY_PRODUCT_ID: printf("HID_USAGE_DISPLAY_PRODUCT_ID"); break;
    case HID_USAGE_DISPLAY_SERIAL_NUMBER: printf("HID_USAGE_DISPLAY_SERIAL_NUMBER"); break;
    case HID_USAGE_DISPLAY_MANUFACTURER_DATE: printf("HID_USAGE_DISPLAY_MANUFACTURER_DATE"); break;
    case HID_USAGE_CALIBRATED_SCREEN_WIDTH: printf("HID_USAGE_CALIBRATED_SCREEN_WIDTH"); break;
    case HID_USAGE_CALIBRATED_SCREEN_HEIGHT: printf("HID_USAGE_CALIBRATED_SCREEN_HEIGHT"); break;

        // HID_USAGE_TRACKER_STATUS - Feature Collection 
    case HID_USAGE_DEVICE_STATUS: printf("HID_USAGE_DEVICE_STATUS"); break;
    case HID_USAGE_CONFIGURATION_STATUS: printf("HID_USAGE_CONFIGURATION_STATUS"); break;
    case HID_USAGE_SAMPLING_FREQUENCY: printf("HID_USAGE_SAMPLING_FREQUENCY"); break;

        // HID_USAGE_TRACKER_CONTROL - Feature Collection 
    case HID_USAGE_MODE_REQUEST: printf("HID_USAGE_MODE_REQUEST"); break;
    default: printf("Unknown Usage"); break;
    }
}

VOID
PrintReportType
(
    HIDP_REPORT_TYPE reportType
)
{
    switch (reportType)
    {
    case HidP_Input: printf("HidP_Input"); break;
    case HidP_Output: printf("HidP_Output"); break;
    case HidP_Feature: printf("HidP_Feature"); break;
    default: printf("Unknown ReportType"); break;
    }
}


VOID
PrintUsagePage
(
    USAGE usagePage
)
{
    switch (usagePage)
    {
    case HID_USAGE_PAGE_EYE_HEAD_TRACKER: printf("HID_USAGE_PAGE_EYE_HEAD_TRACKER"); break;
    default: printf("Unknown UsagePage"); break;
    }
}

USHORT
GetLinkCollectionIndex
(
    HIDP_REPORT_TYPE reportType,
    USAGE reportID,
    USAGE usage
)
{
    USHORT reportIdx = 0;

    if (reportType == HidP_Input)
    {
        for (reportIdx = 0; reportIdx < m_Caps.NumberInputValueCaps; ++reportIdx)
        {
            if (m_pInputValueCaps[reportIdx].ReportID == reportID)
            {
                if (usage == 0)
                {
                    return m_pInputValueCaps[reportIdx].LinkCollection;
                }

                // found report
                for (USHORT usageIdx = m_pLinkCollectionNodes[m_pInputValueCaps[reportIdx].LinkCollection].FirstChild; usageIdx != 0; usageIdx = m_pLinkCollectionNodes[usageIdx].NextSibling)
                {
                    if (m_pLinkCollectionNodes[usageIdx].LinkUsage == usage)
                    {
                        return usageIdx;
                    }
                }
            }
        }
    }

    if (reportType == HidP_Output)
    {
        for (reportIdx = 0; reportIdx < m_Caps.NumberOutputValueCaps; ++reportIdx)
        {
            if (m_pOutputValueCaps[reportIdx].ReportID == reportID)
            {
                if (usage == 0)
                {
                    return m_pOutputValueCaps[reportIdx].LinkCollection;
                }

                // found report
                for (USHORT usageIdx = m_pLinkCollectionNodes[m_pOutputValueCaps[reportIdx].LinkCollection].FirstChild; usageIdx != 0; usageIdx = m_pLinkCollectionNodes[usageIdx].NextSibling)
                {
                    if (m_pLinkCollectionNodes[usageIdx].LinkUsage == usage)
                    {
                        return usageIdx;
                    }
                }
            }
        }
    }

    if (reportType == HidP_Feature)
    {
        for (reportIdx = 0; reportIdx < m_Caps.NumberFeatureValueCaps; ++reportIdx)
        {
            if (m_pFeatureValueCaps[reportIdx].ReportID == reportID)
            {
                if (usage == 0)
                {
                    return m_pFeatureValueCaps[reportIdx].LinkCollection;
                }

                // found report
                for (USHORT usageIdx = m_pLinkCollectionNodes[m_pFeatureValueCaps[reportIdx].LinkCollection].FirstChild; usageIdx != 0; usageIdx = m_pLinkCollectionNodes[usageIdx].NextSibling)
                {
                    if (m_pLinkCollectionNodes[usageIdx].LinkUsage == usage)
                    {
                        return usageIdx;
                    }
                }
            }
        }
    }

    return 0;
}

VOID
PrintStatusResult
(
    NTSTATUS status
)
{
    switch (status)
    {
    case HIDP_STATUS_SUCCESS: printf("HIDP_STATUS_SUCCESS"); break;
    case HIDP_STATUS_NULL: printf("HIDP_STATUS_NULL"); break;
    case HIDP_STATUS_INVALID_PREPARSED_DATA: printf("HIDP_STATUS_INVALID_PREPARSED_DATA"); break;
    case HIDP_STATUS_INVALID_REPORT_TYPE: printf("HIDP_STATUS_INVALID_REPORT_TYPE"); break;
    case HIDP_STATUS_INVALID_REPORT_LENGTH: printf("HIDP_STATUS_INVALID_REPORT_LENGTH"); break;
    case HIDP_STATUS_USAGE_NOT_FOUND: printf("HIDP_STATUS_USAGE_NOT_FOUND"); break;
    case HIDP_STATUS_VALUE_OUT_OF_RANGE: printf("HIDP_STATUS_VALUE_OUT_OF_RANGE"); break;
    case HIDP_STATUS_BAD_LOG_PHY_VALUES: printf("HIDP_STATUS_BAD_LOG_PHY_VALUES"); break;
    case HIDP_STATUS_BUFFER_TOO_SMALL: printf("HIDP_STATUS_BUFFER_TOO_SMALL"); break;
    case HIDP_STATUS_INTERNAL_ERROR: printf("HIDP_STATUS_INTERNAL_ERROR"); break;
    case HIDP_STATUS_I8042_TRANS_UNKNOWN: printf("HIDP_STATUS_I8042_TRANS_UNKNOWN"); break;
    case HIDP_STATUS_INCOMPATIBLE_REPORT_ID: printf("HIDP_STATUS_INCOMPATIBLE_REPORT_ID"); break;
    case HIDP_STATUS_NOT_VALUE_ARRAY: printf("HIDP_STATUS_NOT_VALUE_ARRAY"); break;
    case HIDP_STATUS_IS_VALUE_ARRAY: printf("HIDP_STATUS_IS_VALUE_ARRAY"); break;
    case HIDP_STATUS_DATA_INDEX_NOT_FOUND: printf("HIDP_STATUS_DATA_INDEX_NOT_FOUND"); break;
    case HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE: printf("HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE"); break;
    case HIDP_STATUS_BUTTON_NOT_PRESSED: printf("HIDP_STATUS_BUTTON_NOT_PRESSED"); break;
    case HIDP_STATUS_REPORT_DOES_NOT_EXIST: printf("HIDP_STATUS_REPORT_DOES_NOT_EXIST"); break;
    case HIDP_STATUS_NOT_IMPLEMENTED: printf("HIDP_STATUS_NOT_IMPLEMENTED"); break;
    default: printf("Unknown status 0x%08X", status); break;
    }
}

NTSTATUS
GetUsageValue
(
    _In_ HIDP_REPORT_TYPE reportType,
    _In_ USHORT usCollectionIdx,
    _In_ USAGE usage,
    _In_ PCHAR pbBuffer,
    _In_ DWORD cbBuffer,
    _Out_ PULONG ulUsageValue
)
{
    NTSTATUS status = HidP_GetUsageValue(
        reportType,
        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
        usCollectionIdx,
        usage,
        ulUsageValue,
        m_pPpd,
        pbBuffer,
        cbBuffer);
    PrintUsageString(usage);
    printf(" ");
    if (status == HIDP_STATUS_SUCCESS)
    {
        printf("0x%08X", *ulUsageValue);
    }
    else
    {
        PrintStatusResult(status);
    }

    return status;
}