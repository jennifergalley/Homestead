"""Cornish winter broccoli (heading broccoli / winter cauliflower) crop plot, five stages.

Real plant research / art targets (written before modelling):
- In Victorian England "broccoli" meant the hardy heading kind that forms a white curd in winter
  and early spring; Penzance and west Cornwall were famous for it (Penzance Early, Cornish
  broccoli), sent up to London by rail from the 1860s.
- The plant is a big upright brassica 50-70 cm tall and as wide: long oblong blue-grey-green
  leaves 40-60 cm on pale fleshy petioles, with a broad white midrib, wavy margins and a waxy
  bloom, set spirally up a thick stem. The inner leaves fold in over the curd to shield it from
  frost, and the old outer leaves yellow and flop.
- The curd is a dense cream-white dome 12-20 cm across of tight flower-bud clusters (florets),
  seen framed by the inner leaves; it's cut with a collar of trimmed leaves.
- Homestead crop plot: three plants staggered on the ridge tops (x=-0.2,+0.2,-0.2 at y=-0.3,0,
  +0.3), base z=0.045; the leaves are kept inside the 1 m square. Walk-through.

The curd is separate instanced produce (SM_CropWinterBroccoli_Produce, sitting on the stem top)
that the game swells and whitens from a small green button; the plant meshes carry the leaves.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_crop as C
import homestead_foliage as F

NAME = "CropWinterBroccoli"
DESCRIPTION = ("One 1 m tilled-bed plot of three Cornish winter broccoli plants in five stages: big "
               "blue-green leaf rosettes; the white curd is separate instanced produce.")
COLLISION = "none"
TRIANGLE_BUDGET = 6000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1853_909

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 4.2,
          "meshes": {
              "SM_CropWinterBroccoli_Growing": {"focus": (0.0, 0.0, 0.15), "eye_distance": 3.6},
              "SM_CropWinterBroccoli_Ripe": {"focus": (0.0, 0.0, 0.25), "eye_distance": 4.2},
              "SM_CropWinterBroccoli_Produce": {"focus": (0.0, 0.0, 0.04), "eye_distance": 0.7,
                                                "detail_distance": 0.35},
              "SM_CropWinterBroccoli_Harvest": {"focus": (0.0, 0.0, -0.08), "eye_distance": 1.4},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / stage height",
             "G": "per-plant phase", "B": "leaf flutter 0.1 at the petiole -> 1 at the tip", "A": "1"},
    "crop_layout": "Three plants at (-0.2,-0.3), (0.2,0), (-0.2,0.3); base z=0.045 m.",
    "material_notes": "One M_CropWinterBroccoli atlas: alpha in basecolor; roughness R, translucency G, AO B. "
                      "The curd uses M_CropWinterBroccoliProduce (re-parented to M_CropProduce in Unreal).",
    "harvest_mesh": "SM_CropWinterBroccoli_Harvest: cut curd in a collar of trimmed leaves, grip at the cut stalk "
                    "(origin), head hanging -Z.",
}

PLANTS = [(-0.20, -0.30), (0.20, 0.0), (-0.20, 0.30)]
UP = C.UP
STAGES = {
    "Sprout": dict(height=0.11, leaves=3, length=0.09, stem=0.004, wrap=0, yellow=0),
    "Young": dict(height=0.16, leaves=6, length=0.14, stem=0.012, wrap=0, yellow=0),
    "Growing": dict(height=0.30, leaves=10, length=0.27, stem=0.045, wrap=2, yellow=0),
    "Mature": dict(height=0.46, leaves=14, length=0.38, stem=0.110, wrap=4, yellow=2),
    "Ripe": dict(height=0.50, leaves=15, length=0.42, stem=0.130, wrap=3, yellow=4),
}
# The curd sits on the stem top, cupped by the inner leaves.
CURD_Z = {"Young": 0.020, "Growing": 0.060, "Mature": 0.130, "Ripe": 0.150}
CURD_R = 0.080


# ------------------------------------------------------------------ painting

def _veins(width, nrng):
    veins = [([(0.0, 0.0), (0.0, 0.97)], 1.9)]
    shape = F.ovate(width=width, widest=0.58, tip_sharp=0.5, base_round=0.6, base=0.0, tip=0.99)
    for k in range(9):
        s0 = 0.14 + k * 0.085 + nrng.uniform(-0.008, 0.008)
        for side in (-1, 1):
            pts = []
            for t in np.linspace(0, 1, 8):
                y = s0 + 0.14 * t + 0.03 * t * t
                _, _, hw = shape(np.zeros(1), np.array([min(y, 0.96)]))
                pts.append((side * hw[0] * 0.9 * math.sin(t * math.pi * 0.5), y))
            veins.append((pts, 0.55 - 0.03 * k))
    return veins


def _leaf_layer(X, Y, px, nrng, pal, **extra):
    a = float(X.max())
    width = a * 1.02
    shape = F.ovate(width=width, widest=0.58, tip_sharp=0.5, base_round=0.6, base=0.0, tip=0.99,
                    teeth=18, tooth_depth=0.02, double=0.3)
    layer = F.paint_blade(X, Y, nrng, shape, _veins(width, nrng), pal, px, vein_width=0.016,
                          vein_depth=0.00025, puff=0.00022, tertiary=0.5, gloss=-0.2, hair=0.0,
                          stalk=0.035, trans=0.40, **extra)
    wax = F.smoothstep(0.25, 0.85, F.noise(X.shape, nrng, freq=14, beta=2.0)) * layer.alpha
    layer.color = F.lerp(layer.color, (0.15, 0.18, 0.165), wax * 0.22)
    layer.rough = np.clip(layer.rough + 0.12 * wax, 0.45, 0.92)
    return layer


def _paint_curd(atlas, nrng):
    """Top-down curd tile: tight rounded floret clusters, cream-white, shadowed between them."""
    X, Y, px = atlas.grid("curd")
    rng = random.Random(SEED + 17)
    h = np.zeros(X.shape)
    for size, count in ((0.13, 26), (0.065, 90), (0.03, 320)):
        for _ in range(count):
            cx, cy = rng.uniform(-0.55, 0.55), rng.uniform(-0.05, 1.05)
            r2 = ((X - cx) ** 2 + (Y - cy) ** 2) / (size * size)
            h = np.maximum(h, np.sqrt(np.clip(1 - r2, 0, 1)) * size)
    h = h / h.max()
    n = F.noise(X.shape, nrng, freq=80, beta=1.3)
    layer = F.Layer(X.shape)
    cream = np.asarray((0.74, 0.71, 0.57))
    shade = np.asarray((0.42, 0.40, 0.26))
    layer.color = F.lerp(shade, cream, np.clip(0.35 + 0.75 * h, 0, 1)) * (0.94 + 0.1 * n)[..., None]
    layer.alpha[...] = 1
    layer.height = 0.004 * h + 0.0002 * n
    layer.ao = 0.55 + 0.45 * np.clip(h * 1.4, 0, 1)
    layer.rough = 0.72 + 0.1 * n
    layer.trans[...] = 0.1
    atlas.put("curd", layer, meters_per_px=0.20 / X.shape[0], opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=12)
    atlas.column("stem", 64)
    atlas.tile("cotyledon", 300, 300)
    for key in ("leaf", "leaf2", "inner", "yellow"):
        atlas.tile(key, 440, 820)
    atlas.tile("curd", 560, 560)
    atlas.tile("cut", 160, 160)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    pal = dict(base=(0.070, 0.110, 0.092), tip=(0.080, 0.124, 0.104), vein=(0.200, 0.225, 0.180),
               margin=(0.060, 0.094, 0.080), yellow=(0.24, 0.22, 0.09), brown=(0.08, 0.05, 0.025),
               stalk=(0.200, 0.225, 0.175))
    pals = {
        "leaf": pal,
        "leaf2": dict(pal, base=(0.062, 0.104, 0.098), tip=(0.074, 0.118, 0.110)),
        "inner": dict(pal, base=(0.105, 0.150, 0.110), tip=(0.125, 0.170, 0.125), vein=(0.24, 0.26, 0.20)),
        "yellow": dict(pal, base=(0.15, 0.15, 0.075), tip=(0.19, 0.18, 0.08)),
    }
    for key in ("leaf", "leaf2", "inner", "yellow"):
        X, Y, px = atlas.grid(key)
        extra = dict(yellow=0.45, edge_burn=0.3, holes=0.15, damage=0.3) if key == "yellow" else dict(damage=0.04)
        atlas.put(key, _leaf_layer(X, Y, px, nrng, pals[key], **extra), meters_per_px=0.42 / X.shape[0])
    X, Y, px = atlas.grid("cotyledon")
    shape = F.ovate(width=0.42, widest=0.5, tip_sharp=0.35, base_round=1.4, base=0.03, tip=0.93, cordate=0.2)
    atlas.put("cotyledon", F.paint_blade(X, Y, nrng, shape, [([(0, 0.02), (0, 0.9)], 1.0)], pal, px,
                                         tertiary=0.2, trans=0.5), meters_per_px=0.03 / X.shape[0])
    _paint_curd(atlas, nrng)
    X, Y, px = atlas.grid("cut")
    cut = F.Layer(X.shape)
    r = np.hypot(X, Y - 0.5)
    n = F.noise(X.shape, nrng, freq=36, beta=1.5)
    cut.color = F.lerp((0.30, 0.31, 0.21), (0.42, 0.41, 0.27), n)
    cut.alpha = 1 - F.smoothstep(0.43, 0.46, r)
    cut.height = 0.0001 * n
    atlas.put("cut", cut, meters_per_px=0.04 / X.shape[0])
    U, V = atlas.column_grid("stem")
    n = F.noise(U.shape, nrng, freq=40, beta=1.5, aniso=(1.0, 7.0))
    stem = F.Layer(U.shape)
    stem.color = F.lerp((0.10, 0.13, 0.10), (0.19, 0.21, 0.15), n)
    scars = F.smoothstep(0.85, 0.95, F.noise(U.shape, nrng, freq=12, beta=1.5, aniso=(2.0, 1.0)))
    stem.color = F.lerp(stem.color, (0.22, 0.20, 0.13), scars * 0.7)
    stem.alpha[...] = 1
    stem.height = 0.0001 * n + 0.0003 * scars
    stem.rough = 0.7 + 0.1 * n
    stem.trans[...] = 0.2
    atlas.put("stem", stem, meters_per_px=0.30 / U.shape[0], opaque=True, wrap=True)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ geometry

def _leaf(b, atlas, key, base, az, length, phase, rng, lod, elev_deg, droop, cup=0.03):
    rad = C.heading(az)
    length *= C.edge_scale(base, rad, length * math.cos(math.radians(elev_deg)) + 0.02)
    rows, cols = ((9, 6), (4, 3), (2, 2))[lod]
    C.cupped_leaf(b, atlas.uv(key), base, rad, length, length * 0.46, phase, rng,
                  elev=math.radians(elev_deg), droop=droop, cup=cup * length / 0.3,
                  rows=rows, cols=cols, flutter=0.85, wave=0.10 if lod == 0 else 0.0,
                  frill=0.012 if lod == 0 else 0.0, narrow=0.22)


def emit(stage, lod, atlas):
    cfg = STAGES[stage]
    b = F.Batch(height=C.BASE_Z + cfg["height"])
    for pidx, (x, y) in enumerate(PLANTS):
        rng = C.plant_rng(SEED, stage, pidx)
        base = Vector((x, y, C.BASE_Z))
        phase = rng.random()
        az0 = rng.uniform(0, math.tau)
        if stage == "Sprout":
            # Two cotyledons and the first three true leaves: big enough to read as a seedling at 7 m.
            for j in range(5):
                az = az0 + (math.pi * j if j < 2 else 1.4 + (j - 2) * 2.1)
                key = "cotyledon" if j < 2 else "inner"
                d = (C.heading(az) + UP * (0.8 if j < 2 else 1.3)).normalized()
                ln = cfg["length"] * (1.0 if j < 2 else 0.9 - 0.12 * (j - 2))
                b.card(base + UP * 0.012, d, (UP - C.heading(az) * 0.4).normalized(), ln,
                       ln * (0.9 if j < 2 else 0.5), atlas.uv(key), rows=(2, 1, 1)[lod], cols=1, fold=0.01,
                       droop=0.1, phase=phase, flutter=0.9, flutter_base=0.1)
            b.tube([base, base + UP * 0.013], [0.0015, 0.0013], 4, atlas.uv("stem"), v_length=0.3, phase=phase)
            continue
        top = base + UP * cfg["stem"]
        b.tube([base - UP * 0.01, base.lerp(top, 0.5), top], [0.014 * cfg["stem"] / 0.13 + 0.004] * 2 +
               [0.011 * cfg["stem"] / 0.13 + 0.003], (8, 5, 4)[lod], atlas.uv("stem"), v_length=0.3,
               phase=phase, flutter=0.0, cap=True)
        n = cfg["leaves"]
        keep = (1.0, 0.62, 0.38)[lod]
        for i in range(n):
            t = i / max(n - 1, 1)                          # 0 oldest (bottom, outer) .. 1 youngest
            az = az0 + i * 2.39996 + rng.uniform(-0.12, 0.12)
            node = base + UP * (cfg["stem"] * (0.15 + 0.8 * t))
            yellow = i < cfg["yellow"]
            inner = i >= n - cfg["wrap"]
            length = cfg["length"] * rng.uniform(0.88, 1.06) * (0.45 if inner else (0.92 if yellow else 1.0 - 0.15 * t))
            if inner:
                elev, droop, key = rng.uniform(50, 62), -0.10, "inner"   # a collar cupping the curd
            elif yellow:
                elev, droop, key = rng.uniform(8, 22), rng.uniform(0.55, 0.8), "yellow"
            else:
                elev = 24 + 30 * t + rng.uniform(-6, 6) + (12 if stage == "Young" else 0)
                droop, key = rng.uniform(0.25, 0.45), rng.choice(["leaf", "leaf", "leaf2"])
            skip = i < n - max(3, int(round(n * keep)))
            if skip and not inner:
                continue
            # Inner leaves stand up round the curd as a collar rather than from its middle.
            offset = {"Growing": 0.035, "Mature": 0.08, "Ripe": 0.095}.get(stage, 0.01) if inner else 0.01
            _leaf(b, atlas, key, node + C.heading(az) * offset, az, length, phase, rng, lod, elev, droop,
                  cup=0.02 if inner else 0.035)
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _anchors():
    rows = {}
    for stage in C.PRODUCE_STAGES:
        rows[stage] = [(Vector((x, y, C.BASE_Z + CURD_Z[stage])), (pidx * 67.0) % 360.0, (1.0, 1.08, 0.94)[pidx])
                       for pidx, (x, y) in enumerate(PLANTS)]
    return C.anchors_report(f"SM_{NAME}_Produce", rows)


def _curd(b, atlas, center, radius, height, lod=0):
    """A floret-lobed dome: base ring at ``center``, top at ``center + height``; UVs projected
    from above onto the curd tile."""
    rng = random.Random(SEED + 31)
    florets = [(rng.uniform(0, math.tau), rng.uniform(0.15, 1.05)) for _ in range(11)]
    segs, rings = ((28, 10), (16, 6), (10, 4))[lod]
    u0, u1, v0, v1 = atlas.uv("curd")
    grid = []
    for i in range(rings + 1):
        th = (i / rings) * math.radians(100)           # 0 at the crown .. 100 deg just under the rim
        row = []
        for j in range(segs):
            ph = math.tau * j / segs
            lobe = 0.0
            for fa, fth in florets:
                dang = math.acos(max(-1.0, min(1.0, math.cos(th) * math.cos(fth) +
                                               math.sin(th) * math.sin(fth) * math.cos(ph - fa))))
                lobe = max(lobe, math.exp(-(dang / 0.32) ** 2))
            r = radius * (1.0 + 0.07 * lobe - 0.03)
            p = Vector((math.sin(th) * math.cos(ph) * r, math.sin(th) * math.sin(ph) * r,
                        math.cos(th) * height * (1.0 + 0.06 * lobe)))
            row.append(b._vert(center + p, 0.0, 0.0))
        grid.append(row)
    top = b._vert(center + Vector((0, 0, height * 1.02)), 0.0, 0.0)

    def uv_of(v):
        co = Vector(b.verts[v]) - center
        return (u0 + (u1 - u0) * (0.5 + co.x / (2.3 * radius)), v0 + (v1 - v0) * (0.5 + co.y / (2.3 * radius)))
    for j in range(segs):
        k = (j + 1) % segs
        b.faces.append((top, grid[0][j], grid[0][k]))
        b.uvs.append([uv_of(top), uv_of(grid[0][j]), uv_of(grid[0][k])])
    for i in range(rings):
        for j in range(segs):
            k = (j + 1) % segs
            f = (grid[i][j], grid[i + 1][j], grid[i + 1][k], grid[i][k])
            b.faces.append(f)
            b.uvs.append([uv_of(v) for v in f])


def emit_produce(atlas):
    """The ripe curd on the stem top (origin at its base), in a collar of short inner leaves."""
    b = F.Batch(height=0.2)
    rng = random.Random(SEED + 1200)
    _curd(b, atlas, Vector((0, 0, 0.012)), CURD_R, 0.075)
    for j in range(6):
        az = j * math.tau / 6 + rng.uniform(-0.1, 0.1)
        C.cupped_leaf(b, atlas.uv("inner"), C.heading(az) * CURD_R * 0.55, C.heading(az), 0.085, 0.060, 0.0, rng,
                      elev=math.radians(62), droop=-0.15, cup=0.012, rows=5, cols=4, flutter=0.0, wave=0.05,
                      frill=0.004, narrow=0.4)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(atlas):
    """Cut head: stalk cut at the grip (origin), curd hanging below in a collar of trimmed leaves."""
    b = F.Batch(height=0.25)
    rng = random.Random(SEED + 1300)
    b.tube([Vector((0, 0, 0)), Vector((0, 0, -0.035))], [0.016, 0.018], 12, atlas.uv("stem"), v_length=0.2, cap=False)
    b.flat(Vector((0, 0, 0.001)), UP, 0.034, atlas.uv("cut"), spin=0.2, phase=0.0, flutter=0.0, segs=2)
    # Held stalk-up, so the curd faces down.
    center = Vector((0, 0, -0.035))
    mark = len(b.verts)
    _curd(b, atlas, Vector((0, 0, 0)), CURD_R * 0.95, 0.07)
    for v in range(mark, len(b.verts)):
        x, y, z = b.verts[v]
        b.verts[v] = (x, y, center.z - z)
    for j in range(7):
        az = j * math.tau / 7 + rng.uniform(-0.1, 0.1)
        C.cupped_leaf(b, atlas.uv(rng.choice(["inner", "leaf"])), center + C.heading(az) * 0.02, C.heading(az),
                      rng.uniform(0.11, 0.14), 0.07, 0.0, rng, elev=math.radians(-55), droop=-0.25, cup=0.015,
                      rows=6, cols=4, flutter=0.0, wave=0.06, frill=0.005, narrow=0.4)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(0.95, 1.10, 0.8))
    objs = C.stage_meshes(kit, NAME, material, lambda stage, lod: emit(stage, lod, atlas))
    REPORT["produce"] = _anchors()
    produce_material = material.copy()
    produce_material.name = f"M_{NAME}Produce"
    objs.append(C.single(kit, emit_produce(atlas), f"SM_{NAME}_Produce", produce_material, still_wind=True))
    objs.append(C.single(kit, emit_harvest(atlas), f"SM_{NAME}_Harvest", material, still_wind=True))
    return objs
