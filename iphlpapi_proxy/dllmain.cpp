// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <winsock2.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include <strsafe.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <map>
#include <WS2tcpip.h>
using namespace std;

// A simple proxy DLL for iphlpapi.dll

HANDLE hLogFile = INVALID_HANDLE_VALUE;
TCHAR tempString[2048];
WCHAR tempStringW[2048];
WCHAR* targetIPAddress;
WCHAR* targetHostname;

void WriteToLogFile(void* lpBuffer, size_t cbBytes)
{
    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten = 0;
        WriteFile(hLogFile, lpBuffer, (DWORD)cbBytes, &dwWritten, NULL);
    }
}

//void Log(string message)
//{
//    std::stringstream logMessage;
//    logMessage << GetCurrentProcessId() << ": " << message;
//
//    StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), (logMessage.str() + "\r\n").c_str());
//    WriteToLogFile(tempString, _tcslen(tempString) * sizeof(TCHAR));
//}

void LogW(wstring message)
{
    wstringstream logMessage;
    logMessage << GetCurrentProcessId() << ": " << message;

    StringCchPrintfW(tempStringW, sizeof(tempStringW) / sizeof(WCHAR), (logMessage.str() + L"\r\n").c_str());
    WriteToLogFile(tempStringW, wcslen(tempStringW) * sizeof(WCHAR));
}

extern "C" __declspec(dllexport) ULONG WINAPI MyGetAdaptersAddresses2(
    ULONG Family,
    ULONG Flags,
    PVOID Reserved,
    PIP_ADAPTER_ADDRESSES AdapterAddresses,
    PULONG SizePointer
) {
    LogW(L"GetAdaptersAddresses2 called");

    if (AdapterAddresses == nullptr)
    {
        //LogW(L"AdapterAddresses is NULL");
        *SizePointer = sizeof(IP_ADAPTER_ADDRESSES);

        return ERROR_BUFFER_OVERFLOW;
    }
    else
    {
        //LogW(L"AdapterAddresses is not NULL");
        if (*SizePointer < sizeof(IP_ADAPTER_ADDRESSES))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }

        /*FILE* file = fopen("C:\\Games\\Lost Planet 2\\adapteraddresses.txt", "r");
        if (file == NULL)
        {
            return ERROR_FILE_NOT_FOUND;
        }*/

        AdapterAddresses->Next = nullptr;
        // Adapter Name: {A631780E-56E5-4141-90A6-DD483A377755}
        PCHAR AdapterName = new char[100];
        strcpy(AdapterName, "{A631780E-56E5-4141-90A6-DD483A377755}");
        AdapterAddresses->AdapterName = AdapterName;
        // Adapter Description: Realtek USB GbE Family Controller
        /*PWCHAR Description = new wchar_t[100];
        wcscpy(Description, L"Realtek USB GbE Family Controller");
        AdapterAddresses->Description = Description;*/
        //// Adapter FriendlyName: Ethernet 2
        //PWCHAR FriendlyName = new wchar_t[100];
        //wcscpy(FriendlyName, L"Ethernet 2");
        //AdapterAddresses->FriendlyName = FriendlyName;
        //// Adapter Physical Address Length: 6
        //AdapterAddresses->PhysicalAddressLength = 6;
        //// Adapter Physical Address: 0-e0-4c-e3-2a-77
        //unsigned char macAddress[6] = { 0x00, 0xE0, 0x4C, 0xE3, 0x2A, 0x77 };
        //memcpy(AdapterAddresses->PhysicalAddress, macAddress, 6);
        //// Adapter Alignment: 1300000178
        //AdapterAddresses->Alignment = 1300000178;
        //// Adapter Length: 178
        //AdapterAddresses->Length = 178;
        // Adapter Index: 13
        AdapterAddresses->IfIndex = 13;
        // Adapter FirstUnicastAddress:

        AdapterAddresses->FirstUnicastAddress = (PIP_ADAPTER_UNICAST_ADDRESS)malloc(sizeof(IP_ADAPTER_UNICAST_ADDRESS));
        AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
        AdapterAddresses->FirstUnicastAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
        AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr->sa_family = AF_INET;
        size_t length = wcslen(targetIPAddress) + 1;
        char* narrowString = new char[length];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, narrowString, length, targetIPAddress, _TRUNCATE);
        ((SOCKADDR_IN*)AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr(narrowString);
        ((SOCKADDR_IN*)AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr)->sin_port = 0;
        AdapterAddresses->FirstUnicastAddress->Next = nullptr;

        AdapterAddresses->FirstAnycastAddress = nullptr;

        //// Adapter FirstMulticastAddress:
        //AdapterAddresses->FirstMulticastAddress = (PIP_ADAPTER_MULTICAST_ADDRESS)malloc(sizeof(PIP_ADAPTER_MULTICAST_ADDRESS));
        //AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
        //AdapterAddresses->FirstMulticastAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
        //AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr->sa_family = AF_INET;
        //((SOCKADDR_IN*)AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr("239.255.255.250");
        //((SOCKADDR_IN*)AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr)->sin_port = 0;
        //AdapterAddresses->FirstMulticastAddress->Next = nullptr;
        //// Adapter FirstDnsServerAddress:
        //AdapterAddresses->FirstDnsServerAddress = (PIP_ADAPTER_DNS_SERVER_ADDRESS)malloc(sizeof(PIP_ADAPTER_DNS_SERVER_ADDRESS));
        //AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
        //AdapterAddresses->FirstDnsServerAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
        //AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr->sa_family = AF_INET;
        //((SOCKADDR_IN*)AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr("75.75.75.75");
        //((SOCKADDR_IN*)AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr)->sin_port = 0;
        //AdapterAddresses->FirstDnsServerAddress->Next = nullptr;
        //// Adapter DnsSuffix: 010476DC
        //PWCHAR DnsSuffix = new wchar_t[100];
        //wcscpy(DnsSuffix, L"010476DC");
        //AdapterAddresses->DnsSuffix = DnsSuffix;
        //// Adapter Flags: c5
        //AdapterAddresses->Flags = 0xc5;
        //// Adapter DdnsEnabled: 1
        //AdapterAddresses->DdnsEnabled = 1;
        //// Adapter RegisterAdapterSuffix: 0
        //AdapterAddresses->RegisterAdapterSuffix = 0;
        //// Adapter Dhcpv4Enabled: 1
        //AdapterAddresses->Dhcpv4Enabled = 1;
        //// Adapter ReceiveOnly: 0
        //AdapterAddresses->ReceiveOnly = 0;
        //// Adapter NoMulticast: 0
        //AdapterAddresses->NoMulticast = 0;
        //// Adapter Ipv6OtherStatefulConfig: 0
        //AdapterAddresses->Ipv6OtherStatefulConfig = 0;
        //// Adapter NetbiosOverTcpipEnabled: 1
        //AdapterAddresses->NetbiosOverTcpipEnabled = 1;
        // Adapter Ipv4Enabled: 1
        AdapterAddresses->Ipv4Enabled = 1;
        // Adapter Ipv6Enabled: 0
        AdapterAddresses->Ipv6Enabled = 0;
        // Adapter Ipv6ManagedAddressConfigurationSupported: 0
        //AdapterAddresses->Ipv6ManagedAddressConfigurationSupported = 0;
        // Adapter Mtu: 5dc
        AdapterAddresses->Mtu = 0x5dc;
        // Adapter IfType: 6
        AdapterAddresses->IfType = IF_TYPE_ETHERNET_CSMACD;
        // Adapter OperStatus: 1
        AdapterAddresses->OperStatus = IfOperStatusUp;
        // Adapter Ipv6IfIndex: 0
        //AdapterAddresses->Ipv6IfIndex = 0;
        // Adapter ZoneIndices: 010475A0
        /*memset(AdapterAddresses->ZoneIndices, 0, 16);
        memcpy(AdapterAddresses->ZoneIndices, new BYTE[16], 16);*/
       // // Adapter FirstPrefix: 010478B8
       // AdapterAddresses->FirstPrefix = (IP_ADAPTER_PREFIX*)malloc(sizeof(IP_ADAPTER_PREFIX));
       // // Adapter TransmitLinkSpeed: 3b9aca00
       // AdapterAddresses->TransmitLinkSpeed = 0x3b9aca00;
       // // Adapter ReceiveLinkSpeed: 3b9aca00
       // AdapterAddresses->ReceiveLinkSpeed = 0x3b9aca00;
       // // Adapter FirstWinsServerAddress: 00000000
       // AdapterAddresses->FirstWinsServerAddress = nullptr;
       // // Adapter FirstGatewayAddress: 00000000
       // AdapterAddresses->FirstGatewayAddress = nullptr;
       // // Adapter Ipv4Metric: 1388
       // AdapterAddresses->Ipv4Metric = 1388;
       // // Adapter Ipv6Metric: 0
       // AdapterAddresses->Ipv6Metric = 0;
       // // Adapter Luid.Value: 6008013000000
       // AdapterAddresses->Luid.Value = 0x6008013000000;
       // // Adapter Dhcpv4Server.lpSockaddr: 010476CC
       // AdapterAddresses->Dhcpv4Server = SOCKET_ADDRESS();
       // AdapterAddresses->Dhcpv4Server.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
       // AdapterAddresses->Dhcpv4Server.lpSockaddr->sa_family = AF_INET;
       // ((SOCKADDR_IN*)AdapterAddresses->Dhcpv4Server.lpSockaddr)->sin_addr.s_addr = inet_addr("123.123.123.123");
       // ((SOCKADDR_IN*)AdapterAddresses->Dhcpv4Server.lpSockaddr)->sin_port = 0;
       // // Adapter CompartmentId: 1
       // AdapterAddresses->CompartmentId = 1;
       // // Adapter NetworkGuid.Data1: 2e9f0c89
       // AdapterAddresses->NetworkGuid.Data1 = 0x2e9f0c89;
       // // Adapter ConnectionType: 1
       // AdapterAddresses->ConnectionType = NET_IF_CONNECTION_DEDICATED;
       // // Adapter TunnelType: 0
       // AdapterAddresses->TunnelType = TUNNEL_TYPE_NONE;
       // // Adapter Dhcpv6Server.lpSockaddr: 00000000
       // AdapterAddresses->Dhcpv6Server.lpSockaddr = nullptr;
       // // Adapter Dhcpv6ClientDuid:
       ///* memset(AdapterAddresses->Dhcpv6ClientDuid, 0, MAX_DHCPV6_DUID_LENGTH);
       // memcpy(AdapterAddresses->Dhcpv6ClientDuid, new BYTE[MAX_DHCPV6_DUID_LENGTH], MAX_DHCPV6_DUID_LENGTH);*/
       // // Adapter Dhcpv6ClientDuidLength: 0
       // AdapterAddresses->Dhcpv6ClientDuidLength = 0;
       // // Adapter Dhcpv6Iaid: 0
       // AdapterAddresses->Dhcpv6Iaid = 0;
       // // Adapter FirstDnsSuffix: 00000000
       // AdapterAddresses->FirstDnsSuffix = nullptr;



        //AdapterAddresses->Next = nullptr;
        //AdapterAddresses->FirstAnycastAddress = nullptr;
        //AdapterAddresses->FirstPrefix = nullptr;
        //AdapterAddresses->TransmitLinkSpeed = NULL;
        //AdapterAddresses->ReceiveLinkSpeed = NULL;
        //memset(AdapterAddresses->Dhcpv6ClientDuid, 0, sizeof(AdapterAddresses->Dhcpv6ClientDuid));

        ////memcpy(AdapterAddresses->Dhcpv6ClientDuid, new BYTE[130], 130);
        //AdapterAddresses->Dhcpv6ClientDuidLength = 0;
        //AdapterAddresses->Dhcpv6Iaid = 0;
        //AdapterAddresses->FirstDnsSuffix = nullptr;

        // Read the file line by line
        /*char line[256];
        while (fgets(line, sizeof(line), file))
        {
            if (strstr(line, "Adapter Name: "))
            {
                char* adapterName = strstr(line, "Adapter Name: ") + strlen("Adapter Name: ");
                PCHAR adapterNameBuf = new char[strlen(adapterName)];
                strcpy(adapterNameBuf, adapterName);
                adapterNameBuf[strlen(adapterName) - 1] = '\0';
                AdapterAddresses->AdapterName = adapterNameBuf;
            }
            else if (strstr(line, "Adapter Description: "))
            {
                char* adapterDescription = strstr(line, "Adapter Description: ") + strlen("Adapter Description: ");
                adapterDescription[strlen(adapterDescription) - 1] = '\0';
                size_t size = mbstowcs(NULL, adapterDescription, 0) + 1;
                wchar_t* adapterDescriptionBuf = new wchar_t[size];
                mbstowcs(adapterDescriptionBuf, adapterDescription, size);
                AdapterAddresses->Description = adapterDescriptionBuf;
            }
            else if (strstr(line, "Adapter FriendlyName: "))
            {
                char* adapterFriendlyName = strstr(line, "Adapter FriendlyName: ") + strlen("Adapter FriendlyName: ");
                adapterFriendlyName[strlen(adapterFriendlyName) - 1] = '\0';
                size_t size = mbstowcs(NULL, adapterFriendlyName, 0) + 1;
                wchar_t* adapterFriendlyNameBuf = new wchar_t[size];
                mbstowcs(adapterFriendlyNameBuf, adapterFriendlyName, size);
                AdapterAddresses->FriendlyName = adapterFriendlyNameBuf;
            }
            else if (strstr(line, "Adapter Physical Address: "))
            {
                char* adapterPhysicalAddress = strstr(line, "Adapter Physical Address: ") + strlen("Adapter Physical Address: ");
                adapterPhysicalAddress[strlen(adapterPhysicalAddress) - 1] = '\0';
                sscanf(adapterPhysicalAddress, "%x-%x-%x-%x-%x-%x",
                    &AdapterAddresses->PhysicalAddress[0],
                    &AdapterAddresses->PhysicalAddress[1],
                    &AdapterAddresses->PhysicalAddress[2],
                    &AdapterAddresses->PhysicalAddress[3],
                    &AdapterAddresses->PhysicalAddress[4],
                    &AdapterAddresses->PhysicalAddress[5]);

                int length = 6;
                AdapterAddresses->PhysicalAddressLength = static_cast<ULONG>(length);
            }
            else if (strstr(line, "Adapter Alignment: "))
            {
                char* adapterAlignment = strstr(line, "Adapter Alignment: ") + strlen("Adapter Alignment: ");
                adapterAlignment[strlen(adapterAlignment) - 1] = '\0';
                AdapterAddresses->Alignment = _atoi64(adapterAlignment);
            }
            else if (strstr(line, "Adapter Length: "))
            {
                char* adapterLength = strstr(line, "Adapter Length: ") + strlen("Adapter Length: ");
                adapterLength[strlen(adapterLength) - 1] = '\0';
                AdapterAddresses->Length = atoi(adapterLength);
            }
            else if (strstr(line, "Adapter Index: "))
            {
                char* adapterIndex = strstr(line, "Adapter Index: ") + strlen("Adapter Index: ");
                adapterIndex[strlen(adapterIndex) - 1] = '\0';
                AdapterAddresses->IfIndex = atoi(adapterIndex);
            }
            else if (strstr(line, "Adapter FirstUnicastAddress: "))
            {
                char* adapterFirstUnicastAddress = strstr(line, "Adapter FirstUnicastAddress: ") + strlen("Adapter FirstUnicastAddress: ");
                adapterFirstUnicastAddress[strlen(adapterFirstUnicastAddress) - 1] = '\0';
                AdapterAddresses->FirstUnicastAddress = (PIP_ADAPTER_UNICAST_ADDRESS)malloc(sizeof(IP_ADAPTER_UNICAST_ADDRESS));
                AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
                AdapterAddresses->FirstUnicastAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
                AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr->sa_family = AF_INET;
                ((SOCKADDR_IN*)AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr(adapterFirstUnicastAddress);
                ((SOCKADDR_IN*)AdapterAddresses->FirstUnicastAddress->Address.lpSockaddr)->sin_port = 0;
                AdapterAddresses->FirstUnicastAddress->Next = nullptr;
            }
            else if (strstr(line, "Adapter FirstMulticastAddress: "))
            {
                char* adapterFirstMulticastAddress = strstr(line, "Adapter FirstMulticastAddress: ") + strlen("Adapter FirstMulticastAddress: ");
                adapterFirstMulticastAddress[strlen(adapterFirstMulticastAddress) - 1] = '\0';
                AdapterAddresses->FirstMulticastAddress = (PIP_ADAPTER_MULTICAST_ADDRESS)malloc(sizeof(PIP_ADAPTER_MULTICAST_ADDRESS));
                AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
                AdapterAddresses->FirstMulticastAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
                AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr->sa_family = AF_INET;
                ((SOCKADDR_IN*)AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr(adapterFirstMulticastAddress);
                ((SOCKADDR_IN*)AdapterAddresses->FirstMulticastAddress->Address.lpSockaddr)->sin_port = 0;
                AdapterAddresses->FirstMulticastAddress->Next = nullptr;
            }
            else if (strstr(line, "Adapter FirstDnsServerAddress: "))
            {
                char* adapterFirstDnsServerAddress = strstr(line, "Adapter FirstDnsServerAddress: ") + strlen("Adapter FirstDnsServerAddress: ");
                adapterFirstDnsServerAddress[strlen(adapterFirstDnsServerAddress) - 1] = '\0';
                AdapterAddresses->FirstDnsServerAddress = (PIP_ADAPTER_DNS_SERVER_ADDRESS)malloc(sizeof(PIP_ADAPTER_DNS_SERVER_ADDRESS));
                AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
                AdapterAddresses->FirstDnsServerAddress->Address.iSockaddrLength = sizeof(SOCKADDR);
                AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr->sa_family = AF_INET;
                ((SOCKADDR_IN*)AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr)->sin_addr.s_addr = inet_addr(adapterFirstDnsServerAddress);
                ((SOCKADDR_IN*)AdapterAddresses->FirstDnsServerAddress->Address.lpSockaddr)->sin_port = 0;
                AdapterAddresses->FirstDnsServerAddress->Next = nullptr;
            }
            else if (strstr(line, "Adapter DnsSuffix: "))
            {
                char* adapterDnsSuffix = strstr(line, "Adapter DnsSuffix: ") + strlen("Adapter DnsSuffix: ");
                adapterDnsSuffix[strlen(adapterDnsSuffix) - 1] = '\0';
                size_t size = mbstowcs(NULL, adapterDnsSuffix, 0) + 1;
                wchar_t* adapterDnsSuffixBuf = new wchar_t[size];
                mbstowcs(adapterDnsSuffixBuf, adapterDnsSuffix, size);
                AdapterAddresses->DnsSuffix = adapterDnsSuffixBuf;
            }
            else if (strstr(line, "Adapter Flags: "))
            {
                char* adapterFlags = strstr(line, "Adapter Flags: ") + strlen("Adapter Flags: ");
                adapterFlags[strlen(adapterFlags) - 1] = '\0';
                AdapterAddresses->Flags = atoi(adapterFlags);
            }
            else if (strstr(line, "Adapter DdnsEnabled: "))
            {
                char* adapterDdnsEnabled = strstr(line, "Adapter DdnsEnabled: ") + strlen("Adapter DdnsEnabled: ");
                adapterDdnsEnabled[strlen(adapterDdnsEnabled)] = '\0';
                AdapterAddresses->DdnsEnabled = atoi(adapterDdnsEnabled);
            }

            else if (strstr(line, "Adapter Ipv4Enabled: "))
            {
                char* adapterIpv4Enabled = strstr(line, "Adapter Ipv4Enabled: ") + strlen("Adapter Ipv4Enabled: ");
                adapterIpv4Enabled[strlen(adapterIpv4Enabled) - 1] = '\0';
                AdapterAddresses->Ipv4Enabled = atoi(adapterIpv4Enabled);
            }
            else if (strstr(line, "Adapter Ipv6Enabled: "))
            {
                char* adapterIpv6Enabled = strstr(line, "Adapter Ipv6Enabled: ") + strlen("Adapter Ipv6Enabled: ");
                adapterIpv6Enabled[strlen(adapterIpv6Enabled) - 1] = '\0';
                AdapterAddresses->Ipv6Enabled = atoi(adapterIpv6Enabled);
            }
            else if (strstr(line, "Adapter Mtu: "))
            {
                char* adapterMtu = strstr(line, "Adapter Mtu: ") + strlen("Adapter Mtu: ");
                adapterMtu[strlen(adapterMtu) - 1] = '\0';
                AdapterAddresses->Mtu = atoi(adapterMtu);
            }
            else if (strstr(line, "Adapter IfType: "))
            {
                char* adapterIfType = strstr(line, "Adapter IfType: ") + strlen("Adapter IfType: ");
                adapterIfType[strlen(adapterIfType) - 1] = '\0';
                AdapterAddresses->IfType = atoi(adapterIfType);
            }
            else if (strstr(line, "Adapter OperStatus: "))
            {
                char* adapterOperStatus = strstr(line, "Adapter OperStatus: ") + strlen("Adapter OperStatus: ");
                adapterOperStatus[strlen(adapterOperStatus) - 1] = '\0';
                AdapterAddresses->OperStatus = (IF_OPER_STATUS)atoi(adapterOperStatus);
            }
            else if (strstr(line, "Adapter Ipv6IfIndex: "))
            {
                char* adapterIpv6IfIndex = strstr(line, "Adapter Ipv6IfIndex: ") + strlen("Adapter Ipv6IfIndex: ");
                adapterIpv6IfIndex[strlen(adapterIpv6IfIndex) - 1] = '\0';
                AdapterAddresses->Ipv6IfIndex = atoi(adapterIpv6IfIndex);
            }
            else if (strstr(line, "Adapter FirstWinsServerAddress: "))
            {
                char* adapterFirstWinsServerAddress = strstr(line, "Adapter FirstWinsServerAddress: ") + strlen("Adapter FirstWinsServerAddress: ");
                adapterFirstWinsServerAddress[strlen(adapterFirstWinsServerAddress) - 1] = '\0';
                AdapterAddresses->FirstWinsServerAddress = (PIP_ADAPTER_WINS_SERVER_ADDRESS)malloc(sizeof(PIP_ADAPTER_WINS_SERVER_ADDRESS));
                AdapterAddresses->FirstWinsServerAddress->Next = nullptr;
            }
            else if (strstr(line, "Adapter FirstGatewayAddress: "))
            {
                char* adapterFirstGatewayAddress = strstr(line, "Adapter FirstGatewayAddress: ") + strlen("Adapter FirstGatewayAddress: ");
                adapterFirstGatewayAddress[strlen(adapterFirstGatewayAddress) - 1] = '\0';
                AdapterAddresses->FirstGatewayAddress = (PIP_ADAPTER_GATEWAY_ADDRESS)malloc(sizeof(PIP_ADAPTER_GATEWAY_ADDRESS));
                AdapterAddresses->FirstGatewayAddress->Next = nullptr;
                }
            else if (strstr(line, "Adapter Ipv4Metric: "))
            {
                char* adapterIpv4Metric = strstr(line, "Adapter Ipv4Metric: ") + strlen("Adapter Ipv4Metric: ");
                adapterIpv4Metric[strlen(adapterIpv4Metric) - 1] = '\0';
                AdapterAddresses->Ipv4Metric = atoi(adapterIpv4Metric);
            }
            else if (strstr(line, "Adapter Ipv6Metric: "))
            {
                char* adapterIpv6Metric = strstr(line, "Adapter Ipv6Metric: ") + strlen("Adapter Ipv6Metric: ");
                adapterIpv6Metric[strlen(adapterIpv6Metric) - 1] = '\0';
                AdapterAddresses->Ipv6Metric = atoi(adapterIpv6Metric);
            }
            else if (strstr(line, "Adapter Luid.Value: "))
            {
                char* adapterLuidValue = strstr(line, "Adapter Luid.Value: ") + strlen("Adapter Luid.Value: ");
                adapterLuidValue[strlen(adapterLuidValue) - 1] = '\0';
                AdapterAddresses->Luid.Value = _atoi64(adapterLuidValue);
            }
            else if (strstr(line, "Adapter Dhcpv4Server.lpSockaddr: "))
            {
                AdapterAddresses->Dhcpv4Server.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
                AdapterAddresses->Dhcpv4Server.lpSockaddr->sa_family = AF_INET;
                ((SOCKADDR_IN*)AdapterAddresses->Dhcpv4Server.lpSockaddr)->sin_addr.s_addr = inet_addr("123.123.123.123");
                ((SOCKADDR_IN*)AdapterAddresses->Dhcpv4Server.lpSockaddr)->sin_port = 0;
            }
            else if (strstr(line, "Adapter CompartmentId: "))
            {
                char* adapterCompartmentId = strstr(line, "Adapter CompartmentId: ") + strlen("Adapter CompartmentId: ");
                adapterCompartmentId[strlen(adapterCompartmentId) - 1] = '\0';
                AdapterAddresses->CompartmentId = atoi(adapterCompartmentId);
            }
            else if (strstr(line, "Adapter NetworkGuid.Data1: "))
            {
                char* adapterNetworkGuidData1 = strstr(line, "Adapter NetworkGuid.Data1: ") + strlen("Adapter NetworkGuid.Data1: ");
                adapterNetworkGuidData1[strlen(adapterNetworkGuidData1) - 1] = '\0';
                AdapterAddresses->NetworkGuid.Data1 = atoi(adapterNetworkGuidData1);
            }
            else if (strstr(line, "Adapter ConnectionType: "))
            {
                char* adapterConnectionType = strstr(line, "Adapter ConnectionType: ") + strlen("Adapter ConnectionType: ");
                adapterConnectionType[strlen(adapterConnectionType) - 1] = '\0';
                AdapterAddresses->ConnectionType = (NET_IF_CONNECTION_TYPE)atoi(adapterConnectionType);
            }
            else if (strstr(line, "Adapter TunnelType: "))
            {
                char* adapterTunnelType = strstr(line, "Adapter TunnelType: ") + strlen("Adapter TunnelType: ");
                adapterTunnelType[strlen(adapterTunnelType) - 1] = '\0';
                AdapterAddresses->TunnelType = (TUNNEL_TYPE)atoi(adapterTunnelType);
            }
            else if (strstr(line, "Adapter Dhcpv6Server.lpSockaddr: "))
            {
                AdapterAddresses->Dhcpv6Server.lpSockaddr = (LPSOCKADDR)malloc(sizeof(SOCKADDR));
                AdapterAddresses->Dhcpv6Server.lpSockaddr->sa_family = AF_INET;
                ((SOCKADDR_IN*)AdapterAddresses->Dhcpv6Server.lpSockaddr)->sin_addr.s_addr = inet_addr("123.123.123.123");
                ((SOCKADDR_IN*)AdapterAddresses->Dhcpv6Server.lpSockaddr)->sin_port = 0;
            }
        }*/

        return NO_ERROR;
    }
}

extern "C" __declspec(dllexport) ULONG WINAPI MyGetAdaptersInfo2(
    PIP_ADAPTER_INFO pAdapterInfo,
    PULONG pOutBufLen
)
{
    LogW(L"GetAdaptersInfo2 called");

    // If adapterInfo is nullptr, we're being asked for the required buffer size.
    if (pAdapterInfo == nullptr)
    {
        //LogW(L"AdapterInfo is nullptr");
        // Assign a buffer size for a single adapter.
        *pOutBufLen = sizeof(IP_ADAPTER_INFO);

        return ERROR_BUFFER_OVERFLOW;
    }
    else
    {
        //LogW(L"AdapterInfo is not nullptr");

        if (*pOutBufLen < sizeof(IP_ADAPTER_INFO))
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }

        // Run the command ipconfig /all 
        //system("ipconfig /all > ipconfig.txt");
        // Load the results and fill the PIP_ADAPTER_INFO structure
        /*FILE* file = fopen("C:\\Games\\Lost Planet 2\\adapterinfo.txt", "r");
        if (file == NULL)
        {
            return ERROR_FILE_NOT_FOUND;
        }*/

        /*Adapter ComboIndex: 13
        Adapter Name: {A631780E-56E5-4141-90A6-DD483A377755}
        Adapter Description: Realtek USB GbE Family Controller
        Adapter Address Length: 6
        Adapter Address: 0-e0-4c-e3-2a-77
        Adapter Index: 13
        Adapter Type: 6
        Adapter DhcpEnabled: 0
        Adapter FirstIpAddress: 192.168.0.15
        Adapter FirstIpMask: 255.255.255.0
        Adapter Gateway: 192.168.0.1
        Adapter DhcpServer:
        Adapter HaveWins: 0
        Adapter PrimaryWinsServer:
        Adapter SecondaryWinsServer:
        Adapter LeaseObtained: 0
        Adapter LeaseExpires: a012525d0*/

        //pAdapterInfo->ComboIndex = 13;
        //strcpy_s(pAdapterInfo->AdapterName, "{A631780E-56E5-4141-90A6-DD483A377755}");
        //strcpy_s(pAdapterInfo->Description, "Realtek USB GbE Family Controller");
        strcpy_s(pAdapterInfo->AdapterName, "{39D3391D-335D-4311-AD86-F1A19446032F}");
        strcpy_s(pAdapterInfo->Description, "Realtek USB GbE Family Controller #3");
        /*pAdapterInfo->AddressLength = 6;
        pAdapterInfo->Address[0] = 0x00;
        pAdapterInfo->Address[1] = 0xE0;
        pAdapterInfo->Address[2] = 0x4C;
        pAdapterInfo->Address[3] = 0xE3;
        pAdapterInfo->Address[4] = 0x2A;
        pAdapterInfo->Address[5] = 0x77;*/
        pAdapterInfo->Index = 13;
        /*pAdapterInfo->Type = 6;
        pAdapterInfo->DhcpEnabled = 0;
        pAdapterInfo->CurrentIpAddress = nullptr;*/

        pAdapterInfo->IpAddressList = IP_ADDR_STRING();
        size_t length = wcslen(targetIPAddress) + 1;
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, pAdapterInfo->IpAddressList.IpAddress.String, length, targetIPAddress, _TRUNCATE);
        strcpy_s(pAdapterInfo->IpAddressList.IpMask.String, "255.255.255.0");
        pAdapterInfo->IpAddressList.Next = nullptr;

        /*pAdapterInfo->GatewayList = IP_ADDR_STRING();
        strcpy_s(pAdapterInfo->GatewayList.IpAddress.String, "192.168.0.1");
        strcpy_s(pAdapterInfo->GatewayList.IpMask.String, "255.255.255.0");
        pAdapterInfo->GatewayList.Next = nullptr;

        pAdapterInfo->DhcpServer = IP_ADDR_STRING();
        pAdapterInfo->HaveWins = 0;
        pAdapterInfo->PrimaryWinsServer = IP_ADDR_STRING();
        pAdapterInfo->SecondaryWinsServer = IP_ADDR_STRING();
        pAdapterInfo->LeaseExpires = 0;
        pAdapterInfo->LeaseObtained = 0;*/


        //// Read the file line by line
        //char line[256];
        //while (fgets(line, sizeof(line), file))
        //{
        //    if (strstr(line, "Adapter ComboIndex: "))
        //    {

        //    }
        //    else if (strstr(line, "Adapter Name: "))
        //    {
        //        char* adapterName = strstr(line, "Adapter Name: ") + strlen("Adapter Name: ");
        //        adapterName[strlen(adapterName) - 1] = '\0';
        //        strcpy_s(pAdapterInfo->AdapterName, adapterName);
        //    }
        //    else if (strstr(line, "Adapter Description: "))
        //    {
        //        char* adapterDescription = strstr(line, "Adapter Description: ") + strlen("Adapter Description: ");
        //        adapterDescription[strlen(adapterDescription) - 1] = '\0';
        //        strcpy_s(pAdapterInfo->Description, adapterDescription);
        //    }
        //    else if (strstr(line, "Adapter Address: "))
        //    {
        //        char* adapterAddress = strstr(line, "Adapter Address: ") + strlen("Adapter Address: ");
        //        adapterAddress[strlen(adapterAddress) - 1] = '\0';
        //        sscanf(adapterAddress, "%x-%x-%x-%x-%x-%x",
        //            &pAdapterInfo->Address[0],
        //            &pAdapterInfo->Address[1],
        //            &pAdapterInfo->Address[2],
        //            &pAdapterInfo->Address[3],
        //            &pAdapterInfo->Address[4],
        //            &pAdapterInfo->Address[5]);

        //        int length = 6;
        //        pAdapterInfo->AddressLength = static_cast<ULONG>(length);
        //    }
        //    else if (strstr(line, "Adapter Index: "))
        //    {
        //        char* adapterIndex = strstr(line, "Adapter Index: ") + strlen("Adapter Index: ");
        //        adapterIndex[strlen(adapterIndex) - 1] = '\0';
        //        pAdapterInfo->Index = atoi(adapterIndex);
        //    }
        //    else if (strstr(line, "Adapter Type: "))
        //    {
        //        char* adapterType = strstr(line, "Adapter Type: ") + strlen("Adapter Type: ");
        //        adapterType[strlen(adapterType) - 1] = '\0';
        //        pAdapterInfo->Type = atoi(adapterType);
        //    }
        //    else if (strstr(line, "Adapter DhcpEnabled: "))
        //    {
        //        char* adapterDhcpEnabled = strstr(line, "Adapter DhcpEnabled: ") + strlen("Adapter DhcpEnabled: ");
        //        adapterDhcpEnabled[strlen(adapterDhcpEnabled) - 1] = '\0';
        //        pAdapterInfo->DhcpEnabled = atoi(adapterDhcpEnabled);
        //    }
        //    else if (strstr(line, "Adapter FirstIpAddress: "))
        //    {
        //        char* adapterFirstUnicastAddress = strstr(line, "Adapter FirstIpAddress: ") + strlen("Adapter FirstIpAddress: ");
        //        adapterFirstUnicastAddress[strlen(adapterFirstUnicastAddress) - 1] = '\0'; 
        //        pAdapterInfo->IpAddressList = IP_ADDR_STRING();
        //        pAdapterInfo->IpAddressList.IpAddress = IP_ADDRESS_STRING();
        //        strcpy_s(pAdapterInfo->IpAddressList.IpAddress.String, adapterFirstUnicastAddress);
        //        strcpy_s(pAdapterInfo->IpAddressList.IpMask.String, "255.255.255.0");
        //        pAdapterInfo->IpAddressList.Next = nullptr;
        //    }
        //    else if (strstr(line, "Adapter Gateway: "))
        //    {
        //        char* adapterGateway = strstr(line, "Adapter Gateway: ") + strlen("Adapter Gateway: ");
        //        adapterGateway[strlen(adapterGateway) - 1] = '\0';
        //        pAdapterInfo->GatewayList = IP_ADDR_STRING();
        //        pAdapterInfo->GatewayList.IpAddress = IP_ADDRESS_STRING();
        //        strcpy_s(pAdapterInfo->GatewayList.IpAddress.String, adapterGateway);
        //        strcpy_s(pAdapterInfo->GatewayList.IpMask.String, "255.255.255.0");
        //        pAdapterInfo->GatewayList.Next = nullptr;
        //    }
        //    //else if (strstr(line, "Adapter DhcpServer: "))
        //    //{
        //    //    /*char* adapterDhcpServer = strstr(line, "Adapter DhcpServer: ") + strlen("Adapter DhcpServer: ");
        //    //    adapterDhcpServer[strlen(adapterDhcpServer) - 1] = '\0';
        //    //    strcpy_s(pAdapterInfo->DhcpServer.IpAddress.String, adapterDhcpServer);*/
        //    //    pAdapterInfo->DhcpServer = IP_ADDR_STRING();
        //    //}
        //    else if (strstr(line, "Adapter HaveWins: "))
        //    {
        //        char* adapterHaveWins = strstr(line, "Adapter HaveWins: ") + strlen("Adapter HaveWins: ");
        //        adapterHaveWins[strlen(adapterHaveWins) - 1] = '\0';
        //        pAdapterInfo->HaveWins = atoi(adapterHaveWins);
        //    }
        //    //else if (strstr(line, "Adapter PrimaryWinsServer: "))
        //    //{
        //    //    /*char* adapterPrimaryWinsServer = strstr(line, "Adapter PrimaryWinsServer: ") + strlen("Adapter PrimaryWinsServer: ");
        //    //    adapterPrimaryWinsServer[strlen(adapterPrimaryWinsServer) - 1] = '\0';*/
        //    //    pAdapterInfo->PrimaryWinsServer = IP_ADDR_STRING();
        //    //    pAdapterInfo->PrimaryWinsServer.IpAddress = IP_ADDRESS_STRING();
        //    //    strcpy_s(pAdapterInfo->PrimaryWinsServer.IpAddress.String, "123.123.123.123");
        //    //    strcpy_s(pAdapterInfo->PrimaryWinsServer.IpMask.String, "255.255.255.0");
        //    //    pAdapterInfo->PrimaryWinsServer.Next = nullptr;
        //    //}
        //    //else if (strstr(line, "Adapter SecondaryWinsServer: "))
        //    //{
        //    //    /*char* adapterSecondaryWinsServer = strstr(line, "Adapter SecondaryWinsServer: ") + strlen("Adapter SecondaryWinsServer: ");
        //    //    adapterSecondaryWinsServer[strlen(adapterSecondaryWinsServer) - 1] = '\0';*/
        //    //    pAdapterInfo->SecondaryWinsServer = IP_ADDR_STRING();
        //    //    pAdapterInfo->SecondaryWinsServer.IpAddress = IP_ADDRESS_STRING();
        //    //    strcpy_s(pAdapterInfo->SecondaryWinsServer.IpAddress.String, "123.123.123.123");
        //    //    strcpy_s(pAdapterInfo->SecondaryWinsServer.IpMask.String, "255.255.255.0");
        //    //    pAdapterInfo->SecondaryWinsServer.Next = nullptr;
        //    //}
        //}

        //fclose(file);

        return NO_ERROR;
    }
}

// Proxy fuction for GetBestInterfaceEx
extern "C" __declspec(dllexport) DWORD WINAPI MyGetBestInterfaceEx(
    struct sockaddr* pDestAddr,
    PDWORD pdwBestIfIndex
)
{
    LogW(L"GetBestInterfaceEx called");


    /*HMODULE hIphlpapi = nullptr;

    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat_s(path, "\\iphlpapi.dll");
    hIphlpapi = LoadLibraryA(path);

    using GetBestInterfaceEx_t = DWORD(WINAPI*)(struct sockaddr*, PDWORD);
    GetBestInterfaceEx_t pfnGetBestInterfaceEx = (GetBestInterfaceEx_t)GetProcAddress(hIphlpapi, "GetBestInterfaceEx");

    DWORD dwResult = pfnGetBestInterfaceEx(pDestAddr, pdwBestIfIndex);

    FreeLibrary(hIphlpapi);

    return dwResult;*/

    *pdwBestIfIndex = 13;

    //Log the destination address
    char destAddr[256];
    inet_ntop(AF_INET, &((struct sockaddr_in*)pDestAddr)->sin_addr, destAddr, sizeof(destAddr));
    LogW(L"Destination address: ");
    std::wstringstream logMessage;
    logMessage << destAddr;
    LogW(logMessage.str());

    return NO_ERROR;
}

// Proxy fuction for GetIfEntry
extern "C" __declspec(dllexport) DWORD WINAPI MyGetIfEntry(
    PMIB_IFROW pIfRow
)
{
    LogW(L"GetIfEntry called");
    // Log the interface index
    std::wstringstream logMessage;
    logMessage << "Interface index: " << pIfRow->dwIndex;
    LogW(logMessage.str());

    /*
    Interface Name: \DEVICE\TCPIP_{59FBA4C8-88B4-4F88-9A11-8A11707A2AD1}
    Interface Index: 13
    Interface Type: 6
    Interface Mtu: 1500
    Interface Speed: 0
    Interface PhysAddr Length: 6
    Interface PhysAddr: 12-4-d0-75-83-cc
    Interface Admin Status: 1
    Interface Oper Status: 0
    Interface Last Change: 0
    Interface In Octets: 0
    Interface In Ucast Pkts: 0
    Interface In Nucast Pkts: 0
    Interface In Discards: 0
    Interface In Errors: 0
    Interface In Unknown Protos: 0
    Interface Out Octets: 0
    Interface Out Ucast Pkts: 0
    Interface Out Nucast Pkts: 0
    Interface Out Discards: 0
    Interface Out Errors: 0
    Interface Out Queue Length: 0
    Interface Descr Len: 23
    Interface Descr: Realtek PCIe GBE Family Controller*/

    // Fill in the MIB_IFROW structure
    // Interface Name: \DEVICE\TCPIP_{59FBA4C8-88B4-4F88-9A11-8A11707A2AD1}
    //wchar_t name[MAX_INTERFACE_NAME_LEN];
    //wcscpy(pIfRow->wszName, L"\\DEVICE\\TCPIP_{59FBA4C8-88B4-4F88-9A11-8A11707A2AD1}");

    //pIfRow->dwIndex = 13;
    //pIfRow->dwType = MIB_IF_TYPE_ETHERNET;
    //pIfRow->dwMtu = 1500;
    //pIfRow->dwSpeed = 0;
    //pIfRow->dwPhysAddrLen = 6;
    ///*unsigned char macAddress[6] = { 0x00, 0xE0, 0x4C, 0xE3, 0x2A, 0x77 };
    //memcpy(AdapterAddresses->PhysicalAddress, macAddress, 6);*/
    //pIfRow->bPhysAddr[0] = 0x12;
    //pIfRow->bPhysAddr[1] = 0x4;
    //pIfRow->bPhysAddr[2] = 0xD0;
    //pIfRow->bPhysAddr[3] = 0x75;
    //pIfRow->bPhysAddr[4] = 0x83;
    //pIfRow->bPhysAddr[5] = 0xCC;
    //pIfRow->dwAdminStatus = 1;
    //pIfRow->dwOperStatus = IF_OPER_STATUS_OPERATIONAL;
    /*pIfRow->dwLastChange = 0;
    pIfRow->dwInOctets = 0;
    pIfRow->dwInUcastPkts = 0;
    pIfRow->dwInNUcastPkts = 0;
    pIfRow->dwInDiscards = 0;
    pIfRow->dwInErrors = 0;
    pIfRow->dwInUnknownProtos = 0;
    pIfRow->dwOutOctets = 0;
    pIfRow->dwOutUcastPkts = 0;
    pIfRow->dwOutNUcastPkts = 0;
    pIfRow->dwOutDiscards = 0;
    pIfRow->dwOutErrors = 0;
    pIfRow->dwOutQLen = 0;
    pIfRow->dwDescrLen = 23;*/

    /*size_t length = strlen("Realtek PCIe GBE Family Controller");
    unsigned char description[sizeof(length + 1)];
    for (size_t i = 0; i < length; ++i) {
        pIfRow->bDescr[i] = static_cast<unsigned char>("Realtek PCIe GBE Family Controller"[i]);
    }
    pIfRow->bDescr[length] = '\0';*/

    return NO_ERROR;
}

// Proxy fuction for GetIpForwardTable
extern "C" __declspec(dllexport) DWORD WINAPI MyGetIpForwardTable(
    PMIB_IPFORWARDTABLE pIpForwardTable,
    PULONG pdwSize,
    BOOL bOrder
)
{
    LogW(L"GetIpForwardTable called");

    //// Load the original iphlpapi.dll
    //HMODULE hIphlpapi = LoadLibrary(L"iphlpapi.dll");
    //if (hIphlpapi == NULL)
    //{
    //    return ERROR_MOD_NOT_FOUND;
    //}

    //// Get the original function pointer
    //using GetIpForwardTable_t = DWORD(WINAPI*)(PMIB_IPFORWARDTABLE, PULONG, BOOL);
    //GetIpForwardTable_t pfnGetIpForwardTable = (GetIpForwardTable_t)GetProcAddress(hIphlpapi, "GetIpForwardTable");
    //if (pfnGetIpForwardTable == NULL)
    //{
    //    FreeLibrary(hIphlpapi);
    //    return ERROR_PROC_NOT_FOUND;
    //}

    //// Call the original function
    //DWORD dwResult = pfnGetIpForwardTable(pIpForwardTable, pdwSize, bOrder);

    //// Free the original DLL
    //FreeLibrary(hIphlpapi);

    //return dwResult;

    return GetIpForwardTable(pIpForwardTable, pdwSize, bOrder);
}

// Proxy fuction for GetIpStatisticsEx
extern "C" __declspec(dllexport) DWORD WINAPI MyGetIpStatisticsEx(
    PMIB_IPSTATS pStats,
    DWORD dwFamily
)
{
    LogW(L"GetIpStatisticsEx called");

    // Load the original iphlpapi.dll
    //HMODULE hIphlpapi = LoadLibrary(L"iphlpapi.dll");
    //if (hIphlpapi == NULL)
    //{
    //    return ERROR_MOD_NOT_FOUND;
    //}

    //// Get the original function pointer
    //using GetIpStatisticsEx_t = DWORD(WINAPI*)(PMIB_IPSTATS, DWORD);
    //GetIpStatisticsEx_t pfnGetIpStatisticsEx = (GetIpStatisticsEx_t)GetProcAddress(hIphlpapi, "GetIpStatisticsEx");
    //if (pfnGetIpStatisticsEx == NULL)
    //{
    //    FreeLibrary(hIphlpapi);
    //    return ERROR_PROC_NOT_FOUND;
    //}

    //// Call the original function
    //DWORD dwResult = pfnGetIpStatisticsEx(pStats, dwFamily);

    //// Free the original DLL
    //FreeLibrary(hIphlpapi);

    //return dwResult;

    return GetIpStatisticsEx(pStats, dwFamily);
}

// Proxy fuction for GetNetworkParams
extern "C" __declspec(dllexport) DWORD WINAPI MyGetNetworkParams(
    PFIXED_INFO pFixedInfo,
    PULONG pOutBufLen
)
{
    LogW(L"GetNetworkParams called");

    //pFixedInfo = (FIXED_INFO*)malloc(sizeof(FIXED_INFO));

    /*Host Name : DESKTOP - 5QTTFAA
    Domain Name :
    DNS Servers :
        75.75.75.75
        75.75.76.76
    Node Type : Hybrid
    NetBIOS Scope ID :
    Enable Routing : No
    Enable Proxy : No
    Enable DNS : No*/

    if (pFixedInfo != nullptr) {
        // Fill in the FIXED_INFO structure

        /*size_t length = wcslen(targetHostname) + 1;
        char* narrowString = new char[length];
        size_t convertedChars = 0;*/
       // wcstombs_s(&convertedChars, pFixedInfo->HostName, length, targetHostname, _TRUNCATE);

        //strcpy_s(pFixedInfo->DomainName, "");
        // assign "75.75.75.75" to CurrentDnsServer
        //PIP_ADDR_STRING pAddrString = (PIP_ADDR_STRING)malloc(sizeof(IP_ADDR_STRING));
        pFixedInfo->CurrentDnsServer = (PIP_ADDR_STRING)malloc(sizeof(IP_ADDR_STRING));
        pFixedInfo->CurrentDnsServer->Next = nullptr;
        strcpy(pFixedInfo->CurrentDnsServer->IpAddress.String, "75.75.75.75");
        strcpy(pFixedInfo->CurrentDnsServer->IpMask.String, "255.255.255.0");
        pFixedInfo->CurrentDnsServer->Context = 0;

        /*strcpy_s(pFixedInfo->DnsServerList.IpAddress.String, "75.75.75.75");
        pFixedInfo->DnsServerList.Next = nullptr;
        pFixedInfo->NodeType = 0;
        strcpy_s(pFixedInfo->ScopeId, "");
        pFixedInfo->EnableRouting = FALSE;
        pFixedInfo->EnableProxy = FALSE;
        pFixedInfo->EnableDns = FALSE;*/
    }
    else
    {
        *pOutBufLen = sizeof(FIXED_INFO);

        return ERROR_BUFFER_OVERFLOW;
    }

    // Update the buffer length
    *pOutBufLen = sizeof(FIXED_INFO);

    return NO_ERROR;
}

typedef ULONG(WINAPI* GetAdaptersAddresses_t)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
GetAdaptersAddresses_t pfnGetAdaptersAddresses;




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    static HMODULE hIphlpapi = nullptr;
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_ADDRESSES adapterAddresses = nullptr;
    ULONG dwResult = 0;

    std::wstring iniFilePath = L"";
    std::wstringstream logMessage;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        ZeroMemory(tempString, sizeof(tempString));
        ZeroMemory(tempStringW, sizeof(tempStringW));

        // See if there is a log file which indicates we should log our progress.
        hLogFile = CreateFile(L"iphlpapi.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hLogFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwWritten = 0;
            StringCchPrintf(tempString, sizeof(tempString) / sizeof(TCHAR), L"(%d): iphlpapi.dll\r\n", GetCurrentProcessId());
            SetFilePointer(hLogFile, 0, 0, FILE_END);
            if (!WriteFile(hLogFile, tempString, (DWORD)(_tcslen(tempString) * sizeof(TCHAR)), &dwWritten, NULL))
            {
                // If we can't write our first line then don't bother trying to log later.
                CloseHandle(hLogFile);
                hLogFile = INVALID_HANDLE_VALUE;
            }
        }
        

        WCHAR executablePath[MAX_PATH];
        if (GetCurrentDirectory(MAX_PATH, executablePath)) {
            iniFilePath = std::wstring(executablePath) + L"\\iphlpapi.ini";
        }

        WCHAR targetIPBuffer[256];
        GetPrivateProfileString(L"Settings", L"TargetIPAddress", L"", targetIPBuffer, sizeof(targetIPBuffer), iniFilePath.c_str());

        targetIPAddress = new WCHAR[wcslen(targetIPBuffer) + 1];
        wcscpy(targetIPAddress, targetIPBuffer);
        LogW(L"TargetIPAddress");
        LogW(targetIPAddress);

        WCHAR targetHostnameBuffer[256];
        GetPrivateProfileString(L"Settings", L"TargetHostname", L"DefaultHostname", targetHostnameBuffer, sizeof(targetHostnameBuffer), iniFilePath.c_str());

        targetHostname = new WCHAR[wcslen(targetHostnameBuffer) + 1];
        wcscpy(targetHostname, targetHostnameBuffer);
        LogW(L"TargetHostname: ");
        LogW(targetHostname);

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
