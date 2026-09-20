"""Review real ordinary-control setup and watering; never infer performance or exact contact."""
import argparse
import csv
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    folder = args.directory
    rows = list(csv.DictReader((folder / "telemetry.csv").open(encoding="utf-8-sig")))
    before = [r for r in rows if r["pass"] == "before-water"]
    action = [r for r in rows if r["pass"] == "water"]
    after = [r for r in rows if r["pass"] == "after-water"]
    if not before or not action or not after:
        raise ValueError("Missing before, action or recovered watering evidence.")
    anchor = before[-1]
    visible = [r for r in action if int(r["tool_visible"])]
    if not visible:
        raise ValueError("No hand-held tool was actually observed.")

    def distance(a, b, prefix):
        return math.sqrt(sum((float(a[prefix + axis]) - float(b[prefix + axis])) ** 2 for axis in "xyz"))

    report = {
        "frames": len(rows), "duration_seconds": float(rows[-1]["seconds"]),
        "setup": "Fresh ordinary clearing, mapped gathering/crafting/refill/till/plant; no injected save, teleport or debug time/state edit.",
        "tool_samples": len(visible),
        "water_debit": int(anchor["water_stock"]) - int(after[-1]["water_stock"]),
        "moisture_before": float(anchor["plot_moisture"]),
        "moisture_after": float(after[-1]["plot_moisture"]),
        "action_starts": int(after[-1]["water_starts"]) - int(anchor["water_starts"]),
        "final_weight": float(after[-1]["water_weight"]),
        "final_tool_visible": bool(int(after[-1]["tool_visible"])),
        "peak_tilt_degrees": max(-float(r["can_pitch"]) for r in visible),
        "maximum_tool_world_radius_cm": max(float(r["tool_radius"]) for r in visible) if "tool_radius" in rows[0] else None,
        "tool_world_scale": sorted(set(float(r["tool_scale"]) for r in visible)) if "tool_scale" in rows[0] else None,
        "wrist_travel_cm": max(distance(r, anchor, "right_hand_") for r in action),
        "actor_travel_cm": max(distance(r, anchor, "") for r in action),
        "toe_travel_cm": {s: max(distance(r, anchor, s + "_toe_") for r in action) for s in ("left", "right")},
        "limits": "Sparse sampled images, not FPS/smoothness/controller comfort. Generic forward pour, no target/terrain IK or exact ground contact.",
    }
    selected = [anchor] + [min(action, key=lambda r: abs(float(r["water_phase"]) - phase)) for phase in (.4, .75, 1.1, 1.55)] + [after[-1]]
    sheet = Image.new("RGB", (1280, 1626), "#101b16")
    draw = ImageDraw.Draw(sheet)
    for index, row in enumerate(selected):
        with Image.open(folder / "Frames" / f"frame-{int(row['frame']):05d}.png") as image:
            w, h = image.size
            crop = image.crop((round(w * .2), round(h * .25), round(w * .75), h)).resize((640, 510))
            x, y = index % 2 * 640, index // 2 * 542
            sheet.paste(crop, (x, y + 32))
            draw.text((x + 8, y + 8), f"{row['seconds']}s {row['pass']} phase={row['water_phase']} tool={row['tool_visible']}", fill="white")
    sheet.save(folder / "watering-sheet.png")
    (folder / "watering-review.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
