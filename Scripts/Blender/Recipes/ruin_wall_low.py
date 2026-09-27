"""Ruined manor kit: a 3 m granite rubble run fallen almost to its footing, 0.4-1.4 m high

Real-object research: Cornish gentry houses of the 18th and early 19th centuries (Trewithen,
Prideaux Place and the many smaller "barton" houses) were built of granite rubble brought to
courses, with dressed long-and-short quoins at the corners and around openings, granite sills and
single-stone lintels over tall sash windows, under Delabole slate. Walls were 55-75 cm thick; a
two-storey front stood 7-8 m to the eaves, with chimney stacks rising 2-3 m above the ridge at
the gables. A house left roofless for a generation loses its timbers first; the walls then fall
from the top down, stone by stone, leaving stepped, ragged heads, sound runs between collapses,
and window openings whose lintels drop once the wall above them has gone. Lichen, moss and soil
splash climb the faces; soot marks the stacks.

Geometry (Unreal local cm; Blender Y is negated, see stone_building/__init__): a 300 cm run along X, 56 cm thick, broken top 40-140 cm.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "RuinWallLow"
DESCRIPTION = "Ruined manor: a 3 m granite rubble run fallen almost to its footing, 0.4-1.4 m high (original)."
COLLISION = "box"
TRIANGLE_BUDGET = 400000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.4, 0.9, 0.6), "views": ["hero", "detail"]}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Base centre of the run (0, 0, 0); the run lies along X, faces at Y +-28, footing buried to Z -15.", "placement": "Yaw 0/90/180/270 along the manor walls; butt runs end to end (their ends are dressed quoins)."}


def _modules():
    from stone_building import masonry, ruin
    if not bpy.app.background:
        for module in (masonry, ruin):
            importlib.reload(module)
    return masonry, ruin


def build(kit):
    masonry, ruin = _modules()
    m = ruin.wall_run(300.0, 40.0, 140.0, 37)
    obj = m.build("SM_RuinWallLow", ruin.materials(kit, "SM_RuinWallLow", 37))
    masonry.unwrap(obj, shrink={ruin.MAT_MORTAR: 0.35})
    meshes = masonry.finish(kit, obj, lod_ratios=(0.35, 0.12))
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
