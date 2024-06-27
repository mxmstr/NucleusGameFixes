#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <fstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int main()
{
    // Call GetAdaptersInfo and print the adapter information
    ULONG bufferSize = 0;
    PIP_ADAPTER_INFO adapterInfo = nullptr;

    // Call GetAdaptersInfo to get the required buffer size
    DWORD result = GetAdaptersInfo(nullptr, &bufferSize);
    if (result == ERROR_BUFFER_OVERFLOW)
    {
        adapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new char[bufferSize]);

        // Call GetAdaptersInfo again to get the adapter information
        result = GetAdaptersInfo(adapterInfo, &bufferSize);
        if (result == NO_ERROR)
        {
            PIP_ADAPTER_INFO currentAdapter = adapterInfo;
            while (currentAdapter != nullptr)
            {
                std::cout << "Adapter ComboIndex: " << currentAdapter->ComboIndex << std::endl;
                std::cout << "Adapter Name: " << currentAdapter->AdapterName << std::endl;
                std::cout << "Adapter Description: " << currentAdapter->Description << std::endl;
                std::cout << "Adapter Address Length: " << currentAdapter->AddressLength << std::endl;
                std::cout << "Adapter Address: ";
                for (UINT i = 0; i < currentAdapter->AddressLength; i++)
                {
                    if (i == currentAdapter->AddressLength - 1)
                    {
                        std::cout << std::hex << static_cast<int>(currentAdapter->Address[i]);
                    }
                    else
                    {
                        std::cout << std::hex << static_cast<int>(currentAdapter->Address[i]) << "-";
                    }
                }
                std::cout << std::endl;
                std::cout << "Adapter Index: " << currentAdapter->Index << std::endl;
                std::cout << "Adapter Type: " << currentAdapter->Type << std::endl;
                std::cout << "Adapter DhcpEnabled: " << currentAdapter->DhcpEnabled << std::endl;
                //std::cout << "Adapter CurrentIpAddress: " << currentAdapter->CurrentIpAddress->IpAddress.String << std::endl;
                //std::cout << "Adapter CurrentIpMask: " << currentAdapter->CurrentIpAddress->IpMask.String << std::endl;
                std::cout << "Adapter FirstIpAddress: " << currentAdapter->IpAddressList.IpAddress.String << std::endl;
                std::cout << "Adapter FirstIpMask: " << currentAdapter->IpAddressList.IpMask.String << std::endl;
                std::cout << "Adapter Gateway: " << currentAdapter->GatewayList.IpAddress.String << std::endl;
                std::cout << "Adapter DhcpServer: " << currentAdapter->DhcpServer.IpAddress.String << std::endl;
                std::cout << "Adapter HaveWins: " << currentAdapter->HaveWins << std::endl;
                std::cout << "Adapter PrimaryWinsServer: " << currentAdapter->PrimaryWinsServer.IpAddress.String << std::endl;
                std::cout << "Adapter SecondaryWinsServer: " << currentAdapter->SecondaryWinsServer.IpAddress.String << std::endl;
                std::cout << "Adapter LeaseObtained: " << currentAdapter->LeaseObtained << std::endl;
                std::cout << "Adapter LeaseExpires: " << currentAdapter->LeaseExpires << std::endl;
                std::cout << std::endl;

                currentAdapter = currentAdapter->Next;
            }
        }
    }

    /*typedef struct _IP_ADAPTER_INFO {
        struct _IP_ADAPTER_INFO* Next;
        DWORD                   ComboIndex;
        char                    AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
        char                    Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
        UINT                    AddressLength;
        BYTE                    Address[MAX_ADAPTER_ADDRESS_LENGTH];
        DWORD                   Index;
        UINT                    Type;
        UINT                    DhcpEnabled;
        PIP_ADDR_STRING         CurrentIpAddress;
        IP_ADDR_STRING          IpAddressList;
        IP_ADDR_STRING          GatewayList;
        IP_ADDR_STRING          DhcpServer;
        BOOL                    HaveWins;
        IP_ADDR_STRING          PrimaryWinsServer;
        IP_ADDR_STRING          SecondaryWinsServer;
        time_t                  LeaseObtained;
        time_t                  LeaseExpires;
    } IP_ADAPTER_INFO, * PIP_ADAPTER_INFO;*/

    //ULONG bufferSize = 0;
    //PIP_ADAPTER_ADDRESSES adapterAddresses = nullptr;

    //// Call GetAdaptersAddresses to get the required buffer size
    //DWORD result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    //if (result == ERROR_BUFFER_OVERFLOW)
    //{
    //    adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new char[bufferSize]);

    //    // Call GetAdaptersAddresses again to get the adapter addresses
    //    result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapterAddresses, &bufferSize);

    //    if (result == NO_ERROR)
    //    {
    //        PIP_ADAPTER_ADDRESSES currentAdapter = adapterAddresses;
    //        while (currentAdapter != nullptr)
    //        {
    //            std::cout << "Adapter Name: " << std::string(currentAdapter->AdapterName) << std::endl;
    //            std::wstring Description(currentAdapter->Description);
    //            std::cout << "Adapter Description: " << std::string(Description.begin(), Description.end()) << std::endl;
    //            std::wstring FriendlyName(currentAdapter->FriendlyName);
    //            std::cout << "Adapter FriendlyName: " << std::string(FriendlyName.begin(), FriendlyName.end()) << std::endl;
    //            std::cout << "Adapter Physical Address Length: " << currentAdapter->PhysicalAddressLength << std::endl;
    //            std::cout << "Adapter Physical Address: ";
    //            for (ULONG i = 0; i < currentAdapter->PhysicalAddressLength; i++)
    //            {
    //                if (i == currentAdapter->PhysicalAddressLength - 1)
    //                {
    //                    std::cout << std::hex << static_cast<int>(currentAdapter->PhysicalAddress[i]);
    //                }
    //                else
    //                {
    //                    std::cout << std::hex << static_cast<int>(currentAdapter->PhysicalAddress[i]) << "-";
    //                }
    //            }
    //            std::cout << std::endl;

    //            std::cout << "Adapter Alignment: " << currentAdapter->Alignment << std::endl;
    //            std::cout << "Adapter Length: " << currentAdapter->Length << std::endl;
    //            std::cout << "Adapter Index: " << currentAdapter->IfIndex << std::endl;
    //            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = currentAdapter->FirstUnicastAddress;
    //            while (pUnicast != nullptr)
    //            {
    //                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
    //                {
    //                    SOCKADDR_IN* sa_in = (SOCKADDR_IN*)pUnicast->Address.lpSockaddr;
    //                    char buffer[INET_ADDRSTRLEN];
    //                    inet_ntop(AF_INET, &(sa_in->sin_addr), buffer, INET_ADDRSTRLEN);
    //                    std::cout << "Adapter FirstUnicastAddress: " << buffer << std::endl;
    //                }
    //                pUnicast = pUnicast->Next;
    //            }
    //            PIP_ADAPTER_ANYCAST_ADDRESS_XP pAnycast = currentAdapter->FirstAnycastAddress;
    //            while (pAnycast != nullptr)
    //            {
    //                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET)
    //                {
    //                    SOCKADDR_IN* sa_in = (SOCKADDR_IN*)pAnycast->Address.lpSockaddr;
    //                    char buffer[INET_ADDRSTRLEN];
    //                    inet_ntop(AF_INET, &(sa_in->sin_addr), buffer, INET_ADDRSTRLEN);
    //                    std::cout << "Adapter FirstAnycastAddress: " << buffer << std::endl;
    //                }
    //                pAnycast = pAnycast->Next;
    //            }
    //            PIP_ADAPTER_MULTICAST_ADDRESS_XP pMulticast = currentAdapter->FirstMulticastAddress;
    //            while (pMulticast != nullptr)
    //            {
    //                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET)
    //                {
    //                    SOCKADDR_IN* sa_in = (SOCKADDR_IN*)pMulticast->Address.lpSockaddr;
    //                    char buffer[INET_ADDRSTRLEN];
    //                    inet_ntop(AF_INET, &(sa_in->sin_addr), buffer, INET_ADDRSTRLEN);
    //                    std::cout << "Adapter FirstMulticastAddress: " << buffer << std::endl;
    //                }
    //                pMulticast = pMulticast->Next;
    //            }
    //            PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = currentAdapter->FirstDnsServerAddress;
    //            while (pDns != nullptr)
    //            {
    //                if (pDns->Address.lpSockaddr->sa_family == AF_INET)
    //                {
    //                    SOCKADDR_IN* sa_in = (SOCKADDR_IN*)pDns->Address.lpSockaddr;
    //                    char buffer[INET_ADDRSTRLEN];
    //                    inet_ntop(AF_INET, &(sa_in->sin_addr), buffer, INET_ADDRSTRLEN);
    //                    std::cout << "Adapter FirstDnsServerAddress: " << buffer << std::endl;
    //                }
    //                pDns = pDns->Next;
    //            }
    //            std::cout << "Adapter DnsSuffix: " << currentAdapter->DnsSuffix << std::endl;
    //            std::cout << "Adapter Flags: " << currentAdapter->Flags << std::endl;
    //            std::cout << "Adapter DdnsEnabled: " << currentAdapter->DdnsEnabled << std::endl;
    //            std::cout << "Adapter RegisterAdapterSuffix: " << currentAdapter->RegisterAdapterSuffix << std::endl;
    //            std::cout << "Adapter Dhcpv4Enabled: " << currentAdapter->Dhcpv4Enabled << std::endl;
    //            std::cout << "Adapter ReceiveOnly: " << currentAdapter->ReceiveOnly << std::endl;
    //            std::cout << "Adapter NoMulticast: " << currentAdapter->NoMulticast << std::endl;
    //            std::cout << "Adapter Ipv6OtherStatefulConfig: " << currentAdapter->Ipv6OtherStatefulConfig << std::endl;
    //            std::cout << "Adapter NetbiosOverTcpipEnabled: " << currentAdapter->NetbiosOverTcpipEnabled << std::endl;
    //            std::cout << "Adapter Ipv4Enabled: " << currentAdapter->Ipv4Enabled << std::endl;
    //            std::cout << "Adapter Ipv6Enabled: " << currentAdapter->Ipv6Enabled << std::endl;
    //            std::cout << "Adapter Ipv6ManagedAddressConfigurationSupported: " << currentAdapter->Ipv6ManagedAddressConfigurationSupported << std::endl;
    //            std::cout << "Adapter Mtu: " << currentAdapter->Mtu << std::endl;
    //            std::cout << "Adapter IfType: " << currentAdapter->IfType << std::endl;
    //            std::cout << "Adapter OperStatus: " << currentAdapter->OperStatus << std::endl;
    //            std::cout << "Adapter Ipv6IfIndex: " << currentAdapter->Ipv6IfIndex << std::endl;
    //            //std::cout << "Adapter ZoneIndices: " << currentAdapter->ZoneIndices << std::endl;
    //            std::cout << "Adapter FirstPrefix: " << currentAdapter->FirstPrefix << std::endl;
    //            std::cout << "Adapter TransmitLinkSpeed: " << currentAdapter->TransmitLinkSpeed << std::endl;
    //            std::cout << "Adapter ReceiveLinkSpeed: " << currentAdapter->ReceiveLinkSpeed << std::endl;
    //            std::cout << "Adapter FirstWinsServerAddress: " << currentAdapter->FirstWinsServerAddress << std::endl;
    //            std::cout << "Adapter FirstGatewayAddress: " << currentAdapter->FirstGatewayAddress << std::endl;
    //            std::cout << "Adapter Ipv4Metric: " << currentAdapter->Ipv4Metric << std::endl;
    //            std::cout << "Adapter Ipv6Metric: " << currentAdapter->Ipv6Metric << std::endl;
    //            std::cout << "Adapter Luid.Value: " << currentAdapter->Luid.Value << std::endl;
    //            std::cout << "Adapter Dhcpv4Server.lpSockaddr: " << currentAdapter->Dhcpv4Server.lpSockaddr << std::endl;
    //            std::cout << "Adapter CompartmentId: " << currentAdapter->CompartmentId << std::endl;
    //            std::cout << "Adapter NetworkGuid.Data1: " << currentAdapter->NetworkGuid.Data1 << std::endl;
    //            std::cout << "Adapter ConnectionType: " << currentAdapter->ConnectionType << std::endl;
    //            std::cout << "Adapter TunnelType: " << currentAdapter->TunnelType << std::endl;
    //            std::cout << "Adapter Dhcpv6Server.lpSockaddr: " << currentAdapter->Dhcpv6Server.lpSockaddr << std::endl;
    //            //std::cout << "Adapter Dhcpv6ClientDuid: " << currentAdapter->Dhcpv6ClientDuid << std::endl;
    //            std::cout << "Adapter Dhcpv6ClientDuidLength: " << currentAdapter->Dhcpv6ClientDuidLength << std::endl;
    //            std::cout << "Adapter Dhcpv6Iaid: " << currentAdapter->Dhcpv6Iaid << std::endl;
    //            std::cout << "Adapter FirstDnsSuffix: " << currentAdapter->FirstDnsSuffix << std::endl;

    //            std::cout << std::endl;

    //            currentAdapter = currentAdapter->Next;
    //        }
    //    }
    //    else
    //    {
    //        std::cerr << "Failed to get adapter addresses. Error code: " << result << std::endl;
    //    }

    //    delete[] reinterpret_cast<char*>(adapterAddresses);
    //}
    //else
    //{
    //    std::cerr << "Failed to get buffer size. Error code: " << result << std::endl;
    //}

    //// Call GetIfEntry for index 13
    //MIB_IFROW ifRow;
    //ifRow.dwIndex = 13;
    //DWORD result = GetIfEntry(&ifRow);
    //if (result == NO_ERROR)
    //{
    //    std::wcout << "Interface Name: " << ifRow.wszName << std::endl;
    //    std::cout << "Interface Index: " << ifRow.dwIndex << std::endl;
    //    std::cout << "Interface Type: " << ifRow.dwType << std::endl;
    //    std::cout << "Interface Mtu: " << ifRow.dwMtu << std::endl;
    //    std::cout << "Interface Speed: " << ifRow.dwSpeed << std::endl;
    //    std::cout << "Interface PhysAddr Length: " << ifRow.dwPhysAddrLen << std::endl;
    //    std::cout << "Interface PhysAddr: ";
    //    for (UINT i = 0; i < ifRow.dwPhysAddrLen; i++)
    //    {
    //        if (i == ifRow.dwPhysAddrLen - 1)
    //        {
    //            std::cout << std::hex << static_cast<int>(ifRow.bPhysAddr[i]);
    //        }
    //        else
    //        {
    //            std::cout << std::hex << static_cast<int>(ifRow.bPhysAddr[i]) << "-";
    //        }
    //    }
    //    std::cout << std::endl;
    //    std::cout << "Interface Admin Status: " << ifRow.dwAdminStatus << std::endl;
    //    std::cout << "Interface Oper Status: " << ifRow.dwOperStatus << std::endl;
    //    std::cout << "Interface Last Change: " << ifRow.dwLastChange << std::endl;
    //    std::cout << "Interface In Octets: " << ifRow.dwInOctets << std::endl;
    //    std::cout << "Interface In Ucast Pkts: " << ifRow.dwInUcastPkts << std::endl;
    //    std::cout << "Interface In Nucast Pkts: " << ifRow.dwInNUcastPkts << std::endl;
    //    std::cout << "Interface In Discards: " << ifRow.dwInDiscards << std::endl;
    //    std::cout << "Interface In Errors: " << ifRow.dwInErrors << std::endl;
    //    std::cout << "Interface In Unknown Protos: " << ifRow.dwInUnknownProtos << std::endl;
    //    std::cout << "Interface Out Octets: " << ifRow.dwOutOctets << std::endl;
    //    std::cout << "Interface Out Ucast Pkts: " << ifRow.dwOutUcastPkts << std::endl;
    //    std::cout << "Interface Out Nucast Pkts: " << ifRow.dwOutNUcastPkts << std::endl;
    //    std::cout << "Interface Out Discards: " << ifRow.dwOutDiscards << std::endl;
    //    std::cout << "Interface Out Errors: " << ifRow.dwOutErrors << std::endl;
    //    std::cout << "Interface Out Queue Length: " << ifRow.dwOutQLen << std::endl;
    //    std::cout << "Interface Descr Len: " << ifRow.dwDescrLen << std::endl;
    //    std::cout << "Interface Descr: " << ifRow.bDescr << std::endl;
    //}



    /*FIXED_INFO *pFixedInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;
    IP_ADDR_STRING *pIPAddr;

    pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
    if (pFixedInfo == NULL) {
        printf("Error allocating memory needed to call GetNetworkParams\n");
        return 1;
    }
    ulOutBufLen = sizeof (FIXED_INFO);

    if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pFixedInfo);
        pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
        if (pFixedInfo == NULL) {
            printf("Error allocating memory needed to call GetNetworkParams\n");
            return 1;
        }
    }

    if (dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR) {
        printf("Host Name: %s\n", pFixedInfo->HostName);
        printf("Domain Name: %s\n", pFixedInfo->DomainName);
        printf("Current DNS Server: %s\n", pFixedInfo->DnsServerList.IpAddress.String);
        printf("DNS Servers:\n");
        printf("    %s\n", pFixedInfo->DnsServerList.IpAddress.String);
        pIPAddr = pFixedInfo->DnsServerList.Next;
        while (pIPAddr) {
            printf("    %s\n", pIPAddr->IpAddress.String);
            pIPAddr = pIPAddr->Next;
        }
        printf("Node Type: ");
        switch (pFixedInfo->NodeType) {
            case 1:
                printf("Broadcast\n");
                break;
            case 2:
                printf("Peer to Peer\n");
                break;
            case 4:
                printf("Mixed\n");
                break;
            case 8:
                printf("Hybrid\n");
                break;
            default:
                printf("Unknown node type %ld\n", pFixedInfo->NodeType);
                break;
        }
        printf("NetBIOS Scope ID: %s\n", pFixedInfo->ScopeId);
        printf("Enable Routing: ");
        if (pFixedInfo->EnableRouting)
            printf("Yes\n");
        else
            printf("No\n");
        printf("Enable Proxy: ");
        if (pFixedInfo->EnableProxy)
            printf("Yes\n");
        else
            printf("No\n");
        printf("Enable DNS: ");
        if (pFixedInfo->EnableDns)
            printf("Yes\n");
        else
            printf("No\n");
    }
    else {
        printf("GetNetworkParams failed with error: %d\n", dwRetVal);
    }*/


    return 0;
}
