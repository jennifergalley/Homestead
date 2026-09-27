"""Stone building kit: a 3 m wall panel with a doorway (SM_StoneDoorway).

Same construction, envelope, pier and pivot as SM_StoneWall (see stone_wall.py for the
research notes): random granite rubble brought to courses, pointed with lime mortar, a
long-and-short quoin pier on the +X end and a capstone coping. The centred doorway is
130 cm wide and 220 cm tall (clear), framed by long-and-short dressed jamb stones and
spanned by a single 192 cm granite lintel (Z 220..248), the way pre-industrial builders
opened a rubble wall. No door leaf, frame or threshold: the floor stays clear at Z 0.

Dimensions and pivot (Unreal local cm; Blender Y is negated, see stone_building/__init__):
pivot at the building cell centre on the floor. Wall X -150..+150 (+160 at the pier),
faces at Y 130/158, Z -10..260; opening X -65..65, Z 0..220. Collision must be complex
(per-poly) so the opening stays walkable: import_props.COLLISION_OVERRIDES does that.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "StoneDoorway"
DESCRIPTION = ("Building kit: 3 m granite rubble wall with a centred 130 x 220 cm doorway under a single "
               "granite lintel, dressed jambs, +X quoin pier and coping; LOD1/LOD2 (original). Pivot = cell "
               "centre on the floor; wall faces at Unreal Y 130/158.")
COLLISION = "box"   # import_props.COLLISION_OVERRIDES makes it per-poly ("complex") so the opening is walkable.
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.55, -1.6, 2.15), "views": ["hero", "detail", "eye"],
          "eye_distance": 4.5}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Cell centre on the floor (0, 0, 0)",
          "extent_cm": {"x": [-150, 160.5], "y": [128, 160.5], "z": [-10, 260]},
          "faces_y_cm": {"inner": 130, "outer": 158, "quoin_pier": [128, 160]},
          "opening_cm": {"x": [-65, 65], "z": [0, 220], "lintel_z": [220, 248]},
          "collision_note": "Use per-poly (complex) collision; a box would plug the doorway.",
          "placement": ("Yaw 0/90/180/270 about the cell centre, interchangeable with SM_StoneWall. The +X end "
                        "carries the corner pier; the plain -X end tucks inside the neighbour's pier.")}


def _modules():
    from stone_building import masonry, wall
    if not bpy.app.background:
        for module in (masonry, wall):
            importlib.reload(module)
    return masonry, wall


def build(kit):
    masonry, wall = _modules()
    obj = wall.make(kit, "SM_StoneDoorway", doorway=True, seed=2)
    meshes = masonry.finish(kit, obj)
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
