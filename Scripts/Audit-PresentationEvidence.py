"""Read existing game logs/capture timestamps without modifying historical proofs."""
import argparse
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path


STAMP = re.compile(r"\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]")
MODE = re.compile(r"Set new viewmode: (\w+)")


def timestamp(line):
    match = STAMP.search(line)
    return (datetime.strptime(match[1], "%Y.%m.%d-%H.%M.%S:%f")
            .replace(tzinfo=timezone.utc).timestamp()) if match else None


def inspect(path):
    raw = path.read_bytes()
    transitions, requests, keys = [], [], []
    mode = None
    times = []
    for number, line in enumerate(raw.decode("utf-8-sig", errors="replace").splitlines(), 1):
        time = timestamp(line)
        if time is not None:
            times.append(time)
        match = MODE.search(line)
        if match:
            mode = match[1]
            transitions.append({"line": number, "utcSeconds": timestamp(line), "mode": mode, "text": line})
        if "INPUT F5" in line or "INPUT F9" in line:
            keys.append({"line": number, "text": line})
        if "Smoke trace:" in line and "BEGIN Capture" in line:
            requests.append({"line": number, "lastLoggedMode": mode,
                             "request": line.split("BEGIN ", 1)[1].split(" actor=", 1)[0]})
    first = next((t["utcSeconds"] for t in transitions if t["mode"] == "ShaderComplexity"), None)
    frames = []
    candidates = list(path.parent.glob("*.png")) + list((path.parent / "Frames").glob("*.png"))
    for image in sorted(candidates):
        if image.parent.name != "Frames" and not image.with_suffix(".frame.txt").exists():
            continue
        written = image.stat().st_mtime
        if not times or written < min(times) or written > max(times) + 1:
            continue
        delta = written - first if first is not None else None
        relation = ("no_logged_shader_transition" if delta is None else
                    "timestamp_near_transition" if abs(delta) < 2 else
                    "written_after_first_transition" if delta > 0 else "written_before_first_transition")
        preceding = [t for t in transitions if t["utcSeconds"] is not None and t["utcSeconds"] <= written]
        frames.append({"name": image.name, "relation": relation, "secondsFromFirstTransition": delta,
                       "lastLoggedModeAtWrite": preceding[-1]["mode"] if preceding else None})
    return {
        "log": str(path), "logSha256": hashlib.sha256(raw).hexdigest(),
        "transitions": transitions, "mappedSaveLoadInputs": keys, "captureRequests": requests,
        "frameWriteCorrelations": frames,
        "limits": "Capture-request ordering is stronger than file write time. File timestamps alone are not per-frame ShowFlags. Absence of a transition is not proof of all renderer settings.",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("roots", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError("Use a fresh audit output file.")
    reports = []
    for root in args.roots:
        for path in sorted(root.rglob("*.log")):
            if "feedback-layout" in str(path):
                continue
            report = inspect(path)
            if report["transitions"] or report["captureRequests"] or report["frameWriteCorrelations"]:
                reports.append(report)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(reports, indent=2), encoding="utf-8")
    print(f"Read-only audit recorded {len(reports)} relevant logs in {args.output}")


if __name__ == "__main__":
    main()
