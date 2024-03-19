#include "easyhook.h"

#ifdef JAILBREAKHOOK_EXPORTS
#define SDLHOOK_API __declspec(dllexport)
#else
#define SDLHOOK_API __declspec(dllimport)
#endif

extern "C" SDLHOOK_API void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * remoteInfo);
