"""Packaged walk-off: teleport onto the road inside the gateway, walk out past the boundary and back,
recording frames. Args: exe w h outdir [wait] [settle].

Reference driver for packaged-game playtests (packaged tests are orchestrator-only during multi-lane
rounds). From the estate-boundary lane, verified on the bbab1de7 package. It shows the pieces that work:
find the game window by process image (never by size), open the console with backtick, type with
keybd_event, `Walk` after `BugItGo`, and record with ffmpeg ddagrab.

Caution: keybd_event types into whatever window has focus. It brings the game forward first
(front()), but check nothing else, especially Jenny's own apps, can take focus while it runs.
Extra console commands can be passed in WO_EXTRA, separated by ';'.
"""
import ctypes, subprocess, sys, time, os
u = ctypes.windll.user32; u.SetProcessDPIAware()
exe, w, h, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
wait = int(sys.argv[5]) if len(sys.argv) > 5 else 80
import imageio_ffmpeg  # pip install imageio-ffmpeg
ff = imageio_ffmpeg.get_ffmpeg_exe()
os.makedirs(out, exist_ok=True)
mode = "-fullscreen" if w >= 3840 else "-windowed"
p = subprocess.Popen([exe, "/Game/SurvivalGame/Maps/Estate", f"-ResX={w}", f"-ResY={h}", mode, "-ForceRes", "-nosplash", "-nosound"])
k = ctypes.windll.kernel32

def image(pid):
    hp = k.OpenProcess(0x1000, False, pid)
    if not hp: return ""
    buf = ctypes.create_unicode_buffer(1024); n = ctypes.c_ulong(1024)
    k.QueryFullProcessImageNameW(hp, 0, buf, ctypes.byref(n)); k.CloseHandle(hp); return buf.value

def window():
    found = []
    @ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def cb(hw, l):
        pid = ctypes.c_ulong(); u.GetWindowThreadProcessId(hw, ctypes.byref(pid))
        if u.IsWindowVisible(hw):
            r = (ctypes.c_long * 4)(); u.GetWindowRect(hw, r)
            if r[2] - r[0] >= w * 0.9: found.append((hw, pid.value))
        return True
    u.EnumWindows(cb, 0)
    return [f for f in found if "JennysHomesteadGame" in image(f[1])]

def front(hw):
    u.keybd_event(0x12, 0, 0, 0); u.SetForegroundWindow(hw); u.keybd_event(0x12, 0, 2, 0); time.sleep(0.4)

def down(vk): u.keybd_event(vk, u.MapVirtualKeyW(vk, 0), 0, 0)
def up(vk): u.keybd_event(vk, u.MapVirtualKeyW(vk, 0), 2, 0)
def key(vk, hold=0.08): down(vk); time.sleep(hold); up(vk); time.sleep(0.25)

def type_text(text):
    for ch in text:
        r = u.VkKeyScanW(ord(ch)); vk, shift = r & 0xFF, (r >> 8) & 1
        if shift: down(0x10)
        key(vk, 0.03)
        if shift: up(0x10)

def console(cmd):
    key(0xC0); time.sleep(0.6); type_text(cmd); time.sleep(0.2); key(0x0D); time.sleep(0.8)

def client(hw):
    pt = (ctypes.c_long * 2)(0, 0); u.ClientToScreen(hw, pt); return pt[0], pt[1]

def record(hw, name, seconds, fps=2):
    x, y = client(hw)
    return subprocess.Popen([ff, "-y", "-loglevel", "error", "-filter_complex",
        f"ddagrab=output_idx=0:framerate={fps}:offset_x={x}:offset_y={y}:video_size={w}x{h}:draw_mouse=0,hwdownload,format=bgra",
        "-t", str(seconds), os.path.join(out, name + "_%03d.png")])

deadline = time.time() + 240; hw = None
while time.time() < deadline:
    c = window()
    if c: hw = c[-1][0]; break
    time.sleep(2)
if not hw: print("no window"); p.kill(); sys.exit(1)
time.sleep(wait)
front(hw)
console("EnableCheats")
# Road index 203, ~15 m inside where the road leaves the estate; face out along the road (yaw 120).
console("BugItGo -5076 8156 5100 -12 120 0")
# BugItGo leaves the pawn in Ghost (flying, no collision); Walk restores walking.
console("Walk")
console("HomesteadMorning 9")
for extra in filter(None, os.environ.get('WO_EXTRA', '').split(';')): console(extra)
time.sleep(int(sys.argv[6]) if len(sys.argv) > 6 else 4)
front(hw)
rec = record(hw, "out", 22)
time.sleep(0.5)
down(0x57); time.sleep(20); up(0x57)
rec.wait()
front(hw)
rec = record(hw, "back", 22)
time.sleep(0.5)
down(0x53); time.sleep(20); up(0x53)
rec.wait()
print("ok", hw)
p.kill()

