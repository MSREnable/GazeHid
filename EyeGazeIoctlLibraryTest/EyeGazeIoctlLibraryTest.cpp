#include <iostream>                 // for printf
#include <wtypes.h>                 // for mouse and monitor info
#include <time.h>                   // for __time
#include <conio.h>                  // for _kbhit
#include "EyeGazeIoctlLibrary.h"

int main()
{
	InitializeEyeGaze();

    INT32 x = 0;
    INT32 y = 0;
    INT32 monitorWidthUm = GetPrimaryMonitorWidth();
    INT32 monitorHeightUm = GetPrimaryMonitorHeight();
    INT32 velocity = 125; // step size in micrometers
    FLOAT PI = 3.14159f;
    FLOAT angle = 45.0f;
    INT32 noise = 1000;
    INT32 noise_step = 500;
    INT32 noiseX = 0;
    INT32 noiseY = 0;
    BOOL  mouseMode = FALSE;

    INT32 gazePointX = 0;
    INT32 gazePointY = 0;
    INT64 timestamp;

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
        gazePointX = x;
        gazePointY = y;
        _time64((__time64_t*)(&timestamp));

        // Add noise
        if (noise > 0)
        {
            UINT rangeOffset = noise / 2;
            noiseX = (rand() % noise) - rangeOffset;
            noiseY = (rand() % noise) - rangeOffset;

            gazePointX += noiseX;
            gazePointY += noiseY;
        }

        //if (IsTrackerEnabled())
        {
            if (SendGazeReport(gazePointX, gazePointY, timestamp))
            {
                printf("Original (%8dum, %8dum) Noise (%8dum) Sent (%8d, %8d) Source: %11s\r",
                    x, y,
                    noise,
                    gazePointX, gazePointY,
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
}