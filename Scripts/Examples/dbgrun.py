r"""Minimal Win32 debugger for a packaged game that dies before it writes a log.

Usage: python Scripts\Examples\dbgrun.py "<full command line of the exe and its arguments>"

Starts the process with DEBUG_ONLY_THIS_PROCESS and prints OutputDebugString text ("ODS:"), each
exception with its code and a dbghelp-symbolised address, a short symbolised stack scan, and the exit
code. There's no cdb/WinDbg on this machine; this found a CrashDuringStaticInit (exit code 777006) in
one run. Symbols come from the .pdb files next to the exe (Development builds ship them).
From the integration session, 2026-09-28.
"""
import ctypes, sys
from ctypes import wintypes as W

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
dbg = ctypes.WinDLL("dbghelp", use_last_error=True)

DEBUG_ONLY_THIS_PROCESS = 0x2
EXCEPTION_DEBUG_EVENT, CREATE_PROCESS_DEBUG_EVENT, EXIT_PROCESS_DEBUG_EVENT, LOAD_DLL_DEBUG_EVENT, OUTPUT_DEBUG_STRING_EVENT = 1, 3, 5, 6, 8
DBG_CONTINUE, DBG_EXCEPTION_NOT_HANDLED = 0x00010002, 0x80010001


class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", W.DWORD), ("lpReserved", W.LPWSTR), ("lpDesktop", W.LPWSTR), ("lpTitle", W.LPWSTR),
                ("dwX", W.DWORD), ("dwY", W.DWORD), ("dwXSize", W.DWORD), ("dwYSize", W.DWORD),
                ("dwXCountChars", W.DWORD), ("dwYCountChars", W.DWORD), ("dwFillAttribute", W.DWORD),
                ("dwFlags", W.DWORD), ("wShowWindow", W.WORD), ("cbReserved2", W.WORD),
                ("lpReserved2", ctypes.c_void_p), ("hStdInput", W.HANDLE), ("hStdOutput", W.HANDLE), ("hStdError", W.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", W.HANDLE), ("hThread", W.HANDLE), ("dwProcessId", W.DWORD), ("dwThreadId", W.DWORD)]


class EXCEPTION_RECORD(ctypes.Structure):
    pass


EXCEPTION_RECORD._fields_ = [("ExceptionCode", W.DWORD), ("ExceptionFlags", W.DWORD),
                             ("ExceptionRecord", ctypes.c_void_p), ("ExceptionAddress", ctypes.c_void_p),
                             ("NumberParameters", W.DWORD), ("ExceptionInformation", ctypes.c_uint64 * 15)]


class EXCEPTION_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("ExceptionRecord", EXCEPTION_RECORD), ("dwFirstChance", W.DWORD)]


class OUTPUT_DEBUG_STRING_INFO(ctypes.Structure):
    _fields_ = [("lpDebugStringData", ctypes.c_void_p), ("fUnicode", W.WORD), ("nDebugStringLength", W.WORD)]


class U(ctypes.Union):
    _fields_ = [("Exception", EXCEPTION_DEBUG_INFO), ("DebugString", OUTPUT_DEBUG_STRING_INFO), ("pad", ctypes.c_byte * 160)]


class DEBUG_EVENT(ctypes.Structure):
    _fields_ = [("dwDebugEventCode", W.DWORD), ("dwProcessId", W.DWORD), ("dwThreadId", W.DWORD), ("u", U)]


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [("SizeOfStruct", W.ULONG), ("TypeIndex", W.ULONG), ("Reserved", ctypes.c_uint64 * 2), ("Index", W.ULONG),
                ("Size", W.ULONG), ("ModBase", ctypes.c_uint64), ("Flags", W.ULONG), ("Value", ctypes.c_uint64),
                ("Address", ctypes.c_uint64), ("Register", W.ULONG), ("Scope", W.ULONG), ("Tag", W.ULONG),
                ("NameLen", W.ULONG), ("MaxNameLen", W.ULONG), ("Name", ctypes.c_char * 1024)]


class IMAGEHLP_LINE64(ctypes.Structure):
    _fields_ = [("SizeOfStruct", W.DWORD), ("Key", ctypes.c_void_p), ("LineNumber", W.DWORD), ("FileName", ctypes.c_char_p), ("Address", ctypes.c_uint64)]


class CONTEXT64(ctypes.Structure):
    _pack_ = 16
    _fields_ = [("P", ctypes.c_uint64 * 6), ("ContextFlags", W.DWORD), ("MxCsr", W.DWORD), ("Seg", W.WORD * 6), ("EFlags", W.DWORD),
                ("Dr", ctypes.c_uint64 * 6), ("Rax", ctypes.c_uint64), ("Rcx", ctypes.c_uint64), ("Rdx", ctypes.c_uint64),
                ("Rbx", ctypes.c_uint64), ("Rsp", ctypes.c_uint64), ("Rbp", ctypes.c_uint64), ("Rsi", ctypes.c_uint64),
                ("Rdi", ctypes.c_uint64), ("R", ctypes.c_uint64 * 8), ("Rip", ctypes.c_uint64), ("rest", ctypes.c_byte * 976)]


def read(h, addr, n):
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t()
    k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, n, ctypes.byref(got))
    return buf.raw[:got.value]


def sym(h, addr):
    s = SYMBOL_INFO(); s.SizeOfStruct = 88; s.MaxNameLen = 1000
    disp = ctypes.c_uint64()
    name = "?"
    if dbg.SymFromAddr(h, ctypes.c_uint64(addr), ctypes.byref(disp), ctypes.byref(s)):
        name = f"{s.Name.decode(errors='replace')}+0x{disp.value:x}"
    ln = IMAGEHLP_LINE64(); ln.SizeOfStruct = ctypes.sizeof(IMAGEHLP_LINE64)
    d32 = W.DWORD()
    if dbg.SymGetLineFromAddr64(h, ctypes.c_uint64(addr), ctypes.byref(d32), ctypes.byref(ln)):
        name += f"  {ln.FileName.decode(errors='replace')}:{ln.LineNumber}"
    return name


def main():
    cmd = sys.argv[1]
    si = STARTUPINFO(); si.cb = ctypes.sizeof(si)
    pi = PROCESS_INFORMATION()
    if not k32.CreateProcessW(None, ctypes.c_wchar_p(cmd), None, None, False, DEBUG_ONLY_THIS_PROCESS, None, None, ctypes.byref(si), ctypes.byref(pi)):
        print("CreateProcess failed", ctypes.get_last_error()); return
    hproc = pi.hProcess
    dbg.SymSetOptions(0x2 | 0x10 | 0x4)  # UNDNAME | LOAD_LINES | DEFERRED_LOADS
    ev = DEBUG_EVENT()
    reported = 0
    inited = False
    while True:
        if not k32.WaitForDebugEvent(ctypes.byref(ev), 120000):
            print("timeout"); k32.TerminateProcess(hproc, 1); break
        cont = DBG_CONTINUE
        code = ev.dwDebugEventCode
        if code == OUTPUT_DEBUG_STRING_EVENT:
            info = ev.u.DebugString
            n = info.nDebugStringLength
            raw = read(hproc, info.lpDebugStringData, n * (2 if info.fUnicode else 1))
            txt = raw.decode("utf-16-le" if info.fUnicode else "mbcs", errors="replace").rstrip("\x00\r\n")
            print("ODS:", txt[:800])
        elif code == EXCEPTION_DEBUG_EVENT:
            rec = ev.u.Exception.ExceptionRecord
            first = ev.u.Exception.dwFirstChance
            exc = rec.ExceptionCode
            if exc in (0x80000003, 0x4000001F) and reported == 0 and not inited:
                inited = True  # initial loader breakpoint
            else:
                if not inited or True:
                    if not getattr(main, "syminit", False):
                        dbg.SymInitialize(hproc, None, True); main.syminit = True
                    else:
                        dbg.SymRefreshModuleList(hproc)
                addr = rec.ExceptionAddress or 0
                if exc not in (0x406D1388, 0x40010006, 0x4001000A) and reported < 6:
                    reported += 1
                    print(f"EXC 0x{exc:08x} first={first} at 0x{addr:x} {sym(hproc, addr)}")
                    # stack walk via RtlVirtualUnwind is complex; dump stack words that symbolize into the exe.
                    hthr = k32.OpenThread(0x1FFFFF, False, ev.dwThreadId)
                    ctx = CONTEXT64(); ctx.ContextFlags = 0x10001F
                    if k32.GetThreadContext(hthr, ctypes.byref(ctx)):
                        data = read(hproc, ctx.Rsp, 8 * 400)
                        shown = 0
                        for i in range(0, len(data), 8):
                            v = int.from_bytes(data[i:i + 8], "little")
                            s = sym(hproc, v)
                            if s != "?" and shown < 40:
                                print("   stk", s); shown += 1
                    k32.CloseHandle(hthr)
                cont = DBG_EXCEPTION_NOT_HANDLED
        elif code == EXIT_PROCESS_DEBUG_EVENT:
            print("exit"); k32.ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE); break
        k32.ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, cont)
    code = W.DWORD(); k32.GetExitCodeProcess(hproc, ctypes.byref(code)); print("exitcode", code.value)


main()

