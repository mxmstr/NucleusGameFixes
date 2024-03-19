#include "easyhook.h"

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JAILBREAKHOOK_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JAILBREAKHOOK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JAILBREAKHOOK_EXPORTS
#define BCFIX_API __declspec(dllexport)
#else
#define BCFIX_API __declspec(dllimport)
#endif

extern "C" BCFIX_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo);
