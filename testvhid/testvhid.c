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
GetStrings(
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

BOOLEAN
FindMatchingDevice(
    _In_ LPGUID Guid
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

    srand( (unsigned)time( NULL ) );

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

        bSuccess = GetStrings();
        //if (bSuccess == FALSE) {
        //    goto cleanup;
        //}

        //
        // Get/Set feature loopback
        //
        bSuccess = GetFeatureCapabilities();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetFeatureConfiguration();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetFeatureTrackerStatus();
        if (bSuccess == FALSE) {
            goto cleanup;
        }

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

    //wprintf(L"%s\n", deviceInterfaceList);
    printf("Looking for our HID device (with UP=0x%04X "
        "and Usage=0x%02X)\n\n", HID_USAGE_PAGE_EYE_HEAD_TRACKER, HID_USAGE_EYE_TRACKER);

    //
    // Enumerate devices of this interface class
    //
    for (currentInterface = deviceInterfaceList;
        *currentInterface && m_file == INVALID_HANDLE_VALUE;
        currentInterface += wcslen(currentInterface) + 1) {

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

    if (bSuccess && !HidP_GetCaps (m_pPpd, &m_Caps))
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
    BOOLEAN bSuccess;

    CAPABILITIES_REPORT capabilities = { 0 };
    capabilities.ReportId = HID_USAGE_CAPABILITIES;
    bSuccess = HidD_GetFeature(m_file, &capabilities, sizeof(capabilities));
    if (bSuccess)
    {
        /*
        HidP_GetUsageValue(HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            0,
            HID_USAGE_TRACKER_QUALITY,
            usage,
            Ppd,
            (PCHAR)pbReportBuffer,
            cbReportBuffer
        );
        */

        printf("\n\nHID_USAGE_CAPABILITIES\n");
        printf("Report ID:                   0x%04x\n", capabilities.ReportId);
        printf("Tracker Quality:           %8d\n", capabilities.TrackerQuality);
        if (capabilities.TrackerQuality < 1 || capabilities.TrackerQuality > 4)
        {
            printf("ERROR: Tracker Quality value invalid. Should be 1, 2, 3, 4\n");
        }
        printf("Gaze Location Origin:      %8d\n", capabilities.GazeLocationOrigin);
        if (capabilities.GazeLocationOrigin < 1 || capabilities.GazeLocationOrigin > 4)
        {
            printf("ERROR: Gaze Location Origin value invalid. Should be 1, 2, 3, 4\n");
        }
        printf("Eye Position Origin:       %8d\n", capabilities.EyePositionOrigin);
        if (capabilities.EyePositionOrigin < 1 || capabilities.EyePositionOrigin > 4)
        {
            printf("ERROR: Eye Position Origin value invalid. Should be 1, 2, 3, 4\n");
        }
        printf("Max frames per second:     %8d\n", capabilities.MaxFramesPerSecond);
        printf("Minimum Tracking Distance: %8d\n", capabilities.MinimumTrackingDistance);
        printf("Optimum Tracking Distance: %8d\n", capabilities.OptimumTrackingDistance);
        printf("Maximum Tracking Distance: %8d\n", capabilities.MaximumTrackingDistance);
        printf("Max Screen Plane Width:    %8d\n", capabilities.MaximumScreenPlaneWidth);
        printf("Max Screen Plane Height:   %8d\n", capabilities.MaximumScreenPlaneHeight);
        printf("Screen Plane Curvature:    %8d\n", capabilities.ScreenPlaneCurvature);
        
        printf("Screen Plane Top Left:     (%3d, %3d, %3d)\n", capabilities.ScreenPlaneTopLeft.X, capabilities.ScreenPlaneTopLeft.Y, capabilities.ScreenPlaneTopLeft.Z);
        printf("Screen Plane Top Right:    (%3d, %3d, %3d)\n", capabilities.ScreenPlaneTopRight.X, capabilities.ScreenPlaneTopRight.Y, capabilities.ScreenPlaneTopRight.Z);
        printf("Screen Plane Bottom Left:  (%3d, %3d, %3d)\n", capabilities.ScreenPlaneBottomLeft.X, capabilities.ScreenPlaneBottomLeft.Y, capabilities.ScreenPlaneBottomLeft.Z);
    }
    else
    {
        printf("ERROR: HidD_GetFeature failed. GLE=0x%08x\n", GetLastError());
    }
    return bSuccess;
}

BOOLEAN
GetFeatureConfiguration(
)
{
    BOOLEAN bSuccess;
    CONFIGURATION_REPORT configuration = { 0 };
    configuration.ReportId = HID_USAGE_CONFIGURATION;
    bSuccess = HidD_GetFeature(m_file, &configuration, sizeof(configuration));
    if (bSuccess)
    {
        printf("\n\nHID_USAGE_CONFIGURATION\n");
        printf("Report ID:                   0x%04x\n", configuration.ReportId);

        // https://en.wikipedia.org/wiki/Extended_Display_Identification_Data

        printf("Display Manufacturer Id:     0x%04x\n", configuration.DisplayManufacturerId);
        printf("Display Product Id:          0x%04x\n", configuration.DisplayProductId);
        printf("Display Serial Number:       0x%04d\n", configuration.DisplaySerialNumber);
        printf("Display Manufacturer Date:   0x%04d\n", configuration.DisplayManufacturerDate);
    }
    else
    {
        printf("ERROR: HidD_GetFeature failed. GLE=0x%08x\n", GetLastError());
    }
    return bSuccess;
}

BOOLEAN
GetFeatureTrackerStatus(
)
{
    BOOLEAN bSuccess;
    TRACKER_STATUS_REPORT status = { 0 };
    status.ReportId = HID_USAGE_TRACKER_STATUS;
    bSuccess = HidD_GetFeature(m_file, &status, sizeof(status));
    if (bSuccess)
    {
        printf("\n\nHID_USAGE_TRACKER_STATUS\n");
        printf("Report ID:                   0x%04x\n", status.ReportId);
        printf("Tracker Status:            %8d\n", status.TrackerStatus);
        if (status.TrackerStatus > 1)
        {
            printf("ERROR: Tracker Status value invalid. Should be 0 or 1\n");
        }
        printf("Configuration Status:      %8d\n", status.ConfigurationStatus);
        if (status.ConfigurationStatus > 2)
        {
            printf("ERROR: Configuration Status value invalid. Should be 0, 1, 2\n");
        }
        printf("Tracker Status:            %8d\n", status.FramesPerSecond);
    }
    else
    {
        printf("ERROR: HidD_GetFeature failed. GLE=0x%08x\n", GetLastError());
    }
    return bSuccess;
}

BOOLEAN
SetFeatureTrackerControl(
    uint8_t modeRequest
)
{
    BOOLEAN bSuccess = FALSE;
    TRACKER_CONTROL_REPORT control = { 0 };
    control.ReportId = HID_USAGE_TRACKER_CONTROL;

    if (control.ModeRequest > 1)
    {
        printf("ERROR: Mode Request value invalid. Should be 0 or 1\n");
    }
    else
    {
        control.ModeRequest = modeRequest;
        bSuccess = HidD_SetFeature(m_file, &control, sizeof(control));

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
    GAZE_REPORT report = { 0 };
    BOOL bSuccess = TRUE;
    DWORD bytesRead;

    //
    // Allocate memory
    //
    report.ReportId = HID_USAGE_TRACKING_DATA;

    printf("\n\nHID_USAGE_TRACKING_DATA\n");

    //HDC screen = GetDC(NULL);
    //int hSize = GetDeviceCaps(screen, HORZSIZE);
    //int hRes = GetDeviceCaps(screen, HORZRES);
    //float PixelsPerMM = (float)hRes / hSize;   // pixels per millimeter
    //float PixelsPerInch = PixelsPerMM*25.4f; //dpi

    //printf("[%d %d %f %f\n", hSize, hRes, PixelsPerMM, PixelsPerInch);

    //HDC monitor = GetDC(NULL);
    //int horizSize = GetDeviceCaps(monitor, HORZSIZE);
    //int vertSize = GetDeviceCaps(monitor, VERTSIZE);

    //printf("[%d %d\n", horizSize, vertSize);

    //
    // get input data.
    //
    while ((bSuccess) && (!_kbhit()))
    {
        bSuccess = ReadFile(
        m_file,        // HANDLE hFile, 
        &report,      // LPVOID lpBuffer, 
        sizeof(report),  // DWORD nNumberOfBytesToRead, 
        &bytesRead,  // LPDWORD lpNumberOfBytesRead, 
        NULL         // LPOVERLAPPED lpOverlapped 
        );

        // XXX: calculate monitor width and height manually rather than using calibrated value from sensor 
        //const int micrometersPerInch = 25400;

        //auto di = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
        //_monitorWidth = ((float)di->ScreenWidthInRawPixels / di->RawDpiX) * micrometersPerInch;
        //_monitorHeight = ((float)di->ScreenHeightInRawPixels / di->RawDpiY) * micrometersPerInch;

        if (!bSuccess)
        {
            printf("ERROR: ReadFile failed. GLE=0x%08x\n", GetLastError());
        }
        else
        {
            printf("[0x%lld - [%1.20f, %1.20f]\n", report.TimeStamp, report.GazePoint.X, report.GazePoint.Y);
        }
    }

    if (_kbhit())
    {
        _getch();
    }

    return (BOOLEAN) bSuccess;
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
    if (!buffer )
    {
        printf("calloc failed\n");
        return FALSE;
    }

    //ZeroMemory(buffer, bufferLength);

    bSuccess = HidD_GetIndexedString(
        m_file,
        VHIDMINI_DEVICE_STRING_INDEX,  // IN ULONG  StringIndex,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetIndexedString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Indexed string: %S\n", (PWSTR) buffer);
    }

    free(buffer);

    return bSuccess;
}

BOOLEAN
GetStrings(
    )
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = calloc(bufferLength, sizeof(BYTE));
    if (!buffer )
    {
        printf("calloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetProductString(
        m_file,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetProductString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Product string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetSerialNumberString(
        m_file,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetSerialNumberString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Serial number string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetManufacturerString(
        m_file,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetManufacturerString failed. GLE=0x%08x\n", GetLastError());
    }
    else
    {
        printf("Manufacturer string: %S\n", (PWSTR) buffer);
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
    USHORT length,
    PHIDP_VALUE_CAPS* ppValueCaps
)
{
    BOOLEAN bSuccess = TRUE;

    if (length > 0)
    {
        *ppValueCaps = calloc(length, sizeof(HIDP_VALUE_CAPS));
        //ZeroMemory(*ppValueCaps, length * sizeof(HIDP_VALUE_CAPS));

        HidP_GetValueCaps(
            reportType,
            *ppValueCaps,
            &length,
            m_pPpd
        );

        switch (reportType)
        {
        case HidP_Input:
            printf("ReportType:           HidP_Input\n");
            break;
        case HidP_Output:
            printf("ReportType:           HidP_Output\n");
            break;
        case HidP_Feature:
            printf("ReportType:           HidP_Feature\n");
            break;
        }
        printf("UsagePage:            0x%04x\n", (*ppValueCaps)->UsagePage);
        printf("ReportID:             0x%04x\n", (*ppValueCaps)->ReportID);
        printf("LinkCollection:     %8d\n", (*ppValueCaps)->LinkCollection);
        printf("LinkUsage:            0x%04x\n", (*ppValueCaps)->LinkUsage);
        printf("LinkUsagePage:        0x%04x\n", (*ppValueCaps)->LinkUsagePage);
        printf("\n");
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
    USHORT length,
    PHIDP_BUTTON_CAPS *ppButtonCaps
)
{
    BOOLEAN bSuccess = TRUE;

    if (length > 0)
    {
        *ppButtonCaps = calloc(length, sizeof(HIDP_BUTTON_CAPS));
        //ZeroMemory(*ppButtonCaps, length * sizeof(HIDP_BUTTON_CAPS));

        HidP_GetButtonCaps(
            reportType,
            *ppButtonCaps,
            &length,
            m_pPpd
        );

        switch (reportType)
        {
        case HidP_Input:
            printf("ReportType:           HidP_Input\n");
            break;
        case HidP_Output:
            printf("ReportType:           HidP_Output\n");
            break;
        case HidP_Feature:
            printf("ReportType:           HidP_Feature\n");
            break;
        }
        printf("UsagePage:            0x%04x\n", (*ppButtonCaps)->UsagePage);
        printf("ReportID:             0x%04x\n", (*ppButtonCaps)->ReportID);
        printf("LinkCollection:     %8d\n", (*ppButtonCaps)->LinkCollection);
        printf("LinkUsage:            0x%04x\n", (*ppButtonCaps)->LinkUsage);
        printf("LinkUsagePage:        0x%04x\n", (*ppButtonCaps)->LinkUsagePage);
        printf("\n");
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
    printf("Usage:                0x%04x\n", m_pLinkCollectionNodes[currentNodeIdx].LinkUsage);
    if (m_pLinkCollectionNodes[currentNodeIdx].LinkUsage == 0)
    {
        printf("Error: Invalid Usage, should not be 0\n");
    }
    printf("UsagePage:            0x%04x\n", m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage);
    if (m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage != HID_USAGE_PAGE_EYE_HEAD_TRACKER)
    {
        printf("Error: Invalid Usage Page, should be 0x%04x\n", HID_USAGE_PAGE_EYE_HEAD_TRACKER);
    }
    printf("CollectionType:     %8d\n", m_pLinkCollectionNodes[currentNodeIdx].CollectionType);
    printf("IsAlias:            %8d\n", m_pLinkCollectionNodes[currentNodeIdx].IsAlias);
    printf("Parent:             %8d\n", m_pLinkCollectionNodes[currentNodeIdx].Parent);
    printf("NumberOfChildren:   %8d\n", m_pLinkCollectionNodes[currentNodeIdx].NumberOfChildren);
    printf("FirstChild:         %8d\n", m_pLinkCollectionNodes[currentNodeIdx].FirstChild);
    printf("NextSibling:        %8d\n", m_pLinkCollectionNodes[currentNodeIdx].NextSibling);
    printf("\n");
}


