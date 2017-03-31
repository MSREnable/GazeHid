// virtualhid.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "virtualhid.h"


BOOL FindEyeTracker(PWCHAR vid, PWCHAR pid, wchar_t *instanceName, int length)
{
    HDEVINFO                hDevInfo;
    SP_DEVINFO_DATA         devInfoData;
    DWORD					index, size;
    const int               hardwareIdLen = 1024;
    wchar_t                 hardwareId[hardwareIdLen];
    HRESULT                 hr;

    if (vid == NULL)
    {
        return FALSE;
    }

    hr = StringCchCopy(hardwareId, hardwareIdLen, vid);
    if (FAILED(hr))
    {
        return FALSE;
    }

    if (pid != NULL)
    {
        hr = StringCchCat(hardwareId, hardwareIdLen, L"&");
        if (FAILED(hr))
        {
            return FALSE;
        }
        hr = StringCchCat(hardwareId, hardwareIdLen, pid);
        if (FAILED(hr))
        {
            return FALSE;
        }
    }

    hDevInfo = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    index = 0;
    devInfoData.cbSize = sizeof(devInfoData);
    while (SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData))
    {
        SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, instanceName, length, &size);
        if (wcswcs(instanceName, hardwareId))
        {
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return TRUE;
        }
        index++;
    }


    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FALSE;
}


VOID WINAPI OnSoftwareDeviceCreate(
    _In_     HSWDEVICE hSwDevice,
    _In_     HRESULT   CreateResult,
    _In_opt_ PVOID     pContext,
    _In_opt_ PCWSTR    pszDeviceInstanceId
)
{
    wchar_t message[1024];

    if (SUCCEEDED(CreateResult))
    {
        StringCchPrintf(message, 1024, L"Created software device %s (0x%p)\n\n",
            pszDeviceInstanceId, hSwDevice);
    }
    else
    {
        StringCchPrintf(message, 1024, L"Failed to create software device %s\n\n", pszDeviceInstanceId);
    }
    OutputDebugString(message);
}

VIRTUALHID_API HSWDEVICE CreateVirtualHid(PWCHAR pid, PWCHAR vid)
{
    HRESULT hr;
    HSWDEVICE hDevice = NULL;
    SW_DEVICE_CREATE_INFO createInfo;
    wchar_t eyeTracker[MAX_PATH];

    if (!FindEyeTracker(pid, vid, eyeTracker, MAX_PATH))
    {
        wprintf(L"Failed to find Tobii Eye Tracker\n");
        return NULL;
    }

    createInfo.cbSize = sizeof(createInfo);
    createInfo.pszInstanceId = L"EyeTracker";
    createInfo.pszzHardwareIds = L"EYETRACKER_HID";
    createInfo.pszzCompatibleIds = NULL;
    createInfo.pContainerId = NULL;
    createInfo.CapabilityFlags = SWDeviceCapabilitiesRemovable;// | SWDeviceCapabilitiesDriverRequired;
    createInfo.pszDeviceDescription = L"HID Eye Tracker Device";
    createInfo.pszDeviceLocation = NULL;
    createInfo.pSecurityDescriptor = NULL;
    hr = SwDeviceCreate(L"EyeTracker", eyeTracker, &createInfo, 0, NULL, OnSoftwareDeviceCreate, NULL, &hDevice);
    if (SUCCEEDED(hr) && (hDevice != NULL))
    {
        return hDevice;
    }

    return NULL;
}

VIRTUALHID_API void CloseVirtualHid(HSWDEVICE hDevice)
{
    SwDeviceClose(hDevice);
}
