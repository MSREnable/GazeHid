#include "driver.h"

#ifndef __cplusplus
typedef unsigned char bool;
static const bool False = 0;
static const bool True = 1;
#endif

#include "QuickLink2.h"
#include <strsafe.h>

typedef struct _SENSOR_CONTEXT
{
    QLDeviceId      DeviceId;
    BOOL            DeviceStarted;
    HANDLE          ThreadHandle;
    DWORD           ThreadId;
    volatile BOOL   ExitThread;
} SENSOR_CONTEXT, *PSENSOR_CONTEXT;

QLDeviceId QL2Initialize(const char* path)
{
    QLError qlerror = QL_ERROR_OK;
    QLDeviceId deviceId = 0;
    QLDeviceInfo deviceInfo;
    QLSettingsId settingsId;

    // Enumerate the bus to find out which eye trackers are connected to the
    // computer. 
    int numDevices = 1;
    qlerror = QLDevice_Enumerate(&numDevices, &deviceId);

    // If the enumeration failed then return 0
    if (qlerror != QL_ERROR_OK)
    {
        KdPrint(("QLDevice_Enumerate() failed with error code %d\n", qlerror));
        return 0;
    }

    // If no devices were found then return 0;
    if (numDevices != 1)
    {
        KdPrint(("Invalid number of devices(%d) found\n", numDevices));
        return 0;
    }

    // Create a blank settings container. QLSettings_Load() can create a
    // settings container but it won't if the file fails to load. By calling
    // QLSettings_Create() we ensure that a container is created regardless.
    qlerror = QLSettings_Create(0, &settingsId);

    // Load the file with the stored password.
    qlerror = QLSettings_Load(path, &settingsId);

    // Get the device info so we can access the serial number.
    QLDevice_GetInfo(deviceId, &deviceInfo);

    // Create an application defined setting name using the serial number. The
    // settings containers can be used to hold settings other than the
    // QuickLink2 defined setting. Using it to store the password for future
    // use as we are doing here is a good example. 
    char serialNumberName[256];
    StringCchCopyA(serialNumberName, 256, "SN_");
    StringCchCatA(serialNumberName, 256, deviceInfo.serialNumber);

    // Create a buffer for getting the stored password.
    char password[256];
    password[0] = 0;

    // Check for the password in the settings file.
    int stringSize = 256;
    QLSettings_GetValue(
        settingsId,
        serialNumberName,
        QL_SETTING_TYPE_STRING,
        stringSize,
        password);

    // Try setting the password for the device.
    qlerror = QLDevice_SetPassword(deviceId, password);
    if (qlerror != QL_ERROR_OK)
    {
        KdPrint(("QLDevice_SetPassword returned %d\n", qlerror));
        return 0;
    }

    // The application defined password setting that is stored in the settings 
    // container will have no effect if imported to the device, but there may 
    // have been other settings in the file that was loaded which are Quick 
    // Link 2 settings. Try and import them to the device.
    QLDevice_ImportSettings(deviceId, settingsId);

    QLAPI_ImportSettings(settingsId);

    return deviceId;
};

DWORD WINAPI EyeTechFrameProc(PVOID startParam)
{
    QLDeviceId deviceId;
    QLCalibrationId calibrationId;

    char settingsFile[MAX_PATH];
    HRESULT hr;

    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)startParam;
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;

    if (GetTempPathA(MAX_PATH, settingsFile) == 0)
    {
        return FALSE;
    }

    hr = StringCchCatA(settingsFile, MAX_PATH, "settings.txt");
    if (FAILED(hr))
    {
        return FALSE;
    }

    deviceId = QL2Initialize(settingsFile);
    if (deviceId == 0)
    {
        return FALSE;
    }

    sensorContext->DeviceId = deviceId;
    sensorContext->DeviceStarted = FALSE;
    sensorContext->ExitThread = FALSE;

    if (QLDevice_Start(deviceId) != QL_ERROR_OK)
    {
        KdPrint(("Device not started successfully!\n"));
        return FALSE;
    }
    sensorContext->DeviceStarted = TRUE;

    if (QLDevice_ApplyLastCalibration(deviceId) != QL_ERROR_OK)
    {
        KdPrint(("QLDevice_ApplyLastCalibration failed\n"));
        return FALSE;
    }

    if (QLCalibration_Load("c:\\ProgramData\\EyeTechDS\\Cache\\AEye 1.2.3.3.cal", &calibrationId) != QL_ERROR_OK)
    {
        KdPrint(("QLCalibration_Load failed\n"));
        return FALSE;
    }
    
    if (QLDevice_ApplyCalibration(deviceId, calibrationId) != QL_ERROR_OK)
    {
        KdPrint(("QLDevice_ApplyCalibration failed\n"));
        return FALSE;
    }

    while (TRUE)
    {
        QLFrameData frameData = { 0 };
        GAZE_REPORT gazeReport = { 0 };

        QLDevice_GetFrame(deviceId, 10000, &frameData);
        if (frameData.WeightedGazePoint.Valid)
        {
            gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
            // TODO: Check on timestamp data type
            gazeReport.TimeStamp = (uint64_t)frameData.ImageData.Timestamp;
            gazeReport.GazePoint.X = (int32_t)(frameData.WeightedGazePoint.x / 100 * deviceContext->ConfigurationReport.CalibratedScreenWidth);
            gazeReport.GazePoint.Y = (int32_t)(frameData.WeightedGazePoint.y / 100 * deviceContext->ConfigurationReport.CalibratedScreenHeight);
            KdPrint(("GazePoint = [%1.5d, %1.5d]\n", gazeReport.GazePoint.X, gazeReport.GazePoint.Y));
            SendGazeReport(deviceContext, &gazeReport);
        }

        Sleep(15);

        // TODO: Graceful thread exit and cleanup
    }
    return 0;
}

BOOL InitializeEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)LocalAlloc(0, (sizeof(*sensorContext)));
    if (!sensorContext)
    {
        return FALSE;
    }

    deviceContext->EyeTracker = sensorContext;

    sensorContext->ThreadHandle = CreateThread(NULL, 0, EyeTechFrameProc, deviceContext, CREATE_SUSPENDED, &sensorContext->ThreadId);
    if (!sensorContext->ThreadHandle)
    {
        KdPrint(("CreateThread failed with error=0x%08x\n", GetLastError()));
        return FALSE;
    }

    ResumeThread(sensorContext->ThreadHandle);

    return TRUE;
}

void ShutdownEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;
    if (!sensorContext)
    {
        return;
    }
    sensorContext->ExitThread = TRUE;
    if (sensorContext->ThreadHandle)
    {
        WaitForSingleObject(sensorContext->ThreadHandle, INFINITE);
        CloseHandle(sensorContext->ThreadHandle);
    }
    QLDevice_Stop_All();
    LocalFree(sensorContext);
    deviceContext->EyeTracker = NULL;
}