#include "driver.h"
#include "Tracker.h"


// Deviations from spec
// * Gaze point units in float instead of micrometers
// * Eye positions in a different callback than gaze point
// * The discovery protocol cannot be based on tcp urls. We shouldnt rely on opening the first found device either
// * What about implementing a head tracker now that stream sdk has support for it


#include "tobii/tobii.h"
#include "tobii/tobii_streams.h"

typedef struct _SENSOR_CONTEXT
{
    tobii_api_t*        TobiiApi;
    tobii_device_t*     TobiiDevice;
    BOOL                GazeStreamSubscribed;
    BOOL                DeviceStarted;
    HANDLE              ThreadHandle;
    DWORD               ThreadId;
    volatile BOOL       ExitThread;
} SENSOR_CONTEXT, *PSENSOR_CONTEXT;


#define CLEANUP_ON_FAIL(error) if (error != TOBII_ERROR_NO_ERROR) goto Cleanup;

void OnGazeEvent(tobii_gaze_point_t const* gazePoint, void* userData)
{
    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)userData;
    GAZE_REPORT gazeReport;

    KdPrint(("OnGazeEvent: Timestamp=0x%p x=%e y=%e, valid=%d\n", 
        gazePoint->timestamp_us, gazePoint->position_xy[0], gazePoint->position_xy[1], gazePoint->validity));

    if (gazePoint->validity == TOBII_VALIDITY_VALID)
    {
        gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
        gazeReport.TimeStamp = gazePoint->timestamp_us;
        gazeReport.GazePoint.X = gazePoint->position_xy[0];
        gazeReport.GazePoint.Y = gazePoint->position_xy[1];
        gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
        SendGazeReport(deviceContext, &gazeReport);
    }
}

DWORD WINAPI TobiiFrameProc(PVOID startParam)
{
    // Almost all methods in the Tobii API returns a tobii_error_t. The Tobii API never throws an exception.
    tobii_error_t error;

    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)startParam;
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;

    while (!sensorContext->ExitThread)
    {
        error = tobii_wait_for_callbacks(sensorContext->TobiiDevice);
        if (error == TOBII_ERROR_NO_ERROR)
        {
            tobii_process_callbacks(sensorContext->TobiiDevice);
        }
        else if (error == TOBII_ERROR_CONNECTION_FAILED)
        {
            error = tobii_reconnect(sensorContext->TobiiDevice);
            KdPrint(("Connection failed. Reconnect attempt returned error: 0x%0x\n", error));
        }
        else
        {
            KdPrint(("tobii_wait_for_callbacks returned error: 0x%0x\n", error));
        }
        Sleep(15);
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
    memset(sensorContext, 0, sizeof(*sensorContext));

    tobii_api_t* api;
    tobii_device_t *device;
    tobii_error_t error;

    error = tobii_api_create(&api, NULL, NULL);
    CLEANUP_ON_FAIL(error);

    sensorContext->TobiiApi = api;

    // Sending an empty url connects to the first eye tracker found.
    error = tobii_device_create(api, NULL, &device);
    CLEANUP_ON_FAIL(error);
    sensorContext->TobiiDevice = device;

    // Subscribe to a stream of wearable eye tracking data. The callback you supply will be called from tobii_process_callbacks.
    error = tobii_gaze_point_subscribe(device, OnGazeEvent, deviceContext);
    CLEANUP_ON_FAIL(error);
    sensorContext->GazeStreamSubscribed = TRUE;

    // Create a thread for data processing
    sensorContext->ThreadHandle = CreateThread(NULL, 0, TobiiFrameProc, deviceContext, CREATE_SUSPENDED, &sensorContext->ThreadId);
    if (sensorContext->ThreadHandle == NULL)
    {
        error = TOBII_ERROR_NO_ERROR;
        goto Cleanup;
    }

    ResumeThread(sensorContext->ThreadHandle);
Cleanup:
    return (error == TOBII_ERROR_NO_ERROR);
}

void ShutdownEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;
    
    sensorContext->ExitThread = TRUE;
    if (sensorContext->ThreadHandle)
    {
        WaitForSingleObject(sensorContext->ThreadHandle, INFINITE);
        sensorContext->ThreadHandle = NULL;
    }

    if (sensorContext->GazeStreamSubscribed)
    {
        tobii_gaze_point_unsubscribe(sensorContext->TobiiDevice);
        sensorContext->GazeStreamSubscribed = FALSE;
    }

    if (sensorContext->TobiiDevice)
    {
        tobii_device_destroy(sensorContext->TobiiDevice);
        sensorContext->TobiiDevice = NULL;
    }
    
    if (sensorContext->TobiiApi)
    {
        tobii_api_destroy(sensorContext->TobiiApi);
        sensorContext->TobiiApi = NULL;
    }
    
    deviceContext->EyeTracker = NULL;
}
