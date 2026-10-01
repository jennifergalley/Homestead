// A process started suspended inside its own kill-on-close job object, so nothing it spawns (shader
// workers, a crash reporter) outlives the script that ran it. Used by Scripts\Inspect-Animation.ps1.
//
//     Add-Type -Path Scripts\KillOnCloseJob.cs
//     $job = [Homestead.Tools.KillOnCloseJob]::new($exe, $commandLine, $directory)
//     try { $exited = $job.WaitForExit(600000) } finally { $job.Stop(@('zenserver.exe')); $job.Release() }
//
// Stop() ends every process still in the job except the named images (a shared Zen server the editor may
// have launched serves other sessions too); Release() then disarms kill-on-close and closes the handles. If
// the owning script dies before that, closing the job's last handle ends everything in it.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Homestead.Tools
{
    public sealed class KillOnCloseJob : IDisposable
    {
        const uint KillOnJobClose = 0x2000;
        const uint CreateSuspended = 0x4;
        const int ExtendedLimitInformation = 9;
        const int BasicProcessIdList = 3;
        const uint ProcessTerminateAndQuery = 0x0001 | 0x1000;

        [StructLayout(LayoutKind.Sequential)]
        struct BasicLimits
        {
            public long PerProcessUserTimeLimit, PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize, MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass, SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct IoCounters { public ulong Reads, Writes, Others, ReadBytes, WriteBytes, OtherBytes; }

        [StructLayout(LayoutKind.Sequential)]
        struct ExtendedLimits
        {
            public BasicLimits Basic;
            public IoCounters Io;
            public UIntPtr ProcessMemoryLimit, JobMemoryLimit, PeakProcessMemoryUsed, PeakJobMemoryUsed;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct StartupInfo
        {
            public int Size;
            public string Reserved, Desktop, Title;
            public int X, Y, XSize, YSize, XCountChars, YCountChars, FillAttribute, Flags;
            public short ShowWindow, Reserved2Size;
            public IntPtr Reserved2, StdInput, StdOutput, StdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct ProcessInformation { public IntPtr Process, Thread; public int ProcessId, ThreadId; }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr CreateJobObjectW(IntPtr attributes, string name);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool SetInformationJobObject(IntPtr job, int infoClass, ref ExtendedLimits info, uint length);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryInformationJobObject(IntPtr job, int infoClass, IntPtr info, uint length, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool CreateProcessW(string application, StringBuilder commandLine, IntPtr processAttributes,
            IntPtr threadAttributes, bool inheritHandles, uint flags, IntPtr environment, string directory,
            ref StartupInfo startup, out ProcessInformation information);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint ResumeThread(IntPtr thread);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool TerminateProcess(IntPtr process, uint code);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GetExitCodeProcess(IntPtr process, out uint code);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint access, bool inherit, uint processId);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool QueryFullProcessImageNameW(IntPtr process, uint flags, StringBuilder name, ref uint size);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr handle);

        IntPtr job, process;

        public int ProcessId { get; private set; }

        public KillOnCloseJob(string executable, string commandLine, string directory)
        {
            job = CreateJobObjectW(IntPtr.Zero, null);
            if (job == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateJobObject");
            try
            {
                SetLimitFlags(KillOnJobClose);
                var startup = new StartupInfo { Size = Marshal.SizeOf<StartupInfo>() };
                ProcessInformation created;
                if (!CreateProcessW(executable, new StringBuilder(commandLine), IntPtr.Zero, IntPtr.Zero, false,
                        CreateSuspended, IntPtr.Zero, directory, ref startup, out created))
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateProcess " + executable);
                process = created.Process;
                ProcessId = created.ProcessId;
                try
                {
                    // In the job before its first instruction, so every child it starts is in the job too.
                    if (!AssignProcessToJobObject(job, process))
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "AssignProcessToJobObject");
                    if (ResumeThread(created.Thread) == uint.MaxValue)
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "ResumeThread");
                }
                catch
                {
                    TerminateProcess(process, 1);
                    throw;
                }
                finally
                {
                    CloseHandle(created.Thread);
                }
            }
            catch
            {
                Dispose();
                throw;
            }
        }

        public bool WaitForExit(int milliseconds)
        {
            return WaitForSingleObject(process, (uint)Math.Max(0, milliseconds)) == 0;
        }

        public int ExitCode
        {
            get
            {
                uint code;
                return GetExitCodeProcess(process, out code) ? unchecked((int)code) : -1;
            }
        }

        /// <summary>Ends every process still in the job except those whose image file name is in
        /// <paramref name="spare"/>, waiting up to 5 s for each; returns the image names it ended.</summary>
        public string[] Stop(string[] spare)
        {
            var spared = new HashSet<string>(spare ?? new string[0], StringComparer.OrdinalIgnoreCase);
            var ended = new List<string>();
            foreach (uint id in Members())
            {
                IntPtr member = OpenProcess(ProcessTerminateAndQuery | 0x00100000, false, id);
                if (member == IntPtr.Zero) continue;
                try
                {
                    var name = new StringBuilder(32768);
                    uint size = (uint)name.Capacity;
                    string image = QueryFullProcessImageNameW(member, 0, name, ref size) ? Path.GetFileName(name.ToString()) : "?";
                    if (spared.Contains(image) || WaitForSingleObject(member, 0) == 0) continue;
                    if (TerminateProcess(member, 1))
                    {
                        WaitForSingleObject(member, 5000);
                        ended.Add(image);
                    }
                }
                finally
                {
                    CloseHandle(member);
                }
            }
            return ended.ToArray();
        }

        /// <summary>Disarms kill-on-close (so a spared process lives on) and closes the handles.</summary>
        public void Release()
        {
            if (job != IntPtr.Zero) SetLimitFlags(0);
            Dispose();
        }

        public void Dispose()
        {
            if (process != IntPtr.Zero) { CloseHandle(process); process = IntPtr.Zero; }
            if (job != IntPtr.Zero) { CloseHandle(job); job = IntPtr.Zero; }
        }

        void SetLimitFlags(uint flags)
        {
            var limits = new ExtendedLimits();
            limits.Basic.LimitFlags = flags;
            if (!SetInformationJobObject(job, ExtendedLimitInformation, ref limits, (uint)Marshal.SizeOf<ExtendedLimits>()))
                throw new Win32Exception(Marshal.GetLastWin32Error(), "SetInformationJobObject");
        }

        uint[] Members()
        {
            // JOBOBJECT_BASIC_PROCESS_ID_LIST: two counts, then the ids (ULONG_PTR each).
            const int Capacity = 1024;
            int size = 8 + IntPtr.Size * Capacity;
            IntPtr buffer = Marshal.AllocHGlobal(size);
            try
            {
                if (!QueryInformationJobObject(job, BasicProcessIdList, buffer, (uint)size, IntPtr.Zero))
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "QueryInformationJobObject");
                int count = Marshal.ReadInt32(buffer, 4);
                var ids = new uint[count];
                for (int i = 0; i < count; i++)
                    ids[i] = (uint)Marshal.ReadIntPtr(buffer, 8 + i * IntPtr.Size).ToInt64();
                return ids;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }
    }
}
