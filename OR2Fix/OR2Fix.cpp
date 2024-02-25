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

char* targetIPAddress = "";
const int MAX_PROCESS_ID_LENGTH = 10;
const char PADDING_CHAR = '#';
std::map<DWORD, std::pair<std::string, std::string>> pidAddressMap;

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

        modified_addr.sin_addr.s_addr = inet_addr(targetIPAddress);

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

int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr_in* to, int tolen)
{
    std::stringstream logMessage;
    logMessage << "MySendTo Virtual Address: " << inet_ntoa(reinterpret_cast<const sockaddr_in*>(to)->sin_addr);
    Log(logMessage.str());

    // Get the current process ID
    DWORD processId = GetCurrentProcessId();

    // Convert the process ID to a string
    std::string processIdStr = std::to_string(processId);

    // Pad the processIdStr to the maximum length
    processIdStr.resize(MAX_PROCESS_ID_LENGTH - 1, PADDING_CHAR);

    // Prepend the padded processIdStr to the buffer
    std::string modifiedBuf = processIdStr + std::string(buf, buf + len);

    // Create a copy of the to address
    struct sockaddr_in toCopy = *to;

    // Convert the to address to a string
    char* toAddress = inet_ntoa(toCopy.sin_addr);

    // Search the ipAddressMap for a value that matches the to address
    for (const auto& pair : pidAddressMap)
    {
        if (pair.second.second == toAddress)
        {
            // If a match is found, replace the to address with the corresponding key
            toCopy.sin_addr.s_addr = inet_addr(pair.second.first.c_str());

            std::stringstream logMessage;
            logMessage << "MySendTo Real Address: " << inet_ntoa(reinterpret_cast<const sockaddr_in*>(to)->sin_addr);
            Log(logMessage.str());
            break;
        }
    }

    // Call the original sendto function with the possibly modified to address
    return sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (struct sockaddr*)&toCopy, tolen);
}

std::string GenerateRandomIPAddress()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254); // Avoid 0 and 255 in each octet

    std::stringstream ss;
    ss << dis(gen) << "." << dis(gen) << "." << dis(gen) << "." << dis(gen);

    return ss.str();
}

int WINAPI MyRecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr_in* from, int* fromlen)
{
    // Create a buffer to hold the data received from recvfrom
    char* recvBuf = new char[len + MAX_PROCESS_ID_LENGTH];

    // Call the original recvfrom function
    int result = recvfrom(s, recvBuf, len + MAX_PROCESS_ID_LENGTH - 1, flags, (struct sockaddr*)from, fromlen);

    // Check if the call was successful
    if (result > 0)
    {
        std::string realIPAddress = inet_ntoa(reinterpret_cast<sockaddr_in*>(from)->sin_addr);

        // Extract the process ID from the start of the buffer
        std::string processIdStr(recvBuf, recvBuf + MAX_PROCESS_ID_LENGTH - 1);

        // Remove the padding from the process ID
        processIdStr.erase(processIdStr.find(PADDING_CHAR));

        // Convert the process ID string to an integer
        DWORD processId = std::stoul(processIdStr);

        // Check if the process ID is in the map
        if (pidAddressMap.find(processId) == pidAddressMap.end())
        {
            // If it's not, generate a new virtual IP address and add it to the map
            std::string virtualIPAddress = GenerateRandomIPAddress();
            pidAddressMap[processId].first = realIPAddress;
            pidAddressMap[processId].second = virtualIPAddress;
        }

        // Get the virtual IP address from the map
        std::string virtualIPAddress = pidAddressMap[processId].second;

        Log("MyRecvFrom Process ID: " + processIdStr);
        Log("MyRecvFrom realIPAddress: " + realIPAddress);
        Log("MyRecvFrom virtualIPAddress: " + virtualIPAddress);

        // Assign the virtual IP address to the returned from addr field
        from->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());

        // Remove the process ID from the start of the buffer
        memmove(buf, recvBuf + MAX_PROCESS_ID_LENGTH - 1, result - (MAX_PROCESS_ID_LENGTH - 1));

        // Update the result to reflect the new length of the buffer
        result -= (MAX_PROCESS_ID_LENGTH - 1);
    }

    // Clean up the recvBuf
    delete[] recvBuf;

    // Return the result
    return result;
}

int WSAAPI MyWSAIoctl(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    // Call the original function
    int result = WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);

    if (dwIoControlCode == SIO_GET_INTERFACE_LIST && result == 0)
    {
        // Cast the output buffer to INTERFACE_INFO structure
        INTERFACE_INFO* pInterfaceInfo = (INTERFACE_INFO*)lpvOutBuffer;

        // Clear the structure
        memset(pInterfaceInfo, 0, sizeof(INTERFACE_INFO));

        // Set the IP address to targetIPAddress
        pInterfaceInfo->iiAddress.AddressIn.sin_addr.s_addr = inet_addr(targetIPAddress);

        // Set the number of bytes returned
        *lpcbBytesReturned = sizeof(INTERFACE_INFO);

        Log("SIO_GET_INTERFACE_LIST overwritten with address: " + std::string(targetIPAddress));
    }

    return result;
}

bool GetBestLocalIPAddress()
{
    // Allocate memory for the adapter info structures
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);

    // Call GetAdaptersInfo to fill the pAdapterInfo structure
    DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);

    // If the buffer was too small, allocate a larger one and call GetAdaptersInfo again
    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    }

    // If the call was successful, iterate through the list of adapters
    if (dwRetVal == NO_ERROR)
    {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            // Iterate through the list of IP addresses for this adapter
            IP_ADDR_STRING* pIpAddressList = &pAdapter->IpAddressList;
            while (pIpAddressList)
            {
                // Check if the IP address starts with "192"
                if (strncmp(pIpAddressList->IpAddress.String, "192", 3) == 0)
                {
                    // If it does, return it
                    targetIPAddress = new char[strlen(pIpAddressList->IpAddress.String) + 1]; // +1 for the null-terminator
                    strcpy(targetIPAddress, pIpAddressList->IpAddress.String);

                    free(pAdapterInfo);
                    return true;
                }

                // Move to the next IP address
                pIpAddressList = pIpAddressList->Next;
            }

            // Move to the next adapter
            pAdapter = pAdapter->Next;
        }
    }

    // If no suitable IP address was found, return an empty string
    free(pAdapterInfo);
    return false;
}

OR2FIX_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("or2fix.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): OR2Fix.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    GetBestLocalIPAddress();
    if (targetIPAddress[0] != '\0') {
        Log("TargetIPAddress: ");
        Log(targetIPAddress);
        pidAddressMap[GetCurrentProcessId()] = std::make_pair(targetIPAddress, targetIPAddress);
    }
    else {
        Log("TargetIPAddress not found");
    }

    InstallHook("ws2_32.dll", "bind", MyBind);
    Log("Installed bind hook");

    InstallHook("ws2_32.dll", "WSAIoctl", MyWSAIoctl);
    Log("Installed WSAIoctl hook");

    InstallHook("ws2_32.dll", "sendto", MySendTo);
    Log("Installed sendto hook");

    InstallHook("ws2_32.dll", "recvfrom", MyRecvFrom);
    Log("Installed recvfrom hook");

}
