"""Validate this opt-in observer's timing/capture separation, not physical tearing."""
import argparse
import csv
import json
import math
from pathlib import Path
import statistics


def analyze(directory):
    with (directory / "presentation-timings.csv").open(encoding="utf-8-sig") as source:
        rows = list(csv.DictReader(source))
    with (directory / "telemetry.csv").open(encoding="utf-8-sig") as source:
        captures = list(csv.DictReader(source))
    labels = ("timing-idle", "timing-walk", "timing-turn", "timing-sweep")
    timing = [r for r in rows if r["pass"] in labels]
    if not timing or any(int(r["captures_requested_before_tick"]) or int(r["capture_requested_this_tick"]) for r in timing):
        raise ValueError("The timing interval is missing or contaminated by screenshot requests.")
    if not captures or any(not r["pass"].startswith("capture-") for r in captures):
        raise ValueError("Images must belong only to the subsequent capture interval.")
    if sum(int(r["capture_requested_this_tick"]) for r in rows) != len(captures):
        raise ValueError("Requested and recorded capture counts differ.")
    wall_seconds = 0
    request_times = []
    for row in rows:
        wall_seconds += float(row["wall_frame_ms"]) / 1000
        if int(row["capture_requested_this_tick"]):
            request_times.append(wall_seconds)
    request_span = request_times[-1] - request_times[0] if len(request_times) > 1 else 0
    groups = {}
    for label in labels:
        group = [r for r in timing if r["pass"] == label]
        if not group:
            raise ValueError(f"Missing required normal-control segment: {label}")
        values = sorted(float(r["wall_frame_ms"]) for r in group)
        if any(not math.isfinite(value) or value <= 0 for value in values):
            raise ValueError("Invalid unclamped wall-clock frame interval.")
        minimum_seconds = 1.9 if label == "timing-idle" else 5.9
        if sum(values) / 1000 < minimum_seconds:
            raise ValueError(f"Timing interval was incomplete: {label}")
        speed = max(float(r["speed"]) for r in group)
        yaw = sum(abs((float(b["view_yaw"]) - float(a["view_yaw"]) + 180) % 360 - 180)
                  for a, b in zip(group, group[1:]))
        if label in ("timing-walk", "timing-turn") and speed < 20:
            raise ValueError(f"Movement was not actually observed: {label}")
        if label == "timing-sweep" and yaw < 40:
            raise ValueError("The mapped camera sweep did not occur.")
        groups[label] = {
            "samples": len(values), "wall_seconds": sum(values) / 1000,
            "mean_tick_hz": 1000 / statistics.mean(values),
            "median_ms": statistics.median(values),
            "p95_ms": values[math.ceil(len(values) * .95) - 1],
            "p99_ms": values[math.ceil(len(values) * .99) - 1],
            "max_ms": max(values), "over_33_33ms": sum(v > 33.333 for v in values),
            "maximum_actor_speed_cm_s": speed, "camera_yaw_travel_degrees": yaw,
        }
    settings = (directory / "presentation-settings.txt").read_text(encoding="utf-8-sig")
    if "[start]" not in settings or "[end]" not in settings or "actual_viewport=" not in settings:
        raise ValueError("Missing active runtime settings snapshots.")
    admission = directory / "qa-admission.txt"
    if admission.exists():
        entries = [line.split("=", 1) for line in admission.read_text(encoding="utf-8-sig").splitlines()]
        if any(len(entry) != 2 for entry in entries) or len({entry[0] for entry in entries}) != len(entries):
            raise ValueError("Malformed Shipping QA admission.")
        values = dict(entries)
        if (values.get("version") != "1" or values.get("shipping") != "1"
                or values.get("trace_compiled") != "0" or values.get("route") != "visual"
                or Path(values.get("save_directory", "")).resolve() != (directory / "SmokeSave").resolve()
                or Path(values.get("project_saved_directory", "")).resolve() != (directory / "EngineUser" / "Saved").resolve()):
            raise ValueError("Shipping QA did not confirm the isolated visual route and backing directories.")
        admission_source = "qa-admission.txt (native Shipping QA; no Development log is assumed)"
    else:
        log = (directory / "engine.log").read_text(encoding="utf-8-sig", errors="replace")
        if "mode=test-sandbox" not in log or "automation_input=1 smoke_actor=0 visual_actor=1" not in log:
            raise ValueError("The actual process did not confirm isolated saves/input and the single observer.")
        admission_source = "engine.log (native Development sandbox and observer)"
    report = {
        "status": "diagnostic-tooling-verified; symptom-unresolved",
        "screenshot_free_timing": groups, "captured_frames": len(captures),
        "capture_request_cadence": {
            "nominal_threshold_hz": 8,
            "measured_request_hz": (len(request_times) - 1) / request_span if request_span > 0 else None,
            "first_to_last_request_wall_seconds": request_span,
            "recorded_tick_wall_seconds": wall_seconds,
            "ticks_exceeding_legacy_one_second_route_clock_cap": sum(float(r["wall_frame_ms"]) > 1000 for r in rows),
        },
        "settings": "presentation-settings.txt (start/end runtime queries; no CVar writes)",
        "admission": admission_source,
        "pipeline": "Offscreen rendered game framebuffer; not physical display/scanout.",
        "limits": [
            "Instrumented game-thread tick intervals are not GPU or DXGI Present measurements.",
            "The inherited observer's 8Hz wording is a nominal threshold, not delivered sample rate; use measured_request_hz.",
            "Legacy telemetry.seconds uses a capped route clock on >1s stalls; this report uses unclamped wall intervals.",
            "Readback perturbs capture pacing and can miss brief temporal artifacts.",
            "Timing and capture visit different positions; not a controlled performance A/B.",
            "No clean-performance, tearing diagnosis, visual approval or fix claim.",
        ],
    }
    (directory / "presentation-analysis.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(json.dumps(analyze(args.directory.resolve()), indent=2))
