#include "driver.h"
#include "Tracker.h"


// Deviations from spec
// * Gaze point units in float instead of micrometers
// * Eye positions in a different callback than gaze point
// * The discovery protocol cannot be based on tcp urls. We shouldnt rely on opening the first found device either
// * What about implementing a head tracker now that stream sdk has support for it
// * Calibration requires a license. This is incompatible with standardization


#define TOBII_USE_STREAM_SDK

#ifdef TOBII_USE_STREAM_SDK
#include "tobii/tobii.h"
#include "tobii/tobii_streams.h"

typedef struct _SENSOR_CONTEXT
{
    tobii_context_t*    TobiiContext;
    BOOL                DeviceStarted;
    HANDLE              ThreadHandle;
    DWORD               ThreadId;
    volatile BOOL       ExitThread;
} SENSOR_CONTEXT, *PSENSOR_CONTEXT;


#define CLEANUP_ON_FAIL(error) if (error != TOBII_ERROR_NO_ERROR) goto Cleanup;

void OnGazeEvent(float x, float y, int64_t timestamp_us, void* userData)
{
    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)userData;
    GAZE_REPORT gazeReport;

    KdPrint(("OnGazeEvent: Timestamp=0x%p x=%e y=%e\n", timestamp_us, x, y));

    gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
    gazeReport.TimeStamp = timestamp_us;
    gazeReport.GazePoint.X = x;
    gazeReport.GazePoint.Y = y;
    gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
    SendGazeReport(deviceContext, &gazeReport);
}

DWORD WINAPI TobiiFrameProc(PVOID startParam)
{
    // Almost all methods in the Tobii API returns a tobii_error_t. The Tobii API never throws an exception.
    tobii_error_t error;

    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)startParam;
    PSENSOR_CONTEXT sensorContext = (PSENSOR_CONTEXT)deviceContext->EyeTracker;

    while (!sensorContext->ExitThread)
    {
        error = tobii_wait_for_callbacks(sensorContext->TobiiContext);
        if (error == TOBII_ERROR_NO_ERROR)
        {
            tobii_process_callbacks(sensorContext->TobiiContext);
        }
        else if (error == TOBII_ERROR_CONNECTION_FAILED)
        {
            error = tobii_reconnect(sensorContext->TobiiContext);
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

    tobii_error_t error;
    tobii_context_t * context;

    sensorContext->ExitThread = FALSE;

    // To be able to use the API you need to initialize it once. This is your chance to provide custom handling for memory allocation and logging.
    error = tobii_initialize(NULL, NULL);
    CLEANUP_ON_FAIL(error);

    // Sending an empty url connects to the first eye tracker found.
    error = tobii_context_create(&context, 0);
    CLEANUP_ON_FAIL(error);

    sensorContext->TobiiContext = context;

    // Subscribe to a stream of wearable eye tracking data. The callback you supply will be called from tobii_process_callbacks.
    error = tobii_gaze_point_subscribe(context, OnGazeEvent, deviceContext);
    CLEANUP_ON_FAIL(error);

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
    WaitForSingleObject(sensorContext->ThreadHandle, INFINITE);

    tobii_gaze_point_unsubscribe(sensorContext->TobiiContext);
    tobii_context_destroy(sensorContext->TobiiContext);
    tobii_terminate();
    
    deviceContext->EyeTracker = NULL;
}


#else
#include "tobiigaze.h"
#include "tobiigaze_discovery.h"
#include "tobiigaze_error_codes.h"

#define CLEANUP_ON_FAIL(error) if (error != TOBIIGAZE_ERROR_SUCCESS) goto Cleanup;


void OnEventLoopExit(tobiigaze_error_code error, void *userData)
{
    UNREFERENCED_PARAMETER(error);
    UNREFERENCED_PARAMETER(userData);
}

void OnError(tobiigaze_error_code error, void *userData)
{
    UNREFERENCED_PARAMETER(error);
    UNREFERENCED_PARAMETER(userData);

}

void OnGazeEvent(const struct tobiigaze_gaze_data *gazeData, const struct tobiigaze_gaze_data_extensions *gazeExtensions, void *userData)
{
    UNREFERENCED_PARAMETER(gazeExtensions);
    UNREFERENCED_PARAMETER(userData);

    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)userData;
    const struct tobiigaze_gaze_data_eye *eye = NULL;

    if (gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
        gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_LEFT_EYE_TRACKED ||
        gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_LEFT)
    {
        eye = &gazeData->left;
    }

    if (gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
        gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_RIGHT_EYE_TRACKED ||
        gazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_RIGHT)
    {
        eye = &gazeData->right;
    }

    if (eye == NULL)
    {
        return;
    }

    GAZE_REPORT gazeReport;
    gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
    gazeReport.TimeStamp = gazeData->timestamp;
    gazeReport.GazePoint.X = (float)eye->gaze_point_on_display_normalized.x;
    gazeReport.GazePoint.Y = (float)eye->gaze_point_on_display_normalized.y;
    SendGazeReport(deviceContext, &gazeReport);
}

BOOL InitializeEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    char url[MAX_PATH];
    tobiigaze_error_code error;

    tobiigaze_get_connected_eye_tracker(url, MAX_PATH, &error);
    CLEANUP_ON_FAIL(error);

    deviceContext->EyeTracker = tobiigaze_create(url, &error);
    CLEANUP_ON_FAIL(error);

    tobiigaze_register_error_callback((tobiigaze_eye_tracker *)deviceContext->EyeTracker, OnError, deviceContext);

    tobiigaze_run_event_loop_on_internal_thread(deviceContext->EyeTracker, OnEventLoopExit, deviceContext);

    tobiigaze_connect(deviceContext->EyeTracker, &error);
    CLEANUP_ON_FAIL(error);

    tobiigaze_start_tracking(deviceContext->EyeTracker, OnGazeEvent, &error, deviceContext);
    CLEANUP_ON_FAIL(error);

Cleanup:
    return error == TOBIIGAZE_ERROR_SUCCESS;
}


void ShutdownEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    tobiigaze_error_code error;
    tobiigaze_stop_tracking(deviceContext->EyeTracker, &error);

    tobiigaze_disconnect(deviceContext->EyeTracker);

    tobiigaze_break_event_loop(deviceContext->EyeTracker);

    tobiigaze_destroy(deviceContext->EyeTracker);
}

#endif
