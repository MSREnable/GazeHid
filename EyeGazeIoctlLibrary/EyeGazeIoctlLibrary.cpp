// EyeGazeIoctlLibrary.cpp : Defines the exported functions for the DLL.
#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier

#include <cfgmgr32.h>
#include <strsafe.h>
#include <initguid.h>
#include <stdlib.h>
#include "devioctl.h"
#include <SetupAPI.h>

#include "EyeGazeIoctlLibrary.h"
#include "../EyeGazeIoctlTestApp/EyeGazeIoctl_Public.h"
DEFINE_GUID(GUID_CLASS_MONITOR, 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

#define MAX_DEVPATH_LENGTH 256

HANDLE              g_deviceHandle;
CONFIGURATION_DATA  g_ConfigurationData;
CAPABILITIES_DATA   g_CapabilitiesData;
RECT                g_desktopRect;
FLOAT               g_xMonitorRatio;
FLOAT               g_yMonitorRatio;

_Success_(return)
BOOL
GetDevicePath(
    _In_  LPGUID InterfaceGuid,
    _Out_writes_z_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        goto Error;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        goto Error;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        bRet = FALSE;
        goto Error;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        goto Error;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        goto Error;
    }

Error:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}

_Check_return_
_Ret_notnull_
_Success_(return != INVALID_HANDLE_VALUE)
HANDLE
OpenDevice(
    _In_ BOOL Synchronous
)

/*++
Routine Description:

    Called by main() to open an instance of our device after obtaining its name

Arguments:

    Synchronous - TRUE, if Device is to be opened for synchronous access.
                  FALSE, otherwise.

Return Value:

    Device handle on success else INVALID_HANDLE_VALUE

--*/

{
    HANDLE hDev;
    WCHAR completeDeviceName[MAX_DEVPATH_LENGTH];

    if (!GetDevicePath(
        (LPGUID)&VirtualEyeGaze_GUID,
        completeDeviceName,
        sizeof(completeDeviceName) / sizeof(completeDeviceName[0])))
    {
        return  INVALID_HANDLE_VALUE;
    }

    if (Synchronous) {
        hDev = CreateFile(completeDeviceName,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL, // default security
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else {

        hDev = CreateFile(completeDeviceName,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL, // default security
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);
    }

    if (hDev == INVALID_HANDLE_VALUE) {
        printf("Failed to open the device, error - %d", GetLastError());
    }
    else {
        printf("Opened the device successfully.\n");
    }

    return hDev;
}

BOOL
FetchEDIDInfo()
{
    ZeroMemory(&g_ConfigurationData, sizeof(CONFIGURATION_DATA));

    HDEVINFO devInfo = SetupDiGetClassDevsEx(&GUID_CLASS_MONITOR, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE, NULL, NULL, NULL);

    if (NULL == devInfo)
    {
        return FALSE;
    }

    for (ULONG i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i)
    {
        SP_DEVINFO_DATA devInfoData;

        memset(&devInfoData, 0, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);

        if (!SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
        {
            return FALSE;
        }
        TCHAR Instance[MAX_DEVICE_ID_LEN + 60]; // SetupDiGetDeviceInstanceId claims to return 260 chars, not 200 as specified by MAX_DEVICE_ID_LEN 
        if (!SetupDiGetDeviceInstanceId(devInfo, &devInfoData, Instance, MAX_PATH, NULL))
        {
            return FALSE;
        }

        HKEY hEDIDRegKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

        if (!hEDIDRegKey || (hEDIDRegKey == INVALID_HANDLE_VALUE))
        {
            continue;
        }

        BYTE EDIDdata[1024];
        DWORD edidsize = sizeof(EDIDdata);

        if (ERROR_SUCCESS != RegQueryValueEx(hEDIDRegKey, L"EDID", NULL, NULL, EDIDdata, &edidsize))
        {
            RegCloseKey(hEDIDRegKey);
            continue;
        }

        // this only handles the case of the primary monitor
        g_ConfigurationData.DisplayManufacturerId = (EDIDdata[0x08] << 8) + EDIDdata[0x09];
        g_ConfigurationData.DisplayProductId = (EDIDdata[0x0A] << 8) + EDIDdata[0x0B];
        g_ConfigurationData.DisplaySerialNumber = (EDIDdata[0x0C] << 24) + (EDIDdata[0x0D] << 16) + (EDIDdata[0x0E] << 8) + EDIDdata[0x0F];
        g_ConfigurationData.DisplayManufacturerDate = (EDIDdata[0x11] << 8) + EDIDdata[0x10];
        g_ConfigurationData.CalibratedScreenWidth = (((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66]) * 1000; // width in micrometers
        g_ConfigurationData.CalibratedScreenHeight = (((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67]) * 1000; // height in micrometers

        RegCloseKey(hEDIDRegKey);

        break;
    }
    SetupDiDestroyDeviceInfoList(devInfo);

    HWND desktopHwnd = GetDesktopWindow();
    GetWindowRect(desktopHwnd, &g_desktopRect);

    g_xMonitorRatio = (FLOAT)g_ConfigurationData.CalibratedScreenWidth / (FLOAT)g_desktopRect.right;
    g_yMonitorRatio = (FLOAT)g_ConfigurationData.CalibratedScreenHeight / (FLOAT)g_desktopRect.bottom;

    return TRUE;
}

BOOL
InitializeCapabilitiesData(
    _In_ HANDLE deviceHandle
)
{
    DWORD code;
    ULONG index;
    BOOL success = TRUE;

    ZeroMemory(&g_CapabilitiesData, sizeof(CAPABILITIES_DATA));
    g_CapabilitiesData.TrackerQuality = TRACKER_QUALITY_FINE_GAZE;
    g_CapabilitiesData.MinimumTrackingDistance = 50000;
    g_CapabilitiesData.OptimumTrackingDistance = 65000;
    g_CapabilitiesData.MaximumTrackingDistance = 90000;

    if (!DeviceIoControl(deviceHandle,
        IOCTL_EYEGAZE_CAPABILITIES_REPORT,
        &g_CapabilitiesData,
        sizeof(CAPABILITIES_DATA),
        NULL,
        0,
        &index,
        0))
    {
        code = GetLastError();

        printf("DeviceIoControl failed with error 0x%x\n", code);

        success = FALSE;
    }

    return success;
}

BOOL
InitializeConfigurationData(
    _In_ HANDLE deviceHandle
)
{
    DWORD code;
    ULONG index;
    BOOL success = TRUE;

    if (!DeviceIoControl(deviceHandle,
        IOCTL_EYEGAZE_CONFIGURATION_REPORT,
        &g_ConfigurationData,
        sizeof(CONFIGURATION_DATA),
        NULL,
        0,
        &index,
        0))
    {
        code = GetLastError();

        printf("DeviceIoControl failed with error 0x%x\n", code);

        success = FALSE;
    }

    return success;
}

bool
InitializeEyeGaze()
{
    bool success = true;

    g_deviceHandle = OpenDevice(FALSE);

    if (g_deviceHandle == INVALID_HANDLE_VALUE) {

        printf("Unable to find any EyeGazeIoctl device!\n");

        success = false;
        goto Error;
    }

    FetchEDIDInfo();

    if (!InitializeCapabilitiesData(g_deviceHandle))
    {
        success = false;
        goto Error;
    }

    if (!InitializeConfigurationData(g_deviceHandle))
    {
        success = false;
        goto Error;
    }

Error:
    return success;
}

unsigned long long
GetTimestamp()
{
    LARGE_INTEGER qpcTimestamp;
    LARGE_INTEGER Frequency;

    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&qpcTimestamp);

    qpcTimestamp.QuadPart *= 1000000;
    qpcTimestamp.QuadPart /= Frequency.QuadPart;

    return qpcTimestamp.QuadPart;
}

/// <summary>
/// Sends a Gaze Input Report with the specified X/Y location
/// </summary>
/// <param name="X">Horizontal distance from the upper left corner of the primary display in micrometers.</param>
/// <param name="Y">Vertical distance from the upper left corner of the primary display in micrometers.</param>
/// <param name="timestamp">Optional 64-bit timestamp in microseconds. If set to 0 then the timestamp is autogenerated.</param>
/// <returns>True if the Gaze Input Report IOCTL was successful, false otherwise.</returns>
bool
SendGazeReportUm(
    long X,
    long Y,
    unsigned long long timestamp)
{
    DWORD code;
    ULONG index;
    bool success = true;

    GAZE_DATA gaze_data = { 0 };

    if (timestamp == 0)
    {
        timestamp = GetTimestamp();
    }

    gaze_data.GazePoint.X = X;
    gaze_data.GazePoint.Y = Y;
    gaze_data.TimeStamp = timestamp;

    if (!DeviceIoControl(g_deviceHandle,
        IOCTL_EYEGAZE_GAZE_DATA,
        &gaze_data,
        sizeof(GAZE_DATA),
        NULL,
        0,
        &index,
        0))
    {
        code = GetLastError();

        printf("DeviceIoControl failed with error 0x%x\n", code);

        success = false;
    }

    return success;
}

/// <summary>
/// Sends a Gaze Input Report with the specified X/Y location
/// </summary>
/// <param name="X">Horizontal distance from the upper left corner of the primary display in pixels.</param>
/// <param name="Y">Vertical distance from the upper left corner of the primary display in pixels.</param>
/// <param name="timestamp">Optional 64-bit timestamp in microseconds. If set to 0 then the timestamp is autogenerated.</param>
/// <returns>True if the Gaze Input Report IOCTL was successful, false otherwise.</returns>
bool
SendGazeReportPixel(
    long X,
    long Y,
    unsigned long long timestamp)
{
    long xInUm = (long)(X * g_xMonitorRatio);
    long yInUm = (long)(Y * g_yMonitorRatio);

    return SendGazeReportUm(xInUm, yInUm, timestamp);
}

int
GetPrimaryMonitorWidthUm()
{
    if (g_ConfigurationData.CalibratedScreenWidth == 0)
        FetchEDIDInfo();

    return g_ConfigurationData.CalibratedScreenWidth;
}

int
GetPrimaryMonitorHeightUm()
{
    if (g_ConfigurationData.CalibratedScreenHeight == 0)
        FetchEDIDInfo();

    return g_ConfigurationData.CalibratedScreenHeight;
}