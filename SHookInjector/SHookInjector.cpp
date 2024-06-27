#include "stdafx.h"
#include <string>
#include <vector>

#ifdef _M_X64
#define PROGNAME "SHook64"
#else
#define PROGNAME "SHook32"
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    std::string iniFilePath = "";

    TCHAR executablePath[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, executablePath)) {
        iniFilePath = std::string(executablePath) + "\\SHook.ini";
    }

    char programNameBuffer[256];
    GetPrivateProfileString("Settings", "ProgramName", "", programNameBuffer, sizeof(programNameBuffer), iniFilePath.c_str());

    char* programName = new char[strlen(programNameBuffer) + 1];  // +1 for the null terminator
    std::strcpy(programName, programNameBuffer);


    int ret = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    std::string pCommandLine = programName;
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
    hLogFile = CreateFile("shook.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString)/sizeof(TCHAR), "\r\n(%d): %S\r\n", GetCurrentProcessId(), PROGNAME);
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            printf("Failed to write to lhook.log no logging will be performed. (%d)\n", GetLastError());
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

    //Sleep(10000);

    // Get the first DLL file
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile("plugins\\*.dll", &findFileData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Convert cFileName to wstring
            std::wstring cFileName =
                L"plugins\\" +
                std::wstring(findFileData.cFileName, findFileData.cFileName + strlen(findFileData.cFileName));
            WCHAR* dllPath = const_cast<WCHAR*>(cFileName.c_str());

            // Inject the DLL
            nt = RhInjectLibrary(pi.dwProcessId, 0, EASYHOOK_INJECT_DEFAULT,
                dllPath, dllPath, NULL, 0);

            if (nt != 0)
            {
                printf("RhInjectLibrary failed for %ls with error code = %d\n", dllPath, nt);
                ret = -1;
            }
            printf("\n");
        } while (FindNextFile(hFind, &findFileData) != 0);

        FindClose(hFind);
    }

    ResumeThread(pi.hThread);

    /*if (pCommandLine)
        free(pCommandLine);*/

    WaitForSingleObject(pi.hProcess, 5000);

    if (hLogFile != INVALID_HANDLE_VALUE)
        CloseHandle(hLogFile);

    return ret;
}

