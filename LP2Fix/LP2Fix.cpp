// jailbreakhook.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include <thread>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <string>
#include <tlhelp32.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <mswsock.h>
//#include <iphlpapi.h>
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
#include <detours.h>
using namespace std;

HANDLE hLogFile = INVALID_HANDLE_VALUE;
TCHAR tempString[2048];
WCHAR tempStringW[2048];
char* targetIPAddress = "";
char* targetIPAddress2 = "";
const int MAX_PROCESS_ID_LENGTH = 10;
const int MAX_PORT_OFFSET_LENGTH = 2;
const char PADDING_CHAR = '#';
std::map<DWORD, std::tuple<std::string, std::string, int>> pidAddressMap;
int portOffset = 0;
SOCKET virtualSendSocket = INVALID_SOCKET;

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

NTSTATUS InstallHook(LPCSTR moduleHandle, LPCSTR proc, void* callBack, bool manual=false)
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

void LogCallStack()
{
    const int max_frames = 64;
    void* frame[max_frames];

    // Capture up to max_frames of the call stack
    int frames = CaptureStackBackTrace(0, max_frames, frame, NULL);

    // Initialize symbol handler
    SymInitialize(GetCurrentProcess(), NULL, TRUE);

    // Buffer sufficiently large to hold the symbol info, plus room for the name
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    // Buffer to hold the module info
    IMAGEHLP_MODULE64 moduleInfo;
    moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    // Iterate over the frames and print the function and module names
    for (int i = 0; i < frames; i++)
    {
        DWORD64 address = (DWORD64)(frame[i]);

        SymFromAddr(GetCurrentProcess(), address, 0, symbol);

        if (SymGetModuleInfo64(GetCurrentProcess(), address, &moduleInfo))
        {
            std::stringstream logMessage;
            logMessage << frames - i - 1 << ": " << moduleInfo.ModuleName << "!" << symbol->Name;
            Log(logMessage.str());
        }
        else
        {
            std::stringstream logMessage;
            logMessage << frames - i - 1 << ": " << symbol->Name;
            Log(logMessage.str());
        }
    }

    free(symbol);
}

// Define the type for the original function
typedef BOOL(WINAPI* GetComputerNameW_t)(LPWSTR, LPDWORD);

// Pointer to the original function
GetComputerNameW_t pGetComputerNameW = GetComputerNameW;

std::wstring GenerateRandomName()
{
    std::wstring characters = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::wstring randomName;
    for (int i = 0; i < 15; ++i) // 15 is the maximum length for a NetBIOS name
    {
        randomName += characters[distribution(generator)];
    }

    return randomName;
}

// Detour function
BOOL WINAPI DetourGetComputerNameW(LPWSTR lpBuffer, LPDWORD nSize)
{
    Log("GetComputerNameW");

    // Generate a random computer name
    std::wstring randomName = GenerateRandomName();

    // Check if the buffer is large enough
    if (*nSize < randomName.size() + 1) // +1 for the null terminator
    {
        // If not, set the required size and return FALSE
        *nSize = randomName.size() + 1;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    // Copy the random name to the buffer
    wcscpy(lpBuffer, randomName.c_str());

    // Set the size to the length of the random name
    *nSize = randomName.size();

    return TRUE;
}

//void InstallHook()
//{
//    DetourRestoreAfterWith();
//
//    DetourTransactionBegin();
//    DetourUpdateThread(GetCurrentThread());
//
//    void* proc = GetProcAddressManual(GetModuleHandle("kernel32.dll"), "GetComputerNameW");
//
//    // Attach the detour function
//    //DetourAttach(&(PVOID&)pGetComputerNameW, DetourGetComputerNameW);
//    DetourAttach(
//        &(PVOID&)proc,
//        DetourGetComputerNameW
//    );
//
//    DetourTransactionCommit();
//}

int WINAPI MyBind(SOCKET s, const struct sockaddr* name, int namelen)
{
    Log("MyBind");

    // Ensure the address is IPv4 (AF_INET)
    if (name->sa_family == AF_INET)
    {
        struct sockaddr_in modified_addr = {};
        memcpy(&modified_addr, name, sizeof(modified_addr));

        // Set IP to 0.0.0.0
        //modified_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //modified_addr.sin_addr.s_addr = inet_addr("192.168.0.12");

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(targetIPAddress);
        addr.sin_port = htons(3074 + portOffset);

        bind(s, (struct sockaddr*)&addr, sizeof(addr));

        return 0;

        //// Check the original port
        //USHORT originalPort = ntohs(modified_addr.sin_port);

        //if (originalPort == 3074) {
        //    //int optval = 1;
        //    //setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
        //    originalPort += portOffset + 1;
        //}

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

        //// If no ports are available, return error
        //return SOCKET_ERROR;
    }

    // For other cases, proceed as normal
    return bind(s, name, namelen);
}

int WINAPI MyGetSockName(SOCKET s, struct sockaddr* name, int* namelen)
{

    Log("MyGetSockName");

    // Set the address to 192.168.0.12
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(targetIPAddress2);
    addr.sin_port = htons(3074);

    // Copy the address to the buffer
    memcpy(name, &addr, sizeof(addr));

    // Set the length to the size of the address
    *namelen = sizeof(addr);

    return 0;

}

std::string GenerateRandomIPAddress()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254); // Avoid 0 and 255 in the last octet

    std::stringstream ss;
    ss << "192.168.0." << dis(gen);

    return ss.str();
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

int WINAPI MySetSocketOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    Log("MySetSocketOpt s: " + std::to_string(s));
    Log("MySetSocketOpt level: " + std::to_string(level));
    Log("MySetSocketOpt optname: " + std::to_string(optname));
    Log("MySetSocketOpt optlen: " + std::to_string(optlen));
    Log("MySetSocketOpt optval: " + std::to_string(*optval));

    if (level == SOL_SOCKET)
    {
        Log("SOL_SOCKET");
        int option = 1;
        /*int result = setsockopt(
            s, SOL_SOCKET, 
            SO_BROADCAST | SO_DONTROUTE | SO_KEEPALIVE | SO_LINGER | SO_OOBINLINE | TCP_NODELAY | 4294954560, 
            (const char*)&option, sizeof(option)
        );*/
        //setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));

        //setsockopt(s, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&option, sizeof(option));
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, TCP_NODELAY, (char*)&option, sizeof(option));
        //setsockopt(s, SOL_SOCKET, 4294954560, (char*)&option, sizeof(option));
        
        return 0;
        /*if (result == -1) {
            int errorCode = WSAGetLastError();
            Log("setsockopt failed with error: " + to_string(errorCode));
        }
        return result;*/
    }

    
    /*if (optname == SO_CONDITIONAL_ACCEPT)
    {
        Log("SO_CONDITIONAL_ACCEPT");
    }*/

    //// Enable SO_BROADCAST for the socket
    //int broadcast = 1;
    //setsockopt(s, level, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

    //return 0;

    return setsockopt(s, level, optname, optval, optlen);
}

int WINAPI MyIoCtlSocket(SOCKET s, long cmd, u_long* argp)
{
    Log("MyIoCtlSocket s: " + std::to_string(s));
    Log("MyIoCtlSocket cmd: " + std::to_string(cmd));
    Log("MyIoCtlSocket argp: " + std::to_string(*argp));

    return ioctlsocket(s, cmd, argp);
}

int WINAPI MySendTo2(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
{
    Log("MySendTo2");

    // Get the current process ID
    DWORD processId = GetCurrentProcessId();

    // Convert the process ID to a string
    std::string processIdStr = std::to_string(processId);

    // Pad the processIdStr to the maximum length
    processIdStr.resize(MAX_PROCESS_ID_LENGTH - 1, PADDING_CHAR);

    // Create a copy of the to address
    sockaddr_in* toCopy = reinterpret_cast<sockaddr_in*>(const_cast<sockaddr*>(to));

    // Convert the to address to a string
    std::string toAddress = inet_ntoa(toCopy->sin_addr);
    u_short toPort = ntohs(toCopy->sin_port);

    Log("MySendTo Process ID: " + processIdStr);
    Log("MySendTo IPAddress: " + toAddress + " " + std::to_string(toPort));

    // Search the ipAddressMap for a value that matches the to address
    for (const auto& pair : pidAddressMap)
    {
        if (std::get<1>(pair.second) == toAddress.c_str())
        {
            // If a match is found, replace the to address with the corresponding key
            toCopy->sin_addr.s_addr = inet_addr(std::get<0>(pair.second).c_str());
            toCopy->sin_port = htons(ntohs(toCopy->sin_port) + std::get<2>(pair.second));

            Log("MySendTo realIPAddress: " + std::get<0>(pair.second) + " " + to_string(std::get<2>(pair.second)) + " " + to_string(toCopy->sin_port));
            break;
        }
    }

    // Convert the portOffset to a string
    std::string portOffsetStr = std::to_string(portOffset);

    // Pad the portOffsetStr to the maximum length of 2 digits
    portOffsetStr.resize(MAX_PORT_OFFSET_LENGTH, PADDING_CHAR);

    // Prepend the padded processIdStr, portOffsetStr to the buffer
    std::string modifiedBuf = processIdStr + portOffsetStr + std::string(buf, buf + len);
    //std::string modifiedBuf = std::string(buf, buf + len);


    //// Split the IP address into octets
    //std::vector<std::string> octets;
    //std::stringstream ss(toAddress);
    //std::string octet;
    //while (std::getline(ss, octet, '.')) {
    //    octets.push_back(octet);
    //}

    //// Check if the last octet is "255"
    //if (octets.back() == "255") {
    //    int result = 0;
    //    USHORT originalPort = ntohs(toCopy->sin_port);

    //    // If the last three digits are 255, send the packet to the destination port and the following 3 ports
    //    for (int i = 0; i < 8; i++) {
    //        toCopy->sin_port = htons(originalPort + i);
    //        result = sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (sockaddr*)toCopy, tolen);
    //        /*if (result == SOCKET_ERROR) {
    //            return result;
    //        }*/
    //    }

    //    return result;
    //}


    // Call the original sendto function with the modified buffer
    return sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (sockaddr*)toCopy, tolen);
}

// Define a type for the WSARecvMsg function
typedef int (WSAAPI* LPFN_WSARECVMSG) (SOCKET, LPWSAMSG, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

// Pointer to the original function
LPFN_WSARECVMSG pWSARecvMsg = NULL;

// Proxy function
int WSAAPI ProxyWSARecvMsg(
    SOCKET s, 
    LPWSAMSG lpMsg, 
    LPDWORD lpdwNumberOfBytesRecvd, 
    LPWSAOVERLAPPED lpOverlapped, 
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
) {
    Log("ProxyWSARecvMsg");

    // Log the control codes in the lpMsg structure
    //Log("lpMsg->Control.buf: ");
    //printRawBytes(lpMsg->Control.buf, lpMsg->Control.len);

    return pWSARecvMsg(s, lpMsg, lpdwNumberOfBytesRecvd, lpOverlapped, lpCompletionRoutine);

    //WSAMSG msgCopy = *lpMsg;
    // Create a new WSAMSG structure to store the modified buffer
    /*WSAMSG msgCopy = {};
    msgCopy.dwBufferCount = lpMsg->dwBufferCount;
    msgCopy.lpBuffers = new WSABUF[lpMsg->dwBufferCount];
    msgCopy.lpBuffers[0].buf = new char[lpMsg->lpBuffers->len];
    msgCopy.lpBuffers[0].len = lpMsg->lpBuffers->len;
    memcpy(msgCopy.lpBuffers[0].buf, lpMsg->lpBuffers->buf, lpMsg->lpBuffers->len);*/

    int result = pWSARecvMsg(s, lpMsg, lpdwNumberOfBytesRecvd, lpOverlapped, lpCompletionRoutine);
    
    if (result == 0)
    {
        sockaddr_in* from = reinterpret_cast<sockaddr_in*>(lpMsg);

        std::string realIPAddress = inet_ntoa(from->sin_addr);
        u_short realIPPort = ntohs(from->sin_port);

        // Extract the process ID from the start of the buffer
        std::string processIdStr(lpMsg->lpBuffers->buf, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1);
        processIdStr.erase(processIdStr.find(PADDING_CHAR));
        DWORD processId = std::stoul(processIdStr);

        /*if (processId == GetCurrentProcessId()) {
            return 0;
        }*/
        
        // Extract the PortOffset from the buffer
        std::string portOffsetStr(lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH + MAX_PORT_OFFSET_LENGTH);
        portOffsetStr.erase(portOffsetStr.find(PADDING_CHAR));
        int fromPortOffset = std::stoi(portOffsetStr);

        // Check if the process ID is in the map
        if (pidAddressMap.find(processId) == pidAddressMap.end())
        {
            // If it's not, generate a new virtual IP address and add it to the map
            std::string virtualIPAddress = GenerateRandomIPAddress();
            std::get<0>(pidAddressMap[processId]) = realIPAddress;
            std::get<1>(pidAddressMap[processId]) = virtualIPAddress;
            std::get<2>(pidAddressMap[processId]) = fromPortOffset;
        }

        // Get the virtual IP address from the map
        std::string virtualIPAddress = std::get<1>(pidAddressMap[processId]);

        Log("MyRecvFrom Process ID: " + processIdStr);
        Log("MyRecvFrom realIPAddress: " + realIPAddress + " " + std::to_string(realIPPort));
        Log("MyRecvFrom virtualIPAddress: " + virtualIPAddress);

        //from->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());
        //from->sin_port = htons(ntohs(from->sin_port) - fromPortOffset);

        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());
        //reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr.s_addr = inet_addr(realIPAddress.c_str());
        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_port = htons(3074);
        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_family = AF_INET;

        // Remove the process ID and port offset from the start of the buffer
        memmove(lpMsg->lpBuffers->buf, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH, *lpdwNumberOfBytesRecvd - (MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH));

        // Update the lpNumberOfBytesRecvd to reflect the new length of the buffer
        *lpdwNumberOfBytesRecvd -= MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH;

        Log("MyRecvFrom finalIPAddress: " + 
            std::string(inet_ntoa(reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr)) + " " + 
            std::to_string(ntohs((reinterpret_cast<sockaddr_in*>(lpMsg)->sin_port)))
        );
    }

    return result;

    //while (true)
    //{
    //    // Make a copy of lpMsg
    //    WSAMSG msgCopy = *lpMsg;

    //    int result = pWSARecvMsg(s, &msgCopy, lpdwNumberOfBytesRecvd, lpOverlapped, lpCompletionRoutine);

    //    if (result == 0)
    //    {
    //        // Create a copy of the to address
    //        sockaddr_in* from = reinterpret_cast<sockaddr_in*>(&msgCopy);

    //        std::string realIPAddress = inet_ntoa(from->sin_addr);
    //        u_short realIPPort = ntohs(from->sin_port);

    //        //// Extract the process ID from the start of the buffer
    //        //std::string processIdStr(lpMsg->lpBuffers->buf, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1);
    //        //processIdStr.erase(processIdStr.find(PADDING_CHAR));
    //        //DWORD processId = std::stoul(processIdStr);

    //        //// Extract the PortOffset from the buffer
    //        //std::string portOffsetStr(lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH + MAX_PORT_OFFSET_LENGTH);
    //        //portOffsetStr.erase(portOffsetStr.find(PADDING_CHAR));
    //        //int fromPortOffset = std::stoi(portOffsetStr);

    //        //if (processId == GetCurrentProcessId()) {
    //        //    continue;
    //        //}

    //        //// Check if the process ID is in the map
    //        //if (pidAddressMap.find(processId) == pidAddressMap.end())
    //        //{
    //        //    // If it's not, generate a new virtual IP address and add it to the map
    //        //    std::string virtualIPAddress = GenerateRandomIPAddress();
    //        //    std::get<0>(pidAddressMap[processId]) = realIPAddress;
    //        //    std::get<1>(pidAddressMap[processId]) = virtualIPAddress;
    //        //    std::get<2>(pidAddressMap[processId]) = fromPortOffset;
    //        //}

    //        //// Get the virtual IP address from the map
    //        //std::string virtualIPAddress = std::get<1>(pidAddressMap[processId]);

    //        //Log("MyRecvFrom Process ID: " + processIdStr);
    //        //Log("MyRecvFrom realIPAddress: " + realIPAddress + " " + std::to_string(realIPPort));
    //        //Log("MyRecvFrom virtualIPAddress: " + virtualIPAddress);

    //        ////from->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());
    //        ////from->sin_port = htons(ntohs(from->sin_port) + fromPortOffset);

    //        //reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());
    //        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr.s_addr = inet_addr(realIPAddress.c_str());
    //        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_port = htons(3074);
    //        reinterpret_cast<sockaddr_in*>(lpMsg)->sin_family = AF_INET;

    //        //// Remove the process ID and port offset from the start of the buffer
    //        //memmove(lpMsg->lpBuffers->buf, lpMsg->lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH, *lpdwNumberOfBytesRecvd - (MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH));

    //        //// Update the lpNumberOfBytesRecvd to reflect the new length of the buffer
    //        //*lpdwNumberOfBytesRecvd -= MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH;

    //        //Log("MyRecvFrom finalIPAddress: " + 
    //        //    std::string(inet_ntoa(reinterpret_cast<sockaddr_in*>(lpMsg)->sin_addr)) + " " + 
    //        //    std::to_string(ntohs((reinterpret_cast<sockaddr_in*>(lpMsg)->sin_port)))
    //        //);

    //    }

    //    return result;
    //}
}

int WINAPI MyWSAIoctl(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    Log("MyWSAIoctl s: " + std::to_string(s));
    Log("MyWSAIoctl dwIoControlCode: " + std::to_string(dwIoControlCode));

    //If the control code is SIO_GET_EXTENSION_FUNCTION_POINTER, log the function GUID
    if (dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER)
    {
        GUID* guid = (GUID*)lpvInBuffer;

        GUID GUID_WSAID_ACCEPTEX = WSAID_ACCEPTEX;
        GUID GUID_WSAID_CONNECTEX = WSAID_CONNECTEX;
        GUID GUID_WSAID_DISCONNECTEX = WSAID_DISCONNECTEX;
        GUID GUID_WSAID_GETACCEPTEXSOCKADDRS = WSAID_GETACCEPTEXSOCKADDRS;
        GUID GUID_WSAID_TRANSMITFILE = WSAID_TRANSMITFILE;
        GUID GUID_WSAID_TRANSMITPACKETS = WSAID_TRANSMITPACKETS;
        GUID GUID_WSAID_WSARECVMSG = WSAID_WSARECVMSG;
        GUID GUID_WSAID_WSASENDMSG = WSAID_WSASENDMSG;

        if (GUID_WSAID_ACCEPTEX.Data1 == guid->Data1)
        {
            Log("WSAID_ACCEPTEX");
        }
        else if (GUID_WSAID_CONNECTEX.Data1 == guid->Data1)
        {
            Log("WSAID_CONNECTEX");
        }
        else if (GUID_WSAID_DISCONNECTEX.Data1 == guid->Data1)
        {
            Log("WSAID_DISCONNECTEX");
        }
        else if (GUID_WSAID_GETACCEPTEXSOCKADDRS.Data1 == guid->Data1)
        {
            Log("WSAID_GETACCEPTEXSOCKADDRS");
        }
        else if (GUID_WSAID_TRANSMITFILE.Data1 == guid->Data1)
        {
            Log("WSAID_TRANSMITFILE");
        }
        else if (GUID_WSAID_TRANSMITPACKETS.Data1 == guid->Data1)
        {
            Log("WSAID_TRANSMITPACKETS");
        }
        else if (GUID_WSAID_WSARECVMSG.Data1 == guid->Data1)
        {
            Log("WSAID_WSARECVMSG");

            return WSAEINVAL;
            // Retrieve the original function pointer
            DWORD dwBytes;
            if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID_WSARECVMSG, sizeof(GUID_WSAID_WSARECVMSG), &pWSARecvMsg, sizeof(pWSARecvMsg), &dwBytes, NULL, NULL) == SOCKET_ERROR)
            {
                Log("WSAIoctl failed");
            }

            // Return the proxy function
            *(LPFN_WSARECVMSG*)lpvOutBuffer = ProxyWSARecvMsg;
            *lpcbBytesReturned = sizeof(LPFN_WSARECVMSG);

            return 0; // Indicate success
        }
        else if (GUID_WSAID_WSASENDMSG.Data1 == guid->Data1)
        {
            Log("WSAID_WSASENDMSG");
        }

    }

    //Log("MyWSAIoctl cbInBuffer: " + std::to_string(cbInBuffer));
    //Log("MyWSAIoctl cbOutBuffer: " + std::to_string(cbOutBuffer));

    return WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
}

int WINAPI MyWSAGetLastError()
{
    Log("MyWSAGetLastError");

    int result = WSAGetLastError();

    Log("MyWSAGetLastError result: " + std::to_string(result));

    return result;
}

int WINAPI MySendTo3(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
{
    Log("MySendTo3");

    sockaddr_in* toCopy = reinterpret_cast<sockaddr_in*>(const_cast<sockaddr*>(to));
    if (toCopy->sin_addr.s_addr == INADDR_BROADCAST)
    {
        /*if (strcmp(targetIPAddress, "169.254.128.201") == 0)
        {
            toCopy->sin_addr.s_addr = inet_addr("169.254.149.218");
        }
        else if (strcmp(targetIPAddress, "169.254.149.218") == 0)
        {
            toCopy->sin_addr.s_addr = inet_addr("169.254.128.201");
        }*/

        /*const char* ipAddresses[] = { "169.254.82.152", "169.254.128.201", "169.254.149.218" };
        
        for (const char* ipAddress : ipAddresses)
        {
            toCopy->sin_addr.s_addr = inet_addr(ipAddress);
            sendto(s, buf, len, flags, (sockaddr*)toCopy, tolen);
        }*/

        toCopy->sin_addr.s_addr = inet_addr("127.0.0.255");

        return sendto(s, buf, len, flags, (sockaddr*)toCopy, tolen);
    }

    return sendto(s, buf, len, flags, to, tolen);
}

__declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    string log = "lp2fix" + to_string(GetCurrentProcessId()) + ".log";
    hLogFile = CreateFile(log.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): LP2Fix.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    std::string iniFilePath = "";
    TCHAR executablePath[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, executablePath)) {
        iniFilePath = std::string(executablePath) + "\\plugins\\LP2Fix.ini";
        //iniFilePath = std::string(executablePath) + "\\iphlpapi.ini";
    }
    
    char targetIPBuffer[256];
    GetPrivateProfileString("Settings", "TargetIPAddress", "", targetIPBuffer, sizeof(targetIPBuffer), iniFilePath.c_str());
    
    targetIPAddress = new char[strlen(targetIPBuffer) + 1];
    std::strcpy(targetIPAddress, targetIPBuffer);
    
    Log("TargetIPAddress");
    Log(targetIPAddress);

    char targetIPBuffer2[256];
    GetPrivateProfileString("Settings", "TargetIPAddress2", "", targetIPBuffer2, sizeof(targetIPBuffer2), iniFilePath.c_str());

    targetIPAddress2 = new char[strlen(targetIPBuffer2) + 1];
    std::strcpy(targetIPAddress2, targetIPBuffer2);

    Log("TargetIPAddress2");
    Log(targetIPAddress2);
    
    // Load the portOffset value
    portOffset = GetPrivateProfileInt("Settings", "PortOffset", 0, iniFilePath.c_str());
    Log("PortOffset: ");
    Log(std::to_string(portOffset));

    // Initialize Winsock
    //WSADATA wsaData;
    //WSAStartup(MAKEWORD(2, 2), &wsaData);

    //// Create a socket with broadcast enabled
    //virtualSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //int broadcast = 1;
    //setsockopt(virtualSendSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
    //// bind to anyaddr on port 3074 + portOffset
    //sockaddr_in addr;
    //addr.sin_family = AF_INET;
    //addr.sin_port = htons(3074 + portOffset);
    //addr.sin_addr.s_addr = INADDR_ANY;
    //bind(virtualSendSocket, (sockaddr*)&addr, sizeof(addr));


    //InstallHook("ws2_32.dll", "sendto", MySendTo2);
    //Log("Installed sendto hook");

    //InstallHook("ws2_32.dll", "WSAIoctl", MyWSAIoctl);

    InstallHook("ws2_32.dll", "bind", MyBind);
    Log("Installed bind hook");

    InstallHook("ws2_32.dll", "getsockname", MyGetSockName);

    //InstallHook("ws2_32.dll", "setsockopt", MySetSocketOpt);

    //InstallHook("ws2_32.dll", "WSAGetLastError", MyWSAGetLastError);

}
