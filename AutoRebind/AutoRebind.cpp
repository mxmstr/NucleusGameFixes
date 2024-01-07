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
using namespace std;

unsigned char macAddress[6];
char* targetIPAddress = "";
char* hostIPAddress = "";
USHORT hostPort = 12345;
char targetHostname[18];
char originalHostname[18];
bool joined = false;

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

int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
{
    Log("MyBind");

    // Ensure the address is IPv4 (AF_INET)
    if (name->sa_family == AF_INET)
    {
        struct sockaddr_in modified_addr = {};
        memcpy(&modified_addr, name, sizeof(modified_addr));

        // Set IP to 0.0.0.0
        modified_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Check the original port
        USHORT originalPort = ntohs(modified_addr.sin_port);
        // Find the first available port starting from originalPort
        for (USHORT port = originalPort; port < 65535; ++port)
        {
            modified_addr.sin_port = htons(port);
            if (bind(s, (struct sockaddr*)&modified_addr, sizeof(modified_addr)) == 0)
            {
                // Successfully bound
                return 0;
            }
        }

        // If no ports are available, return error
        return SOCKET_ERROR;
    }

    // For other cases, proceed as normal
    return bind(s, name, namelen);
}

AUTOREBIND_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("injector.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): AutoRebind.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    InstallHook("ws2_32.dll", "bind", MyBind);
    Log("Installed Bind hook");

}
