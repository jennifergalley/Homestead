"""Stone building kit: a 3 m wall panel of pointed granite fieldstone (SM_StoneWall).

Real-object research (written before modeling):
- Pre-industrial rural masonry in granite country (Dartmoor longhouses, Cornish and
  Breton farmsteads, Sierra foothill ranch buildings) is "random rubble brought to
  courses": split fieldstones picked for a flat face, laid in rough courses 15-35 cm
  high with the joints broken, bigger stones low, smaller ones pinned in above.
  Walls are 45-75 cm thick in reality; the game kit uses 28 cm so a room stays roomy.
- Corners and openings get the best stones: roughly squared long-and-short quoins,
  often slightly proud of the rubble; openings are spanned by a single granite lintel.
- The joints are pointed with lime mortar (lime putty and sharp sand): near-white when
  fresh, ageing to a dull oatmeal grey, recessed a little behind the stone arrises,
  greened and dirtied near the ground. Granite faces weather to a warm grey rind with
  grey-green crustose lichen, rust blotches round the biotite and a soil splash line.
- The top is finished with a course of long capstones (coping) under the roof plates.

Dimensions and pivot (Unreal local cm; Blender Y is negated, see stone_building/__init__):
pivot at the building cell centre on the floor (0, 0, 0). The wall runs X -150..+150
(+160 at the quoin pier), faces at Y 130 (inner) and 158 (outer), Z -10 (buried plinth)
to 260. Place it with yaw 0/90/180/270 about the cell centre; corners close with the +X
pier (see stone_building/wall.py). Everything is generated here: no scanned or
downloaded geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "StoneWall"
DESCRIPTION = ("Building kit: 3 m wall of pointed granite rubble with a long-and-short quoin pier on its +X "
               "end and a capstone coping; LOD1/LOD2 (original). Pivot = cell centre on the floor; wall "
               "faces at Unreal Y 130/158.")
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (1.35, -1.6, 1.9), "views": ["hero", "detail", "eye"],
          "eye_distance": 4.5}
REPORT = {"unreal_frame": "Extents below are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Cell centre on the floor (0, 0, 0)",
          "extent_cm": {"x": [-150, 160.5], "y": [128, 160.5], "z": [-10, 260]},
          "faces_y_cm": {"inner": 130, "outer": 158, "quoin_pier": [128, 160]},
          "placement": ("Yaw 0/90/180/270 about the cell centre (like the wooden wall). The +X end carries a "
                        "proud quoin pier that fills the corner; the plain -X end tucks inside the neighbour's "
                        "pier. The plinth course below Z 0 is meant to be buried.")}


def _modules():
    from stone_building import masonry, wall
    if not bpy.app.background:
        for module in (masonry, wall):
            importlib.reload(module)
    return masonry, wall


def build(kit):
    masonry, wall = _modules()
    obj = wall.make(kit, "SM_StoneWall", doorway=False, seed=1)
    meshes = masonry.finish(kit, obj)
    print("HOMESTEAD_STONE tris", [masonry.triangles(o) for o in meshes])
    return meshes
