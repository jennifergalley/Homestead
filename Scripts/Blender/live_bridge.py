"""Live bridge: lets scripts drive a visible (GUI) Blender session.

Start with Scripts\\Blender\\Start-BlenderLive.ps1, which launches Blender with
``--python live_bridge.py``. The bridge listens on 127.0.0.1 only and requires
the per-session token from HOMESTEAD_BLENDER_TOKEN. Each request is one JSON
line: {"token", "code" | "file", "args"}; the code runs on Blender's main
thread with a 3D-viewport context and the reply is {"ok", "output", "error"}.
"""
import contextlib
import io
import json
import os
import queue
import socket
import sys
import threading
import traceback

import bpy

HOST = "127.0.0.1"
PORT = int(os.environ.get("HOMESTEAD_BLENDER_PORT", "9876"))
TOKEN = os.environ.get("HOMESTEAD_BLENDER_TOKEN", "")
REQUESTS = queue.Queue()


def _view3d_override():
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            if area.type == "VIEW_3D":
                region = next(r for r in area.regions if r.type == "WINDOW")
                return {"window": window, "screen": window.screen, "area": area, "region": region}
    return {}


def _run(request):
    output = io.StringIO()
    saved_argv = sys.argv
    try:
        path = request.get("file")
        code = open(path, encoding="utf-8").read() if path else request["code"]
        sys.argv = ["blender", "--"] + list(request.get("args", []))
        scope = {"__name__": "__main__", "__file__": path or "<live>"}
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            with bpy.context.temp_override(**_view3d_override()):
                exec(compile(code, path or "<live>", "exec"), scope)
        return {"ok": True, "output": output.getvalue()}
    except BaseException:
        return {"ok": False, "output": output.getvalue(), "error": traceback.format_exc()}
    finally:
        sys.argv = saved_argv


def _pump():
    while True:
        try:
            request, reply = REQUESTS.get_nowait()
        except queue.Empty:
            return 0.1
        reply["result"] = _run(request)
        reply["done"].set()


def _serve(connection):
    with connection:
        data = b""
        while not data.endswith(b"\n"):
            chunk = connection.recv(65536)
            if not chunk:
                break
            data += chunk
        try:
            request = json.loads(data.decode("utf-8"))
        except ValueError:
            result = {"ok": False, "error": "Malformed request"}
        else:
            if not TOKEN or request.get("token") != TOKEN:
                result = {"ok": False, "error": "Bad token"}
            elif request.get("ping"):
                result = {"ok": True, "output": bpy.app.version_string}
            else:
                reply = {"done": threading.Event()}
                REQUESTS.put((request, reply))
                reply["done"].wait()
                result = reply["result"]
        connection.sendall((json.dumps(result) + "\n").encode("utf-8"))


def _listen():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    while True:
        connection, _ = server.accept()
        threading.Thread(target=_serve, args=(connection,), daemon=True).start()


if not TOKEN:
    print("HOMESTEAD_LIVE_BRIDGE disabled: HOMESTEAD_BLENDER_TOKEN is not set")
else:
    bpy.app.timers.register(_pump, persistent=True)
    threading.Thread(target=_listen, daemon=True).start()
    print(f"HOMESTEAD_LIVE_BRIDGE listening on {HOST}:{PORT}")
