"""Offline, non-mutating inventory for the hairstyle recipe."""
import json
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Build" / "CharacterPreview" / "HairstyleRefinement"
OUT.mkdir(parents=True, exist_ok=True)
records = {}
for body, source in [
    ("Preferred", "Variants/Heroine_Variants.blend"),
    ("Willow", "BodyPresets/Willow/Willow.blend"),
    ("Hazel", "BodyPresets/Hazel/Hazel.blend"),
]:
    bpy.ops.wm.open_mainfile(filepath=str(ROOT / "Assets" / "Characters" / source))
    record = {}
    for style in ["LongWave", "Bob"]:
        obj = bpy.data.objects["Heroine_Hair_" + style]
        record[style] = {
            "matrix": [list(r) for r in obj.matrix_world],
            "vertices": [list(v.co) for v in obj.data.vertices],
            "faces": [list(p.vertices) for p in obj.data.polygons],
            "modifiers": [(m.name, m.type) for m in obj.modifiers],
            "materials": [m.name for m in obj.data.materials],
            "groups": [g.name for g in obj.vertex_groups],
            "images": [n.image.filepath for m in obj.data.materials for n in m.node_tree.nodes
                       if n.type == "TEX_IMAGE" and n.image],
        }
    record["bones"] = {b.name: list(b.head_local) for b in bpy.data.objects["Heroine_Rig"].data.bones}
    records[body] = record
(OUT / "source-inventory.json").write_text(json.dumps(records))
print("HAIRSTYLE_SOURCE_INVENTORY", str(OUT / "source-inventory.json"))
