<#
.SYNOPSIS
Real keyboard input and DPI-correct captures for a standalone game window (not PIE). Dot-source it.
.DESCRIPTION
For testing UI at real resolutions and DPI, which PIE at editor size hides:

    & 'E:\Program Files\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' "$PWD\SurvivalGame.uproject" `
        /Game/SurvivalGame/Maps/Estate -game -windowed -ResX=3840 -ResY=2160 -log=ui-4k.log
    . .\Scripts\GameWindow.ps1
    $h = Find-GameWindow -ProcessId <pid>          # waits for its main window
    # wait until Saved\Logs\ui-4k.log has "MUSIC_TRACK started", then about 20 s more
    [GameWin]::Key($h, 0x0D)                        # VK_RETURN; PostMessage WM_KEYDOWN/UP
    'Eleanor'.ToCharArray() | ForEach-Object { [GameWin]::Char($h, $_) }   # WM_CHAR, types into text boxes
    [GameWin]::Capture($h, "$PWD\Saved\ui-4k-names.png")                   # PrintWindow, DPI-aware

PostMessage reaches the game where SetForegroundWindow/SendInput don't (the Copilot app keeps focus),
and the standalone game counts toward the machine's 2-Unreal-process limit. Close it by PID when done.
Contributed by the ruined-manor lane.
#>
if (-not ('GameWin' -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing,System.Drawing.Common,System.Drawing.Primitives,System.Threading.Thread,System.Runtime.InteropServices -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class GameWin {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    static IntPtr KeyL(uint vk, bool up) {
        uint scan = MapVirtualKey(vk, 0);
        long l = 1 | (scan << 16);
        if (up) l |= (1L << 30) | (1L << 31);
        return (IntPtr)l;
    }
    public static void Key(IntPtr h, uint vk) {
        PostMessage(h, 0x0100, (IntPtr)vk, KeyL(vk, false));
        System.Threading.Thread.Sleep(60);
        PostMessage(h, 0x0101, (IntPtr)vk, KeyL(vk, true));
        System.Threading.Thread.Sleep(60);
    }
    public static void Char(IntPtr h, char c) {
        PostMessage(h, 0x0102, (IntPtr)c, (IntPtr)1);
        System.Threading.Thread.Sleep(40);
    }
    public static void Capture(IntPtr h, string path) {
        RECT r; GetClientRect(h, out r);
        using (var bmp = new Bitmap(Math.Max(1, r.R - r.L), Math.Max(1, r.B - r.T))) {
            using (var g = Graphics.FromImage(bmp)) { IntPtr dc = g.GetHdc(); PrintWindow(h, dc, 3); g.ReleaseHdc(dc); }
            bmp.Save(path, ImageFormat.Png);
        }
    }
}
"@
}
[void][GameWin]::SetProcessDPIAware()

function Find-GameWindow([Parameter(Mandatory)][int]$ProcessId, [int]$TimeoutSeconds = 300) {
    # Match by process, never by window size: other sessions' maximised editors are also ~3840 wide.
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $p = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if (-not $p) { throw "Process $ProcessId exited." }
        if ($p.MainWindowHandle -ne [IntPtr]::Zero) { return $p.MainWindowHandle }
        Start-Sleep -Milliseconds 500
    }
    throw "Process $ProcessId has no main window after $TimeoutSeconds s."
}
