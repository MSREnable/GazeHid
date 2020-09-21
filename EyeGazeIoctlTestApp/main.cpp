#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "devioctl.h"
#include "strsafe.h"

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <cfgmgr32.h>
#include <basetyps.h>
#include "usbdi.h"

#include <time.h>
#include <math.h>
#include <conio.h> 

#include <SetupAPI.h>
#include <initguid.h>
#include <cfgmgr32.h>
DEFINE_GUID(GUID_CLASS_MONITOR, 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);


#include "EyeGazeIoctl_Public.h"

#define MAX_DEVPATH_LENGTH 256

CONFIGURATION_DATA g_ConfigurationData;
CAPABILITIES_DATA g_CapabilitiesData;

INT32 GetMonitorWidth()
{
    return g_ConfigurationData.CalibratedScreenWidth;
}

INT32 GetMonitorHeight()
{
    return g_ConfigurationData.CalibratedScreenHeight;
}

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
        bRet = FALSE;
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

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
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

    printf("DeviceName = (%S)\n", completeDeviceName);

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

void
InitializeCapabilitiesData(
    _In_ PCAPABILITIES_DATA pCapabilitiesData
)
{
    ZeroMemory(pCapabilitiesData, sizeof(CAPABILITIES_DATA));
    pCapabilitiesData->TrackerQuality = TRACKER_QUALITY_FINE_GAZE;
    pCapabilitiesData->MinimumTrackingDistance = 50000;
    pCapabilitiesData->OptimumTrackingDistance = 65000;
    pCapabilitiesData->MaximumTrackingDistance = 90000;
}

void
InitializeConfigurationData(
    _In_ PCONFIGURATION_DATA pConfigurationData
)
{
    ZeroMemory(pConfigurationData, sizeof(CONFIGURATION_DATA));

    HDEVINFO devInfo = SetupDiGetClassDevsEx(&GUID_CLASS_MONITOR, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE, NULL, NULL, NULL);

    if (NULL == devInfo)
    {
        return;
    }

    for (ULONG i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i)
    {
        SP_DEVINFO_DATA devInfoData;

        memset(&devInfoData, 0, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);

        if (!SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
        {
            return;
        }
        TCHAR Instance[MAX_DEVICE_ID_LEN + 60]; // SetupDiGetDeviceInstanceId claims to return 260 chars, not 200 as specified by MAX_DEVICE_ID_LEN 
        if (!SetupDiGetDeviceInstanceId(devInfo, &devInfoData, Instance, MAX_PATH, NULL))
        {
            return;
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
        pConfigurationData->DisplayManufacturerId = (EDIDdata[0x08] << 8) + EDIDdata[0x09];
        pConfigurationData->DisplayProductId = (EDIDdata[0x0A] << 8) + EDIDdata[0x0B];
        pConfigurationData->DisplaySerialNumber = (EDIDdata[0x0C] << 24) + (EDIDdata[0x0D] << 16) + (EDIDdata[0x0E] << 8) + EDIDdata[0x0F];
        pConfigurationData->DisplayManufacturerDate = (EDIDdata[0x11] << 8) + EDIDdata[0x10];
        pConfigurationData->CalibratedScreenWidth = (((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66]) * 1000; // width in micrometers
        pConfigurationData->CalibratedScreenHeight = (((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67]) * 1000; // height in micrometers

        RegCloseKey(hEDIDRegKey);

        break;
    }
    SetupDiDestroyDeviceInfoList(devInfo);
}

int
_cdecl
main(
    //_In_ int argc,
    //_In_reads_(argc) LPSTR* argv
)
/*++
Routine Description:

    Entry point to rwbulk.exe
    Parses cmdline, performs user-requested tests

Arguments:

    argc, argv  standard console  'c' app arguments

Return Value:

    Zero

--*/

{
    int    retValue = 0;

    HANDLE          deviceHandle;
    DWORD           code;
    ULONG           index;

    deviceHandle = OpenDevice(FALSE);

    if (deviceHandle == INVALID_HANDLE_VALUE) {

        printf("Unable to find any EyeGazeIoctl device!\n");

        return FALSE;

    }

    InitializeCapabilitiesData(&g_CapabilitiesData);
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

        goto Error;
    }

    InitializeConfigurationData(&g_ConfigurationData);
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

        goto Error;
    }

    INT32 x = 0;
    INT32 y = 0;
    INT32 monitorWidthUm = GetMonitorWidth();
    INT32 monitorHeightUm = GetMonitorHeight();
    INT32 velocity = 125; // step size in micrometers
    FLOAT PI = 3.14159f;
    FLOAT angle = 45.0f;
    INT32 noise = 1000;
    INT32 noise_step = 500;
    INT32 noiseX = 0;
    INT32 noiseY = 0;
    BOOL  mouseMode = FALSE;

    GAZE_DATA gaze_data;
    ZeroMemory(&gaze_data, sizeof(GAZE_DATA));

    POINT mousePoint;

    printf("\n\nPress + to increase noise by %dum\nPress - to decrease noise by %dum\n\n", noise_step, noise_step);
    printf("Press M to use mouse\nPress F to use fake data\n\nPress esc key to stop...\n\n");

    RECT desktopRect;
    HWND desktopHwnd = GetDesktopWindow();
    GetWindowRect(desktopHwnd, &desktopRect);

    printf("Monitor Width (%10d)um Height (%10d)um\n", monitorWidthUm, monitorHeightUm);
    printf("Monitor Width (%10d)px Height (%10d)px\n\n", desktopRect.right, desktopRect.bottom);

    // screen size to pixel ratio
    FLOAT xMonitorRatio = (FLOAT)monitorWidthUm / (FLOAT)desktopRect.right;
    FLOAT yMonitorRatio = (FLOAT)monitorHeightUm / (FLOAT)desktopRect.bottom;

    while (TRUE)
    {
        gaze_data.GazePoint.X = x;
        gaze_data.GazePoint.Y = y;
        _time64((__time64_t*)(&gaze_data.TimeStamp));

        // Add noise
        if (noise > 0)
        {
            UINT rangeOffset = noise / 2;
            noiseX = (rand() % noise) - rangeOffset;
            noiseY = (rand() % noise) - rangeOffset;

            gaze_data.GazePoint.X += noiseX;
            gaze_data.GazePoint.Y += noiseY;
        }

        //if (IsTrackerEnabled())
        {
            // SendGazeReport
            if (!DeviceIoControl(deviceHandle,
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

                goto Error;
            }
            else
            {
                printf("Original (%8dum, %8dum) Noise (%8dum) Sent (%8d, %8d) Source: %11s\r",
                    x, y,
                    noise,
                    gaze_data.GazePoint.X, gaze_data.GazePoint.Y,
                    mouseMode == TRUE ? "Mouse Data" : "Fake Data"
                );
            }
        }

        Sleep(10);

        if (mouseMode == TRUE)
        {
            if (GetCursorPos(&mousePoint))
            {
                // mouse is returned in pixels, need to convert to um
                x = (INT32)(mousePoint.x * xMonitorRatio);
                y = (INT32)(mousePoint.y * yMonitorRatio);
            }
        }
        else
        {
            // Generate bouncing test pattern
            x += (INT32)(velocity * cos(angle * PI / 180));
            y += (INT32)(velocity * sin(angle * PI / 180));

            if (x < 0 || x > monitorWidthUm)
            {
                angle = 180 - angle;
            }
            else if (y < 0 || y > monitorHeightUm)
            {
                angle = 360 - angle;
            }
        }

        if (_kbhit())
        {
            int charval = _getch();
            //printf("\n_getch() returned %d\n", charval);

            if (charval == 27) // esc
            {
                break;
            }
            else if (charval == 'm' || charval == 'M')
            {
                mouseMode = TRUE;
            }
            else if (charval == 'f' || charval == 'F')
            {
                mouseMode = FALSE;
            }
            else if (charval == '+')
            {
                noise += noise_step;
            }
            else if (charval == '-')
            {
                noise -= noise_step;
                if (noise < 0)
                {
                    noise = 0;
                }
            }
        }
    }

Error:
    CloseHandle(deviceHandle);

    return retValue;
}