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

HANDLE hLogFile = INVALID_HANDLE_VALUE;
TCHAR tempString[2048];
WCHAR tempStringW[2048];

char* targetIPAddress = "";

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

BOOL WINAPI MyGetCurrentHwProfileW(LPHW_PROFILE_INFOW lpHwProfileInfo)
{
	BOOL result = GetCurrentHwProfileW(lpHwProfileInfo);

	std::stringstream logMessage;
	logMessage << "SHOOK: GetCurrentHwProfileW called";
	Log(logMessage.str());

	return result;
}

BOOL WINAPI MyGetVolumeInformationA(
	LPCSTR lpRootPathName,
	LPSTR lpVolumeNameBuffer,
	DWORD nVolumeNameSize,
	LPDWORD lpVolumeSerialNumber,
	LPDWORD lpMaximumComponentLength,
	LPDWORD lpFileSystemFlags,
	LPSTR lpFileSystemNameBuffer,
	DWORD nFileSystemNameSize)
{
	/*BOOL result = GetVolumeInformationA(
		lpRootPathName,
		lpVolumeNameBuffer,
		nVolumeNameSize,
		lpVolumeSerialNumber,
		lpMaximumComponentLength,
		lpFileSystemFlags,
		lpFileSystemNameBuffer,
		nFileSystemNameSize);*/

	// generate a random integer and assign it to lpVolumeSerialNumber 
	srand(static_cast<unsigned int>(time(NULL)));
	*lpVolumeSerialNumber = rand() % 1000;

	// Log lpVolumeSerialNumber
	std::stringstream logMessage;
	logMessage << "SHOOK: GetVolumeInformationA called, lpVolumeSerialNumber: " << *lpVolumeSerialNumber;
	Log(logMessage.str());

	return 1;
}

int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
{
    Log("MyBind");

    if (name->sa_family == AF_INET)
    {
        struct sockaddr_in modified_addr = {};
        memcpy(&modified_addr, name, sizeof(modified_addr));

        modified_addr.sin_addr.s_addr = inet_addr(targetIPAddress);

        bind(s, (struct sockaddr*)&modified_addr, sizeof(modified_addr));

        return 0;
    }

    return bind(s, name, namelen);
}

BOOL WINAPI MyGetVolumeInformationW(
	LPCWSTR lpRootPathName,
	LPWSTR lpVolumeNameBuffer,
	DWORD nVolumeNameSize,
	LPDWORD lpVolumeSerialNumber,
	LPDWORD lpMaximumComponentLength,
	LPDWORD lpFileSystemFlags,
	LPWSTR lpFileSystemNameBuffer,
	DWORD nFileSystemNameSize)
{
	BOOL result = GetVolumeInformationW(
		lpRootPathName,
		lpVolumeNameBuffer,
		nVolumeNameSize,
		lpVolumeSerialNumber,
		lpMaximumComponentLength,
		lpFileSystemFlags,
		lpFileSystemNameBuffer,
		nFileSystemNameSize);

	std::stringstream logMessage;
	logMessage << "SHOOK: GetVolumeInformationW called";
	Log(logMessage.str());

	return result;
}

void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("bo3fix.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): BO3Fix.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    std::stringstream logMessage;

	std::string iniFilePath = "";
	TCHAR executablePath[MAX_PATH];
	if (GetCurrentDirectory(MAX_PATH, executablePath)) {
		iniFilePath = std::string(executablePath) + "\\plugins\\bo3fix.ini";
	}

	char targetIPBuffer[256];
	GetPrivateProfileString("Settings", "TargetIPAddress", "", targetIPBuffer, sizeof(targetIPBuffer), iniFilePath.c_str());

	targetIPAddress = new char[strlen(targetIPBuffer) + 1];
	std::strcpy(targetIPAddress, targetIPBuffer);

	Log("TargetIPAddress");
	Log(targetIPBuffer);

	// Hook winsock2 bind
	InstallHook("ws2_32.dll", "bind", MyBind);
	InstallHook("kernel32.dll", "GetVolumeInformationA", MyGetVolumeInformationA);
}
