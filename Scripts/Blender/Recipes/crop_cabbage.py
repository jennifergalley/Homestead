"""Procedural 1 m plot of Early York / Cornish spring cabbage crop stages for Homestead.

Real plant research (written before modeling):
- Early York and similar 19th-century spring cabbages are compact brassicas with a low stem,
  waxy glaucous blue-green outer leaves, and a pale pointed-to-round heart. Plants in small
  kitchen-garden rows are set on ridge tops, one per ridge here, with broad leaves cupping upward
  then relaxing outward as they size up.
- Seedlings first show two kidney-shaped cotyledons and a small first true leaf at about 3 cm.
  Young plants have 4-6 rounded petioled leaves, 10-12 cm tall and 15-20 cm across. Growing plants
  form open rosettes 25-30 cm across with heavy pale midribs and branching veins. Mature plants
  reach 40-45 cm across with a firm pale heart 10-12 cm. Ripe heads are tight, 14-18 cm, paler
  green-white and slightly sugar-loaf/York pointed, cupped by frayed blue-green leaves with a few
  slug holes and yellowed edges.
- This recipe represents one 1 m tilled-bed plot: three plants on ridge tops at y=-0.3/0/+0.3 m,
  staggered x=-0.2/+0.2/-0.2 m, base z=0.045 m. The pivot remains plot bottom-centre.

All geometry/textures are original procedural work. One painted 2K atlas, one two-sided masked
foliage material, Wind vertex colours, and five stage meshes each with LOD1/LOD2.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "CropCabbage"
DESCRIPTION = ("One 1 m garden plot of Early York/Cornish spring cabbage in five stages: seedling, young, "
               "open rosette and hearting/ripe wrapper leaves; the cabbage head is drawn by the game "
               "as a separate anchored produce mesh.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"], "eye_distance": 5.0, "meshes": {
    "SM_CropCabbage_Growing": {"focus": (0.0, 0.0, 0.16), "eye_distance": 4.8},
    "SM_CropCabbage_Ripe": {"focus": (0.0, 0.0, 0.18), "eye_distance": 5.0},
    "SM_CropCabbage_Produce": {"focus": (0.0, 0.0, 0.082), "eye_distance": 0.78,
                               "detail_distance": 0.46, "views": ["hero", "detail"]},
    "SM_CropCabbage_Harvest": {"focus": (0.0, 0.0, -0.09), "eye_distance": 1.5, "views": ["hero", "detail"]},
}}
REPORT = {
    "blocking": False,
    "plot": "1 m square, pivot bottom-centre; cabbage crowns stand on tilled-bed ridge tops at z≈0.045 m.",
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above plot ground / stage height",
             "G": "per-plant random phase", "B": "outer-leaf flutter; produce mesh is static", "A": "1"},
    "material_notes": ("One material M_CropCabbage: 2K alpha-masked foliage atlas with waxy glaucous leaves, "
                       "slug damage/yellowed margins and opaque wrapped-head tiles. "
                       "SM_CropCabbage_Produce uses material slot M_CropCabbageProduce with the same atlas."),
}

SEED = 1852
BASE_Z = 0.045
PLANTS = [(-0.20, -0.30), (0.20, 0.0), (-0.20, 0.30)]
STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")


def _produce_anchor_lists():
    scales = (0.94, 1.08, 0.89)
    yaws = (9.0, -16.0, 23.0)
    stage_z = {
        "Young": BASE_Z + 0.056,
        "Growing": BASE_Z + 0.075,
        "Mature": BASE_Z + 0.071,
        "Ripe": BASE_Z + 0.068,
    }
    anchors = {}
    for stage, z in stage_z.items():
        anchors[stage] = [
            [round(x, 3), round(y, 3), round(z, 3), yaws[idx], scales[idx]]
            for idx, (x, y) in enumerate(PLANTS)
        ]
    return anchors


REPORT["produce"] = {
    "mesh": "SM_CropCabbage_Produce",
    "anchors": _produce_anchor_lists(),
}


def _leaf_shape(kind="outer"):
    if kind == "cotyledon":
        return F.ovate(width=0.46, widest=0.46, tip_sharp=0.34, base_round=1.45, base=0.03, tip=0.93,
                       cordate=0.20)
    if kind == "young":
        return F.ovate(width=0.48, widest=0.50, tip_sharp=0.45, base_round=0.95, base=0.02, tip=0.96,
                       teeth=10, tooth_depth=0.018, double=0.15)
    return F.ovate(width=0.62, widest=0.55, tip_sharp=0.34, base_round=0.78, base=0.015, tip=0.98,
                   teeth=14, tooth_depth=0.026, double=0.25, cordate=0.08)


def _veins(width=0.62, nrng=None):
    nrng = nrng or np.random.default_rng(0)
    veins = [([(0.0, 0.02), (0.0, 0.98)], 1.0)]
    shape = F.ovate(width=width, widest=0.55, tip_sharp=0.34, base_round=0.78, base=0.015, tip=0.98)
    for k in range(8):
        s0 = 0.13 + k * 0.095 + nrng.uniform(-0.010, 0.010)
        for side in (-1, 1):
            pts = []
            for t in np.linspace(0, 1, 8):
                y = s0 + 0.18 * t + 0.03 * t * t
                _, _, hw = shape(np.zeros(1), np.array([min(y, 0.96)]))
                pts.append((side * hw[0] * 0.86 * math.sin(t * math.pi * 0.5), y))
            veins.append((pts, 0.72 - 0.04 * k))
    return veins


def _palette(key):
    pals = {
        "outer": dict(base=(0.090, 0.135, 0.095), tip=(0.108, 0.162, 0.115),
                      vein=(0.160, 0.190, 0.135), margin=(0.074, 0.112, 0.082),
                      yellow=(0.23, 0.21, 0.085), brown=(0.075, 0.045, 0.024), stalk=(0.125, 0.150, 0.105)),
        "outer2": dict(base=(0.080, 0.125, 0.100), tip=(0.098, 0.150, 0.120),
                       vein=(0.145, 0.178, 0.138), margin=(0.068, 0.104, 0.088),
                       yellow=(0.23, 0.21, 0.085), brown=(0.075, 0.045, 0.024), stalk=(0.115, 0.140, 0.105)),
        "pale": dict(base=(0.135, 0.185, 0.118), tip=(0.165, 0.215, 0.138),
                     vein=(0.210, 0.250, 0.165), margin=(0.105, 0.155, 0.095),
                     yellow=(0.25, 0.23, 0.105), brown=(0.075, 0.045, 0.024), stalk=(0.190, 0.225, 0.145)),
        "yellow": dict(base=(0.155, 0.150, 0.075), tip=(0.205, 0.195, 0.085),
                       vein=(0.195, 0.200, 0.115), margin=(0.100, 0.085, 0.042),
                       yellow=(0.255, 0.230, 0.105), brown=(0.080, 0.047, 0.022), stalk=(0.155, 0.140, 0.075)),
        "cotyledon": dict(base=(0.070, 0.105, 0.086), tip=(0.082, 0.120, 0.098),
                          vein=(0.125, 0.155, 0.112), margin=(0.060, 0.090, 0.078),
                          brown=(0.075, 0.045, 0.024), stalk=(0.100, 0.130, 0.090)),
    }
    return pals[key]


def _cabbage_leaf_layer(X, Y, px, nrng, key, kind="outer", damage=0.0):
    pal_key = "cotyledon" if kind == "cotyledon" else key
    layer = F.paint_blade(X, Y, nrng, _leaf_shape(kind), _veins(0.46 if kind != "outer" else 0.62, nrng),
                          _palette(pal_key), px, vein_width=0.018 if kind == "outer" else 0.014,
                          vein_depth=0.00022, puff=0.00020, tertiary=0.55, damage=damage,
                          holes=0.16 if key == "yellow" else (0.10 * damage), yellow=0.35 if key == "yellow" else 0.0,
                          edge_burn=0.25 if key == "yellow" else 0.05 * damage, gloss=-0.18,
                          hair=0.04, stalk=0.030 if kind != "cotyledon" else 0.010, trans=0.42)
    wax = F.smoothstep(0.25, 0.85, F.noise(X.shape, nrng, freq=18, beta=2.0)) * layer.alpha
    layer.color = F.lerp(layer.color, (0.145, 0.175, 0.145), wax * (0.18 if key != "pale" else 0.08))
    layer.rough = np.clip(layer.rough + 0.12 * wax, 0.45, 0.92)
    return layer


def _paint_head(atlas, key, nrng):
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=9, beta=2.2)
    fine = F.noise(X.shape, nrng, freq=70, beta=1.5, aniso=(1.0, 2.6))
    base = F.lerp((0.140, 0.200, 0.122), (0.225, 0.285, 0.165), n)
    bands = 0.5 + 0.5 * np.sin((X * 9.0 + 0.35 * np.sin(Y * math.tau * 3.0)) * math.tau)
    ribs = F.smoothstep(0.80, 0.96, bands) * (0.35 + 0.65 * fine)
    layer.color = F.lerp(base, (0.090, 0.145, 0.095), ribs * 0.22)
    layer.color = F.lerp(layer.color, (0.245, 0.305, 0.185), F.smoothstep(0.75, 1.0, Y) * 0.25)
    layer.alpha[...] = 1
    layer.height = 0.00020 * ribs + 0.00008 * fine
    layer.rough = 0.66 + 0.12 * (1 - fine)
    layer.trans[...] = 0.25
    atlas.put(key, layer, meters_per_px=0.11 / X.shape[0], opaque=True)


def _paint_stem(atlas, nrng):
    U, V = atlas.column_grid("stem")
    n = F.noise(U.shape, nrng, freq=40, beta=1.5, aniso=(1.0, 7.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.085, 0.115, 0.080), (0.160, 0.185, 0.110), n)
    layer.alpha[...] = 1
    layer.height = 0.00008 * n
    layer.rough = 0.68 + 0.10 * n
    layer.trans[...] = 0.25
    atlas.put("stem", layer, meters_per_px=0.30 / U.shape[0], opaque=True, wrap=True)


def _paint_cut(atlas, nrng):
    X, Y, px = atlas.grid("cut")
    layer = F.Layer(X.shape)
    r = np.hypot(X, Y - 0.5)
    n = F.noise(X.shape, nrng, freq=36, beta=1.5)
    disc = 1 - F.smoothstep(0.43, 0.46, r)
    layer.color = F.lerp((0.255, 0.260, 0.175), (0.360, 0.345, 0.205), n)
    layer.alpha = disc
    layer.height = 0.00010 * n
    layer.rough = 0.68 + 0.10 * n
    layer.trans[...] = 0.08
    atlas.put("cut", layer, meters_per_px=0.04 / X.shape[0])


def _paint_crown(atlas, nrng):
    X, Y, px = atlas.grid("crown")
    r = np.hypot(X, Y - 0.5)
    n = F.noise(X.shape, nrng, freq=24, beta=1.8)
    disc = 1 - F.smoothstep(0.42, 0.46, r)
    layer = F.Layer(X.shape)
    layer.color = F.lerp((0.165, 0.225, 0.135), (0.250, 0.300, 0.175), n)
    layer.alpha = disc
    layer.height = 0.00008 * n
    layer.rough = 0.60 + 0.06 * n
    layer.trans[...] = 0.20
    atlas.put("crown", layer, meters_per_px=0.035 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=12)
    atlas.column("stem", 56)
    atlas.tile("cotyledon", 340, 360)
    atlas.tile("young", 420, 520)
    atlas.tile("outer", 560, 700)
    atlas.tile("outer2", 560, 700)
    atlas.tile("yellow", 560, 700)
    atlas.tile("pale", 480, 620)
    atlas.tile("head", 520, 520)
    atlas.tile("cut", 180, 180)
    atlas.tile("crown", 160, 160)
    atlas.tile("spray", 620, 620)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    for key, kind, dmg in (("cotyledon", "cotyledon", 0.0), ("young", "young", 0.0), ("outer", "outer", 0.05),
                           ("outer2", "outer", 0.07), ("yellow", "outer", 0.45), ("pale", "young", 0.0)):
        X, Y, px = atlas.grid(key)
        paint_key = key if key not in ("young", "cotyledon") else ("outer" if key == "young" else "cotyledon")
        atlas.put(key, _cabbage_leaf_layer(X, Y, px, nrng, paint_key, kind, dmg),
                  meters_per_px=(0.18 if kind == "outer" else 0.10) / X.shape[0])
    X, Y, px = atlas.grid("spray")
    layer = F.Layer(X.shape)
    for i in range(8):
        th = i * math.tau / 8 + rng.uniform(-0.10, 0.10)
        ln = rng.uniform(0.42, 0.54)
        Xl, Yl = S.rotated(X, Y, (0.0, 0.38), th, ln, squash=0.95)
        layer.over(_cabbage_leaf_layer(Xl, Yl, px / ln, nrng, rng.choice(["outer", "outer2", "pale"]), "outer", 0.05))
    atlas.put("spray", layer, meters_per_px=0.32 / X.shape[0])
    _paint_head(atlas, "head", nrng)
    _paint_stem(atlas, nrng)
    _paint_cut(atlas, nrng)
    _paint_crown(atlas, nrng)
    atlas.save()
    return atlas


def _restore_plot_origin(objs):
    for obj in objs:
        shift = Vector(obj.get("homestead_shift", (0, 0, 0)))
        if shift.length > 1e-8:
            obj.data.transform(Matrix.Translation(shift))
            obj["homestead_shift"] = [0.0, 0.0, 0.0]
            obj.data.update()
    return objs


def _card_leaf(b, atlas, base, radial, length, key, lod, phase, rng, elev, cup=0.18, fray=0.0):
    d = (radial * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    up = (Vector((0, 0, 1)) - radial * 0.42).normalized()
    rows = (4, 2, 1)[lod]
    cols = (2, 1, 1)[lod]
    width = length * (0.72 if key != "cotyledon" else 0.82)
    b.card(base, d, up, length, width, atlas.uv(key), rows=rows, cols=cols,
           fold=cup * (1.0, 0.65, 0.25)[lod], curl=rng.uniform(-0.04, 0.08) + fray * 0.10,
           droop=rng.uniform(0.02, 0.16) + fray * 0.25, lift=0.12 if elev > 0.35 else 0.0,
           twist=rng.uniform(-0.18, 0.18), phase=phase, flutter=1.0, flutter_base=0.10)


def _cupped_leaf_grid(b, atlas, base, radial, length, width, key, phase, rng, *,
                      elev=0.65, droop=0.25, cup=0.030, rows=8, cols=8, flutter=0.65):
    """Actual rounded, cupped cabbage leaf geometry (not a rectangular card)."""
    radial = Vector(radial).normalized()
    side = radial.cross(Vector((0, 0, 1)))
    if side.length < 1e-4:
        side = Vector((1, 0, 0))
    side.normalize()
    normal = (Vector((0, 0, 1)) - radial * 0.30).normalized()
    direction = (radial * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    u0, u1, v0, v1 = atlas.uv(key)
    idx = []
    for i in range(rows + 1):
        v = i / rows
        # Broad brassica profile: narrow petiole, widest beyond the middle, rounded tip.
        prof = math.sin(math.pi * (0.04 + 0.92 * v)) ** 0.55
        prof *= (0.38 + 0.62 * min(v / 0.28, 1.0))
        if v > 0.86:
            prof *= 1.0 - 0.35 * (v - 0.86) / 0.14
        wave_phase = rng.uniform(0, math.tau)
        center = Vector(base) + direction * (length * v) - Vector((0, 0, 1)) * (droop * length * v * v)
        row = []
        for j in range(cols + 1):
            s = j / cols * 2 - 1
            margin_wave = 1.0 + 0.075 * math.sin(17.0 * v + wave_phase) * (abs(s) ** 4)
            hw = width * 0.5 * max(prof, 0.08) * margin_wave
            cupping = cup * (abs(s) ** 1.7) * (0.45 + 0.75 * math.sin(math.pi * v))
            frill = Vector((0, 0, 1)) * (0.010 * math.sin(23 * v + wave_phase) * (abs(s) ** 6))
            co = center + side * (s * hw) + normal * cupping + frill
            row.append(b._vert(co, phase, flutter * (0.12 + 0.88 * v)))
        idx.append(row)
    for i in range(rows):
        for j in range(cols):
            b.faces.append((idx[i][j], idx[i][j + 1], idx[i + 1][j + 1], idx[i + 1][j]))
            b.uvs.append([(u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * ((i + 1) / rows)),
                          (u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * ((i + 1) / rows))])


def _head_leaf_layers(b, atlas, center, radius, height, phase, lod, rng, count=6):
    if lod > 1:
        return
    rows, cols = ((3, 4), (2, 3))[lod]
    for j in range(count if lod == 0 else max(3, count // 2)):
        az = j * math.tau / count + rng.uniform(-0.10, 0.10)
        rad = Vector((math.cos(az), math.sin(az), 0))
        base = Vector(center) + rad * (radius * rng.uniform(0.12, 0.30)) + Vector((0, 0, -height * 0.34))
        _cupped_leaf_grid(b, atlas, base, rad, radius * rng.uniform(0.85, 1.08),
                          radius * rng.uniform(0.48, 0.62), "pale", phase + 0.03 * j, rng,
                          elev=math.radians(54), droop=0.06, cup=0.010,
                          rows=rows, cols=cols, flutter=0.05)


def _head(b, center, radius, height, rect, phase, lod, pointed=True, crown_rect=None):
    segs = (24, 14, 10)[lod]
    rings = (10, 6, 5)[lod]
    u0, u1, v0, v1 = rect
    grid = []
    for i in range(rings + 1):
        t = i / rings
        z = -height * 0.46 + height * t
        if pointed:
            prof = (math.sin(math.pi * (0.08 + 0.86 * t)) ** 0.52) * (1.08 - 0.36 * t)
            prof *= (1 - 0.72 * max(t - 0.72, 0) / 0.28)
        else:
            prof = math.sin(math.pi * (0.10 + 0.80 * t)) ** 0.55
        if i == rings:
            prof = 0.018 if pointed else 0.035
        row = []
        for j in range(segs):
            a = math.tau * j / segs
            wobble = 1.0 + 0.035 * math.sin(5 * a + 8 * t) + 0.018 * math.sin(11 * a)
            p = Vector(center) + Vector((math.cos(a) * radius * prof * wobble,
                                         math.sin(a) * radius * prof * wobble,
                                         z))
            row.append(b._vert(p, phase, 0.05))
        grid.append(row)
    bottom = b._vert(Vector(center) + Vector((0, 0, -height * 0.50)), phase, 0.02)
    top = b._vert(Vector(center) + Vector((0, 0, height * 0.54)), phase, 0.02)
    cu = (u0 + u1) * 0.5
    for i in range(rings):
        for j in range(segs):
            k = (j + 1) % segs
            b.faces.append((grid[i][j], grid[i][k], grid[i + 1][k], grid[i + 1][j]))
            b.uvs.append([(u0 + (u1 - u0) * j / segs, v0 + (v1 - v0) * i / rings),
                          (u0 + (u1 - u0) * (j + 1) / segs, v0 + (v1 - v0) * i / rings),
                          (u0 + (u1 - u0) * (j + 1) / segs, v0 + (v1 - v0) * (i + 1) / rings),
                          (u0 + (u1 - u0) * j / segs, v0 + (v1 - v0) * (i + 1) / rings)])
    for j in range(segs):
        k = (j + 1) % segs
        b.faces.append((bottom, grid[0][j], grid[0][k]))
        b.uvs.append([(cu, v0), (u0 + (u1 - u0) * j / segs, v0), (u0 + (u1 - u0) * (j + 1) / segs, v0)])
        b.faces.append((grid[-1][j], grid[-1][k], top))
        b.uvs.append([(u0 + (u1 - u0) * j / segs, v1), (u0 + (u1 - u0) * (j + 1) / segs, v1), (cu, v1)])
    if crown_rect is not None:
        b.flat(Vector(center) + Vector((0, 0, height * 0.548)), Vector((0, 0, 1)), radius * 0.42,
               crown_rect, spin=0.0, cup=-0.03, phase=phase, flutter=0.02, segs=(2, 2, 1)[lod])


def _head_wrap_patch(b, atlas, center, radius, height, azimuth, span, twist, phase, *,
                     rows=5, cols=4, offset=0.003):
    """Overlapping outside leaf panel on a drumhead cabbage."""
    u0, u1, v0, v1 = atlas.uv("head", inset=False)
    grid = []
    for i in range(rows + 1):
        t = i / rows
        z = -height * 0.45 + height * (0.08 + 0.86 * t)
        prof = math.sin(math.pi * (0.10 + 0.80 * (0.08 + 0.86 * t))) ** 0.55
        row = []
        for j in range(cols + 1):
            s = j / cols * 2 - 1
            curl = span * s * (1.04 - 0.38 * t)
            a = azimuth + curl + twist * (t - 0.5)
            rib = 0.0015 * math.cos(s * math.pi) * math.sin(t * math.pi)
            rr = radius * prof + offset + rib
            p = Vector(center) + Vector((math.cos(a) * rr, math.sin(a) * rr, z))
            row.append(b._vert(p, phase, 0.0))
        grid.append(row)
    for i in range(rows):
        for j in range(cols):
            b.faces.append((grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j]))
            b.uvs.append([(u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * ((i + 1) / rows)),
                          (u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * ((i + 1) / rows))])


def emit(stage, atlas, lod):
    rng = random.Random(SEED + 151 * STAGES.index(stage))
    cfg = {
        "Sprout": dict(height=0.055, leaves=3, length=0.038, keys=["cotyledon"], head=0),
        "Young": dict(height=0.13, leaves=6, length=0.105, keys=["young", "outer"], head=0),
        "Growing": dict(height=0.22, leaves=10, length=0.150, keys=["outer", "outer2", "pale"], head=0.045),
        "Mature": dict(height=0.31, leaves=13, length=0.220, keys=["outer", "outer2", "pale"], head=0.080),
        "Ripe": dict(height=0.34, leaves=15, length=0.235, keys=["outer", "outer2", "yellow", "pale"], head=0.115),
    }[stage]
    b = F.Batch(height=cfg["height"])
    stats = {"leaves": 0, "heads": 0}
    for pidx, (x, y) in enumerate(PLANTS):
        base = Vector((x, y, BASE_Z))
        phase = rng.random()
        prng = random.Random(SEED * 7 + pidx * 300 + STAGES.index(stage) * 31 + lod)
        if stage == "Sprout":
            for j, az in enumerate((0.35, math.pi + 0.35, 1.65)):
                rad = Vector((math.cos(az), math.sin(az), 0))
                key = "cotyledon" if j < 2 else "young"
                t0 = b.triangles
                _card_leaf(b, atlas, base + Vector((0, 0, 0.004)), rad, cfg["length"] * (1.0 if j < 2 else 0.75),
                           key, lod, phase, prng, math.radians(38 if j < 2 else 58), cup=0.04)
                stats["leaves"] += b.triangles - t0
            continue
        leaves = cfg["leaves"]
        if lod == 1:
            leaves = int(leaves * 0.62)
        elif lod == 2:
            leaves = max(3, int(leaves * 0.34))
        if lod == 2 and stage in ("Growing", "Mature", "Ripe"):
            t0 = b.triangles
            b.flat(base + Vector((0, 0, 0.055)), Vector((0, 0, 1)), cfg["length"] * 1.55,
                   atlas.uv("spray"), spin=prng.uniform(0, math.tau), cup=-0.10, phase=phase,
                   flutter=0.8, segs=2)
            stats["leaves"] += b.triangles - t0
        else:
            for i in range(leaves):
                az = i * 2.39996 + prng.uniform(-0.22, 0.22) + pidx * 0.35
                rad = Vector((math.cos(az), math.sin(az), 0))
                if abs(base.x + rad.x * cfg["length"]) > 0.515:
                    rad.x *= 0.55
                if abs(base.y + rad.y * cfg["length"]) > 0.515:
                    rad.y *= 0.55
                rad.normalize()
                key = prng.choice(cfg["keys"])
                if stage == "Ripe" and i > leaves * 0.68:
                    key = prng.choice(["yellow", "outer2"])
                if stage == "Mature" and i > leaves * 0.72:
                    key = "pale"
                elev = math.radians(prng.uniform(18, 38) if i < leaves * 0.6 else prng.uniform(35, 65))
                if stage == "Young":
                    elev = math.radians(prng.uniform(38, 65))
                length = cfg["length"] * prng.uniform(0.82, 1.08) * (0.72 if key == "pale" else 1.0)
                t0 = b.triangles
                _card_leaf(b, atlas, base + rad * 0.008 + Vector((0, 0, 0.006 + 0.004 * (i % 3))), rad,
                           length, key, lod, phase, prng, elev,
                           cup=0.22 if stage in ("Growing", "Mature", "Ripe") else 0.13,
                           fray=0.5 if key == "yellow" else 0.0)
                stats["leaves"] += b.triangles - t0
        if cfg["head"]:
            radius = {"Growing": 0.048, "Mature": 0.070, "Ripe": 0.090}[stage]
            height = {"Growing": 0.060, "Mature": 0.105, "Ripe": 0.155}[stage]
            if lod < 2:
                wrappers = 3 if stage in ("Mature", "Ripe") else 2
                for j in range(wrappers):
                    az = j * math.tau / wrappers + prng.uniform(-0.12, 0.12)
                    rad = Vector((math.cos(az), math.sin(az), 0))
                    t1 = b.triangles
                    key = "yellow" if stage == "Ripe" and j == 0 else ("outer2" if j % 2 else "outer")
                    _cupped_leaf_grid(b, atlas, base + rad * radius * 0.12 + Vector((0, 0, 0.076)), rad,
                                      radius * (1.35 if stage == "Ripe" else 1.10),
                                      radius * (1.05 if stage == "Ripe" else 0.88), key, phase, prng,
                                      elev=math.radians(60), droop=0.18, cup=0.026,
                                      rows=(8, 4)[lod], cols=(8, 4)[lod], flutter=0.45)
                    stats["leaves"] += b.triangles - t1
    print("HOMESTEAD_TRIS", stage, lod, stats)
    return b


def harvest(kit, atlas, material):
    """Hand-held produce: one cut cabbage head with loose outer leaves and a flat stalk cut.

    Pivot/GRIP is at the origin on the cut stalk; the head hangs down -Z from the hand.
    """
    rng = random.Random(SEED + 900)
    b = F.Batch(height=0.22)
    phase = 0.28
    b.tube([Vector((0, 0, 0)), Vector((0, 0, -0.025))], [0.013, 0.016], 12, atlas.uv("stem"),
           v_length=0.20, phase=phase, flutter=0.02, cap=True)
    b.flat(Vector((0, 0, 0.001)), Vector((0, 0, 1)), 0.040, atlas.uv("cut"), spin=0.2,
           cup=0.0, phase=phase, flutter=0.0, segs=3)
    head_center = Vector((0, 0, -0.097))
    _head(b, head_center, 0.077, 0.132, atlas.uv("head", inset=False), phase, 0, pointed=False,
          crown_rect=atlas.uv("crown"))
    _head_leaf_layers(b, atlas, head_center, 0.077, 0.132, phase, 0, rng, count=5)
    for i, az in enumerate((0.2, 2.35, 4.3)):
        rad = Vector((math.cos(az), math.sin(az), 0))
        key = "outer" if i != 1 else "yellow"
        _cupped_leaf_grid(b, atlas, Vector((0, 0, -0.040)) + rad * 0.016, rad,
                          rng.uniform(0.105, 0.130), rng.uniform(0.078, 0.095), key,
                          phase + 0.1 * i, rng, elev=math.radians(-34), droop=0.24,
                          cup=0.025, rows=8, cols=8, flutter=0.35)
    obj = b.build("SM_CropCabbage_Harvest", material)
    obj = kit.join([obj], "SM_CropCabbage_Harvest", pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    print("HOMESTEAD_LODS Harvest", F.lod_report([obj]))
    return obj


def _static_wind(obj):
    attr = obj.data.color_attributes.get("Wind") or obj.data.attributes.get("Wind")
    if attr:
        for datum in attr.data:
            datum.color = (0.0, 0.0, 0.0, 1.0)
        obj.data.color_attributes.active_color = attr
    return obj


def produce(kit, atlas, material):
    """Separate ripe-size firm cabbage head; pivot is at the base where it sits in the rosette."""
    rng = random.Random(SEED + 1200)
    m2 = material.copy()
    m2.name = "M_CropCabbageProduce"
    b = F.Batch(height=0.24)
    phase = 0.0
    radius = 0.100
    height = 0.150
    center = Vector((0, 0, height * 0.50))
    _head(b, center, radius, height, atlas.uv("head", inset=False), phase, 0,
          pointed=False, crown_rect=None)
    for j in range(8):
        _head_wrap_patch(b, atlas, center, radius, height, j * math.tau / 8.0 + rng.uniform(-0.05, 0.05),
                         span=rng.uniform(0.28, 0.34), twist=rng.uniform(-0.34, 0.34),
                         phase=phase + j * 0.015, rows=5, cols=4, offset=0.0035 + 0.0004 * j)
    b.flat(center + Vector((0, 0, height * 0.548)), Vector((0, 0, 1)), radius * 0.18,
           atlas.uv("crown"), spin=0.15, cup=-0.012, phase=phase, flutter=0.0, segs=2)
    for j in range(5):
        az = j * math.tau / 5.0 + rng.uniform(-0.08, 0.08)
        rad = Vector((math.cos(az), math.sin(az), 0))
        base = rad * rng.uniform(0.024, 0.042) + Vector((0, 0, 0.010 + 0.004 * (j % 2)))
        _cupped_leaf_grid(b, atlas, base, rad, rng.uniform(0.078, 0.098), rng.uniform(0.046, 0.058),
                          "pale", phase + 0.02 * j, rng, elev=math.radians(70),
                          droop=0.035, cup=0.013, rows=5, cols=4, flutter=0.02)
    obj = b.build("SM_CropCabbage_Produce", m2)
    obj = kit.join([obj], "SM_CropCabbage_Produce", pivot=None, unwrap=False,
                   reshade=True, smooth_angle=179.0)
    _static_wind(obj)
    print("HOMESTEAD_LODS Produce", F.lod_report([obj]))
    return obj


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(0.95, 1.10, 0.72))
    out = []
    for stage in STAGES:
        batches = [emit(stage, atlas, lod) for lod in range(3)]
        objs = F.finish_lods(kit, batches, f"SM_{NAME}_{stage}", material, smooth_angle=179.0)
        _restore_plot_origin(objs)
        out.extend(objs)
        print("HOMESTEAD_LODS", stage, F.lod_report(objs))
    out.append(produce(kit, atlas, material))
    out.append(harvest(kit, atlas, material))
    return out
