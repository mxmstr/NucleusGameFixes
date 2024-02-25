// jailbreak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <vector>

#ifdef _M_X64
#define PROGNAME "AutoRebindInjector64"
#else
#define PROGNAME "AutoRebindInjector32"
#endif

// Helper function to convert string to DWORD (process ID)
DWORD StringToDWORD(const TCHAR* str) {
    return static_cast<DWORD>(_ttoi(str));
}

int _tmain(int argc, _TCHAR* argv[])
{
    int ret = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR* lptstr = GetCommandLine();
    TCHAR* pCommandLine = NULL;
    TCHAR* pFirstArg = NULL;
    TCHAR* pPos = NULL;
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
    hLogFile = CreateFile("injector.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // Using argv[] and GetCommandLine so I can steal the correctly quoted command line from GetCommandLine
    // instead of using argv[] and concatenating a bunch of strings and getting quoting correct. Using
    // argv[] so I know where the arguments start in the string from GetCommandLine
    pFirstArg = argv[1];
    pPos = _tcsstr(lptstr, pFirstArg);
    if (pPos == NULL)
    {
        printf("Could not find first argument [%S] in command line [%S]\n", pFirstArg, lptstr);
        return -1;
    }

    // If first parameter is quoted the quote doesn't show up in argv[] but
    // it does show up in GetCommandLine.
    if (pPos[-1] == _T('\"') || pPos[-1] == _T('\''))
        pPos--;

    len = _tcslen(pPos);
    pCommandLine = (TCHAR*)calloc(len + 1, sizeof(TCHAR));
    if (!pCommandLine)
    {
        printf("Failed to allocate memory\n");
        return -1;
    }

    StringCchCopy(pCommandLine, len + 1, pPos);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString)/sizeof(TCHAR), "\r\n(%d): %S\r\n", GetCurrentProcessId(), PROGNAME);
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            printf("Failed to write to injector.log no logging will be performed. (%d)\n", GetLastError());
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): Launching new process = %s\r\n", GetCurrentProcessId(), pCommandLine);
        WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL);
    }

    // Check for command line option for runtime injection
    bool runtimeInjection = false;
    DWORD targetProcessId = 0;
    if (argc > 1) {
        if (_tcscmp(argv[1], _T("-runtime")) == 0) {
            runtimeInjection = true;
            if (argc < 3) {
                printf("Process ID not specified for runtime injection.\n");
                return -1;
            }
            targetProcessId = StringToDWORD(argv[2]);
        }
    }

    // Modify injection logic based on the chosen mode
    if (runtimeInjection) {
        // Runtime injection logic
        nt = RhInjectLibrary(targetProcessId, 0, EASYHOOK_INJECT_DEFAULT,
            L"AutoRebind32.dll",
            L"AutoRebind64.dll", NULL, 0);
        if (nt != 0) {
            printf("RhInjectLibrary failed with error code = %d\n", nt);
            ret = -1;
        }

        if (pCommandLine)
            free(pCommandLine);

        if (hLogFile != INVALID_HANDLE_VALUE)
            CloseHandle(hLogFile);

        return ret;
    }

    bResult = CreateProcess(NULL,
        pCommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED,
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
        L"AutoRebind32.dll",
        L"AutoRebind64.dll", NULL, 0);

    if (nt != 0)
    {
        printf("RhInjectLibrary failed with error code = %d\n", nt);
        ret = -1;
    }
    printf("\n");

    ResumeThread(pi.hThread);

    if (pCommandLine)
        free(pCommandLine);

    WaitForSingleObject(pi.hProcess, 5000);

    if (hLogFile != INVALID_HANDLE_VALUE)
        CloseHandle(hLogFile);

    return ret;
}

