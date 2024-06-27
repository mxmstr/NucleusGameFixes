using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using EasyHook;
using PInvoke;

namespace MGO3OfflineLauncher
{
    internal class Program
    {
        [DllImport("kernel32.dll")]
        public static extern bool CreateProcess(string lpApplicationName, string lpCommandLine, IntPtr lpProcessAttributes, IntPtr lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags, IntPtr lpEnvironment, string lpCurrentDirectory, [In] ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll")]
        public static extern int ResumeThread(IntPtr hThread);

        [DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr hObject);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct STARTUPINFO
        {
            public Int32 cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public Int32 dwX;
            public Int32 dwY;
            public Int32 dwXSize;
            public Int32 dwYSize;
            public Int32 dwXCountChars;
            public Int32 dwYCountChars;
            public Int32 dwFillAttribute;
            public Int32 dwFlags;
            public Int16 wShowWindow;
            public Int16 cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        static void Main(string[] args)
        {
            const uint CREATE_SUSPENDED = 0x00000004;

            STARTUPINFO si = new STARTUPINFO();
            PROCESS_INFORMATION pi = new PROCESS_INFORMATION();

            bool success = CreateProcess(
                null, 
                @"mgsvmgo.exe /AppData 99c85cdbf2c837d50d37c82af2c837d5c12d5e80fbc837d5f2c837d5f2c837d5f2", 
                IntPtr.Zero, IntPtr.Zero, false, 0, IntPtr.Zero, null, ref si, out pi
                );

            if (!success)
            {
                throw new Exception("CreateProcess failed.  Error: " + Marshal.GetLastWin32Error());
            }
            
            try {
                // Inject the DLL into the process
                var injectionLibrary = "MGO3Offline2.dll";
                var injectionLibraryPath = System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(), injectionLibrary);
                RemoteHooking.Inject(pi.dwProcessId, InjectionOptions.Default, injectionLibraryPath, injectionLibraryPath, null);
            } catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }

            // Resume the thread
            if (ResumeThread(pi.hThread) == -1)
            {
                throw new Exception("ResumeThread failed.  Error: " + Marshal.GetLastWin32Error());
            }

            // Close the handle to the thread
            if (!CloseHandle(pi.hThread))
            {
                throw new Exception("CloseHandle failed.  Error: " + Marshal.GetLastWin32Error());
            }
        }
    }
}
