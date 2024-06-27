using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Channels.Ipc;
using System.Runtime.Remoting.Channels;
using System.Text;
using System.Threading.Tasks;
using EasyHook;

namespace MGO3Offline2
{
    public class EntryPoint : IEntryPoint
    {
        /*[DllImport("winhttp.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        public static extern int WinHttpSendRequest(
            IntPtr hRequest,
            IntPtr lpszHeaders,
            int dwHeadersLength,
            IntPtr lpOptional,
            int dwOptionalLength,
            int dwTotalLength,
            int dwContext
            );
        
        public delegate int DWinHttpSendRequest(IntPtr hRequest, IntPtr lpszHeaders, int dwHeadersLength, IntPtr lpOptional, int dwOptionalLength, int dwTotalLength, int dwContext);
        public int MyWinHttpSendRequest(
            IntPtr hRequest,
            IntPtr lpszHeaders,
            int dwHeadersLength,
            IntPtr lpOptional,
            int dwOptionalLength,
            int dwTotalLength,
            int dwContext
            )
        {
            Console.WriteLine("WinHttpSendRequest called");
            return WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
        }

        public void InstallHook(String moduleName, String functionName, Delegate InCallback)
        {
            var hook = LocalHook.Create(
                LocalHook.GetProcAddress(moduleName, functionName),
                InCallback,
                this);

            hook.ThreadACL.SetExclusiveACL(new int[0]);
        }*/

        public EntryPoint(RemoteHooking.IContext context, string channelName)
        {

        }

        public void Run(RemoteHooking.IContext context, string channelName)
        {
            //InstallHook("winhttp.dll", "WinHttpSendRequest", new DWinHttpSendRequest(MyWinHttpSendRequest));
        }
    }
}
