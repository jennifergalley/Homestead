"""The standing room's granite hearth (SM_StoneHearth), built against the manor wall.

Real-object research (written before modeling):
- Cornish farmhouse and gentry kitchens of the 18th-19th centuries cooked on a wide open
  hearth: two dressed granite jamb stones carrying one massive granite lintel (sometimes an
  oak bressummer), the chimney breast built out from the gable in rubble, and a large granite
  hearthstone laid flush with the floor in front. Openings of 1.0-1.8 m wide and about 1 m
  high were typical; by the 1850s many had a cloam oven or an iron range fitted, but a
  ruined house's surviving room keeps the old open hearth.
- Inside, the back and cheeks are rubble blackened by a century of soot, with a bed of wood
  ash on the hearth floor. Logs lay on a pair of wrought-iron firedogs (andirons).
- The lintel and the stones nearest the fire soot over; the breast above stays grey granite
  pointed in lime mortar like the walls (see stone_building/wall.py).

Dimensions and pivot (Unreal local cm; Blender Y is negated, see stone_building/__init__):
pivot at the building cell centre on the floor (0, 0, 0), the hearth against the +Y edge whose
wall's inner face is at Y 130. The breast spans X -85..85 (lintel -90..90), Y 66..130, Z 0..258;
the hearthstone X -96..96, Y 40..129. Fire opening X -56..56, Z 3..108; firebox back at Y 122.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "StoneHearth"
DESCRIPTION = ("Standing room hearth: an open granite kitchen fireplace with dressed jambs, one lintel, a rubble "
               "chimney breast, sooted firebox, ash bed, wrought-iron firedogs and charred split logs; LOD1/LOD2 "
               "(original). Pivot = cell centre on the floor; its back meets the wall's inner face at Unreal Y 130.")
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
# Turned to face the review camera (it looks from Unreal +Y, the wall side).
BEAUTY = {"pose": (0, 0, 180), "focus": (0.0, 0.9, 0.45), "views": ["hero", "detail", "eye"],
          "eye_distance": 3.2}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Cell centre on the floor (0, 0, 0)",
          "extent_cm": {"x": [-96, 96], "y": [40, 130], "z": [-3, 258]},
          "fire_cm": {"opening_x": [-56, 56], "opening_z": [3, 108], "logs_centre": [0, 100, 20]},
          "placement": ("Rotated with its piece (yaw 0/90/180/270 about the cell centre) so its back meets the "
                        "wall on that edge. Put the flame and point light at the logs' centre.")}


def _modules():
    from stone_building import hearth, masonry
    if not bpy.app.background:
        for module in (masonry, hearth):
            importlib.reload(module)
    return masonry, hearth


def build(kit):
    masonry, hearth = _modules()
    obj = hearth.make(kit, "SM_StoneHearth")
    masonry.unwrap(obj, shrink={hearth.MAT_MORTAR: 0.4})
    meshes = masonry.finish(kit, obj)
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
