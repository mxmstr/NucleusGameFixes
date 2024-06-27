#ifndef IPHLPAPI2_H
#define IPHLPAPI2_H

#include <windef.h>
#include <iptypes.h>

extern "C" {
    ULONG WINAPI GetAdaptersAddresses(
        _In_    ULONG                 Family,
        _In_    ULONG                 Flags,
        _In_    PVOID                 Reserved,
        _Out_   PIP_ADAPTER_ADDRESSES AdapterAddresses,
        _Inout_ PULONG                SizePointer
    );
}

#endif // IPHLPAPI2_H
