// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

// Exported function that takes a string, loads the python class Decoder from module Decoder and calls the decode method on it
extern "C" __declspec(dllexport) const char* DecodeString(const char* str)
{
    PyObject* pName, * pModule, * pDict, * pClass, * pInstance;
    PyObject* pValue = NULL;
    Py_Initialize();
    //pName = PyUnicode_DecodeFSDefault("Decoder");
    //pModule = PyImport_Import(pName);
    ////Py_DECREF(pName);
    //if (pModule != NULL)
    //{
    //    pClass = PyObject_GetAttrString(pModule, "Decoder");
    //    if (pClass != NULL)
    //    {
    //        pInstance = PyObject_CallObject(pClass, NULL);
    //        if (pInstance != NULL)
    //        {
    //            pValue = PyObject_CallMethod(pInstance, "decode", "s", str);
    //            if (pValue != NULL)
    //            {
    //                //Py_DECREF(pValue);
    //            }
    //            //Py_DECREF(pInstance);
    //        }
    //        //Py_DECREF(pClass);
    //    }
    //    //Py_DECREF(pModule);
    //}
    Py_Finalize();

    //// return pValue as a string
    //return PyUnicode_AsUTF8(pValue);
    return "";
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

