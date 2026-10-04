"""Review modeled fish anatomy without pigment, preserving the source blend.

Usage: blender -b <proof.blend> --python render_fish_anatomy.py -- --samples 192
Writes separate 4K anatomy views with the same framing, lighting and wet film.
"""
import argparse
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_kit as kit
import homestead_materials as materials


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--samples", type=int, default=192)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    folder = Path(bpy.data.filepath).parent
    report_path = folder / "report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    clay = materials.Graph("M_FishStructuralClay")
    clay.set("Base Color", (.26, .26, .26, 1))
    clay.set("Roughness", .22)
    clay.set("Coat Weight", materials.FISH_COAT_WEIGHT)
    clay.set("Coat Roughness", materials.FISH_COAT_ROUGHNESS)
    oral = materials.Graph("M_FishStructuralOral")
    oral.set("Base Color", (.045, .045, .045, 1))
    oral.set("Roughness", .24)
    oral.set("Coat Weight", materials.FISH_COAT_WEIGHT)
    oral.set("Coat Roughness", materials.FISH_COAT_ROUGHNESS)
    evidence = {}
    for name, info in report["meshes"].items():
        obj = bpy.data.objects[name]
        for other in bpy.context.scene.objects:
            other.hide_render = other is not obj
        for slot in obj.material_slots:
            if slot.material.name.endswith("Mouth"):
                slot.material = oral.mat
            elif not slot.material.name.endswith("Eye"):
                slot.material = clay.mat
        review = info["review"]
        result = kit.render_beauty(obj, folder, "anatomy_" + name, samples=args.samples,
                                   resolution=(3840, 2160), pose=review["pose"],
                                   focus=review["focus"], detail_distance=review["detail_distance"])
        result["views"] = {key: Path(path).name for key, path in result["views"].items()}
        evidence[name] = result
    report["structural_review"] = evidence
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("ORIGINAL_FISH_ANATOMY_REVIEW_DONE", len(evidence))


if __name__ == "__main__":
    main()
