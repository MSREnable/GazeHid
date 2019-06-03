#include "driver.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <wchar.h>
#include <strsafe.h>
#include <stdlib.h>

#define DEFAULT_SERVER  "127.0.0.1"
#define DEFAULT_PORT    4242
#define MAXLEN_STRINGS  32

typedef struct _SENSOR_CONTEXT
{
    SOCKET          Socket;
    BOOL            DeviceStarted;
    HANDLE          ThreadHandle;
    DWORD           ThreadId;
    volatile BOOL   ExitThread;
    int64_t         TimeTickFrequency;
    RECT            ScreenCoordinates;
    SIZE            CameraSize;
    int             SamplingRate;
    CHAR            Bus[MAXLEN_STRINGS];
    CHAR            ProductId[MAXLEN_STRINGS];
    CHAR            SerialId[MAXLEN_STRINGS];
    CHAR            CompanyId[MAXLEN_STRINGS];
    CHAR            ApiId[MAXLEN_STRINGS];
    int             ActiveId;
    int             MaxId;
} SENSOR_CONTEXT, *PSENSOR_CONTEXT;

DWORD InitializeWinsock()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    DWORD ret = ERROR_INVALID_PARAMETER;

    wVersionRequested = MAKEWORD(2, 2);

    if (ERROR_SUCCESS != (ret = WSAStartup(wVersionRequested, &wsaData)))
    {
        KdPrint(("WSAStartup failed. Error=0x%08x\n", ret));
        return ret;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
    {
        WSACleanup();
        KdPrint(("WSAStartup returned unsupported winsock version [%d, %d]\n", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion)));
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

BOOL SetConfigVariable(PSENSOR_CONTEXT sensorContext, PSTR variable, BOOL value)
{
    HRESULT hr;
    char data[1024];
    char expected[1024];
    int result;

    hr = StringCchPrintfA(data, 1024, "<SET ID=\"%s\" STATE=\"%d\" />\r\n", variable, value);
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = StringCchPrintfA(expected, 1024, "<ACK ID=\"%s\" STATE=\"%d\" />\r\n", variable, value);
    result = send(sensorContext->Socket, data, (int)(strlen(data) + 1), 0);
    if (result == SOCKET_ERROR)
    {
        return FALSE;
    }

    result = recv(sensorContext->Socket, data, 1024, 0);
    if (result == SOCKET_ERROR)
    {
        return FALSE;
    }

    if (0 != strcmpi(data, expected))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL GetVariable(PSENSOR_CONTEXT sensorContext, PSTR variable, PSTR value, int cchValue)
{
    HRESULT hr;
    char data[1024];
    char expected[1024];
    int result;

    hr = StringCchPrintfA(data, 1024, "<GET ID=\"%s\" />\r\n", variable);
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = StringCchPrintfA(expected, 1024, "<ACK ID=\"%s\"", variable);

    result = send(sensorContext->Socket, data, (int)(strlen(data) + 1), 0);
    if (result == SOCKET_ERROR)
    {
        return FALSE;
    }

    result = recv(sensorContext->Socket, value, cchValue, 0);
    if (result == SOCKET_ERROR)
    {
        return FALSE;
    }

    if (0 != _strnicmp(value, expected, strlen(expected)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CollectTrackerParameters(PSENSOR_CONTEXT sensorContext)
{
    char data[1024];
    int size = sizeof(data);
    int ret;

    if (!GetVariable(sensorContext, "TIME_TICK_FREQUENCY", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"TIME_TICK_FREQUENCY\" FREQ=\"%lld\" />", &sensorContext->TimeTickFrequency);
    if (ret != 1)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "SCREEN_SIZE", data, size))
    {
        return FALSE;
    }

    PRECT rc = &sensorContext->ScreenCoordinates;
    ret = sscanf_s(data, "<ACK ID=\"SCREEN_SIZE\" X=\"%d\" Y=\"%d\" WIDTH=\"%d\" HEIGHT=\"%d\" />",
        &rc->left, &rc->top, &rc->right, &rc->bottom);
    if (ret != 4)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "CAMERA_SIZE", data, size))
    {
        return FALSE;
    }
    PSIZE sz = &sensorContext->CameraSize;
    ret = sscanf_s(data, "<ACK ID=\"CAMERA_SIZE\" WIDTH=\"%d\" HEIGHT=\"%d\" />", &sz->cx, &sz->cy);

    if (!GetVariable(sensorContext, "PRODUCT_ID", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"PRODUCT_ID\" VALUE=\"%[A-Z0-9]\" BUS=\"%[A-Z0-9]\" RATE=\"%d\" />", 
        &sensorContext->ProductId, MAXLEN_STRINGS, &sensorContext->Bus, MAXLEN_STRINGS, &sensorContext->SamplingRate);
    if (ret != 3)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "SERIAL_ID", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"SERIAL_ID\" VALUE=\"%[A-Z0-9]\" />", &sensorContext->SerialId, MAXLEN_STRINGS);
    if (ret != 1)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "COMPANY_ID", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"COMPANY_ID\" VALUE=\"%[A-Z0-9]\" />", &sensorContext->CompanyId, MAXLEN_STRINGS);
    if (ret != 1)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "API_ID", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"API_ID\" VALUE=\"%[A-Z0-9]\" />", &sensorContext->ApiId, MAXLEN_STRINGS);
    if (ret != 1)
    {
        return FALSE;
    }

    if (!GetVariable(sensorContext, "TRACKER_ID", data, size))
    {
        return FALSE;
    }
    ret = sscanf_s(data, "<ACK ID=\"TRACKER_ID\" ACTIVE_ID=\"%d\" MAX_ID=\"%d\" SEARCH=\"NONE\" />",
        &sensorContext->ActiveId, &sensorContext->MaxId);
    if (ret != 2)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL SetDataRecordVariables(PSENSOR_CONTEXT sensorContext)
{
    PSTR variables[] = {
        "ENABLE_SEND_TIME_TICK",
        "ENABLE_SEND_POG_FIX",
        "ENABLE_SEND_DATA"  // this has to be last
    };
    for (int i = 0; i < __crt_countof(variables); i++)
    {
        if (!SetConfigVariable(sensorContext, variables[i], TRUE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

DWORD ConnectToEyeTracker(PSENSOR_CONTEXT sensorContext)
{
    DWORD err = ERROR_INVALID_PARAMETER;
    if (sensorContext->Socket != INVALID_SOCKET)
    {
        closesocket(sensorContext->Socket);
        sensorContext->Socket = INVALID_SOCKET;
    }

    sensorContext->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sensorContext->Socket == INVALID_SOCKET)
    {
        err = WSAGetLastError();
        KdPrint(("Failed to create socket: 0x%08x\n", err));
        return err;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(DEFAULT_SERVER);
    addr.sin_port = htons(DEFAULT_PORT);

    if (SOCKET_ERROR == connect(sensorContext->Socket, (PSOCKADDR)&addr, (int)sizeof(addr)))
    {
        err = WSAGetLastError();
        KdPrint(("Failed to connect to server: 0x%08x\n", err));
        closesocket(sensorContext->Socket);
        sensorContext->Socket = INVALID_SOCKET;
        return err;
    }

    err = ERROR_SUCCESS;

    return err;
}

void ProcessGazeRecords(PDEVICE_CONTEXT deviceContext, PSTR recordStr)
{
    //PSENSOR_CONTEXT sensorContext = deviceContext->EyeTracker;
    char *delimiters = "\r\n";
    char *token = strtok(recordStr, delimiters);
    while (token != NULL)
    {
        KdPrint(("GazePointFrameProc: %s\n", token));

        // The data is in the format below
        //<REC TIME_TICK = "297599783255" BPOGX = "0.62688" BPOGY = "1.30695" BPOGV = "0" / >
        GAZE_REPORT gazeReport;

        char time_str[16] = { 0 };
        float pogX;
        float pogY;
		float pogS;
		float pogD;
		int pogID;
        BOOL valid;		
		PSTR dataFormat = "<REC TIME_TICK=\"%[0-9]\" FPOGX=\"%g\" FPOGY=\"%g\" FPOGS=\"%g\" FPOGD=\"%g\" FPOGID=\"%i\" FPOGV=\"%d\" />";
        int result = sscanf_s(token, dataFormat, time_str, sizeof(time_str), &pogX, &pogY, &pogS, &pogD, &pogID, &valid);
		
        token = strtok(NULL, delimiters);

        if (result != 7)
        {
            KdPrint(("GazePointFrameProc: Invalid record received\n", result));
            break;
        }

        if (!valid)
        {
            // This is a valid error condition. Dont reset the connection.
            KdPrint(("GazePointFrameProc: POG is not valid\n"));
            continue;
        }

        gazeReport.ReportId = HID_USAGE_TRACKING_DATA;
        gazeReport.TimeStamp = _atoi64(time_str)/10;
        gazeReport.GazePoint.X = (int32_t)(pogX * deviceContext->ConfigurationReport.CalibratedScreenWidth);
        gazeReport.GazePoint.Y = (int32_t)(pogY * deviceContext->ConfigurationReport.CalibratedScreenHeight);
        KdPrint(("GazePointFrameProc: %lld, %d, %d\n\n", gazeReport.TimeStamp, gazeReport.GazePoint.X, gazeReport.GazePoint.Y));
        SendGazeReport(deviceContext, &gazeReport);
    }
}

// Removed:
//PSTR dataFormat = "<REC TIME_TICK=\"%[0-9]\" BPOGX=\"%g\" BPOGY=\"%g\" BPOGV=\"%d\" />";
//int result = sscanf_s(token, dataFormat, time_str, sizeof(time_str), &pogX, &pogY,&valid);

DWORD WINAPI GazePointFrameProc(PVOID startParam)
{
    DWORD ret;
    PDEVICE_CONTEXT deviceContext = (PDEVICE_CONTEXT)startParam;
    PSENSOR_CONTEXT sensorContext = deviceContext->EyeTracker;

    while (TRUE)
    {
        ret = ConnectToEyeTracker(sensorContext);
        if (ret != ERROR_SUCCESS)
        {
            // connection failed. Sleep for a second and try again
            Sleep(1000);
            continue;
        }

        // in general, for any error in this function, we close the socket and reinitiate the connection
        if (!CollectTrackerParameters(sensorContext))
        {
            continue;
        }

        if (!SetDataRecordVariables(sensorContext))
        {
            continue;
        }

        SendTrackerStatusReport(deviceContext, TRACKER_STATUS_READY);

#define BUFFER_SIZE 128
        char buffer[BUFFER_SIZE];
        char *data = buffer;
        int len = BUFFER_SIZE;

        while (!sensorContext->ExitThread)
        {
            int result = recv(sensorContext->Socket, data, len - 1, 0);
            if (result == SOCKET_ERROR)
            {
                KdPrint(("GazePointFrameProc: recv failed with error=0x%08x\n", result));
                break;
            }
            data[result] = '\0';

            // find the last newline character
            char *end = strrchr(data, '\n');
            if (end == NULL)
            {
                break;
            }
            *end = '\0';
            end++; // and have it point to the next character

            ProcessGazeRecords(deviceContext, buffer);
            
            // this is a quick hack. not the best way to do this.
            // copy the remaining stuff to the beginning of the string.
            int fragmentLen = (int)strlen(end);
            memmove(buffer, end, fragmentLen);
            buffer[fragmentLen] = '\0';
            data = buffer + fragmentLen;
            len = BUFFER_SIZE - fragmentLen;
        }
    }
    return 0;
}


BOOL InitializeEyeTracker(PDEVICE_CONTEXT deviceContext)
{
    PSENSOR_CONTEXT sensorContext;
    
    if (ERROR_SUCCESS != InitializeWinsock())
    {
        KdPrint(("InitializeWinsock failed \n"));
        return FALSE;
    }

    sensorContext = (PSENSOR_CONTEXT)LocalAlloc(0, (sizeof(*sensorContext)));
    if (!sensorContext)
    {
        return FALSE;
    }

    deviceContext->EyeTracker = sensorContext;

    sensorContext->Socket = INVALID_SOCKET;
    sensorContext->ExitThread = FALSE;
    sensorContext->ThreadHandle = CreateThread(NULL, 0, GazePointFrameProc, deviceContext, CREATE_SUSPENDED, &sensorContext->ThreadId);
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

