"""Minimal command-line client for the Unreal Editor MCP server started by Start-EditorMcp.ps1.

Agents normally use the `unreal` MCP server from .github/mcp.json directly. This client exists for
shells and sessions that were started before the editor (MCP clients only connect at startup).

Examples:
    python Scripts/editor_mcp.py tools
    python Scripts/editor_mcp.py call list_toolsets
    python Scripts/editor_mcp.py call describe_toolset "{\"toolset\": \"EditorAppToolset\"}"
    python Scripts/editor_mcp.py call call_tool @args.json --images Saved/McpCaptures
    python Scripts/editor_mcp.py gallery list
    python Scripts/editor_mcp.py gallery shop-buy --input Pad --wait 6

`gallery <id>` puts one UI gallery state on screen in the running in-viewport PIE
(homestead.UIGallery, Source/SurvivalGame/HomesteadUIGallery.h) and captures the editor window,
Slate included. It runs only on an isolated save route: start the editor with
Start-EditorMcp.ps1 -PreviewProfile gallery. For every state at several resolutions use
Scripts/Capture-UiGallery.ps1.
"""
from __future__ import annotations

import argparse
import base64
import json
import os
import sys
import time
import urllib.error
import urllib.request

PROTOCOL_VERSION = "2025-06-18"
EDITOR_TOOLSET = "EditorToolset.EditorAppToolset"
PYTHON_TOOLSET = "homestead_agent.toolset.HomesteadEditorPython"
# Runs a console command in the PIE world (or the editor world when PIE isn't running).
CONSOLE_CODE = """import unreal
sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = sub.get_game_world() or sub.get_editor_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0) if sub.get_game_world() else None
unreal.SystemLibrary.execute_console_command(w, {command!r}, pc)
"""


class McpClient:
    def __init__(self, url: str, timeout: float):
        self.url = url
        self.timeout = timeout
        self.session_id: str | None = None
        self.next_id = 1

    def _post(self, payload: dict) -> dict | None:
        headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
        if self.session_id:
            headers["Mcp-Session-Id"] = self.session_id
            headers["MCP-Protocol-Version"] = PROTOCOL_VERSION
        request = urllib.request.Request(self.url, json.dumps(payload).encode(), headers, method="POST")
        with urllib.request.urlopen(request, timeout=self.timeout) as response:
            self.session_id = response.headers.get("Mcp-Session-Id") or self.session_id
            body = response.read().decode("utf-8", "replace")
            content_type = response.headers.get("Content-Type", "")
        if not body.strip():
            return None
        if "text/event-stream" in content_type:
            messages = [json.loads(line[5:]) for line in body.splitlines() if line.startswith("data:") and line[5:].strip()]
            for message in reversed(messages):
                if message.get("id") == payload.get("id"):
                    return message
            return messages[-1] if messages else None
        return json.loads(body)

    def request(self, method: str, params: dict | None = None) -> dict:
        payload = {"jsonrpc": "2.0", "id": self.next_id, "method": method}
        self.next_id += 1
        if params is not None:
            payload["params"] = params
        message = self._post(payload) or {}
        if "error" in message:
            raise RuntimeError(json.dumps(message["error"], indent=2))
        return message.get("result", {})

    def initialize(self) -> dict:
        result = self.request("initialize", {
            "protocolVersion": PROTOCOL_VERSION,
            "capabilities": {},
            "clientInfo": {"name": "survivalgame-editor-mcp-cli", "version": "1.0"},
        })
        self._post({"jsonrpc": "2.0", "method": "notifications/initialized"})
        return result


def _write_image(node: dict, directory: str, counter: list[int]) -> None:
    os.makedirs(directory, exist_ok=True)
    extension = node["mimeType"].split("/")[-1]
    path = os.path.abspath(os.path.join(directory, f"mcp-{time.strftime('%Y%m%d-%H%M%S')}-{time.time_ns() % 1_000_000_000:09d}-{counter[0]}.{extension}"))
    counter[0] += 1
    with open(path, "wb") as handle:
        handle.write(base64.b64decode(node["data"]))
    node["data"] = f"<saved to {path}>"


def _extract_images(node, directory: str, counter: list[int]):
    if isinstance(node, dict):
        if str(node.get("mimeType", "")).startswith("image/") and isinstance(node.get("data"), str) and len(node["data"]) > 64:
            _write_image(node, directory, counter)
        for value in node.values():
            _extract_images(value, directory, counter)
    elif isinstance(node, list):
        for value in node:
            _extract_images(value, directory, counter)


def save_images(result: dict, directory: str) -> None:
    """Writes returned images to disk, including toolset images embedded in JSON text results."""
    counter = [0]
    for item in result.get("content", []):
        if item.get("type") == "image" and item.get("data"):
            _write_image(item, directory, counter)
        elif item.get("type") == "text" and '"image/' in item.get("text", ""):
            try:
                parsed = json.loads(item["text"])
            except json.JSONDecodeError:
                continue
            _extract_images(parsed, directory, counter)
            item["text"] = json.dumps(parsed)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--url", default=os.environ.get("UNREAL_MCP_URL", "http://127.0.0.1:8765/mcp"))
    parser.add_argument("--timeout", type=float, default=600)
    parser.add_argument("--images", default=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "McpCaptures"),
                        help="directory for returned images")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("tools", help="list MCP tools")
    call = sub.add_parser("call", help="call an MCP tool")
    call.add_argument("tool")
    call.add_argument("arguments", nargs="?", default="{}", help="JSON object, or @file.json")
    gallery = sub.add_parser("gallery", help="show a UI gallery state in PIE and capture it (or 'list')")
    gallery.add_argument("id")
    gallery.add_argument("--input", choices=["KBM", "Pad"], default="KBM")
    gallery.add_argument("--wait", type=float, default=6.0,
                         help="seconds to let it settle (teleports across the estate can take 20+)")
    args = parser.parse_args()

    client = McpClient(args.url, args.timeout)
    try:
        client.initialize()
        if args.command == "tools":
            result = client.request("tools/list")
        elif args.command == "gallery":
            command = "homestead.UIGallery list" if args.id == "list" else f"homestead.UIGallery {args.id} {args.input}"
            result = client.request("tools/call", {"name": "call_tool", "arguments": {
                "toolset_name": PYTHON_TOOLSET, "tool_name": "run_python",
                "arguments": {"code": CONSOLE_CODE.format(command=command)}}})
            if args.id != "list" and not result.get("isError"):
                time.sleep(args.wait)
                result = client.request("tools/call", {"name": "call_tool", "arguments": {
                    "toolset_name": EDITOR_TOOLSET, "tool_name": "CaptureEditorImage", "arguments": {}}})
                save_images(result, args.images)
        else:
            raw = args.arguments
            if raw.startswith("@"):
                with open(raw[1:], encoding="utf-8") as handle:
                    raw = handle.read()
            result = client.request("tools/call", {"name": args.tool, "arguments": json.loads(raw)})
            save_images(result, args.images)
    except urllib.error.URLError as error:
        print(f"Cannot reach Unreal MCP at {args.url}: {error}. Run Scripts\\Start-EditorMcp.ps1 first.", file=sys.stderr)
        return 2
    json.dump(result, sys.stdout, indent=2)
    print()
    return 1 if result.get("isError") else 0


if __name__ == "__main__":
    sys.exit(main())
