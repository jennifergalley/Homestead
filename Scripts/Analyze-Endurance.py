"""Summarize bounded game-only evidence; never infer scanout or indefinite leak freedom."""
import csv
import hashlib
import json
import pathlib
import re
import struct
import sys
import zlib


def analyze(directory):
    root = pathlib.Path(directory)
    result = json.loads((root / "progress.json").read_text(encoding="utf-8-sig"))
    assert result["status"] == "passed", f"Not a full pass: {result['status']}"
    target = result["targetSeconds"]
    assert result["wallSeconds"] >= target
    assert result["unpausedSeconds"] >= target * 0.9
    assert result["movingSeconds"] >= target * 0.3
    assert result["loads"] == 1 and result["exactSameWorldRoundTrip"]
    assert result["navigationFailures"] <= 3 and result["recoveryAttempts"] == 0
    assert result["gathers"] >= (2 if target >= 2700 else 1) and result["eats"] >= 1
    assert result["waypoints"] >= (40 if target >= 2700 else 4)
    if target >= 2700:
        assert result["naturalGameHours"] >= 15
        assert result["autosaveSlots"] == 3 and result["autosaveWrites"] >= 8
    if target == 4200:
        assert result["freshWorld"] and result["naturalGameHours"] >= 24
        assert result["verifiedHarvestedResourceRegrowth"] >= 1
    events = (root / "endurance-events.txt").read_text(encoding="utf-8-sig")
    writes = re.findall(r"valid rotating autosave (Homestead_Auto_\d) at UTC (\d+)", events)
    assert len(writes) == len(set(writes)) == result["autosaveWrites"]
    engine_log = root / "engine.log"
    log_path = engine_log
    if not engine_log.exists():
        assert result.get("freshWorld") and result.get("debugBindingQueryAvailable") is False
        log_path = root / "qa-native.log"
    log = log_path.read_text(encoding="utf-8-sig", errors="replace")
    errors = [line for line in log.splitlines()
              if any(term in line for term in ("Fatal error:", "Assertion failed:", "Ensure condition failed:", "Unhandled Exception"))]
    assert not errors, "\n".join(errors)
    saves = []
    for file in sorted((root / "SmokeSave").glob("*")):
        if not (file.name.endswith(".sav") or file.name.endswith(".sav.bak")):
            continue
        data = file.read_bytes()
        assert data[:8] == b"HOMESAV1", file
        assert struct.unpack("<I", data[8:12])[0] == zlib.crc32(data[12:]), file
        saves.append({"file": file.name, "sha256": hashlib.sha256(data).hexdigest(), "crcValid": True})
    rows = list(csv.DictReader((root / "endurance-samples.csv").open(encoding="utf-8-sig", newline="")))
    warmed = [row for row in rows if float(row["wall_seconds"]) >= 60]
    assert len(warmed) >= 3
    trends = {}
    for key in ("physical_bytes", "virtual_bytes", "object_slots_in_use", "resources", "structures", "plots"):
        values = [float(row[key]) for row in warmed]
        trends[key] = {"first": values[0], "last": values[-1], "min": min(values), "max": max(values),
                       "netChange": values[-1] - values[0]}
    frames = sorted((root / "Frames").glob("*.png"))
    assert len(frames) == result["captures"] and len(frames) <= (target // 600 + 2)
    if "litGuardVersion" in result:
        assert result["litGuardVersion"] == 1 and result["litGuardTicks"] > 0
        assert result["startupViewMode"] == 3 and result["startupLighting"] and not result["startupShaderComplexity"]
        if result.get("debugBindingQueryAvailable", True):
            assert result["effectiveF5DebugBinding"] == result["effectiveF9DebugBinding"] == ""
        else:
            assert result["freshWorld"], "Missing debug-binding query is admitted only for fresh Shipping"
            assert "shipping=1\ntrace_compiled=0\nroute=visual" in (root / "qa-admission.txt").read_text()
            guard = json.loads((root / "qa-guard-result.json").read_text(encoding="utf-8-sig"))
            assert guard["status"] == "passed" and guard["subjectExited"] and guard["guardDisposed"]
            assert not guard["hardTerminated"] and not guard["cleanupErrors"]
        loads = 1 if result.get("freshWorld", False) else 2
        assert result["f9NoScreenshotChecks"] == loads
        assert re.findall(r"F9 screenshot request before=(\d) after=(\d)", events) == [("0", "0")] * loads
        assert not list((root / "EngineUser").rglob("*.png")), "Unsolicited engine screenshot files"
        presentations = [result["presentation"]]
        presentations += [json.loads(frame.with_suffix(".json").read_text(encoding="utf-8-sig")) for frame in frames]
        for presentation in presentations:
            assert presentation["available"] and presentation["viewMode"] == 3
            assert presentation["lighting"] and not presentation["shaderComplexity"]
            flags = dict(part.split("=", 1) for part in presentation["showFlags"].split(","))
            assert flags["Lighting"] == "1" and flags["ShaderComplexity"] == "0"
    analysis = {"result": result, "postWarmupTrends": trends, "validSaveEnvelopes": saves,
                "diagnosticLog": log_path.name, "engineLogAvailable": engine_log.exists(),
                "distinctAutosaveTransitions": writes, "actualFrames": len(frames),
                "naturalHoursFromEngineDeltaAtRecordedDayLength": result["engineUnpausedSeconds"] * 24 / (result.get("dayMinutes", 60) * 60),
                "pausedPercent": 100 * result["pausedSeconds"] / result["wallSeconds"],
                "movingPercent": 100 * result["movingSeconds"] / result["wallSeconds"],
                "limits": "Instrumented actor cadence, not GPU/Present/scanout. Observer allocations/IO and concurrent machine load included. Not an indefinite leak-free claim."}
    (root / "analysis.json").write_text(json.dumps(analysis, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"wallSeconds": result["wallSeconds"], "naturalGameHours": result["naturalGameHours"],
                      "distinctAutosaveWrites": len(writes), "validSaves": len(saves), "frames": len(frames)}))


if __name__ == "__main__":
    analyze(sys.argv[1])
