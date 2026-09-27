"""Pitch torch in a forged wall sconce (SM_TorchWall) and its burnt-out state
(SM_TorchWall_Spent).

Real-object research (written before modeling):
- Medieval and later wall sconces for torches ("cressets" are the fire baskets; torch
  holders are simpler) are a blacksmith's job in wrought iron: a flat back strap
  30-45 cm long nailed or spiked to the wall with rose-headed nails, finished with a
  scroll or a drawn point, carrying one or two rings on short arms. The torch drops
  through the upper ring and rests in the lower one, leaning out 15-25 degrees so the
  flame and soot clear the wall.
- Square bar arms are often given a decorative twist, fire-welded to the strap. The
  iron is black-brown forge scale with rust blooms, bright only on worn edges.
- The torch itself is a shorter hand torch (60-80 cm): hardwood stick, a head of
  pitch-soaked linen strips wound in overlapping laps with a folded-over top, tied
  under the head; the grip is darkened by handling.

Dimensions and pivot (Unreal local cm): pivot at the sconce's mounting point, the back
of the strap lying on the plane Y = 0 with the wall on +Y (the strap is 0.7 cm thick
toward -Y). The upper ring is at Z 0, 9.5 cm out from the wall; the torch leans 20
degrees out toward -Y and the top of the lit head is 60 cm above the pivot. The flame is
not modelled: the game adds it at ``flameSocket`` (report.json). Everything is generated
here: no scanned or downloaded geometry or textures.
"""
import importlib
import math
import os
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "TorchWall"
DESCRIPTION = ("Pitch torch leaning out of a forged wrought-iron wall sconce (nailed strap, twisted arm, two "
               "rings); plus a burnt-out variant (original). Pivot = mounting point, strap back on Unreal "
               "Y = 0 with the wall on +Y.")
COLLISION = "box"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96, "repack": False,
        "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"],
          "meshes": {"SM_TorchWall": {"focus": (0.0, 0.07, 0.0)},
                     "SM_TorchWall_Spent": {"focus": (0.0, 0.2, 0.42)}}}

LEAN = math.radians(20)
RING = (0.095, 0.0)                  # upper ring centre (Blender y out from the wall, z)
HEAD_TOP_Z = 0.60
LIT = dict(radius=0.016, head_z0=0.56, head_length=0.22, head_radius=0.041, spike=0.0, seed=2)


def _modules():
    from torches import torch
    if not bpy.app.background:
        importlib.reload(torch)
    return torch


def torch_matrix(torch):
    """Torch-local -> asset: lean the +Z axis out toward Blender +Y (Unreal -Y) through the
    upper ring so the lit head top lands at HEAD_TOP_Z."""
    lit_top = torch.Spec(**LIT).top
    s_ring = lit_top - HEAD_TOP_Z / math.cos(LEAN)
    rot = Matrix.Rotation(-LEAN, 4, "X")
    ring = Vector((0.0, RING[0], RING[1]))
    return Matrix.Translation(ring - rot @ Vector((0, 0, s_ring))) @ rot, s_ring


def _one(kit, torch, name, spent):
    spec = torch.Spec(spent=spent, **LIT)
    suffix = "_Spent" if spent else ""
    mats = torch.materials(kit, spec, suffix)
    parts = torch.torch(kit, spec, mats)
    torch.bow(parts, 0.003, spec.top, seed=0.4)
    matrix, s_ring = torch_matrix(torch)
    torch.place(parts, matrix)
    lower = matrix @ Vector((0, 0, 0.05))
    iron = kit.mats.wrought_iron("M_TorchSconce" + suffix, rust=0.45, wear=0.35, seed=1.0)
    parts += torch.sconce(kit, iron, [RING, (lower.y, lower.z)])
    obj = kit.join(parts, name, pivot=None, unwrap=False, reshade=True, smooth_angle=50)
    return obj, torch.head_top(spec, matrix)


def build(kit):
    torch = _modules()
    lit, top = _one(kit, torch, "SM_TorchWall", False)
    spent, spent_top = _one(kit, torch, "SM_TorchWall_Spent", True)
    REPORT["flameSocket"] = {
        n: [round(p.x * 100, 1), round(-p.y * 100, 1), round(p.z * 100, 1)]
        for n, p in (("SM_TorchWall", top), ("SM_TorchWall_Spent", spent_top))}
    lods = torch.finish(kit, lit)
    spent_lods = torch.finish(kit, spent)
    meshes = [lods[0], spent_lods[0]] + lods[1:] + spent_lods[1:]
    print("HOMESTEAD_TORCH tris", {o.name: torch.triangles(o) for o in meshes}, REPORT["flameSocket"])
    return meshes


REPORT = {"unreal_frame": "Sockets and extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
          "pivot": "Sconce mounting point (0, 0, 0): back of the strap on the plane Y = 0, wall on +Y",
          "lean_deg": 20,
          "flameSocket_note": ("Top centre of the wrapped head (lit) or of the burnt stake end (spent), leaning "
                               "20 deg toward -Y. Put the emissive flame and point light here; flames rise "
                               "vertically, so offset the flame volume straight up (+Z) from the socket."),
          "placement": ("On a stone wall's inner face (Unreal wall-local Y 130, rubble proud to ~128): place at "
                        "wall-local (x, 128.5, 140..170) with the wall's yaw so the torch leans into the room.")}
