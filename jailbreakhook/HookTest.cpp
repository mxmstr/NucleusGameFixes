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

    // Call the original GetAdaptersInfo function
    //DWORD result = GetAdaptersInfo(pAdapterInfo, pOutBufLen);

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

int WINAPI MyWSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
    Log("MyWSAStartup");

    return WSAStartup(wVersionRequested, lpWSAData);
}

SOCKET WINAPI MySocket(int af, int type, int protocol)
{
    Log("MySocket");
    return socket(af, type, protocol);;
}

int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
{

    Log("MyBind");

    std::stringstream logMessage;
    logMessage << s << std::endl;
    Log(logMessage.str());

    // Ensure the address is IPv4 (AF_INET)
    if (name->sa_family == AF_INET)
    {
        struct sockaddr_in modified_addr = {};
        memcpy(&modified_addr, name, sizeof(modified_addr));

        // Set IP to 0.0.0.0
        modified_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //Log(targetIPAddress);
        //inet_pton(AF_INET, targetIPAddress, &(modified_addr.sin_addr.s_addr));

        // Check the original port
        USHORT originalPort = ntohs(modified_addr.sin_port);
        if (originalPort == 1200 || originalPort == 3074)
        {
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
        else
        {
            //originalPort = 50000;
            //// Find the first available port starting from originalPort
            //for (USHORT port = originalPort; port < 65535; ++port)
            //{
            //    modified_addr.sin_port = htons(port);
            //    if (bind(s, (struct sockaddr*)&modified_addr, sizeof(modified_addr)) == 0)
            //    {
            //        // Successfully bound
            //        return 0;
            //    }
            //}
            return bind(s, (struct sockaddr*)&modified_addr, sizeof(modified_addr));
        }
    }

    // For other cases, proceed as normal
    return bind(s, name, namelen);

    // Extract IP address and port from the sockaddr structure
    //if (name->sa_family == AF_INET)
    //{
    //    sockaddr_in* sockaddrIPv4 = (sockaddr_in*)name;
    //    char ipBuffer[INET_ADDRSTRLEN];
    //    inet_ntop(AF_INET, &(sockaddrIPv4->sin_addr), ipBuffer, INET_ADDRSTRLEN);
    //    USHORT port = ntohs(sockaddrIPv4->sin_port);

    //    std::stringstream logMessage;
    //    logMessage << "Bound IP Address: " << ipBuffer << std::endl;
    //    logMessage << "Bound Port: " << port << std::endl;
    //    Log(logMessage.str());
    //    
    //    if (inet_pton(AF_INET, targetIPAddress, &(sockaddrIPv4->sin_addr)) != 1)
    //    {
    //        Log("Failed to convert IP address");
    //        return SOCKET_ERROR;
    //    }
    //}
    //else if (name->sa_family == AF_INET6)
    //{
    //    sockaddr_in6* sockaddrIPv6 = (sockaddr_in6*)name;
    //    char ipBuffer[INET6_ADDRSTRLEN];
    //    inet_ntop(AF_INET6, &(sockaddrIPv6->sin6_addr), ipBuffer, INET6_ADDRSTRLEN);
    //    USHORT port = ntohs(sockaddrIPv6->sin6_port);

    //    std::stringstream logMessage;
    //    logMessage << "Bound IP Address: " << ipBuffer << std::endl;
    //    logMessage << "Bound Port: " << port << std::endl;
    //    Log(logMessage.str());
    //}
    //else
    //{
    //    // Handle other address families if needed
    //}

    //return bind(s, name, namelen);
}

int WINAPI MyGetSockName(SOCKET s, struct sockaddr* name, int* namelen)
{
    std::stringstream logMessage;
    // Log information about the getsockname call
    logMessage << "getsockname intercepted!" << std::endl;
    logMessage << "Socket: " << s << std::endl;

    int result = getsockname(s, name, namelen);
    //
    //
    ///*if (inet_pton(AF_INET, "192.168.0.123", &(((struct sockaddr_in*)name)->sin_addr)) <= 0) {
    //    logMessage << "error" << result << std::endl;
    //}*/
    //
    //// Log information about the result and parameters
    //logMessage << "Result: " << result << std::endl;
    //
    //if (result == 0) {
    //    // Log the information returned by getsockname
    //    char ipAddress[INET_ADDRSTRLEN];
    //    inet_ntop(AF_INET, &(((struct sockaddr_in*)name)->sin_addr), ipAddress, INET_ADDRSTRLEN);
    //
    //    logMessage << "Address: " << ipAddress << std::endl;
    //    logMessage << "Port: " << ntohs(((struct sockaddr_in*)name)->sin_port) << std::endl;
    //}
    //
    //Log(logMessage.str());

    return result;//WSAEFAULT;
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

int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    // Log information about the sendto call
    std::stringstream logMessage;
    logMessage << "sendto intercepted!" << std::endl;
    logMessage << "Socket: " << s << std::endl;
    logMessage << "Length: " << len << std::endl;
    logMessage << "Flags: " << flags << std::endl;

    // Log information about the destination address
    if (to != nullptr && tolen >= sizeof(sockaddr_in))
    {
        const sockaddr_in* addr = reinterpret_cast<const sockaddr_in*>(to);

        char destAddrString[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &(addr->sin_addr), destAddrString, INET_ADDRSTRLEN) != nullptr)
        {
            logMessage << "MySendTo Address: " << destAddrString << std::endl;
            logMessage << "MySendTo Port: " << ntohs(addr->sin_port) << std::endl;
        }
        else
        {
            logMessage << "inet_ntop error " << std::endl;
        }
    }

    Log(logMessage.str());

    printRawBytes(buf, len);
    return sendto(s, buf, len, flags, to, tolen);


    const sockaddr_in* localAddress = reinterpret_cast<const sockaddr_in*>(to);

    // Check for the specific byte sequence
    const unsigned char JoinQueryHeader[] = {
        0x00, 0x00, 0x00, 0x00, 0x6a, 0xb0, 0xc0, 0xf4, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x77, 0xc9, 0x00, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x03, 0x00
    };

    const unsigned char JoinConfirmHeader[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xffffffe0, 0xffffffe5,
        0x48, 0x04
    };

    const unsigned char ClientRequestFunction = 0x2c;
    const unsigned char ServerResponseFunction = 0x37;

    static bool clientStarted = false;
    static unsigned char originalBytes1[6] = { 0 };
    static unsigned char newBytes1[6] = { 0 };

    static bool serverStarted = false;
    static unsigned char originalBytes2[6] = { 0 };
    static unsigned char newBytes2[6] = { 0 };


    if (clientStarted || serverStarted) {
        char* newBuffer = (char*)malloc(len);
        memcpy(newBuffer, buf, len);

        /*if (clientStarted) {
            for (int i = 0; i <= len - 6; ++i) {
                if (memcmp(newBuffer + i, originalBytes1, 6) == 0) {
                    const unsigned char newBytes[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
                    memcpy(newBuffer + i, newBytes, 6);
                }
            }
        }
        if (serverStarted) {
            for (int i = 0; i <= len - 6; ++i) {
                if (memcmp(newBuffer + i, originalBytes2, 6) == 0) {
                    const unsigned char newBytes[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
                    memcpy(newBuffer + i, newBytes, 6);
                }
            }
        }*/
        printRawBytes(newBuffer, len);
        int result = sendto(s, newBuffer, len, flags, to, tolen);
        free(newBuffer);
        return result;
    }


    if (len > sizeof(JoinQueryHeader) && std::memcmp(buf, JoinQueryHeader, sizeof(JoinQueryHeader)) == 0) {

        unsigned char nextByte = buf[sizeof(JoinQueryHeader)];

        if (nextByte == ClientRequestFunction) {
            Log("Client Request");

            char* newBuffer = (char*)malloc(len);
            memcpy(newBuffer, buf, len);

            // Capture the original bytes
            memcpy(originalBytes1, newBuffer + 68, 6);

            // Generate random bytes
            /*std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            for (int i = 0; i < 6; ++i) {
                newBytes1[i] = static_cast<unsigned char>(dis(gen));
            }*/

            // Replace with new random bytes
            //memcpy(newBuffer + 68, newBytes1, 6);
            const unsigned char newBytes[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
            //memcpy(newBuffer + 68, newBytes, 6);

            clientStarted = true;

            printRawBytes(newBuffer, len);
            int result = sendto(s, newBuffer, len, flags, to, tolen);
            free(newBuffer);

            return result;
        }
        //if (nextByte == ClientRequestFunction) {//&& strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
        //    Log("Client Request");

        //    char* newBuffer = (char*)malloc(len);
        //    memcpy(newBuffer, buf, len);

        //    /*const unsigned char newBytesA[] = { 0xffffff88 };
        //    std::memcpy(newBuffer + 56, newBytesA, sizeof(newBytesA));*/
        //    
        //    const unsigned char newBytesB[] = { 0xffffffad, 0x00000055, 0x00000036, 0xffffffa5, 0xffffffa5, 0xffffffa8 };
        //    std::memcpy(newBuffer + 68, newBytesB, sizeof(newBytesB));

        //    //////const unsigned char newBytes[] = { 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5, 0xa5, 0xa8 };
        //    //const unsigned char newBytes[] = { 0x00 };//, 0x00, 0x00, 0x00, 0x00, 0x00 };
        //    ////const unsigned char newBytes[] = { 0x55, 0xad, 0x36, 0xa5, 0xa5, 0xa8 };
        //    //std::memcpy(newBuffer + 68, newBytes, sizeof(newBytes));

        //    printRawBytes(newBuffer, len);
        //    int result = sendto(s, newBuffer, len, flags, to, tolen);
        //    free(newBuffer);

        //    return result;
        //}
        //else if (nextByte == ServerResponseFunction) {
        //    Log("Server Response");
        //    // 78 -> 0x10
        //    // 90 -> 0000005a ffffffaa 0000006d 0000004b 0000004b 00000050

        //    char* newBuffer = (char*)malloc(len);
        //    memcpy(newBuffer, buf, len);

        //    /*const unsigned char newBytesA[] = { 0xffffffb7, 0xffffffd4, 0xffffffb8, 0xfffffffd, 0x00000050 };
        //    std::memcpy(newBuffer + 45, newBytesA, sizeof(newBytesA));*/

        //    /*const unsigned char newBytesA[] = { 0x10 };
        //    std::memcpy(newBuffer + 78, newBytesA, sizeof(newBytesA));*/

        //    const unsigned char newBytesB[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
        //    std::memcpy(newBuffer + 90, newBytesB, sizeof(newBytesB));

        //    printRawBytes(newBuffer, len);
        //    int result = sendto(s, newBuffer, len, flags, to, tolen);
        //    free(newBuffer);

        //    return result;
        //}
        else if (nextByte == ServerResponseFunction) {
            Log("Server Response");

            char* newBuffer = (char*)malloc(len);
            memcpy(newBuffer, buf, len);

            // Capture the original bytes
            memcpy(originalBytes2, newBuffer + 90, 6);

            // Generate random bytes
            /*std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            for (int i = 0; i < 6; ++i) {
                newBytes2[i] = static_cast<unsigned char>(dis(gen));
            }*/

            // Replace with new random bytes
            //memcpy(newBuffer + 90, newBytes2, 6);
            const unsigned char newBytes[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
            //std::memcpy(newBuffer + 90, newBytes, 6);

            serverStarted = true;

            printRawBytes(newBuffer, len);
            int result = sendto(s, newBuffer, len, flags, to, tolen);
            free(newBuffer);

            return result;
        }
    }
    //else if (len > sizeof(JoinConfirmHeader) && std::memcmp(buf, JoinConfirmHeader, sizeof(JoinConfirmHeader)) == 0) {

    //    unsigned char nextByte = buf[sizeof(JoinQueryHeader)];

    //    if (nextByte == ClientRequestFunction) {//&& strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
    //        Log("Client Request");

    //        char* newBuffer = (char*)malloc(len);
    //        memcpy(newBuffer, buf, len);

    //        const unsigned char newBytesB[] = { 0xaa, 0x5a, 0x6d, 0x4b, 0x4b, 0x50 };
    //        std::memcpy(newBuffer + 39, newBytesB, sizeof(newBytesB));

    //        printRawBytes(newBuffer, len);
    //        int result = sendto(s, newBuffer, len, flags, to, tolen);
    //        free(newBuffer);

    //        return result;
    //    }
    //}
    /*else {

        char* newBuffer = (char*)malloc(len);
        memcpy(newBuffer, buf, len);

        for (int i = 0; i <= len - 6; ++i) {
            if (memcmp(newBuffer + i, originalBytes2, 6) == 0) {
                memcpy(newBuffer + i, newBytes2, 6);
            }
        }
    }*/

    printRawBytes(buf, len);
    return sendto(s, buf, len, flags, to, tolen);
}

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



int WINAPI MyRecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
    std::stringstream logMessage;

    int result = recvfrom(s, buf, len, flags, from, fromlen);

    sockaddr_in* localAddress = reinterpret_cast<sockaddr_in*>(from);

    if (strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) == 0)
        return WSAETIMEDOUT;

    /*logMessage << "MyRecvFrom Local Address: " << inet_ntoa(localAddress->sin_addr) << std::endl;
    logMessage << "MyRecvFrom Local Port: " << ntohs(localAddress->sin_port) << std::endl;
    logMessage << "MyRecvFrom Family: " << ntohs(localAddress->sin_family) << std::endl;
    logMessage << "MyRecvFrom: socket - " << s << std::endl;
    printRawBytes(buf, len);
    logMessage << "MyRecvFrom: Length - " << len << std::endl;
    logMessage << "MyRecvFrom: Flags - " << flags << std::endl;
    logMessage << "MyRecvFrom: From length - " << fromlen << std::endl;*/


    // Check for the specific byte sequence
    //const unsigned char JoinQueryHeader[] = {
    //    0x00, 0x00, 0x00, 0x00, 0x6a, 0xb0, 0xc0, 0xf4, 0x00, 0x00, 0x00, 0x01,
    //    0x00, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x77, 0xc9, 0x00, 0x00, 0x00, 0x03, 
    //    0x00, 0x00, 0x00, 0x03, 0x00
    //};
    //
    //const unsigned char ClientRequestFunction = 0x2c;
    //const unsigned char ServerResponseFunction = 0x37;
    //
    //if (result > sizeof(JoinQueryHeader) && std::memcmp(buf, JoinQueryHeader, sizeof(JoinQueryHeader)) == 0) {
    //
    //    // Check the next byte for "Join Request" or "Join Response"
    //    unsigned char nextByte = buf[sizeof(JoinQueryHeader)];
    //
    //    if (nextByte == ClientRequestFunction && strcmp(inet_ntoa(localAddress->sin_addr), targetIPAddress) != 0) {
    //        logMessage << "Client Request" << std::endl;
    //
    //        const unsigned char newBytes[] = { 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5, 0xa5, 0xa8 };
    //        std::memcpy(buf + 66, newBytes, sizeof(newBytes));
    //
    //        //std::memset(buf + 89, 0, len - 89);
    //
    //        //const unsigned char newBytes[] = { 0x26, 0x05, 0x40, 0x00, 0x88, 0x60, 0x14 };
    //        //std::memcpy(buf + 52, newBytes, sizeof(newBytes));
    //
    //        /*const unsigned char newBytes[] = {
    //            0x00, 0x00, 0x00, 0x00, 0x6a, 0xb0, 0xc0, 0xf4, 0x00, 0x00, 0x00, 0x01,
    //            0x00, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x77, 0xc9, 0x00, 0x00, 0x00, 0x03,
    //            0x00, 0x00, 0x00, 0x03, 0x00, 0x2c, 0x9f, 0x93, 0xa8, 0x58, 0x55, 0xff,
    //            0x9a, 0xac, 0x30, 0x3d, 0x01, 0xaf, 0x24, 0x3c, 0x50, 0xf0, 0x35, 0x22,
    //            0xe6, 0xbf, 0x9c, 0xb8, 0x26, 0x05, 0x40, 0x00, 0x88, 0x60, 0x14, 0xb8,
    //            0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x31, 0xad, 0x55, 0x36, 0xa5,
    //            0xa5, 0xa8, 0x18, 0x05, 0xe0, 0x00, 0x12, 0xec, 0x50, 0x60, 0x52, 0xee,
    //            0xd6, 0x50, 0x5a, 0x52, 0xf0, 0x70, 0x6b, 0xfe, 0xf6, 0x7f, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xcc, 0x4c, 0x3f, 0x0b,
    //            0xd7, 0xa3, 0x3e, 0x97, 0x00, 0x00, 0x00, 0x10, 0xa5, 0xb0, 0xf3, 0xd5,
    //            0x01, 0x00, 0x00, 0x30, 0xb8, 0x30, 0x84, 0xd5, 0x01, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd5, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0xf3, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0xc8, 0x9e, 0x6b, 0xfe, 0xf6, 0x7f, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //            0x00, 0x00, 0x00, 0x00
    //            };*/
    //
    //        //buf = new char[sizeof(newBytes)];
    //        //std::memcpy(buf, newBytes, sizeof(newBytes));
    //    }
    //    else if (nextByte == ServerResponseFunction) {
    //        logMessage << "Server Response" << std::endl;
    //    }
    //}


    
    // Modify the instruction
    //DWORD oldProtect;
    //BYTE* instructionAddress = (BYTE*)0x00007FF62FEE5BAF; // The address to modify

    //// Change memory protection to write
    //if (VirtualProtect(instructionAddress, 2, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    //    instructionAddress[0] = 0xEB; // Opcode for JMP (short jump)
    //    // Note: The operand (jump address) remains unchanged

    //    // Restore the original memory protection
    //    VirtualProtect(instructionAddress, 2, oldProtect, &oldProtect);
    //}
    //else {
    //    logMessage << "Failed to change memory protection." << std::endl;
    //}


    // Check if the call was successful and the buffer length is 0
    //if (result != SOCKET_ERROR && from != nullptr && from->sa_family == AF_INET)
    //{
    //    Log("Success");
    //    sockaddr_in* senderAddr = reinterpret_cast<sockaddr_in*>(from);

    //    // Check if the sender's IPv4 address matches the specified address
    //    char senderIP[INET_ADDRSTRLEN];
    //    inet_ntop(AF_INET, &(senderAddr->sin_addr), senderIP, INET_ADDRSTRLEN);
    //    inet_pton(AF_INET, "192.168.1.2", &(senderAddr->sin_addr));
    //    senderAddr->sin_port = htons(12345);

    //    std::string hexString = "00000000 00000000 00000000 00000000 0000006a ffffffb0 ffffffc0 fffffff4 00000000 00000000 00000000 00000001 00000000 00000000 00000000 0000000b 00000000 00000002 00000077 ffffffc9 00000000 00000000 00000000 00000003 00000000 00000000 00000000 00000003 00000000 0000002c ffffff9f";
    //    hexString.erase(std::remove(hexString.begin(), hexString.end(), ' '), hexString.end());
    //    size_t bufferSize = hexString.length() / 2;
    //    char* buffer = new char[bufferSize];
    //    hexStringToCharArray(hexString, buffer);
    //    *buf = *buffer;

    //    logMessage << "Set content" << std::endl;

    //    if (strcmp(senderIP, hostIPAddress) == 0)
    //    {
    //        // Modify the sender's IPv4 address to 192.168.0.22
    //        /*inet_pton(AF_INET, "192.168.0.22", &(senderAddr->sin_addr));
    //        senderAddr->sin_port = htons(56515);
    //        *fromlen = 0x00000063E89FF890;*/
    //        std::string hexString = "00000000 00000000 00000000 00000000 0000006a ffffffb0 ffffffc0 fffffff4 00000000 00000000 00000000 00000001 00000000 00000000 00000000 0000000b 00000000 00000002 00000077 ffffffc9 00000000 00000000 00000000 00000003 00000000 00000000 00000000 00000003 00000000 00000037 ffffffc7 0000000d ffffff88 fffffff7 0000003b 0000005b 00000002 00000028 ffffffab ffffffa2 ffffffa9 00000060 00000000 00000000 00000009 00000075 00000060 ffffff9b fffffffd ffffff85 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ffffff8f ffffffff ffffffff ffffffff fffffffc 0000003c 00000021 00000000 00000008 0000001f ffffffe6 ffffffa4 0000005c ffffffd7 fffffff3 ffffff97 00000004 ffffffc0 ffffffa8 00000000 0000000f 0000000c 00000002 ffffff97 00000004 00000000 00000000 00000000 00000000 00000000 00000000 ffffff86 00000033 00000020 ffffffdd 00000074 00000001 00000004 ffffff83 00000000 ffffffbc 00000000 00000002 0000005d ffffff8a 0000000c 0000000a 0000005d ffffffda ffffffca 0000000b 0000004a 0000005e 00000000";
    //        hexString.erase(std::remove(hexString.begin(), hexString.end(), ' '), hexString.end());
    //        size_t bufferSize = hexString.length() / 2;
    //        char* buffer = new char[bufferSize];
    //        hexStringToCharArray(hexString, buffer);

    //        logMessage << "Force send" << std::endl;
    //        MySendTo(s, buffer, bufferSize, flags, from, *fromlen);
    //    }
    //}

    /*localAddress = reinterpret_cast<sockaddr_in*>(from);

    logMessage << "MyRecvFrom Local Address 2: " << inet_ntoa(localAddress->sin_addr) << std::endl;
    logMessage << "MyRecvFrom Local Port 2: " << ntohs(localAddress->sin_port) << std::endl;
    logMessage << "MyRecvFrom Family 2: " << ntohs(localAddress->sin_family) << std::endl;
    logMessage << "MyRecvFrom: socket 2 - " << s << std::endl;
    logMessage << "MyRecvFrom: Received data 2 - " << buf << std::endl;
    logMessage << "MyRecvFrom: Length 2 - " << len << std::endl;
    logMessage << "MyRecvFrom: Flags 2 - " << flags << std::endl;
    logMessage << "MyRecvFrom: From length 2 - " << fromlen << std::endl;*/

    //Log(logMessage.str());

    return result;
}

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

DWORD WINAPI MyGetOwnerModuleFromUdpEntry(PMIB_UDPROW_OWNER_MODULE pUdpEntry, TCPIP_OWNER_MODULE_INFO_CLASS Class, PVOID pBuffer, PDWORD pdwSize)
{
    std::stringstream logMessage;

    DWORD result = GetOwnerModuleFromUdpEntry(pUdpEntry, Class, pBuffer, pdwSize);

    Log(logMessage.str());

    return result;
}

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
    strncpy(name, targetHostname, namelen - 1);
    name[namelen - 1] = '\0'; // Ensure null-termination
    
    // Log the modified host name
    Log("Modified Host Name:");
    Log(name);

    return result;
}

hostent* MyGetHostByName(const char* name)
{
    Log("MyGetHostByName");
    std::stringstream logMessage;
    // Call the original gethostbyname function
    hostent* result = gethostbyname(name);
    // 
    // Allocate memory for the custom hostent structure
    //hostent* customHostEnt = new hostent;
    //
    //// Set the hostent fields
    //customHostEnt->h_name = _strdup(targetHostname);  // Set the host name
    //customHostEnt->h_aliases = nullptr;           // No aliases for simplicity
    //customHostEnt->h_addrtype = AF_INET;          // IPv4 address
    //customHostEnt->h_length = sizeof(in_addr);    // Length of address
    //customHostEnt->h_addr_list = new char* [2];    // Allocate an array of pointers
    //
    //// Allocate memory for the custom IP address and copy it
    //in_addr customAddress;
    //inet_pton(AF_INET, targetIPAddress, &customAddress);
    //customHostEnt->h_addr_list[0] = new char[sizeof(in_addr)];
    //memcpy(customHostEnt->h_addr_list[0], &customAddress, sizeof(in_addr));

    //// Terminate the list with a nullptr
    //customHostEnt->h_addr_list[1] = nullptr;

    // Access and print the custom hostent information
    logMessage << "Original Host Name: " << name << std::endl;
    //logMessage << "Custom Host Name: " << customHostEnt->h_name << std::endl;
    //logMessage << "Custom IP Address: " << inet_ntoa(customAddress) << std::endl;
    //
    //// Clean up memory
    //delete[] customHostEnt->h_name;
    //delete[] customHostEnt->h_addr_list[0];
    //delete[] customHostEnt->h_addr_list;
    //delete customHostEnt;

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
    return WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
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
    // Call the original getaddrinfo function
    INT result = getaddrinfo(originalHostname, pServiceName, pHints, ppResult);

    // Log after the getaddrinfo call
    Log("getaddrinfo");
    Log(originalHostname);
    //Log(pServiceName);

    // Log additional information if needed, e.g., results
    if (result == 0 && *ppResult != nullptr) {
        // Traverse the linked list and keep only the matching result
        PADDRINFOA pCurrent = *ppResult;
        PADDRINFOA pPrev = nullptr;

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

INT WINAPI MyGetAddrInfoW(PCWSTR pNodeName, PCWSTR pServiceName, const ADDRINFOW* pHints, PADDRINFOW* ppResult)
{
    std::wstring myWideString = ConvertToWideString(originalHostname);
    // Call the original getaddrinfo function
    INT result = GetAddrInfoW(myWideString.c_str(), pServiceName, pHints, ppResult);

    //// Log after the getaddrinfo call
    Log("GetAddrInfoW");
    LogW(myWideString.c_str());

    //// Log additional information if needed, e.g., results
    if (result == 0 && *ppResult != nullptr) {
        // Traverse the linked list and keep only the matching result
        PADDRINFOW pCurrent = *ppResult;
        PADDRINFOW pPrev = nullptr;

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

    return ERROR_NOT_FOUND;

    // Call the original function
    //DWORD result = GetUnicastIpAddressTable(Family, Table);

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
        return m_decorated->CreateInstanceEnum(strClass, lFlags, pCtx, ppEnum);
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

    //return REGDB_E_CLASSNOTREG;

    WCHAR guidString[40] = { 0 };
    StringFromGUID2(riid, guidString, 40);

    std::wstringstream logMessage;
    logMessage << L"CoCreateInstance called. Requested interface IID: " << guidString;
    LogW(logMessage.str());

    HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    if (SUCCEEDED(result) && riid == __uuidof(IWbemLocator)) {
        Log("Decorator");
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

HOOKTEST_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("jailbreak.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): jailbreakhook.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }


    targetIPAddress = getenv("TargetIPAddress");
    if (targetIPAddress != nullptr) {
        Log("TargetIPAddress: ");
        Log(targetIPAddress);
    }
    else {
        Log("TargetIPAddress not found");
    }


    // Initialize random seed
    srand(static_cast<unsigned int>(time(NULL)));

    GenerateRandomMACAddress(macAddress);


    /*strcpy(targetHostname, "DESKTOP-AAAAAAA");
    RandomAlphanumeric(targetHostname + 8, 7 + 1);*/

    Log("Rando");


    //targetIPAddress = "192.168.0.10";
    //hostIPAddress = "192.168.0.10";
    //targetHostname = "DESKTOP-BBBBBBB";
    //targetIPAddress = "192.168.0.15";
    //targetHostname = "DESKTOP-BBBBBBB";

    //InstallHook("ole32.dll", "CoInitializeEx", MyCoInitializeEx);
    /*InstallHook("ole32.dll", "CoCreateInstance", MyCoCreateInstance);
    InstallHook("ole32.dll", "CoCreateInstanceEx", MyCoCreateInstanceEx);*/

    //InstallHook("kernel32", "LoadLibraryA", HookedLoadLibraryA);
    //InstallHook("Iphlpapi", "GetIpNetEntry2", Hooked_GetIpNetEntry2);
    //InstallHook("Iphlpapi", "GetIpNetTable2", Hooked_GetIpNetTable2);
    //InstallHook("Iphlpapi", "GetIfEntry2", Hooked_GetIfEntry2);
    //InstallHook("Iphlpapi", "GetIfEntry2Ex", Hooked_GetIfEntry2Ex);
    //InstallHook("Iphlpapi", "GetIfTable2", Hooked_GetIfTable2);
    //InstallHook("Iphlpapi", "GetIfTable2Ex", Hooked_GetIfTable2Ex);
    InstallHook("Iphlpapi", "GetAdaptersInfo", MyGetAdaptersInfo); //
    //InstallHook("Iphlpapi", "GetUnicastIpAddressTable", MyGetUnicastIpAddressTable); //
    //InstallHook("Iphlpapi", "GetAdaptersAddresses", HookedGetAdaptersAddresses);
    //InstallHook("Iphlpapi", "GetIfTable", HookedGetIfTable);
    //InstallHook("Iphlpapi", "GetIfEntry", HookedGetIfEntry);
    //InstallHook("Iphlpapi", "GetIpNetTable", HookedGetIpNetTable);
    //
    //InstallHook("ws2_32.dll", "WSAStartup", MyWSAStartup);
    //InstallHook("ws2_32.dll", "socket", MySocket);
    InstallHook("ws2_32.dll", "bind", MyBind); //
    ////InstallHook("ws2_32.dll", "getsockname", MyGetSockName);
    InstallHook("ws2_32.dll", "sendto", MySendTo); //
    //InstallHook("ws2_32.dll", "recvfrom", MyRecvFrom); //
    //InstallHook("ws2_32.dll", "closesocket", MyCloseSocket);
    //InstallHook("ws2_32.dll", "WSACleanup", MyWSACleanup);
    //
    //InstallHook("ws2_32.dll", "gethostname", MyGetHostName); //
    ////InstallHook("ws2_32.dll", "gethostbyname", MyGetHostByName); //
    ////////InstallHook("ws2_32.dll", "gethostbyaddr", MyGetHostByAddr);
    ////InstallHook("ws2_32.dll", "getnameinfo", MyGetNameInfo);
    /////////*InstallHook("ws2_32.dll", "GetNameInfo", MyGetNameInfo2);
    ////////InstallHook("ws2_32.dll", "GetNameInfoA", MyGetNameInfoA);*/
    ////InstallHook("ws2_32.dll", "GetNameInfoW", MyGetNameInfoW);
    ////////InstallHook("ws2_32.dll", "WSAEnumNetworkEvents", MyWSAEnumNetworkEvents);
    ////////InstallHook("ws2_32.dll", "WSAConnect", MyWSAConnect);
    ////////InstallHook("ws2_32.dll", "WSAConnectByList", MyWSAConnectByList);
    ////////InstallHook("ws2_32.dll", "WSAConnectByNameA", MyWSAConnectByNameA);
    ////////InstallHook("ws2_32.dll", "WSAConnectByNameW", MyWSAConnectByNameW);
    ////////InstallHook("ws2_32.dll", "WSAAsyncGetHostByAddr", MyWSAAsyncGetHostByAddr);
    ////////InstallHook("ws2_32.dll", "WSAAsyncGetHostByName", MyWSAAsyncGetHostByName);
    ////////InstallHook("ws2_32.dll", "WSARecv", MyWSARecv);
    ////////InstallHook("ws2_32.dll", "WSARecvFrom", MyWSARecvFrom);
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
    ////InstallHook("ws2_32.dll", "WSALookupServiceNextA", MyWSALookupServiceNextA);      //
    ////InstallHook("ws2_32.dll", "WSALookupServiceNextW", MyWSALookupServiceNextW);      //
    ////InstallHook("ws2_32.dll", "WSALookupServiceBeginA", MyWSALookupServiceBeginA);
    ////InstallHook("ws2_32.dll", "WSALookupServiceBeginW", MyWSALookupServiceBeginW);
    //////
    ///////*InstallHook("ws2_32.dll", "__WSAFDIsSet", MyWSAFDIsSet);
    //////InstallHook("ws2_32.dll", "htons", MyHtons);*/
    //////
    ////////InstallHook("Iphlpapi", "CreatePersistentUdpPortReservation", MyCreatePersistentUdpPortReservation);
    ////////InstallHook("Iphlpapi", "CreateProxyArpEntry", MyCreateProxyArpEntry);
    ////////InstallHook("Iphlpapi", "GetBestInterface", MyGetBestInterface);
    ////////InstallHook("Iphlpapi", "GetBestInterfaceEx", MyGetBestInterfaceEx);
    ////////InstallHook("Iphlpapi", "GetExtendedUdpTable", MyGetExtendedUdpTable);
    ////////InstallHook("Iphlpapi", "GetInterfaceInfo", MyGetInterfaceInfo);
    ////////InstallHook("Iphlpapi", "GetIpAddrTable", MyGetIpAddrTable);
    ////////InstallHook("Iphlpapi", "GetIpNetTable", MyGetIpNetTable);
    ////////InstallHook("Iphlpapi", "GetIpNetTable2", MyGetIpNetTable2);
    ////////InstallHook("Iphlpapi", "GetOwnerModuleFromUdpEntry", MyGetOwnerModuleFromUdpEntry);
    ////////InstallHook("Iphlpapi", "GetPerAdapterInfo", MyGetPerAdapterInfo);
    //////////InstallHook("Iphlpapi", "GetUdpTable", MyGetUdpTable);
    ////////
    ////////InstallHook("dnsapi.dll", "DnsQuery_A", MyDnsQuery_A);
    ////////InstallHook("dnsapi.dll", "DnsQuery_UTF8", MyDnsQuery_UTF8);
    ////////InstallHook("dnsapi.dll", "DnsQuery_W", MyDnsQuery_W);
    //InstallHook("dnsapi.dll", "DnsQueryEx", MyDnsQueryEx); //
    
    
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
    //
    ////InstallHook("advapi32.dll", "SetEntriesInAclW", MySetEntriesInAclW);
    ////InstallHook("advapi32.dll", "GetTokenInformation", MyGetTokenInformation);
    ////InstallHook("advapi32.dll", "RegOpenKeyA", MyRegOpenKeyA);
    ////InstallHook("advapi32.dll", "RegOpenKeyExA", MyRegOpenKeyExA);
    ////InstallHook("advapi32.dll", "RegOpenKeyW", MyRegOpenKeyW);
    ////InstallHook("advapi32.dll", "RegOpenKeyExW", MyRegOpenKeyExW);
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


    Log("Installed");

}
