"""Leek crop plot (Allium ampeloprasum var. porrum, Musselburgh / London Flag type), five stages.

Real plant research / art targets (written before modelling):
- Victorian gardeners raised leeks in a seedbed and dibbled pencil-thin transplants into deep
  holes in early summer, earthing them up so the stem blanches; Musselburgh and London Flag were
  the standard hardy winter leeks, lifted from autumn right through the winter.
- The leaves ("flags") are flat, V-keeled straps 3-5 cm wide and 30-50 cm long, a distinctly
  blue-grey-green with a waxy bloom, arranged in one plane as a two-sided fan; each new flag comes
  out of the one before, higher up a green neck. Old outer flags yellow and flop at the tips.
- The edible part is the long white shank (15-20 cm, 3-5 cm thick at ripeness), shading through
  pale yellow-green into the neck, with a tuft of fibrous white roots at the base.
- Homestead crop plot: twelve leeks at x=-0.33,-0.11,+0.11,+0.33 on the ridge tops y=-0.3,0,+0.3,
  base z=0.045, fans turned at varied angles; walk-through.

The plant meshes carry the fans and green necks; the white shank is separate instanced produce
(SM_CropLeek_Produce, standing on the soil line) that the game swells and pushes up as it ripens
from pale green to white.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_crop as C
import homestead_foliage as F
import homestead_shrub as S

NAME = "CropLeek"
DESCRIPTION = ("One 1 m tilled-bed plot of twelve Musselburgh leeks in five stages: blue-green two-sided "
               "fans on green necks; the blanched white shanks are separate instanced produce.")
COLLISION = "none"
TRIANGLE_BUDGET = 5000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1852_303

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 3.8,
          "meshes": {
              "SM_CropLeek_Growing": {"focus": (0.0, 0.0, 0.18), "eye_distance": 3.2},
              "SM_CropLeek_Ripe": {"focus": (0.0, 0.0, 0.25), "eye_distance": 3.6},
              "SM_CropLeek_Produce": {"focus": (0.0, 0.0, 0.05), "eye_distance": 0.6,
                                      "detail_distance": 0.3, "detail_fstop": 32.0},
              "SM_CropLeek_Harvest": {"focus": (0.0, 0.0, 0.0), "eye_distance": 1.4},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / crop height",
             "G": "per-plant phase", "B": "flag flutter 0.05 at the neck -> 1 at the tip", "A": "1"},
    "crop_layout": "Twelve leeks at x=-0.33,-0.11,+0.11,+0.33 on ridges y=-0.3,0,+0.3; base z=0.045 m.",
    "material_notes": "One M_CropLeek atlas: alpha in basecolor; roughness R, translucency G, AO B. "
                      "The shank uses M_CropLeekProduce (re-parented to M_CropProduce in Unreal).",
    "harvest_mesh": "SM_CropLeek_Harvest: whole pulled leek, grip at the neck (origin); shank and roots hang -Z, "
                    "flags rise above.",
}

XS = (-0.33, -0.11, 0.11, 0.33)
UP = C.UP
# neck: where the fan starts above the soil; the shank (produce) covers the neck below that.
STAGES = {
    "Sprout": dict(height=0.14, flags=3, flag_len=0.12, flag_w=0.010, neck=0.020, neck_r=0.0035, old=0),
    "Young": dict(height=0.28, flags=5, flag_len=0.24, flag_w=0.018, neck=0.045, neck_r=0.0065, old=0),
    "Growing": dict(height=0.42, flags=6, flag_len=0.34, flag_w=0.032, neck=0.080, neck_r=0.0095, old=1),
    "Mature": dict(height=0.52, flags=7, flag_len=0.40, flag_w=0.040, neck=0.115, neck_r=0.0120, old=1),
    "Ripe": dict(height=0.56, flags=8, flag_len=0.42, flag_w=0.045, neck=0.135, neck_r=0.0130, old=2),
}
SHANK_TOP = 0.115   # ripe shank above the soil line (before the game's boost)
SHANK_R = 0.0175


def _plants():
    out = []
    rng = random.Random(SEED + 11)
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate(XS):
            out.append(dict(pos=Vector((x + rng.uniform(-0.012, 0.012), y + rng.uniform(-0.012, 0.012), C.BASE_Z)),
                            fan=rng.uniform(0, math.pi), phase=rng.random(), idx=j * 10 + i,
                            scale=rng.uniform(0.9, 1.08)))
    return out


# ------------------------------------------------------------------ painting

def _flag_layer(X, Y, px, nrng, pal, **extra):
    width = 0.85 * float(X.max())   # the blade fills 85% of the tile's width
    shape = F.ovate(width=width, base=0.0, tip=0.995, widest=0.45, tip_sharp=0.40, base_round=0.15)
    veins = [([(0.0, 0.0), (0.0, 0.97)], 1.3)]
    for k in (-4, -3, -2, -1, 1, 2, 3, 4):
        x0 = width * 0.21 * k
        veins.append(([(x0, 0.0), (x0 * 0.8, 0.5), (x0 * 0.15, 0.96)], 0.28))
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.010, vein_depth=0.00006,
                          puff=0.00005, tertiary=0.0, gloss=0.12, trans=0.45, **extra)
    bloom = F.smoothstep(0.35, 0.85, F.noise(X.shape, nrng, freq=5.0, beta=2.2, aniso=(1.0, 4.0)))
    layer.color = F.lerp(layer.color, (0.14, 0.18, 0.17), bloom * 0.30)
    return layer


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=10)
    atlas.column("neck", 64)
    atlas.column("shank", 160)
    atlas.column("root", 48)
    for key in ("flag", "flag_blue", "flag_old"):
        atlas.tile(key, 130, 1020)
    atlas.tile("flag_young", 100, 800)
    atlas.tile("soil", 200, 200)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    pal = dict(base=(0.075, 0.135, 0.115), tip=(0.088, 0.150, 0.125), vein=(0.110, 0.170, 0.145),
               margin=(0.062, 0.115, 0.098), yellow=(0.24, 0.21, 0.07), brown=(0.10, 0.07, 0.03),
               dry=(0.20, 0.15, 0.07))
    blue = dict(pal, base=(0.078, 0.135, 0.135), tip=(0.090, 0.150, 0.145))
    young = dict(pal, base=(0.085, 0.160, 0.100), tip=(0.098, 0.175, 0.112))
    for key, p, extra in (("flag", pal, {}), ("flag_blue", blue, {}),
                          ("flag_old", pal, dict(yellow=0.45, edge_burn=0.5, dry=0.2)),
                          ("flag_young", young, {})):
        X, Y, px = atlas.grid(key)
        atlas.put(key, _flag_layer(X, Y, px, nrng, p, **extra), meters_per_px=0.40 / X.shape[0])
    # Shank column: v 0 (root plate) .. 1 (top): cream-white, then pale yellow-green into the neck.
    U, V = atlas.column_grid("shank")
    n = F.noise(U.shape, nrng, freq=60.0, beta=1.6, aniso=(1.0, 10.0))
    layer = F.Layer(U.shape)
    white = np.asarray((0.74, 0.73, 0.62))
    green = np.asarray((0.26, 0.36, 0.14))
    t = F.smoothstep(0.52, 0.95, V)
    layer.color = F.lerp(white, green, t) * (0.93 + 0.12 * n)[..., None]
    # Fine longitudinal lines where each leaf sheath wraps the next.
    lines = np.exp(-(((U * 7.0) % 1.0 - 0.5) / 0.05) ** 2)
    layer.color = F.lerp(layer.color, layer.color * 0.86, lines * 0.6)
    # A little earth staining at the base.
    dirt = (1 - F.smoothstep(0.02, 0.14, V)) * F.smoothstep(0.4, 0.7, n)
    layer.color = F.lerp(layer.color, (0.20, 0.14, 0.08), dirt * 0.7)
    layer.height = 0.00006 * lines + 0.00004 * n
    layer.alpha[...] = 1
    layer.rough = 0.42 + 0.1 * n
    layer.trans[...] = 0.2
    atlas.put("shank", layer, meters_per_px=0.18 / U.shape[0], wrap=True, opaque=True)
    for key, base, hi in (("neck", (0.060, 0.100, 0.060), (0.090, 0.130, 0.080)),
                          ("root", (0.40, 0.36, 0.28), (0.55, 0.50, 0.40))):
        U, V = atlas.column_grid(key)
        n = F.noise(U.shape, nrng, freq=80.0, beta=1.8, aniso=(1.0, 8.0))
        layer = F.Layer(U.shape)
        layer.color = F.lerp(base, hi, np.clip(0.3 + 0.5 * n, 0, 1))
        layer.height = 0.00004 * n
        layer.alpha[...] = 1
        layer.rough[...] = 0.6
        layer.trans[...] = 0.3
        atlas.put(key, layer, meters_per_px=0.3 / U.shape[0], wrap=True, opaque=True)
    X, Y, px = atlas.grid("soil")
    soil = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=20.0, beta=1.4)
    soil.color = F.lerp((0.07, 0.05, 0.03), (0.14, 0.10, 0.06), n)
    soil.alpha[...] = 1
    soil.height = 0.0004 * n
    soil.rough[...] = 0.9
    atlas.put("soil", soil, meters_per_px=0.05 / X.shape[0], opaque=True)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ geometry

def _fan(b, atlas, base, fan_az, phase, lod, rng, stage, scale=1.0, neck_z=None, flags=None, old=None):
    """The two-sided fan: flags alternate sides of one plane, each out of the one before and a
    little higher up the green neck, the outer ones longest and most arched."""
    cfg = STAGES[stage]
    neck = cfg["neck"] if neck_z is None else neck_z
    count = cfg["flags"] if flags is None else flags
    old = cfg["old"] if old is None else old
    top = base + UP * (neck + cfg["flag_len"] * 0.18 * scale)
    b.tube([base, base + UP * neck * 0.5, top], [cfg["neck_r"] * scale, cfg["neck_r"] * scale * 0.95,
                                                 cfg["neck_r"] * scale * 0.7],
           (6, 4, 3)[lod], atlas.uv("neck"), v_length=0.3, phase=phase, flutter=0.02, cap=True)
    keep = (1.0, 0.65, 0.4)[lod]
    for k in range(count):
        outer = 1.0 - k / max(count, 1)                 # 1 for the oldest, outermost flag
        side = 1 if k % 2 == 0 else -1
        hd = C.heading(fan_az + (0 if side > 0 else math.pi) + rng.uniform(-0.08, 0.08))
        z = neck * 0.55 + (neck * 0.45 + cfg["flag_len"] * 0.15 * scale) * (k / max(count - 1, 1))
        elev = math.radians(60 + 22 * (1 - outer) + rng.uniform(-6, 6))
        length = cfg["flag_len"] * scale * (0.75 + 0.35 * outer) * rng.uniform(0.9, 1.08)
        droop = (0.7 + 1.1 * outer) * rng.uniform(0.85, 1.15)
        is_old = k < old
        key = "flag_young" if stage == "Sprout" else ("flag_old" if is_old else rng.choice(["flag", "flag", "flag_blue"]))
        if is_old:
            droop += 0.5
        if k >= max(2, int(round(count * keep))) and k >= old:
            continue
        at = base + UP * z + hd * cfg["neck_r"] * scale * 0.6
        d = (hd * math.cos(elev) + UP * math.sin(elev)).normalized()
        length *= C.edge_scale(at, hd, length * 0.75)
        w = cfg["flag_w"] * scale * (0.85 + 0.25 * outer)
        C.strap(b, atlas.uv(key), at, d, length, w / 0.85, phase,
                rows=(7, 4, 2)[lod], droop=droop, twist=rng.uniform(-0.35, 0.35) if lod < 2 else 0.0,
                fold=0.35 if lod == 0 else 0.15, flutter=1.0, flutter_base=0.05, taper=0.30)


def emit(stage, lod, atlas):
    cfg = STAGES[stage]
    b = F.Batch(height=C.BASE_Z + cfg["height"])
    for plant in _plants():
        rng = C.plant_rng(SEED, stage, plant["idx"])
        _fan(b, atlas, plant["pos"], plant["fan"], plant["phase"], lod, rng, stage, plant["scale"])
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _anchors():
    rows = {}
    for stage in C.PRODUCE_STAGES:
        rows[stage] = [(p["pos"], math.degrees(p["fan"]), p["scale"]) for p in _plants()]
    return C.anchors_report(f"SM_{NAME}_Produce", rows)


def _shank(b, atlas, bottom, top, radius, sides=14):
    """The blanched shank from ``bottom`` to ``top``: a slight bulb at the root plate, then a
    straight white column that narrows a touch into the neck."""
    rings = 9
    pts, radii = [], []
    for i in range(rings):
        t = i / (rings - 1)
        pts.append(bottom.lerp(top, t))
        bulb = 1.0 + 0.12 * math.exp(-((t - 0.06) / 0.08) ** 2)
        radii.append(radius * bulb * (1.0 - 0.18 * t) * (0.55 + 0.45 * math.sin(min(t / 0.05, 1) * math.pi * 0.5)))
    b.tube(pts, radii, sides, atlas.uv("shank", inset=False), v_length=(top - bottom).length, phase=0.0,
           flutter=0.0, cap=True)
    # Close the root plate.
    ring0 = len(b.verts) - (rings * sides + 1)
    centre = b._vert(bottom - UP * radius * 0.2, 0.0, 0.0)
    u0, u1, v0, _ = atlas.uv("root")
    for j in range(sides):
        k = (j + 1) % sides
        b.faces.append((ring0 + k, ring0 + j, centre))
        b.uvs.append([(u1, v0), (u0, v0), ((u0 + u1) / 2, v0 + 0.01)])


def emit_produce(atlas):
    """Ripe shank standing on the soil line (origin), mostly above it; it's pushed up from below
    and fattened by the game as it grows."""
    b = F.Batch(height=0.2)
    _shank(b, atlas, Vector((0, 0, -0.05)), Vector((0, 0, SHANK_TOP)), SHANK_R)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(atlas):
    """A whole pulled leek gripped at the neck: shank and roots below, flags above."""
    rng = random.Random(SEED + 707)
    b = F.Batch(height=0.6)
    _shank(b, atlas, Vector((0, 0, -0.17)), Vector((0, 0, 0.0)), SHANK_R * 0.95, sides=12)
    for k in range(14):
        az = k * 2.39996 + rng.uniform(-0.2, 0.2)
        start = Vector((math.cos(az) * 0.008, math.sin(az) * 0.008, -0.175))
        pts = [start]
        for s in range(3):
            pts.append(pts[-1] + Vector((math.cos(az) * rng.uniform(0.006, 0.014), math.sin(az) * rng.uniform(0.006, 0.014),
                                         -rng.uniform(0.012, 0.022))))
        b.tube(pts, [0.0010, 0.0008, 0.0006, 0.0003], 3, atlas.uv("root"), v_length=0.2, phase=0.3, flutter=0.1)
    for k in range(6):
        az = k * 2.39996
        b.sphere(Vector((math.cos(az) * 0.012, math.sin(az) * 0.012, -0.19 + rng.uniform(-0.01, 0.01))),
                 rng.uniform(0.004, 0.008), atlas.uv("soil", inset=False), segs=5, rings=3, stretch=0.7)
    _fan(b, atlas, Vector((0, 0, -0.02)), 0.3, 0.3, 0, rng, "Mature", scale=0.85, neck_z=0.02, flags=6, old=1)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(1.0, 1.1, 0.8))
    objs = C.stage_meshes(kit, NAME, material, lambda stage, lod: emit(stage, lod, atlas))
    REPORT["produce"] = _anchors()
    produce_material = material.copy()
    produce_material.name = f"M_{NAME}Produce"
    objs.append(C.single(kit, emit_produce(atlas), f"SM_{NAME}_Produce", produce_material, still_wind=True))
    objs.append(C.single(kit, emit_harvest(atlas), f"SM_{NAME}_Harvest", material, still_wind=True))
    return objs
