// EyeGazeIoctlLibrary.h - Contains declarations of math functions
#pragma once

#ifdef EYEGAZEIOCTLLIBRARY_EXPORTS
#define EYEGAZEIOCTLLIBRARY_API __declspec(dllexport)
#else
#define EYEGAZEIOCTLLIBRARY_API __declspec(dllimport)
#endif

extern "C" EYEGAZEIOCTLLIBRARY_API bool InitializeEyeGaze();

extern "C" EYEGAZEIOCTLLIBRARY_API bool SendGazeReport(long X, long Y, unsigned long long timestamp);

extern "C" EYEGAZEIOCTLLIBRARY_API int GetPrimaryMonitorWidth();

extern "C" EYEGAZEIOCTLLIBRARY_API int GetPrimaryMonitorHeight();