"""Minimal command-line client for the Unreal Editor MCP server started by Start-EditorMcp.ps1.

Agents normally use the `unreal` MCP server from .github/mcp.json directly. This client exists for
shells and sessions that were started before the editor (MCP clients only connect at startup).

Examples:
    python Scripts/editor_mcp.py tools
    python Scripts/editor_mcp.py call list_toolsets
    python Scripts/editor_mcp.py call describe_toolset "{\"toolset\": \"EditorAppToolset\"}"
    python Scripts/editor_mcp.py call call_tool @args.json --images Saved/McpCaptures
    python Scripts/editor_mcp.py animinspect Weeds --recipe kneel_pull_weeds
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
    inspect = sub.add_parser("animinspect", help="record a heroine action frame by frame (Scripts/Inspect-Animation.ps1)")
    inspect.add_argument("clip", help="a Character Lab LabAction name, e.g. Weeds, Mow, Sticks")
    inspect.add_argument("--recipe", help="homestead_agent recipe whose FRAMES name the beats")
    inspect.add_argument("--contacts", default="", help="comma-separated FRAMES keys of strikes or impacts")
    inspect.add_argument("--every", type=int, default=2)
    inspect.add_argument("--views", default="front,left,right,top,threequarter")
    inspect.add_argument("--hold", help="a LabHold tool to carry first")
    args = parser.parse_args()

    if args.command == "animinspect":
        # Runs its own hidden game process (not this editor), so no MCP connection is needed.
        import subprocess
        script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Inspect-Animation.ps1")
        command = ["pwsh", "-NoProfile", "-File", script, "-Clip", args.clip, "-Every", str(args.every),
                   "-Views", args.views]
        if args.recipe:
            command += ["-Recipe", args.recipe]
        if args.contacts:
            command += ["-Contacts", args.contacts]
        if args.hold:
            command += ["-Hold", args.hold]
        return subprocess.call(command)

    client = McpClient(args.url, args.timeout)
    try:
        client.initialize()
        if args.command == "tools":
            result = client.request("tools/list")
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
