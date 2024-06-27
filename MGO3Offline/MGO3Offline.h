#include "easyhook.h"

#ifdef JAILBREAKHOOK_EXPORTS
#define MGO3OFFLINE __declspec(dllexport)
#else
#define MGO3OFFLINE __declspec(dllimport)
#endif

extern "C" MGO3OFFLINE void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo);
