// jailbreak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <vector>

#ifdef _M_X64
#define PROGNAME "jailbreak64"
#else
#define PROGNAME "jailbreak32"
#endif

std::string concatenateStringAndTChar(const std::string& str, const TCHAR* tcharStr) {
#ifdef UNICODE
    // Convert TCHAR* (wchar_t*) to std::wstring
    std::wstring wideTCharStr(tcharStr);

    // Convert std::wstring to std::string
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string convertedTCharStr = converter.to_bytes(wideTCharStr);

    // Concatenate
    return str + convertedTCharStr;
#else
    // Direct concatenation (TCHAR* is char*)
    return str + tcharStr;
#endif
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

    if (argc <= 1)
    {
        printf(PROGNAME " - Launches other applications and hooks function calls\n");
        printf("            to allow for the export of non-exportable certificates.\n");
        printf("(c) 2014 iSEC Partners\n");
        printf("-------------------------------------------------------------------\n");
        printf("Usage:\n");
        printf(" " PROGNAME " <program> <options>\n\n");
        printf("Example:\n\n");
        printf(PROGNAME " c:\\windows\\system32\\mmc.exe c:\\windows\\system32\\certmgr.msc -32\n");
        printf(" - Launches the Certificate Manager and allows it to export non-exportable\n");
        printf("   certificates.\n");
        return -1;
    }

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
            printf("Failed to write to jailbreak.log no logging will be performed. (%d)\n", GetLastError());
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


    TCHAR* targetIPAddress = argv[2];
    printf("TargetIPAddress = %s\n", targetIPAddress);

    // Create the custom environment variable string
    std::string customEnv = std::string("TargetIPAddress=") + targetIPAddress + "\0";
    printf("customEnv = %s\n", customEnv.c_str());
    
    // Get the current environment block
    LPCH currEnv = GetEnvironmentStrings();

    // Copy the current environment block and append the custom variable
    std::vector<char> newEnv;
    for (LPCH p = currEnv; *p != 0; p += strlen(p) + 1) {
        newEnv.insert(newEnv.end(), p, p + strlen(p) + 1);
    }
    FreeEnvironmentStrings(currEnv);

    // Append the custom environment variable
    newEnv.insert(newEnv.end(), customEnv.begin(), customEnv.end());

    // Ensure double-null termination
    newEnv.push_back('\0');
    newEnv.push_back('\0');

    bResult = CreateProcess(NULL,
        pCommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED, 
        //NULL, NULL, &si, &pi);
        newEnv.data(), NULL, &si, &pi);

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

    //Sleep(10000);

    nt = RhInjectLibrary(pi.dwProcessId, 0, EASYHOOK_INJECT_DEFAULT,
        L"HookTest32.dll",
        L"HookTest64.dll", NULL, 0);

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

