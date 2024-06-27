// MGO3Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Python.h>
#include "Decoder.cpp"
#include <string>
#include <cryptopp/base64.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>

int main()
{
    //std::string request = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=";
    //Decoder* decoder = new Decoder();
    //Blowfish b = Blowfish();
    //std::string str = "bq\\xdd.?\\xf5o\\x84Ae|\\xa7\\x97LF\\xd4\\x1d\\x1d\\xa2a\\x1a\\xca\\x9f\\xf5\\x94\\xb9=\\xf6E\\xf8\\xc2:;5\\xb9q\"Ho\\xd4\\x1c\\x9eV\\xf7Mzx\\xc4%6\\xaf\\x00\\xb9l}\\x97\\xd2\\x80\\x1aK*\\xe0\\x8c\\xbcy\\x1a0wM9o`\\xfb\\x8c\\xad\\x1a\\xaf\\xdc\\x15\\xc5\\xce\\xa5n\\x96\\xde\\xb8\\xe8\\x0f0U\\x00U\\xcbV?\\x87D\\x04\\xf1\\xfe\\x16\\xfa\\xf0E\\xf7\\xfbM\\x1f\\x0e\\xa0\\xdfsO\\xf0uq\\x1aW\\xe1\\xbb\\x87\\x96\\\\xe0\\xcd\\x90\\xa0^c\\xb8\\xb7*\\xb2B\\xf5\\x88\\xb0\\xd8BR\\xd5\\xb11\\xf9T(\\x02\\xeeb\\xb6\\xd3\\x8c\\xa0\\x06\\xb2\\xba\\x06\\x8aH\\xe9\\x16M\\xea\\x95\\xfe\\xe9\\x1f\\xa5}?\\xf4\\xd4a1A[";


    //Json::Value decoded_request = decoder->decode(request);

    Py_Initialize();
    //// import base64 module
    //PyObject* pModule = PyImport_ImportModule("base64");
    //// get the base64.b64decode method
    //PyObject* pFunc = PyObject_GetAttrString(pModule, "b64decode");
    //// create a Python string object
    //PyObject* pArgs = PyTuple_Pack(1, PyUnicode_FromString("YnHdLj/1b4RBZXynl0xG1B0domEayp/1LMojq6tQ2JDRCuq4vFOnjrcmEnYyhX2g53Vz7aowKt+EVzqGW9ksvQeMWOeYWxuZaj9Kypfl33nW4xS+gkOu8ZIhsn3DDlyCvj4/ygqH0zbpRxitLa6vKmYTlYB8niSWz1DW+Mh+MZ95YAkibYdI6vLUYmIvQZgg337ElM9JYg4GdTtFCRAc8kAUdbf8FMrEi4Q4qrartezAnh+DddW6Eg=="));
    //// call the function with the string object
    //PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
    //// print the result
    //PyObject* pResultStr = PyObject_Repr(pValue);
    //std::cout << "Result: " << PyUnicode_AsUTF8(pResultStr) << std::endl;

    //PyConfig config;
    //PyConfig_InitPythonConfig(&config);

    //// Set the Python path
    //wchar_t* path = const_cast<wchar_t*>(L"C:\\Users\\lynch\\source\\repos\\hooking\\MGO3Test");
    //wchar_t paths[] = { *path, NULL };
    //config.pythonpath_env = paths;

    //PyStatus r = Py_InitializeFromConfig(&config);

    /*PyObject* sysModule = PyImport_ImportModule("sys");
    PyObject* pypath = PyObject_GetAttrString(sysModule, "path");
    PyObject* pathStr = PyObject_Repr(pypath);
    std::cout << "Python path: " << PyUnicode_AsUTF8(pathStr) << std::endl;*/
    
    /*PyObject* myModule = PyImport_ImportModule("Decoder");
    if (myModule == NULL)
        PyErr_Print();

    PyObject* myClass = PyObject_GetAttrString(myModule, "Decoder");
    if (myClass == NULL)
        PyErr_Print();

    PyObject* myInstance = PyObject_CallObject(myClass, NULL);
    if (myInstance == NULL)
        PyErr_Print();

    PyObject* myResult = PyObject_CallMethod(myInstance, "decode", "(s)", 
        "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"
    );
    if (myResult == NULL)
        PyErr_Print();

    PyObject* myResultStr = PyObject_Repr(myResult);
    std::cout << "Result: " << PyUnicode_AsUTF8(myResultStr) << std::endl;

    Py_Finalize();*/

    std::string base64Message = "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";
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

    return 0;

}
