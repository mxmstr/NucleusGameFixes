#include "stdafx.h"
using namespace std;

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

NTSTATUS InstallHook(FARPROC procAddress, void* callBack)
{
    HOOK_TRACE_INFO hHook = { NULL };

    NTSTATUS result = LhInstallHook(
        procAddress,
        callBack,
        NULL,
        &hHook);
    if (FAILED(result))
    {
        std::wstringstream logMessage;
        logMessage << "SHOOK: Error installing " << procAddress << " hook, error msg: " << RtlGetLastErrorString();
        LogW(logMessage.str());
    }
    else
    {
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hHook);
    }

    return result;
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

typedef HRESULT(WINAPI* tDirectInput8Create)(
    HINSTANCE				 hinst,
    DWORD				     dwVersion,
    REFIID					 riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN				 punkOuter);

tDirectInput8Create oDirectInput8Create;

typedef HRESULT(WINAPI* tEnumDevicesA)(
    LPDIRECTINPUT8A			 This,
    DWORD					 dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID				     pvRef,
    DWORD					 dwFlags);

tEnumDevicesA oEnumDevicesA;
tEnumDevicesA pEnumDevicesA;

typedef HRESULT(WINAPI* tEnumDevicesW)(
    LPDIRECTINPUT8W			 This,
    DWORD					 dwDevType,
    LPDIENUMDEVICESCALLBACKW lpCallback,
    LPVOID				     pvRef,
    DWORD					 dwFlags);


tEnumDevicesW oEnumDevicesW;
tEnumDevicesW pEnumDevicesW;

HRESULT WINAPI dEnumDevicesA(
	LPDIRECTINPUT8A	This,
	DWORD dwDevType,
	LPDIENUMDEVICESCALLBACKA lpCallback,
	LPVOID pvRef,
	DWORD dwFlags
)
{
    Log("dEnumDevicesA called");

	if (dwDevType == 1)
	{
		return NULL;
	}
	else
	{
		return pEnumDevicesA(This, dwDevType, lpCallback, pvRef, dwFlags);
	}

}

HRESULT WINAPI dEnumDevicesW(
	LPDIRECTINPUT8W	This,
	DWORD dwDevType,
	LPDIENUMDEVICESCALLBACKW lpCallback,
	LPVOID pvRef,
	DWORD dwFlags
)
{
    Log("dEnumDevicesW called");

	if (dwDevType == 1)
	{
		return NULL;
	}
	else
	{
		return pEnumDevicesW(This, dwDevType, lpCallback, pvRef, dwFlags);
	}

}

HRESULT WINAPI MyDirectInput8Create(
	HINSTANCE hinst,
	DWORD dwVersion,
	REFIID	riidltf,
	LPVOID* ppvOut,
	LPUNKNOWN punkOuter
) {
    Log("DirectInput8Create called");

	HRESULT ret = DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	LPDIRECTINPUT8A pDICa = static_cast<LPDIRECTINPUT8A>(*ppvOut);
	LPDIRECTINPUT8W pDICu = static_cast<LPDIRECTINPUT8W>(*ppvOut);

	if (IsEqualIID(riidltf, IID_IDirectInput8A))
	{
		if (pDICa->lpVtbl->EnumDevices)
		{
			oEnumDevicesA = pDICa->lpVtbl->EnumDevices;

            Log("Installing EnumDevicesA hook");
			//InstallHook((FARPROC)oEnumDevicesA, dEnumDevicesA);
			MH_CreateHook((DWORD_PTR*)oEnumDevicesA, dEnumDevicesA, reinterpret_cast<void**>(&pEnumDevicesA));
			MH_EnableHook((DWORD_PTR*)oEnumDevicesA);
		}
	}

	if (IsEqualIID(riidltf, IID_IDirectInput8W))
	{
		if (pDICu->lpVtbl->EnumDevices)
		{
			oEnumDevicesW = pDICu->lpVtbl->EnumDevices;

            Log("Installing EnumDevicesW hook");
            //InstallHook((FARPROC)oEnumDevicesW, dEnumDevicesW);;
			MH_CreateHook((DWORD_PTR*)oEnumDevicesW, dEnumDevicesW, reinterpret_cast<void**>(&pEnumDevicesW));
			MH_EnableHook((DWORD_PTR*)oEnumDevicesW);
		}
	}

	return ret;

}

__declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo)
{
    ZeroMemory(tempString, sizeof(tempString));
    ZeroMemory(tempStringW, sizeof(tempStringW));

    // See if there is a log file which indicates we should log our progress.
    hLogFile = CreateFile("dinput8fpsfix.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), "(%d): DInput8FPSFix.dll\r\n", GetCurrentProcessId());
        SetFilePointer(hLogFile, 0, 0, FILE_END);
        if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString)*sizeof(TCHAR)), &dwWritten, NULL))
        {
            // If we can't write our first line then don't bother trying to log later.
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
    }

	//InstallHook("dinput8.dll", "DirectInput8Create", MyDirectInput8Create);

    if (MH_Initialize() != MH_OK)
    {
        MessageBox(NULL, ("Error"), ("Failed to Initialize MinHook"), MB_OK);
        //return 1;
    }

    HMODULE din8DLL = nullptr;

    char path[MAX_PATH];
    GetSystemDirectory(path, MAX_PATH);
    strcat_s(path, "\\dinput8.dll");
    din8DLL = LoadLibrary(path);

    oDirectInput8Create = (tDirectInput8Create)GetProcAddress(din8DLL, "DirectInput8Create");

    MH_CreateHook((DWORD_PTR*)oDirectInput8Create, MyDirectInput8Create, reinterpret_cast<void**>(&oDirectInput8Create));

}
