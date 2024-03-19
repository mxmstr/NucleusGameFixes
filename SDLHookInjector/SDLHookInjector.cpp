// jailbreak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <vector>

#ifdef _M_X64
#define PROGNAME "SDLHookInjector64"
#else
#define PROGNAME "SDLHookInjector32"
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    int ret = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    std::string pCommandLine = "Battlefront.exe";
    size_t len = 0;
    BOOL bResult = FALSE;
    NTSTATUS nt = 0;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;
    TCHAR tempString[2048];

    ZeroMemory(tempString, sizeof(tempString));

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory(&pi, sizeof(pi));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("sdlhook.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString)/sizeof(TCHAR), "\r\n(%d): %S\r\n", GetCurrentProcessId(), PROGNAME);
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            printf("Failed to write to jailbreak.log no logging will be performed. (%d)\n", GetLastError());
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    // Append any command line arguments
    for (int i = 1; i < argc; i++) // Start from 1 to skip the program name
    {
        pCommandLine += " ";
        pCommandLine += std::string(argv[i]);
    }

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): Launching new process = %s\r\n", GetCurrentProcessId(), pCommandLine);
        WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL);
    }

    LPSTR argstr = const_cast<char*>(pCommandLine.c_str());

    bResult = CreateProcessA(NULL,
        argstr, NULL, NULL, FALSE, CREATE_SUSPENDED,
        NULL, NULL, &si, &pi);

    if (!bResult)
    {
        printf("CreateProcess failed with error code = %d\n", GetLastError());
        return -1;
    }

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): Injecting hook library to process %d\r\n", GetCurrentProcessId(), pi.dwProcessId);
        WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL);
    }

    nt = RhInjectLibrary(pi.dwProcessId, 0, EASYHOOK_INJECT_DEFAULT,
        L"SDLHook32.dll",
        L"SDLHook64.dll", NULL, 0);

    if (nt != 0)
    {
        printf("RhInjectLibrary failed with error code = %d\n", nt);
        ret = -1;
    }
    printf("\n");

    ResumeThread(pi.hThread);

    /*if (pCommandLine)
        free(pCommandLine);*/

    WaitForSingleObject(pi.hProcess, 5000);

    if (hLogFile != INVALID_HANDLE_VALUE)
        CloseHandle(hLogFile);

    return ret;
}

