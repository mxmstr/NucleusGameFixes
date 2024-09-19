#include "stdafx.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <iostream>
//#include <Python.h>
#include <winhttp.h>
#include <pybind11/embed.h>
#include <detours.h>
#include <iomanip>
#include <codecvt>
using namespace std;
namespace py = pybind11;

HANDLE hLogFile = INVALID_HANDLE_VALUE;
CHAR tempString[2048];
WCHAR tempStringW[2048];

LPCWSTR swzServerName;
LPCWSTR wszVerb;
LPCWSTR wszObjectName;
LPCWSTR headers;
LPVOID optional;
std::wstring encoded_response_buffer;
std::string encoded_response_bufferA;

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

    StringCchPrintfA(tempString, sizeof(tempString) / sizeof(CHAR), (logMessage.str() + "\r\n").c_str());
    WriteToLogFile(tempString, strlen(tempString) * sizeof(CHAR));
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
        GetProcAddress(GetModuleHandleA(moduleHandle), proc),
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

// Define types for detour functions
typedef HINTERNET(WINAPI* WinHttpConnect_t)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
typedef HINTERNET(WINAPI* WinHttpOpenRequest_t)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
typedef BOOL(WINAPI* WinHttpSendRequest_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL(WINAPI* WinHttpSetOption_t)(HINTERNET, DWORD, LPVOID, DWORD);
typedef BOOL(WINAPI* WinHttpReceiveResponse_t)(HINTERNET, LPVOID);
typedef BOOL(WINAPI* WinHttpQueryHeaders_t)(HINTERNET, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
typedef BOOL(WINAPI* WinHttpReadData_t)(HINTERNET, LPVOID, DWORD, LPDWORD);

// Define original functions
WinHttpConnect_t oWinHttpConnect = WinHttpConnect;
WinHttpOpenRequest_t oWinHttpOpenRequest = WinHttpOpenRequest;
WinHttpSendRequest_t oWinHttpSendRequest = WinHttpSendRequest;
WinHttpSetOption_t oWinHttpSetOption = WinHttpSetOption;
WinHttpReceiveResponse_t oWinHttpReceiveResponse = WinHttpReceiveResponse;
WinHttpQueryHeaders_t oWinHttpQueryHeaders = WinHttpQueryHeaders;
WinHttpReadData_t oWinHttpReadData = WinHttpReadData;

HINTERNET WINAPI MyWinHttpConnect(HINTERNET hSession, LPCWSTR pswzServerName, INTERNET_PORT nServerPort, DWORD dwReserved)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpConnect called with server name: " << pswzServerName;
    LogW(logMessage.str());

    swzServerName = pswzServerName;
    LogW(L"Server name: " + std::wstring(pswzServerName));

    //return WinHttpConnect(hSession, pswzServerName, nServerPort, dwReserved);
    return (void*)0x12345678;
}

HINTERNET WINAPI MyWinHttpOpenRequest(HINTERNET hConnect, LPCWSTR pwszVerb, LPCWSTR pwszObjectName, LPCWSTR pwszVersion, LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes, DWORD dwFlags)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpOpenRequest called with verb: " << pwszVerb << endl;
    logMessage << "WinHttpOpenRequest called with object name: " << pwszObjectName;
    LogW(logMessage.str());

    wszVerb = pwszVerb;
    wszObjectName = pwszObjectName;

    //return WinHttpOpenRequest(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer, ppwszAcceptTypes, dwFlags);
    return (void*)0x12345678;
}

BOOL WINAPI MyWinHttpSetOption(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpSetOption called with option: " << dwOption;
    LogW(logMessage.str());

    return TRUE;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;

    // Get the current working directory
    char currentDir[MAX_PATH];
    if (!GetCurrentDirectoryA(MAX_PATH, currentDir)) {
        throw std::runtime_error("GetCurrentDirectory failed");
    }

	Log("Current directory: " + std::string(currentDir));

    // Set up the security attributes struct.
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // Create the pipes for the child process's STDOUT.
    HANDLE hStdOutRead, hStdOutWrite;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        throw std::runtime_error("CreatePipe failed");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
        throw std::runtime_error("SetHandleInformation failed");
    }

    // Set up the start up info struct.
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hStdOutWrite;
    si.hStdOutput = hStdOutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW; // Make the command prompt window visible

    // Set up the process info struct.
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    // Create the child process.
    if (!CreateProcessA(NULL, const_cast<char*>(cmd), NULL, NULL, TRUE, 0, NULL, currentDir, &si, &pi)) {
		// Log last error
		std::wstringstream logMessage;
		logMessage << "CreateProcess failed with error: " << GetLastError();
		LogW(logMessage.str());

		throw std::runtime_error("CreateProcess failed");
    }

    // Close the write end of the pipe before reading from the read end of the pipe.
    CloseHandle(hStdOutWrite);

    // Read the output from the child process.
    DWORD bytesRead;
    while (ReadFile(hStdOutRead, buffer.data(), buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
        std::string line(buffer.data(), bytesRead);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        //LogW(L"line: " + converter.from_bytes(line)); // Log each line of output
        result += line;
    }

    // Wait for the child process to exit.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutRead);

    return result;
}

BOOL WINAPI MyWinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext)
{

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    //   std::wstringstream logMessage;
    //   logMessage << "WinHttpSendRequest called with headers: " << lpszHeaders;
    //   LogW(logMessage.str());

       //BOOL result = WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);

       //// Log lpszHeaders
       //LogW(L"lpszHeaders: " + std::wstring(lpszHeaders));

    //   // Create a string from lpOptional with the specified length
    //   std::wstring optionalData(static_cast<const wchar_t*>(lpOptional), dwOptionalLength / sizeof(wchar_t));

       //LogW(L"lpOptional: " + optionalData);

    //   // Log lpOptional as a full wide string
    //   //LogFullWideString(static_cast<const wchar_t*>(lpOptional), dwOptionalLength / sizeof(wchar_t));

       //// Log dwOptionalLength
       //Log("dwOptionalLength: " + std::to_string(dwOptionalLength));

       //// Log dwTotalLength
       //Log("dwTotalLength: " + std::to_string(dwTotalLength));

       //// Log dwContext
       //Log("dwContext: " + std::to_string(dwContext));

       //return result;

    headers = lpszHeaders;
    optional = lpOptional;

    // if wszObjectName starts with "/tppstmweb" or "/legal" set response to "La Li Lu Le Lo"
    if (wcsstr(wszObjectName, L"/tppstmweb") || wcsstr(wszObjectName, L"/legal")) {
        Log("Setting response to La Li Lu Le Lo");
        encoded_response_bufferA = "La Li Lu Le Lo";
        return TRUE;
    }

    std::wstring request = std::wstring(static_cast<const wchar_t*>(lpOptional), dwOptionalLength / sizeof(wchar_t));

    LogW(L"RequestW: " + request);

    // convert to short string
	//std::string request = std::string(optionalData.begin(), optionalData.end());

    //Log("Request: " + request);

    try {
		std::stringstream commandStream;
        std::string command = "cmd.exe /C cd plugins\\MGO3OPython && Python-3.8.0-64\\python.exe CommandProcessor.py ";
		commandStream << "cd plugins\\MGO3OPython && Python-3.8.0-64\\python.exe CommandProcessor.py ";
        //command += R"("YnHdLj/1b4SBvSa1/0bYhcd4UAB70VUx5O9bf1qPreoZthl/BZLQ76wlsSUACiuiZQbHR2TZczXAx1QOdz+MimZdqXl5kBwO4Qk4gCH2KahOdA9Q1HePoVE2yC6i+XcRcZ2EIbiOe36cahdUdtbS9tb4Lc/6wDCi3xr/d/QsbNNtcp+b0EJCs9gJvpl71Fn/Ra7uwIsdfd/QEm2TNoE0YfMcl9GqTi4xQrqzpBWpEgblnBGZVjEXNi8I9ePga4KN5DRQwpS6wxUlvGoceP5dCv7tHS944HKAnd6FUCwlWx3lqeC2Yq0OhzqFqr5Zy/5RqDx1DlLfpmcr9VP3C2T9LYR5ksUXGrov1ZIWKSYcPxDxuZNAna00UUtx2yRoymyJ4PvnagHu6o+pgCIl7Pj5hoxp77/p9fHjcj4Hp2kgRo1oiJIrA+XBOMZnzFsQEWXUNC9R7gBo2BNANz0f5O8Kioynl6Rg/wsHJRuX6+PMbnagu0k/GPCaYCb9mJ1uGXNaj8JMgc6ShCq+Jj67Z/Ki0+7Ox/7td3GJTj1FhocWWxccF4YlUo/W0SqUowFaNOI3224EQD1IuZAqkFchBfixkh5qPddbdVATVnVBPoGWeY9O/j/d3Y66x7ClQUVBKLWCje+XhDOydvpT5kLHulYMDbUL4/duW7u0ng9/C0HiDJdcn3CHuBOe4L+Il5hwXRWrhcszq/lsPf9RxLCVze1urVh6aBZktxliMNoVPWWhAMu74Pobeesa1Y/f5K6rqcw+XVy2khuXyYRauaYqVOrckVakUSJ721aPi/dJLS6ZZis07OSL3QhHaWYTlYB8niSWV7qW601zgqvS+QuH2Mo2bQ==")";

        request = request.substr(8 / sizeof(wchar_t));

        // convert request to c string, iterate each character and find first newline
        std::wstringstream requestFilteredStream;
		std::wstringstream logMessage;

		const wchar_t* request_cstr = request.c_str();

		for (size_t i = 0; i < dwOptionalLength; i++) {
            //logMessage << L"Checking character: " << request_cstr[i] << endl;

            if (request_cstr[i] == '\0') {
                //LogW(L"Found null at index: " + std::to_wstring(i));
                break;
            } 

			if (request_cstr[i] == '\r\n') {
				//LogW(L"Found newline at index: " + std::to_wstring(i));
                continue;
			}

            requestFilteredStream << request_cstr[i];
		}

		//LogW(logMessage.str());
		//LogW(requestFilteredStream.str());
		std::wstring requestFilteredW = requestFilteredStream.str();

	    //LogW(L"Request 2: " + requestFiltered);
        std::string requestFilteredA;
        for (wchar_t wc : requestFilteredW) {
            // Use a union to access each byte of the wchar_t
            union {
                wchar_t wide_char;
                char bytes[sizeof(wchar_t)];
            } converter;

            converter.wide_char = wc;

            // Append each byte as a char to the result string
            for (size_t i = 0; i < sizeof(wchar_t); ++i) {
                requestFilteredA += converter.bytes[i];
            }
        }

        Log("RequestA: " + requestFilteredA);

		//commandStream << "\"" << converter2.to_bytes(requestFiltered) << "\"";
        command += "\"" + requestFilteredA + "\" ";

		//LogW(L"Command: " + commandStream.str());
		// Convert command to wide string
        Log("Command: " + command);

        std::string result = exec(command.c_str());

        Log("Command output 1: " + result);
		/*while (result.find("\r\r") != std::string::npos) {
			result.erase(result.find("\r\r"), 2);
		}*/
        // remove returns and newlines
		/*while (result.find("\r") != std::string::npos) {
			result.erase(result.find("\r"), 1);
		}
        while (result.find("\n") != std::string::npos) {
            result.erase(result.find("\n"), 1);
        }*/

        //encoded_response_buffer = converter.from_bytes(result);
        encoded_response_bufferA = result;
        Log("Command output 2: " + result);
    }
    catch (const std::exception& e) {
        std::wstringstream err;
        err << "Error: " << e.what() << std::endl;
		LogW(err.str());
    }


	return TRUE;
}

BOOL WINAPI MyWinHttpWriteData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD lpdwNumberOfBytesWritten)
{
	LogW(L"WinHttpWriteData called");

	return WinHttpWriteData(hRequest, lpBuffer, dwNumberOfBytesToWrite, lpdwNumberOfBytesWritten);
}

BOOL WINAPI MyWinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpReceiveResponse called";
    LogW(logMessage.str());

    return TRUE;
}

BOOL WINAPI MyWinHttpQueryHeaders(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LogW(L"WinHttpQueryHeaders called");

	//BOOL result = WinHttpQueryHeaders(hRequest, dwInfoLevel, pwszName, lpBuffer, lpdwBufferLength, lpdwIndex);

 //   // Log the number of bytes to read
 //   Log("Number of bytes to read: " + std::to_string(*lpdwBufferLength));
 //   
 //   // Log lpBuffer as char array
 //   Log("lpBuffer: " + std::string(static_cast<CHAR*>(lpBuffer), *lpdwBufferLength));
 //   
 //   return result;

    unsigned int hexArray[] = { 0xC8, 0x0, 0x0, 0x0 };
    memcpy(lpBuffer, hexArray, 4);

    //Log lpBuffer as char array
    LogW(L"lpBuffer: " + std::wstring(static_cast<WCHAR*>(lpBuffer), *lpdwBufferLength));


    // Set the number of bytes written to the buffer
    *lpdwBufferLength = 4;

    return TRUE;
}

BOOL WINAPI MyWinHttpReadData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
{
    LogW(L"WinHttpReadData called");

    //BOOL result = WinHttpReadData(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    //
    //// Log the number of bytes to read
    //Log("Number of bytes to read: " + std::to_string(dwNumberOfBytesToRead));
    //
    //// Log the number of bytes read
    //Log("Number of bytes read: " + std::to_string(*lpdwNumberOfBytesRead));
    //
    //// Log lpBuffer 
    //std::stringstream logMessage;
    //logMessage << "lpBuffer: " << std::string(static_cast<CHAR*>(lpBuffer), *lpdwNumberOfBytesRead);
    //Log(logMessage.str());
    //
    //return result;

    static size_t position = 0; // Static variable to maintain position across calls

    // Reset position if the end of response is reached
    if (position >= encoded_response_bufferA.size())
    {
        Log("End of response reached");
        position = 0;
        *lpdwNumberOfBytesRead = 0;
        return TRUE;
    }

    Log("Number of bytes to read: " + std::to_string(dwNumberOfBytesToRead));

    // Calculate the number of bytes to copy
    size_t bytesRemaining = encoded_response_bufferA.size() - position;
    DWORD bytesToCopy = min(dwNumberOfBytesToRead, static_cast<DWORD>(bytesRemaining));

    // Copy the data to lpBuffer
    memcpy(lpBuffer, encoded_response_bufferA.data() + position, bytesToCopy);

    // Update the position index
    position += bytesToCopy;

    // Set the number of bytes read
    *lpdwNumberOfBytesRead = bytesToCopy;

    Log("Number of bytes read: " + std::to_string(*lpdwNumberOfBytesRead));

    // Log lpBuffer
    std::string bufferContent(static_cast<char*>(lpBuffer), *lpdwNumberOfBytesRead);
    Log("lpBuffer: " + bufferContent);

    // Return TRUE to indicate success
    return TRUE;
}

__declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    hLogFile = CreateFile(L"MGO3OPy.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    /*DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)oWinHttpConnect, MyWinHttpConnect);
	DetourAttach(&(PVOID&)oWinHttpOpenRequest, MyWinHttpOpenRequest);
	DetourAttach(&(PVOID&)oWinHttpSendRequest, MyWinHttpSendRequest);
	DetourAttach(&(PVOID&)oWinHttpSetOption, MyWinHttpSetOption);
	DetourAttach(&(PVOID&)oWinHttpReceiveResponse, MyWinHttpReceiveResponse);
	DetourAttach(&(PVOID&)oWinHttpQueryHeaders, MyWinHttpQueryHeaders);
	DetourAttach(&(PVOID&)oWinHttpReadData, MyWinHttpReadData);
    DetourTransactionCommit();*/

    InstallHook("winhttp.dll", "WinHttpConnect", MyWinHttpConnect);
    InstallHook("winhttp.dll", "WinHttpOpenRequest", MyWinHttpOpenRequest);
    InstallHook("winhttp.dll", "WinHttpSendRequest", MyWinHttpSendRequest);
    InstallHook("winhttp.dll", "WinHttpWriteData", MyWinHttpWriteData);
    InstallHook("winhttp.dll", "WinHttpSetOption", MyWinHttpSetOption);
    InstallHook("winhttp.dll", "WinHttpReceiveResponse", MyWinHttpReceiveResponse);
    InstallHook("winhttp.dll", "WinHttpQueryHeaders", MyWinHttpQueryHeaders);
    InstallHook("winhttp.dll", "WinHttpReadData", MyWinHttpReadData);

    // initialize Python
    //Py_Initialize();
    //PyEval_InitThreads(); // nb: creates and locks the GIL
    //// NOTE: We save the current thread state, and restore it when we unload,
    //// so that we can clean up properly.
    //PyThreadState* pMainThreadState = PyEval_SaveThread(); // nb: this also releases the GIL

    /*runPython();

    runPython();*/
}
