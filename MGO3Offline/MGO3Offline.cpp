// jailbreakhook.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include <sstream>
#include <iostream>
#include <windows.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <winhttp.h>
#include <algorithm>
#include <thread>
#include <fstream>
#include <codecvt>
using namespace std;
using namespace CryptoPP;

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

LPCWSTR swzServerName;
LPCWSTR wszVerb;
LPCWSTR wszObjectName;
LPVOID lpBuffer = NULL;
std::string encoded_response;

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
    //return WinHttpSetOption(hInternet, dwOption, lpBuffer, dwBufferLength);
}

BOOL WINAPI MyWinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpSendRequest called with headers: " << lpszHeaders;
    LogW(logMessage.str());

    // Log the request body
    /*if (lpOptional != NULL)
    {
        std::wstringstream logMessage;
        logMessage << "SHOOK: WinHttpSendRequest called with request body: " << (WCHAR*)lpOptional;
        LogW(logMessage.str());
    }*/

    if (wcscmp(wszObjectName, L"/tppstmweb/eula/eula.var") == 0)
    {
        encoded_response = "La Li Lu Le Lo";
        return TRUE;
    }
    else if (wcscmp(wszObjectName, L"/tppstmweb/coin/coin.var") == 0)
    {
        encoded_response = "La Li Lu Le Lo";
        return TRUE;
    }
    else if (wcscmp(wszObjectName, L"/tppstmweb/privacy/privacy.var") == 0)
    {
        encoded_response = "La Li Lu Le Lo";
        return TRUE;
    }

    // Decode the request body
    if (lpOptional != NULL)
    {
        std::string base64Message = (CHAR*)lpOptional;
        base64Message = base64Message.substr(8);
        base64Message.erase(std::remove(base64Message.begin(), base64Message.end(), '\n'), base64Message.end());
        base64Message.erase(std::remove(base64Message.begin(), base64Message.end(), '\r'), base64Message.end());
        // Replace all %2B with +
        std::string::size_type n = 0;
        while ((n = base64Message.find("%2B", n)) != std::string::npos) {
            base64Message.replace(n, 3, "+");
            n += 1;
        }

        // Decode Base64
        std::string decoded;
        StringSource ss(base64Message, true,
            new Base64Decoder(
                new StringSink(decoded)
            ) // Base64Decoder
        ); // StringSource

        // Load key from binary file
        std::ifstream keyFile = std::ifstream("C:\\Users\\lynch\\Documents\\mgsv_emulator-2\\files\\static_key.bin", std::ios::binary);
        if (!keyFile) {
            LogW(L"Failed to open key file");
            //throw std::runtime_error("Could not open key file");
        }

        std::string key((std::istreambuf_iterator<char>(keyFile)),
            std::istreambuf_iterator<char>());

        // Decrypt Blowfish
        std::string decrypted;
        Blowfish::Decryption decryption((byte*)key.data(), key.size());
        ECB_Mode_ExternalCipher::Decryption ecbDecryption(decryption);

        try {
            StringSource ss2(decoded, true,
                new StreamTransformationFilter(ecbDecryption,
                    new StringSink(decrypted)
                ) // StreamTransformationFilter
            ); // StringSource
        }
        catch (CryptoPP::Exception& e) {
            //Log("Failed to decrypt request body");
        }

        n = 0;
        while ((n = decrypted.find("\\\\r\\\\n", n)) != std::string::npos) {
            decrypted.erase(n, 6);
        }

        n = 0;
        while ((n = decrypted.find("\\r\\n", n)) != std::string::npos) {
            decrypted.erase(n, 4);
        }

        n = 0;
        while ((n = decrypted.find("\\", n)) != std::string::npos) {
            decrypted.erase(n, 1);
        }

        n = 0;
        while ((n = decrypted.find("\"{", n)) != std::string::npos) {
            decrypted.replace(n, 2, "{");
            n += 1;
        }

        n = 0;
        while ((n = decrypted.find("}\"", n)) != std::string::npos) {
            decrypted.replace(n, 2, "}");
            n += 1;
        }

        decrypted = decrypted.substr(0, decrypted.rfind('}') + 1);


        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        LogW(L"Body: " + converter.from_bytes(decrypted));

        // Parse JSON
        Json::Value decrypted_json;
        try {
            Json::CharReaderBuilder builder;
            std::istringstream iss(decrypted);
            std::string errs = "";
            if (!Json::parseFromStream(builder, iss, &decrypted_json, nullptr)) {
                //throw std::runtime_error("Failed to parse JSON: " + errs);
            }
        }
        catch (std::runtime_error& e) {
            LogW(L"Failed to parse JSON: " + converter.from_bytes(std::string(e.what())));
        }


        std::string msgid = decrypted_json["data"]["msgid"].asCString();
        // find the .json file in the subdirectory plugins\\MGO3OfflineCommands whose name is the same as the msgid
        std::string jsonFile = "plugins\\MGO3OfflineCommands\\" + msgid + ".json";
        std::ifstream file(jsonFile);
        if (file.is_open())
        {
            std::string line;
            std::string response;
            while (std::getline(file, line))
            {
                response += line;
            }
            file.close();

            // Replace all ": " with ":" and ", " with ","
            /*n = 0;
            while ((n = response.find(": ", n)) != std::string::npos) {
                response.replace(n, 2, ":");
                n += 1;
            }
            n = 0;
            while ((n = response.find(", ", n)) != std::string::npos) {
                response.replace(n, 2, ",");
                n += 1;
            }*/
            // Remove all whitespace, newlines, and carriage returns
            response.erase(std::remove(response.begin(), response.end(), ' '), response.end());
            response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
            response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());

            Json::Value response_json;
            try {
                Json::CharReaderBuilder builder;
                std::istringstream iss(response);
                std::string errs = "";
                if (!Json::parseFromStream(builder, iss, &response_json, nullptr)) {
                    //throw std::runtime_error("Failed to parse JSON: " + errs);
                }
            }
            catch (std::runtime_error& e) {
                LogW(L"Failed to parse JSON: " + converter.from_bytes(std::string(e.what())));
            }

            //if (response_json["compress"].asBool() && !response_json["session_crypto"].asBool())
            //{
            //    // Use zlib to compress the response

            //}

            //response = response_json.asString();

            //response_json["compress"] = false;
            //response_json["session_crypto"] = false;

            LogW(L"Response: " + converter.from_bytes(response));

            // Enrypt response with Blowfish
            Blowfish::Encryption encryption((byte*)key.data(), key.size());
            ECB_Mode_ExternalCipher::Encryption ecbEncryption(encryption);

            std::string encrypted;
            StringSource ss2(response, true,
                new StreamTransformationFilter(ecbEncryption,
                    new StringSink(encrypted)
                ) // StreamTransformationFilter
            ); // StringSource

            // 

            /*if len(text) % 8 != 0:
            x = bytes([8 - len(text) % 8]) * (8 - len(text) % 8)
                if isinstance(text, str) :
                    text = text.encode() + x
                else :
                    text = text + x*/
            /*if (encrypted.size() % 8 != 0)
            {
                std::string x = std::string(8 - encrypted.size() % 8, 8 - encrypted.size() % 8);
                encrypted += x;
            }*/

            
            // Encode response
            StringSource ss(encrypted, true,
                new Base64Encoder(
                    new StringSink(encoded_response)
                ) // Base64Encoder
            ); // StringSource

            // Remove all whitespace, newlines, and carriage returns
            //encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), ' '), encoded_response.end());
            encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), '\n'), encoded_response.end());
            encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), '\r'), encoded_response.end());

            LogW(L"Response: " + converter.from_bytes(encoded_response));
            LogW(L"");
                        
        }
        else
        {
            LogW(L"Failed to open file: " + converter.from_bytes(jsonFile));
        }
        
        //if (decrypted_json["session_crypto"].asBool())
        //{
        //    // 'MyCoolCryptoKeyAAAAAAA=='
        //    encoder.__init_session_blowfish__(player['crypto_key'])
        //    decoder.__init_session_blowfish__(player['crypto_key'])
        //    decoded_request = decoder.decode(request)
        //}

    }

    return TRUE;
    //return WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
}

BOOL WINAPI MyWinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved)
{
    std::wstringstream logMessage;
    logMessage << "WinHttpReceiveResponse called";
    LogW(logMessage.str());

    return TRUE;
    //return WinHttpReceiveResponse(hRequest, lpReserved);
}

BOOL WINAPI MyWinHttpQueryHeaders(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LogW(L"WinHttpQueryHeaders called");

    // Set headers to Content-Type: text/plain and Content-Length to the length of the response body
    std::wstring headers = L"Content-Type: text/plain\r\n"
        L"Content-Length: " + std::to_wstring(encoded_response.size()) + L"\r\n";

    //if (wcscmp(wszObjectName, L"/tppstmweb/eula/eula.var") == 0)
    //{
    //    std::wstring headers = L"Content-Type: text/html\r\n"
    //        L"Content-Length: 5\r\n";
    //}
    //else if (wcscmp(wszObjectName, L"/tppstmweb/coin/coin.var") == 0)
    //{
    //    std::wstring headers = L"Content-Type: text/html\r\n"
    //        L"Content-Length: 5\r\n";
    //}
    //else if (wcscmp(wszObjectName, L"/tppstmweb/privacy/privacy.var") == 0)
    //{
    //    std::wstring headers = L"Content-Type: text/html\r\n"
    //        L"Content-Length: 5\r\n";
    //}
    //else
    //{
    //    // 
    //}
    

    // Check if the buffer is large enough to hold the headers
    //if (*lpdwBufferLength < headers.size())
    //{
    //    // If the buffer is not large enough, set the required size and return FALSE
    //    *lpdwBufferLength = headers.size();
    //    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    //    LogW(L"Insufficient buffer");
    //    return FALSE;
    //}

    // Copy the headers to the buffer
    memcpy(lpBuffer, headers.c_str(), headers.size() * sizeof(wchar_t));

    // Set the number of bytes written to the buffer
    *lpdwBufferLength = headers.size();

    return TRUE;
}

BOOL WINAPI MyWinHttpReadData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
{
    LogW(L"WinHttpReadData called");

    //BOOL result = WinHttpReadData(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);

    //// Log the response body
    //if (lpBuffer != NULL)
    //{
    //    std::wstringstream logMessage;
    //    logMessage << "SHOOK: WinHttpReadData called with response body: " << std::wstring(static_cast<WCHAR*>(lpBuffer), *lpdwNumberOfBytesRead);
    //    LogW(logMessage.str());
    //}

    //return result;

    // Check if the buffer is large enough to hold the response body
    if (dwNumberOfBytesToRead < encoded_response.size())
    {
        // If the buffer is not large enough, set the required size and return FALSE
        *lpdwNumberOfBytesRead = encoded_response.size();
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Copy the response body to the buffer
    memcpy(lpBuffer, encoded_response.c_str(), encoded_response.size());

    // Set the number of bytes written to the buffer
    *lpdwNumberOfBytesRead = encoded_response.size();

    return TRUE;
}


MGO3OFFLINE void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile(L"mgo3offline.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), L"(%d): MGO3Offline.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

    InstallHook("winhttp.dll", "WinHttpConnect", MyWinHttpConnect);
    InstallHook("winhttp.dll", "WinHttpOpenRequest", MyWinHttpOpenRequest);
    InstallHook("winhttp.dll", "WinHttpSendRequest", MyWinHttpSendRequest);
    InstallHook("winhttp.dll", "WinHttpSetOption", MyWinHttpSetOption);
    InstallHook("winhttp.dll", "WinHttpReceiveResponse", MyWinHttpReceiveResponse);
    InstallHook("winhttp.dll", "WinHttpQueryHeaders", MyWinHttpQueryHeaders);
    InstallHook("winhttp.dll", "WinHttpReadData", MyWinHttpReadData);

    std::string base64Message = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=";

    //// Decode Base64
    //std::string decoded;
    //StringSource ss(base64Message, true,
    //    new Base64Decoder(
    //        new StringSink(decoded)
    //    ) // Base64Decoder
    //); // StringSource

    //// Load key from binary file
    //std::ifstream keyFile = std::ifstream("C:\\Users\\lynch\\Documents\\mgsv_emulator-2\\files\\static_key.bin", std::ios::binary);
    //if (!keyFile) {
    //    //throw std::runtime_error("Could not open key file");
    //}

    //std::string key((std::istreambuf_iterator<char>(keyFile)),
    //    std::istreambuf_iterator<char>());

    //// Decrypt Blowfish
    //std::string decrypted;
    //Blowfish::Decryption decryption((byte*)key.data(), key.size());
    //ECB_Mode_ExternalCipher::Decryption ecbDecryption(decryption);

    //StringSource ss2(decoded, true,
    //    new StreamTransformationFilter(ecbDecryption,
    //        new StringSink(decrypted)
    //    ) // StreamTransformationFilter
    //); // StringSource

    //Log("Decrypted: " + decrypted);

}
