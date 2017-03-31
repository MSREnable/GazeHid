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
#include "common.h"
#include <time.h>
#include "Tracker.h"

//
// These are the default device attributes set in the driver
// which are used to identify the device.
//
#define HIDMINI_DEFAULT_PID              0xFEED
#define HIDMINI_DEFAULT_VID              0xDEED

//
// These are the device attributes returned by the mini driver in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//
#define HIDMINI_TEST_PID              0xDEEF
#define HIDMINI_TEST_VID              0xFEED
#define HIDMINI_TEST_VERSION          0x0505

USAGE g_MyUsagePage = 0x11;
USAGE g_MyUsage = 0x01;

//
// Function prototypes
//
BOOLEAN
GetFeature(
    HANDLE file
    );

BOOLEAN
CheckIfOurDevice(
    HANDLE file
    );

BOOLEAN
ReadInputData(
    _In_ HANDLE file
    );

BOOLEAN
GetIndexedString(
    HANDLE File
    );

BOOLEAN
GetStrings(
    HANDLE File
    );

BOOLEAN
FindMatchingDevice(
    _In_ LPGUID   Guid,
    _Out_ HANDLE* Handle
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
    GUID hidguid;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOLEAN found = FALSE;
    BOOLEAN bSuccess = FALSE;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);


    srand( (unsigned)time( NULL ) );

    HidD_GetHidGuid(&hidguid);

    found = FindMatchingDevice(&hidguid, &file);
    if (found) {
        printf("...sending control request to our device\n");

        //
        // Get/Set feature loopback
        //
        bSuccess = GetFeature(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        //
        // Get Strings
        //
        bSuccess = GetIndexedString(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

        bSuccess = GetStrings(file);
        if (bSuccess == FALSE) {
            goto cleanup;
        }

		////
		//// Read/Write report loopback
		////
		bSuccess = ReadInputData(file);
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

    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }

    return (bSuccess ? 0 : 1);
}

BOOLEAN
FindMatchingDevice(
    _In_  LPGUID  InterfaceGuid,
    _Out_ HANDLE* Handle
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR currentInterface;
    BOOLEAN bRet = FALSE;
    HANDLE devHandle = INVALID_HANDLE_VALUE;

    if (NULL == Handle) {
        printf("Error: Invalid device handle parameter\n");
        return FALSE;
    }

    *Handle = INVALID_HANDLE_VALUE;

    cr = CM_Get_Device_Interface_List_Size(
            &deviceInterfaceListLength,
            InterfaceGuid,
            NULL,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
            InterfaceGuid,
            NULL,
            deviceInterfaceList,
            deviceInterfaceListLength,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    //
    // Enumerate devices of this interface class
    //
    printf("\n....looking for our HID device (with UP=0x%04X "
        "and Usage=0x%02X)\n", g_MyUsagePage, g_MyUsage);

    for (currentInterface = deviceInterfaceList;
        *currentInterface;
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
            bRet = TRUE;
            *Handle = devHandle;
        }
        else {
            CloseHandle(devHandle);
        }
    }

clean0:
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
    HANDLE file)
{
    PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
    HIDP_CAPS                       Caps; // The Capabilities of this hid device.
    HIDD_ATTRIBUTES attr; // Device attributes

    if (!HidD_GetAttributes(file, &attr))
    {
        printf("Error: HidD_GetAttributes failed \n");
        return FALSE;
    }

    printf("Device Attributes - PID: 0x%x, VID: 0x%x \n", attr.ProductID, attr.VendorID);
    if ((attr.VendorID != HIDMINI_DEFAULT_VID) || (attr.ProductID != HIDMINI_DEFAULT_PID))
    {
        printf("Device attributes doesn't match the sample \n");
        return FALSE;
    }

    if (!HidD_GetPreparsedData (file, &Ppd))
    {
        printf("Error: HidD_GetPreparsedData failed \n");
        return FALSE;
    }

    if (!HidP_GetCaps (Ppd, &Caps))
    {
        printf("Error: HidP_GetCaps failed \n");
        HidD_FreePreparsedData (Ppd);
        return FALSE;
    }

    if ((Caps.UsagePage == g_MyUsagePage) && (Caps.Usage == g_MyUsage)){
        printf("Success: Found my device.. \n");
        return TRUE;

    }

    return FALSE;

}

BOOLEAN
GetFeature(
    HANDLE file
    )
{
	BOOLEAN bSuccess;
	CAPABILITIES_REPORT capabilities = { 0 };
	capabilities.ReportId = HID_USAGE_CAPABILITIES;
	bSuccess = HidD_GetFeature(file, &capabilities, sizeof(capabilities));
	if (bSuccess)
	{
		printf("Max frames per second: %d\n", capabilities.MaxFramesPerSecond);
		printf("Max Screen Plane Width: %d\n", capabilities.MaximumScreenPlaneWidth);
		printf("Max Screen Plane Height: %d\n", capabilities.MaximumScreenPlaneHeight);
	}
    else
    {
        printf("HidD_GetFeature failed. GLE=0x%08x\n", GetLastError());
    }
    return bSuccess;
}



BOOLEAN
ReadInputData(
    _In_ HANDLE file
    )
{
    GAZE_REPORT report = { 0 };
    BOOL bSuccess = TRUE;
    DWORD bytesRead;
	PHIDP_PREPARSED_DATA preparsedData;

	bSuccess = HidD_GetPreparsedData(file, &preparsedData);
    //
    // Allocate memory
    //
    report.ReportId = HID_USAGE_TRACKING_DATA;

    //
    // get input data.
    //
    while ((bSuccess) && (!_kbhit()))
    {
        bSuccess = ReadFile(
            file,        // HANDLE hFile,
            &report,      // LPVOID lpBuffer,
            sizeof(report),  // DWORD nNumberOfBytesToRead,
            &bytesRead,  // LPDWORD lpNumberOfBytesRead,
            NULL         // LPOVERLAPPED lpOverlapped
        );

        if (!bSuccess)
        {
            printf("failed ReadFile \n");
        }
        else
        {
            printf("[0x%lld - [%f, %f]\n", report.TimeStamp, report.GazePoint.X, report.GazePoint.Y);
        }
    }

    return (BOOLEAN) bSuccess;
}


BOOLEAN
GetIndexedString(
    HANDLE File
    )
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = malloc(bufferLength);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);

    bSuccess = HidD_GetIndexedString(
        File,
        VHIDMINI_DEVICE_STRING_INDEX,  // IN ULONG  StringIndex,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        ) ;

    if (!bSuccess)
    {
        printf("failed WriteFile \n");
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
    HANDLE File
    )
{
    BOOLEAN bSuccess;
    BYTE* buffer;
    ULONG bufferLength;

    bufferLength = MAXIMUM_STRING_LENGTH;
    buffer = malloc(bufferLength);
    if (!buffer )
    {
        printf("malloc failed\n");
        return FALSE;
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetProductString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetProductString \n");
        goto exit;
    }
    else
    {
        printf("Product string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetSerialNumberString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetSerialNumberString \n");
        goto exit;
    }
    else
    {
        printf("Serial number string: %S\n", (PWSTR) buffer);
    }

    ZeroMemory(buffer, bufferLength);
    bSuccess = HidD_GetManufacturerString(
        File,
        (PVOID) buffer,  //OUT PVOID  Buffer,
        bufferLength // IN ULONG  BufferLength
        );

    if (!bSuccess)
    {
        printf("failed HidD_GetManufacturerString \n");
        goto exit;
    }
    else
    {
        printf("Manufacturer string: %S\n", (PWSTR) buffer);
    }

exit:

    free(buffer);

    return bSuccess;
}


