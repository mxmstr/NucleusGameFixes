// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define CINTERFACE
// Windows Header Files:
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <dinput.h>
#include <string>
#include <sstream>
#include "MinHook.h"
#include "Strsafe.h"
#include "easyhook.h"
#include "DInput8FPSFix.h"

#pragma comment (lib, "dxguid.lib")
