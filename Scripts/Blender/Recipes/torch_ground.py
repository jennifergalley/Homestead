"""Pitch torch staked into the ground (SM_TorchGround) and its burnt-out state
(SM_TorchGround_Spent).

Real-object research (written before modeling):
- Pre-modern hand torches were a stick of green or seasoned hardwood (hazel, ash, oak)
  3-4 cm thick with the top 15-25 cm wound in strips of old linen or wool and soaked in
  pine pitch, resin or tallow; bundles of resinous splints were the alternative. Staked
  torches (for a yard or path) had the foot whittled to a point and pushed 10-20 cm
  into the ground.
- Wrapping: strips 4-6 cm wide wound upwards with each turn overlapping the last by a
  third, the top strip folded down over the end so the stick doesn't burn through
  first; a tie of twine or wire under the head keeps the wrap from sliding.
- A fresh head is near-black and glossy where the pitch has clotted in the weave, dull
  brown where it soaked thin, with drips run down the stick. After burning out, the head
  is a shrunken, cracked, matte crust of charred cloth with grey-white ash in the weave,
  and the stick end is burnt to a black, split cone; the stick is charred and sooted
  a hand's breadth below the head.

Dimensions and pivot (Unreal local cm): pivot at ground level on the stake axis; the
knife-cut point goes 12 cm below Z 0. Stake 3.4 cm thick; the lit head is ~9 cm across,
Z 88..110 plus a domed top to ~112. The flame is not modelled: the game adds it at
``flameSocket`` (report.json). Everything is generated here: no scanned or downloaded
geometry or textures.
"""
import importlib
import os
import sys
from pathlib import Path

import bpy
from mathutils import Matrix

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "TorchGround"
DESCRIPTION = ("Pitch torch on a knife-pointed hazel stake: pitch-soaked linen head, twine tie, drips; plus a "
               "burnt-out variant (original). Pivot = ground level on the stake axis; point to Z -12 cm.")
COLLISION = "box"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "views": ["hero", "detail"],
          "meshes": {"SM_TorchGround": {"focus": (0.0, 0.0, 1.0)},
                     "SM_TorchGround_Spent": {"focus": (0.0, 0.0, 0.96)}}}

SINK = 0.12          # spike length below ground
LENGTH = 1.22        # head_z0 + head length in torch-local meters (s = 0 at the point)
LIT = dict(radius=0.019, head_z0=1.00, head_length=0.22, head_radius=0.047, spike=0.15, seed=1)


def _modules():
    from torches import torch
    if not bpy.app.background:
        importlib.reload(torch)
    return torch


def _one(kit, torch, name, spent):
    spec = torch.Spec(spent=spent, **LIT)
    mats = torch.materials(kit, spec, "_Spent" if spent else "")
    parts = torch.torch(kit, spec, mats)
    torch.bow(parts, 0.006, spec.top, seed=1.3)
    matrix = Matrix.Translation((0, 0, -SINK))
    torch.place(parts, matrix)
    obj = kit.join(parts, name, pivot=None, unwrap=False, reshade=True, smooth_angle=50)
    top = torch.head_top(spec, matrix)
    return obj, spec, top


def build(kit):
    torch = _modules()
    lit, spec, top = _one(kit, torch, "SM_TorchGround", False)
    spent, spent_spec, spent_top = _one(kit, torch, "SM_TorchGround_Spent", True)
    REPORT["flameSocket"] = {
        "SM_TorchGround": [round(top.x * 100, 1), round(-top.y * 100, 1), round(top.z * 100, 1)],
        "SM_TorchGround_Spent": [round(spent_top.x * 100, 1), round(-spent_top.y * 100, 1),
                                 round(spent_top.z * 100, 1)]}
    lods = torch.finish(kit, lit)
    spent_lods = torch.finish(kit, spent)
    meshes = [lods[0], spent_lods[0]] + lods[1:] + spent_lods[1:]
    print("HOMESTEAD_TORCH tris", {o.name: torch.triangles(o) for o in meshes}, REPORT["flameSocket"])
    return meshes


REPORT = {"unreal_frame": "Sockets and extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Ground level on the stake axis (0, 0, 0); the knife-cut point reaches Z -12",
          "flameSocket_note": ("Top centre of the wrapped head (lit) or of the burnt stake end (spent). Put the "
                               "emissive flame and point light here; the lit head body spans Z ~88..110, "
                               "~9 cm across, so a flame volume centred ~6 cm above the socket reads right."),
          "placement": "Stand upright on terrain; sink up to 12 cm (the point is meant to be buried)."}
