#include "easyhook.h"
#include <string>
#include <windows.h>

#ifdef JAILBREAKHOOK_EXPORTS
#define LHOOK_API __declspec(dllexport)
#else
#define LHOOK_API __declspec(dllimport)
#endif

extern "C" LHOOK_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo);
