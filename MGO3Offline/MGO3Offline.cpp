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

std::string key;
LPCWSTR swzServerName;
LPCWSTR wszVerb;
LPCWSTR wszObjectName;
LPVOID lpBuffer = NULL;
std::string encoded_response_buffer;
bool responding = false;

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

std::wstring ToWideString(const std::string& input)
{
    std::wstring ws(input.size(), L' '); // Overestimate number of code points.
    ws.resize(std::mbstowcs(&ws[0], input.c_str(), input.size())); // Shrink to fit.
	return ws;
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

std::string Encrypt(const std::string& plainText, const std::string& key) {
    std::string cipherText;

    try {
        Blowfish::Encryption encryption((byte*)key.data(), key.size());
        ECB_Mode_ExternalCipher::Encryption ecbEncryption(encryption);

        StringSource(plainText, true,
            new StreamTransformationFilter(ecbEncryption,
                new StringSink(cipherText)
            ) // StreamTransformationFilter
        ); // StringSource
    }
    catch (const CryptoPP::Exception& e) {
        std::cerr << e.what() << std::endl;
        // Handle the error appropriately
    }

    return cipherText;
}

std::string Decrypt(const std::string& cipherText, const std::string& key) {
    std::string decryptedText;

    try {
        Blowfish::Decryption decryption((byte*)key.data(), key.size());
        ECB_Mode_ExternalCipher::Decryption ecbDecryption(decryption);

        StringSource(cipherText, true,
            new StreamTransformationFilter(ecbDecryption,
                new StringSink(decryptedText)
            ) // StreamTransformationFilter
        ); // StringSource
    }
    catch (const CryptoPP::Exception& e) {
        LogW(ToWideString(e.what()));
    }

    return decryptedText;
}

std::string EncodeBase64(const std::string& input) {
    std::string encoded;
    StringSource(input, true,
        new Base64Encoder(
            new StringSink(encoded)
        )
    );
    return encoded;
}

std::string DecodeBase64(const std::string& input) {
    std::string decoded;
    StringSource(input, true,
        new Base64Decoder(
            new StringSink(decoded)
        )
    );
    return decoded;
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
        encoded_response_buffer = "La Li Lu Le Lo";
        return TRUE;
    }
    else if (wcscmp(wszObjectName, L"/tppstmweb/coin/coin.var") == 0)
    {
        encoded_response_buffer = "La Li Lu Le Lo";
        return TRUE;
    }
    else if (wcscmp(wszObjectName, L"/tppstmweb/privacy/privacy.var") == 0)
    {
        encoded_response_buffer = "La Li Lu Le Lo";
        return TRUE;
    }

    // Decode the request body
    if (lpOptional != NULL)
    {
        std::string base64Message = (CHAR*)lpOptional;

        base64Message = base64Message.substr(8);
        base64Message.erase(std::remove(base64Message.begin(), base64Message.end(), '\n'), base64Message.end());
        base64Message.erase(std::remove(base64Message.begin(), base64Message.end(), '\r'), base64Message.end());

        std::string::size_type n = 0;
        while ((n = base64Message.find("%2B", n)) != std::string::npos) {
            base64Message.replace(n, 3, "+");
            n += 1;
        }


		LogW(L"base64Message: " + ToWideString(base64Message));


        std::string decoded = DecodeBase64(base64Message);
        LogW(L"decoded text: " + ToWideString(decoded));
        std::string decrypted = Decrypt(decoded, key);
        LogW(L"decrypted text: " + ToWideString(decrypted));

        //// Decode Base64
        //std::string decoded;
        //StringSource ss(base64Message, true,
        //    new Base64Decoder(
        //        new StringSink(decoded)
        //    ) // Base64Decoder
        //); // StringSource

        //// Load key from binary file
        //std::ifstream keyFile = std::ifstream("plugins\\static_key.bin", std::ios::binary);
        //if (!keyFile) {
        //    LogW(L"Failed to open key file");
        //    //throw std::runtime_error("Could not open key file");
        //}

        //std::string key((std::istreambuf_iterator<char>(keyFile)),
        //    std::istreambuf_iterator<char>());

        //// Decrypt Blowfish
        //std::string decrypted;
        //Blowfish::Decryption decryption((byte*)key.data(), key.size());
        //ECB_Mode_ExternalCipher::Decryption ecbDecryption(decryption);

        //try {
        //    StringSource ss2(decoded, true,
        //        new StreamTransformationFilter(ecbDecryption,
        //            new StringSink(decrypted)
        //        ) // StreamTransformationFilter
        //    ); // StringSource
        //}
        //catch (CryptoPP::Exception& e) {
        //    //Log("Failed to decrypt request body");
        //}

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


        //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        LogW(L"Body: " + ToWideString(decrypted));

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
            LogW(L"Failed to parse JSON: " + ToWideString(std::string(e.what())));
        }


        std::string msgid = decrypted_json["data"]["msgid"].asCString();
        // find the .json file in the subdirectory plugins\\MGO3OfflineCommands whose name is the same as the msgid
        std::string jsonFile = "plugins\\MGO3OfflineCommands\\" + msgid + ".json";
        std::ifstream file(jsonFile);

        if (file.is_open())
        {
            std::string line;
            std::string response;
            std::string response_encoded;
            while (std::getline(file, line))
            {
                response += line;
            }
            file.close();

            response.erase(std::remove(response.begin(), response.end(), ' '), response.end());
			LogW(L"Response: " + ToWideString(response));
            
            std::string encryptedResponse = Encrypt(response, key);
            LogW(L"encoded text: " + ToWideString(encryptedResponse));
            encoded_response_buffer = EncodeBase64(encryptedResponse);
            encoded_response_buffer = "httpMsg=" + encoded_response_buffer + "=";
            LogW(L"cipher text: " + ToWideString(encoded_response_buffer));
			// Log the last character of the encoded response
			LogW(L"Last character of encoded response: " + ToWideString(encoded_response_buffer.substr(encoded_response_buffer.size() - 1)));


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
            //// Remove all whitespace, newlines, and carriage returns
            //response.erase(std::remove(response.begin(), response.end(), ' '), response.end());
            //response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
            //response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());

            //Json::Value response_json;
            //try {
            //    Json::CharReaderBuilder builder;
            //    std::istringstream iss(response);
            //    std::string errs = "";
            //    if (!Json::parseFromStream(builder, iss, &response_json, nullptr)) {
            //        //throw std::runtime_error("Failed to parse JSON: " + errs);
            //    }
            //}
            //catch (std::runtime_error& e) {
            //    LogW(L"Failed to parse JSON: " + converter.from_bytes(std::string(e.what())));
            //}

            //if (response_json["compress"].asBool() && !response_json["session_crypto"].asBool())
            //{
            //    // Use zlib to compress the response

            //}

            //response = response_json.asString();

            //response_json["compress"] = false;
            //response_json["session_crypto"] = false;



            //// Enrypt response with Blowfish
            //Blowfish::Encryption encryption((byte*)key.data(), key.size());
            //ECB_Mode_ExternalCipher::Encryption ecbEncryption(encryption);

            //std::string encrypted;
            //StringSource ss2(response, true,
            //    new StreamTransformationFilter(ecbEncryption,
            //        new StringSink(encrypted)
            //    ) // StreamTransformationFilter
            //); // StringSource

            //// 

            ///*if len(text) % 8 != 0:
            //x = bytes([8 - len(text) % 8]) * (8 - len(text) % 8)
            //    if isinstance(text, str) :
            //        text = text.encode() + x
            //    else :
            //        text = text + x*/
            ///*if (encrypted.size() % 8 != 0)
            //{
            //    std::string x = std::string(8 - encrypted.size() % 8, 8 - encrypted.size() % 8);
            //    encrypted += x;
            //}*/

            //// Encode response
            //StringSource ss3(encrypted, true,
            //    new Base64Encoder(
            //        new StringSink(encoded_response)
            //    ) // Base64Encoder
            //); // StringSource

            //// Remove all whitespace, newlines, and carriage returns
            ////encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), ' '), encoded_response.end());
            ////encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), '\n'), encoded_response.end());
            ////encoded_response.erase(std::remove(encoded_response.begin(), encoded_response.end(), '\r'), encoded_response.end());

            //LogW(L"Response encoded 2: " + converter.from_bytes(encoded_response));
            //LogW(L"");

            //encoded_response = "httpMsg=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";
                        
        }
        else
        {
            LogW(L"Failed to open file: " + ToWideString(jsonFile));
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

    unsigned int hexArray[] = { 0xC8, 0x0, 0x0, 0x0 };
	// Convert hexArray to byte array
	byte* byteArray = new byte[4];
	for (int i = 0; i < 4; i++)
	{
		byteArray[i] = hexArray[i];
	}
	// assign byteArray to lpBuffer
	memcpy(lpBuffer, byteArray, 4);

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

    //// Log the response body
    //if (lpBuffer != NULL)
    //{
    //    std::wstringstream logMessage;
    //    logMessage << "SHOOK: WinHttpReadData called with response body: " << std::wstring(static_cast<WCHAR*>(lpBuffer), *lpdwNumberOfBytesRead);
    //    LogW(logMessage.str());
    //}

    //return result;

	if (encoded_response_buffer.size() == 0)
	{
		LogW(L"Encoded response is empty");
        *lpdwNumberOfBytesRead = 0;
		return TRUE;
	}

    static size_t position = 0; // Static variable to maintain position across calls

	LogW(L"Number of bytes to read: " + std::to_wstring(dwNumberOfBytesToRead));

    // Calculate the number of bytes to copy
    size_t bytesRemaining = encoded_response_buffer.size() - position;
    DWORD bytesToCopy = min(dwNumberOfBytesToRead, static_cast<DWORD>(bytesRemaining));

    // Copy the data to lpBuffer
    memcpy(lpBuffer, encoded_response_buffer.data() + position, bytesToCopy);

    // Update the position index
    position += bytesToCopy;

    // Set the number of bytes read
    *lpdwNumberOfBytesRead = bytesToCopy;

	LogW(L"Number of bytes read: " + std::to_wstring(*lpdwNumberOfBytesRead));

    // Reset position if the end of encoded_response is reached
    if (position >= encoded_response_buffer.size())
    {
		LogW(L"End of response reached");
        position = 0;
		encoded_response_buffer.clear();
		responding = false;
    }

    // Log lpBuffer 
    std::wstringstream logMessage;
    logMessage << "lpBuffer: " << std::wstring(static_cast<WCHAR*>(lpBuffer), *lpdwNumberOfBytesRead);
    LogW(logMessage.str());

    // Return TRUE to indicate success
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

    /*InstallHook("winhttp.dll", "WinHttpConnect", MyWinHttpConnect);
    InstallHook("winhttp.dll", "WinHttpOpenRequest", MyWinHttpOpenRequest);
    InstallHook("winhttp.dll", "WinHttpSendRequest", MyWinHttpSendRequest);
    InstallHook("winhttp.dll", "WinHttpSetOption", MyWinHttpSetOption);
    InstallHook("winhttp.dll", "WinHttpReceiveResponse", MyWinHttpReceiveResponse);
    InstallHook("winhttp.dll", "WinHttpQueryHeaders", MyWinHttpQueryHeaders);
    InstallHook("winhttp.dll", "WinHttpReadData", MyWinHttpReadData);*/

    /*std::ifstream keyFile = std::ifstream("plugins\\static_key.bin", std::ios::binary);
    if (!keyFile) {
        LogW(L"Failed to open key file");
        throw std::runtime_error("Could not open key file");
    }

    key = string((std::istreambuf_iterator<char>(keyFile)),
        std::istreambuf_iterator<char>());*/




 //   std::string jsonString = R"({"compress":true,"data":{"crypto_type":"COMMON","flowid":null,"msgid":"CMD_GET_URLLIST","result":"NOERR","rqid":0,"url_list":[{"type":"GATE","url":"https://mgstpp-game.konamionline.com/mgostm/gate","version":0},{"type":"WEB","url":"https://mgstpp-game.konamionline.com/mgostm/main","version":0},{"type":"EULA","url":"http://mgstpp-game.konamionline.com/tppstmweb/eula/eula.var","version":0},{"type":"EULA_COIN","url":"http://mgstpp-game.konamionline.com/tppstmweb/coin/coin.var","version":0},{"type":"POLICY_GDPR","url":"http://mgstpp-game.konamionline.com/tppstmweb/gdpr/privacy.var","version":0},{"type":"POLICY_JP","url":"http://mgstpp-game.konamionline.com/tppstmweb/privacy_jp/privacy.var","version":0},{"type":"POLICY_ELSE","url":"http://mgstpp-game.konamionline.com/tppstmweb/privacy/privacy.var","version":0},{"type":"LEGAL","url":"https://legal.konami.com/games/mgsvtpp/","version":0},{"type":"PERMISSION","url":"https://www.konami.com/","version":0},{"type":"POLICY_CCPA","url":"http://mgstpp-game.konamionline.com/tppstmweb/privacy_ccpa/privacy.var","version":0},{"type":"EULA_TEXT","url":"https://legal.konami.com/games/mgsvtpp/terms/","version":0},{"type":"EULA_COIN_TEXT","url":"https://legal.konami.com/games/mgsvtpp/terms/currency/","version":0},{"type":"POLICY_GDPR_TEXT","url":"https://legal.konami.com/games/mgsvtpp/","version":0},{"type":"POLICY_JP_TEXT","url":"https://legal.konami.com/games/privacy/view/","version":0},{"type":"POLICY_ELSE_TEXT","url":"https://legal.konami.com/games/privacy/view/","version":0},{"type":"POLICY_CCPA_TEXT","url":"https://legal.konami.com/games/mgsvtpp/ppa4ca/","version":0}],"url_num":16,"xuid":null},"original_size":1633,"session_crypto":false,"session_key":null})";
 //   
 //   
 //   // Encrypt the response
 //   std::string encryptedResponse = Encrypt(jsonString, key);
 //   
 //   LogW(L"encrypted text: " + ToWideString(encryptedResponse));
 //   
 //   // Encode the encrypted string to Base64
 //   std::string encoded = EncodeBase64(encryptedResponse);

 //   // Remove last character and add ==
	//encoded = encoded.substr(0, encoded.size() - 1);
	////encoded = encoded + "\=\=";
 //   
 //   //encoded = "httpMsg=" + encoded;
 //   LogW(L"encoded text: " + ToWideString(encoded) + ToWideString("equals=\n"));
    
    
    //// Decode the Base64 string back to the encrypted string
    //std::string decoded = DecodeBase64(encoded);
    //
    //LogW(L"decoded text: " + ToWideString(decoded));
    //
    //// Decrypt the response (if needed)
    //std::string decryptedResponse = Decrypt(encryptedResponse, key);
    //LogW(L"decrypted text: " + ToWideString(decryptedResponse));



}

