[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if (-not ('HomesteadDisplayProbe' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class HomesteadDisplayProbe {
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct Mode {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string DeviceName;
        public ushort SpecVersion, DriverVersion, Size, DriverExtra;
        public uint Fields;
        public int PositionX, PositionY;
        public uint Orientation, FixedOutput;
        public short Color, Duplex, YResolution, TTOption, Collate;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string FormName;
        public ushort LogPixels;
        public uint BitsPerPixel, Width, Height, DisplayFlags, Frequency;
        public uint ICMMethod, ICMIntent, MediaType, DitherType, Reserved1, Reserved2, PanningWidth, PanningHeight;
    }
    [DllImport("user32.dll", EntryPoint="EnumDisplaySettingsW", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool EnumDisplaySettings(string name, int index, ref Mode mode);
    public static Mode Read() {
        var mode = new Mode();
        mode.Size = (ushort)Marshal.SizeOf(typeof(Mode));
        if (!EnumDisplaySettings(null, -1, ref mode))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        return mode;
    }
}
'@
}
$mode = [HomesteadDisplayProbe]::Read()
[ordered]@{
    observedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    primaryGdiMode = [ordered]@{
        width = $mode.Width; height = $mode.Height; refreshHz = $mode.Frequency
        bitsPerPixel = $mode.BitsPerPixel; displayFlags = $mode.DisplayFlags; orientation = $mode.Orientation
    }
    wmiAdapters = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion,
        CurrentHorizontalResolution, CurrentVerticalResolution, CurrentRefreshRate)
    limit = 'Read-only reported desktop modes, not measured panel scanout, VRR engagement or per-game DXGI Present events.'
}
