"""One 1 m plot of Early Horn / Altrincham carrots in five growth stages.

Real-object research (written before modeling):
- Historic short-horn carrots (Early Horn) and longer Altrincham carrots form slender orange
  taproots with a shoulder about 2.5-3.5 cm across visible at maturity. The edible shoulder pushes
  1-2 cm above loose soil while the rest of the root remains buried.
- Carrot foliage is a basal crown of petioles ending in triangular, finely dissected pinnate to
  bipinnate leaf blades. The mature top is dark, slightly blue-green, ferny, 25-30 cm tall and wide.
- Seedlings first show paired, grass-like cotyledons 2-4 cm tall, then a few small feathery true
  leaves. The foliage should be painted into cards rather than modeled leaflet by leaflet.
- Homestead plot layout follows SM_TilledBed: 1 m square, pivot bottom-centre, ridge tops along X at
  y=-0.3,0,+0.3 m, z about 4.5-5.5 cm. Twelve carrots are planted, x=-0.30,-0.10,+0.10,+0.30 on each ridge.

Game notes: walk-through (no collision). One 2K atlas and one material for all five stages
(alpha-masked, two-sided foliage). Wind vertex colours per homestead_foliage.py. The plant-stage
meshes carry only foliage/stems; the game places SM_CropCarrot_Produce on report anchors and scales
it from sunk pale-green young roots up to the ripe orange shoulder/taproot.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "CropCarrot"
DESCRIPTION = ("Twelve Early Horn / Altrincham carrots on a 1 m tilled-bed plot, authored as five "
               "growth-stage meshes from sprout to ripe with shared atlas/material.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.16), "views": ("hero", "detail"),
          "detail_distance": 1.6,
          "meshes": {
              "SM_CropCarrot_Growing": {"focus": (0.0, 0.0, 0.13), "detail_distance": 1.35},
              "SM_CropCarrot_Ripe": {"focus": (0.0, -0.02, 0.16), "eye_distance": 2.5,
                                      "detail_distance": 1.20},
              "SM_CropCarrot_Harvest": {"focus": (0.0, 0.0, -0.055), "pose": (0, 0, 0),
                                         "detail_distance": 0.60},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / stage height",
             "G": "per-plant random phase", "B": "leaf flutter 0 at petiole -> 1 at blade tip",
             "A": "1"},
    "material_notes": ("One material M_CropCarrot: T_CropCarrot_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency, B AO). "
                       "Two-sided masked foliage; carrot root and soil tiles are opaque in the same atlas."),
    "harvest_mesh": "SM_CropCarrot_Harvest pivot is the grip point at the leaf base/top of root; carrot hangs down -Z.",
}

SEED = 185011
RIDGES = (-0.30, 0.0, 0.30)
PLANT_X = (-0.30, -0.10, 0.10, 0.30)
GROUND_Z = 0.045
STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")


def _stage_name(stage):
    return "SM_" + NAME + "_" + stage


def _leaf_palettes():
    base = dict(yellow=(0.18, 0.17, 0.06), brown=(0.055, 0.040, 0.018))
    return {
        "frond": dict(base, base=(0.060, 0.135, 0.040), tip=(0.070, 0.160, 0.045),
                      vein=(0.075, 0.145, 0.055), margin=(0.046, 0.110, 0.034)),
        "frond_light": dict(base, base=(0.070, 0.160, 0.044), tip=(0.082, 0.178, 0.052),
                            vein=(0.092, 0.175, 0.066), margin=(0.052, 0.130, 0.038)),
        "frond_dark": dict(base, base=(0.048, 0.110, 0.034), tip=(0.058, 0.132, 0.040),
                           vein=(0.070, 0.132, 0.052), margin=(0.036, 0.090, 0.030)),
        "cotyledon": dict(base, base=(0.055, 0.105, 0.035), tip=(0.070, 0.125, 0.040),
                          vein=(0.090, 0.145, 0.055), margin=(0.045, 0.086, 0.030)),
    }


def _narrow_leaf(X, Y, px, nrng, pal):
    return F.paint_blade(X, Y, nrng,
                         F.ovate(width=0.085, widest=0.45, tip_sharp=0.75, base_round=0.35,
                                 base=0.0, tip=0.99),
                         [([(0, 0.0), (0, 0.99)], 1.0)], pal, px, vein_width=0.010,
                         vein_depth=0.00005, puff=0.00004, tertiary=0.0, trans=0.62)


def _paint_frond(atlas, key, nrng, rng, pal, young=False):
    """Paint one airy carrot leaf card: a very thin rachis with many bipinnate/tripinnate lobes."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    # Rachis / midrib.
    bend = 0.025 * np.sin(Y * math.pi * 1.4)
    d, along = F.polyline_distance(X, Y, [(0.0, 0.0), (0.01, 0.25), (-0.01, 0.62), (0.0, 0.98)])
    rach = F.Layer(X.shape)
    rach.color[...] = pal["vein"]
    rach.alpha = np.clip(-(d - 0.0026 * (1 - 0.45 * along)) / px + 0.5, 0, 1)
    rach.height = 0.00008 * rach.alpha
    rach.rough[...] = 0.58
    rach.trans[...] = 0.35
    layer.over(rach)
    # Needle-narrow lobes arranged along pinnae and secondary pinnae. The empty alpha
    # between them is what makes the crop read like carrot lace instead of a fern frond.
    leaflet = F.ovate(width=0.020 if not young else 0.026, widest=0.34, tip_sharp=0.95,
                      base_round=0.25, base=0.0, tip=1.0)
    count = 11 if young else 20
    for k in range(count):
        t = (k + 0.55) / count
        y = 0.055 + 0.86 * t
        half = X.max() * (0.35 + 0.85 * math.sin(math.pi * min(t * 1.05, 1.0))) * (1.0 - 0.25 * t)
        for side in (-1, 1):
            base_angle = side * math.radians(48 + 25 * (1 - t) + rng.uniform(-8, 8))
            branch_len = half * rng.uniform(0.78, 1.10)
            origin = (bend[int(np.clip(y * (X.shape[0] - 1), 0, X.shape[0] - 1)), 0], y)
            for j, (frac, length_scale, offset_angle) in enumerate(((0.00, 0.58, 0.00), (0.22, 0.34, 0.58),
                                                                    (0.38, 0.30, -0.54), (0.56, 0.25, 0.46),
                                                                    (0.72, 0.20, -0.40))):
                ln = branch_len * length_scale * rng.uniform(0.82, 1.12)
                oo = (origin[0] + side * branch_len * frac * math.sin(abs(base_angle)),
                      origin[1] + branch_len * frac * math.cos(abs(base_angle)) * 0.18)
                th = base_angle + side * offset_angle
                centre = (oo[0] + ln * 0.50 * math.sin(th), oo[1] + ln * 0.50 * math.cos(th))
                if centre[1] < 0.02 or centre[1] > 0.98:
                    continue
                S.over_window(layer, X, Y, centre, ln * 0.72,
                              lambda Xs, Ys, oo=oo, th=th, ln=ln: F.paint_blade(
                                  *S.rotated(Xs, Ys, oo, th, ln), nrng, leaflet,
                                  [([(0, 0), (0, 1)], 1.0)], pal, px / max(ln, 1e-5),
                                  vein_width=0.040, vein_depth=0.00005, puff=0.00006,
                                  tertiary=0.0, hair=0.18, trans=0.62))
    n = F.noise(X.shape, nrng, freq=28.0, beta=1.8)
    layer.color = F.lerp(layer.color, np.asarray(layer.color) * 0.83, (n > 0.72) * 0.08)
    layer.rough = np.where(layer.alpha > 0, np.clip(layer.rough + 0.04 * n, 0, 1), layer.rough)
    atlas.put(key, layer, meters_per_px=(0.18 if not young else 0.10) / X.shape[0])


def _paint_carrot_root(atlas, key, nrng):
    X, Y, px = atlas.grid(key)
    U = X / (2 * X.max()) + 0.5
    V = np.clip(Y, 0, 1)
    n = F.noise(X.shape, nrng, freq=18.0, beta=1.8)
    rings = 0.5 + 0.5 * np.sin(V * math.tau * 10.0 + 0.5 * np.sin(U * math.tau * 3))
    root = F.lerp((0.52, 0.145, 0.020), (0.82, 0.245, 0.035), n)
    green_crown = F.smoothstep(0.86, 0.98, V)
    soil = F.smoothstep(0.0, 0.12, V) * (1 - F.smoothstep(0.18, 0.32, V))
    layer = F.Layer(X.shape)
    layer.color = F.lerp(root, (0.34, 0.20, 0.09), rings * 0.035)
    layer.color = F.lerp(layer.color, (0.070, 0.095, 0.040), green_crown * 0.55)
    layer.color = F.lerp(layer.color, (0.13, 0.085, 0.040), soil * 0.18)
    layer.alpha[...] = 1
    layer.height = 0.00018 * n + 0.00005 * rings
    layer.rough = 0.58 + 0.08 * rings
    layer.trans[...] = 0.0
    atlas.put(key, layer, meters_per_px=0.040 / X.shape[0], opaque=True)


def _paint_stem_column(atlas, nrng):
    U, V = atlas.column_grid("stem")
    n = F.noise(U.shape, nrng, freq=72.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.045, 0.082, 0.032), (0.070, 0.110, 0.045), n * 0.65)
    layer.height = 0.00006 * n
    layer.rough = 0.58 + 0.06 * n
    layer.trans[...] = 0.32
    atlas.put("stem", layer, meters_per_px=0.35 / U.shape[0], opaque=True)


def _paint_soil_tile(atlas, nrng):
    X, Y, px = atlas.grid("soil")
    n = F.noise(X.shape, nrng, freq=24.0, beta=1.7)
    grit = F.smoothstep(0.78, 0.96, F.noise(X.shape, nrng, freq=115.0, beta=1.1))
    layer = F.Layer(X.shape)
    layer.color = F.lerp((0.080, 0.052, 0.030), (0.155, 0.105, 0.058), n)
    layer.color = F.lerp(layer.color, (0.055, 0.038, 0.025), grit * 0.35)
    layer.alpha[...] = 1
    layer.height = 0.00035 * n + 0.00010 * grit
    layer.rough = 0.78 + 0.10 * grit
    layer.trans[...] = 0.0
    atlas.put("soil", layer, meters_per_px=0.020 / X.shape[0], opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.tile("cotyledon", 120, 520)
    atlas.tile("lobe", 70, 360)
    atlas.tile("frond_young", 330, 780)
    atlas.tile("frond", 420, 980)
    atlas.tile("frond_light", 420, 980)
    atlas.tile("frond_dark", 420, 980)
    atlas.tile("root", 220, 220)
    atlas.tile("soil", 160, 160)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _leaf_palettes()
    X, Y, px = atlas.grid("cotyledon")
    atlas.put("cotyledon", _narrow_leaf(X, Y, px, nrng, pals["cotyledon"]), meters_per_px=0.040 / X.shape[0])
    X, Y, px = atlas.grid("lobe")
    atlas.put("lobe", _narrow_leaf(X, Y, px, nrng, pals["frond_light"]), meters_per_px=0.055 / X.shape[0])
    _paint_frond(atlas, "frond_young", nrng, rng, pals["frond_light"], young=True)
    _paint_frond(atlas, "frond", nrng, rng, pals["frond"], young=False)
    _paint_frond(atlas, "frond_light", nrng, rng, pals["frond_light"], young=False)
    _paint_frond(atlas, "frond_dark", nrng, rng, pals["frond_dark"], young=False)
    _paint_carrot_root(atlas, "root", nrng)
    _paint_soil_tile(atlas, nrng)
    _paint_stem_column(atlas, nrng)
    atlas.save()
    return atlas


def _plot_positions(rng):
    plants = []
    for y in RIDGES:
        for x in PLANT_X:
            plants.append(Vector((x + rng.uniform(-0.008, 0.008),
                                  y + rng.uniform(-0.009, 0.009),
                                  GROUND_Z + rng.uniform(-0.002, 0.002))))
    return plants


def _produce_anchor_rows(shift=(0.0, 0.0, 0.0)):
    rng = random.Random(SEED + 704)
    offset = Vector(shift)
    rows = []
    for base in _plot_positions(random.Random(SEED + 53)):
        local = base - offset
        rows.append([round(local.x, 4), round(local.y, 4), round(local.z, 4),
                     round(rng.uniform(0.0, 360.0), 1), round(rng.uniform(0.86, 1.08), 3)])
    return rows


REPORT["produce"] = {
    "mesh": _stage_name("Produce"),
    "anchors": {stage: [list(row) for row in _produce_anchor_rows()]
                for stage in ("Young", "Growing", "Mature", "Ripe")},
}


STAGE_PARAMS = {
    "Sprout": dict(height=0.045, leaves=0, leaf_len=0.034, petiole=0.006, root=0.0),
    "Young": dict(height=0.105, leaves=3, leaf_len=0.078, petiole=0.030, root=0.0),
    "Growing": dict(height=0.230, leaves=5, leaf_len=0.165, petiole=0.078, root=0.0),
    "Mature": dict(height=0.330, leaves=5, leaf_len=0.205, petiole=0.105, root=0.0),
    "Ripe": dict(height=0.330, leaves=5, leaf_len=0.205, petiole=0.105, root=0.0145),
    "HarvestTop": dict(height=0.220, leaves=5, leaf_len=0.120, petiole=0.050, root=0.0),
}


def _edge_scale(base, heading, reach, margin=0.455):
    allowed = 9.0
    if abs(heading.x) > 1e-4:
        allowed = min(allowed, ((margin - base.x) if heading.x > 0 else (-margin - base.x)) / heading.x)
    if abs(heading.y) > 1e-4:
        allowed = min(allowed, ((margin - base.y) if heading.y > 0 else (-margin - base.y)) / heading.y)
    if allowed <= 0:
        return 0.48
    return min(1.0, max(0.52, allowed / max(reach, 1e-4)))


def _arc(base, heading, elev, length, droop, n):
    d = (heading * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    side = heading.cross(Vector((0, 0, 1))).normalized()
    pts = [Vector(base)]
    for i in range(n):
        d = (Matrix.Rotation(-droop / max(n, 1), 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / max(n, 1)))
    return pts


def _emit_sprout(b, atlas, base, heading, phase, rng, lod):
    for side_sign in (-1, 1):
        az = side_sign * 0.72 + rng.uniform(-0.12, 0.12)
        d = Matrix.Rotation(az, 3, Vector((0, 0, 1))) @ heading
        up = (Vector((0, 0, 1)) + d * 0.15).normalized()
        b.card(base + Vector((0, 0, 0.010)), d, up, 0.035, 0.035 * S.tile_aspect(atlas, "cotyledon"),
               atlas.uv("cotyledon"), rows=(2, 1, 1)[lod], cols=1, fold=0.02, droop=0.03,
               phase=phase, flutter=1.0, flutter_base=0.1)
    if lod < 2:
        d = (heading + Vector((0, 0, 0.85))).normalized()
        b.card(base + Vector((0, 0, 0.016)), d, (Vector((0, 0, 1)) - heading * 0.2).normalized(),
               0.026, 0.026 * S.tile_aspect(atlas, "frond_young"), atlas.uv("frond_young"),
               rows=1, cols=1, fold=0.01, droop=0.03, phase=phase, flutter=1.0, flutter_base=0.2)


def _emit_frond(b, atlas, base, heading, phase, rng, stage, lod):
    p = STAGE_PARAMS[stage]
    elev = math.radians(rng.uniform(56, 78) if stage == "Growing" else
                        (rng.uniform(66, 86) if stage in ("Mature", "Ripe", "HarvestTop") else rng.uniform(48, 75)))
    edge_scale = _edge_scale(base, heading, p["petiole"] + p["leaf_len"])
    pet = p["petiole"] * rng.uniform(0.75, 1.10) * edge_scale
    pts = _arc(base + Vector((0, 0, 0.003)), heading, elev, pet, rng.uniform(0.12, 0.38), (3, 2, 1)[lod])
    b.tube(pts, [0.00125 * (1 - 0.30 * i / max(len(pts) - 1, 1)) for i in range(len(pts))],
           3, atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.12)
    if stage == "Young":
        key = "frond_young"
    else:
        key = rng.choices(["frond", "frond_light", "frond_dark"], [50, 24, 26])[0]
    leaf_elev = math.radians(rng.uniform(42, 68) if stage == "Growing" else
                             (rng.uniform(56, 80) if stage in ("Mature", "Ripe", "HarvestTop") else rng.uniform(30, 62)))
    d = (heading * math.cos(leaf_elev) + Vector((0, 0, math.sin(leaf_elev)))).normalized()
    up = (Vector((0, 0, 1)) - heading * rng.uniform(0.25, 0.65)).normalized()
    length = p["leaf_len"] * rng.uniform(0.82, 1.10) * (0.78 if stage == "Young" else 1.0) * edge_scale
    width = length * S.tile_aspect(atlas, key) * (1.05 if lod == 2 else 1.0)
    b.card(pts[-1], d, up, length, width, atlas.uv(key), rows=(3, 2, 1)[lod],
           cols=1, fold=0.015 if lod == 0 else 0.0,
           droop=rng.uniform(0.05, 0.25) + (0.10 if stage in ("Mature", "Ripe", "HarvestTop") else 0.0),
           twist=rng.uniform(-0.22, 0.22) if lod == 0 else 0.0, phase=phase, flutter=1.0,
           flutter_base=0.18)
    if lod == 0 and stage in ("Growing", "Mature", "Ripe", "HarvestTop"):
        side = d.cross(up)
        if side.length < 1e-4:
            side = d.cross(Vector((0, 0, 1)))
        side.normalize()
        lobe_count = 1 if stage == "Growing" else 2
        for j in range(lobe_count):
            t = (j + 1) / (lobe_count + 1)
            for sign in (-1, 1):
                if stage == "Growing" and j == lobe_count - 1 and sign < 0:
                    continue
                node = pts[-1] + d * (length * (0.20 + 0.62 * t))
                bd = (d * 0.42 + side * sign * (0.72 - 0.15 * t) + Vector((0, 0, 0.12))).normalized()
                ln = length * rng.uniform(0.10, 0.17) * (1.0 - 0.20 * t)
                b.card(node, bd, up, ln, ln * S.tile_aspect(atlas, "lobe"), atlas.uv("lobe"),
                       rows=1, cols=1, fold=0.0, droop=0.03, twist=rng.uniform(-0.12, 0.12),
                       phase=phase, flutter=1.0, flutter_base=0.35)


def emit(stage, atlas, lod):
    rng = random.Random(SEED + 100 * STAGES.index(stage) + lod)
    p = STAGE_PARAMS[stage]
    b = F.Batch(height=GROUND_Z + p["height"])
    stats = {"leaves": 0, "roots": 0}
    for pi, base in enumerate(_plot_positions(random.Random(SEED + 53))):
        phase = (pi * 0.071 + 0.33) % 1.0
        az0 = rng.uniform(0, math.tau)
        t0 = b.triangles
        if stage == "Sprout":
            _emit_sprout(b, atlas, base, Vector((math.cos(az0), math.sin(az0), 0)), phase, rng, lod)
        else:
            leaf_count = p["leaves"]
            if lod == 1:
                leaf_count = max(2, int(leaf_count * 0.58))
            elif lod == 2:
                leaf_count = max(1, int(leaf_count * 0.30))
            for i in range(leaf_count):
                az = az0 + i * 2.39996 + rng.uniform(-0.28, 0.28)
                _emit_frond(b, atlas, base, Vector((math.cos(az), math.sin(az), 0)), phase, rng, stage, lod)
        stats["leaves"] += b.triangles - t0
    print("HOMESTEAD_TRIS", stage, lod, stats)
    return b


def _zero_wind(obj):
    attr = obj.data.attributes.get("Wind")
    if attr:
        values = []
        for i in range(len(attr.data)):
            g = ((i * 41) % 251) / 251.0
            values.extend((0.0, g, 0.0, 1.0))
        attr.data.foreach_set("color_srgb", values)
        obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]


def emit_produce(atlas):
    """Ripe carrot shoulder/taproot used by the game as a separately scaled/tinted instance."""
    rng = random.Random(SEED + 808)
    b = F.Batch(height=1.0)
    phase = 0.0
    u0, u1, v0, v1 = atlas.uv("root", inset=False)
    sides = 16
    profile = [
        (0.018, 0.0100),
        (0.010, 0.0190),
        (0.000, 0.0225),
        (-0.025, 0.0200),
        (-0.060, 0.0150),
        (-0.102, 0.0076),
        (-0.140, 0.0015),
    ]
    rings = []
    for ri, (z, rad) in enumerate(profile):
        t = ri / (len(profile) - 1)
        center = Vector((0.0025 * math.sin(t * math.pi * 1.1), -0.0015 * math.sin(t * math.pi * 0.7), z))
        row = []
        for j in range(sides):
            a = math.tau * j / sides
            shoulder = 1.0 + 0.055 * math.sin(3 * a + 0.4) + 0.025 * math.sin(7 * a + 1.1)
            row.append(b._vert(center + Vector((math.cos(a) * rad * shoulder,
                                                math.sin(a) * rad * (0.93 + 0.05 * math.cos(a)),
                                                0)), phase, 0.0))
        rings.append(row)
    for ri in range(len(rings) - 1):
        ta, tb = ri / (len(rings) - 1), (ri + 1) / (len(rings) - 1)
        va, vb = v1 + (v0 - v1) * ta, v1 + (v0 - v1) * tb
        for j in range(sides):
            k = (j + 1) % sides
            b.faces.append((rings[ri][j], rings[ri][k], rings[ri + 1][k], rings[ri + 1][j]))
            ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
            b.uvs.append([(ua, va), (ub, va), (ub, vb), (ua, vb)])
    top = b._vert(Vector((0, 0, 0.020)), phase, 0.0)
    tip = b._vert(Vector((0.0025, -0.0015, -0.144)), phase, 0.0)
    for j in range(sides):
        k = (j + 1) % sides
        ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
        b.faces.append((top, rings[0][j], rings[0][k]))
        b.uvs.append([((u0 + u1) * 0.5, v1), (ua, v1), (ub, v1)])
        b.faces.append((rings[-1][j], tip, rings[-1][k]))
        b.uvs.append([(ua, v0), ((u0 + u1) * 0.5, v0), (ub, v0)])
    for i in range(5):
        az = i * 2.39996 + rng.uniform(-0.20, 0.20)
        radial = Vector((math.cos(az), math.sin(az), 0))
        start = radial * rng.uniform(0.002, 0.007) + Vector((0, 0, 0.018 + rng.uniform(-0.001, 0.002)))
        mid = start + radial * rng.uniform(0.003, 0.008) + Vector((0, 0, rng.uniform(0.006, 0.010)))
        end = mid + radial * rng.uniform(0.004, 0.010) + Vector((0, 0, rng.uniform(0.005, 0.010)))
        radius = rng.uniform(0.0011, 0.0018)
        b.tube([start, mid, end], [radius, radius * 0.75, radius * 0.45], 4,
               atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.0, cap=True)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(atlas):
    """Hand-held pulled carrot: grip at origin, root hangs down -Z, trimmed fern tuft above."""
    rng = random.Random(SEED + 909)
    b = F.Batch(height=0.24)
    phase = 0.37
    # 12 cm pulled carrot: broad shoulder just below the fist, tapering to a dirty point.
    u0, u1, v0, v1 = atlas.uv("root", inset=False)
    sides = 12
    profile = [(0.000, 0.0155), (0.012, 0.0160), (0.035, 0.0130), (0.065, 0.0090),
               (0.095, 0.0046), (0.118, 0.0012)]
    rings = []
    for ri, (down, rad) in enumerate(profile):
        t = ri / (len(profile) - 1)
        center = Vector((0.0030 * math.sin(t * math.pi * 1.2), -0.0015 * math.sin(t * math.pi * 0.8),
                         -0.004 - down))
        row = []
        for j in range(sides):
            a = math.tau * j / sides
            oval = 1.0 + 0.08 * math.sin(3 * a + 0.6) + 0.04 * math.sin(7 * a)
            co = center + Vector((math.cos(a) * rad * oval, math.sin(a) * rad * (0.92 + 0.05 * math.cos(a)), 0))
            row.append(b._vert(co, phase, 0.1))
        rings.append(row)
    for ri in range(len(rings) - 1):
        ta, tb = ri / (len(rings) - 1), (ri + 1) / (len(rings) - 1)
        va, vb = v1 + (v0 - v1) * ta, v1 + (v0 - v1) * tb
        for j in range(sides):
            k = (j + 1) % sides
            b.faces.append((rings[ri][j], rings[ri][k], rings[ri + 1][k], rings[ri + 1][j]))
            ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
            b.uvs.append([(ua, va), (ub, va), (ub, vb), (ua, vb)])
    top = b._vert(Vector((0, 0, -0.003)), phase, 0.1)
    tip = b._vert(Vector((0.003, -0.0015, -0.126)), phase, 0.1)
    for j in range(sides):
        k = (j + 1) % sides
        ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
        b.faces.append((top, rings[0][k], rings[0][j]))
        b.uvs.append([((u0 + u1) * 0.5, v1), (ub, v1), (ua, v1)])
        b.faces.append((rings[-1][j], tip, rings[-1][k]))
        b.uvs.append([(ua, v0), ((u0 + u1) * 0.5, v0), (ub, v0)])
    for k in range(5):
        az = k * 2.39996 + rng.uniform(-0.3, 0.3)
        c = Vector((math.cos(az) * rng.uniform(0.002, 0.010),
                    math.sin(az) * rng.uniform(0.002, 0.010),
                    -0.112 + rng.uniform(-0.003, 0.005)))
        b.sphere(c, rng.uniform(0.0018, 0.0038), atlas.uv("soil", inset=False),
                 segs=4, rings=3, stretch=rng.uniform(0.7, 1.4), axis=Vector((0, 0, 1)), phase=phase)
    for k in range(6):
        az = k * 2.39996 + rng.uniform(-0.25, 0.25)
        start = Vector((math.cos(az) * 0.006, math.sin(az) * 0.006, -0.100 - rng.uniform(0, 0.006)))
        end = start + Vector((math.cos(az) * rng.uniform(0.006, 0.015),
                              math.sin(az) * rng.uniform(0.006, 0.015),
                              -rng.uniform(0.015, 0.030)))
        b.tube([start, start.lerp(end, 0.5), end], [0.00036, 0.00024, 0.00012], 3,
               atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.05)
    # Trimmed ferny top, roughly 15-20 cm long from the fist.
    for i in range(5):
        az = i * 2.39996 + rng.uniform(-0.24, 0.24)
        _emit_frond(b, atlas, Vector((0, 0, 0)), Vector((math.cos(az), math.sin(az), 0)),
                    phase, rng, "HarvestTop", 0)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    produce_material = material.copy()
    produce_material.name = "M_CropCarrotProduce"
    objects = []
    for stage in STAGES:
        batches = [emit(stage, atlas, lod) for lod in range(3)]
        lods = F.finish_lods(kit, batches, _stage_name(stage), material, smooth_angle=179.0)
        if stage in REPORT["produce"]["anchors"]:
            REPORT["produce"]["anchors"][stage] = _produce_anchor_rows(lods[0].get("homestead_shift", (0, 0, 0)))
        print("HOMESTEAD_LODS", stage, F.lod_report(lods))
        objects.extend(lods)
    harvest = emit_harvest(atlas).build(_stage_name("Harvest"), material)
    harvest = kit.join([harvest], _stage_name("Harvest"), pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    harvest.data.color_attributes.active_color = harvest.data.color_attributes["Wind"]
    objects.append(harvest)
    produce = emit_produce(atlas).build(_stage_name("Produce"), produce_material)
    produce = kit.join([produce], _stage_name("Produce"), pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    _zero_wind(produce)
    objects.append(produce)
    return objects
