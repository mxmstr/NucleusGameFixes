// jailbreakhook.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include <sstream>
#include <string>
#include <winsock2.h>
#include <detours.h>
#include <windns.h>
#include <psapi.h>
#include <intrin.h>
#include <random>
#include <map>
using namespace std;

SOCKET broadcastSocket = INVALID_SOCKET;
int portOffset = 0;
SOCKET mainSocketOriginal = INVALID_SOCKET;
SOCKET mainSocketCurrent = INVALID_SOCKET;
char* targetIPAddress = "";
char* targetHostname = "Player1";
MODULEINFO mainModuleInfo;
const int MAX_PROCESS_ID_LENGTH = 10;
const int MAX_PORT_OFFSET_LENGTH = 2;
const char PADDING_CHAR = '#';
std::map<DWORD, std::tuple<std::string, std::string, int>> pidAddressMap;

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
        //modified_addr.sin_addr.s_addr = inet_addr(targetIPAddress);

        // Check the original port
        USHORT originalPort = ntohs(modified_addr.sin_port);

        if (originalPort == 26001) {
            originalPort += portOffset;
        }

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

std::string GenerateRandomIPAddress()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254); // Avoid 0 and 255 in the last octet

    std::stringstream ss;
    ss << "192.168.0." << dis(gen);

    return ss.str();
}

int WINAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
{
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

    /*Log("MySendTo Process ID: " + processIdStr);
    Log("MySendTo realIPAddress: " + toAddress + " " + std::to_string(toPort));*/

    // Search the ipAddressMap for a value that matches the to address
    for (const auto& pair : pidAddressMap)
    {
        if (std::get<1>(pair.second) == toAddress.c_str())
        {
            // If a match is found, replace the to address with the corresponding key
            toCopy->sin_addr.s_addr = inet_addr(std::get<0>(pair.second).c_str());
            toCopy->sin_port = htons(ntohs(toCopy->sin_port) + std::get<2>(pair.second));

            //Log("MySendTo virtualIPAddress: " + std::get<0>(pair.second));
            break;
        }
    }

    // Convert the portOffset to a string
    std::string portOffsetStr = std::to_string(portOffset);

    // Pad the portOffsetStr to the maximum length of 2 digits
    portOffsetStr.resize(MAX_PORT_OFFSET_LENGTH, PADDING_CHAR);

    // Prepend the padded processIdStr, portOffsetStr to the buffer
    std::string modifiedBuf = processIdStr + portOffsetStr + std::string(buf, buf + len);


    // Split the IP address into octets
    std::vector<std::string> octets;
    std::stringstream ss(toAddress);
    std::string octet;
    while (std::getline(ss, octet, '.')) {
        octets.push_back(octet);
    }

    // Check if the last octet is "255"
    if (octets.back() == "255") {
        int result = 0;
        USHORT originalPort = ntohs(toCopy->sin_port);

        // If the last three digits are 255, send the packet to the destination port and the following 3 ports
        for (int i = 0; i < 8; i++) {
            toCopy->sin_port = htons(originalPort + i);
            result = sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (sockaddr*)toCopy, tolen);
            /*if (result == SOCKET_ERROR) {
                return result;
            }*/
        }

        return result;
    }


    // Call the original sendto function with the modified buffer
    return sendto(s, modifiedBuf.c_str(), modifiedBuf.size(), flags, (sockaddr*)toCopy, tolen);
}

int WINAPI MyWSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    // Initialize the result
    int result = WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);

    if (result == 0)
    {
        // Create a copy of the to address
        sockaddr_in* from = reinterpret_cast<sockaddr_in*>(const_cast<sockaddr*>(lpFrom));

        std::string realIPAddress = inet_ntoa(from->sin_addr);
        u_short realIPPort = ntohs(from->sin_port);

        // Extract the process ID from the start of the buffer
        std::string processIdStr(lpBuffers->buf, lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1);
        processIdStr.erase(processIdStr.find(PADDING_CHAR));
        DWORD processId = std::stoul(processIdStr);

        // Extract the PortOffset from the buffer
        std::string portOffsetStr(lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1, lpBuffers->buf + MAX_PROCESS_ID_LENGTH + MAX_PORT_OFFSET_LENGTH);
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

        /*Log("MyRecvFrom Process ID: " + processIdStr);
        Log("MyRecvFrom realIPAddress: " + realIPAddress + " " + std::to_string(realIPPort));
        Log("MyRecvFrom virtualIPAddress: " + virtualIPAddress);*/

        from->sin_addr.s_addr = inet_addr(virtualIPAddress.c_str());
        from->sin_port = htons(ntohs(from->sin_port) - fromPortOffset);


        // Remove the process ID and port offset from the start of the buffer
        memmove(lpBuffers->buf, lpBuffers->buf + MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH, *lpNumberOfBytesRecvd - (MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH));

        // Update the lpNumberOfBytesRecvd to reflect the new length of the buffer
        *lpNumberOfBytesRecvd -= MAX_PROCESS_ID_LENGTH - 1 + MAX_PORT_OFFSET_LENGTH;
    }

    return result;
}

// Define a function pointer type that matches the signature of gethostname
typedef int (WSAAPI* GetHostNameFuncType)(char*, int);

// Declare a variable to hold the original gethostname function
GetHostNameFuncType TrueGetHostName = (GetHostNameFuncType)gethostname;

// Define your own gethostname function
int WSAAPI MyGetHostName(char* name, int namelen)
{
    // Call the original gethostname function
    int result = TrueGetHostName(name, namelen);

    // Modify the behavior of gethostname as needed
    // For example, you can log the hostname
    Log("Original Hostname: " + std::string(name));

    // Copy targetHostname into name
    strncpy(name, targetHostname, namelen - 1);
    name[namelen - 1] = '\0'; // Ensure null-termination

    Log("Modified Hostname: " + std::string(name));

    return result;
}

// Function to install the hook
void DInstallHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueGetHostName, MyGetHostName);
    DetourTransactionCommit();
}

// Function to remove the hook
void DRemoveHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueGetHostName, MyGetHostName);
    DetourTransactionCommit();
}

BCFIX_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("bcfix.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): BCFix.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    InstallHook("ws2_32.dll", "sendto", MySendTo);
    Log("Installed sendto hook");

    InstallHook("ws2_32.dll", "WSARecvFrom", MyWSARecvFrom);
    Log("Installed WSARecvFrom hook");

    InstallHook("ws2_32.dll", "bind", MyBind);
    Log("Installed bind hook");

    DInstallHook();

    /*targetHostname = getenv("TargetHostname");
    if (targetIPAddress != "") {
        Log("TargetHostname: ");
        Log(targetHostname);
    }
    else {
        Log("TargetHostname not found");
    }

    portOffset = std::atoi(getenv("PortOffset"));
    Log("PortOffset: ");
    Log(getenv("PortOffset"));*/

    // Allocate a buffer to hold the executable path
    std::string iniFilePath = "";

    TCHAR executablePath[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, executablePath)) {
        iniFilePath = std::string(executablePath) + "\\BCFix.ini";
    }

    //HANDLE currentProcess = GetCurrentProcess();
    //char executablePath[MAX_PATH + 1];  // +1 for the null terminator
    //if (GetModuleFileNameEx(currentProcess, NULL, executablePath, MAX_PATH) != 0)
    //{
    //    std::string executablePathStr = executablePath;
    //    size_t lastBackslash = executablePathStr.find_last_of('\\');
    //    std::string directory = executablePathStr.substr(0, lastBackslash);
    //    iniFilePath = directory + "\\BCFix.ini";
    //    Log(iniFilePath);
    //}

    // Load the targetHostname value
    char targetHostnameBuffer[256];
    GetPrivateProfileString("Settings", "TargetHostname", "DefaultHostname", targetHostnameBuffer, sizeof(targetHostnameBuffer), iniFilePath.c_str());
    
    targetHostname = new char[strlen(targetHostnameBuffer) + 1];  // +1 for the null terminator
    std::strcpy(targetHostname, targetHostnameBuffer);
    Log("TargetHostname: ");
    Log(targetHostname);

    // Load the portOffset value
    portOffset = GetPrivateProfileInt("Settings", "PortOffset", 0, iniFilePath.c_str());
    Log("PortOffset: ");
    Log(std::to_string(portOffset));
}