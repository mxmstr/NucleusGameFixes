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
#include <setupapi.h>
//#include <cstdio>
//#include <ntdef.h>
//#include <winternl.h>
using namespace std;

SOCKET broadcastSocket = INVALID_SOCKET;
unsigned char macAddress[6];
char* targetIPAddress = "";
char* hostIPAddress = "";
std::string randomIPAddress = "";
USHORT hostPort = 12345;
char targetHostname[18];
char originalHostname[18];
bool joined = false;
unsigned char idBytes[1] = { 0 };

HANDLE hLogFile = INVALID_HANDLE_VALUE;
TCHAR tempString[2048];
WCHAR tempStringW[2048];

LPCSTR ConvertWideToNarrow(LPCWSTR wideString) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
    char* narrowBuffer = new char[bufferSize];
    WideCharToMultiByte(CP_UTF8, 0, wideString, -1, narrowBuffer, bufferSize, NULL, NULL);
    LPCSTR narrowString = narrowBuffer;
    delete[] narrowBuffer;
    return narrowString;
}

std::wstring ConvertToWideString(const char* str)
{
    if (str == nullptr)
        return std::wstring(); // Return an empty wstring if the input is NULL

    // Calculate the size of the destination wide string
    int wcharCount = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if (wcharCount == 0)
        return std::wstring();

    // Allocate memory for the wide string
    std::wstring wideStr(wcharCount, L'\0');

    // Convert the ANSI string to wide string
    if (MultiByteToWideChar(CP_ACP, 0, str, -1, &wideStr[0], wcharCount) == 0)
        return std::wstring();

    return wideStr;
}

// Function to compare PWCHAR and char*
int ComparePWCharAndChar(PWCHAR pwcharStr, const char* charStr) {
    std::wstring convertedCharStr = ConvertToWideString(charStr);

    // Now use wcscmp to compare the strings
    return wcscmp(pwcharStr, convertedCharStr.c_str());
}

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

std::string pwchar_to_str(PWCHAR pwchar)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, pwchar, -1, NULL, 0, NULL, NULL);
    char* buffer = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, pwchar, -1, buffer, len, NULL, NULL);
    std::string str(buffer);
    delete[] buffer;
    return str;
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
        Log("SHOOK: Error installing " + string(proc) + " hook, error msg: " + pwchar_to_str(RtlGetLastErrorString()));
    }
    else
    {
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hHook);
    }

    return result;
}

// Function to check if an IP address matches the target IPv4 address
bool IsTargetIPAddress(const char* ipAddress, const char* targetIPAddress)
{
    return strcmp(ipAddress, targetIPAddress) == 0;
}

// Example function to fill out a custom adapter info
void FillCustomAdapterInfo(IP_ADAPTER_INFO& adapterInfo)
{
    // Clear the structure
    memset(&adapterInfo, 0, sizeof(IP_ADAPTER_INFO));

    // Fill in adapter name
    strcpy_s(adapterInfo.AdapterName, "CustomAdapter");

    // Fill in a description
    strcpy_s(adapterInfo.Description, "Custom Virtual Network Adapter");

    // Set a custom MAC address
    adapterInfo.AddressLength = 6;
    memcpy(adapterInfo.Address, macAddress, 6);
    /*adapterInfo.Address[0] = 0x00;
    adapterInfo.Address[1] = 0x1A;
    adapterInfo.Address[2] = 0x2B;
    adapterInfo.Address[3] = 0x3C;
    adapterInfo.Address[4] = 0x4D;
    adapterInfo.Address[5] = 0x5E;*/

    // Set IP address information (example IP)
    IP_ADDR_STRING ipAddr;
    ipAddr.Next = NULL;
    strcpy_s(ipAddr.IpAddress.String, "192.168.0.123");
    strcpy_s(ipAddr.IpMask.String, "255.255.255.0");
    adapterInfo.CurrentIpAddress = &ipAddr;

    // Set other fields like type, DHCP enabled, etc., as needed
    adapterInfo.Type = MIB_IF_TYPE_ETHERNET;
    adapterInfo.DhcpEnabled = 0; // or 1 if DHCP is enabled

    // Add more fields as needed
}

// Hooked GetAdaptersInfo function
DWORD WINAPI MyGetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
{
    std::stringstream logMessage;

    Log("MyGetAdaptersInfo");

    // Call the original GetAdaptersInfo function
    DWORD result = GetAdaptersInfo(pAdapterInfo, pOutBufLen);

    return result;

    //// Check if the call was successful
    //if (result == ERROR_SUCCESS)
    //{
    //    // Loop through the list of adapters
    //    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    //    PIP_ADAPTER_INFO pLastAdapter = nullptr; // Keep track of the last adapter

    //    while (pAdapter)
    //    {
    //        // Loop through the list of IP addresses for the current adapter
    //        PIP_ADDR_STRING pIpAddress = &(pAdapter->IpAddressList);
    //        while (pIpAddress)
    //        {
    //            // Check if the current IP address matches the target IP address
    //            if (IsTargetIPAddress(pIpAddress->IpAddress.String, targetIPAddress))
    //            {
    //                // Log or process the information for the desired adapter
    //                logMessage << "Adapter Name: " << pAdapter->AdapterName << std::endl;
    //                logMessage << "IPv4 Address: " << pIpAddress->IpAddress.String << std::endl;

    //                // Additional information can be logged or processed here

    //                // Set Next to NULL for the last adapter
    //                pAdapter->Next = nullptr;

    //                // Break out of the loop since we found the desired adapter
    //                break;
    //            }

    //            // Move to the next IP address in the list
    //            pIpAddress = pIpAddress->Next;
    //        }

    //        // Keep track of the last adapter in the list
    //        pLastAdapter = pAdapter;

    //        // Move to the next adapter in the list
    //        pAdapter = pAdapter->Next;
    //    }

    //    // Set Next to NULL for the last adapter in the list
    //    if (pLastAdapter)
    //    {
    //        pLastAdapter->Next = nullptr;
    //    }
    //}
    //else
    //{
    //    // Log an error message if the GetAdaptersInfo call fails
    //    logMessage << "Error calling GetAdaptersInfo: " << result << std::endl;
    //}

    //Log(logMessage.str());

    IP_ADAPTER_INFO customAdapterInfo;
    FillCustomAdapterInfo(customAdapterInfo);

    // Check if the buffer is large enough
    if (*pOutBufLen < sizeof(IP_ADAPTER_INFO))
    {
        *pOutBufLen = sizeof(IP_ADAPTER_INFO);
        return ERROR_BUFFER_OVERFLOW;
    }

    // Copy the custom adapter info to pAdapterInfo
    if (pAdapterInfo != NULL)
    {
        memcpy(pAdapterInfo, &customAdapterInfo, sizeof(IP_ADAPTER_INFO));
    }

    // Update the buffer length
    *pOutBufLen = sizeof(IP_ADAPTER_INFO);

    // Return success
    return ERROR_SUCCESS;
}

// Hooked GetAdaptersAddresses function
DWORD WINAPI HookedGetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved, PIP_ADAPTER_ADDRESSES pAdapterAddresses, PULONG SizePointer)
{
    Log("HookedGetAdaptersAddresses");
    return GetAdaptersAddresses(Family, Flags, Reserved, pAdapterAddresses, SizePointer);
}

// Hooked GetIfTable function
DWORD WINAPI HookedGetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder)
{
    Log("HookedGetIfTable");
    return GetIfTable(pIfTable, pdwSize, bOrder);
}

// Hooked GetIfEntry function
DWORD WINAPI HookedGetIfEntry(PMIB_IFROW pIfRow)
{
    Log("HookedGetIfEntry");
    return GetIfEntry(pIfRow);
}

// Hooked GetIpNetTable function
DWORD WINAPI HookedGetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize, BOOL bOrder)
{
    Log("HookedGetIpNetTable");
    return GetIpNetTable(pIpNetTable, pdwSize, bOrder);
}

DWORD WINAPI Hooked_GetIpNetEntry2(PMIB_IPNET_ROW2 pIpNetRow)
{
    Log("Hooked_GetIpNetEntry2");
    return GetIpNetEntry2(pIpNetRow);
}

DWORD WINAPI Hooked_GetIpNetTable2(ADDRESS_FAMILY Family, PMIB_IPNET_TABLE2* Table)
{
    Log("Hooked_GetIpNetTable2");
    return GetIpNetTable2(Family, Table);
}

DWORD WINAPI Hooked_GetIfEntry2(PMIB_IF_ROW2 pIfEntry)
{
    Log("Hooked_GetIfEntry2");
    return GetIfEntry2(pIfEntry);
}

DWORD WINAPI Hooked_GetIfEntry2Ex(MIB_IF_ENTRY_LEVEL InterfaceIndex, PMIB_IF_ROW2 pIfEntry)
{
    Log("Hooked_GetIfEntry2Ex");
    return GetIfEntry2Ex(InterfaceIndex, pIfEntry);
}

DWORD WINAPI Hooked_GetIfTable2(PMIB_IF_TABLE2* Table)
{
    Log("Hooked_GetIfTable2");
    return GetIfTable2(Table);
}

DWORD WINAPI Hooked_GetIfTable2Ex(MIB_IF_TABLE_LEVEL Family, PMIB_IF_TABLE2* Table)
{
    Log("Hooked_GetIfTable2Ex");
    return GetIfTable2Ex(Family, Table);
}

SOCKET WINAPI MySocket(int af, int type, int protocol)
{
    Log("MySocket");
    std::stringstream logMessage;
    logMessage << "af: " << af << std::endl;
    logMessage << "type: " << type << std::endl;
    logMessage << "protocol: " << protocol << std::endl;
    Log(logMessage.str());
    return socket(af, type, protocol);;
}

//int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
//{
//    Log("MyBind");
//
//    // Ensure the address is IPv4 (AF_INET)
//    if (name->sa_family == AF_INET)
//    {
//        sockaddr_in* sockaddrIPv4 = (sockaddr_in*)name;
//        char ipBuffer[INET_ADDRSTRLEN];
//        inet_ntop(AF_INET, &(sockaddrIPv4->sin_addr), ipBuffer, INET_ADDRSTRLEN);
//        USHORT port = ntohs(sockaddrIPv4->sin_port);
//
//        std::stringstream logMessage;
//        logMessage << "Bound IP Address: " << ipBuffer << std::endl;
//        logMessage << "Bound Port: " << port << std::endl;
//        Log(logMessage.str());
//
//
//        struct sockaddr_in modified_addr = {};
//        memcpy(&modified_addr, name, sizeof(modified_addr));
//
//        // Set IP to 0.0.0.0
//        modified_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//        //modified_addr.sin_addr.s_addr = inet_addr(targetIPAddress);
//
//        // Check the original port
//        USHORT originalPort = ntohs(modified_addr.sin_port);
//        // Find the first available port starting from originalPort
//        for (USHORT port = originalPort; port < 65535; ++port)
//        {
//            modified_addr.sin_port = htons(port);
//            if (bind(s, (struct sockaddr*)&modified_addr, sizeof(modified_addr)) == 0)
//            {
//                // Successfully bound
//                return 0;
//            }
//        }
//
//        // If no ports are available, return error
//        return SOCKET_ERROR;
//    }
//
//    // For other cases, proceed as normal
//    return bind(s, name, namelen);
//
//    // Extract IP address and port from the sockaddr structure
//    //if (name->sa_family == AF_INET)
//    //{
//    //    sockaddr_in* sockaddrIPv4 = (sockaddr_in*)name;
//    //    char ipBuffer[INET_ADDRSTRLEN];
//    //    inet_ntop(AF_INET, &(sockaddrIPv4->sin_addr), ipBuffer, INET_ADDRSTRLEN);
//    //    USHORT port = ntohs(sockaddrIPv4->sin_port);
//
//    //    std::stringstream logMessage;
//    //    logMessage << "Bound IP Address: " << ipBuffer << std::endl;
//    //    logMessage << "Bound Port: " << port << std::endl;
//    //    Log(logMessage.str());
//    //    
//    //    if (inet_pton(AF_INET, targetIPAddress, &(sockaddrIPv4->sin_addr)) != 1)
//    //    {
//    //        Log("Failed to convert IP address");
//    //        return SOCKET_ERROR;
//    //    }
//    //}
//    //else if (name->sa_family == AF_INET6)
//    //{
//    //    sockaddr_in6* sockaddrIPv6 = (sockaddr_in6*)name;
//    //    char ipBuffer[INET6_ADDRSTRLEN];
//    //    inet_ntop(AF_INET6, &(sockaddrIPv6->sin6_addr), ipBuffer, INET6_ADDRSTRLEN);
//    //    USHORT port = ntohs(sockaddrIPv6->sin6_port);
//
//    //    std::stringstream logMessage;
//    //    logMessage << "Bound IP Address: " << ipBuffer << std::endl;
//    //    logMessage << "Bound Port: " << port << std::endl;
//    //    Log(logMessage.str());
//    //}
//    //else
//    //{
//    //    // Handle other address families if needed
//    //}
//
//    //return bind(s, name, namelen);
//}

int WINAPI MyGetSockName(SOCKET s, struct sockaddr* name, int* namelen)
{
    std::stringstream logMessage;

    int result = getsockname(s, name, namelen);

    // Log information about the getsockname call
    logMessage << "getsockname intercepted!" << std::endl;
    logMessage << "Socket: " << s << std::endl;

    const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(name);
    char destAddrString[INET_ADDRSTRLEN];

    /*if (inet_ntop(AF_INET, addr, destAddrString, INET_ADDRSTRLEN) != nullptr)
    {
        logMessage << "Address: " << destAddrString << std::endl;
    }
    else
    {
        logMessage << "inet_ntop error " << std::endl;
    }*/

    //
    //

    //std::string baseIP = "192.168.0.";

    //// Initialize random seed
    //std::srand(std::time(0));

    //// Generate a random number from 0 to 999
    //int lastDigits = std::rand() % 1000;

    //// Combine the base IP with the random last digits
    //std::string randomizedIP = baseIP + std::to_string(lastDigits);

    //std::cout << "Randomized IP Address: " << randomizedIP << std::endl;


    if (inet_pton(AF_INET, randomIPAddress.c_str(), &(((struct sockaddr_in*)name)->sin_addr)) <= 0) {
        logMessage << "error" << result << std::endl;
    }
    
    //// Log information about the result and parameters
    //logMessage << "Result: " << result << std::endl;
    //
    if (result == 0) {
        // Log the information returned by getsockname
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)name)->sin_addr), ipAddress, INET_ADDRSTRLEN);
    
        logMessage << "Address: " << ipAddress << std::endl;
        logMessage << "Port: " << ntohs(((struct sockaddr_in*)name)->sin_port) << std::endl;
    }
    
    Log(logMessage.str());

    return result;
}

void printRawBytes(const char* data, std::size_t size) {
    std::stringstream logMessage;

    for (std::size_t i = 0; i < size; ++i) {
        // Print each byte as a two-digit hexadecimal number
        logMessage << std::hex << std::setw(8) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    logMessage << std::dec;  // Switch back to decimal mode

    Log(logMessage.str());

}

//int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
//{
//    Log("MySendTo1");
//    // Create a new sockaddr_in structure for the broadcast address
//    struct sockaddr_in broadcastAddr;
//    broadcastAddr.sin_family = AF_INET;
//    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
//
//    // Loop through the port range and send the packet to each port
//    int result = 0;
//    for (int port = 50000; port <= 70000; ++port) {
//        broadcastAddr.sin_port = htons((u_short)port);
//        result = sendto(s, buf, len, flags, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
//
//        if (result == SOCKET_ERROR) {
//            Log("Socket error");
//            break;
//        }
//    }
//    Log("MySendTo2");
//
//    return result;
//}

//int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
//{
//    // Log information about the sendto call
//    std::stringstream logMessage;
//    logMessage << "sendto intercepted!" << std::endl;
//    logMessage << "Socket: " << s << std::endl;
//    logMessage << "Length: " << len << std::endl;
//    logMessage << "Flags: " << flags << std::endl;
//
//    // Log information about the destination address
//    if (to != nullptr && tolen >= sizeof(sockaddr_in))
//    {
//        const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(to);
//
//        char destAddrString[INET_ADDRSTRLEN];
//        if (inet_ntop(AF_INET, &(addr->sin_addr), destAddrString, INET_ADDRSTRLEN) != nullptr)
//        {
//            logMessage << "MySendTo Address: " << destAddrString << std::endl;
//            logMessage << "MySendTo Port: " << ntohs(addr->sin_port) << std::endl;
//        }
//        else
//        {
//            logMessage << "inet_ntop error " << std::endl;
//        }
//    }
//
//    Log(logMessage.str());
//
//    printRawBytes(buf, len);
//    return sendto(s, buf, len, flags, to, tolen);
//}
//
//
//    //const sockaddr_in* localAddress = reinterpret_cast<const sockaddr_in*>(to);
//
//    //// Check for the specific byte sequence
//    //const unsigned char JoinQueryHeader[] = {
//    //    0x0000006e,0x00000066,0x00000073,0x00000070,
//    //    0x00000073,0x00000032,0x0000002d,0x00000070
//    //};
//
//    ///*const unsigned char ClientRequestFunction = 0x2c;
//    //const unsigned char ServerResponseFunction = 0x37;
//
//    //static bool clientStarted = false;
//    //static unsigned char originalBytes1[6] = { 0 };
//    //        logMessage << "MySendTo Address: " << destAddrString << std::endl;
//    //        logMessage << "MySendTo Port: " << ntohs(addr->sin_port) << std::endl;
//    //    }
//    //    else
//    //    {
//    //        logMessage << "inet_ntop error " << std::endl;
//    //    }
//    //}
//    //static unsigned char newBytes1[6] = { 0 };
//
//    //static bool serverStarted = false;
//    //static unsigned char originalBytes2[6] = { 0 };
//    //static unsigned char newBytes2[6] = { 0 };*/
//
//
//    ////if (clientStarted || serverStarted) {
//    ////    char* newBuffer = (char*)malloc(len);
//    ////    memcpy(newBuffer, buf, len);
//
//    ////    /*if (clientStarted) {
//    ////        for (int i = 0; i <= len - 6; ++i) {
//    ////            if (memcmp(newBuffer + i, originalBytes1, 6) == 0) {
//    ////                const unsigned char newBytes[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
//    ////                memcpy(newBuffer + i, newBytes, 6);
//    ////            }
//    ////        }
//    ////    }
//    ////    if (serverStarted) {
//    ////        for (int i = 0; i <= len - 6; ++i) {
//    ////            if (memcmp(newBuffer + i, originalBytes2, 6) == 0) {
//    ////                const unsigned char newBytes[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
//    ////                memcpy(newBuffer + i, newBytes, 6);
//    ////            }
//    ////        }
//    ////    }*/
//    ////    printRawBytes(newBuffer, len);
//    ////    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    ////    free(newBuffer);
//    ////    return result;
//    ////}
//
//
//    //if (len > sizeof(JoinQueryHeader) && std::memcmp(buf, JoinQueryHeader, sizeof(JoinQueryHeader)) == 0) {
//
//    //    
//    //    
//    //    char* newBuffer = (char*)malloc(len);
//    //    memcpy(newBuffer, buf, len);
//
//    //    
//
//    //    memcpy(newBuffer + 8, idBytes, 1);
//
//    //    printRawBytes(newBuffer, len);
//    //    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    //    free(newBuffer);
//
//    //    return result;
//
//    //    //unsigned char nextByte = buf[sizeof(JoinQueryHeader)];
//
//    //    //if (nextByte == ClientRequestFunction) {
//    //    //    Log("Client Request");
//
//    //    //    char* newBuffer = (char*)malloc(len);
//    //    //    memcpy(newBuffer, buf, len);
//
//    //    //    // Capture the original bytes
//    //    //    memcpy(originalBytes1, newBuffer + 68, 6);
//
//    //    //    // Generate random bytes
//    //    //    /*std::random_device rd;
//    //    //    std::mt19937 gen(rd());
//    //    //    std::uniform_int_distribution<> dis(0, 255);
//    //    //    for (int i = 0; i < 6; ++i) {
//    //    //        newBytes1[i] = static_cast<unsigned char>(dis(gen));
//    //    //    }*/
//
//    //    //    // Replace with new random bytes
//    //    //    //memcpy(newBuffer + 68, newBytes1, 6);
//    //    //    const unsigned char newBytes[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
//    //    //    //memcpy(newBuffer + 68, newBytes, 6);
//
//    //    //    clientStarted = true;
//
//    //    //    printRawBytes(newBuffer, len);
//    //    //    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    //    //    free(newBuffer);
//
//    //    //    return result;
//    //    //}
//    //    ////if (nextByte == ClientRequestFunction) {//&& strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
//    //    ////    Log("Client Request");
//
//    //    ////    char* newBuffer = (char*)malloc(len);
//    //    ////    memcpy(newBuffer, buf, len);
//
//    //    ////    /*const unsigned char newBytesA[] = { 0xffffff88 };
//    //    ////    std::memcpy(newBuffer + 56, newBytesA, sizeof(newBytesA));*/
//    //    ////    
//    //    ////    const unsigned char newBytesB[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
//    //    ////    std::memcpy(newBuffer + 68, newBytesB, sizeof(newBytesB));
//
//    //    ////    //////const unsigned char newBytes[] = { 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5, 0xa5, 0xa8 };
//    //    ////    //const unsigned char newBytes[] = { 0x00 };//, 0x00, 0x00, 0x00, 0x00, 0x00 };
//    //    ////    ////const unsigned char newBytes[] = { 0x55, 0xad, 0x36, 0xa5, 0xa5, 0xa8 };
//    //    ////    //std::memcpy(newBuffer + 68, newBytes, sizeof(newBytes));
//
//    //    ////    printRawBytes(newBuffer, len);
//    //    ////    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    //    ////    free(newBuffer);
//
//    //    ////    return result;
//    //    ////}
//    //    ////else if (nextByte == ServerResponseFunction) {
//    //    ////    Log("Server Response");
//    //    ////    // 78 -> 0x10
//    //    ////    // 90 -> 0000005a ffffffaa 0000006d 0000004b 0000004b 00000050
//
//    //    ////    char* newBuffer = (char*)malloc(len);
//    //    ////    memcpy(newBuffer, buf, len);
//
//    //    ////    /*const unsigned char newBytesA[] = { 0xffffffb7, 0xffffffd4, 0xffffffb8, 0xfffffffd, 0x00000050 };
//    //    ////    std::memcpy(newBuffer + 45, newBytesA, sizeof(newBytesA));*/
//
//    //    ////    /*const unsigned char newBytesA[] = { 0x10 };
//    //    ////    std::memcpy(newBuffer + 78, newBytesA, sizeof(newBytesA));*/
//
//    //    ////    const unsigned char newBytesB[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
//    //    ////    std::memcpy(newBuffer + 90, newBytesB, sizeof(newBytesB));
//
//    //    ////    printRawBytes(newBuffer, len);
//    //    ////    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    //    ////    free(newBuffer);
//
//    //    ////    return result;
//    //    ////}
//    //    //else if (nextByte == ServerResponseFunction) {
//    //    //    Log("Server Response");
//
//    //    //    char* newBuffer = (char*)malloc(len);
//    //    //    memcpy(newBuffer, buf, len);
//
//    //    //    // Capture the original bytes
//    //    //    memcpy(originalBytes2, newBuffer + 90, 6);
//
//    //    //    // Generate random bytes
//    //    //    /*std::random_device rd;
//    //    //    std::mt19937 gen(rd());
//    //    //    std::uniform_int_distribution<> dis(0, 255);
//    //    //    for (int i = 0; i < 6; ++i) {
//    //    //        newBytes2[i] = static_cast<unsigned char>(dis(gen));
//    //    //    }*/
//
//    //    //    // Replace with new random bytes
//    //    //    //memcpy(newBuffer + 90, newBytes2, 6);
//    //    //    const unsigned char newBytes[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
//    //    //    //std::memcpy(newBuffer + 90, newBytes, 6);
//
//    //    //    serverStarted = true;
//
//    //    //    printRawBytes(newBuffer, len);
//    //    //    int result = sendto(s, newBuffer, len, flags, to, tolen);
//    //    //    free(newBuffer);
//
//    //    //    return result;
//    //    //}
//    //}
//    ////else if (len > sizeof(JoinConfirmHeader) && std::memcmp(buf, JoinConfirmHeader, sizeof(JoinConfirmHeader)) == 0) {
//
//    ////    unsigned char nextByte = buf[sizeof(JoinQueryHeader)];
//
//    ////    if (nextByte == ClientRequestFunction) {//&& strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
//    ////        Log("Client Request");
//
//    ////        char* newBuffer = (char*)malloc(len);
//    ////        memcpy(newBuffer, buf, len);
//
//    ////        const unsigned char newBytesB[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
//    ////        std::memcpy(newBuffer + 39, newBytesB, sizeof(newBytesB));
//
//    ////        printRawBytes(newBuffer, len);
//    ////        int result = sendto(s, newBuffer, len, flags, to, tolen);
//    ////        free(newBuffer);
//
//    ////        return result;
//    ////    }
//    ////}
//    ///*else {
//
//    //    char* newBuffer = (char*)malloc(len);
//    //    memcpy(newBuffer, buf, len);
//
//    //    for (int i = 0; i <= len - 6; ++i) {
//    //        if (memcmp(newBuffer + i, originalBytes2, 6) == 0) {
//    //            memcpy(newBuffer + i, newBytes2, 6);
//    //        }
//    //    }
//    //}*/
//
//    //printRawBytes(buf, len);
//    //return sendto(s, buf, len, flags, to, tolen);
//}

// Function to convert a hexadecimal string to an integer
unsigned int hexToUInt(const std::string& hexStr) {
    std::stringstream ss;
    ss << std::hex << hexStr;
    unsigned int result;
    ss >> result;
    return result;
}

// Function to convert a string of hexadecimal values to a char* buffer
void hexStringToCharArray(const std::string& hexString, char* buffer) {
    std::vector<std::string> hexValues;

    // Split the input string into individual hexadecimal values
    for (size_t i = 0; i < hexString.length(); i += 8) {
        hexValues.push_back(hexString.substr(i, 8));
    }

    // Convert each hexadecimal value to an integer and store in the buffer
    for (size_t i = 0; i < hexValues.size(); ++i) {
        unsigned int intValue = hexToUInt(hexValues[i]);
        buffer[i] = static_cast<char>(intValue);
    }
}

LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {

    std::stringstream logMessage;
    Log("VectoredExceptionHandler1");

    if (false){//pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) {
        // Your callback logic here
        Log("VectoredExceptionHandler2");

        // Remove the hardware breakpoint
        CONTEXT context = { 0 };
        HANDLE hThread = GetCurrentThread();
        logMessage << "GetCurrentThread " << hThread << endl;
        GetThreadContext(hThread, &context);

        // Get the current thread context
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        // Clear the debug register used for the breakpoint (e.g., DR0)
        context.Dr0 = 0x0; // Assuming DR0 was used
        context.Dr7 &= ~0x1; // Clear the L0 flag in DR7 to disable the breakpoint

        // Apply the new context to the thread
        SetThreadContext(hThread, &context);
        Log(logMessage.str());

        return EXCEPTION_CONTINUE_EXECUTION; // This will resume execution after handling the exception
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void SetHardwareBreakpoint(void* address) {

    std::stringstream logMessage;

    CONTEXT context = { 0 };
    HANDLE hThread = GetCurrentThread();
    logMessage << "GetCurrentThread " << hThread << endl;

    Log(logMessage.str());

    // Get the current thread context
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(hThread, &context);

    // Set the address in one of the debug registers (e.g., DR0)
    context.Dr0 = (DWORD_PTR)address;
    context.Dr7 |= 0x1; // Set the L0 (local breakpoint 0) flag in DR7

    // Apply the new context to the thread
    SetThreadContext(hThread, &context);
}

//int WINAPI MyRecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
//{
//    std::stringstream logMessage;
//
//    int result = recvfrom(s, buf, len, flags, from, fromlen);
//
//    sockaddr_in* localAddress = reinterpret_cast<sockaddr_in*>(from);
//
//    /*if (strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) == 0)
//        return WSAETIMEDOUT;*/
//
//    logMessage << "MyRecvFrom Local Address: " << inet_ntoa(localAddress->sin_addr) << std::endl;
//    logMessage << "MyRecvFrom Local Port: " << ntohs(localAddress->sin_port) << std::endl;
//    logMessage << "MyRecvFrom Family: " << ntohs(localAddress->sin_family) << std::endl;
//    logMessage << "MyRecvFrom: socket - " << s << std::endl;
//    printRawBytes(buf, len);
//    logMessage << "MyRecvFrom: Length - " << len << std::endl;
//    logMessage << "MyRecvFrom: Flags - " << flags << std::endl;
//    logMessage << "MyRecvFrom: From length - " << fromlen << std::endl;
//
//
//    // Check for the specific byte sequence
//    //const unsigned char JoinQueryHeader[] = {
//    //    0x00, 0x00, 0x00, 0x00, 0x6a, 0xb0, 0xc0, 0xf4, 0x00, 0x00, 0x00, 0x01,
//    //    0x00, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x77, 0xc9, 0x00, 0x00, 0x00, 0x03, 
//    //    0x00, 0x00, 0x00, 0x03, 0x00
//    //};
//    //
//    //const unsigned char ClientRequestFunction = 0x2c;
//    //const unsigned char ServerResponseFunction = 0x37;
//    //
//    //if (result > sizeof(JoinQueryHeader) && std::memcmp(buf, JoinQueryHeader, sizeof(JoinQueryHeader)) == 0) {
//    //
//    //    // Check the next byte for "Join Request" or "Join Response"
//    //    unsigned char nextByte = buf[sizeof(JoinQueryHeader)];
//    //
//    //    if (nextByte == ClientRequestFunction && strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
//    //        logMessage << "Client Request" << std::endl;
//    //
//    //        const unsigned char newBytes[] = { 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5, 0xa5, 0xa8 };
//    //        std::memcpy(buf + 66, newBytes, sizeof(newBytes));
//    //
//    //        //std::memset(buf + 89, 0, len - 89);
//    //
//    //        //const unsigned char newBytes[] = { 0x26, 0x05, 0x40, 0x00, 0x88, 0x60, 0x14 };
//    //        //std::memcpy(buf + 52, newBytes, sizeof(newBytes));
//    //
//    //        /*const unsigned char newBytes[] = {
//    //            0x00, 0x00, 0x00, 0x00, 0x6a, 0xb0, 0xc0, 0xf4, 0x00, 0x00, 0x00, 0x01,
//    //            0x00, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x77, 0xc9, 0x00, 0x00, 0x00, 0x03,
//    //            0x00, 0x00, 0x00, 0x03, 0x00, 0x2c, 0x9f, 0x93, 0xa8, 0x58, 0x55, 0xff,
//    //            0x9a, 0xac, 0x30, 0x3d, 0x01, 0xaf, 0x24, 0x3c, 0x50, 0xf0, 0x35, 0x22,
//    //            0xe6, 0xbf, 0x9c, 0xb8, 0x26, 0x05, 0x40, 0x00, 0x88, 0x60, 0x14, 0xb8,
//    //            0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5,
//    //            0xa5, 0xa8, 0x18, 0x05, 0xe0, 0x00, 0x12, 0xec, 0x50, 0x60, 0x52, 0xee,
//    //            0xd6, 0x50, 0x5a, 0x52, 0xf0, 0x70, 0x6b, 0xfe, 0xf6, 0x7f, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xcc, 0x4c, 0x3f, 0x0b,
//    //            0xd7, 0xa3, 0x3e, 0x97, 0x00, 0x00, 0x00, 0x10, 0xa5, 0xb0, 0xf3, 0xd5,
//    //            0x01, 0x00, 0x00, 0x30, 0xb8, 0x30, 0x84, 0xd5, 0x01, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd5, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0xf3, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0xc8, 0x9e, 0x6b, 0xfe, 0xf6, 0x7f, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//    //            0x00, 0x00, 0x00, 0x00
//    //            };*/
//    //
//    //        //buf = new char[sizeof(newBytes)];
//    //        //std::memcpy(buf, newBytes, sizeof(newBytes));
//    //    }
//    //    else if (nextByte == ServerResponseFunction) {
//    //        logMessage << "Server Response" << std::endl;
//    //    }
//    //}
//
//
//    
//    // Modify the instruction
//    //DWORD oldProtect;
//    //BYTE* instructionAddress = (BYTE*)0x00007FF62FEE5BAF; // The address to modify
//
//    //// Change memory protection to write
//    //if (VirtualProtect(instructionAddress, 2, PAGE_EXECUTE_READWRITE, &oldProtect)) {
//    //    instructionAddress[0] = 0xEB; // Opcode for JMP (short jump)
//    //    // Note: The operand (jump address) remains unchanged
//
//    //    // Restore the original memory protection
//    //    VirtualProtect(instructionAddress, 2, oldProtect, &oldProtect);
//    //}
//    //else {
//    //    logMessage << "Failed to change memory protection." << std::endl;
//    //}
//
//
//    // Check if the call was successful and the buffer length is 0
//    //if (result != SOCKET_ERROR && from != nullptr && from->sa_family == AF_INET)
//    //{
//    //    Log("Success");
//    //    sockaddr_in* senderAddr = reinterpret_cast<sockaddr_in*>(from);
//
//    //    // Check if the sender's IPv4 address matches the specified address
//    //    char senderIP[INET_ADDRSTRLEN];
//    //    inet_ntop(AF_INET, &(senderAddr->sin_addr), senderIP, INET_ADDRSTRLEN);
//    //    inet_pton(AF_INET, "192.168.1.2", &(senderAddr->sin_addr));
//    //    senderAddr->sin_port = htons(12345);
//
//    //    std::string hexString = "00000000 00000000 00000000 00000000 0000006a ffffffb0 ffffffc0 fffffff4 00000000 00000000 00000000 00000001 00000000 00000000 00000000 0000000b 00000000 00000002 00000077 ffffffc9 00000000 00000000 00000000 00000003 00000000 00000000 00000000 00000003 00000000 0000002c ffffff9f";
//    //    hexString.erase(std::remove(hexString.begin(), hexString.end(), ' '), hexString.end());
//    //    size_t bufferSize = hexString.length() / 2;
//    //    char* buffer = new char[bufferSize];
//    //    hexStringToCharArray(hexString, buffer);
//    //    *buf = *buffer;
//
//    //    logMessage << "Set content" << std::endl;
//
//    //    if (strcmp(senderIP, hostIPAddress) == 0)
//    //    {
//    //        // Modify the sender's IPv4 address to 192.168.0.22
//    //        /*inet_pton(AF_INET, "192.168.0.22", &(senderAddr->sin_addr));
//    //        senderAddr->sin_port = htons(56515);
//    //        *fromlen = 0x00000063E89FF890;*/
//    //        std::string hexString = "00000000 00000000 00000000 00000000 0000006a ffffffb0 ffffffc0 fffffff4 00000000 00000000 00000000 00000001 00000000 00000000 00000000 0000000b 00000000 00000002 00000077 ffffffc9 00000000 00000000 00000000 00000003 00000000 00000000 00000000 00000003 00000000 00000037 ffffffc7 0000000d ffffff88 fffffff7 0000003b 0000005b 00000002 00000028 ffffffab ffffffa2 ffffffa9 00000060 00000000 00000000 00000009 00000075 00000060 ffffff9b fffffffd ffffff85 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ffffff8f ffffffff ffffffff ffffffff fffffffc 0000003c 00000021 00000000 00000008 0000001f ffffffe6 ffffffa4 0000005c ffffffd7 fffffff3 ffffff97 00000004 ffffffc0 ffffffa8 00000000 0000000f 0000000c 00000002 ffffff97 00000004 00000000 00000000 00000000 00000000 00000000 00000000 ffffff86 00000033 00000020 ffffffdd 00000074 00000001 00000004 ffffff83 00000000 ffffffbc 00000000 00000002 0000005d ffffff8a 0000000c 0000000a 0000005d ffffffda ffffffca 0000000b 0000004a 0000005e 00000000";
//    //        hexString.erase(std::remove(hexString.begin(), hexString.end(), ' '), hexString.end());
//    //        size_t bufferSize = hexString.length() / 2;
//    //        char* buffer = new char[bufferSize];
//    //        hexStringToCharArray(hexString, buffer);
//
//    //        logMessage << "Force send" << std::endl;
//    //        MySendTo(s, buffer, bufferSize, flags, from, *fromlen);
//    //    }
//    //}
//
//    /*localAddress = reinterpret_cast<sockaddr_in*>(from);
//
//    logMessage << "MyRecvFrom Local Address 2: " << inet_ntoa(localAddress->sin_addr) << std::endl;
//    logMessage << "MyRecvFrom Local Port 2: " << ntohs(localAddress->sin_port) << std::endl;
//    logMessage << "MyRecvFrom Family 2: " << ntohs(localAddress->sin_family) << std::endl;
//    logMessage << "MyRecvFrom: socket 2 - " << s << std::endl;
//    logMessage << "MyRecvFrom: Received data 2 - " << buf << std::endl;
//    logMessage << "MyRecvFrom: Length 2 - " << len << std::endl;
//    logMessage << "MyRecvFrom: Flags 2 - " << flags << std::endl;
//    logMessage << "MyRecvFrom: From length 2 - " << fromlen << std::endl;*/
//
//    Log(logMessage.str());
//
//    return result;
//}

int WINAPI MyCloseSocket(SOCKET s)
{
    Log("MyCloseSocket");
    return closesocket(s);
}

int WINAPI MyWSACleanup(void)
{
    Log("MyWSACleanup");
    return WSACleanup();
}

DWORD WINAPI MyGetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder)
{
    Log("MyGetIpAddrTable");
    return GetIpAddrTable(pIpAddrTable, pdwSize, bOrder);
}

DWORD WINAPI MyCreatePersistentUdpPortReservation(USHORT StartPort, USHORT NumberOfPorts, PULONG64 Token)
{
    Log("MyCreatePersistentUdpPortReservation");
    return CreatePersistentUdpPortReservation(StartPort, NumberOfPorts, Token);
}

DWORD WINAPI MyCreateProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
    std::stringstream logMessage;
    logMessage << "CreateProxyArpEntry intercepted!" << std::endl;
    Log(logMessage.str());
    return CreateProxyArpEntry(dwAddress, dwMask, dwIfIndex);
}

ULONG WINAPI MyGetBestInterface(IPAddr dwDestAddr, PDWORD pdwBestIfIndex)
{
    std::stringstream logMessage;
    // Log information about the GetBestInterface call
    logMessage << "GetBestInterface intercepted!" << std::endl;
    logMessage << "Destination Address: " << dwDestAddr << std::endl;

    ULONG result = GetBestInterface(dwDestAddr, pdwBestIfIndex);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;
    logMessage << "Best Interface Index: " << *pdwBestIfIndex << std::endl;

    Log(logMessage.str());

    return result;
}

ULONG WINAPI MyGetBestInterfaceEx(struct sockaddr* pDestAddr, PDWORD pdwBestIfIndex)
{
    std::stringstream logMessage;
    // Log information about the GetBestInterfaceEx call
    logMessage << "GetBestInterfaceEx intercepted!" << std::endl;

    ULONG result = GetBestInterfaceEx(pDestAddr, pdwBestIfIndex);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;
    if (result == NO_ERROR) {
        logMessage << "Best Interface Index: " << *pdwBestIfIndex << std::endl;
    }

    Log(logMessage.str());

    return result;
}

ULONG WINAPI MyGetExtendedUdpTable(PVOID pUdpTable, PDWORD pdwSize, BOOL bOrder, ULONG ulAf, UDP_TABLE_CLASS TableClass, ULONG Reserved)
{
    std::stringstream logMessage;

    ULONG result = GetExtendedUdpTable(pUdpTable, pdwSize, bOrder, ulAf, TableClass, Reserved);

    // Log information about the result and parameters
    logMessage << "GetExtendedUdpTable intercepted!" << std::endl;
    logMessage << "Result: " << result << std::endl;

    Log(logMessage.str());

    return result;
}

ULONG WINAPI MyGetInterfaceInfo(PIP_INTERFACE_INFO pIfTable, PULONG dwOutBufLen)
{
    std::stringstream logMessage;
    // Log information about the GetInterfaceInfo call
    logMessage << "GetInterfaceInfo intercepted!" << std::endl;

    ULONG result = GetInterfaceInfo(pIfTable, dwOutBufLen);

    // Log information about the result and parameters
    logMessage << "Result: %lu\n" << result << std::endl;

    // Log information about each network interface
    //if (result == NO_ERROR) {
    //    logMessage << "Number of Interfaces: " << pIfTable->NumAdapters << std::endl;
    //    for (DWORD i = 0; i < pIfTable->NumAdapters; ++i) {
    //        logMessage << "Adapter Index: " << pIfTable->Adapter[i].Index << std::endl;
    //        logMessage << "Adapter Name: " << pIfTable->Adapter[i].Name << std::endl;
    //        //logMessage << "Adapter Description: " << pIfTable->Adapter[i].Description << std::endl;
    //    }
    //}

    Log(logMessage.str());

    return result;
}

DWORD WINAPI MyGetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize, BOOL bOrder)
{
    std::stringstream logMessage;
    // Log information about the GetIpNetTable call
    logMessage << "GetIpNetTable intercepted!" << std::endl;

    DWORD result = GetIpNetTable(pIpNetTable, pdwSize, bOrder);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;

    if (result == NO_ERROR) {
        // Log the information returned by GetIpNetTable
        for (DWORD i = 0; i < pIpNetTable->dwNumEntries; ++i) {
            logMessage << "Index: " << pIpNetTable->table[i].dwIndex << std::endl;
            logMessage << "Physical Address: ";
            for (UINT j = 0; j < pIpNetTable->table[i].dwPhysAddrLen; ++j) {
                logMessage << pIpNetTable->table[i].bPhysAddr[j];
                if (j < pIpNetTable->table[i].dwPhysAddrLen - 1) {
                    logMessage << ":";
                }
            }
            /*logMessage << std::endl;
            logMessage << "IP Address: " << inet_ntoa(*reinterpret_cast<IN_ADDR*>(&pIpNetTable->table[i].dwAddr)) << std::endl;
            logMessage << "Type: " << pIpNetTable->table[i].dwType << std::endl;
            logMessage << std::endl;*/
        }
    }

    Log(logMessage.str());

    return result;
}

DWORD WINAPI MyGetIpNetTable2(ADDRESS_FAMILY Family, PMIB_IPNET_TABLE2* Table)
{
    std::stringstream logMessage;
    // Log information about the GetIpNetTable2 call
    logMessage << "GetIpNetTable2 intercepted!\n" << std::endl;

    // Call the original function
    DWORD result = GetIpNetTable2(Family, Table);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;

    // You can log additional information based on the content of the MIB_IPNET_TABLE2 structure if needed

    Log(logMessage.str());

    return result;
}

//DWORD WINAPI MyGetOwnerModuleFromUdpEntry(PMIB_UDPROW_OWNER_MODULE pUdpEntry, TCPIP_OWNER_MODULE_INFO_CLASS Class, PVOID pBuffer, PDWORD pdwSize)
//{
//    std::stringstream logMessage;
//
//    DWORD result = GetOwnerModuleFromUdpEntry(pUdpEntry, Class, pBuffer, pdwSize);
//
//    Log(logMessage.str());
//
//    return result;
//}

DWORD WINAPI MyGetPerAdapterInfo(ULONG IfIndex, PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
    std::stringstream logMessage;
    // Log information about the GetPerAdapterInfo call
    logMessage << "GetPerAdapterInfo intercepted!" << std::endl;
    logMessage << "Interface Index: " << IfIndex << std::endl;

    DWORD result = GetPerAdapterInfo(IfIndex, pPerAdapterInfo, pOutBufLen);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;

    //if (result == ERROR_SUCCESS) {
    //    // Log information returned by GetPerAdapterInfo
    //    logMessage << "Adapter Name: " << pPerAdapterInfo->AdapterName << std::endl;
    //    logMessage << "Description: " << pPerAdapterInfo->Description << std::endl;
    //    logMessage << "IP Address: " << pPerAdapterInfo->IpAddressList.IpAddress.String << std::endl;
    //    logMessage << "Subnet Mask: " << pPerAdapterInfo->IpAddressList.IpMask.String << std::endl;
    //    logMessage << std::endl;
    //}

    Log(logMessage.str());

    return result;
}

DWORD WINAPI MyGetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
    std::stringstream logMessage;
    // Log information about the GetUdpTable call
    logMessage << "GetUdpTable intercepted!" << std::endl;

    DWORD result = GetUdpTable(pUdpTable, pdwSize, bOrder);

    // Log information about the result and parameters
    logMessage << "Result: " << result << std::endl;
    logMessage << "Buffer Size: " << *pdwSize << std::endl;

    if (result == NO_ERROR) {
        // Log information about the UDP table
        for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++) {
            logMessage << "Local Address: " << pUdpTable->table[i].dwLocalAddr << std::endl;
            logMessage << "Local Port: " << ntohs((u_short)pUdpTable->table[i].dwLocalPort) << std::endl;
            logMessage << std::endl;
        }
    }

    Log(logMessage.str());

    return result;
}

int MyGetHostName(char* name, int namelen)
{
    Log("MyGetHostName");
    // Call the original gethostname to get the actual host name
    int result = gethostname(name, namelen);

    // Log the original host name
    Log("Original Host Name:");
    strncpy(originalHostname, name, namelen - 1);
    Log(originalHostname);
    //originalHostname[namelen - 1] = '\0'; // Ensure null-termination

    // Assign a custom value to the name field
    //strncpy(name, targetHostname, namelen - 1);
    //name[namelen - 1] = '\0'; // Ensure null-termination
    //
    //// Log the modified host name
    //Log("Modified Host Name:");
    //Log(name);

    return result;
}

hostent* WSAAPI MyGetHostByName(const char* name)
{
    Log("MyGetHostByName");
    std::stringstream logMessage;
    // Call the original gethostbyname function
    hostent* result = gethostbyname(name);
    
    // Allocate memory for the custom hostent structure
    hostent* customHostEnt = new hostent;
    
    // Set the hostent fields
    customHostEnt->h_name = _strdup(targetHostname);  // Set the host name
    customHostEnt->h_aliases = nullptr;           // No aliases for simplicity
    customHostEnt->h_addrtype = AF_INET;          // IPv4 address
    customHostEnt->h_length = sizeof(in_addr);    // Length of address
    customHostEnt->h_addr_list = new char* [2];    // Allocate an array of pointers
    

    std::string baseIP = "192.168.0.";

    // Initialize random seed
    std::srand(std::time(0));
    
    // Generate a random number from 0 to 999
    int lastDigits = std::rand() % 1000;

    // Combine the base IP with the random last digits
    std::string randomizedIP = baseIP + std::to_string(lastDigits);

    std::cout << "Randomized IP Address: " << randomizedIP << std::endl;


    // Allocate memory for the custom IP address and copy it
    in_addr customAddress;
    inet_pton(AF_INET, randomizedIP.c_str(), &customAddress);
    customHostEnt->h_addr_list[0] = new char[sizeof(in_addr)];
    memcpy(customHostEnt->h_addr_list[0], &customAddress, sizeof(in_addr));

    // Terminate the list with a nullptr
    customHostEnt->h_addr_list[1] = nullptr;

    // Access and print the custom hostent information
    logMessage << "Original Host Name: " << name << std::endl;
    logMessage << "Custom Host Name: " << customHostEnt->h_name << std::endl;
    logMessage << "Custom IP Address: " << inet_ntoa(customAddress) << std::endl;
    
    // Clean up memory
    delete[] customHostEnt->h_name;
    delete[] customHostEnt->h_addr_list[0];
    delete[] customHostEnt->h_addr_list;
    delete customHostEnt;

    Log(logMessage.str());

    return result;
}

hostent* WINAPI MyGetHostByAddr(const char* addr, int len, int type)
{
    Log("MyGetHostByAddr");
    return gethostbyaddr(addr, len, type);
}

DNS_STATUS MyDnsQuery_A(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORD* ppQueryResults, PVOID* pReserved)
{
    Log("DnsQuery_A");
    return DnsQuery_A(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
}

DNS_STATUS MyDnsQuery_UTF8(PCSTR pszName, WORD wType, DWORD Options, PVOID pExtra, PDNS_RECORD* ppQueryResults, PVOID* pReserved)
{
    Log("DnsQuery_UTF8");
    return DnsQuery_UTF8(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
}

DNS_STATUS MyDnsQuery_W(PCWSTR pszName,WORD wType,DWORD Options,PVOID pExtra,PDNS_RECORD* ppQueryResults,PVOID* pReserved)
{
    Log("DnsQuery_W");
    return DnsQuery_W(pszName, wType, Options, pExtra, ppQueryResults, pReserved);
}

void PrintIPAddressAndPort(const PDNS_RECORD pRecord) {
    std::stringstream logMessage;

    if (pRecord->wType == DNS_TYPE_A) {
        // IPv4 address
        sockaddr_in addr;
        addr.sin_addr.s_addr = pRecord->Data.A.IpAddress;

        logMessage << "IPv4 Address: " << inet_ntoa(addr.sin_addr) << std::endl;
    }
    else {
        // Handle other record types if needed
        logMessage << "Unsupported record type" << std::endl;
    }

    Log(logMessage.str());
}

//void printDnsRecord(const DNS_RECORDA& record) {
//    std::stringstream logMessage;
//
//    logMessage << "Printing fields of DNS_RECORDA" << std::endl;
//
//    logMessage << "pNext: " << record.pNext << std::endl;
//    logMessage << "pName: " << record.pName << std::endl;
//    logMessage << "wType: " << record.wType << std::endl;
//    logMessage << "wDataLength: " << record.wDataLength << std::endl;
//    logMessage << "Flags.DW: " << record.Flags.DW << std::endl;
//    logMessage << "Flags.S: " << record.Flags.S << std::endl;
//    logMessage << "dwTtl: " << record.dwTtl << std::endl;
//    logMessage << "dwReserved: " << record.dwReserved << std::endl;
//
//    // Print fields within the Data union (customize based on your needs)
//    logMessage << "Data.A: " << record.Data.A << std::endl;
//    logMessage << "Data.SOA: " << record.Data.SOA << std::endl;
//    logMessage << "Data.Soa: " << record.Data.Soa << std::endl;
//    logMessage << "Data.PTR: " << record.Data.PTR << std::endl;
//    logMessage << "Data.Ptr: " << record.Data.Ptr << std::endl;
//    logMessage << "Data.NS: " << record.Data.NS << std::endl;
//    logMessage << "Data.Ns: " << record.Data.Ns << std::endl;
//    logMessage << "Data.CNAME: " << record.Data.CNAME << std::endl;
//    logMessage << "Data.Cname: " << record.Data.Cname << std::endl;
//    logMessage << "Data.DNAME: " << record.Data.DNAME << std::endl;
//    // ... (other members omitted for brevity)
//    logMessage << "Data.pDataPtr: " << record.Data.pDataPtr << std::endl;
//
//    Log(logMessage.str());
//}

/*typedef struct _DnsRecordA {
    struct _DnsRecordA* pNext;
    PSTR               pName;
    WORD               wType;
    WORD               wDataLength;
    union {
        DWORD            DW;
        DNS_RECORD_FLAGS S;
    } Flags;
    DWORD              dwTtl;
    DWORD              dwReserved;
    union {
        DNS_A_DATA          A;
        DNS_SOA_DATAA       SOA;
        DNS_SOA_DATAA       Soa;
        DNS_PTR_DATAA       PTR;
        DNS_PTR_DATAA       Ptr;
        DNS_PTR_DATAA       NS;
        DNS_PTR_DATAA       Ns;
        DNS_PTR_DATAA       CNAME;
        DNS_PTR_DATAA       Cname;
        DNS_PTR_DATAA       DNAME;
        DNS_PTR_DATAA       Dname;
        DNS_PTR_DATAA       MB;
        DNS_PTR_DATAA       Mb;
        DNS_PTR_DATAA       MD;
        DNS_PTR_DATAA       Md;
        DNS_PTR_DATAA       MF;
        DNS_PTR_DATAA       Mf;
        DNS_PTR_DATAA       MG;
        DNS_PTR_DATAA       Mg;
        DNS_PTR_DATAA       MR;
        DNS_PTR_DATAA       Mr;
        DNS_MINFO_DATAA     MINFO;
        DNS_MINFO_DATAA     Minfo;
        DNS_MINFO_DATAA     RP;
        DNS_MINFO_DATAA     Rp;
        DNS_MX_DATAA        MX;
        DNS_MX_DATAA        Mx;
        DNS_MX_DATAA        AFSDB;
        DNS_MX_DATAA        Afsdb;
        DNS_MX_DATAA        RT;
        DNS_MX_DATAA        Rt;
        DNS_TXT_DATAA       HINFO;
        DNS_TXT_DATAA       Hinfo;
        DNS_TXT_DATAA       ISDN;
        DNS_TXT_DATAA       Isdn;
        DNS_TXT_DATAA       TXT;
        DNS_TXT_DATAA       Txt;
        DNS_TXT_DATAA       X25;
        DNS_NULL_DATA       Null;
        DNS_WKS_DATA        WKS;
        DNS_WKS_DATA        Wks;
        DNS_AAAA_DATA       AAAA;
        DNS_KEY_DATA        KEY;
        DNS_KEY_DATA        Key;
        DNS_SIG_DATAA       SIG;
        DNS_SIG_DATAA       Sig;
        DNS_ATMA_DATA       ATMA;
        DNS_ATMA_DATA       Atma;
        DNS_NXT_DATAA       NXT;
        DNS_NXT_DATAA       Nxt;
        DNS_SRV_DATAA       SRV;
        DNS_SRV_DATAA       Srv;
        DNS_NAPTR_DATAA     NAPTR;
        DNS_NAPTR_DATAA     Naptr;
        DNS_OPT_DATA        OPT;
        DNS_OPT_DATA        Opt;
        DNS_DS_DATA         DS;
        DNS_DS_DATA         Ds;
        DNS_RRSIG_DATAA     RRSIG;
        DNS_RRSIG_DATAA     Rrsig;
        DNS_NSEC_DATAA      NSEC;
        DNS_NSEC_DATAA      Nsec;
        DNS_DNSKEY_DATA     DNSKEY;
        DNS_DNSKEY_DATA     Dnskey;
        DNS_TKEY_DATAA      TKEY;
        DNS_TKEY_DATAA      Tkey;
        DNS_TSIG_DATAA      TSIG;
        DNS_TSIG_DATAA      Tsig;
        DNS_WINS_DATA       WINS;
        DNS_WINS_DATA       Wins;
        DNS_WINSR_DATAA     WINSR;
        DNS_WINSR_DATAA     WinsR;
        DNS_WINSR_DATAA     NBSTAT;
        DNS_WINSR_DATAA     Nbstat;
        DNS_DHCID_DATA      DHCID;
        DNS_NSEC3_DATA      NSEC3;
        DNS_NSEC3_DATA      Nsec3;
        DNS_NSEC3PARAM_DATA NSEC3PARAM;
        DNS_NSEC3PARAM_DATA Nsec3Param;
        DNS_TLSA_DATA       TLSA;
        DNS_TLSA_DATA       Tlsa;
        DNS_SVCB_DATA       SVCB;
        DNS_SVCB_DATA       Svcb;
        DNS_UNKNOWN_DATA    UNKNOWN;
        DNS_UNKNOWN_DATA    Unknown;
        PBYTE               pDataPtr;
    } Data;
} DNS_RECORDA, * PDNS_RECORDA;*/


DNS_STATUS MyDnsQueryEx(PDNS_QUERY_REQUEST pQueryRequest, PDNS_QUERY_RESULT pQueryResults, PDNS_QUERY_CANCEL pCancelHandle)
{
    Log("DnsQueryEx");

    DNS_STATUS dnsStatus = DnsQueryEx(pQueryRequest, pQueryResults, pCancelHandle);

    PDNS_RECORD pPrevRecord = nullptr;
    PDNS_RECORD pCurrentRecord = pQueryResults->pQueryRecords;

    while (pCurrentRecord != nullptr)
    {
        if (pCurrentRecord->wType == DNS_TYPE_A)
        {
            sockaddr_in addr;
            addr.sin_addr.s_addr = pCurrentRecord->Data.A.IpAddress;

            LogW(pQueryRequest->QueryName);
            Log(inet_ntoa(addr.sin_addr));

            //if (strcmp(inet_ntoa(addr.sin_addr), targetIPAddress) == 0)
            //{
            //    Log("found");
            //    if (pPrevRecord != nullptr)
            //    {
            //        // Update the previous record's pNext to terminate the linked list
            //        pPrevRecord->pNext = nullptr;
            //    }

            //    // Update pQueryResults to point to the found record
            //    pQueryResults->pQueryRecords = pCurrentRecord;
            //    break;
            //}
        }

        // Move to the next record in the linked list
        pPrevRecord = pCurrentRecord;
        pCurrentRecord = pCurrentRecord->pNext;
    }


    //pCurrentRecord = pQueryResults->pQueryRecords;

    //while (pCurrentRecord != nullptr)
    //{
    //    PrintIPAddressAndPort(pCurrentRecord);
    //    // Move to the next record in the linked list
    //    pCurrentRecord = pCurrentRecord->pNext;
    //}

    //// Allocate memory for your custom DNS record
    //PDNS_RECORD pCustomDnsRecord = (PDNS_RECORD)malloc(sizeof(DNS_RECORD));
    //if (pCustomDnsRecord == nullptr)
    //{
    //    Log("Memory allocation failed");
    //    return DNS_ERROR_NO_MEMORY;
    //}

    //// Populate your custom DNS record with the target IPv4 address
    //pCustomDnsRecord->pNext = nullptr;
    //pCustomDnsRecord->wType = DNS_TYPE_A;
    //pCustomDnsRecord->Data.A.IpAddress = inet_addr(targetIPAddress); // Convert targetIPAddress to network byte order

    //// Set pQueryResults to point to your custom DNS record
    //pQueryResults->pQueryRecords = pCustomDnsRecord;

    // Cancel the original DNS query (optional, depending on your requirements)
    // Set pCancelHandle to nullptr if you don't want to cancel the original query
    //if (pCancelHandle != nullptr)
    //{
    //    *pCancelHandle = nullptr; // Assuming pCancelHandle is an output parameter
    //}

    //Log("Custom DNS record set");

    return dnsStatus;//DNS_INFO_NO_RECORDS;
}



int WINAPI MyWSAEnumNetworkEvents(SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents)
{
    Log("MyWSAEnumNetworkEvents");
    return WSAEnumNetworkEvents(s, hEventObject, lpNetworkEvents);
}

int WINAPI MyWSAConnect(SOCKET s, const struct sockaddr* name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS)
{
    Log("MyWSAConnect");
    return WSAConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
}

BOOL WINAPI MyWSAConnectByList(SOCKET s, PSOCKET_ADDRESS_LIST SocketAddress, LPDWORD LocalAddressLength, LPSOCKADDR LocalAddress, LPDWORD RemoteAddressLength, LPSOCKADDR RemoteAddress, const struct timeval* Timeout, LPWSAOVERLAPPED Reserved)
{
    Log("MyWSAConnectByList");
    return WSAConnectByList(s, SocketAddress, LocalAddressLength, LocalAddress, RemoteAddressLength, RemoteAddress, Timeout, Reserved);
}

BOOL WINAPI MyWSAConnectByNameA(SOCKET s, LPSTR nodename, LPSTR servicename, LPDWORD LocalAddressLength, LPSOCKADDR LocalAddress, LPDWORD RemoteAddressLength, LPSOCKADDR RemoteAddress, const struct timeval* Timeout, LPWSAOVERLAPPED Reserved)
{
    Log("MyWSAConnectByNameA");
    return WSAConnectByNameA(s, nodename, servicename, LocalAddressLength, LocalAddress, RemoteAddressLength, RemoteAddress, Timeout, Reserved);
}

BOOL WINAPI MyWSAConnectByNameW(SOCKET s, LPWSTR nodename, LPWSTR servicename, LPDWORD LocalAddressLength, LPSOCKADDR LocalAddress, LPDWORD RemoteAddressLength, LPSOCKADDR RemoteAddress, const struct timeval* Timeout, LPWSAOVERLAPPED Reserved)
{
    Log("MyWSAConnectByNameW");
    return WSAConnectByNameW(s, nodename, servicename, LocalAddressLength, LocalAddress, RemoteAddressLength, RemoteAddress, Timeout, Reserved);
}

HANDLE WINAPI MyWSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char* addr, int len, int type, char* buf, int buflen)
{
    Log("MyWSAAsyncGetHostByAddr");
    return WSAAsyncGetHostByAddr(hWnd, wMsg, addr, len, type, buf, buflen);
}

HANDLE WINAPI MyWSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char* name, char* buf, int buflen)
{
    Log("MyWSAAsyncGetHostByName");
    return WSAAsyncGetHostByName(hWnd, wMsg, name, buf, buflen);
}

int WINAPI MyWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    Log("MyWSARecv");
    return WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

int WINAPI MyWSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    Log("MyWSARecvFrom");

    std::stringstream logMessage;

    int result = WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);

    // Check for any errors
    //if (result != 0) {
    //    // Handle error
    //    Log("MyWSARecvFrom socket error");
    //    return result;
    //}


    // Log sender's address and port
    if (lpFrom->sa_family == AF_INET) { // IPv4
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)lpFrom;

        //inet_pton(AF_INET, "192.168.0.123", &(ipv4->sin_addr));

        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), addr, INET_ADDRSTRLEN);
        int port = ntohs(ipv4->sin_port);
        logMessage << "Sender's Address: " << addr << endl;
        logMessage << "Sender's Port: " << port << endl;
    }
    // Add similar block for IPv6 if needed

    // Log payload bytes
    

    if (lpNumberOfBytesRecvd && *lpNumberOfBytesRecvd > 0) {
        for (DWORD i = 0; i < dwBufferCount; ++i) {
            //printRawBytes(lpBuffers[i].buf, lpBuffers[i].len);
            //logMessage << "Payload: " << lpBuffers[i].buf << " Length: " << lpBuffers[i].len;
        }
    }

    Log(logMessage.str());

    return result;
}

int WINAPI MyWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    Log("MyWSASend");
    return WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

int WINAPI MyWSASendTo(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const struct sockaddr* lpTo, int iToLen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    Log("MyWSASendTo");
    return WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iToLen, lpOverlapped, lpCompletionRoutine);
}

SOCKET WINAPI MyWSASocketA(int af, int type, int protocol, LPWSAPROTOCOL_INFOA lpProtocolInfo, GROUP g, DWORD dwFlags)
{
    Log("MyWSASocketA");
    return WSASocketA(af, type, protocol, lpProtocolInfo, g, dwFlags);
}

SOCKET WINAPI MyWSASocketW(int af, int type, int protocol, LPWSAPROTOCOL_INFOW lpProtocolInfo, GROUP g, DWORD dwFlags)
{
    Log("MyWSASocketW");
    return WSASocketW(af, type, protocol, lpProtocolInfo, g, dwFlags);
}

void LogSockAddr(const sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        const sockaddr_in* sa4 = reinterpret_cast<const sockaddr_in*>(sa);
        char ip4[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sa4->sin_addr), ip4, INET_ADDRSTRLEN);
        Log("  IPv4 Address: " + std::string(ip4));
        Log("  Port: " + std::to_string(ntohs(sa4->sin_port)));
    }
    else if (sa->sa_family == AF_INET6) {
        const sockaddr_in6* sa6 = reinterpret_cast<const sockaddr_in6*>(sa);
        char ip6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(sa6->sin6_addr), ip6, INET6_ADDRSTRLEN);
        Log("  IPv6 Address: " + std::string(ip6));
        Log("  Port: " + std::to_string(ntohs(sa6->sin6_port)));
    }
    // Add support for other address families if needed
}

void LogSockAddr(const SOCKADDR* sa, socklen_t saLen) {
    if (sa->sa_family == AF_INET) {
        const sockaddr_in* sa4 = reinterpret_cast<const sockaddr_in*>(sa);
        char ip4[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sa4->sin_addr), ip4, INET_ADDRSTRLEN);
        Log("  IPv4 Address: " + std::string(ip4));
        Log("  Port: " + std::to_string(ntohs(sa4->sin_port)));
    }
    else if (sa->sa_family == AF_INET6) {
        const sockaddr_in6* sa6 = reinterpret_cast<const sockaddr_in6*>(sa);
        char ip6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(sa6->sin6_addr), ip6, INET6_ADDRSTRLEN);
        Log("  IPv6 Address: " + std::string(ip6));
        Log("  Port: " + std::to_string(ntohs(sa6->sin6_port)));
    }
    // Add support for other address families if needed
}


INT WINAPI MyGetAddrInfo(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult)
{
    Log("getaddrinfo hooked");

    // Allocate memory for the new ADDRINFOA structure
    PADDRINFOA pCustomResult = (PADDRINFOA)malloc(sizeof(ADDRINFOA));
    if (!pCustomResult) {
        return EAI_MEMORY; // Return an error if the allocation fails
    }

    // Clear the memory
    ZeroMemory(pCustomResult, sizeof(ADDRINFOA));

    // Set up the custom address info
    pCustomResult->ai_family = AF_INET; // For IPv4, use AF_INET6 for IPv6
    pCustomResult->ai_socktype = SOCK_STREAM; // or SOCK_DGRAM for UDP
    pCustomResult->ai_protocol = IPPROTO_TCP; // or IPPROTO_UDP for UDP

    // Allocate and set up the sockaddr structure
    pCustomResult->ai_addrlen = sizeof(sockaddr_in);
    pCustomResult->ai_addr = (struct sockaddr*)malloc(sizeof(sockaddr_in));
    if (!pCustomResult->ai_addr) {
        free(pCustomResult);
        return EAI_MEMORY;
    }

    std::string baseIP = "192.168.0.";

    // Initialize random seed
    std::srand(std::time(0));

    // Generate a random number from 0 to 999
    int lastDigits = std::rand() % 1000;

    // Combine the base IP with the random last digits
    std::string randomizedIP = baseIP + std::to_string(lastDigits);

    std::cout << "Randomized IP Address: " << randomizedIP << std::endl;


    struct sockaddr_in* sockAddr = (struct sockaddr_in*)pCustomResult->ai_addr;
    sockAddr->sin_family = AF_INET;
    sockAddr->sin_port = htons(80); // Replace with your desired port
    inet_pton(AF_INET, randomizedIP.c_str(), &sockAddr->sin_addr); // Replace with your desired IP

    // Set the result
    *ppResult = pCustomResult;

    return 0; // Return success

    //// Call the original getaddrinfo function
    //INT result = getaddrinfo(originalHostname, pServiceName, pHints, ppResult);

    //// Log after the getaddrinfo call
    //Log("getaddrinfo");
    //Log(originalHostname);
    ////Log(pServiceName);

    //// Log additional information if needed, e.g., results
    //if (result == 0 && *ppResult != nullptr) {
    //    // Traverse the linked list and keep only the matching result
    //    PADDRINFOA pCurrent = *ppResult;
    //    PADDRINFOA pPrev = nullptr;

    //    while (pCurrent != nullptr)
    //    {
    //        sockaddr* sa = pCurrent->ai_addr;

    //        if (sa->sa_family == AF_INET) {
    //            const sockaddr_in* sa4 = reinterpret_cast<const sockaddr_in*>(sa);
    //            char ip4[INET_ADDRSTRLEN];
    //            inet_ntop(AF_INET, &(sa4->sin_addr), ip4, INET_ADDRSTRLEN);

    //            // Check if the current result matches the specified IPv4 address
    //            if (strcmp(ip4, targetIPAddress) == 0) {
    //                pCurrent->ai_next = nullptr;
    //                *ppResult = pCurrent;
    //                break;
    //            }
    //        }

    //        pCurrent = pCurrent->ai_next;
    //    }

    //    pCurrent = *ppResult;

    //    while (pCurrent != nullptr) {
    //        sockaddr* sa = pCurrent->ai_addr;

    //        // Log information about the address
    //        Log("Address information:");
    //        Log("  Family: " + std::to_string(pCurrent->ai_family));
    //        Log("  Type: " + std::to_string(pCurrent->ai_socktype));
    //        Log("  Protocol: " + std::to_string(pCurrent->ai_protocol));

    //        // Log sockaddr information
    //        LogSockAddr(sa);

    //        // Move to the next element
    //        pPrev = pCurrent;
    //        pCurrent = pCurrent->ai_next;
    //    }
    //}


    //return result;//EAI_NONAME;
}

INT WINAPI MyGetAddrInfoW(PCWSTR pNodeName, PCWSTR pServiceName, const ADDRINFOW* pHints, PADDRINFOW* ppResult)
{
    std::wstring myWideString = ConvertToWideString(originalHostname);
    // Call the original getaddrinfo function
    INT result = GetAddrInfoW(myWideString.c_str(), pServiceName, pHints, ppResult);

    return -1;

    //// Log after the getaddrinfo call
    Log("GetAddrInfoW");
    LogW(myWideString.c_str());

    //// Log additional information if needed, e.g., results
    if (result == 0 && *ppResult != nullptr) {
        // Traverse the linked list and keep only the matching result
        PADDRINFOW pCurrent = *ppResult;
        PADDRINFOW pPrev = nullptr;

        while (pCurrent != nullptr)
        {
            sockaddr* sa = pCurrent->ai_addr;

            if (sa->sa_family == AF_INET) {
                const sockaddr_in* sa4 = reinterpret_cast<const sockaddr_in*>(sa);
                char ip4[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa4->sin_addr), ip4, INET_ADDRSTRLEN);

                // Check if the current result matches the specified IPv4 address
                if (strcmp(ip4, targetIPAddress) == 0) {
                    pCurrent->ai_next = nullptr;
                    *ppResult = pCurrent;
                    break;
                }
            }

            pCurrent = pCurrent->ai_next;
        }

        pCurrent = *ppResult;

        while (pCurrent != nullptr) {
            sockaddr* sa = pCurrent->ai_addr;

            // Log information about the address
            Log("Address information:");
            Log("  Family: " + std::to_string(pCurrent->ai_family));
            Log("  Type: " + std::to_string(pCurrent->ai_socktype));
            Log("  Protocol: " + std::to_string(pCurrent->ai_protocol));

            // Log sockaddr information
            LogSockAddr(sa);

            // Move to the next element
            pPrev = pCurrent;
            pCurrent = pCurrent->ai_next;
        }
    }

    return result;//EAI_NONAME;
}

INT WINAPI MyGetAddrInfoExW(PCWSTR pName, PCWSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXW* hints, PADDRINFOEXW* ppResult, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpHandle)
{
    Log("GetAddrInfoExW");

    return -1;

    INT result = GetAddrInfoExW(pName, pServiceName, dwNameSpace, lpNspId, hints, ppResult, timeout, lpOverlapped, lpCompletionRoutine, lpHandle);

    if (result == 0 && *ppResult != nullptr) {
        // Traverse the linked list and keep only the matching result
        PADDRINFOEXW pCurrent = *ppResult;
        PADDRINFOEXW pPrev = nullptr;

        while (pCurrent != nullptr) {
            sockaddr* sa = pCurrent->ai_addr;

            // Log information about the address
            Log("Address information:");
            Log("  Family: " + std::to_string(pCurrent->ai_family));
            Log("  Type: " + std::to_string(pCurrent->ai_socktype));
            Log("  Protocol: " + std::to_string(pCurrent->ai_protocol));

            // Log sockaddr information
            LogSockAddr(sa);

            // Move to the next element
            pPrev = pCurrent;
            pCurrent = pCurrent->ai_next;
        }
    }

    return result;
}

INT WINAPI MyGetNameInfo(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PCHAR pNodeBuffer, DWORD NodeBufferSize, PCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags)
{
    INT result = getnameinfo(pSockaddr, SockaddrLength, pNodeBuffer, NodeBufferSize, pServiceBuffer, ServiceBufferSize, Flags);

    // Log after the getnameinfo call
    Log("getnameinfo");

    // Log additional information if needed, e.g., results
    if (result == 0) {
        // Log resolved host and service names
        if (pNodeBuffer != nullptr) {

            if (strcmp(pNodeBuffer, originalHostname) == 0) {
                Log("Match");
                *pNodeBuffer = *targetHostname;
            }

            Log("  Resolved Host Name: " + std::string(pNodeBuffer));
        }
        if (pServiceBuffer != nullptr) {
            Log("  Resolved Service Name: " + std::string(pServiceBuffer));
        }
    }
    else {
        Log("  Error: " + std::to_string(result));
    }

    return result;
}

INT WINAPI MyGetNameInfoW(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PWCHAR pNodeBuffer, DWORD NodeBufferSize, PWCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags)
{
    INT result = GetNameInfoW(pSockaddr, SockaddrLength, pNodeBuffer, NodeBufferSize, pServiceBuffer, ServiceBufferSize, Flags);

    // Log after the getnameinfo call
    Log("GetNameInfoW");

    // Log additional information if needed, e.g., results
    if (result == 0) {
        // Log resolved host and service names
        if (pNodeBuffer != nullptr) {

            if (ComparePWCharAndChar(pNodeBuffer, originalHostname) == 0) {
                Log("Match");
                *pNodeBuffer = *targetHostname;
            }

            Log("  Resolved Host Name: " + pwchar_to_str(pNodeBuffer));
        }
        if (pServiceBuffer != nullptr) {
            Log("  Resolved Service Name: " + pwchar_to_str(pServiceBuffer));
        }
    }
    else {
        Log("  Error: " + std::to_string(result));
    }

    return result;
}

INT WINAPI MyWSALookupServiceBeginA(LPWSAQUERYSETA lpqsRestrictions, DWORD dwControlFlags, LPHANDLE lphLookup)
{
    Log("MyWSALookupServiceBeginA");

    // Check if lpqsRestrictions is NULL
    if (lpqsRestrictions == NULL) {
        Log("lpqsRestrictions is NULL");
        return WSALookupServiceBeginA(lpqsRestrictions, dwControlFlags, lphLookup);
    }

    // Call the original function
    INT result = WSALookupServiceBeginA(lpqsRestrictions, dwControlFlags, lphLookup);

    std::stringstream logMessage;

    /*if (lpqsRestrictions->lpszServiceInstanceName != NULL) {
        logMessage << "Service Instance Name: " << lpqsRestrictions->lpszServiceInstanceName << std::endl;
    }
    else {
        logMessage << "Service Instance Name: NULL" << std::endl;
    }*/

    // Check if lpszServiceInstanceName is NULL before logging
    if (lpqsRestrictions->lpszContext != NULL) {
        //logMessage << "lpszServiceInstanceName: " << lpqsRestrictions->lpszServiceInstanceName << std::endl;
        //logMessage << "lpServiceClassId: " << lpqsRestrictions->lpServiceClassId << std::endl;
        //logMessage << "dwNameSpace: " << lpqsRestrictions->dwNameSpace << std::endl;
        //logMessage << "lpszContext: " << lpqsRestrictions->lpszContext << std::endl;
        logMessage << "lpszContext: " << lpqsRestrictions->lpszContext << std::endl;
    }
    else {
        logMessage << "Service Instance Name: NULL" << std::endl;
    }

    Log(logMessage.str());

    return result;
}

INT WINAPI MyWSALookupServiceBeginW(LPWSAQUERYSETW lpqsRestrictions, DWORD dwControlFlags, LPHANDLE lphLookup)
{
    Log("MyWSALookupServiceBeginW");

    // Check if lpqsRestrictions is NULL
    if (lpqsRestrictions == NULL) {
        Log("lpqsRestrictions is NULL");
        return WSALookupServiceBeginW(lpqsRestrictions, dwControlFlags, lphLookup);
    }

    // Call the original function
    INT result = WSALookupServiceBeginW(lpqsRestrictions, dwControlFlags, lphLookup);

    std::wstringstream logMessage;

    /*if (lpqsRestrictions->lpszServiceInstanceName != NULL) {
        logMessage << "Service Instance Name: " << lpqsRestrictions->lpszServiceInstanceName << std::endl;
    }
    else {
        logMessage << "Service Instance Name: NULL" << std::endl;
    }*/

    // Check if lpszServiceInstanceName is NULL before logging
    if (lpqsRestrictions->lpszContext != NULL) {
        //logMessage << "lpszServiceInstanceName: " << lpqsRestrictions->lpszServiceInstanceName << std::endl;
        //logMessage << "lpServiceClassId: " << lpqsRestrictions->lpServiceClassId << std::endl;
        //logMessage << "dwNameSpace: " << lpqsRestrictions->dwNameSpace << std::endl;
        //logMessage << "lpszContext: " << lpqsRestrictions->lpszContext << std::endl;
        logMessage << "lpszContext: " << lpqsRestrictions->lpszContext << std::endl;
    }
    else {
        logMessage << "Service Instance Name: NULL" << std::endl;
    }

    LogW(logMessage.str());

    return result;
}

INT WINAPI MyWSALookupServiceNextA(HANDLE hLookup,DWORD dwControlFlags,LPDWORD lpdwBufferLength,LPWSAQUERYSETA lpqsResults)
{
    Log("MyWSALookupServiceNextA");

    std::stringstream logMessage;

    return -1;

    // Call the original function
    INT result = WSALookupServiceNextA(hLookup, dwControlFlags, lpdwBufferLength, lpqsResults);

    // Check if the call was successful
    if (result == 0)
    {
        logMessage << "addr count " << lpqsResults->dwNumberOfCsAddrs << std::endl;
        // Iterate through the result set and log IPv4 addresses
        for (DWORD i = 0; i < lpqsResults->dwNumberOfCsAddrs; ++i)
        {
            sockaddr* sa = lpqsResults->lpcsaBuffer[i].RemoteAddr.lpSockaddr;
            logMessage << "addr " << i << std::endl;
            if (sa->sa_family == AF_INET)
            {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(sa);
                char addrBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sin->sin_addr), addrBuffer, INET_ADDRSTRLEN);
                logMessage << "IPv4 Address: " << addrBuffer << std::endl;
            }
        }
    }
    else {
        logMessage << "WSALookupServiceNextA failed with error: " << result << std::endl;
    }

    Log(logMessage.str());

    return result;
}

INT WINAPI MyWSALookupServiceNextW(HANDLE hLookup, DWORD dwControlFlags, LPDWORD lpdwBufferLength, LPWSAQUERYSETW lpqsResults)
{
    Log("MyWSALookupServiceNextW");

    std::stringstream logMessage;

    return -1;

    // Call the original function
    INT result = WSALookupServiceNextW(hLookup, dwControlFlags, lpdwBufferLength, lpqsResults);

    // Check if the call was successful
    if (result == 0)
    {
        logMessage << "addr count " << lpqsResults->dwNumberOfCsAddrs << std::endl;
        // Iterate through the result set and log IPv4 addresses
        for (DWORD i = 0; i < lpqsResults->dwNumberOfCsAddrs; ++i)
        {
            sockaddr* sa = lpqsResults->lpcsaBuffer[i].RemoteAddr.lpSockaddr;
            logMessage << "addr " << i << std::endl;
            if (sa->sa_family == AF_INET)
            {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(sa);
                char addrBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sin->sin_addr), addrBuffer, INET_ADDRSTRLEN);
                logMessage << "IPv4 Address: " << addrBuffer << std::endl;
            }
        }
    }
    else {
        logMessage << "WSALookupServiceNextW failed with error: " << result << std::endl;
    }

    Log(logMessage.str());

    return result;
}

DWORD WINAPI MyGetUnicastIpAddressTable(DWORD Family,PMIB_UNICASTIPADDRESS_TABLE* Table)
{
    Log("MyGetUnicastIpAddressTable");

    std::stringstream logMessage;

    //return ERROR_NOT_FOUND;

    // Call the original function
    DWORD result = GetUnicastIpAddressTable(Family, Table);

    return result;

    //if (result == NO_ERROR)
    //{
    //    // Specify the IPv4 address you want to keep
    //    in_addr desiredIPv4Address;
    //    inet_pton(AF_INET, targetIPAddress, &desiredIPv4Address);

    //    // Filter out entries that don't match the specified IPv4 address
    //    for (DWORD i = 0; i < (*Table)->NumEntries; )
    //    {
    //        MIB_UNICASTIPADDRESS_ROW row = (*Table)->Table[i];

    //        // Check if the address family is IPv4
    //        if (row.Address.si_family == AF_INET)
    //        {
    //            // Check if the address matches the specified IPv4 address
    //            if (memcmp(&row.Address.Ipv4.sin_addr, &desiredIPv4Address, sizeof(desiredIPv4Address)) != 0)
    //            {
    //                // Remove the entry by shifting the subsequent entries
    //                for (DWORD j = i; j < (*Table)->NumEntries - 1; ++j)
    //                {
    //                    (*Table)->Table[j] = (*Table)->Table[j + 1];
    //                }

    //                // Decrease the count of entries
    //                --(*Table)->NumEntries;
    //            }
    //            else
    //            {
    //                // Move to the next entry if the address matches
    //                ++i;
    //            }
    //        }
    //        else
    //        {
    //            // Move to the next entry if it's not an IPv4 address
    //            ++i;
    //        }
    //    }
    //}
    //else
    //{
    //    // Print the error information
    //    logMessage << "GetUnicastIpAddressTable failed with error: " << result << std::endl;
    //}


    //// Check if the call was successful
    //if (result == NO_ERROR)
    //{
    //    // Log information about each unicast IP address
    //    for (DWORD i = 0; i < (*Table)->NumEntries; ++i)
    //    {
    //        MIB_UNICASTIPADDRESS_ROW row = (*Table)->Table[i];
    //        char addrBuffer[INET6_ADDRSTRLEN];

    //        if (row.Address.si_family == AF_INET)
    //        {
    //            inet_ntop(AF_INET, &row.Address.Ipv4.sin_addr, addrBuffer, INET6_ADDRSTRLEN);
    //        }
    //        else if (row.Address.si_family == AF_INET6)
    //        {
    //            inet_ntop(AF_INET6, &row.Address.Ipv6.sin6_addr, addrBuffer, INET6_ADDRSTRLEN);
    //        }

    //        logMessage << "Unicast IP Address: " << addrBuffer << std::endl;
    //    }
    //}
    //else
    //{
    //    // Print the error information
    //    logMessage << "GetUnicastIpAddressTable failed with error: " << result << std::endl;
    //}

    //Log(logMessage.str());

    //return result;
}

int MyWSAFDIsSet(SOCKET fd, fd_set* unnamedParam2)
{
    Log("MyWSAFDIsSet"); 
    std::stringstream logMessage;
    logMessage << fd << std::endl;
    Log(logMessage.str());
    return __WSAFDIsSet(fd, unnamedParam2);
}

u_short WINAPI MyHtons(u_short hostshort) {
    
    Log("MyHtons");
    std::stringstream logMessage;
    logMessage << hostshort << std::endl;
    Log(logMessage.str());
    return htons(hostshort);

}

LPWCH WINAPI MyGetEnvironmentStringsW()
{
    Log("MyGetEnvironmentStringsW");
    
    LPWCH result = GetEnvironmentStringsW();

    std::wstringstream logMessage;
    logMessage << result << std::endl;
    LogW(logMessage.str());

    return result;
}

DWORD WINAPI MyGetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
    Log("MyGetEnvironmentVariableW");

    // Original function call
    DWORD result = GetEnvironmentVariableW(lpName, lpBuffer, nSize);

    // Use wstringstream for wide strings
    std::wstringstream logMessage;
    logMessage << lpName << L" " << lpBuffer << std::endl;

    // Assuming Log function can handle wide strings
    LogW(logMessage.str());

    return result;
}

DWORD WINAPI MyGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
    Log("MyGetEnvironmentVariableA");

    // Original function call
    DWORD result = GetEnvironmentVariableA(lpName, lpBuffer, nSize);

    // Use wstringstream for wide strings
    std::stringstream logMessage;
    logMessage << lpName << L" " << lpBuffer << std::endl;

    // Assuming Log function can handle wide strings
    Log(logMessage.str());

    return result;
}


void WINAPI MyGetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
    Log("MyGetSystemInfo");

    _SYSTEM_INFO sysInfo;

    // Filling the _SYSTEM_INFO struct with boilerplate information
    sysInfo.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64; // Example: x64 (AMD or Intel)
    sysInfo.wReserved = 0; // Reserved for future use
    sysInfo.dwPageSize = 4096; // Common page size in bytes
    sysInfo.lpMinimumApplicationAddress = (LPVOID)0x00010000; // Typical minimum address
    sysInfo.lpMaximumApplicationAddress = (LPVOID)0x7FFEFFFF; // Typical maximum address
    sysInfo.dwActiveProcessorMask = 1; // Assuming a single-core processor
    sysInfo.dwNumberOfProcessors = 1; // Single processor
    sysInfo.dwProcessorType = PROCESSOR_INTEL_PENTIUM; // Example processor type
    sysInfo.dwAllocationGranularity = 65536; // Typical allocation granularity
    sysInfo.wProcessorLevel = 6; // Processor level
    sysInfo.wProcessorRevision = 0x0600; // Processor revision

    *lpSystemInfo = sysInfo;

    // Outputting the boilerplate information
    std::cout << "Processor Architecture: " << sysInfo.wProcessorArchitecture << std::endl;
    std::cout << "Page Size: " << sysInfo.dwPageSize << " bytes" << std::endl;
    std::cout << "Minimum Application Address: " << sysInfo.lpMinimumApplicationAddress << std::endl;
    std::cout << "Maximum Application Address: " << sysInfo.lpMaximumApplicationAddress << std::endl;
    std::cout << "Active Processor Mask: " << sysInfo.dwActiveProcessorMask << std::endl;
    std::cout << "Number of Processors: " << sysInfo.dwNumberOfProcessors << std::endl;
    std::cout << "Processor Type: " << sysInfo.dwProcessorType << std::endl;
    std::cout << "Allocation Granularity: " << sysInfo.dwAllocationGranularity << " bytes" << std::endl;
    std::cout << "Processor Level: " << sysInfo.wProcessorLevel << std::endl;
    std::cout << "Processor Revision: " << sysInfo.wProcessorRevision << std::endl;

    /*GetSystemInfo(lpSystemInfo);

    std::stringstream logMessage;
    logMessage << lpSystemInfo->dwOemId << std::endl;
    Log(logMessage.str());

    GetSystemInfo(lpSystemInfo);*/
}

HMODULE WINAPI HookedLoadLibraryA(LPCSTR lpLibFileName)
{
    Log(lpLibFileName);

    HMODULE lib = LoadLibraryA(lpLibFileName);

    InstallHook("Iphlpapi", "GetAdaptersInfo", MyGetAdaptersInfo);
    InstallHook("Iphlpapi", "GetAdaptersAddresses", HookedGetAdaptersAddresses);
    InstallHook("Iphlpapi", "GetIfTable", HookedGetIfTable);
    InstallHook("Iphlpapi", "GetIfEntry", HookedGetIfEntry);
    InstallHook("Iphlpapi", "GetIpNetTable", HookedGetIpNetTable);

    return lib;
}

HMODULE WINAPI HookedLoadLibraryW(LPCWSTR lpLibFileName)
{
    Log(ConvertWideToNarrow(lpLibFileName));

    HMODULE lib = LoadLibraryW(lpLibFileName);

    return lib;
}

DWORD WINAPI MySetEntriesInAclW(ULONG cCountOfExplicitEntries, PEXPLICIT_ACCESS_W pListOfExplicitEntries, PACL OldAcl, PACL* NewAcl)
{
    Log("SetEntriesInAclW");
    return SetEntriesInAclW(cCountOfExplicitEntries, pListOfExplicitEntries, OldAcl, NewAcl);

}

BOOL WINAPI MyGetTokenInformation(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength)
{
    Log("GetTokenInformation");
    return GetTokenInformation(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
}

LSTATUS WINAPI MyRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
    Log("RegOpenKeyA");
    LSTATUS result = RegOpenKeyA(hKey, lpSubKey, phkResult);

    std::stringstream logMessage;
    logMessage << hKey << " " << lpSubKey << " " << phkResult << std::endl;
    Log(logMessage.str());

    return result;
}

LSTATUS WINAPI MyRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    Log("RegOpenKeyExA called.");

    // Call the original function
    LSTATUS result = RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    // Log the hKey and lpSubKey
    std::stringstream logMessage;
    logMessage << L"Key Handle: " << hKey << ", Sub Key: " << (lpSubKey ? lpSubKey : "(null)") << std::endl;
    Log(logMessage.str());

    return result;
}

LSTATUS WINAPI MyRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
    Log("RegOpenKeyW");
    LSTATUS result = RegOpenKeyW(hKey, lpSubKey, phkResult);

    std::wstringstream logMessage;
    logMessage << hKey << " " << lpSubKey << " " << phkResult << std::endl;
    LogW(logMessage.str());

    return result;
}

LSTATUS WINAPI MyRegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    Log("RegOpenKeyExW called.");

    // Call the original function
    LSTATUS result = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    // Log the hKey and lpSubKey
    std::wstringstream logMessage;
    logMessage << L"Key Handle: " << hKey << L", Sub Key: " << (lpSubKey ? lpSubKey : L"(null)") << std::endl;
    LogW(logMessage.str());

    return result;
}


LSTATUS WINAPI MyRegCloseKey(HKEY hKey)
{
    Log("RegCloseKey");
    return RegCloseKey(hKey);
}

LSTATUS MyRegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData)
{
    Log("RegQueryValueA");

    // Log the subkey name
    if (lpSubKey != NULL) {
        Log("SubKey: " + std::string(lpSubKey));
    }

    // Perform the actual query
    LSTATUS result = RegQueryValueA(hKey, lpSubKey, lpData, lpcbData);

    if (result == ERROR_SUCCESS) {
        // Log the retrieved data (assuming it's a string)
        if (lpData != NULL && lpcbData != NULL && *lpcbData > 0) {
            Log("Data: " + std::string(lpData));
        }
    }

    return RegQueryValueA(hKey, lpSubKey, lpData, lpcbData);
}

LSTATUS WINAPI MyRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    Log("RegQueryValueExA");

    // Log the value name
    if (lpValueName != NULL) {
        Log("Value Name: " + std::string(lpValueName));
    }

    // Perform the actual query
    LSTATUS result = RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    return result;
}

LSTATUS MyRegQueryValueW(HKEY hKey, LPCWSTR lpSubKey, LPWSTR lpData, PLONG lpcbData)
{
    Log("RegQueryValueW");

    // Log the subkey name
    if (lpSubKey != NULL) {
        LogW(L"SubKey: " + std::wstring(lpSubKey));
    }

    // Perform the actual query
    LSTATUS result = RegQueryValueW(hKey, lpSubKey, lpData, lpcbData);

    if (result == ERROR_SUCCESS) {
        // Log the retrieved data (assuming it's a string)
        if (lpData != NULL && lpcbData != NULL && *lpcbData > 0) {
            LogW(L"Data: " + std::wstring(lpData));
        }
    }

    return result;
}

LSTATUS WINAPI MyRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    Log("RegQueryValueExW");

    // Log the value name
    if (lpValueName != NULL) {
        LogW(L"Value Name: " + std::wstring(lpValueName));
    }

    // Perform the actual query
    LSTATUS result = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    //if (result == ERROR_SUCCESS) {
    //    // Log the value type
    //    if (lpType != NULL) {
    //        LogW(L"Value Type: " + std::to_wstring(*lpType));
    //    }

    //    // Log the value data based on the value type
    //    if (lpData != NULL && lpcbData != NULL) {
    //        switch (*lpType) {
    //        case REG_SZ: // String data
    //            LogW(L"Value Data: " + std::wstring(reinterpret_cast<LPCWSTR>(lpData)));
    //            break;
    //        case REG_DWORD: // DWORD data
    //            LogW(L"Value Data: " + std::to_wstring(*reinterpret_cast<LPDWORD>(lpData)));
    //            break;
    //            // Add cases for other data types as needed
    //        }
    //    }
    //}

    return result;
}

//MyConvertSidToStringToSidW(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
//{
//    Log("ConvertSidToStringToSidW");
//    return ConvertSidToStringToSidW(a1, a2, a3, a4, a5, a6, a7);
//}

LSTATUS WINAPI MyRegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
{
    Log("RegQueryInfoKeyW");
    LSTATUS result = RegQueryInfoKeyW(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);

    std::wstringstream logMessage;
    logMessage << hKey << std::endl;
    LogW(logMessage.str());

    return result;
}

LSTATUS WINAPI MyRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cchName)
{
    Log("RegEnumKeyW");
    return RegEnumKeyW(hKey, dwIndex, lpName, cchName);
}

LSTATUS WINAPI MyGetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer)
{
    Log("GetUserNameW");

    // Define your custom username
    std::wstring customUsername = L"CustomUserName";

    // Check the size of the custom username (including the null terminator)
    size_t customUsernameLength = customUsername.length() + 1; // +1 for the null terminator

    // Check if the provided buffer is large enough to hold the custom username
    if (*pcbBuffer >= customUsernameLength)
    {
        // Copy the custom username to the provided buffer
        wcscpy_s(lpBuffer, *pcbBuffer, customUsername.c_str());

        // Log the custom username
        std::wstringstream logMessage;
        logMessage << L"Custom Username: " << lpBuffer << std::endl;
        LogW(logMessage.str());

        // Optionally, update the buffer size to the length of the custom username
        *pcbBuffer = static_cast<DWORD>(customUsernameLength);

        return ERROR_SUCCESS;
    }
    else
    {
        // If the buffer is too small, inform the caller of the required size
        *pcbBuffer = static_cast<DWORD>(customUsernameLength);
        std::wstringstream logMessage;
        logMessage << L"ERROR_INSUFFICIENT_BUFFER" << lpBuffer << std::endl;
        LogW(logMessage.str());

        return ERROR_INSUFFICIENT_BUFFER;
    }
}

//MyGetLengthSid
//MyOpenProcessToken
//MyCopySid
//MyIsValidSid
//MyRegOpenKeyExA
//MySetSecurityDescriptorDacl
//MyInitializeSecurityDescriptor
//MySetEntriesInAclW

BOOL MyGetComputerNameA(LPSTR lpBuffer, LPDWORD nSize)
{
    Log("GetComputerNameA");
    return GetComputerNameA(lpBuffer, nSize);
}

BOOL MyGetComputerNameW(LPWSTR lpBuffer, LPDWORD nSize)
{
    Log("GetComputerNameW");
    return GetComputerNameW(lpBuffer, nSize);
}



BOOL MyGetComputerNameExA(COMPUTER_NAME_FORMAT NameType, LPSTR lpBuffer, LPDWORD nSize)
{
    Log("GetComputerNameExA");

    // Original function call
    BOOL result = GetComputerNameExA(NameType, lpBuffer, nSize);

    //strncpy(lpBuffer, targetHostname, strlen(targetHostname) - 1);
    //lpBuffer[strlen(targetHostname) - 1] = '\0'; // Ensure null-termination

    //*lpBuffer = *targetHostname;
    //*nSize = strlen(targetHostname) + 1;

    Log(lpBuffer);

    return result;
}

BOOL MyGetComputerNameExW(COMPUTER_NAME_FORMAT NameType, LPWSTR lpBuffer, LPDWORD nSize)
{
    Log("GetComputerNameExW");
    return GetComputerNameExW(NameType, lpBuffer, nSize);
}

BOOL MyDnsHostnameToComputerNameA(LPCSTR Hostname, LPSTR ComputerName, LPDWORD nSize)
{
    Log("DnsHostnameToComputerNameA");
    return DnsHostnameToComputerNameA(Hostname, ComputerName, nSize);
}

BOOL MyDnsHostnameToComputerNameW(LPCWSTR Hostname, LPWSTR ComputerName, LPDWORD nSize)
{
    Log("DnsHostnameToComputerNameW");
    return DnsHostnameToComputerNameW(Hostname, ComputerName, nSize);
}


//<kernel32.dll.GetComputerNameExW>             
//<kernel32.dll.GetComputerNameExA>        
//<kernel32.dll.BasepGetComputerNameFromNtPath>      
//<kernel32.dll.DnsHostnameToComputerNameW>       
//<kernel32.dll.DnsHostnameToComputerNameExW>   
//<kernel32.dll.GetNamedPipeClientComputerNameW>
//<kernel32.dll.DnsHostnameToComputerNameA>     
//<kernel32.dll.EnumerateLocalComputerNamesA>   
//<kernel32.dll.EnumerateLocalComputerNamesW>   
//<kernel32.dll.GetNamedPipeClientComputerNameA>

HRESULT WINAPI MyCoInitializeEx(LPVOID pvReserved, DWORD dwCoInit)
{
    Log("CoInitializeEx");
    return CoInitializeEx(pvReserved, dwCoInit);
}

class MyEnumWbemClassObjectDecorator : public IEnumWbemClassObject {
private:
    IEnumWbemClassObject* m_decorated;  // Pointer to the decorated IEnumWbemClassObject instance

public:
    // Constructor
    MyEnumWbemClassObjectDecorator(IEnumWbemClassObject* decorated) : m_decorated(decorated) {}

    HRESULT STDMETHODCALLTYPE Clone(IEnumWbemClassObject** ppEnum) {
        Log("Clone");
        return m_decorated->Clone(ppEnum);
    }

    HRESULT STDMETHODCALLTYPE Next(long lTimeout, ULONG uCount, IWbemClassObject** apObjects, ULONG* puReturned) {
        Log("Next");
        return m_decorated->Next(lTimeout, uCount, apObjects, puReturned);
    }

    HRESULT STDMETHODCALLTYPE NextAsync(ULONG uCount, IWbemObjectSink* pSink) {
        Log("NextAsync");
        return m_decorated->NextAsync(uCount, pSink);
    }

    HRESULT STDMETHODCALLTYPE Reset() {
        Log("Reset");
        return m_decorated->Reset();
    }

    HRESULT STDMETHODCALLTYPE Skip(long lTimeout, ULONG nCount) {
        Log("Skip");
        return m_decorated->Skip(lTimeout, nCount);
    }

    // Implement IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() {
        return m_decorated->AddRef();
    }

    ULONG STDMETHODCALLTYPE Release() {
        ULONG count = m_decorated->Release();
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (riid == __uuidof(IWbemLocator) || riid == IID_IUnknown) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        else {
            return m_decorated->QueryInterface(riid, ppv);
        }
    }

    // Destructor
    ~MyEnumWbemClassObjectDecorator() {
        m_decorated->Release();
    }
};



class MyWbemServicesDecorator : public IWbemServices {
private:
    IWbemServices* m_decorated;  // Pointer to the decorated IWbemServices instance

public:
    // Constructor
    MyWbemServicesDecorator(IWbemServices* decorated) : m_decorated(decorated) {}

    HRESULT STDMETHODCALLTYPE CancelAsyncCall(IWbemObjectSink* pSink) {
        Log("CancelAsyncCall");
        return m_decorated->CancelAsyncCall(pSink);
    }

    HRESULT STDMETHODCALLTYPE QueryObjectSink(long lFlags, IWbemObjectSink** ppResponseHandler) {
        Log("QueryObjectSink");
        return m_decorated->QueryObjectSink(lFlags, ppResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE GetObject(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemClassObject** ppObject, IWbemCallResult** ppCallResult) {
        Log("GetObject");
        return m_decorated->GetObject(strObjectPath, lFlags, pCtx, ppObject, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE GetObjectAsync(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("GetObjectAsync");
        return m_decorated->GetObjectAsync(strObjectPath, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE OpenNamespace(const BSTR strNamespace, long lFlags, IWbemContext* pCtx, IWbemServices** ppWorkingNamespace, IWbemCallResult** ppResult) {
        Log("OpenNamespace");
        return m_decorated->OpenNamespace(strNamespace, lFlags, pCtx, ppWorkingNamespace, ppResult);
    }

    HRESULT STDMETHODCALLTYPE PutClass(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) {
        Log("PutClass");
        return m_decorated->PutClass(pObject, lFlags, pCtx, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE PutClassAsync(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("PutClassAsync");
        return m_decorated->PutClassAsync(pObject, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE DeleteClass(const BSTR strClass, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) {
        Log("DeleteClass");
        return m_decorated->DeleteClass(strClass, lFlags, pCtx, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE DeleteClassAsync(const BSTR strClass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("DeleteClassAsync");
        return m_decorated->DeleteClassAsync(strClass, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnum(const BSTR strSuperclass, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) {
        Log("CreateClassEnum");
        return m_decorated->CreateClassEnum(strSuperclass, lFlags, pCtx, ppEnum);
    }

    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(const BSTR strSuperclass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("CreateClassEnumAsync");
        return m_decorated->CreateClassEnumAsync(strSuperclass, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE PutInstance(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) {
        Log("PutInstance");
        return m_decorated->PutInstance(pInst, lFlags, pCtx, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE PutInstanceAsync(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("PutInstanceAsync");
        return m_decorated->PutInstanceAsync(pInst, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE DeleteInstance(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult) {
        Log("DeleteInstance");
        return m_decorated->DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(const BSTR strObjectPath, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("DeleteInstanceAsync");
        return m_decorated->DeleteInstanceAsync(strObjectPath, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE CreateInstanceEnum(const BSTR strClass, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) {
        Log("CreateInstanceEnum");
        HRESULT hr = m_decorated->CreateInstanceEnum(strClass, lFlags, pCtx, ppEnum);
        
        std::stringstream logMessage;
        logMessage << "Result: " << hr << std::endl;
        Log(logMessage.str());

        if (SUCCEEDED(hr)) {
            Log("CreateInstanceEnum success");
            if (ppEnum != nullptr) {
                Log("CreateInstanceEnum decorator");
                IEnumWbemClassObject* original = *ppEnum;
                *ppEnum = new MyEnumWbemClassObjectDecorator(original);
            }
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(const BSTR strClass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("CreateInstanceEnumAsync");
        return m_decorated->CreateInstanceEnumAsync(strClass, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE ExecQuery(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) {
        Log("ExecQuery");
        return m_decorated->ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, ppEnum);
    }

    HRESULT STDMETHODCALLTYPE ExecQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("ExecQueryAsync");
        return m_decorated->ExecQueryAsync(strQueryLanguage, strQuery, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum) {
        Log("ExecNotificationQuery");
        return m_decorated->ExecNotificationQuery(strQueryLanguage, strQuery, lFlags, pCtx, ppEnum);
    }

    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler) {
        Log("ExecNotificationQueryAsync");
        return m_decorated->ExecNotificationQueryAsync(strQueryLanguage, strQuery, lFlags, pCtx, pResponseHandler);
    }

    HRESULT STDMETHODCALLTYPE ExecMethod(const BSTR strObjectPath, const BSTR strMethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemClassObject** ppOutParams, IWbemCallResult** ppCallResult) {
        Log("ExecMethod");
        return m_decorated->ExecMethod(strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult);
    }

    HRESULT STDMETHODCALLTYPE ExecMethodAsync(const BSTR strObjectPath, const BSTR strMethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemObjectSink* pResponseHandler) {
        Log("ExecMethodAsync");
        return m_decorated->ExecMethodAsync(strObjectPath, strMethodName, lFlags, pCtx, pInParams, pResponseHandler);
    }



    // Implement IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() {
        return m_decorated->AddRef();
    }

    ULONG STDMETHODCALLTYPE Release() {
        ULONG count = m_decorated->Release();
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (riid == __uuidof(IWbemServices) || riid == IID_IUnknown) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        else {
            return m_decorated->QueryInterface(riid, ppv);
        }
    }

    // Destructor
    ~MyWbemServicesDecorator() {
        m_decorated->Release();
    }

    // Other IWbemServices methods should be implemented here...
};

class MyWbemLocatorDecorator : public IWbemLocator {
private:
    IWbemLocator* m_decorated;  // Pointer to the decorated IWbemLocator instance

public:
    // Constructor
    MyWbemLocatorDecorator(IWbemLocator* decorated) : m_decorated(decorated) {}

    // Implement all IWbemLocator methods
    HRESULT STDMETHODCALLTYPE ConnectServer(const BSTR strNetworkResource, const BSTR strUser, const BSTR strPassword, 
                                            const BSTR strLocale, long lSecurityFlags, const BSTR strAuthority, 
                                            IWbemContext* pCtx, IWbemServices** ppNamespace
    ) {
        Log("ConnectServer");
        HRESULT hr = m_decorated->ConnectServer(strNetworkResource, strUser, strPassword, strLocale, lSecurityFlags, strAuthority, pCtx, ppNamespace);
        if (SUCCEEDED(hr) && ppNamespace != nullptr) {
            Log("ConnectServer decorator");
            IWbemServices* originalServices = *ppNamespace;
            *ppNamespace = new MyWbemServicesDecorator(originalServices);
        }
        return hr;
    }

    // Implement IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() {
        return m_decorated->AddRef();
    }

    ULONG STDMETHODCALLTYPE Release() {
        ULONG count = m_decorated->Release();
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (riid == __uuidof(IWbemLocator) || riid == IID_IUnknown) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        else {
            return m_decorated->QueryInterface(riid, ppv);
        }
    }

    // Destructor
    ~MyWbemLocatorDecorator() {
        m_decorated->Release();
    }
};

// Custom comparator for GUID
struct GUIDComparer {
    bool operator()(const GUID& lhs, const GUID& rhs) const {
        return memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
    }
};

// Define a map of known IIDs to Interface Names with the custom comparator
std::map<GUID, std::wstring, GUIDComparer> knownInterfaces = {
    { __uuidof(IUnknown), L"IUnknown" },
    { __uuidof(IDispatch), L"IDispatch" },
    { __uuidof(IStream), L"IStream" },
    { __uuidof(IStorage), L"IStorage" },
    { __uuidof(IPersist), L"IPersist" },
    { __uuidof(IPersistStream), L"IPersistStream" },
    { __uuidof(IOleObject), L"IOleObject" },
    { __uuidof(IOleInPlaceObject), L"IOleInPlaceObject" },
    {__uuidof(IWbemLocator), L"IWbemLocator" },
    {__uuidof(IWbemServices), L"IWbemServices" },
    {__uuidof(IWbemClassObject), L"IWbemClassObject" },
    {__uuidof(IWbemObjectSink), L"IWbemObjectSink" },
    {__uuidof(IWbemContext), L"IWbemContext" },
    {__uuidof(IEnumWbemClassObject), L"IEnumWbemClassObject" },
    {__uuidof(IWbemCallResult), L"IWbemCallResult" },
    {__uuidof(IWbemQualifierSet), L"IWbemQualifierSet" },
    {__uuidof(IWbemStatusCodeText), L"IWbemStatusCodeText" },
    {__uuidof(IWbemBackupRestore), L"IWbemBackupRestore" },
    {__uuidof(IWbemBackupRestoreEx), L"IWbemBackupRestoreEx" },
    {__uuidof(IWbemUnsecuredApartment), L"IWbemUnsecuredApartment" },
    {__uuidof(IWbemObjectTextSrc), L"IWbemObjectTextSrc" },
    {__uuidof(IWbemObjectAccess), L"IWbemObjectAccess" },
    {__uuidof(IWbemObjectSinkEx), L"IWbemObjectSinkEx" },
    {__uuidof(IWbemShutdown), L"IWbemShutdown" },
    {__uuidof(IWbemConfigureRefresher), L"IWbemConfigureRefresher" },
    {__uuidof(IWbemRefresher), L"IWbemRefresher" },
    {__uuidof(IWbemHiPerfEnum), L"IWbemHiPerfEnum" },
};

HRESULT WINAPI MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    Log("CoCreateInstance");

    WCHAR guidString[40] = { 0 };
    StringFromGUID2(riid, guidString, 40);

    std::wstringstream logMessage;
    logMessage << L"CoCreateInstance called. Requested interface IID: " << guidString;
    LogW(logMessage.str());

    HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    if (SUCCEEDED(result) && riid == __uuidof(IWbemLocator)) {
        Log("IWbemLocator");
        IWbemLocator* original = static_cast<IWbemLocator*>(*ppv);
        *ppv = new MyWbemLocatorDecorator(original);
    }

    return result;

    //WCHAR guidString[40] = { 0 };
    //StringFromGUID2(riid, guidString, 40);

    //std::wstringstream logMessage;
    //logMessage << L"CoCreateInstance called. Requested interface IID: " << guidString;

    //// Check if the riid is in the knownInterfaces map
    //auto it = knownInterfaces.find(riid);
    //if (it != knownInterfaces.end()) {
    //    logMessage << L" - Interface Name: " << it->second;
    //}
    //else {
    //    logMessage << L" - No matching interface found";
    //}

    //LogW(logMessage.str());

    //HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    //return result;

}

HRESULT WINAPI MyCoCreateInstanceEx(REFCLSID Clsid, IUnknown* punkOuter, DWORD dwClsCtx, COSERVERINFO* pServerInfo, DWORD dwCount, MULTI_QI* pResults)
{
    Log("CoCreateInstanceEx");

    return CoCreateInstanceEx(Clsid, punkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
}


void RandomAlphanumeric(char* str, size_t size) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    if (size) {
        --size; // Leave space for the null terminator
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int)(sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
}

void GenerateRandomMACAddress(unsigned char* macAddress) {
    for (int i = 0; i < 6; ++i) {
        macAddress[i] = static_cast<unsigned char>(rand() % 256);
    }

    //// Optionally set the local bit to indicate a locally administered address
    //macAddress[0] = macAddress[0] | 0x02;
}


//HWND WINAPI GetForegroundWindow_Hook()
//{
//    return Globals::hWnd;
//}
//
//HWND WINAPI WindowFromPoint_Hook(POINT Point)
//{
//    return Globals::hWnd;
//}
//
//HWND WINAPI GetActiveWindow_Hook()
//{
//    return Globals::hWnd;
//}

BOOL WINAPI IsWindowEnabled_Hook(HWND hWnd)
{
    return TRUE;
}

//HWND WINAPI GetFocus_Hook()
//{
//    return Globals::hWnd;
//}
//
//HWND WINAPI GetCapture_Hook()
//{
//    return Globals::hWnd;
//}

HWND WINAPI SetCapture_Hook(HWND inputHwnd)
{
    return inputHwnd;
    //return NULL;
}

BOOL WINAPI ReleaseCapture_Hook()
{
    return TRUE;
}

HWND WINAPI SetActiveWindow_Hook(
    HWND input
)
{
    return input;
}

HWND WINAPI SetFocus_Hook(
    HWND input
)
{
    return input;
}

BOOL WINAPI SetForegroundWindow_Hook(
    HWND hWnd
)
{
    return true;
}

DWORD WINAPI MyGetAnycastIpAddressTable(ADDRESS_FAMILY Family, PMIB_ANYCASTIPADDRESS_TABLE* Table)
{
    Log("GetAnycastIpAddressTable");
    return GetAnycastIpAddressTable(Family, Table);
}

DWORD WINAPI MyGetBestRoute(DWORD dwDestAddr,DWORD dwSourceAddr,PMIB_IPFORWARDROW pBestRoute)
{
    Log("GetBestRoute");
    return GetBestRoute(dwDestAddr, dwSourceAddr, pBestRoute);
}

DWORD WINAPI MyGetBestRoute2(NET_LUID* InterfaceLuid,NET_IFINDEX InterfaceIndex,const SOCKADDR_INET* SourceAddress,const SOCKADDR_INET* DestinationAddress,ULONG AddressSortOptions,PMIB_IPFORWARD_ROW2 BestRoute,SOCKADDR_INET* BestSourceAddress)
{
    Log("GetBestRoute2");
    return GetBestRoute2(InterfaceLuid, InterfaceIndex, SourceAddress, DestinationAddress, AddressSortOptions, BestRoute, BestSourceAddress);
}

DWORD WINAPI MyGetIfStackTable(PMIB_IFSTACK_TABLE* Table)
{
    Log("GetIfStackTable");
    return GetIfStackTable(Table);
}

DWORD WINAPI MyGetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable,PULONG pdwSize,BOOL bOrder)
{
    Log("GetIpForwardTable");
    return GetIpForwardTable(pIpForwardTable, pdwSize, bOrder);
}

DWORD WINAPI MyGetIpForwardTable2(ADDRESS_FAMILY Family,PMIB_IPFORWARD_TABLE2* Table)
{
    Log("GetIpForwardTable2");
    return GetIpForwardTable2(Family, Table);
}

DWORD WINAPI MyGetIpInterfaceTable(ADDRESS_FAMILY Family, PMIB_IPINTERFACE_TABLE* Table)
{
    Log("GetIpInterfaceTable");
    return GetIpInterfaceTable(Family, Table);
}

DWORD WINAPI MyGetIpPathTable(ADDRESS_FAMILY Family, PMIB_IPPATH_TABLE* Table)
{
    Log("GetIpPathTable");
    return GetIpPathTable(Family, Table);
}

ULONG MyGetIpStatistics(PMIB_IPSTATS Statistics)
{
    Log("GetIpStatistics");
    return GetIpStatistics(Statistics);
}

ULONG MyGetIpStatisticsEx(PMIB_IPSTATS Statistics, ULONG Family)
{
    Log("GetIpStatisticsEx");
    return GetIpStatisticsEx(Statistics, Family);
}

DWORD WINAPI MyGetMulticastIpAddressTable(ADDRESS_FAMILY Family, PMIB_MULTICASTIPADDRESS_TABLE* Table)
{
    Log("GetMulticastIpAddressTable");
    return GetMulticastIpAddressTable(Family, Table);
}

DWORD WINAPI MyGetNetworkParams(PFIXED_INFO pFixedInfo,PULONG pOutBufLen)
{
    Log("GetNetworkParams");
    return GetNetworkParams(pFixedInfo, pOutBufLen);
}

DWORD WINAPI MyGetOwnerModuleFromTcpEntry(PMIB_TCPROW_OWNER_MODULE pTcpEntry, TCPIP_OWNER_MODULE_INFO_CLASS Class, PVOID pBuffer, PDWORD pdwSize)
{
    Log("GetOwnerModuleFromTcpEntry");
    return GetOwnerModuleFromTcpEntry(pTcpEntry, Class, pBuffer, pdwSize);
}

DWORD WINAPI MyGetOwnerModuleFromUdpEntry(PMIB_UDPROW_OWNER_MODULE pUdpEntry, TCPIP_OWNER_MODULE_INFO_CLASS Class, PVOID pBuffer, PDWORD pdwSize)
{
    Log("GetOwnerModuleFromUdpEntry");
    return GetOwnerModuleFromUdpEntry(pUdpEntry, Class, pBuffer, pdwSize);
}

ULONG MyGetTcpTable(PMIB_TCPTABLE TcpTable, PULONG SizePointer, BOOL Order)
{
    Log("GetTcpTable");
    return GetTcpTable(TcpTable, SizePointer, Order);
}

ULONG MyGetTcpTable2(PMIB_TCPTABLE2 TcpTable, PULONG SizePointer, BOOL Order)
{
    Log("GetTcpTable2");
    return GetTcpTable2(TcpTable, SizePointer, Order);
}

HANDLE MyCreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCSTR lpName)
{
    Log("CreateMutexA");
    return CreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
}

HANDLE MyCreateMutexExA(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    Log("CreateMutexExA");
    return CreateMutexExA(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
}

HANDLE MyCreateMutexExW(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCWSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    Log("CreateMutexExW");
    return CreateMutexExW(lpMutexAttributes, lpName, dwFlags, dwDesiredAccess);
}

HANDLE MyCreateMutexW(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCWSTR lpName)
{
    Log("CreateMutexW");
    return CreateMutexW(lpMutexAttributes, bInitialOwner, lpName);
}

//BOOL WINAPI MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
//{
//    UNICODE_STRING objectName;
//    ULONG returnLength;
//    NTSTATUS status = NtQueryObject(
//        hDevice,
//        ObjectBasicInformation,
//        &objectName,
//        sizeof(objectName),
//        &returnLength
//    );
//
//    if (NT_SUCCESS(status)) {
//        // Log the objectName here
//        std::stringstream logMessage;
//        logMessage << "Device name: %wZ" << objectName.Buffer << std::endl;
//        Log(logMessage.str());
//    }
//
//    return DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
//}

HANDLE WINAPI MyCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    Log("CreateFileA");
    Log(lpFileName);
    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HANDLE WINAPI MyCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    Log("CreateFileW");
    LogW(lpFileName);
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI MySetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, const GUID* InterfaceClassGuid, DWORD MemberIndex, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    Log("SetupDiEnumDeviceInterfaces");

    BOOL result = SetupDiEnumDeviceInterfaces(DeviceInfoSet, DeviceInfoData, InterfaceClassGuid, MemberIndex, DeviceInterfaceData);

    if (result)
    {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, NULL, 0, &requiredSize, NULL);

        PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        if (detailData)
        {
            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, detailData, requiredSize, NULL, NULL))
            {
                Log("Device Path: ");
                Log(detailData->DevicePath);
            }
            free(detailData);
        }
    }

    return result;
}

HDEVINFO WINAPI MySetupDiGetClassDevsW(const GUID* ClassGuid, PCWSTR Enumerator, HWND hwndParent, DWORD Flags)
{
    Log("SetupDiGetClassDevsW");
    // Buffer to hold the GUID string
    wchar_t guidString[39]; // GUIDs are 38 characters plus null terminator

    // Convert the GUID to a string (if it's not null)
    if (ClassGuid != nullptr && StringFromGUID2(*ClassGuid, guidString, sizeof(guidString) / sizeof(wchar_t)))
    {
        // Log the GUID string
        LogW(L"SetupDiGetClassDevsW - ClassGuid: " + std::wstring(guidString));
    }
    else
    {
        // Log a placeholder or error message if ClassGuid is null
        LogW(L"SetupDiGetClassDevsW - ClassGuid: null or conversion failed");
    }

    // Call the original function
    return SetupDiGetClassDevsW(ClassGuid, Enumerator, hwndParent, Flags);
}

BOOL WINAPI MyCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    Log("CreateProcessA");
    Log(lpCommandLine);
    return CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

BOOL WINAPI MyCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    Log("CreateProcessW");
    LogW(lpCommandLine);
    return CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}


std::string GenerateRandomString(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string randomString;

    for (size_t i = 0; i < length; ++i) {
        randomString += characters[rand() % characters.length()];
    }

    return randomString;
}

FILE* hooked_popen(const char* command, const char* type) {
    // Do something before calling the original popen
    // e.g., logging the command
    printf("Hooked popen called with command: %s\n", command);

    // Call the original popen function
    return _popen(command, type);
}

void CreateBroadcastSocket()
{
    // Create a socket
    broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (broadcastSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // Set SO_REUSEADDR option
    int optval = 1;
    if (setsockopt(broadcastSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(int)) < 0) {
        printf("setsockopt 1 failed: %d\n", WSAGetLastError());
        closesocket(broadcastSocket);
        WSACleanup();
        return;
    }

    // Set SO_BROADCAST option
    if (setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(int)) < 0) {
        printf("setsockopt 2 failed: %d\n", WSAGetLastError());
        closesocket(broadcastSocket);
        WSACleanup();
        return;
    }

    // Bind the socket
    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(9999);
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(broadcastSocket, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(broadcastSocket);
        WSACleanup();
        return;
    }
}

int WINAPI MyWSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
    Log("MyWSAStartup");
    int result = WSAStartup(wVersionRequested, lpWSAData);

    CreateBroadcastSocket();

    return result;
}

int WINAPI MySetSocketOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    Log("MySetSocketOpt");


    return 0;// setsockopt(s, level, optname, optval, optlen);
}

int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
{
    Log("MyBind");

    /*if (broadcastSocket == INVALID_SOCKET) {
        CreateBroadcastSocket();
    }*/

    // Ensure the address is IPv4 (AF_INET)
    if (name->sa_family == AF_INET)
    {
        struct sockaddr_in modified_addr = {};
        memcpy(&modified_addr, name, sizeof(modified_addr));

        // Set IP to 0.0.0.0
        //modified_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        modified_addr.sin_addr.s_addr = inet_addr(targetIPAddress);

        // Check the original port
        USHORT originalPort = ntohs(modified_addr.sin_port);

        /*if (originalPort == 41455) {
            int optval = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
        }*/

        // Specify the interface
        struct in_addr target_interface;
        target_interface.s_addr = inet_addr(targetIPAddress); // replace with your interface's IP address

        // Set the IP_UNICAST_IF option
        //if (setsockopt(s, IPPROTO_IP, IP_UNICAST_IF, (char*)&target_interface, sizeof(target_interface)) == SOCKET_ERROR) {
        //    Log("setsockopt for IP_UNICAST_IF failed: ");//%ld\n", WSAGetLastError());
        //    closesocket(s);
        //    WSACleanup();
        //    return 1;
        //}

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

//int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
//{
//    Log("MySendTo");
//
//    // Log information about the sendto call
//    std::stringstream logMessage;
//    logMessage << "sendto intercepted!" << std::endl;
//    logMessage << "Socket: " << s << std::endl;
//    logMessage << "Length: " << len << std::endl;
//    logMessage << "Flags: " << flags << std::endl;
//
//    // Log information about the destination address
//    if (to != nullptr && tolen >= sizeof(sockaddr_in))
//    {
//        const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(to);
//
//        char destAddrString[INET_ADDRSTRLEN];
//        if (inet_ntop(AF_INET, &(addr->sin_addr), destAddrString, INET_ADDRSTRLEN) != nullptr)
//        {
//            logMessage << "MySendTo Address: " << destAddrString << std::endl;
//            logMessage << "MySendTo Port: " << ntohs(addr->sin_port) << std::endl;
//        }
//        else
//        {
//            logMessage << "inet_ntop error " << std::endl;
//        }
//    }
//
//    Log(logMessage.str());
//
//    //printRawBytes(buf, len);
//    return sendto(s, buf, len, flags, to, tolen);
//
//    //// Ensure the address is IPv4 (AF_INET)
//    //if (to->sa_family == AF_INET)
//    //{
//    //    sockaddr_in* sockaddrIPv4 = (sockaddr_in*)to;
//
//    //    // Check if the destination is a broadcast address
//    //    if (sockaddrIPv4->sin_addr.S_un.S_addr == INADDR_BROADCAST)
//    //    {
//    //        // Send the packet multiple times
//    //        for (int i = 0; i < 8; i++)
//    //        {
//    //            // Update the destination port
//    //            sockaddrIPv4->sin_port = htons(ntohs(sockaddrIPv4->sin_port) + (2 * i));
//
//    //            // Send the packet
//    //            int result = sendto(s, buf, len, flags, (struct sockaddr*)sockaddrIPv4, tolen);
//    //            if (result == SOCKET_ERROR)
//    //            {
//    //                // Handle error
//    //                int error = WSAGetLastError();
//    //                std::stringstream logMessage;
//    //                logMessage << "sendto failed with error: " << error << std::endl;
//    //                Log(logMessage.str());
//    //            }
//    //        }
//
//    //        // Return the number of bytes sent
//    //        return len;
//    //    }
//    //}
//
//    //// If not a broadcast address, call the original sendto function
//    //return sendto(s, buf, len, flags, to, tolen);
//}
//
//int WINAPI MyRecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
//{
//    // Convert the 'from' address to a string
//    char ipString[INET_ADDRSTRLEN];
//    struct sockaddr_in* from_in = (struct sockaddr_in*)from;
//    inet_ntop(AF_INET, &(from_in->sin_addr), ipString, INET_ADDRSTRLEN);
//
//    // Check if the string starts with "192.168."
//    /*if (strncmp(ipString, "192.168.", 8) != 0) {
//        return SOCKET_ERROR;
//    }*/
//
//
//    std::stringstream logMessage;
//
//    int result = recvfrom(s, buf, len, flags, from, fromlen);
//
//    sockaddr_in* localAddress = reinterpret_cast<sockaddr_in*>(from);
//
//    /*if (strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) == 0)
//        return WSAETIMEDOUT;*/
//
//    logMessage << "MyRecvFrom Local Address: " << inet_ntoa(localAddress->sin_addr) << std::endl;
//    logMessage << "MyRecvFrom Local Port: " << ntohs(localAddress->sin_port) << std::endl;
//    logMessage << "MyRecvFrom Family: " << ntohs(localAddress->sin_family) << std::endl;
//    logMessage << "MyRecvFrom: socket - " << s << std::endl;
//   /* printRawBytes(buf, len);
//    logMessage << "MyRecvFrom: Length - " << len << std::endl;
//    logMessage << "MyRecvFrom: Flags - " << flags << std::endl;
//    logMessage << "MyRecvFrom: From length - " << fromlen << std::endl;*/
//
//    Log(logMessage.str());
//
//    return result;
//}

// Maximum length of an IP address string (including null terminator)
//const int MAX_IP_ADDRESS_LENGTH = 16;
const int MAX_PROCESS_ID_LENGTH = 10;

// Padding character
const char PADDING_CHAR = '#';

// Map of real IP addresses to virtual IP addresses
std::map<DWORD, std::pair<std::string, std::string>> pidAddressMap;

// Hooked sendto function
//int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr_in* to, int tolen)
//{
//    std::stringstream logMessage;
//    logMessage << "MySendTo Address: " << inet_ntoa(reinterpret_cast<const sockaddr_in*>(to)->sin_addr);
//    Log(logMessage.str());
//
//    // Pad the targetIPAddress to the maximum length
//    std::string paddedTargetIPAddress(targetIPAddress);
//    paddedTargetIPAddress.resize(MAX_IP_ADDRESS_LENGTH - 1, PADDING_CHAR);
//
//    // Prepend the padded targetIPAddress to the buffer
//    std::string modifiedBuf = paddedTargetIPAddress + std::string(buf, buf + len);
//
//   // return sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (struct sockaddr*)to, tolen);
//
//    // Create a copy of the to address
//    struct sockaddr_in toCopy = *to;
//
//    // Convert the to address to a string
//    char* toAddress = inet_ntoa(toCopy.sin_addr);
//
//    // Search the ipAddressMap for a value that matches the to address
//    for (const auto& pair : ipAddressMap)
//    {
//        if (pair.second == toAddress)
//        {
//            // If a match is found, replace the to address with the corresponding key
//            toCopy.sin_addr.s_addr = inet_addr(pair.first.c_str());
//            break;
//        }
//    }
//
//    // Call the original sendto function with the possibly modified to address
//    return sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (struct sockaddr*)&toCopy, tolen);
//}

// Hooked sendto function
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


// Function to generate a random IP address
std::string GenerateRandomIPAddress()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254); // Avoid 0 and 255 in each octet

    std::stringstream ss;
    ss << dis(gen) << "." << dis(gen) << "." << dis(gen) << "." << dis(gen);

    return ss.str();
}

// Hooked recvfrom function
//int WINAPI MyRecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr_in* from, int* fromlen)
//{
//    // Create a buffer to hold the data received from recvfrom
//    char* recvBuf = new char[len + MAX_IP_ADDRESS_LENGTH];
//
//    // Call the original recvfrom function
//    int result = recvfrom(s, recvBuf, len + MAX_IP_ADDRESS_LENGTH - 1, flags, (struct sockaddr*)from, fromlen);
//
//    /*std::stringstream logMessage;
//    logMessage << "MyRecvFrom Address: " << inet_ntoa(from->sin_addr);
//    Log(logMessage.str());*/
//
//    // Check if the call was successful
//    if (result > 0)
//    {
//        // Extract the real IP address from the start of the buffer
//        std::string realIPAddress(recvBuf, recvBuf + MAX_IP_ADDRESS_LENGTH - 1);
//
//        // Remove the padding from the real IP address
//        realIPAddress.erase(realIPAddress.find(PADDING_CHAR));
//
//        // Check if the real IP address is in the map
//        if (ipAddressMap.find(realIPAddress) == ipAddressMap.end())
//        {
//            // If it's not, generate a new virtual IP address and add it to the map
//            std::string virtualIPAddress = GenerateRandomIPAddress();
//            ipAddressMap[realIPAddress] = virtualIPAddress;
//        }
//
//        // Get the virtual IP address from the map
//        std::string virtualIPAddress = ipAddressMap[realIPAddress];
//
//        Log("MyRecvFrom Address: " + realIPAddress);
//        //Log("MyRecvFrom virtualIPAddress: " + virtualIPAddress);
//
//        // Assign the virtual IP address to the returned from addr field
//        from->sin_addr.s_addr = inet_addr(realIPAddress.c_str());
//
//        // Remove the real IP address from the start of the buffer
//        memmove(buf, recvBuf + MAX_IP_ADDRESS_LENGTH - 1, result - (MAX_IP_ADDRESS_LENGTH - 1));
//
//        // Update the result to reflect the new length of the buffer
//        result -= (MAX_IP_ADDRESS_LENGTH - 1);
//    }
//
//    // Clean up the recvBuf
//    delete[] recvBuf;
//
//    // Return the result
//    return result;
//}

// Hooked recvfrom function
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


//int WSAAPI MyWSAIoctl(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
//{
//    // Call the original function
//    int result = WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
//
//    // If the control code is SIO_ROUTING_INTERFACE_QUERY, overwrite the output buffer address string
//    if (dwIoControlCode == SIO_ROUTING_INTERFACE_QUERY && result == 0)
//    {
//        // The output buffer should be a sockaddr structure
//        sockaddr_in* addr = static_cast<sockaddr_in*>(lpvOutBuffer);
//
//        // Generate a random IP address string
//        if (randomIPAddress.empty())
//        {
//            std::random_device rd;
//            std::mt19937 gen(rd());
//            std::uniform_int_distribution<> dis(0, 99);
//            randomIPAddress = "192.168.0." + std::to_string(dis(gen));
//        }
//
//        // Overwrite the address with the random IP address
//        inet_pton(AF_INET, randomIPAddress.c_str(), &(addr->sin_addr));
//
//        // Convert the new address to a string for logging
//        char str[INET_ADDRSTRLEN];
//        inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
//
//        // Log the new address
//        Log("SIO_ROUTING_INTERFACE_QUERY overwritten with address: " + std::string(str));
//    }
//
//    return result;
//}

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


VIRTUALNET_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("injector.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): jailbreakhook.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString) * sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    //targetIPAddress = getenv("TargetIPAddress");
    GetBestLocalIPAddress();
    if (targetIPAddress[0] != '\0') {
        Log("TargetIPAddress: ");
        Log(targetIPAddress);
        pidAddressMap[GetCurrentProcessId()] = std::make_pair(targetIPAddress, targetIPAddress);
    }
    else {
        Log("TargetIPAddress not found");
    }

    //InstallHook("ws2_32.dll", "WSAStartup", MyWSAStartup);
    //InstallHook("ws2_32.dll", "setsockopt", MySetSocketOpt);
    InstallHook("ws2_32.dll", "bind", MyBind);
    InstallHook("ws2_32.dll", "WSAIoctl", MyWSAIoctl);
    InstallHook("ws2_32.dll", "sendto", MySendTo);
    InstallHook("ws2_32.dll", "recvfrom", MyRecvFrom);


    //char buffer[1024];
    //struct sockaddr_in from;
    //int fromlen = sizeof(from);

    //while (true) // Loop indefinitely
    //{
    //    if (broadcastSocket == INVALID_SOCKET) {
    //        continue;
    //    }

    //    std::stringstream logMessage;

    //    memset(buffer, 0, sizeof(buffer)); // Clear the buffer

    //    // Receive data
    //    int bytesReceived = recvfrom(broadcastSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen);
    //    if (bytesReceived == SOCKET_ERROR)
    //    {
    //        // Handle error
    //        int error = WSAGetLastError();
    //        logMessage << "recvfrom failed with error: " << error << std::endl;
    //    }
    //    else
    //    {
    //        // Process received data
    //        logMessage << "MyRecvFrom Local Address: " << inet_ntoa(from.sin_addr) << std::endl;
    //        logMessage << "MyRecvFrom Local Port: " << ntohs(from.sin_port) << std::endl;
    //        logMessage << "Received data: " << buffer << std::endl;
    //    }

    //    Log(logMessage.str());
    //}


    //// Initialize random seed
    //srand(static_cast<unsigned int>(time(nullptr)));

    //// Generate random values
    //std::string newComputerName = GenerateRandomString(10); // Example length 10
    //std::string newUserName = GenerateRandomString(8); // Example length 8

    //// Set environment variables
    //if (!SetEnvironmentVariable("COMPUTERNAME", newComputerName.c_str())) {
    //    // Handle error
    //    DWORD dwError = GetLastError();
    //}

    //if (!SetEnvironmentVariable("USERNAME", newUserName.c_str())) {
    //    // Handle error
    //    DWORD dwError = GetLastError();
    //}

    //// Rest of your code


    //////InstallHook("user32", "GetForegroundWindow", GetForegroundWindow_Hook);
    //////InstallHook("user32", "WindowFromPoint", WindowFromPoint_Hook);
    //////InstallHook("user32", "GetActiveWindow", GetActiveWindow_Hook);
    ////InstallHook("user32", "IsWindowEnabled", IsWindowEnabled_Hook);
    //////InstallHook("user32", "GetFocus", GetFocus_Hook);
    //////InstallHook("user32", "GetCapture", GetCapture_Hook);
    ////InstallHook("user32", "SetCapture", SetCapture_Hook);
    ////InstallHook("user32", "ReleaseCapture", ReleaseCapture_Hook);
    ////InstallHook("user32", "SetActiveWindow", SetActiveWindow_Hook);
    ////InstallHook("user32", "SetFocus", SetFocus_Hook);
    ////
    ////InstallHook("user32", "SetForegroundWindow", SetForegroundWindow_Hook);


    //// Generate random bytes
    //std::random_device rd;
    //std::mt19937 gen(rd());
    //std::uniform_int_distribution<> dis(0, 255);
    //for (int i = 0; i < 1; ++i) {
    //    idBytes[i] = static_cast<unsigned char>(dis(gen));
    //}


    //
    //
    //// Initialize random seed
    //srand(static_cast<unsigned int>(time(NULL)));
    //
    //GenerateRandomMACAddress(macAddress);
    //
    //
    ///*strcpy(targetHostname, "DESKTOP-AAAAAAA");
    //RandomAlphanumeric(targetHostname + 8, 7 + 1);*/
    //
    //Log("Rando");


    //targetIPAddress = "192.168.0.10";
    //hostIPAddress = "192.168.0.10";
    //targetHostname = "DESKTOP-BBBBBBB";
    //targetIPAddress = "192.168.0.15";
    //targetHostname = "DESKTOP-BBBBBBB";

    //InstallHook("ole32.dll", "CoInitializeEx", MyCoInitializeEx);
    //InstallHook("ole32.dll", "CoCreateInstance", MyCoCreateInstance);
    //InstallHook("ole32.dll", "CoCreateInstanceEx", MyCoCreateInstanceEx);

    //InstallHook("kernel32", "LoadLibraryA", HookedLoadLibraryA);
    //InstallHook("kernel32", "CreateProcessA", MyCreateProcessA);
    //InstallHook("kernel32", "CreateProcessW", MyCreateProcessW);

    //InstallHook("setupapi.dll", "SetupDiEnumDeviceInterfaces", MySetupDiEnumDeviceInterfaces);
    //InstallHook("setupapi.dll", "SetupDiGetClassDevsW", MySetupDiGetClassDevsW);

    //InstallHook("Iphlpapi", "GetIpNetEntry2", Hooked_GetIpNetEntry2);
    //InstallHook("Iphlpapi", "GetIfEntry2", Hooked_GetIfEntry2);
    //InstallHook("Iphlpapi", "GetIfEntry2Ex", Hooked_GetIfEntry2Ex);
    //InstallHook("Iphlpapi", "GetIfTable2", Hooked_GetIfTable2);
    //InstallHook("Iphlpapi", "GetIfTable2Ex", Hooked_GetIfTable2Ex);
    //InstallHook("Iphlpapi", "GetAdaptersInfo", MyGetAdaptersInfo); //
    //InstallHook("Iphlpapi", "GetUnicastIpAddressTable", MyGetUnicastIpAddressTable); //
    //InstallHook("Iphlpapi", "GetAdaptersAddresses", HookedGetAdaptersAddresses);
    //InstallHook("Iphlpapi", "GetIfTable", HookedGetIfTable);
    //InstallHook("Iphlpapi", "GetIfEntry", HookedGetIfEntry);
    //InstallHook("Iphlpapi", "CreatePersistentUdpPortReservation", MyCreatePersistentUdpPortReservation);
    //InstallHook("Iphlpapi", "CreateProxyArpEntry", MyCreateProxyArpEntry);
    //InstallHook("Iphlpapi", "GetBestInterface", MyGetBestInterface);
    //InstallHook("Iphlpapi", "GetBestInterfaceEx", MyGetBestInterfaceEx);
    //InstallHook("Iphlpapi", "GetExtendedUdpTable", MyGetExtendedUdpTable);
    //InstallHook("Iphlpapi", "GetInterfaceInfo", MyGetInterfaceInfo);
    //InstallHook("Iphlpapi", "GetIpAddrTable", MyGetIpAddrTable);
    //InstallHook("Iphlpapi", "GetIpNetTable", MyGetIpNetTable);
    //InstallHook("Iphlpapi", "GetIpNetTable2", MyGetIpNetTable2);
    //InstallHook("Iphlpapi", "GetOwnerModuleFromUdpEntry", MyGetOwnerModuleFromUdpEntry);
    //InstallHook("Iphlpapi", "GetPerAdapterInfo", MyGetPerAdapterInfo);
    //InstallHook("Iphlpapi", "GetUdpTable", MyGetUdpTable);
    //InstallHook("Iphlpapi", "GetAnycastIpAddressTable", MyGetAnycastIpAddressTable);
    //InstallHook("Iphlpapi", "GetBestRoute", MyGetBestRoute);
    //InstallHook("Iphlpapi", "GetBestRoute2", MyGetBestRoute2);
    //InstallHook("Iphlpapi", "GetIfStackTable", MyGetIfStackTable);
    //InstallHook("Iphlpapi", "GetIpForwardTable", MyGetIpForwardTable);
    //InstallHook("Iphlpapi", "GetIpForwardTable2", MyGetIpForwardTable2);
    //InstallHook("Iphlpapi", "GetIpInterfaceTable", MyGetIpInterfaceTable);
    //InstallHook("Iphlpapi", "GetIpPathTable", MyGetIpPathTable);
    //InstallHook("Iphlpapi", "GetIpStatistics", MyGetIpStatistics);
    //InstallHook("Iphlpapi", "GetIpStatisticsEx", MyGetIpStatisticsEx);
    //InstallHook("Iphlpapi", "GetMulticastIpAddressTable", MyGetMulticastIpAddressTable);
    //InstallHook("Iphlpapi", "GetNetworkParams", MyGetNetworkParams);
    //InstallHook("Iphlpapi", "GetOwnerModuleFromTcpEntry", MyGetOwnerModuleFromTcpEntry);
    //InstallHook("Iphlpapi", "GetOwnerModuleFromUdpEntry", MyGetOwnerModuleFromUdpEntry);
    //InstallHook("Iphlpapi", "GetTcpTable", MyGetTcpTable);
    //InstallHook("Iphlpapi", "GetTcpTable2", MyGetTcpTable2);

    // Generate a random IP address string
    /*std::random_device rd2;
    std::mt19937 gen2(rd2());
    std::uniform_int_distribution<> dis2(0, 99);
    std::string randomIPAddress = "192.168.0." + std::to_string(dis2(gen2));*/

    ////
    ////InstallHook("ws2_32.dll", "WSAStartup", MyWSAStartup);
    //InstallHook("ws2_32.dll", "socket", MySocket);
    //InstallHook("ws2_32.dll", "bind", MyBind); //
    //InstallHook("ws2_32.dll", "WSAIoctl", MyWSAIoctl);
    //InstallHook("ws2_32.dll", "getsockname", MyGetSockName);
    //InstallHook("ws2_32.dll", "sendto", MySendTo); //
    //InstallHook("ws2_32.dll", "recvfrom", MyRecvFrom); //
    //InstallHook("ws2_32.dll", "closesocket", MyCloseSocket);
    //InstallHook("ws2_32.dll", "WSACleanup", MyWSACleanup);
    //
    //InstallHook("ws2_32.dll", "gethostname", MyGetHostName); //
    //InstallHook("ws2_32.dll", "gethostbyname", MyGetHostByName); //
    ////////InstallHook("ws2_32.dll", "gethostbyaddr", MyGetHostByAddr);
    ////InstallHook("ws2_32.dll", "getnameinfo", MyGetNameInfo);
    /////////*InstallHook("ws2_32.dll", "GetNameInfo", MyGetNameInfo2);
    ////////InstallHook("ws2_32.dll", "GetNameInfoA", MyGetNameInfoA);*/
    ////InstallHook("ws2_32.dll", "GetNameInfoW", MyGetNameInfoW);
    //InstallHook("ws2_32.dll", "WSAEnumNetworkEvents", MyWSAEnumNetworkEvents);
    ////////InstallHook("ws2_32.dll", "WSAConnect", MyWSAConnect);
    ////////InstallHook("ws2_32.dll", "WSAConnectByList", MyWSAConnectByList);
    ////////InstallHook("ws2_32.dll", "WSAConnectByNameA", MyWSAConnectByNameA);
    ////////InstallHook("ws2_32.dll", "WSAConnectByNameW", MyWSAConnectByNameW);
    //InstallHook("ws2_32.dll", "WSAAsyncGetHostByAddr", MyWSAAsyncGetHostByAddr);
    //InstallHook("ws2_32.dll", "WSAAsyncGetHostByName", MyWSAAsyncGetHostByName);
    ////////InstallHook("ws2_32.dll", "WSARecv", MyWSARecv);
    //InstallHook("ws2_32.dll", "WSARecvFrom", MyWSARecvFrom);
    ////////InstallHook("ws2_32.dll", "WSASend", MyWSASend);
    ////////InstallHook("ws2_32.dll", "WSASendTo", MyWSASendTo);
    ////////InstallHook("ws2_32.dll", "WSASocketA", MyWSASocketA);
    ////////InstallHook("ws2_32.dll", "WSASocketW", MyWSASocketW);
    //////
    //InstallHook("ws2_32.dll", "getaddrinfo", MyGetAddrInfo);                          //
    //InstallHook("ws2_32.dll", "GetAddrInfoW", MyGetAddrInfoW);                        //
    //InstallHook("ws2_32.dll", "GetAddrInfoExW", MyGetAddrInfoExW);                        //
    //InstallHook("ws2_32.dll", "getnameinfo", MyGetNameInfo);                          //
    //InstallHook("ws2_32.dll", "GetNameInfoW", MyGetNameInfoW);                        //
    //InstallHook("ws2_32.dll", "WSALookupServiceNextA", MyWSALookupServiceNextA);      //
    //InstallHook("ws2_32.dll", "WSALookupServiceNextW", MyWSALookupServiceNextW);      //
    ////InstallHook("ws2_32.dll", "WSALookupServiceBeginA", MyWSALookupServiceBeginA);
    ////InstallHook("ws2_32.dll", "WSALookupServiceBeginW", MyWSALookupServiceBeginW);
    //////
    ///////*InstallHook("ws2_32.dll", "__WSAFDIsSet", MyWSAFDIsSet);
    //////InstallHook("ws2_32.dll", "htons", MyHtons);*/
    //////
    ////////
    //InstallHook("dnsapi.dll", "DnsQuery_A", MyDnsQuery_A);
    //InstallHook("dnsapi.dll", "DnsQuery_UTF8", MyDnsQuery_UTF8);
    //InstallHook("dnsapi.dll", "DnsQuery_W", MyDnsQuery_W);
    //InstallHook("dnsapi.dll", "DnsQueryEx", MyDnsQueryEx); //
    
    //InstallHook("kernel32.dll", "DeviceIoControl", MyDeviceIoControl);
    
    //InstallHook("kernel32.dll", "GetEnvironmentVariableA", MyGetEnvironmentVariableA);
    //InstallHook("kernel32.dll", "GetEnvironmentStringsW", MyGetEnvironmentStringsW);
    //InstallHook("kernel32.dll", "GetEnvironmentVariableW", MyGetEnvironmentVariableW);
    //InstallHook("kernel32.dll", "GetSystemInfo", MyGetSystemInfo);
    //InstallHook("kernel32.dll", "GetComputerNameA", MyGetComputerNameA);
    //InstallHook("kernel32.dll", "GetComputerNameW", MyGetComputerNameW);
    //InstallHook("kernel32.dll", "GetComputerNameExA", MyGetComputerNameExA);
    //InstallHook("kernel32.dll", "GetComputerNameExW", MyGetComputerNameExW);
    //InstallHook("kernel32.dll", "DnsHostnameToComputerNameA", MyDnsHostnameToComputerNameA);
    //InstallHook("kernel32.dll", "DnsHostnameToComputerNameW", MyDnsHostnameToComputerNameW);

    //InstallHook("kernel32.dll", "CreateMutexA", MyCreateMutexA);
    //InstallHook("kernel32.dll", "CreateMutexExA", MyCreateMutexExA);
    //InstallHook("kernel32.dll", "CreateMutexW", MyCreateMutexW);
    //InstallHook("kernel32.dll", "CreateMutexExW", MyCreateMutexExW);

    //InstallHook("kernel32.dll", "CreateFileA", MyCreateFileA);
    //InstallHook("kernel32.dll", "CreateFileW", MyCreateFileW);
    
    //
    ////InstallHook("advapi32.dll", "SetEntriesInAclW", MySetEntriesInAclW);
    ////InstallHook("advapi32.dll", "GetTokenInformation", MyGetTokenInformation);
    //InstallHook("advapi32.dll", "RegOpenKeyA", MyRegOpenKeyA);
    //InstallHook("advapi32.dll", "RegOpenKeyExA", MyRegOpenKeyExA);
    //InstallHook("advapi32.dll", "RegOpenKeyW", MyRegOpenKeyW);
    //InstallHook("advapi32.dll", "RegOpenKeyExW", MyRegOpenKeyExW);
    ////InstallHook("advapi32.dll", "RegCloseKey", MyRegCloseKey);
    //InstallHook("advapi32.dll", "RegQueryValueW", MyRegQueryValueW);
    //InstallHook("advapi32.dll", "RegQueryValueA", MyRegQueryValueA);
    //InstallHook("advapi32.dll", "RegQueryValueExW", MyRegQueryValueExW);
    //InstallHook("advapi32.dll", "RegQueryValueExA", MyRegQueryValueExA);
    //
    //////InstallHook("advapi32.dll", "ConvertSidToStringSidW", MyConvertSidToStringSidW);
    ////InstallHook("advapi32.dll", "RegQueryInfoKeyW", MyRegQueryInfoKeyW);
    ////InstallHook("advapi32.dll", "RegEnumKeyW", MyRegEnumKeyW);
    //InstallHook("advapi32.dll", "GetUserNameW", MyGetUserNameW);
    /////*InstallHook("advapi32.dll", "GetLengthSid", MyGetLengthSid);
    ////InstallHook("advapi32.dll", "OpenProcessToken", MyOpenProcessToken);
    ////InstallHook("advapi32.dll", "CopySid", MyCopySid);
    ////InstallHook("advapi32.dll", "IsValidSid", MyIsValidSid);*/
    /////*InstallHook("advapi32.dll", "SetSecurityDescriptorDacl", MySetSecurityDescriptorDacl);
    ////InstallHook("advapi32.dll", "InitializeSecurityDescriptor", MyInitializeSecurityDescriptor);*/


    //// The target address where we want to change the code
    //void* targetAddress = (void*)0x00007FF62FEE5BAF;

    //// The destination address where we want to jump
    //void* jumpDestination = (void*)0x00007FF62FEE5D38;

    //// Cast the pointers to intptr_t before subtraction to perform pointer arithmetic
    //intptr_t targetAddrInt = (intptr_t)targetAddress;
    //intptr_t jumpDestInt = (intptr_t)jumpDestination;

    //// Calculate the relative jump offset as a 32-bit integer
    //// The offset is from the next instruction, so subtract 5 bytes (size of the jmp instruction)
    //int32_t offset = (int32_t)(jumpDestInt - (targetAddrInt + 5));

    //// The jump instruction in x64 is E9 followed by a 4-byte relative offset
    //unsigned char jumpInstruction[5] = { 0xE9 };

    //// Copy the offset into the instruction (after the E9 opcode)
    //memcpy(jumpInstruction + 1, &offset, 4); // Use 4 bytes for the offset


    //// Change memory protection to allow writing to the code section
    //DWORD oldProtect;
    //if (VirtualProtect(targetAddress, sizeof(jumpInstruction), PAGE_EXECUTE_READWRITE, &oldProtect))
    //{
    //    // Write the jump instruction
    //    memcpy(targetAddress, jumpInstruction, sizeof(jumpInstruction));

    //    // Restore the original memory protection
    //    VirtualProtect(targetAddress, sizeof(jumpInstruction), oldProtect, &oldProtect);

    //    // Optionally, flush the instruction cache to ensure the CPU executes the modified code
    //    FlushInstructionCache(GetCurrentProcess(), targetAddress, sizeof(jumpInstruction));
    //}



    // Set up the vectored exception handler
    //AddVectoredExceptionHandler(1, VectoredExceptionHandler);

    //BYTE* instructionAddress = (BYTE*)0x00007FF62FEE5BAF; // The address to modify

    //// Set a hardware breakpoint
    //SetHardwareBreakpoint(instructionAddress);

}
