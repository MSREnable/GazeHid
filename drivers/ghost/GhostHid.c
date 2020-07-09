#include "driver.h"
#include <time.h>

#ifndef __cplusplus
typedef unsigned char bool;
static const bool False = 0;
static const bool True = 1;
#endif

#include <strsafe.h>

typedef struct _SENSOR_CONTEXT
{
    BOOL            DeviceStarted;
    HANDLE          ThreadHandle;
    DWORD           ThreadId;
    volatile BOOL   ExitThread;
} SENSOR_CONTEXT, *PSENSOR_CONTEXT;

DWORD WINAPI GhostHidFrameProc(PVOID startParam)
{
    HRESULT hr = S_OK;

    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)startParam;
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;

    sensorContext->DeviceStarted = FALSE;
    sensorContext->ExitThread = FALSE;

	// do stuff to start the device before setting device started
    sensorContext->DeviceStarted = TRUE;
	
	// after device started, do whatever post start stuff here
	// such as load calibration
    int32_t x = 0;
    int32_t y = 0;
    while (TRUE)
    {
        GAZE_REPORT gazeReport = { 0 };

        gazeReport.ReportId = HID_USAGE_TRACKING_DATA;

        __time64_t ltime;
        _time64(&ltime);

        gazeReport.TimeStamp = (uint64_t)ltime;
        gazeReport.GazePoint.X = (int32_t)x;
        gazeReport.GazePoint.Y = (int32_t)y;
        KdPrint(("GazePoint = [%1.5f, %1.5f]\n", gazeReport.GazePoint.X, gazeReport.GazePoint.Y));
        SendGazeReport(deviceContext, &gazeReport);

        if (x > 600)
        {
            x = 0;
            if (y > 600)
            {
                y = 0;
            }
            else
            {
                ++y;
            }
        }
        else
        {
            ++x;
        }

        Sleep(15);

        // TODO: Graceful thread exit and cleanup
    }
    return hr;
}

BOOL InitializeEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)LocalAlloc(0, (sizeof(*sensorContext)));
    if (!sensorContext)
    {
        return FALSE;
    }

    deviceContext->EyeTracker = sensorContext;

    sensorContext->ThreadHandle = CreateThread(NULL, 0, GhostHidFrameProc, deviceContext, CREATE_SUSPENDED, &sensorContext->ThreadId);
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
    LocalFree(sensorContext);
    deviceContext->EyeTracker = NULL;
}