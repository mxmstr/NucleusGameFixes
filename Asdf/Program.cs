using System;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("iphlpapi.dll", SetLastError = true)]
    public static extern uint GetAdaptersAddresses(uint family, uint flags, IntPtr reserved, IntPtr addresses, ref uint size);

    [StructLayout(LayoutKind.Sequential)]
    public struct IP_ADAPTER_ADDRESSES
    {
        // Specify all fields here
        /*public IntPtr Next;
        public uint IfIndex;
        public IntPtr AdapterName;
        public IntPtr FriendlyName;
            public IntPtr Description;
        public byte[] Address;
        public uint PhysicalAddressLength;
        public byte[] PhysicalAddress;
        public uint Flags;*/

        //Name: {41535F46-E000-4E25-A6FB-8D1E8E36AF2F}
        public IntPtr AdapterName;
        //Description: Famatech Radmin VPN Ethernet Adapter
        public IntPtr Description;
        //FriendlyName: Radmin VPN
        public IntPtr FriendlyName;
        //Physical Address Length: 6
        public uint PhysicalAddressLength;
        //Physical Address: 2-50-da-e4-35-b1
        public byte[] PhysicalAddress;
        //Alignment: 900000178
        public uint Flags;
        //Length: 178
        public uint Length;
        //Index: 9
        public uint IfIndex;
        //FirstUnicastAddress: 26.233.216.10
        public IntPtr FirstUnicastAddress;
        //FirstMulticastAddress: 224.0.0.1
        public IntPtr FirstMulticastAddress;
        //DnsSuffix: 00ED0910
        //Flags: 1c5
        //DdnsEnabled: 1
        //RegisterAdapterSuffix: 0
        //Dhcpv4Enabled: 1
        //ReceiveOnly: 0
        //NoMulticast: 0
        //Ipv6OtherStatefulConfig: 0
        //NetbiosOverTcpipEnabled: 1
        //Ipv4Enabled: 1
        //Ipv6Enabled: 1
        //Ipv6ManagedAddressConfigurationSupported: 0
        //Mtu: 5dc
        //IfType: 6
        //OperStatus: 1
        //Ipv6IfIndex: 9
        //ZoneIndices: 00ED07E4
        //FirstPrefix: 00ED0D1C
        //TransmitLinkSpeed: 5f5e100
        //ReceiveLinkSpeed: 5f5e100
        //FirstWinsServerAddress: 00000000
        //FirstGatewayAddress: 00000000
        //Ipv4Metric: 1
        //Ipv6Metric: 23
        //Luid.Value: 6008014000000
        //Dhcpv4Server.lpSockaddr: 00000000
        //CompartmentId: 1
        //NetworkGuid.Data1: 2e9f0c89
        //ConnectionType: 1
        //TunnelType: 0
        //Dhcpv6Server.lpSockaddr: 00000000
        //Dhcpv6ClientDuid:
        //Dhcpv6ClientDuidLength: e
        //Dhcpv6Iaid: 480250da
        //FirstDnsSuffix: 00000000

        public IP_ADAPTER_ADDRESSES(IntPtr ptr)
        {
            this = (IP_ADAPTER_ADDRESSES)Marshal.PtrToStructure(ptr, typeof(IP_ADAPTER_ADDRESSES));
        }
    }

    static void Main()
    {
        uint size = 0;
        GetAdaptersAddresses(2, 0, IntPtr.Zero, IntPtr.Zero, ref size);

        IntPtr addresses = Marshal.AllocHGlobal((int)size);
        GetAdaptersAddresses(2, 0, IntPtr.Zero, addresses, ref size);

        IP_ADAPTER_ADDRESSES adapter = new IP_ADAPTER_ADDRESSES(addresses);

        while (adapter.Next != IntPtr.Zero)
        {
            Console.WriteLine("Adapter Name: " + adapter.AdapterName);
            Console.WriteLine("Adapter Name: " + adapter.Address);
            Console.WriteLine("Adapter Name: " + adapter.Address);
            Console.WriteLine("Adapter IfIndex: " + adapter.IfIndex);

            adapter = new IP_ADAPTER_ADDRESSES(adapter.Next);
        }

        Marshal.FreeHGlobal(addresses);
    }
}
