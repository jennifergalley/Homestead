"""Derelict cleft-oak post-and-rail fence plus a broken field gateway.

Real-object research:
- Mid-19th-century Cornish farm boundaries often combined granite hedges with cheap local oak or
  cleft chestnut/hazel post-and-rail. Cleft rails are triangular/waney, tenoned into mortised
  posts, and weather to mid silver-grey rather than white; end grain and soil-contact areas rot
  dark brown-black. Lichens colonise the top and rain side first, while green moss collects near
  the ground line.
- A field gate of the period is a five-bar ledged-and-braced timber gate hung from forged strap
  hinges on a heavy hanging post. If the top hinge tears out, the gate pivots on the bottom hinge,
  swings open, drops at the head/latch end, and the lower bars fail first where livestock lean on
  them.

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Pivots: posts/gateway at ground-line base; rails at bottom centre. Blender (x,y,z) imports into
Unreal as (x,-y,z). Rails and mortises run along local X.
"""
import importlib.util
import math
import os
import random
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Vector

_spec = importlib.util.spec_from_file_location("farm_common", Path(__file__).with_name("farm") / "common.py")
common = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(common)

NAME = "FarmFence"
DESCRIPTION = ("Modular derelict cleft-oak post-and-rail fence pieces and a broken sagging five-bar "
               "field gateway for an overgrown Cornish manor farm enclosure.")
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {
    "pose": (0, 0, 0),
    "focus": (0.0, -0.03, 0.70),
    "views": ["hero", "detail", "eye"],
    "meshes": {
        "SM_FarmFenceRail": {"pose": (0, 0, 0), "focus": (0.30, 0.0, 0.05), "eye_distance": 2.2},
        "SM_FarmFenceRailBroken": {"pose": (0, 0, 0), "focus": (0.62, 0.0, 0.05), "eye_distance": 1.8},
        "SM_FarmGateway": {"focus": (1.45, -0.55, 0.62), "eye_distance": 4.0},
    },
}
REPORT = {
    "unreal_frame": "Blender (x,y,z) imports as Unreal (x,-y,z); all dimensions below in cm after import.",
    "slot_heights_cm": [35, 70, 105],
    "rail_axis": "Fence rails and post mortises run along local X.",
    "gateway": ("Hanging post is at local origin; shutting post is at +310 cm X. Gate leaf spans about "
                "+18 to +303 cm X before yaw, is swung 25 degrees toward Blender -Y (Unreal +Y) from "
                "the hanging post, and the latch/head end is dropped until the stile foot rests on z=0."),
}
SEED = 185109


def _materials(kit):
    oak = common.weathered_oak(kit, "M_FarmFenceSilverGreyOak", seed=1.0, lichen=0.30, grime=0.30)
    g = kit.mats.Graph("M_FarmFencePostSilverGreyOak")
    p = g.coord()
    x, y, z = g.separate(p)
    streak = g.noise(g.combine(g.math("MULTIPLY", x, 20.0), g.math("MULTIPLY", y, 20.0),
                               g.math("MULTIPLY", z, 1.6)), scale=8.0, detail=9.0,
                     roughness=0.58).outputs["Fac"]
    checks = g.voronoi(g.combine(g.math("MULTIPLY", x, 0.8), g.math("MULTIPLY", y, 0.8),
                                 g.math("MULTIPLY", z, 0.22)), scale=34.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(checks, 0.0, 0.010, 0.55, 0.0)
    post_color = g.ramp(streak, [(0.18, (0.130, 0.126, 0.110)), (0.62, (0.205, 0.198, 0.174)),
                                 (0.94, (0.275, 0.265, 0.230))])
    post_color = g.mix(post_color, (0.060, 0.052, 0.041), crack)
    lich = g.noise(p, scale=10.0, detail=5.0, roughness=0.60).outputs["Fac"]
    post_color = g.mix(post_color, (0.18, 0.21, 0.13), g.remap(lich, 0.68, 0.85, 0.0, 0.22))
    g.set("Base Color", post_color)
    g.set("Roughness", 0.94)
    g.set("Normal", g.bump(g.math("SUBTRACT", streak, g.math("MULTIPLY", crack, 0.75)),
                           strength=0.55, distance=0.0020))
    post_oak = g.mat
    oak_dark = common.weathered_oak(kit, "M_FarmFenceRainDarkenedOak", seed=2.0, lichen=0.18, grime=0.60)
    rot = common.rotten_wood(kit, "M_FarmFenceSoftRottenHeart", seed=3.0)
    moss = common.moss_lichen(kit, "M_FarmFenceMossLichen", seed=4.0)
    rust = common.rusted_iron(kit, "M_FarmFenceRustedStrapIron", seed=5.0)
    soil = kit.material("M_FarmFenceWetSoilInCracks", (0.045, 0.036, 0.026), roughness=0.98)
    common.zero_subsurface(oak, post_oak, oak_dark, rot, moss, rust, soil)
    return dict(oak=oak, post=post_oak, dark=oak_dark, rot=rot, moss=moss, rust=rust, soil=soil)


def _mortise_liner(kit, name, z, mats, width_x=0.152, slot_y=0.074, slot_z=0.044):
    parts = []
    # Four dark internal walls in the through-mortise; opening passes along local X.
    xh, yh, zh = width_x / 2 + 0.003, slot_y / 2, slot_z / 2
    parts.append(common.box_panel(kit, f"{name}_MortiseTop_{z:.2f}", (0, 0, z + zh), ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
                                  (xh, yh, 0.003), mats["rot"]))
    parts.append(common.box_panel(kit, f"{name}_MortiseBottom_{z:.2f}", (0, 0, z - zh), ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
                                  (xh, yh, 0.003), mats["rot"]))
    parts.append(common.box_panel(kit, f"{name}_MortiseLeft_{z:.2f}", (0, -yh, z), ((1, 0, 0), (0, 0, 1), (0, 1, 0)),
                                  (xh, zh, 0.003), mats["rot"]))
    parts.append(common.box_panel(kit, f"{name}_MortiseRight_{z:.2f}", (0, yh, z), ((1, 0, 0), (0, 0, 1), (0, 1, 0)),
                                  (xh, zh, 0.003), mats["rot"]))
    return parts


def _base_moss(kit, rng, radius_x, radius_y, max_z, mats, prefix):
    parts = []
    for i in range(10 if not DRAFT else 4):
        a = rng.uniform(0, math.tau)
        r = rng.uniform(0.55, 1.0)
        z = rng.uniform(0.012, max_z)
        parts.append(kit.sphere(f"{prefix}_MossPad_{i}", rng.uniform(0.010, 0.030),
                                location=(math.cos(a) * radius_x * r, math.sin(a) * radius_y * r, z),
                                material=mats["moss"], segments=8, rings=4, scale=(1.0, 0.65, 0.20)))
    return parts


def _volume(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    volume = bm.calc_volume(signed=True)
    bm.free()
    return volume


def _outward(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bm.calc_volume(signed=True) < 0.0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


MORTISE_MAX_REMOVED_M3 = 0.0010  # One 7.6 x 4.6 cm through-mortise in a ~13 cm post removes ~0.00045 m3.


def _mortise_cuts(body, heights, size, name):
    """Boolean all through-mortises with one joined cutter and verify the post survived.

    Cutting them one at a time let Blender's exact solver delete the whole lofted body at the
    second cut, which shipped SM_FarmFencePost as floating mortise liners with no post (rails then
    appeared to hover in the estate)."""
    before = _volume(body)
    cubes = []
    for z in heights:
        bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, z))
        cube = bpy.context.object
        cube.dimensions = size
        cubes.append(cube)
    bpy.ops.object.select_all(action="DESELECT")
    for cube in cubes:
        cube.select_set(True)
    bpy.context.view_layer.objects.active = cubes[0]
    bpy.ops.object.join()
    cutter = bpy.context.object
    cutter.name = name
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = body
    backup = body.data.copy()
    try:
        # The exact solver collapses this seed's post; the float solver keeps it.
        for solver in ("EXACT", "FLOAT"):
            mod = body.modifiers.new(name, "BOOLEAN")
            mod.operation = "DIFFERENCE"
            mod.object = cutter
            mod.solver = solver
            bpy.ops.object.modifier_apply(modifier=mod.name)
            removed = before - _volume(body)
            if 0.0 < removed < MORTISE_MAX_REMOVED_M3 * len(heights):
                return
            print(f"HOMESTEAD_MORTISE_RETRY {name} solver={solver} removed={removed:.5f}")
            broken = body.data
            body.data = backup.copy()
            bpy.data.meshes.remove(broken)
    finally:
        bpy.data.objects.remove(cutter, do_unlink=True)
        bpy.data.meshes.remove(backup)
    raise RuntimeError(f"{name}: every mortise boolean destroyed the {before:.5f} m3 post body")

def _post_core(kit, name, height, width_x, depth_y, mats, seed, snapped=False):
    rng = random.Random(seed)
    points = []
    rows = 22 if not DRAFT else 14
    top = height
    for i in range(rows):
        u = i / (rows - 1)
        lean = Vector((0.012 * math.sin(u * 2.8 + seed), 0.010 * math.sin(u * 2.2 + seed * 0.2), top * u))
        if snapped and u > 0.70:
            lean.z += 0.035 * math.sin((u - 0.70) * math.pi * 2.0 + seed)
        points.append(tuple(lean))
    taper = lambda u: 1.0 - 0.11 * u + 0.03 * math.sin(u * math.tau + seed)
    body = common.beam(kit, f"{name}_HewnBody", points, width_x, depth_y, mats.get("post", mats["oak"]), seed,
                       spacing=0.060, section="rect", taper=taper, roll=math.radians(rng.uniform(-6, 6)))
    if not snapped:
        # The vertical loft can come out inside-out; an inside-out body makes the exact boolean keep
        # only the cutter slabs and drop the whole post (SM_FarmFencePost shipped that way).
        _outward(body)
        _mortise_cuts(body, (0.35, 0.70, 1.05), (width_x + 0.05, 0.076, 0.046), f"{name}_MortiseCuts")
        # Boolean mortise cuts can leave generated faces without the rest-coordinate attribute
        # that the oak shader needs; retag to keep the silvered grain from baking black.
        kit.tag_coords(body.data)
        body.data.materials.clear()
        body.data.materials.append(mats.get("post", mats["oak"]))
        for poly in body.data.polygons:
            poly.material_index = 0
    else:
        # Broad ragged rot scar in the broken stump top.
        pass
    parts = [body]
    # Real geometry for deep checks and rot seams.
    check_count = 4 if snapped and not DRAFT else (6 if not DRAFT else 3)
    for i in range(check_count):
        z = rng.uniform(0.16, top * (0.92 if not snapped else 0.72))
        side = rng.choice([Vector((1, 0, 0)), Vector((-1, 0, 0)), Vector((0, 1, 0)), Vector((0, -1, 0))])
        tangent = Vector((0, 0, 1))
        face_side = Vector((0, 1, 0)) if abs(side.x) > 0.5 else Vector((1, 0, 0))
        centre = side * (width_x * 0.53 if abs(side.x) > 0.5 else depth_y * 0.53) + Vector((0, 0, z))
        parts.append(common.dark_crack(kit, f"{name}_OpenSeasoningCheck_{i}", centre, tangent, face_side, side,
                                       rng.uniform(0.18, 0.62), rng.uniform(0.006, 0.014), mats["rot"]))
    if not snapped:
        for z in (0.35, 0.70, 1.05):
            parts.extend(_mortise_liner(kit, name, z, mats, width_x=width_x))
    # Weathered cap: uneven rotten crown and lichen mats.
    cap_z = height + (0.010 if not snapped else 0.000)
    top_count = 4 if snapped and not DRAFT else (8 if not DRAFT else 4)
    for i in range(top_count):
        sx = rng.uniform(-width_x * 0.38, width_x * 0.38)
        sy = rng.uniform(-depth_y * 0.38, depth_y * 0.38)
        h = rng.uniform(0.015, 0.070 if snapped else 0.040)
        parts.append(common.make_splinter(kit, f"{name}_SoftTopSplinter_{i}",
                                          (sx, sy, cap_z - h * 0.4), (rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 1),
                                          (1, 0, 0), (0, 1, 0), rng.uniform(0.006, 0.018),
                                          rng.uniform(0.004, 0.010), h, mats["rot"] if snapped or rng.random() < 0.45 else mats["dark"],
                                          seed + 100 + i, rows=4))
    parts.extend(_base_moss(kit, rng, width_x * 0.56, depth_y * 0.60, 0.12, mats, name))
    if not snapped:
        for i in range(5 if not DRAFT else 2):
            parts.append(kit.sphere(f"{name}_TopLichen_{i}", rng.uniform(0.007, 0.020),
                                    location=(rng.uniform(-width_x * 0.38, width_x * 0.38),
                                              rng.uniform(-depth_y * 0.38, depth_y * 0.38),
                                              height + rng.uniform(0.002, 0.022)),
                                    material=mats["moss"], segments=8, rings=4, scale=(1.2, 0.8, 0.24)))
    return parts


def build_post(kit, mats):
    parts = _post_core(kit, "FarmFencePost", 1.30, 0.140, 0.110, mats, SEED + 1, snapped=False)
    obj = kit.join(parts, "SM_FarmFencePost", pivot="base", unwrap=True, reshade=True, smooth_angle=48)
    # Preserve the three mortise heights while taming stray rotten fibres so the authored post
    # stays the requested ~1.30 m rather than gaining a tall silhouette spike.
    for v in obj.data.vertices:
        if v.co.z > 1.31:
            v.co.z = 1.31 + (v.co.z - 1.31) * 0.18
    obj.data.update()
    return obj


def build_snapped_post(kit, mats):
    parts = _post_core(kit, "FarmFencePostSnapped", 0.38, 0.142, 0.112, mats, SEED + 2, snapped=True)
    # Torn dark rotten broken crown flares wider than the post.
    rng = random.Random(SEED + 20)
    for i in range(5 if not DRAFT else 4):
        angle = rng.uniform(0, math.tau)
        base = Vector((math.cos(angle) * rng.uniform(0.025, 0.060), math.sin(angle) * rng.uniform(0.022, 0.050), 0.345))
        parts.append(common.make_splinter(kit, f"FarmFencePostSnapped_RaggedBreak_{i}", base,
                                          (0.18 * math.cos(angle), 0.18 * math.sin(angle), rng.uniform(0.40, 1.0)),
                                          (-math.sin(angle), math.cos(angle), 0), (0, 0, 1),
                                          rng.uniform(0.008, 0.024), rng.uniform(0.005, 0.014),
                                          rng.uniform(0.035, 0.110), mats["rot"], SEED + 260 + i))
    obj = kit.join(parts, "SM_FarmFencePostSnapped", pivot="base", unwrap=True, reshade=True, smooth_angle=48)
    top = max(v.co.z for v in obj.data.vertices)
    if top > 0.49:
        scale = 0.49 / top
        for v in obj.data.vertices:
            v.co.z *= scale
        obj.data.update()
    mod = obj.modifiers.new("BudgetDecimate", "DECIMATE")
    mod.ratio = 0.72
    mod.use_collapse_triangulate = True
    kit.apply_modifiers(obj)
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(48))
    return obj


def _rail_path(length, seed, broken=False):
    rng = random.Random(seed)
    end = length * 0.5
    points = []
    count = 14 if not DRAFT else 8
    for i in range(count):
        u = i / (count - 1)
        x = -end + length * u
        points.append((x, 0.012 * math.sin(u * math.pi * 1.2 + seed) + rng.uniform(-0.004, 0.004),
                       0.070 + 0.025 * math.sin(u * math.pi) + 0.010 * math.sin(u * 7.0 + seed)))
    if broken:
        points[-1] = (end, points[-1][1] + 0.010, points[-1][2] - 0.030)
    return points


def build_rail(kit, mats):
    parts = []
    length = 2.90
    taper = lambda u: 0.60 if u < 0.045 or u > 0.955 else (0.60 + 0.40 * min(common.smoothstep(0.045, 0.085, u), common.smoothstep(0.955, 0.915, u)))
    rail = common.beam(kit, "FarmFenceRail_CleftOakRail", _rail_path(length, SEED + 30), 0.090, 0.060,
                       mats["oak"], SEED + 30, spacing=0.090, section="tri", taper=taper)
    parts.append(rail)
    rng = random.Random(SEED + 31)
    for i in range(12 if not DRAFT else 4):
        x = rng.uniform(-1.25, 1.25)
        side = rng.choice([Vector((0, 1, 0)), Vector((0, -1, 0)), Vector((0, 0, 1))])
        parts.append(common.dark_crack(kit, f"FarmFenceRail_LongCheck_{i}", (x, 0.032 * side.y, 0.095 + 0.032 * side.z),
                                       (1, 0, 0), (0, 0, 1), side, rng.uniform(0.22, 0.72),
                                       rng.uniform(0.003, 0.008), mats["rot"]))
    for i in range(18 if not DRAFT else 5):
        parts.append(kit.sphere(f"FarmFenceRail_Lichen_{i}", rng.uniform(0.004, 0.015),
                                location=(rng.uniform(-1.35, 1.35), rng.uniform(-0.035, 0.035), rng.uniform(0.095, 0.130)),
                                material=mats["moss"], segments=8, rings=4, scale=(1.2, 0.8, 0.25)))
    return kit.join(parts, "SM_FarmFenceRail", pivot="base", unwrap=True, reshade=True, smooth_angle=46)


def build_broken_rail(kit, mats):
    parts = []
    length = 1.58
    taper = lambda u: 0.58 if u < 0.075 else 1.0 - 0.18 * common.smoothstep(0.78, 1.0, u)
    rail = common.beam(kit, "FarmFenceRailBroken_CleftOak", _rail_path(length, SEED + 44, True),
                       0.090, 0.060, mats["oak"], SEED + 44, spacing=0.070, section="tri", taper=taper, cap_end=True)
    parts.append(rail)
    rng = random.Random(SEED + 45)
    break_x = length * 0.5
    for i in range(22 if not DRAFT else 8):
        off = Vector((0, rng.uniform(-0.026, 0.030), rng.uniform(0.040, 0.102)))
        parts.append(common.make_splinter(kit, f"FarmFenceRailBroken_TornFibre_{i}", (break_x - 0.025, off.y, off.z),
                                          (1, rng.uniform(-0.30, 0.30), rng.uniform(-0.18, 0.28)),
                                          (0, 1, 0), (0, 0, 1), rng.uniform(0.006, 0.022),
                                          rng.uniform(0.004, 0.012), rng.uniform(0.08, 0.22),
                                          mats["rot"] if rng.random() < 0.35 else mats["dark"], SEED + 450 + i))
    for i in range(8 if not DRAFT else 3):
        parts.append(common.dark_crack(kit, f"FarmFenceRailBroken_Check_{i}", (rng.uniform(-0.65, 0.55), 0.032, 0.09),
                                       (1, 0, 0), (0, 0, 1), (0, 1, 0), rng.uniform(0.18, 0.52),
                                       rng.uniform(0.003, 0.008), mats["rot"]))
    return kit.join(parts, "SM_FarmFenceRailBroken", pivot="base", unwrap=True, reshade=True, smooth_angle=46)


# Left swung wide open for years: the dropped head rests in the turf clear of the gateway.
GATE_YAW_DEG = -102.0


def _gate_transform(local, yaw=math.radians(GATE_YAW_DEG), hinge=(0.18, 0.0), drop=0.24):
    x, y, z = local
    c, s = math.cos(yaw), math.sin(yaw)
    # Latch end sags smoothly while the bottom hinge remains engaged.
    sag = drop * common.smoothstep(0.15, 2.85, x)
    gx = hinge[0] + x * c - y * s
    gy = hinge[1] + x * s + y * c
    return (gx, gy, z - sag)


def _gate_beam(kit, name, a, b, width, depth, mat, seed):
    pts = []
    rng = random.Random(seed)
    for i in range(8 if not DRAFT else 5):
        u = i / ((8 if not DRAFT else 5) - 1)
        p = Vector(a).lerp(Vector(b), u)
        p.y += rng.uniform(-0.006, 0.006)
        pts.append(_gate_transform(tuple(p)))
    return common.beam(kit, name, pts, width, depth, mat, seed, spacing=0.055, section="cleft")


def build_gateway(kit, mats):
    rng = random.Random(SEED + 90)
    parts = []
    # Heavy hanging and shutting posts.
    hp = _post_core(kit, "FarmGateway_HangingPost", 1.50, 0.205, 0.185, mats, SEED + 91, snapped=False)
    parts.extend(hp)
    sp = _post_core(kit, "FarmGateway_ShuttingPost", 1.34, 0.165, 0.145, mats, SEED + 92, snapped=False)
    for p in sp:
        p.location.x += 3.10
    parts.extend(sp)
    # Gate timber: five rails, stiles and a diagonal brace, with lower bar broken.
    rail_heights = [0.22, 0.42, 0.62, 0.84, 1.08]
    for idx, z in enumerate(rail_heights):
        if idx == 1:
            parts.append(_gate_beam(kit, f"FarmGateway_SnappedLowerBarInboard", (0.10, 0.0, z), (1.70, 0.0, z + 0.015),
                                    0.075, 0.052, mats["oak"], SEED + 100 + idx))
            parts.append(_gate_beam(kit, f"FarmGateway_SnappedLowerBarOutboard", (1.84, -0.035, z - 0.05), (2.85, -0.04, z - 0.015),
                                    0.070, 0.048, mats["dark"], SEED + 110 + idx))
            bx = _gate_transform((1.75, 0, z))
            _, side, up = common.frame((1, -0.15, 0.03), 0)
            for k in range(10 if not DRAFT else 5):
                parts.append(common.make_splinter(kit, f"FarmGateway_LowerBarSplinter_{k}", bx,
                                                  (rng.uniform(-0.2, 0.8), rng.uniform(-0.8, 0.3), rng.uniform(-0.4, 0.4)),
                                                  side, up, rng.uniform(0.005, 0.018), rng.uniform(0.003, 0.010),
                                                  rng.uniform(0.05, 0.16), mats["rot"], SEED + 600 + k))
            continue
        width = 0.095 if idx == 4 else 0.070
        depth = 0.060 if idx == 4 else 0.046
        parts.append(_gate_beam(kit, f"FarmGateway_Bar_{idx}", (0.10, 0.0, z), (2.88, 0.0, z + 0.010 * math.sin(idx)),
                                width, depth, mats["oak"], SEED + 100 + idx))
    for x, label, w in ((0.10, "HeelStile", 0.082), (1.45, "MiddleStile", 0.065), (2.88, "HeadStile", 0.078)):
        parts.append(_gate_beam(kit, f"FarmGateway_{label}", (x, 0.0, 0.08), (x, 0.0, 1.16),
                                w, 0.060, mats["oak"], SEED + 130 + int(x * 100)))
    parts.append(_gate_beam(kit, "FarmGateway_DiagonalBrace", (0.20, 0.005, 0.23), (2.78, 0.005, 1.04),
                            0.074, 0.050, mats["oak"], SEED + 150))
    # Iron straps: bottom hinge intact, top hinge torn off and rusty.
    for z, intact in ((0.26, True), (1.05, False)):
        gate_a = _gate_transform((0.02, -0.036, z))
        gate_b = _gate_transform((0.62 if intact else 0.38, -0.036, z + (-0.05 if not intact else 0.0)))
        parts.append(common.box_panel(kit, f"FarmGateway_HingeStrap_{z:.2f}", Vector(gate_a).lerp(Vector(gate_b), 0.5),
                                      ((Vector(gate_b) - Vector(gate_a)), (0, 0, 1), (0, 1, 0)),
                                      ((Vector(gate_b) - Vector(gate_a)).length / 2, 0.018, 0.0035), mats["rust"]))
        parts.append(kit.cylinder(f"FarmGateway_HingePin_{z:.2f}", 0.014, 0.17 if intact else 0.08,
                                  location=(-0.115, -0.090, z), material=mats["rust"], sides=12, bevel=0.002))
        if not intact:
            # Empty torn upper staple on the post.
            parts.append(common.box_panel(kit, "FarmGateway_TornUpperStaple", (-0.115, -0.095, z + 0.02),
                                          ((1, 0, 0), (0, 0, 1), (0, 1, 0)), (0.045, 0.010, 0.004), mats["rust"]))
    # Latch plate on shutting post and dirt rubbed where the head has dragged.
    parts.append(common.box_panel(kit, "FarmGateway_RustedLatchKeeper", (3.02, -0.080, 0.76),
                                  ((1, 0, 0), (0, 0, 1), (0, 1, 0)), (0.050, 0.060, 0.004), mats["rust"]))
    # Mud scuffed along the arc the head dragged through as it swung open.
    for i in range(14 if not DRAFT else 6):
        a = math.radians(rng.uniform(-30.0, GATE_YAW_DEG + 4.0))
        r = rng.uniform(2.55, 3.0)
        parts.append(kit.sphere(f"FarmGateway_DraggedMud_{i}", rng.uniform(0.008, 0.030),
                                location=(0.18 + r * math.cos(a), r * math.sin(a), rng.uniform(0.004, 0.016)),
                                material=mats["soil"], segments=8, rings=4, scale=(1.4, 0.8, 0.14)))
    # Keep the authored origin at the base centre of the hanging post, not the overall gate bbox.
    obj = kit.join(parts, "SM_FarmGateway", pivot=None, unwrap=True, reshade=True, smooth_angle=45)
    for v in obj.data.vertices:
        if v.co.z > 1.54:
            v.co.z = 1.54 + (v.co.z - 1.54) * 0.12
    obj.data.update()
    return obj


def build(kit):
    mats = _materials(kit)
    return [
        build_post(kit, mats),
        build_snapped_post(kit, mats),
        build_rail(kit, mats),
        build_broken_rail(kit, mats),
        build_gateway(kit, mats),
    ]


def after_bake(kit, obj):
    if obj.name != "SM_FarmFencePost":
        return
    mat = obj.material_slots[0].material
    image = None
    for node in mat.node_tree.nodes:
        if node.bl_idname == "ShaderNodeTexImage" and node.image and node.image.name.startswith("T_FarmFencePost_basecolor"):
            image = node.image
            break
    if image is None:
        return
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    px = pixels.reshape((-1, 4))
    rgb = px[:, :3]
    lum = rgb @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)
    mask = lum > 0.025
    # The boolean-mortised post is dominated by dark generated faces; lift only occupied
    # non-black texels back to rain-weathered oak while retaining cracks and rot.
    rgb[mask] = np.clip(rgb[mask] * 1.85 + 0.055, 0.0, 0.72)
    image.pixels.foreach_set(pixels)
    image.save()
    image.reload()
