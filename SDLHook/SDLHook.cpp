// jailbreakhook.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <string>
#include <tlhelp32.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windns.h>
#include <winreg.h>
#include <aclapi.h>
#include <sddl.h>
#include <random>
#include <objbase.h>
#include <combaseapi.h>
#include <wbemcli.h>
#include <map>
#include <SDL.h>
using namespace std;

int controllerIndex = 0;
SDL_GameController* controllerHandle = nullptr;

HANDLE hLogFile = INVALID_HANDLE_VALUE;
TCHAR tempString[2048];
WCHAR tempStringW[2048];

void WriteToLogFile(void* lpBuffer, size_t cbBytes)
{
    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        WriteFile(hLogFile, lpBuffer, (DWORD)cbBytes, &dwWritten, NULL);
    }
}

void Log(string message)
{
    std::stringstream logMessage;
    logMessage << GetCurrentProcessId() << ": " << message;

    StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), (logMessage.str()+ "\r\n").c_str());
    WriteToLogFile(tempString, _tcslen(tempString) * sizeof(TCHAR));
}

void LogW(wstring message)
{
    std::wstringstream logMessage;
    logMessage << GetCurrentProcessId() << ": " << message;
    
    StringCchPrintfW(tempStringW, sizeof(tempStringW) / sizeof(WCHAR), (logMessage.str() + L"\r\n").c_str());
    WriteToLogFile(tempStringW, wcslen(tempStringW) * sizeof(WCHAR));
}

NTSTATUS InstallHook(LPCSTR moduleHandle, LPCSTR proc, void* callBack)
{
    HOOK_TRACE_INFO hHook = { NULL };

    NTSTATUS result = LhInstallHook(
        GetProcAddress(GetModuleHandle(moduleHandle), proc),
        callBack,
        NULL,
        &hHook);
    if (FAILED(result))
    {
        std::wstringstream logMessage;
        logMessage << "SHOOK: Error installing " << proc << " hook, error msg: " << RtlGetLastErrorString();
        LogW(logMessage.str());
    }
    else
    {
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hHook);
    }

    return result;
}

int MySDL_NumJoysticks()
{
    Log("MySDL_NumJoysticks");
    return 1;
}

SDL_bool SDLCALL MySDL_IsGameController(int joystick_index)
{
    Log("MySDL_IsGameController");
    return (controllerIndex == joystick_index) ? SDL_TRUE : SDL_FALSE;
}

SDL_GameController* SDLCALL MySDL_GameControllerOpen(int joystick_index)
{
    Log("MySDL_GameControllerOpen");
    return (controllerIndex == joystick_index) ? SDL_GameControllerOpen(controllerIndex) : nullptr;
}

SDLHOOK_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("sdlhook.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): SDLHook.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    std::stringstream logMessage;


    //if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
    //    logMessage << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
    //    return;
    //}

    //// Check for connected game controllers
    //int numControllers = SDL_NumJoysticks();
    //logMessage << "Number of connected game controllers: " << numControllers << endl;

    //if (SDL_IsGameController(controllerIndex)) {
    //    controllerHandle = SDL_GameControllerOpen(controllerIndex);

    //    if (controllerHandle) {
    //        logMessage << "Controller is connected " << controllerIndex << endl;

    //        //// Get the name of the controller
    //        //printf("Controller %d name: %s\n", i, SDL_GameControllerName(controller));

    //        //// You can use the controller here for input handling

    //        //// Close the controller
    //        //SDL_GameControllerClose(controller);
    //    }
    //    else {
    //        logMessage << "Failed to open controller " << controllerIndex << endl;
    //    }
    //}
    //else {
    //    logMessage << "Joystick is not a game controller " << controllerIndex << endl;
    //}

    // Allocate a buffer to hold the executable path
    std::string iniFilePath = "";

    TCHAR executablePath[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, executablePath)) {
        iniFilePath = std::string(executablePath) + "\\SDLHook.ini";
    }

    // Load the controllerIndex value
    controllerIndex = GetPrivateProfileInt("Settings", "ControllerIndex", 0, iniFilePath.c_str());
    Log("ControllerIndex: ");
    Log(std::to_string(controllerIndex));

    //InstallHook("SDL.dll", "SDL_NumJoysticks", MySDL_NumJoysticks);
    InstallHook("SDL.dll", "SDL_IsGameController", MySDL_IsGameController);
    //InstallHook("SDL.dll", "SDL_GameControllerOpen", MySDL_GameControllerOpen);

    Log(logMessage.str());
}
