"""Stone building kit: a 3 x 3 m flagstone foundation on a rubble footing (SM_StoneFoundation).

Real-object research (written before modeling):
- A pre-industrial house floor in stone country was a platform of flagstones (split
  granite or slate slabs 5-10 cm thick, 40-100 cm across) bedded on rammed rubble and
  pointed or grouted with lime; the edge was a ring of long kerb stones set on edge so the
  flags could not creep, the whole raised a hand's breadth above the yard on a footing
  of rubble laid in a trench below frost depth.
- Walking wears flag tops smooth and dull and keeps lichen off; dirt collects in the
  joints; the footing below ground is soil-stained rubble.

Dimensions and pivot (Unreal local cm; Blender Y is negated, see stone_building/__init__):
pivot at the centre of the square on the walking surface (0, 0, 0). Flags and kerbs top out
at Z 0 (+-3 mm relief), X and Y -150..150, footing down to Z -40 (meant to be sunk into
uneven ground). Foundations at 300 cm spacing butt edge to edge. Everything is generated
here: no scanned or downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "StoneFoundation"
DESCRIPTION = ("Building kit: 3 x 3 m foundation of pointed granite flagstones inside a ring of kerb stones on "
               "a rubble footing to Z -40; LOD1/LOD2 (original). Pivot = centre of the walking surface.")
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (1.2, -1.45, 0.35), "views": ["hero", "detail", "eye"],
          "eye_distance": 3.5}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Centre of the square on the walking surface (0, 0, 0)",
          "extent_cm": {"x": [-150, 150], "y": [-150, 150], "z": [-40, 0.3]},
          "placement": "300 cm grid; yaw irrelevant (any multiple of 90 works). Sink the footing into the ground."}


def _modules():
    from stone_building import foundation, masonry, wall
    if not bpy.app.background:
        for module in (masonry, wall, foundation):
            importlib.reload(module)
    return masonry, foundation


def build(kit):
    masonry, foundation = _modules()
    obj = foundation.make(kit, "SM_StoneFoundation", seed=3)
    meshes = masonry.finish(kit, obj)
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
