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

#include "testvhid.h"

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

    LONG h, v;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    GetDesktopResolution(&m_lScreenWidth, &m_lScreenHeight);

    HidD_GetHidGuid(&m_hidguid);

    found = FindMatchingDevice(&m_hidguid);

    if (found) {
        PrintLinkCollectionNodes();
        
        PrintValueCaps();
        PrintButtonCaps();
        
        //
        // Get Strings
        //
        PrintWellKnownStrings();

        PrintFeatureCapabilities();
        PrintFeatureConfiguration();
        PrintFeatureTrackerStatus();

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

    Dispose();

    return (bSuccess ? 0 : 1);
}

VOID
Dispose(
)
{
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
}

BOOLEAN
FindMatchingDevice(
    _In_  LPGUID  interfaceGuid
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
        interfaceGuid,
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

    cr = CM_Get_Device_Interface_List(
        interfaceGuid,
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

            GetWellKnownStrings();
            GetIndexedString();
            GetLinkCollectionNodes();

            GetValueCaps(HidP_Input, m_Caps.NumberInputValueCaps, &m_pInputValueCaps);
            GetValueCaps(HidP_Output, m_Caps.NumberOutputValueCaps, &m_pOutputValueCaps);
            GetValueCaps(HidP_Feature, m_Caps.NumberFeatureValueCaps, &m_pFeatureValueCaps);

            GetButtonCaps(HidP_Input, m_Caps.NumberInputButtonCaps, &m_pInputButtonCaps);
            GetButtonCaps(HidP_Output, m_Caps.NumberOutputButtonCaps, &m_pOutputButtonCaps);

            GetFeatureCapabilities();
            GetFeatureConfiguration();
            GetFeatureTrackerStatus();
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
    NTSTATUS status;
    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_CAPABILITIES;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    ZeroMemory(&m_capabilitiesReport, sizeof(CAPABILITIES_REPORT));

    if (HidD_GetFeature(m_file, pbBuffer, cbBuffer))
    {
        m_capabilitiesReport.ReportId = (uint8_t)reportID;

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_TRACKER_QUALITY,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.TrackerQuality = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_TRACKER_QUALITY,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.TrackerQuality = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_GAZE_LOCATION_ORIGIN,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.GazeLocationOrigin = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_EYE_POSITION_ORIGIN,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.EyePositionOrigin = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.MaxFramesPerSecond = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_MINIMUM_TRACKING_DISTANCE,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.MinimumTrackingDistance = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_OPTIMUM_TRACKING_DISTANCE,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.OptimumTrackingDistance = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_MAXIMUM_TRACKING_DISTANCE,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.MaximumTrackingDistance = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.MaximumScreenPlaneWidth = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_capabilitiesReport.MaximumScreenPlaneHeight = (uint16_t)ulUsageValue;
        }
    }

    free(pbBuffer);

    return bSuccess;
}

VOID
PrintFeatureCapabilities(
)
{
    printf("ReportID:   0x%04X %s\n", m_capabilitiesReport.ReportId, GetUsageString(m_capabilitiesReport.ReportId));

    printf("0x%04X %s\n", m_capabilitiesReport.TrackerQuality, GetTrackerQualityString(m_capabilitiesReport.TrackerQuality));
    printf("0x%04X %s\n", m_capabilitiesReport.GazeLocationOrigin, GetCoordinateSystemString(m_capabilitiesReport.GazeLocationOrigin));
    printf("0x%04X %s\n", m_capabilitiesReport.EyePositionOrigin, GetCoordinateSystemString(m_capabilitiesReport.EyePositionOrigin));

    printf("0x%04X Maximum Sampling Frequency %d Hz\n", m_capabilitiesReport.MaxFramesPerSecond, m_capabilitiesReport.MaxFramesPerSecond);
    printf("0x%04X Miniumum Tracking Distance %d millimeters\n", m_capabilitiesReport.MinimumTrackingDistance, m_capabilitiesReport.MinimumTrackingDistance);
    printf("0x%04X Optimum Tracking Distance %d millimeters\n", m_capabilitiesReport.OptimumTrackingDistance, m_capabilitiesReport.OptimumTrackingDistance);
    printf("0x%04X Maximum Tracking Distance %d millimeters\n", m_capabilitiesReport.MaximumTrackingDistance, m_capabilitiesReport.MaximumTrackingDistance);
    printf("0x%04X Maximum Screen Plane Width %d micrometers\n", m_capabilitiesReport.MaximumScreenPlaneWidth, m_capabilitiesReport.MaximumScreenPlaneWidth);
    printf("0x%04X Maximum Screen Plane Height %d micrometers\n", m_capabilitiesReport.MaximumScreenPlaneHeight, m_capabilitiesReport.MaximumScreenPlaneHeight);

    printf("\n");
}

BOOLEAN
GetFeatureConfiguration(
)
{
    BOOLEAN bSuccess = TRUE;
    NTSTATUS status;
    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_CONFIGURATION;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    ZeroMemory(&m_configurationReport, sizeof(CONFIGURATION_REPORT));

    if (HidD_GetFeature(m_file, pbBuffer, cbBuffer))
    {
        m_configurationReport.ReportId = (uint8_t)reportID;

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_DISPLAY_MANUFACTURER_ID,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.DisplayManufacturerId = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_DISPLAY_PRODUCT_ID,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.DisplayProductId = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_DISPLAY_SERIAL_NUMBER,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.DisplaySerialNumber = (uint32_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_DISPLAY_MANUFACTURER_DATE,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.DisplayManufacturerDate = (uint16_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_CALIBRATED_SCREEN_WIDTH,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.CalibratedScreenWidth = (uint32_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_CALIBRATED_SCREEN_HEIGHT,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_configurationReport.CalibratedScreenHeight = (uint32_t)ulUsageValue;
        }
    }

    free(pbBuffer);

    return bSuccess;
}

VOID
PrintFeatureConfiguration(
)
{
    printf("ReportID:   0x%04X %s\n", m_configurationReport.ReportId, GetUsageString(m_configurationReport.ReportId));

    printf("EDID Display Manufacturer Id %d\n", m_configurationReport.DisplayManufacturerId);
    printf("EDID Display Product Id %d\n", m_configurationReport.DisplayProductId);
    printf("EDID Display Manufacturer Date %d\n", m_configurationReport.DisplayManufacturerDate);
    printf("EDID Display Serial Number %d\n", m_configurationReport.DisplaySerialNumber);

    printf("Calibrated Screen Width %d millimeters\n", m_configurationReport.CalibratedScreenWidth);
    printf("Calibrated Screen Height %d millimeters\n", m_configurationReport.CalibratedScreenHeight);

    printf("\n");
}

BOOLEAN
GetFeatureTrackerStatus(
)
{
    BOOLEAN bSuccess = TRUE;
    NTSTATUS status;
    PCHAR pbBuffer = NULL;
    USHORT cbBuffer = m_Caps.FeatureReportByteLength;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    USAGE reportID = HID_USAGE_TRACKER_STATUS;

    pbBuffer = calloc(cbBuffer, sizeof(CHAR));

    pbBuffer[0] = (CHAR)reportID;

    usCollectionIdx = GetLinkCollectionIndex(HidP_Feature, reportID, 0);

    ZeroMemory(&m_trackerStatusReport, sizeof(TRACKER_STATUS_REPORT));

    if (HidD_GetFeature(m_file, pbBuffer, cbBuffer))
    {
        m_trackerStatusReport.ReportId = (uint8_t)reportID;

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_DEVICE_STATUS,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_trackerStatusReport.DeviceStatus = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_CONFIGURATION_STATUS,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_trackerStatusReport.ConfigurationStatus = (uint8_t)ulUsageValue;
        }

        status = HidP_GetUsageValue(
            HidP_Feature,
            HID_USAGE_PAGE_EYE_HEAD_TRACKER,
            usCollectionIdx,
            HID_USAGE_SAMPLING_FREQUENCY,
            &ulUsageValue,
            m_pPpd,
            pbBuffer,
            cbBuffer);
        if (status == HIDP_STATUS_SUCCESS)
        {
            m_trackerStatusReport.SamplingFrequency = (uint16_t)ulUsageValue;
        }


    }

    free(pbBuffer);

    return bSuccess;
}

VOID
PrintFeatureTrackerStatus(
)
{
    printf("ReportID:   0x%04X %s\n", m_trackerStatusReport.ReportId, GetUsageString(m_trackerStatusReport.ReportId));
    printf("%s\n", GetDeviceStatusString(m_trackerStatusReport.DeviceStatus));
    printf("%s\n", GetConfigurationStatusString(m_trackerStatusReport.ConfigurationStatus));
    printf("Sensor Sampling Frequency %d Hz\n", m_trackerStatusReport.SamplingFrequency);

    printf("\n");
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
    PBYTE pbBuffer = NULL;
    NTSTATUS status;
    USHORT cbBuffer = m_Caps.InputReportByteLength;
    DWORD bytesRead;
    USAGE reportID = 0;
    USHORT usCollectionIdx;
    ULONG ulUsageValue;
    UINT64 ullUsageValue;
    LONG lPositionX;
    LONG lPositionY;

    pbBuffer = calloc(cbBuffer, sizeof(BYTE));

    //
    // get input data.
    //
    while ((bSuccess) && (!_kbhit()))
    {
        reportID = 0;
        ZeroMemory(pbBuffer, cbBuffer);
        lPositionX = 0;
        lPositionY = 0;
        ullUsageValue = 0;

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
            printf("ReportID:0x%04X %s ", reportID, GetUsageString(reportID));

            switch (reportID)
            {
            case HID_USAGE_TRACKING_DATA:
                {
                    ZeroMemory(&m_gazeReport, sizeof(GAZE_REPORT));

                    usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, 0);

                    status = HidP_GetUsageValueArray(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_TIMESTAMP,
                        (PCHAR)(&ullUsageValue),
                        sizeof(ullUsageValue),
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.TimeStamp = ullUsageValue;
                    }

                    usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_GAZE_LOCATION);
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_X,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.GazePoint.X = ulUsageValue;
                    }
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_Y,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.GazePoint.Y = ulUsageValue;
                    }

                    usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_LEFT_EYE_POSITION);
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_X,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.LeftEyePosition.X = ulUsageValue;
                    }
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_Y,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.LeftEyePosition.Y = ulUsageValue;
                    }
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_Z,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.LeftEyePosition.Z = ulUsageValue;
                    }

                    usCollectionIdx = GetLinkCollectionIndex(HidP_Input, HID_USAGE_TRACKING_DATA, HID_USAGE_RIGHT_EYE_POSITION);
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_X,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.RightEyePosition.X = ulUsageValue;
                    }
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_Y,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.RightEyePosition.Y = ulUsageValue;
                    }
                    status = HidP_GetUsageValue(
                        HidP_Input,
                        HID_USAGE_PAGE_EYE_HEAD_TRACKER,
                        usCollectionIdx,
                        HID_USAGE_POSITION_Z,
                        &ulUsageValue,
                        m_pPpd,
                        pbBuffer,
                        cbBuffer);
                    if (status == HIDP_STATUS_SUCCESS)
                    {
                        m_gazeReport.RightEyePosition.Z = ulUsageValue;
                    }

                    // TODO remove the 1000.0 multiplier once the calibrated screen width and height are returned in micrometers
                    // rather than millimeters. Ensure that the GazePoint X and Y match 
                    DOUBLE dX = (m_gazeReport.GazePoint.X * 1.0) / (m_configurationReport.CalibratedScreenWidth * 1000.0);
                    DOUBLE dY = (m_gazeReport.GazePoint.Y * 1.0) / (m_configurationReport.CalibratedScreenHeight * 1000.0);

                    ULONG posX = m_lScreenWidth * dX;
                    ULONG posY = m_lScreenHeight * dY;

                    SetCursorPos(posX, posY);

                    printf("%lld - GazePoint (%d,%d) (%0.3f%%, %0.3f%%) - Eye Position L(%d,%d,%d) R(%d,%d,%d)\n",
                        m_gazeReport.TimeStamp,
                        posX, posY,
                        dX, dY,
                        m_gazeReport.LeftEyePosition.X, m_gazeReport.LeftEyePosition.Y, m_gazeReport.LeftEyePosition.Z,
                        m_gazeReport.RightEyePosition.X, m_gazeReport.RightEyePosition.Y, m_gazeReport.RightEyePosition.Z
                        );
                }
                break;
            case HID_USAGE_TRACKER_STATUS:
            {
                PrintFeatureTrackerStatus();
            }
                break;
            default:
                printf("ERROR: Unknown Input Report\n");
                break;

            }
            printf("\n");
        }
    }

    if (_kbhit())
    {
        _getch();
    }

    free(pbBuffer);
    pbBuffer = NULL;

    return (BOOLEAN)bSuccess;
}


BOOLEAN
GetIndexedString(
)
{
    BOOLEAN bSuccess;

    bSuccess = HidD_GetIndexedString(
        m_file,
        VHIDMINI_DEVICE_STRING_INDEX,  // IN ULONG  StringIndex,
        (PVOID)m_pvhidminiDeviceString,  //OUT PVOID  Buffer,
        MAXIMUM_STRING_LENGTH // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetIndexedString failed. GLE=0x%08x\n", GetLastError());
    }

    return bSuccess;
}

BOOLEAN
GetWellKnownStrings(
)
{
    BOOLEAN bSuccess;

    bSuccess = HidD_GetProductString(
        m_file,
        (PVOID)m_pProductString,  //OUT PVOID  Buffer,
        MAXIMUM_STRING_LENGTH // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetProductString failed. GLE=0x%08x\n", GetLastError());
    }

    bSuccess = HidD_GetSerialNumberString(
        m_file,
        (PVOID)m_pSerialNumberString,  //OUT PVOID  Buffer,
        MAXIMUM_STRING_LENGTH // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetSerialNumberString failed. GLE=0x%08x\n", GetLastError());
    }

    bSuccess = HidD_GetManufacturerString(
        m_file,
        (PVOID)m_pManufacturerString,  //OUT PVOID  Buffer,
        MAXIMUM_STRING_LENGTH // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetManufacturerString failed. GLE=0x%08x\n", GetLastError());
    }

    bSuccess = HidD_GetPhysicalDescriptor(
        m_file,
        (PVOID)m_pPhysicalDescriptorString,  //OUT PVOID  Buffer,
        MAXIMUM_STRING_LENGTH // IN ULONG  BufferLength
    );

    if (!bSuccess)
    {
        printf("ERROR: HidD_GetPhysicalDescriptor failed. GLE=0x%08x\n", GetLastError());
    }

    return bSuccess;
}

VOID
PrintWellKnownStrings(
)
{
    printf("VHID Mini Device string:    %S\n", m_pvhidminiDeviceString);
    printf("Product string:             %S\n", m_pProductString);
    printf("Serial number string:       %S\n", m_pSerialNumberString);
    printf("Manufacturer string:        %S\n", m_pManufacturerString);
    printf("Physical Descriptor string: %S\n", m_pPhysicalDescriptorString);

    printf("\n");
}

VOID
PrintValueCaps(
)
{
    printf("ValueCaps\n");
    PrintValueCapsEX(HidP_Input, m_Caps.NumberInputValueCaps, &m_pInputValueCaps);
    PrintValueCapsEX(HidP_Output, m_Caps.NumberOutputValueCaps, &m_pOutputValueCaps);
    PrintValueCapsEX(HidP_Feature, m_Caps.NumberFeatureValueCaps, &m_pFeatureValueCaps);
    printf("\n");
}

BOOLEAN
GetValueCaps(
    HIDP_REPORT_TYPE reportType,
    USHORT valueCapsLength,
    PHIDP_VALUE_CAPS* ppValueCaps
)
{
    BOOLEAN bSuccess = TRUE;

    if (valueCapsLength > 0)
    {
        *ppValueCaps = calloc(valueCapsLength, sizeof(HIDP_VALUE_CAPS));

        HidP_GetValueCaps(
            reportType,
            *ppValueCaps,
            &valueCapsLength,
            m_pPpd
        );
    }

    return bSuccess;
}

VOID
PrintValueCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT valueCapsLength,
    PHIDP_VALUE_CAPS* ppValueCaps
)
{
    for (int idx = 0; idx < valueCapsLength; ++idx)
    {
        printf("ReportType:           %s\n", GetReportTypeString(reportType));
        printf("ReportCount:          %6d\n", (*ppValueCaps)[idx].ReportCount);
        printf("UsagePage:            0x%04X %s\n", (*ppValueCaps)[idx].UsagePage, GetUsagePageString((*ppValueCaps)[idx].UsagePage));
        printf("ReportID:             0x%04X %s\n", (*ppValueCaps)[idx].ReportID, GetUsageString((*ppValueCaps)[idx].ReportID));
        printf("LinkCollection:       %6d (%s)\n", (*ppValueCaps)[idx].LinkCollection, GetCollectionTypeString(m_pLinkCollectionNodes[(*ppValueCaps)[idx].LinkCollection].CollectionType));
        printf("LinkUsage:            0x%04X %s\n", (*ppValueCaps)[idx].LinkUsage, GetUsageString((*ppValueCaps)[idx].LinkUsage));
        printf("LinkUsagePage:        0x%04X %s\n", (*ppValueCaps)[idx].LinkUsagePage, GetUsagePageString((*ppValueCaps)[idx].LinkUsagePage));
        printf("BitSize:              0x%04X %d bits\n", (*ppValueCaps)[idx].BitSize, (*ppValueCaps)[idx].BitSize);
        printf("BitField:             0x%04X\n", (*ppValueCaps)[idx].BitField);

        printf("IsAlias:              %s\n", (*ppValueCaps)[idx].IsAlias ? "True" : "False");
        printf("IsRange:              %s\n", (*ppValueCaps)[idx].IsRange ? "True" : "False");
        printf("IsStringRange:        %s\n", (*ppValueCaps)[idx].IsStringRange ? "True" : "False");
        printf("IsDesignatorRange:    %s\n", (*ppValueCaps)[idx].IsDesignatorRange ? "True" : "False");
        printf("IsAbsolute:           %s\n", (*ppValueCaps)[idx].IsAbsolute ? "True" : "False");
        printf("HasNull:              %s\n", (*ppValueCaps)[idx].HasNull ? "True" : "False");

        printf("Units:                0x%04X %s\n", (*ppValueCaps)[idx].Units, UnitsToString((*ppValueCaps)[idx].Units));
        if ((*ppValueCaps)[idx].Units == 0 && (*ppValueCaps)[idx].UnitsExp == 0)
        {
            printf("UnitsExp:             0x%04X\n", (*ppValueCaps)[idx].UnitsExp);
        }
        else
        {
            printf("UnitsExp:             0x%04X 10^%d\n", (*ppValueCaps)[idx].UnitsExp, CodeToExponent((BYTE)((*ppValueCaps)[idx].UnitsExp)));
        }
        printf("Logical Range:        %d (min) %d (max) %s\n", (*ppValueCaps)[idx].LogicalMin, (*ppValueCaps)[idx].LogicalMax, IsLogicalMinMaxSigned((*ppValueCaps)[idx].LogicalMin, (*ppValueCaps)[idx].LogicalMax));
        if ((*ppValueCaps)[idx].PhysicalMin== 0 && (*ppValueCaps)[idx].PhysicalMax == 0)
        {
            printf("Physical Range:       %d (min) %d (max) (assumed equal to Logical Min & Max)\n", (*ppValueCaps)[idx].LogicalMin, (*ppValueCaps)[idx].LogicalMax);
        }
        else
        {
            printf("Physical Range:       %d (min) %d (max)\n", (*ppValueCaps)[idx].PhysicalMin, (*ppValueCaps)[idx].PhysicalMax);
        }

        if ((*ppValueCaps)[idx].IsRange)
        {
            printf("Usage Range:          %d (min) %d (max)\n", (*ppValueCaps)[idx].Range.UsageMin, (*ppValueCaps)[idx].Range.UsageMin);
            printf("String Range:         %d (min) %d (max)\n", (*ppValueCaps)[idx].Range.StringMin, (*ppValueCaps)[idx].Range.StringMax);
            printf("Designator Range:     %d (min) %d (max)\n", (*ppValueCaps)[idx].Range.DesignatorMin, (*ppValueCaps)[idx].Range.DesignatorMax);
            printf("Data Index Range:     %d (min) %d (max)\n", (*ppValueCaps)[idx].Range.DataIndexMin, (*ppValueCaps)[idx].Range.DataIndexMax);
        }
        else
        {
            printf("Usage Index:          0x%04X %s\n", (*ppValueCaps)[idx].NotRange.Usage, GetUsageString((*ppValueCaps)[idx].NotRange.Usage));
            printf("String Index:         0x%04X\n", (*ppValueCaps)[idx].NotRange.StringIndex);
            printf("Designator Index:     0x%04X\n", (*ppValueCaps)[idx].NotRange.DesignatorIndex);
            printf("Data Index Index:     0x%04X\n", (*ppValueCaps)[idx].NotRange.DataIndex);
        }

        printf("\n");
    }
}

VOID
PrintButtonCaps(
)
{
    printf("ButtonCaps\n");
    PrintButtonCapsEX(HidP_Input, m_Caps.NumberInputButtonCaps, &m_pInputButtonCaps);
    PrintButtonCapsEX(HidP_Output, m_Caps.NumberOutputButtonCaps, &m_pOutputButtonCaps);
    printf("\n");
}

BOOLEAN
GetButtonCaps(
    HIDP_REPORT_TYPE reportType,
    USHORT buttonCapsLength,
    PHIDP_BUTTON_CAPS *ppButtonCaps
)
{
    BOOLEAN bSuccess = TRUE;

    if (buttonCapsLength > 0)
    {
        *ppButtonCaps = calloc(buttonCapsLength, sizeof(HIDP_BUTTON_CAPS));

        HidP_GetButtonCaps(
            reportType,
            *ppButtonCaps,
            &buttonCapsLength,
            m_pPpd
        );
    }

    return bSuccess;
}

VOID
PrintButtonCapsEX(
    HIDP_REPORT_TYPE reportType,
    USHORT buttonCapsLength,
    PHIDP_BUTTON_CAPS *ppButtonCaps
)
{
    for (int idx = 0; idx < buttonCapsLength; ++idx)
    {
        printf("ReportType:           %s\n", GetReportTypeString(reportType));
        printf("UsagePage:            0x%04X %s\n", (*ppButtonCaps)[idx].UsagePage, GetUsagePageString((*ppButtonCaps)[idx].UsagePage));
        printf("ReportID:             0x%04X\n", (*ppButtonCaps)[idx].ReportID);
        printf("LinkCollection:       %6d\n", (*ppButtonCaps)[idx].LinkCollection);
        printf("LinkUsage:            0x%04X %s\n", (*ppButtonCaps)[idx].LinkUsage, GetUsagePageString((*ppButtonCaps)[idx].LinkUsage));
        printf("LinkUsagePage:        0x%04X %s\n", (*ppButtonCaps)[idx].LinkUsagePage, GetUsagePageString((*ppButtonCaps)[idx].LinkUsagePage));
        printf("BitField:             0x%04X\n", (*ppButtonCaps)[idx].BitField);

        printf("IsAlias:              %s\n", (*ppButtonCaps)[idx].IsAlias ? "True" : "False");
        printf("IsRange:              %s\n", (*ppButtonCaps)[idx].IsRange ? "True" : "False");
        printf("IsStringRange:        %s\n", (*ppButtonCaps)[idx].IsStringRange ? "True" : "False");
        printf("IsDesignatorRange:    %s\n", (*ppButtonCaps)[idx].IsDesignatorRange ? "True" : "False");
        printf("IsAbsolute:           %s\n", (*ppButtonCaps)[idx].IsAbsolute ? "True" : "False");

        if ((*ppButtonCaps)[idx].IsRange)
        {
            printf("Usage Range:          %d (min) %d (max)\n", (*ppButtonCaps)[idx].Range.UsageMin, (*ppButtonCaps)[idx].Range.UsageMin);
            printf("String Range:         %d (min) %d (max)\n", (*ppButtonCaps)[idx].Range.StringMin, (*ppButtonCaps)[idx].Range.StringMax);
            printf("Designator Range:     %d (min) %d (max)\n", (*ppButtonCaps)[idx].Range.DesignatorMin, (*ppButtonCaps)[idx].Range.DesignatorMax);
            printf("Data Index Range:     %d (min) %d (max)\n", (*ppButtonCaps)[idx].Range.DataIndexMin, (*ppButtonCaps)[idx].Range.DataIndexMax);
        }
        else
        {
            printf("Usage Index:          0x%04X\n", (*ppButtonCaps)[idx].NotRange.Usage);
            printf("String Index:         0x%04X\n", (*ppButtonCaps)[idx].NotRange.StringIndex);
            printf("Designator Index:     0x%04X\n", (*ppButtonCaps)[idx].NotRange.DesignatorIndex);
            printf("Data Index Index:     0x%04X\n", (*ppButtonCaps)[idx].NotRange.DataIndex);
        }

        printf("\n");
    }
}

BOOLEAN
GetLinkCollectionNodes(
)
{
    ULONG ulLinkCollectionNodesLength = m_Caps.NumberLinkCollectionNodes;

    if (ulLinkCollectionNodesLength > 0)
    {
        m_pLinkCollectionNodes = calloc(ulLinkCollectionNodesLength, sizeof(HIDP_LINK_COLLECTION_NODE));

        HidP_GetLinkCollectionNodes(
            m_pLinkCollectionNodes,
            &ulLinkCollectionNodesLength,
            m_pPpd
        );
    }

    return TRUE;
}

BOOLEAN
TraverseLinkCollectionNodes(
    ULONG currentNodeIdx
)
{
    PrintLinkCollectionNode(currentNodeIdx);

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
PrintLinkCollectionNodes(
)
{
    ULONG ulLinkCollectionNodesLength = m_Caps.NumberLinkCollectionNodes;
    for (ULONG ulCurrentNodeIdx = 0; ulCurrentNodeIdx < ulLinkCollectionNodesLength; ++ulCurrentNodeIdx)
    {
        PrintLinkCollectionNode(ulCurrentNodeIdx);
    }
}

VOID
PrintLinkCollectionNode(
    int currentNodeIdx
)
{
    printf("Index:              %6d\n", currentNodeIdx);
    printf("Usage:              0x%04X %s\n", m_pLinkCollectionNodes[currentNodeIdx].LinkUsage, GetUsageString(m_pLinkCollectionNodes[currentNodeIdx].LinkUsage));
    printf("UsagePage:          0x%04X %s\n", m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage, GetUsagePageString(m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage));
    if (m_pLinkCollectionNodes[currentNodeIdx].LinkUsagePage != HID_USAGE_PAGE_EYE_HEAD_TRACKER)
    {
        printf("Error: Invalid Usage Page, should be 0x%04X\n", HID_USAGE_PAGE_EYE_HEAD_TRACKER);
    }
    printf("CollectionType:     %6d %s\n", m_pLinkCollectionNodes[currentNodeIdx].CollectionType, GetCollectionTypeString(m_pLinkCollectionNodes[currentNodeIdx].CollectionType));
    printf("IsAlias:            %6d\n", m_pLinkCollectionNodes[currentNodeIdx].IsAlias);
    printf("Parent:             %6d\n", m_pLinkCollectionNodes[currentNodeIdx].Parent);
    printf("NumberOfChildren:   %6d ", m_pLinkCollectionNodes[currentNodeIdx].NumberOfChildren); PrintLinkCollectionNodeChildren(currentNodeIdx); printf("\n");
    printf("FirstChild:         %6d\n", m_pLinkCollectionNodes[currentNodeIdx].FirstChild);
    printf("NextSibling:        %6d\n", m_pLinkCollectionNodes[currentNodeIdx].NextSibling);
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

CHAR*
GetCollectionTypeString
(
    int collectionType
)
{
    CHAR* retval = NULL;

    // From HID Spec 6.2.2.6
    switch (collectionType)
    {
    case 0:     retval = "Physical"; break;
    case 1:     retval = "Application"; break;
    case 2:     retval = "Logical"; break;
    case 3:     retval = "Report"; break;
    case 4:     retval = "Named Array"; break;
    case 5:     retval = "Usage Switch"; break;
    case 6:     retval = "Usage Modifier"; break;
    default:    retval = "ERROR: Unknown Collection Type"; break;
    }

    return retval;
}

CHAR*
GetUsageString(
    USAGE usage
)
{
    CHAR* retval = NULL;

    switch (usage)
    {
    case HID_USAGE_UNDEFINED:                       retval = "HID_USAGE_UNDEFINED"; break;
    case HID_USAGE_EYE_TRACKER:                     retval = "HID_USAGE_EYE_TRACKER"; break;
    case HID_USAGE_HEAD_TRACKER:                    retval = "HID_USAGE_HEAD_TRACKER"; break;

        // HID_REPORT_ID List
    case HID_USAGE_TRACKING_DATA:                   retval = "HID_USAGE_TRACKING_DATA"; break;
    case HID_USAGE_CAPABILITIES:                    retval = "HID_USAGE_CAPABILITIES"; break;
    case HID_USAGE_CONFIGURATION:                   retval = "HID_USAGE_CONFIGURATION"; break;
    case HID_USAGE_TRACKER_STATUS:                  retval = "HID_USAGE_TRACKER_STATUS"; break;
    case HID_USAGE_TRACKER_CONTROL:                 retval = "HID_USAGE_TRACKER_CONTROL"; break;

        // HID_USAGE_TRACKING_DATA - Input Collection
    case HID_USAGE_TIMESTAMP:                       retval = "HID_USAGE_TIMESTAMP"; break;
    case HID_USAGE_POSITION_X:                      retval = "HID_USAGE_POSITION_X"; break;
    case HID_USAGE_POSITION_Y:                      retval = "HID_USAGE_POSITION_Y"; break;
    case HID_USAGE_POSITION_Z:                      retval = "HID_USAGE_POSITION_Z"; break;
    case HID_USAGE_GAZE_LOCATION:                   retval = "HID_USAGE_GAZE_LOCATION"; break;
    case HID_USAGE_LEFT_EYE_POSITION:               retval = "HID_USAGE_LEFT_EYE_POSITION"; break;
    case HID_USAGE_RIGHT_EYE_POSITION:              retval = "HID_USAGE_RIGHT_EYE_POSITION"; break;
    case HID_USAGE_HEAD_POSITION:                   retval = "HID_USAGE_HEAD_POSITION"; break;

        // HID_USAGE_CAPABILITIES - Feature Collection 
    case HID_USAGE_TRACKER_QUALITY:                 retval = "HID_USAGE_TRACKER_QUALITY"; break;
    case HID_USAGE_GAZE_LOCATION_ORIGIN:            retval = "HID_USAGE_GAZE_LOCATION_ORIGIN"; break;
    case HID_USAGE_EYE_POSITION_ORIGIN:             retval = "HID_USAGE_EYE_POSITION_ORIGIN"; break;
    case HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY:      retval = "HID_USAGE_MAXIMUM_SAMPLING_FREQUENCY"; break;
    case HID_USAGE_MINIMUM_TRACKING_DISTANCE:       retval = "HID_USAGE_MINIMUM_TRACKING_DISTANCE"; break;
    case HID_USAGE_OPTIMUM_TRACKING_DISTANCE:       retval = "HID_USAGE_OPTIMUM_TRACKING_DISTANCE"; break;
    case HID_USAGE_MAXIMUM_TRACKING_DISTANCE:       retval = "HID_USAGE_MAXIMUM_TRACKING_DISTANCE"; break;
    case HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH:      retval = "HID_USAGE_MAXIMUM_SCREEN_PLANE_WIDTH"; break;
    case HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT:     retval = "HID_USAGE_MAXIMUM_SCREEN_PLANE_HEIGHT"; break;

        // HID_USAGE_CONFIGURATION - Feature Collection 
    case HID_USAGE_DISPLAY_MANUFACTURER_ID:         retval = "HID_USAGE_DISPLAY_MANUFACTURER_ID"; break;
    case HID_USAGE_DISPLAY_PRODUCT_ID:              retval = "HID_USAGE_DISPLAY_PRODUCT_ID"; break;
    case HID_USAGE_DISPLAY_SERIAL_NUMBER:           retval = "HID_USAGE_DISPLAY_SERIAL_NUMBER"; break;
    case HID_USAGE_DISPLAY_MANUFACTURER_DATE:       retval = "HID_USAGE_DISPLAY_MANUFACTURER_DATE"; break;
    case HID_USAGE_CALIBRATED_SCREEN_WIDTH:         retval = "HID_USAGE_CALIBRATED_SCREEN_WIDTH"; break;
    case HID_USAGE_CALIBRATED_SCREEN_HEIGHT:        retval = "HID_USAGE_CALIBRATED_SCREEN_HEIGHT"; break;

        // HID_USAGE_TRACKER_STATUS - Feature Collection 
    case HID_USAGE_DEVICE_STATUS:                   retval = "HID_USAGE_DEVICE_STATUS"; break;
    case HID_USAGE_CONFIGURATION_STATUS:            retval = "HID_USAGE_CONFIGURATION_STATUS"; break;
    case HID_USAGE_SAMPLING_FREQUENCY:              retval = "HID_USAGE_SAMPLING_FREQUENCY"; break;

        // HID_USAGE_TRACKER_CONTROL - Feature Collection 
    case HID_USAGE_MODE_REQUEST:                    retval = "HID_USAGE_MODE_REQUEST"; break;
    default:                                        retval = "ERROR: Unknown Usage"; break;
    }

    return retval;
}

CHAR*
GetReportTypeString
(
    HIDP_REPORT_TYPE reportType
)
{
    CHAR* retval = NULL;

    switch (reportType)
    {
    case HidP_Input:    retval = "HidP_Input"; break;
    case HidP_Output:   retval = "HidP_Output"; break;
    case HidP_Feature:  retval = "HidP_Feature"; break;
    default:            retval = "ERROR: Unknown ReportType"; break;
    }

    return retval;
}


CHAR*
GetUsagePageString
(
    USAGE usagePage
)
{
    CHAR* retval = NULL;

    switch (usagePage)
    {
    case HID_USAGE_PAGE_EYE_HEAD_TRACKER:   retval = "HID_USAGE_PAGE_EYE_HEAD_TRACKER"; break;
    default:                                retval = "ERROR: Unknown UsagePage"; break;
    }

    return retval;
}

CHAR*
GetStatusResultString
(
    NTSTATUS status
)
{
    CHAR* retval = NULL;

    switch (status)
    {
    case HIDP_STATUS_SUCCESS:                   retval = "HIDP_STATUS_SUCCESS"; break;
    case HIDP_STATUS_NULL:                      retval = "HIDP_STATUS_NULL"; break;
    case HIDP_STATUS_INVALID_PREPARSED_DATA:    retval = "HIDP_STATUS_INVALID_PREPARSED_DATA"; break;
    case HIDP_STATUS_INVALID_REPORT_TYPE:       retval = "HIDP_STATUS_INVALID_REPORT_TYPE"; break;
    case HIDP_STATUS_INVALID_REPORT_LENGTH:     retval = "HIDP_STATUS_INVALID_REPORT_LENGTH"; break;
    case HIDP_STATUS_USAGE_NOT_FOUND:           retval = "HIDP_STATUS_USAGE_NOT_FOUND"; break;
    case HIDP_STATUS_VALUE_OUT_OF_RANGE:        retval = "HIDP_STATUS_VALUE_OUT_OF_RANGE"; break;
    case HIDP_STATUS_BAD_LOG_PHY_VALUES:        retval = "HIDP_STATUS_BAD_LOG_PHY_VALUES"; break;
    case HIDP_STATUS_BUFFER_TOO_SMALL:          retval = "HIDP_STATUS_BUFFER_TOO_SMALL"; break;
    case HIDP_STATUS_INTERNAL_ERROR:            retval = "HIDP_STATUS_INTERNAL_ERROR"; break;
    case HIDP_STATUS_I8042_TRANS_UNKNOWN:       retval = "HIDP_STATUS_I8042_TRANS_UNKNOWN"; break;
    case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:    retval = "HIDP_STATUS_INCOMPATIBLE_REPORT_ID"; break;
    case HIDP_STATUS_NOT_VALUE_ARRAY:           retval = "HIDP_STATUS_NOT_VALUE_ARRAY"; break;
    case HIDP_STATUS_IS_VALUE_ARRAY:            retval = "HIDP_STATUS_IS_VALUE_ARRAY"; break;
    case HIDP_STATUS_DATA_INDEX_NOT_FOUND:      retval = "HIDP_STATUS_DATA_INDEX_NOT_FOUND"; break;
    case HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE:   retval = "HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE"; break;
    case HIDP_STATUS_BUTTON_NOT_PRESSED:        retval = "HIDP_STATUS_BUTTON_NOT_PRESSED"; break;
    case HIDP_STATUS_REPORT_DOES_NOT_EXIST:     retval = "HIDP_STATUS_REPORT_DOES_NOT_EXIST"; break;
    case HIDP_STATUS_NOT_IMPLEMENTED:           retval = "HIDP_STATUS_NOT_IMPLEMENTED"; break;
    default:                                    retval = "ERROR: Unknown status"; break;
    }

    return retval;
}

CHAR*
GetTrackerQualityString(
    uint8_t trackerQuality
)
{
    CHAR* retval = NULL;

    switch (trackerQuality)
    {
    case 1:     retval = "Foveated Rendering"; break;
    case 2:     retval = "Interaction Gaze"; break;
    case 3:     retval = "Medium Gaze"; break;
    case 4:     retval = "Rough Gaze"; break;
    default:    retval = "Error: Invalid Quality Level!";
    }

    return retval;
}

CHAR*
GetCoordinateSystemString(
    uint8_t coordinateSystem
)
{
    CHAR* retval = NULL;

    switch (coordinateSystem)
    {
    case 1: printf("Upper Left Origin"); break;
    case 2: printf("Lower Left Origin"); break;
    case 3: printf("Center Origin"); break;
    case 4: printf("Geometrical Center of Eye Tracker"); break;
    default: printf("Error: Invalid Coordinate System!");
    }

    return retval;
}

CHAR*
GetDeviceStatusString(
    uint8_t deviceStatus
)
{
    CHAR* retval = NULL;

    switch (deviceStatus)
    {
    case 0:     retval = "Eye tracking is disabled"; break;
    case 1:     retval = "Eye tracking is enabled"; break;
    default:    retval = "Error: Invalid device status!";
    }

    return retval;
}

CHAR*
GetConfigurationStatusString(
    uint8_t configurationStatus
)
{
    CHAR* retval = NULL;

    switch (configurationStatus)
    {
    case 0:     retval = "Screen Setup Needed"; break;
    case 1:     retval = "User Calibration Needed"; break;
    case 2:     retval = "Device Ready"; break;
    default:    retval = "Error: Invalid configuration status!";
    }

    return retval;
}
int8_t
CodeToExponent(
    BYTE b
)
{
    int8_t retval = 0;

    switch (b)
    {
    case 0x1: retval =  1; break;
    case 0x2: retval =  2; break;
    case 0x3: retval =  3; break;
    case 0x4: retval =  4; break;
    case 0x5: retval =  5; break;
    case 0x6: retval =  6; break;
    case 0x7: retval =  7; break;
    case 0x8: retval = -8; break;
    case 0x9: retval = -7; break;
    case 0xA: retval = -6; break;
    case 0xB: retval = -5; break;
    case 0xC: retval = -4; break;
    case 0xD: retval = -3; break;
    case 0xE: retval = -2; break;
    case 0xF: retval = -1; break;
    }

    return retval;
}

CHAR*
IsLogicalMinMaxSigned(
    LONG logicalMin,
    LONG logicalMax
)
{
    if (logicalMin >= 0 && logicalMax >= 0)
    {
        return "Unsigned";
    }
    else
    {
        return "Signed";
    }
}

CHAR*
UnitsToString(
    ULONG units
)
{
    CHAR* retval = NULL;

    switch (units)
    {
    case 0x0000:    retval = ""; break;
    case 0x0011:    retval = "centimeters"; break;
    case 0x1001:    retval = "seconds"; break;
    default:        retval = "Unknown Units"; break;
    }

    return retval;
}

VOID
GetDesktopResolution(
    PLONG plHorizontal, 
    PLONG plVertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    *plHorizontal = desktop.right;
    *plVertical = desktop.bottom;
}
