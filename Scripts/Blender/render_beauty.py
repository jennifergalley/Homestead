"""Photoreal Cycles review renders of a built asset set, headless.

    blender --background <Assets/Props/Name/Name.blend> --python render_beauty.py
        -- [--samples N] [--width W --height H] [--mesh SM_Name]

Writes beauty_<mesh>_hero.png / _detail.png beside report.json and records them
in the report. Runs outside the live window so the UI stays responsive.
"""
import argparse
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_kit as kit  # noqa: E402


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--samples", type=int, default=256)
    parser.add_argument("--width", type=int, default=3840)
    parser.add_argument("--height", type=int, default=2160)
    parser.add_argument("--mesh", action="append")
    parser.add_argument("--view", action="append", help="hero, detail and/or eye (default: recipe BEAUTY views)")
    parser.add_argument("--prefix", default="beauty", help="Output file prefix (e.g. draft for quick passes)")
    args = parser.parse_args(argv)
    folder = Path(bpy.data.filepath).parent
    report_path = folder / "report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    names = args.mesh or [n for n in report["meshes"] if "_LOD" not in n]
    for obj in bpy.context.scene.objects:
        obj.hide_render = obj.name not in names
    for name in names:
        obj = bpy.data.objects[name]
        for other in bpy.context.scene.objects:
            other.hide_render = other is not obj
        review = report["meshes"][name].get("review", {})
        result = kit.render_beauty(obj, folder, f"{args.prefix}_{name}", samples=args.samples,
                                   resolution=(args.width, args.height),
                                   pose=review.get("pose", (0, 0, 0)), focus=review.get("focus"),
                                   views=tuple(args.view or review.get("views", ("hero", "detail"))),
                                   ground=review.get("ground", "lowest"),
                                   eye_distance=review.get("eye_distance"),
                                   detail_distance=review.get("detail_distance"),
                                   detail_fstop=review.get("detail_fstop", 22.0))
        result["views"] = {view: Path(p).name for view, p in result["views"].items()}
        if args.prefix == "beauty":
            report["meshes"][name]["beauty"] = result
        print(f"HOMESTEAD_BEAUTY {name} {result['device']} {result['gpus']} {result['views']}")
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("HOMESTEAD_BEAUTY_DONE")


if __name__ == "__main__":
    main()
