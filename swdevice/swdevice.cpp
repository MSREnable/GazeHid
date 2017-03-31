#include <windows.h>
#include <swdevice.h>
#include <swdevicedef.h>
#include <stdio.h>
#include <conio.h>
#include <SetupAPI.h>
#include <devpkey.h>
#include <initguid.h>
#include <strsafe.h>

#define CLEANUP_ON_FAIL(hr) if (FAILED(hr)) goto Cleanup;


BOOL FindEyeTracker(int argc, wchar_t *argv[], wchar_t *instanceName, int length)
{
    HDEVINFO                hDevInfo;
    SP_DEVINFO_DATA         devInfoData;
    DWORD					index, size;
    const int               hardwareIdLen = 1024;
    wchar_t                 hardwareId[hardwareIdLen];
    HRESULT                 hr;

    if (argc < 2)
    {
        return FALSE;
    }

    if (argc >= 2)
    {
        hr = StringCchCopy(hardwareId, hardwareIdLen, argv[1]);
        if (FAILED(hr))
        {
            return FALSE;
        }
    }
    if (argc >= 3)
    {
        hr = StringCchCat(hardwareId, hardwareIdLen, L"&");
        if (FAILED(hr))
        {
            return FALSE;
        }
        hr = StringCchCat(hardwareId, hardwareIdLen, argv[2]);
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
	if (SUCCEEDED(CreateResult))
	{
		wprintf(L"Created software device %s (0x%p)\n\n",
			pszDeviceInstanceId, hSwDevice);
	}
	else
	{
		wprintf(L"Failed to create software device %s\n\n", pszDeviceInstanceId);
	}
}

int wmain(int argc, wchar_t* argv[])
{
	HRESULT hr;
	HSWDEVICE hDevice = NULL;
	SW_DEVICE_CREATE_INFO createInfo;
	wchar_t eyeTracker[MAX_PATH];

    if (!FindEyeTracker(argc, argv, eyeTracker, MAX_PATH))
    {
        wprintf(L"Failed to find eye Tracker\n");
        return -1;
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
	if (FAILED(hr))
	{
        wprintf(L"Failed to create software device for EyeTracker. hr=0x%08x\n", hr);
        return -1;
	}

	wprintf(L"Press any key to exit...\n");
	while (!_kbhit())
	{
		Sleep(1000);
	}
	
    SwDeviceClose(hDevice);
    hDevice = NULL;
}