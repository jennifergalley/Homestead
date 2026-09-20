using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace Homestead.Authoring
{
    public sealed class MarkerEvidence
    {
        public string Path;
        public uint Volume, IndexHigh, IndexLow, Attributes;
        public ulong Size, CreationTime, LastWriteTime;
        public string Sha256, SecuritySha256;
    }
    public sealed class JobEvidence
    {
        public uint Flags, ProcessLimit, ActiveProcesses, TotalProcesses, TerminatedForLimits;
        public bool HeldProcessIsMember;
    }

    // Native process primitive only. Approval, socket policy and run controls belong to its caller.
    public sealed class LeafGuard : IDisposable
    {
        const uint Read = 0x80000000, Write = 0x40000000, ShareRead = 1;
        const uint Inherit = 1, Suspended = 4, ExtendedStartup = 0x80000, UnicodeEnvironment = 0x400;
        const uint Limits = 0x2008, ObjectSignaled = 0, Timeout = 258;
        static readonly IntPtr Invalid = new IntPtr(-1);
        IntPtr marker, job, process, thread, input, output;
        public uint ProcessId { get; private set; }
        public ulong ProcessCreationTime { get; private set; }
        public string ImagePath { get; private set; }
        public MarkerEvidence MarkerBefore { get; private set; }
        public JobEvidence LastVerifiedJob { get; private set; }
        public long MarkerHandle { get { return marker.ToInt64(); } }
        public long JobHandle { get { return job.ToInt64(); } }
        public bool Resumed { get; private set; }
        public bool HardTerminated { get; private set; }
        public uint WhitelistedHandleCount { get { return 3; } }

        [StructLayout(LayoutKind.Sequential)] struct Time { public uint Low, High; public ulong Value { get { return ((ulong)High << 32) | Low; } } }
        [StructLayout(LayoutKind.Sequential)] struct FileInfo
        {
            public uint Attributes; public Time Creation, Access, Write;
            public uint Volume, SizeHigh, SizeLow, Links, IndexHigh, IndexLow;
        }
        [StructLayout(LayoutKind.Sequential)] struct BasicLimits
        {
            public long ProcessTime, JobTime; public uint Flags; public UIntPtr MinWorking, MaxWorking;
            public uint Active; public UIntPtr Affinity; public uint Priority, Scheduling;
        }
        [StructLayout(LayoutKind.Sequential)] struct IoCounters { public ulong A, B, C, D, E, F; }
        [StructLayout(LayoutKind.Sequential)] struct ExtendedLimits
        {
            public BasicLimits Basic; public IoCounters Io;
            public UIntPtr ProcessMemory, JobMemory, PeakProcessMemory, PeakJobMemory;
        }
        [StructLayout(LayoutKind.Sequential)] struct Accounting
        {
            public long UserTime, KernelTime, PeriodUser, PeriodKernel;
            public uint Faults, Total, Active, Terminated;
        }
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)] struct Startup
        {
            public uint Size; public IntPtr Reserved, Desktop, Title;
            public uint X, Y, XSize, YSize, XCount, YCount, Fill, Flags;
            public ushort Show, ReservedSize; public IntPtr ReservedBytes, Input, Output, Error;
        }
        [StructLayout(LayoutKind.Sequential)] struct StartupEx { public Startup Startup; public IntPtr Attributes; }
        [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr Process, Thread; public uint Id, ThreadId; }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr CreateFileW(string path, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool SetHandleInformation(IntPtr handle, uint mask, uint flags);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetHandleInformation(IntPtr handle, out uint flags);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetFileInformationByHandle(IntPtr file, out FileInfo information);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool ReadFile(IntPtr file, [Out] byte[] buffer, uint count, out uint read, IntPtr overlapped);
        [DllImport("advapi32.dll", SetLastError = true)] static extern bool GetKernelObjectSecurity(IntPtr handle, uint requested, [Out] byte[] security, uint length, out uint needed);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern IntPtr CreateJobObjectW(IntPtr security, string name);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool SetInformationJobObject(IntPtr job, int kind, ref ExtendedLimits limits, uint size);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool QueryInformationJobObject(IntPtr job, int kind, out ExtendedLimits limits, uint size, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool QueryInformationJobObject(IntPtr job, int kind, out Accounting accounting, uint size, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool IsProcessInJob(IntPtr process, IntPtr job, out bool member);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool InitializeProcThreadAttributeList(IntPtr list, int count, uint flags, ref IntPtr size);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool UpdateProcThreadAttribute(IntPtr list, uint flags, IntPtr attribute, IntPtr value, IntPtr size, IntPtr previous, IntPtr returned);
        [DllImport("kernel32.dll")] static extern void DeleteProcThreadAttributeList(IntPtr list);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern bool CreateProcessW(string application, StringBuilder command, IntPtr processSecurity, IntPtr threadSecurity, bool inherit, uint flags, IntPtr environment, string directory, ref StartupEx startup, out ProcessInfo result);
        [DllImport("kernel32.dll", SetLastError = true)] static extern uint ResumeThread(IntPtr thread);
        [DllImport("kernel32.dll", SetLastError = true)] static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetExitCodeProcess(IntPtr process, out uint code);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetProcessTimes(IntPtr process, out Time created, out Time exited, out Time kernel, out Time user);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)] static extern bool QueryFullProcessImageNameW(IntPtr process, uint flags, StringBuilder path, ref uint size);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool TerminateJobObject(IntPtr job, uint code);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool TerminateProcess(IntPtr process, uint code);

        static void Check(bool okay, string operation) { if (!okay) throw new Win32Exception(Marshal.GetLastWin32Error(), operation); }
        static bool Valid(IntPtr handle) { return handle != IntPtr.Zero && handle != Invalid; }
        static void Close(ref IntPtr handle) { if (Valid(handle)) { Check(CloseHandle(handle), "CloseHandle"); handle = IntPtr.Zero; } }
        static string Hash(byte[] value) { using (var sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(value)).Replace("-", ""); }
        static string FileHash(string path) { using (var file = File.OpenRead(path)) using (var sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(file)).Replace("-", ""); }
        public static string Quote(string value)
        {
            if (value == null || value.IndexOf('\0') >= 0) throw new ArgumentException("Invalid process argument.");
            var result = new StringBuilder("\""); int slashes = 0;
            foreach (char c in value)
            {
                if (c == '\\') { slashes++; continue; }
                result.Append('\\', c == '"' ? slashes * 2 + 1 : slashes);
                result.Append(c); slashes = 0;
            }
            result.Append('\\', slashes * 2); return result.Append('"').ToString();
        }
        static void OrdinaryPath(string path)
        {
            if (!System.IO.Path.IsPathFullyQualified(path) || path.StartsWith(@"\\"))
                throw new InvalidOperationException("Local absolute path required.");
            for (string part = path; !string.IsNullOrEmpty(part); part = System.IO.Path.GetDirectoryName(part))
                if ((File.GetAttributes(part) & FileAttributes.ReparsePoint) != 0)
                    throw new InvalidOperationException("Reparse path prohibited: " + part);
        }
        static MarkerEvidence Snapshot(IntPtr handle, string path)
        {
            FileInfo info; Check(GetFileInformationByHandle(handle, out info), "marker identity");
            if ((info.Attributes & (0x10u | 0x400u)) != 0 || info.SizeHigh != 0 || info.SizeLow != 0)
                throw new InvalidOperationException("Marker must be an existing empty ordinary file.");
            byte[] probe = new byte[1]; uint read;
            Check(ReadFile(handle, probe, 1, out read, IntPtr.Zero), "read empty marker");
            if (read != 0) throw new InvalidOperationException("Marker not empty.");
            uint needed;
            GetKernelObjectSecurity(handle, 7, null, 0, out needed);
            if (needed == 0 || needed > 65536) throw new InvalidOperationException("Invalid marker security size.");
            byte[] security = new byte[needed];
            Check(GetKernelObjectSecurity(handle, 7, security, needed, out needed), "read marker owner/group/DACL");
            return new MarkerEvidence { Path = path, Volume = info.Volume, IndexHigh = info.IndexHigh, IndexLow = info.IndexLow,
                Attributes = info.Attributes, Size = 0, CreationTime = info.Creation.Value, LastWriteTime = info.Write.Value,
                Sha256 = Hash(Array.Empty<byte>()), SecuritySha256 = Hash(security) };
        }
        public static MarkerEvidence InspectMarker(string path)
        {
            OrdinaryPath(path);
            IntPtr handle = CreateFileW(path, Read, ShareRead, IntPtr.Zero, 3, 0x80, IntPtr.Zero);
            Check(Valid(handle), "inspect existing read-only marker");
            try { return Snapshot(handle, path); }
            finally { Close(ref handle); }
        }
        public MarkerEvidence VerifyMarker()
        {
            var current = Snapshot(marker, MarkerBefore.Path);
            if (current.Volume != MarkerBefore.Volume || current.IndexHigh != MarkerBefore.IndexHigh ||
                current.IndexLow != MarkerBefore.IndexLow || current.Attributes != MarkerBefore.Attributes ||
                current.CreationTime != MarkerBefore.CreationTime || current.LastWriteTime != MarkerBefore.LastWriteTime ||
                current.Sha256 != MarkerBefore.Sha256 || current.SecuritySha256 != MarkerBefore.SecuritySha256)
                throw new InvalidOperationException("Marker changed.");
            return current;
        }
        public LeafGuard(string executable, string expectedSha256, string[] arguments, string directory,
            string markerPath, string stdoutPath, IDictionary<string, string> overrides)
        {
            IntPtr attributes = IntPtr.Zero, handles = IntPtr.Zero, environment = IntPtr.Zero;
            bool initializedAttributes = false;
            try
            {
                executable = System.IO.Path.GetFullPath(executable);
                OrdinaryPath(executable); OrdinaryPath(markerPath); OrdinaryPath(directory);
                OrdinaryPath(System.IO.Path.GetDirectoryName(System.IO.Path.GetFullPath(stdoutPath)));
                if (FileHash(executable) != expectedSha256) throw new InvalidOperationException("Executable hash differs.");
                marker = CreateFileW(markerPath, Read, ShareRead, IntPtr.Zero, 3, 0x80, IntPtr.Zero);
                Check(Valid(marker), "acquire existing read-only marker guard");
                MarkerBefore = Snapshot(marker, markerPath);
                input = CreateFileW("NUL", Read, 3, IntPtr.Zero, 3, 0x80, IntPtr.Zero);
                output = CreateFileW(stdoutPath, Write, ShareRead, IntPtr.Zero, 1, 0x80, IntPtr.Zero);
                Check(Valid(input) && Valid(output), "isolated standard handles");
                job = CreateJobObjectW(IntPtr.Zero, null); Check(Valid(job), "unnamed job");
                var limits = new ExtendedLimits(); limits.Basic.Flags = Limits; limits.Basic.Active = 1;
                Check(SetInformationJobObject(job, 9, ref limits, (uint)Marshal.SizeOf<ExtendedLimits>()), "job limits");
                uint jobFlags; Check(GetHandleInformation(job, out jobFlags) && (jobFlags & Inherit) == 0, "noninherited job");
                foreach (var handle in new[] { marker, input, output })
                    Check(SetHandleInformation(handle, Inherit, Inherit), "explicit inheritable handle");
                IntPtr size = IntPtr.Zero; InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref size);
                if (size == IntPtr.Zero) throw new InvalidOperationException("Attribute-list sizing failed.");
                attributes = Marshal.AllocHGlobal(size);
                Check(InitializeProcThreadAttributeList(attributes, 1, 0, ref size), "attribute list");
                initializedAttributes = true;
                handles = Marshal.AllocHGlobal(IntPtr.Size * 3);
                Marshal.Copy(new[] { marker, input, output }, 0, handles, 3);
                Check(UpdateProcThreadAttribute(attributes, 0, new IntPtr(0x20002), handles, new IntPtr(IntPtr.Size * 3),
                    IntPtr.Zero, IntPtr.Zero), "exact handle whitelist");
                var variables = new SortedDictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                foreach (System.Collections.DictionaryEntry entry in Environment.GetEnvironmentVariables())
                    variables[(string)entry.Key] = (string)entry.Value;
                foreach (var entry in overrides)
                {
                    if (String.IsNullOrEmpty(entry.Key) || entry.Key.IndexOfAny(new[] { '\0', '=' }) >= 0 ||
                        (entry.Value != null && entry.Value.IndexOf('\0') >= 0))
                        throw new InvalidOperationException("Invalid environment override.");
                    if (entry.Value == null) variables.Remove(entry.Key); else variables[entry.Key] = entry.Value;
                }
                variables["HOMESTEAD_GUARD_HANDLE"] = MarkerHandle.ToString(System.Globalization.CultureInfo.InvariantCulture);
                variables["HOMESTEAD_GUARD_JOB_HANDLE"] = JobHandle.ToString(System.Globalization.CultureInfo.InvariantCulture);
                string block = string.Join("\0", variables.Select(v => v.Key + "=" + v.Value)) + "\0\0";
                environment = Marshal.StringToHGlobalUni(block);
                var startup = new StartupEx(); startup.Startup.Size = (uint)Marshal.SizeOf<StartupEx>();
                startup.Startup.Flags = 0x100; startup.Startup.Input = input;
                startup.Startup.Output = output; startup.Startup.Error = output; startup.Attributes = attributes;
                var command = new StringBuilder(Quote(executable) + " " + string.Join(" ", arguments.Select(Quote)));
                ulong earliest = (ulong)DateTime.UtcNow.AddSeconds(-1).ToFileTimeUtc();
                ProcessInfo created;
                Check(CreateProcessW(executable, command, IntPtr.Zero, IntPtr.Zero, true,
                    Suspended | ExtendedStartup | UnicodeEnvironment, environment, directory, ref startup, out created), "suspended leaf");
                process = created.Process; thread = created.Thread; ProcessId = created.Id;
                Check(AssignProcessToJobObject(job, process), "pre-resume job assignment");
                var path = new StringBuilder(32768); uint length = (uint)path.Capacity;
                Check(QueryFullProcessImageNameW(process, 0, path, ref length), "actual process image");
                ImagePath = path.ToString();
                Time start, exit, kernel, user;
                Check(GetProcessTimes(process, out start, out exit, out kernel, out user), "actual process start");
                ProcessCreationTime = start.Value;
                if (!String.Equals(ImagePath, executable, StringComparison.OrdinalIgnoreCase) ||
                    start.Value < earliest || start.Value > (ulong)DateTime.UtcNow.ToFileTimeUtc() ||
                    FileHash(ImagePath) != expectedSha256) throw new InvalidOperationException("Created identity differs.");
                VerifyJob(1); VerifyMarker();
            }
            catch (Exception original)
            {
                try
                {
                    if (Valid(process) && !Wait(0))
                    {
                        Check(TerminateProcess(process, 90), "abort owned suspended leaf");
                        Check(WaitForSingleObject(process, 5000) == ObjectSignaled, "aborted leaf exit");
                    }
                    Dispose();
                }
                catch (Exception cleanup) { throw new AggregateException("Guard creation and owned cleanup failed.", original, cleanup); }
                throw;
            }
            finally
            {
                if (initializedAttributes) DeleteProcThreadAttributeList(attributes);
                if (attributes != IntPtr.Zero) Marshal.FreeHGlobal(attributes);
                if (handles != IntPtr.Zero) Marshal.FreeHGlobal(handles);
                if (environment != IntPtr.Zero) Marshal.FreeHGlobal(environment);
            }
        }
        public void VerifyJob(uint active)
        {
            ExtendedLimits limits; Accounting counts; bool member;
            Check(IsProcessInJob(process, job, out member) && member, "held process job identity");
            Check(QueryInformationJobObject(job, 9, out limits, (uint)Marshal.SizeOf<ExtendedLimits>(), IntPtr.Zero), "job policy");
            Check(QueryInformationJobObject(job, 1, out counts, (uint)Marshal.SizeOf<Accounting>(), IntPtr.Zero), "job accounting");
            if (limits.Basic.Flags != Limits || limits.Basic.Active != 1 || counts.Active != active)
                throw new InvalidOperationException("Job policy/count differs.");
            LastVerifiedJob = new JobEvidence { Flags = limits.Basic.Flags, ProcessLimit = limits.Basic.Active,
                ActiveProcesses = counts.Active, TotalProcesses = counts.Total, TerminatedForLimits = counts.Terminated,
                HeldProcessIsMember = member };
        }
        public void Resume()
        {
            if (Resumed) throw new InvalidOperationException("Already resumed.");
            VerifyJob(1); VerifyMarker();
            Check(ResumeThread(thread) == 1, "first thread resume");
            Resumed = true;
        }
        public bool Wait(uint milliseconds)
        {
            uint result = WaitForSingleObject(process, milliseconds);
            if (result == ObjectSignaled) return true;
            if (result == Timeout) return false;
            throw new Win32Exception(Marshal.GetLastWin32Error(), "held process wait");
        }
        public uint ExitCode
        {
            get { if (!Wait(0)) throw new InvalidOperationException("Process still active."); uint code; Check(GetExitCodeProcess(process, out code), "exit code"); return code; }
        }
        public void HardStop(uint code)
        {
            if (Wait(0)) return;
            Check(TerminateJobObject(job, code), "owned-job hard termination");
            HardTerminated = true;
            Check(Wait(5000), "hard-terminated process exit");
        }
        public void Dispose()
        {
            if (Valid(process) && !Wait(0)) throw new InvalidOperationException("Must verify process death before releasing guard.");
            Close(ref thread); Close(ref process); Close(ref job);
            Close(ref input); Close(ref output); Close(ref marker);
        }
    }
}
