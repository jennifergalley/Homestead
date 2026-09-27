"""Scavengeable deer remains: a yearling mule-deer buck that died last winter, lying on
its right side in oak leaf litter (SM_DeerRemains), and the same skeleton after its
hide has been taken (SM_DeerBones), for the heroine to scavenge hides for winter
clothing. Tasteful for a cozy game: no blood, gore or exposed flesh, only dried hide,
fur and clean weathered bone.

Real-object research (written before modeling):
- Winter-killed deer are cleaned by scavengers (coyotes, ravens, vultures) and insects
  within weeks of the thaw. What stays is dried rawhide with its fur, stiff over the
  ribs and hips and collapsed flat where the belly was, torn open along the belly and
  over the chest, pulled back from the neck; lower legs keep their furred skin down to
  the hooves; the skull is bare, the jaw often dragged a little way off; a buck that
  died after late-winter shedding keeps one antler or none (the other lies nearby).
- After a year or two the bones bleach grey-white where the sun reaches, crack along
  the grain of the long bones and flake, and grow green algae and moss where they touch
  the damp ground (Behrensmeyer weathering stages 1-3).
- Mule deer: skull ~26 cm; forked-horn yearling antlers ~25 cm; body length withers to
  rump ~70 cm, 13 rib pairs up to 30 cm; winter coat grizzled grey-brown with a dark
  dorsal line, cream belly, white rump patch and a black-tipped white tail.
- Sierra foothill woodland floor: canyon live oak (small, leathery) and California black
  oak (large, lobed) leaves, cushion moss.

Rigid static meshes; units are meters, Z up. PIVOT: ground centre (the XY centre of the
remains' footprint, z = 0 on the ground; a few millimetres of moss and bone sink below
it). The spine runs along X (skull toward -X, tail +X), the back toward +Y and the legs
toward -Y. SM_DeerRemains and SM_DeerBones share the pivot, axes and footprint
(~1.5 x 0.7 m), so one swaps in place for the other. Each has LOD1 and LOD2 sharing its
UVs and texture set. No collision: the game makes them non-blocking.
Everything is generated here (signed-distance bones meshed with OpenVDB, a solved hide
membrane, swept ribs, antlers and leaves, procedural materials baked to textures): no
scanned or downloaded geometry or textures.
"""
import importlib
import math
import os
import sys
from pathlib import Path

import bpy
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "DeerRemains"
DESCRIPTION = ("Winter-killed mule-deer remains in oak litter: dried furred hide over the skeleton "
               "(SM_DeerRemains) and the bleached, mossy bones after the hide is taken (SM_DeerBones); "
               "LOD1/LOD2 each (original). Pivot = ground centre; skull -X, back +Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 80000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"meshes": {}}
LOD_TRIS = (18000, 6000)
NOTES = {}
REPORT = {}


def _modules():
    from deer import common, hide, litter, materials, skeleton
    if not bpy.app.background:
        for module in (common, skeleton, materials, hide, litter):
            importlib.reload(module)
    return common, skeleton, materials, hide, litter


def material_sets(M):
    remains = {"bone": M.bone("M_DeerRemainsBone", bleach=0.3, moss=0.12, cracks=0.3, seed=1.0),
               "antler": M.antler("M_DeerRemainsAntler", bleach=0.25, seed=2.0),
               "hoof": M.hoof("M_DeerRemainsHoof", seed=3.0),
               "teeth": M.teeth("M_DeerRemainsTeeth"),
               "fur": M.fur("M_DeerRemainsFur", zones=True, seed=4.0, rump_from=None),
               "fur_leg": M.fur("M_DeerRemainsFurLeg", zones=False, seed=5.0, length=0.4,
                                pale=(0.2, 0.16, 0.12)),
               "fur_tail": M.fur("M_DeerRemainsFurTail", zones=False, seed=6.0, length=0.3,
                                 dark=(0.12, 0.1, 0.08), pale=(0.36, 0.33, 0.28), tip_from=0.14),
               "leather": M.leather("M_DeerRemainsLeather", seed=7.0),
               "leaf": M.leaf("M_DeerRemainsLeaf"),
               "moss": M.moss("M_DeerRemainsMoss", seed=8.0)}
    bones = {"bone": M.bone("M_DeerBonesBone", bleach=0.92, moss=0.6, cracks=0.85, seed=11.0),
             "antler": M.antler("M_DeerBonesAntler", bleach=0.7, seed=12.0),
             "hoof": M.hoof("M_DeerBonesHoof", seed=13.0),
             "teeth": M.teeth("M_DeerBonesTeeth"),
             "leaf": M.leaf("M_DeerBonesLeaf"),
             "moss": M.moss("M_DeerBonesMoss", seed=18.0)}
    for mats in (remains, bones):
        for mat in mats.values():
            mat.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value = 0.0
    return remains, bones


LEG_BONES = ("_humerus", "_radius", "_femur", "_tibia", "_cannon", "_p1", "_p2", "_calcaneus")
DENSITY = {"Bone": 1.3, "Antler": 1.3, "Teeth": 0.9, "Hoof": 0.9, "Fur": 1.0, "FurLeg": 1.0, "FurTail": 1.0,
           "Leather": 0.35, "Leaf": 0.7, "Moss": 0.6}


def unwrap_part(obj, margin=0.004):
    """Few large, relaxed islands for one un-joined part: a decimated SDF surface would
    smart-project into thousands of splinters, so find islands on a smoothed copy
    (``homestead_rocks.unwrap``), turn them into seams and relax them with LSCM."""
    import homestead_rocks as rocks
    if len(obj.data.polygons) < 60:
        return obj
    rocks.unwrap(obj, angle=66.0, margin=margin, method="ANGLE_BASED")
    return obj


def tube_uvs(obj):
    """Cylindrical UVs from a swept part's pcoord (angle x mean radius, arclength): one
    clean island per rib, antler tine or stocking."""
    import numpy as np
    mesh = obj.data
    if "pcoord" not in mesh.attributes:
        return unwrap_part(obj)
    raw = np.empty(len(mesh.vertices) * 3)
    mesh.attributes["pcoord"].data.foreach_get("vector", raw)
    pts = raw.reshape(-1, 3)
    radius = float(np.mean(np.hypot(pts[:, 0], pts[:, 1]))) or 0.005
    uv = mesh.uv_layers["UVMap"].data
    for poly in mesh.polygons:
        vs = pts[list(poly.vertices)]
        angles = np.arctan2(vs[:, 1], vs[:, 0])
        if angles.max() - angles.min() > math.pi:
            angles = np.where(angles < 0, angles + 2 * math.pi, angles)
        cap = np.ptp(vs[:, 2]) < 1e-7 and len(poly.vertices) > 2 and np.allclose(vs[:, 2], vs[0, 2])
        for loop, p, a in zip(poly.loop_indices, vs, angles):
            uv[loop].uv = ((p[0] - 4 * radius, p[1]) if cap else (a * radius, p[2]))
    return obj


def material_key(name):
    return name.split("Remains")[-1].split("Bones")[-1].lstrip("_")


def pack(obj, margin=0.0022):
    """Equalise texel density across the joined parts (keeping each island), weight it by
    material (skull and antlers up, hidden flesh side of the hide down) and pack."""
    mesh = obj.data
    names = [slot.material.name for slot in obj.material_slots]
    bpy.context.view_layer.objects.active = obj
    for other in bpy.context.scene.objects:
        other.select_set(other is obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = mesh.uv_layers["UVMap"].data
    for poly in mesh.polygons:
        factor = DENSITY.get(material_key(names[poly.material_index]), 1.0)
        if factor != 1.0:
            for loop in poly.loop_indices:
                uv[loop].uv = (uv[loop].uv[0] * factor, uv[loop].uv[1] * factor)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, margin=margin)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj

def lods(kit, obj, targets):
    out = []
    total = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    for index, target in enumerate(targets, start=1):
        copy = obj.copy()
        copy.data = obj.data.copy()
        copy.name = copy.data.name = f"{obj.name}_LOD{index}"
        bpy.context.scene.collection.objects.link(copy)
        mod = copy.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = min(1.0, target / total)
        mod.use_collapse_triangulate = True
        kit.apply_modifiers(copy)
        out.append(kit.finalize(copy, pivot=None, unwrap=False, reshade=True, smooth_angle=60))
    return out


def build(kit):
    C, S, M, H, L = _modules()
    remains_mats, bones_mats = material_sets(M)
    parts = S.parts_list(kit, remains_mats)

    # SM_DeerBones: a copy of every bone with the bleached, mossy materials.
    bone_parts = []
    for obj, category, group in parts:
        copy = C.copy_object(obj, obj.name + "_B")
        copy.data.materials[0] = bones_mats[category]
        bone_parts.append(copy)
        if obj in S.RAISED:
            # With the hide gone nothing holds the upper left leg up: it lies on the ground.
            C.lay_down([copy])

    # SM_DeerRemains: hide over the skeleton; keep only what it leaves exposed.
    support = [obj for obj, _, group in parts if group in ("body", "neck", "tail")]
    hide, raster = H.body_hide(kit, support, remains_mats["fur"], remains_mats["leather"])
    covering = [hide] + H.leg_stockings(kit, remains_mats["fur_leg"]) + [H.tail(kit, remains_mats["fur_tail"])]
    kept, dropped = [], []
    for obj, category, group in parts:
        if group in ("head", "shed") or (group == "loose" and obj.name.startswith("Rib")):
            keep = True
        elif group in ("tail",) or obj.name == "ScapulaL" or any(k in obj.name for k in LEG_BONES):
            keep = False
        elif group == "leg":
            keep = category == "hoof"
        else:
            keep = H.covered(obj, raster) < 0.8
        (kept if keep else dropped).append(obj)
    for obj in dropped:
        bpy.data.objects.remove(obj)

    bounds = np.concatenate([C.vertices(o) for o in kept + covering + bone_parts])
    region = (bounds[:, 0].min() + 0.03, bounds[:, 0].max() - 0.03, bounds[:, 1].min() + 0.03, bounds[:, 1].max() - 0.03)
    leaves_r = L.scatter_leaves(kit, remains_mats["leaf"], kept + covering, 58, 7001, region, "LeafR")
    hole = S.SPINE.at(S.S_OF["T6"]) + (0.0, -0.125, 0.0)
    leaves_r += L.scatter_leaves(kit, remains_mats["leaf"], [o for o in kept if o.name.startswith("RibR")], 7, 7101,
                                 (hole[0] - 0.06, hole[0] + 0.06, hole[1] - 0.04, hole[1] + 0.04), "LeafHole",
                                 small=True)
    moss_r = L.moss_at_contacts(kit, remains_mats["moss"], [o for o in kept if o.name.startswith(("Skull", "Mandible",
                                                                                                    "RibLoose"))],
                                3, 71, radius=(0.014, 0.028))
    leaves_b = L.scatter_leaves(kit, bones_mats["leaf"], bone_parts, 62, 9001, region, "LeafB")
    moss_b = L.moss_at_contacts(kit, bones_mats["moss"], bone_parts, 10, 91, radius=(0.016, 0.05))

    # Ground-centre pivot shared by both meshes.
    everything = kept + covering + leaves_r + moss_r + bone_parts + leaves_b + moss_b
    v = np.concatenate([C.vertices(o) for o in everything])
    centre = (v[:, :2].min(0) + v[:, :2].max(0)) / 2
    C.shift(everything, (-centre[0], -centre[1], 0.0))

    for obj in kept + covering[1:] + moss_r + bone_parts + moss_b:
        if not obj.name.startswith(("AntlerL", "AntlerShed", "Rib", "Stocking", "Tail")):
            unwrap_part(obj)
        else:
            tube_uvs(obj)
    remains = kit.join(kept + covering + leaves_r + moss_r, "SM_DeerRemains", pivot=None, unwrap=False, reshade=True,
                       smooth_angle=60)
    pack(remains)
    bones = kit.join(bone_parts + leaves_b + moss_b, "SM_DeerBones", pivot=None, unwrap=False, reshade=True,
                     smooth_angle=60)
    pack(bones)

    skull = np.asarray(S.SKULL_ORIGIN) + (-centre[0], -centre[1], 0.0)
    head_focus = tuple(float(c) for c in skull + np.asarray(S.skull_frame()) @ np.array([0.1, 0.0, 0.03]))
    chest = S.SPINE.at(S.S_OF["T6"]) + (-centre[0], -centre[1], 0.0) + (0.0, -0.1, 0.08)
    BEAUTY["meshes"] = {
        "SM_DeerRemains": {"pose": (0, 0, 0), "ground": "origin", "focus": tuple(float(c) for c in chest),
                           "views": ["hero", "detail", "eye"], "eye_distance": 2.6},
        "SM_DeerBones": {"pose": (0, 0, 0), "ground": "origin", "focus": head_focus,
                         "views": ["hero", "detail", "eye"], "eye_distance": 2.6},
    }
    lo, hi = v.min(0), v.max(0)
    NOTES.update({
        "pivot": "Ground centre: XY centre of the footprint, z = 0 on the ground (moss and bone sink a few mm).",
        "axes": "Spine along X (skull -X, tail +X), back +Y, legs -Y; the deer lies on its right side.",
        "swap": "SM_DeerRemains and SM_DeerBones share pivot, axes and footprint: swap in place once the hide is taken.",
        "footprint_cm": [round((hi[0] - lo[0]) * 100, 1), round((hi[1] - lo[1]) * 100, 1)],
        "collision": "None needed (non-blocking in game).",
        "lods": f"LOD1 ~{LOD_TRIS[0]} and LOD2 ~{LOD_TRIS[1]} triangles, decimated from LOD0 and sharing its UVs.",
        "materials": ("One baked 4K texture set per mesh (basecolor, roughness, OpenGL normal, AO; opaque). "
                      "Remains: dried fur hide, rawhide, weathered bone, antler, hoof, teeth, oak leaves, moss. "
                      "Bones: bleached cracked bone with algae and moss, antler, hoof, teeth, leaves, moss."),
    })
    return [remains] + lods(kit, remains, LOD_TRIS) + [bones] + lods(kit, bones, LOD_TRIS)
