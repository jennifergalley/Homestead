"""Stone building kit: a near-flat stone-slate roof on a timber frame (SM_StoneRoof).

Real-object research (written before modeling):
- Stone-slate roofs (Welsh and Cornish slate, Cotswold and Horsham "tilestones", Alpine
  and Ligurian "lose") hang split slabs 1-3 cm thick in overlapping courses: each course
  covers the one below so every point has at least two layers, with a few centimetres of
  head lap; the gauge (exposed length) is a little under half the slate length. The
  eaves course is tipped up on a tilting fillet so it lies like the rest.
- Slates sit on boards or battens over joists carried by wall plates bedded in the wall
  head. Old slate weathers from blue-black to a grey-green bloom with pale crustose
  lichen on the upper faces and moss where water lingers at the butts.
- A truly flat slate roof leaks; this one is "flat-ish": the slates tilt ~6 degrees each
  but the roof plane stays level, which suits a modular building kit.

Dimensions and pivot (Unreal local cm; Blender Y is negated, see stone_building/__init__):
pivot at the building cell centre on the floor (0, 0, 0), so the mesh sits at roof height:
wall plates Z 258..268, joists 268..279, deck 279..281.5, slates to ~288.5. Plan extent
X -160..160 (slates -150..150 plus undercloak verges), Y -158..166 (eaves to top tails).
Place every roof at yaw 0 so the courses lap across cells (see stone_building/roof.py).
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "StoneRoof"
DESCRIPTION = ("Building kit: near-flat roof of lapped stone slates on boards, joists and wall plates, tiling "
               "at 300 cm; LOD1/LOD2 (original). Pivot = cell centre on the floor; mesh at Z 258..~289.")
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 180), "focus": (-1.1, 1.58, 2.86), "views": ["hero", "detail", "eye"],
          "eye_distance": 4.0}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Cell centre on the floor (0, 0, 0); the mesh sits at roof height",
          "extent_cm": {"x": [-160, 160], "y": [-158, 166], "z": [258, 289]},
          "layers_z_cm": {"wall_plates": [258, 268], "joists": [268, 279], "deck": [279, 281.5],
                          "slates_top": 288.5},
          "placement": ("300 cm grid, always yaw 0 (independent of the building's rotation): the slate courses "
                        "run up +Y (Unreal) and lap under the next roof's eaves; verges lap under the X "
                        "neighbour. Rotated roofs would clash in the 10-16 cm lap zones.")}


def _modules():
    from stone_building import masonry, roof
    if not bpy.app.background:
        for module in (masonry, roof):
            importlib.reload(module)
    return masonry, roof


def build(kit):
    masonry, roof = _modules()
    obj = roof.make(kit, "SM_StoneRoof", seed=4)
    meshes = masonry.finish(kit, obj)
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
