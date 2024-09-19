// jailbreakhook.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include <sstream>
#include <vector>
#include <windows.h>
#include <string>
#include <winsock2.h>
#include <detours.h>
#include <algorithm>
#include <DbgHelp.h>
#include <Psapi.h>
using namespace std;

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

NTSTATUS InstallHook(LPCSTR moduleHandle, LPCSTR proc, void* callBack, bool manual)
{
    HOOK_TRACE_INFO hHook = { NULL };

    // Log the function address
    std::stringstream logMessage;
    logMessage << "SHOOK: Installing hook for " << proc << " at address " << GetProcAddress(GetModuleHandle(moduleHandle), proc);
    Log(logMessage.str());

    NTSTATUS result = LhInstallHook(
        manual ? 
            GetProcAddressManual(GetModuleHandle(moduleHandle), proc) : 
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

BOOL WINAPI MyCreateProcessA(
    LPCSTR lpApplicationName, 
    LPSTR lpCommandLine, 
    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    BOOL bInheritHandles, 
    DWORD dwCreationFlags, 
    LPVOID lpEnvironment, 
    LPCSTR lpCurrentDirectory, 
    LPSTARTUPINFOA lpStartupInfo, 
    LPPROCESS_INFORMATION lpProcessInformation)
{
    std::stringstream logMessage;
    logMessage << "CreateProcessA: " << lpApplicationName << " " << lpCommandLine;
    Log(logMessage.str());

    return CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

BOOL WINAPI MyCreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    std::wstringstream logMessage;
    logMessage << L"CreateProcessW: " << lpApplicationName << L" " << lpCommandLine;
    LogW(logMessage.str());
    
    if (!CreateProcessW(
            lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, 
            bInheritHandles, CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation)
    ) {
        Log("CreateProcessW failed");
        return FALSE;
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
            if (0 != RhInjectLibrary(
                lpProcessInformation->dwProcessId, 0, EASYHOOK_INJECT_DEFAULT,
                dllPath, dllPath, NULL, 0)
                ) {
                std::wstringstream logMessage;
                logMessage << L"RhInjectLibrary failed for " << dllPath;
                LogW(logMessage.str());
            }
        } while (FindNextFile(hFind, &findFileData) != 0);

        FindClose(hFind);
    }

    //// Suspend the process using 'C:\Users\lynch\Documents\PSTools\pssuspend.exe' by lpProcessInformation->dwProcessId

    //// lpProcessInformation->dwProcessId to string
    //std::stringstream ss;
    //ss << lpProcessInformation->dwProcessId;
    //std::string str = ss.str();

    //ShellExecuteA(NULL, "open", "C:\\Users\\lynch\\Documents\\PSTools\\pssuspend.exe", str.c_str(), NULL, SW_HIDE);
    
    //ShellExecuteA(NULL, "open", "C:\\Users\\lynch\\Documents\\PSTools\\pssuspend.exe", "lp2dx9.exe", NULL, SW_HIDE);

    /*int i = 0;
    do {
        ShellExecuteA(NULL, "open", "C:\\Users\\lynch\\Documents\\PSTools\\pssuspend.exe", "lp2dx9.exe", NULL, SW_HIDE);
        Sleep(1);
        i++;
    } while (i < 10);*/

    //Sleep(10000);

    ResumeThread(lpProcessInformation->hThread);
    WaitForSingleObject(lpProcessInformation->hProcess, 5000);

    return TRUE;
}

LHOOK_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("lhook.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): LauncherHook.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    InstallHook("kernel32", "CreateProcessW", MyCreateProcessW, true);
}
