"""Weed clumps for Homestead's "Weeds [E] Pull" nodes.

Real plant research (what the geometry and textures follow):
- Broad-leaved dock (Rumex obtusifolius) is a common Cornish pasture and yard weed of damp,
  trampled, enriched ground. In spring it shows a large basal rosette, often 60-75 cm across,
  of long oblong to lanceolate leaves 25-35 cm long with rounded bases, blunt tips, strong pale
  midribs, uneven wavy margins, reddish midribs or red-brown flushing on older blades, slug holes,
  yellowing and brown edge scorch. Last year's fruiting stems can persist into spring as dry
  reddish-brown, branched or simple seed spikes 60-80 cm tall.
- Spear/creeping thistles (Cirsium/Carduus-type pasture thistles) form ground rosettes of
  grey-green, deeply lobed spiny leaves, with pale thorny margins and a cottony look. By late
  spring a few 40-60 cm ribbed stems carry tight purple buds or purple brush-like flower heads.
  Common ragwort (Senecio jacobaea) grows in the same rough pasture: dark ragged pinnate leaves,
  50-70 cm stems, and flat corymbs of bright yellow daisy heads. The ragwort yellow is the
  unmistakable long-range cue that this is not grass.
- Dandelions (Taraxacum officinale) make low basal rosettes of deeply toothed leaves, yellow
  flower heads on hollow 15-25 cm stalks, and round white seed clocks. Ribwort plantain
  (Plantago lanceolata) contributes narrow ribbed leaves and brown seed clubs; creeping buttercup
  (Ranunculus repens) adds low runners, three-lobed leaves and glossy yellow five-petalled flowers.

Game notes: walk-through (no collision). Three separate meshes share one 2K numpy-painted atlas
and one two-sided masked material M_WeedClump. Wind vertex colours follow homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "WeedClump"
DESCRIPTION = ("Three unmistakable spring weed clumps: broad-leaved dock with reddish seed spikes, "
               "a spiny thistle rosette with purple heads and bright yellow ragwort, and a low mixed "
               "dandelion/plantain/buttercup clump. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 15000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"], "eye_distance": 5.5, "meshes": {
    "SM_WeedClump_Dock": {"focus": (0.02, -0.01, 0.27), "eye_distance": 5.8,
                          "detail_distance": 0.78, "detail_fstop": 28.0},
    "SM_WeedClump_Thistle": {"focus": (0.04, -0.02, 0.26), "eye_distance": 5.6,
                             "detail_distance": 1.65, "detail_fstop": 32.0},
    "SM_WeedClump_Dandelion": {"focus": (0.0, 0.0, 0.13), "eye_distance": 4.8,
                               "detail_distance": 0.62, "detail_fstop": 28.0},
}}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / clump height",
             "G": "per-stem or per-rosette random phase",
             "B": "flutter 0 at leaf/stem base -> 1 at leaf or flower tip", "A": "1"},
    "material_notes": ("One material M_WeedClump: T_WeedClump_basecolor (sRGB, alpha opacity mask), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency, "
                       "B AO). Masked, two-sided foliage; roughness painted around 0.52-0.72."),
}

SEED = 18500929
MESHES = {
    "dock": ("SM_WeedClump_Dock", 0.80),
    "thistle": ("SM_WeedClump_Thistle", 0.72),
    "dandelion": ("SM_WeedClump_Dandelion", 0.33),
}
UP = Vector((0, 0, 1))


def _shape_wavy(width, widest=0.45, base=0.03, tip=0.97, amp=0.05, waves=8.0):
    def shape(X, Y):
        s = np.clip((Y - base) / max(tip - base, 1e-6), 0.0, 1.0)
        lower = np.sin(np.clip(s / widest, 0, 1) * math.pi * 0.5) ** 0.55
        upper = np.sin(np.clip((1 - s) / (1 - widest), 0, 1) * math.pi * 0.5) ** 0.45
        hw = width * np.where(s < widest, lower, upper)
        hw *= 1.0 + amp * np.sin(waves * math.pi * s + 0.8) + amp * 0.45 * np.sin(waves * 1.9 * math.pi * s)
        inside = np.minimum(hw - np.abs(X), np.minimum(Y - base, tip - Y) * 3.0)
        return inside, s, hw
    return shape


def _shape_lobed(width, teeth=8, depth=0.38, base=0.02, tip=0.985, backcut=0.0):
    def shape(X, Y):
        s = np.clip((Y - base) / max(tip - base, 1e-6), 0.0, 1.0)
        body = (np.sin(math.pi * s) ** 0.52) * (0.35 + 0.65 * (1 - 0.20 * s))
        lobes = 1.0 + depth * np.sin((s * teeth + 0.15) * math.tau)
        serr = 1.0 + 0.10 * np.sin((s * teeth * 3.0 + np.where(X > 0, 0.3, 0.0)) * math.tau)
        hw = width * body * np.clip(lobes * serr, 0.38, 1.45)
        if backcut:
            bite = backcut * (np.sin(s * teeth * math.pi) ** 2) * np.clip((np.abs(X) / np.maximum(hw, 1e-6) - 0.52) / 0.45, 0, 1)
            hw *= (1.0 - bite)
        inside = np.minimum(hw - np.abs(X), np.minimum(Y - base, tip - Y) * 3.0)
        return inside, s, hw
    return shape


def _shape_plantain(width):
    return F.ovate(width=width, widest=0.47, tip_sharp=0.85, base_round=0.55, base=0.015, tip=0.985)


def _paint_leaf(atlas, key, nrng, shape, veins, pal, meters, **kwargs):
    X, Y, px = atlas.grid(key)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, **kwargs)
    atlas.put(key, layer, meters_per_px=meters / X.shape[0])


def _paint_daisy(atlas, key, nrng, base_col, eye_col, petals=13, notch=0.0):
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    r = np.hypot(Xc, Yc) / 0.5
    th = np.arctan2(Xc, Yc)
    lobe = np.mod(th / math.tau * petals + 0.5, 1.0) - 0.5
    rim = 0.72 + 0.22 * np.exp(-(lobe / 0.20) ** 2) - notch * np.exp(-(lobe / 0.045) ** 2)
    n = F.noise(X.shape, nrng, freq=30.0, beta=1.7)
    layer = F.Layer(X.shape)
    layer.alpha = np.clip((rim + 0.025 * (n - 0.5) - r) / (px * 2.0) + 0.5, 0, 1)
    ray = np.exp(-(lobe / 0.10) ** 2) * F.smoothstep(0.16, 0.9, r)
    col = F.lerp(np.asarray(base_col) * 0.86, base_col, n)
    col = F.lerp(col, np.asarray(base_col) * 1.18, ray * 0.34)
    col = F.lerp(col, eye_col, np.clip(1.0 - r / 0.20, 0, 1))
    layer.color = col
    layer.height = 0.00010 * (1 - r) + 0.00004 * ray
    layer.rough = 0.55 + 0.08 * n
    layer.trans[...] = 0.45
    atlas.put(key, layer, meters_per_px=0.04 / X.shape[0])


def _paint_seed_clock(atlas, key, nrng):
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    r = np.hypot(Xc, Yc) / 0.5
    th = np.arctan2(Xc, Yc)
    spokes = np.maximum(0, np.cos(th * 24.0 + 0.6 * np.sin(th * 7.0))) ** 5
    fuzz = F.smoothstep(0.12, 0.95, r) * spokes + 0.35 * F.smoothstep(0.45, 0.98, F.noise(X.shape, nrng, freq=70.0))
    ball = np.clip((1.0 - r) / (px * 4.0) + 0.5, 0, 1) * np.clip(fuzz + (r < 0.18) * 1.0, 0, 1)
    layer = F.Layer(X.shape)
    layer.alpha = ball
    layer.color = F.lerp((0.42, 0.38, 0.30), (0.86, 0.82, 0.68), np.clip(fuzz, 0, 1))
    layer.color = F.lerp(layer.color, (0.18, 0.12, 0.06), np.clip(1 - r / 0.16, 0, 1))
    layer.height = 0.00006 * fuzz
    layer.rough[...] = 0.72
    layer.trans[...] = 0.30
    atlas.put(key, layer, meters_per_px=0.055 / X.shape[0])


def _paint_disc(atlas, key, nrng, color, edge, meters=0.035):
    X, Y, px = atlas.grid(key)
    r = np.hypot(X, Y - 0.5) / 0.5
    n = F.noise(X.shape, nrng, freq=36.0, beta=1.6)
    layer = F.Layer(X.shape)
    layer.alpha = np.clip((1.0 - r) / (px * 2.0) + 0.5, 0, 1)
    layer.color = F.lerp(color, edge, F.smoothstep(0.65, 1.0, r)) * (0.88 + 0.24 * n)[..., None]
    layer.height = 0.00005 * n
    layer.rough = 0.60 + 0.12 * n
    layer.trans[...] = 0.20
    atlas.put(key, layer, meters_per_px=meters / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    for key, width in (("stem_green", 48), ("stem_red", 48), ("stem_dead", 56), ("stem_thistle", 56),
                       ("stem_ragwort", 48), ("stolon", 40)):
        atlas.column(key, width)
    for key in ("dock_green", "dock_red", "dock_old"):
        atlas.tile(key, 310, 780)
    for key in ("thistle_leaf", "thistle_leaf_pale"):
        atlas.tile(key, 360, 720)
    for key in ("ragwort_leaf", "dandelion_leaf"):
        atlas.tile(key, 260, 650)
    atlas.tile("plantain_leaf", 160, 720)
    atlas.tile("buttercup_leaf", 260, 360)
    for key in ("ragwort_flower", "dandelion_flower", "buttercup_flower", "thistle_head", "seed_clock",
                "dock_seed", "plantain_seed"):
        atlas.tile(key, 260, 260)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    dock_shape = _shape_wavy(0.18, widest=0.42, amp=0.055, waves=8.0)
    dock_veins = F.pinnate_veins(count=8, angle=0.7, curve=0.25, reach=0.88, width=0.18, widest=0.42,
                                 start=0.10, stop=0.82, rng=nrng)
    dock_pal = dict(base=(0.050, 0.088, 0.026), tip=(0.062, 0.105, 0.030), vein=(0.075, 0.125, 0.048),
                    margin=(0.040, 0.066, 0.020), stalk=(0.090, 0.055, 0.036), brown=(0.11, 0.055, 0.025),
                    yellow=(0.23, 0.20, 0.055))
    _paint_leaf(atlas, "dock_green", nrng, dock_shape, dock_veins, dock_pal, 0.34,
                vein_width=0.010, vein_depth=0.00028, puff=0.00024, tertiary=0.30, damage=0.10,
                holes=0.22, stalk=0.020, trans=0.40)
    _paint_leaf(atlas, "dock_red", nrng, dock_shape, dock_veins,
                dict(dock_pal, base=(0.070, 0.070, 0.028), tip=(0.105, 0.050, 0.030),
                     vein=(0.17, 0.055, 0.035), margin=(0.058, 0.050, 0.020)), 0.34,
                vein_width=0.011, vein_depth=0.00030, puff=0.00022, tertiary=0.28, damage=0.16,
                holes=0.26, yellow=0.12, edge_burn=0.25, stalk=0.020, trans=0.35)
    _paint_leaf(atlas, "dock_old", nrng, dock_shape, dock_veins,
                dict(dock_pal, base=(0.070, 0.080, 0.025), tip=(0.090, 0.076, 0.020)), 0.34,
                vein_width=0.011, vein_depth=0.00030, puff=0.00022, tertiary=0.28, damage=0.24,
                holes=0.50, yellow=0.34, edge_burn=0.50, stalk=0.020, trans=0.25)

    th_shape = _shape_lobed(0.20, teeth=8, depth=0.48, backcut=0.18)
    th_veins = F.pinnate_veins(count=7, angle=0.8, curve=0.25, reach=0.95, width=0.20, widest=0.50, rng=nrng)
    th_pal = dict(base=(0.050, 0.080, 0.050), tip=(0.060, 0.095, 0.062), vein=(0.16, 0.18, 0.135),
                  margin=(0.20, 0.205, 0.15), stalk=(0.075, 0.070, 0.055), brown=(0.10, 0.055, 0.025))
    _paint_leaf(atlas, "thistle_leaf", nrng, th_shape, th_veins, th_pal, 0.32,
                vein_width=0.010, vein_depth=0.00025, puff=0.00016, tertiary=0.38, hair=0.35,
                damage=0.06, stalk=0.014, trans=0.32)
    _paint_leaf(atlas, "thistle_leaf_pale", nrng, th_shape, th_veins,
                dict(th_pal, base=(0.060, 0.090, 0.065), tip=(0.075, 0.105, 0.075),
                     margin=(0.27, 0.26, 0.19)), 0.32,
                vein_width=0.010, vein_depth=0.00025, puff=0.00014, tertiary=0.40, hair=0.50,
                yellow=0.05, stalk=0.014, trans=0.30)

    rag_shape = _shape_lobed(0.12, teeth=7, depth=0.38, backcut=0.30)
    rag_veins = F.pinnate_veins(count=6, angle=0.8, curve=0.45, reach=0.92, width=0.12, widest=0.45, rng=nrng)
    _paint_leaf(atlas, "ragwort_leaf", nrng, rag_shape, rag_veins,
                dict(base=(0.035, 0.070, 0.026), tip=(0.044, 0.082, 0.032), vein=(0.075, 0.120, 0.046),
                     margin=(0.028, 0.055, 0.020), stalk=(0.052, 0.065, 0.030), brown=(0.09, 0.05, 0.02)),
                0.22, vein_width=0.006, vein_depth=0.00018, puff=0.00012, tertiary=0.45, trans=0.42)
    dan_shape = _shape_lobed(0.125, teeth=9, depth=0.42, backcut=0.35)
    dan_veins = F.pinnate_veins(count=6, angle=0.78, curve=0.28, reach=0.94, width=0.125, widest=0.42, rng=nrng)
    _paint_leaf(atlas, "dandelion_leaf", nrng, dan_shape, dan_veins,
                dict(base=(0.050, 0.095, 0.028), tip=(0.060, 0.110, 0.034), vein=(0.090, 0.135, 0.050),
                     margin=(0.040, 0.074, 0.022), stalk=(0.075, 0.085, 0.035), brown=(0.10, 0.060, 0.020)),
                0.22, vein_width=0.006, vein_depth=0.00020, puff=0.00016, tertiary=0.35, holes=0.10,
                trans=0.45)
    pl_shape = _shape_plantain(0.070)
    pl_veins = [([(0.0, 0.02), (0.0, 0.985)], 1.0),
                ([(-0.020, 0.04), (-0.035, 0.95)], 0.55), ([(0.020, 0.04), (0.035, 0.95)], 0.55),
                ([(-0.040, 0.10), (-0.052, 0.90)], 0.35), ([(0.040, 0.10), (0.052, 0.90)], 0.35)]
    _paint_leaf(atlas, "plantain_leaf", nrng, pl_shape, pl_veins,
                dict(base=(0.052, 0.090, 0.036), tip=(0.062, 0.105, 0.042), vein=(0.125, 0.145, 0.075),
                     margin=(0.040, 0.070, 0.028), stalk=(0.075, 0.080, 0.040), brown=(0.09, 0.05, 0.02)),
                0.22, vein_width=0.008, vein_depth=0.00025, puff=0.00012, tertiary=0.15, trans=0.40)
    butter_shape = F.ovate(width=0.16, widest=0.48, tip_sharp=0.55, base_round=0.75, base=0.05, tip=0.93,
                           teeth=9, tooth_depth=0.10, double=0.30, cordate=0.18)
    butter_veins = F.pinnate_veins(count=4, angle=1.0, curve=0.4, reach=0.75, width=0.16, widest=0.48, rng=nrng)
    _paint_leaf(atlas, "buttercup_leaf", nrng, butter_shape, butter_veins,
                dict(base=(0.044, 0.094, 0.028), tip=(0.056, 0.120, 0.034), vein=(0.086, 0.140, 0.052),
                     margin=(0.034, 0.074, 0.022), stalk=(0.062, 0.082, 0.035), brown=(0.09, 0.05, 0.02)),
                0.10, vein_width=0.007, vein_depth=0.00016, puff=0.00015, tertiary=0.35, trans=0.45)

    _paint_daisy(atlas, "ragwort_flower", nrng, (0.95, 0.70, 0.055), (0.62, 0.34, 0.02), petals=12)
    _paint_daisy(atlas, "dandelion_flower", nrng, (0.95, 0.76, 0.025), (0.74, 0.46, 0.01), petals=30, notch=0.05)
    _paint_daisy(atlas, "buttercup_flower", nrng, (0.95, 0.76, 0.055), (0.58, 0.36, 0.02), petals=5, notch=0.10)
    _paint_seed_clock(atlas, "seed_clock", nrng)
    _paint_disc(atlas, "thistle_head", nrng, (0.33, 0.08, 0.35), (0.11, 0.12, 0.055), meters=0.055)
    _paint_disc(atlas, "dock_seed", nrng, (0.25, 0.105, 0.040), (0.11, 0.060, 0.030), meters=0.025)
    _paint_disc(atlas, "plantain_seed", nrng, (0.17, 0.115, 0.050), (0.055, 0.043, 0.025), meters=0.035)

    for key, a, bcol, trans in (
        ("stem_green", (0.045, 0.080, 0.030), (0.085, 0.125, 0.048), 0.20),
        ("stem_red", (0.080, 0.035, 0.028), (0.16, 0.065, 0.035), 0.12),
        ("stem_dead", (0.075, 0.052, 0.035), (0.22, 0.115, 0.052), 0.02),
        ("stem_thistle", (0.075, 0.088, 0.060), (0.145, 0.155, 0.115), 0.15),
        ("stem_ragwort", (0.040, 0.078, 0.028), (0.078, 0.125, 0.045), 0.20),
        ("stolon", (0.052, 0.070, 0.026), (0.088, 0.110, 0.040), 0.15),
    ):
        U, V = atlas.column_grid(key)
        n = F.noise(U.shape, nrng, freq=70.0, beta=1.35, aniso=(1.0, 10.0))
        layer = F.Layer(U.shape)
        layer.color = F.lerp(a, bcol, n)
        layer.height = 0.00006 * n
        layer.rough = 0.58 + 0.14 * n
        layer.trans[...] = trans
        atlas.put(key, layer, meters_per_px=0.6 / U.shape[0], wrap=True, opaque=True)
    atlas.save()
    return atlas


def _card(b, atlas, base, heading, length, key, phase, lod, elev=18, droop=0.25, fold=0.12, curl=0.04,
          lift=0.0, scale=1.0, flutter=1.0):
    h = Vector((math.cos(heading), math.sin(heading), 0.0))
    d = (h * math.cos(math.radians(elev)) + UP * math.sin(math.radians(elev))).normalized()
    up = (UP - h * 0.28).normalized()
    rows = (8, 4, 2)[lod]
    cols = (5, 3, 1)[lod]
    b.card(base, d, up, length * scale, length * scale * S.tile_aspect(atlas, key), atlas.uv(key),
           rows=rows, cols=cols, fold=fold if lod < 2 else 0.0, curl=curl if lod == 0 else 0.0,
           droop=droop, lift=lift, twist=0.15 * math.sin(heading) if lod == 0 else 0.0,
           phase=phase, flutter=flutter, flutter_base=0.0)


def _stem_points(base, height, lean, curve, n):
    h = Vector((math.cos(lean), math.sin(lean), 0))
    pts = []
    for i in range(n + 1):
        t = i / n
        pts.append(Vector(base) + h * (curve * math.sin(t * math.pi * 0.72) + 0.03 * t * t) + UP * (height * t))
    return pts


def _cupped_head(b, atlas, center, normal, size, key, phase, lod, spin=0.0, cards=5, cup=0.08):
    """A small flower/seed head made from several tilted cards, keeping the atlas-card workflow but
    giving the silhouette real volume from gameplay angles."""
    normal = Vector(normal).normalized()
    tangent = normal.orthogonal().normalized()
    count = max(1, cards if lod == 0 else max(1, cards // (2 if lod == 1 else 3)))
    segs = (2, 1, 1)[lod]
    for i in range(count):
        az = spin + i * math.tau / count
        side = Matrix.Rotation(az, 3, normal) @ tangent
        face = (normal * (1.0 - 0.10 * (i % 2)) + side * 0.38).normalized()
        b.flat(Vector(center) + face * size * 0.06, face, size * (1.0 - 0.05 * (i % 3)),
               atlas.uv(key), spin=az, cup=cup, phase=phase, flutter=0.55, segs=segs)


def describe_dock(rng):
    leaves = []
    for i in range(40):
        az = i * 2.39996 + rng.uniform(-0.22, 0.22)
        upright = i < 8
        big = 8 <= i < 26
        leaves.append(dict(az=az, length=rng.uniform(0.18, 0.27) if upright else
                           (rng.uniform(0.31, 0.38) if big else rng.uniform(0.24, 0.33)),
                           elev=rng.uniform(34, 62) if upright else (rng.uniform(5, 18) if big else rng.uniform(12, 30)),
                           droop=rng.uniform(-0.06, 0.12) if upright else (rng.uniform(0.22, 0.50) if big else rng.uniform(0.12, 0.36)),
                           key=rng.choices(("dock_green", "dock_red", "dock_old"), (52, 28, 20))[0],
                           keep=rng.random(), phase=rng.random()))
    spikes = []
    for i in range(3):
        spikes.append(dict(base=Vector((rng.uniform(-0.08, 0.10), rng.uniform(-0.08, 0.10), 0.004)),
                           height=rng.uniform(0.58, 0.78), lean=rng.uniform(0, math.tau),
                           curve=rng.uniform(0.025, 0.065), phase=rng.random()))
    return dict(leaves=leaves, spikes=spikes)


def emit_dock(desc, atlas, lod):
    b = F.Batch(height=MESHES["dock"][1])
    for lf in desc["leaves"]:
        if (lod == 1 and lf["keep"] > 0.78) or (lod == 2 and lf["keep"] > 0.52):
            continue
        base = Vector((math.cos(lf["az"]), math.sin(lf["az"]), 0)) * 0.025
        _card(b, atlas, base, lf["az"], lf["length"], lf["key"], lf["phase"], lod,
              elev=lf["elev"], droop=lf["droop"], fold=0.065, curl=0.04, scale=(1.0, 1.06, 1.18)[lod])
    for sp in desc["spikes"]:
        n = (9, 6, 4)[lod]
        pts = _stem_points(sp["base"], sp["height"], sp["lean"], sp["curve"], n)
        b.tube(pts, [0.0042 * (1 - 0.55 * i / n) for i in range(n + 1)], (5, 4, 3)[lod],
               atlas.uv("stem_dead"), v_length=0.55, phase=sp["phase"], flutter=0.08, cap=lod == 0)
        count = (36, 18, 9)[lod]
        for k in range(count):
            t = 0.25 + 0.70 * k / max(count - 1, 1)
            center = pts[min(int(t * n), n)]
            rad = 0.006 + 0.008 * (1 - t)
            sides = (5, 3, 2)[lod]
            for side in range(sides):
                ang = sp["lean"] + side * math.tau / sides + k * 0.7
                normal = Vector((math.cos(ang), math.sin(ang), 0.20)).normalized()
                b.flat(center + normal * rad, normal, 0.013 * (1.0, 1.18, 1.35)[lod], atlas.uv("dock_seed"),
                       spin=ang, cup=0.025, phase=sp["phase"], flutter=0.15, segs=(2, 1, 1)[lod])
    print("HOMESTEAD_TRIS", lod, "dock", b.triangles)
    return b


def describe_thistle(rng):
    thistle = []
    for i in range(34):
        thistle.append(dict(az=i * 2.39996 + rng.uniform(-0.25, 0.25), length=rng.uniform(0.18, 0.34),
                            elev=rng.uniform(4, 32), droop=rng.uniform(0.10, 0.46),
                            key=rng.choice(("thistle_leaf", "thistle_leaf_pale")),
                            keep=rng.random(), phase=rng.random()))
    stems = []
    for i in range(3):
        stems.append(dict(base=Vector((-0.10 + 0.06 * i + rng.uniform(-0.025, 0.025),
                                       -0.02 + rng.uniform(-0.045, 0.060), 0.0)),
                          height=rng.uniform(0.40, 0.62), lean=rng.uniform(0, math.tau),
                          phase=rng.random(), heads=rng.randint(1, 2)))
    rag_leaves = [dict(az=i * 2.39996 + rng.uniform(-0.2, 0.2), length=rng.uniform(0.12, 0.22),
                       elev=rng.uniform(18, 55), keep=rng.random(), phase=rng.random()) for i in range(18)]
    rag = dict(base=Vector((0.20, -0.03, 0.0)), height=rng.uniform(0.58, 0.70), lean=rng.uniform(0, math.tau),
               leaves=rag_leaves, phase=rng.random())
    return dict(thistle=thistle, stems=stems, ragwort=rag)


def emit_thistle(desc, atlas, lod):
    b = F.Batch(height=MESHES["thistle"][1])
    origin = Vector((-0.10, 0.02, 0.006))
    for lf in desc["thistle"]:
        if (lod == 1 and lf["keep"] > 0.75) or (lod == 2 and lf["keep"] > 0.48):
            continue
        _card(b, atlas, origin, lf["az"], lf["length"], lf["key"], lf["phase"], lod,
              elev=lf["elev"], droop=lf["droop"], fold=0.10, curl=0.08, scale=(1.0, 1.08, 1.22)[lod])
    for i, st in enumerate(desc["stems"]):
        n = (7, 5, 3)[lod]
        pts = _stem_points(st["base"], st["height"], st["lean"], 0.030, n)
        b.tube(pts, [0.0060 * (1 - 0.50 * k / n) for k in range(n + 1)], (5, 4, 3)[lod],
               atlas.uv("stem_thistle"), v_length=0.4, phase=st["phase"], flutter=0.12, cap=lod == 0)
        for j in range(6 if lod == 0 else (3 if lod == 1 else 1)):
            t = 0.15 + j * 0.13
            node = pts[min(n - 1, max(1, int(t * n)))]
            az = st["lean"] + j * 2.1
            _card(b, atlas, node, az, max(0.055, 0.13 * (1.0 - 0.08 * j)), "thistle_leaf_pale", st["phase"], lod,
                  elev=22, droop=0.25, fold=0.12, curl=0.10, scale=0.72)
        head_positions = [pts[-1]]
        if st.get("heads", 1) > 1 and lod < 2:
            node = pts[-2]
            side = Vector((math.cos(st["lean"] + 1.4), math.sin(st["lean"] + 1.4), 0))
            branch = [node, node + side * 0.035 + UP * 0.030, node + side * 0.060 + UP * 0.060]
            b.tube(branch, [0.0020, 0.0015, 0.0010], 3, atlas.uv("stem_thistle"),
                   v_length=0.25, phase=st["phase"], flutter=0.20)
            head_positions.append(branch[-1])
        for hi, top in enumerate(head_positions):
            open_head = (i + hi) % 2 == 0
            if open_head:
                # One open thistle head: a small green/purple base plus a ragged crown of cards so it
                # reads as a brushy thistle flower rather than a smooth egg.
                b.sphere(top + UP * 0.012, 0.014 * (1.0, 1.08, 1.20)[lod], atlas.uv("thistle_head"),
                         segs=(6, 5, 4)[lod], rings=(4, 3, 3)[lod], stretch=0.9, axis=UP, phase=st["phase"])
                petals = (12, 8, 4)[lod]
                for p in range(petals):
                    az = p * math.tau / petals + 0.3 * (p % 2)
                    normal = (Vector((math.cos(az), math.sin(az), 0.75))).normalized()
                    b.flat(top + UP * (0.028 + 0.003 * (p % 3)) + normal * 0.006, normal,
                           0.028 * (1.0, 1.12, 1.26)[lod], atlas.uv("thistle_head"),
                           spin=az, cup=-0.02, phase=st["phase"], flutter=0.45, segs=1)
            else:
                b.sphere(top + UP * 0.012, 0.021 * (1.0, 1.08, 1.20)[lod], atlas.uv("thistle_head"),
                         segs=(8, 6, 5)[lod], rings=(5, 4, 3)[lod], stretch=1.18, axis=UP, phase=st["phase"])
                if lod == 0:
                    for p in range(4):
                        normal = (Vector((math.cos(p * math.tau / 4), math.sin(p * math.tau / 4), 0.55))).normalized()
                        b.flat(top + UP * 0.024 + normal * 0.007, normal, 0.021, atlas.uv("thistle_head"),
                               spin=p, cup=-0.01, phase=st["phase"], flutter=0.35, segs=1)
    rag = desc["ragwort"]
    for lf in rag["leaves"]:
        if (lod == 1 and lf["keep"] > 0.80) or (lod == 2 and lf["keep"] > 0.55):
            continue
        _card(b, atlas, rag["base"] + Vector((0, 0, 0.006)), lf["az"], lf["length"], "ragwort_leaf",
              lf["phase"], lod, elev=lf["elev"], droop=0.18, fold=0.05, curl=0.04)
    n = (8, 5, 3)[lod]
    pts = _stem_points(rag["base"], rag["height"], rag["lean"], 0.025, n)
    b.tube(pts, [0.0045 * (1 - 0.45 * k / n) for k in range(n + 1)], (5, 4, 3)[lod],
           atlas.uv("stem_ragwort"), v_length=0.45, phase=rag["phase"], flutter=0.12, cap=lod == 0)
    for j in range(9 if lod == 0 else (5 if lod == 1 else 2)):
        node = pts[min(n - 1, max(1, int((0.22 + 0.13 * j) * n)))]
        az = rag["lean"] + 1.1 + j * 2.2
        _card(b, atlas, node, az, max(0.050, 0.090 * (1.0 - 0.07 * j)), "ragwort_leaf", rag["phase"], lod,
              elev=28, droop=0.16, fold=0.05, curl=0.04, scale=0.72)
    top = pts[-1]
    flower_count = (24, 13, 7)[lod]
    for k in range(flower_count):
        az = k * 2.39996
        r = 0.020 + 0.105 * (k / max(flower_count - 1, 1)) ** 0.6
        center = top + Vector((math.cos(az) * r, math.sin(az) * r, 0.010 * math.sin(k))) + UP * (0.018 * (1 - k / flower_count))
        normal = (UP * 0.80 + Vector((math.cos(az), math.sin(az), 0)) * 0.35).normalized()
        _cupped_head(b, atlas, center, normal, 0.032 * (1.0, 1.10, 1.24)[lod],
                     "ragwort_flower", rag["phase"], lod, spin=az, cards=3, cup=0.055)
    print("HOMESTEAD_TRIS", lod, "thistle", b.triangles)
    return b


def describe_dandelion(rng):
    centers = [Vector((-0.16, -0.08, 0.0)), Vector((0.08, -0.03, 0.0)), Vector((0.02, 0.14, 0.0))]
    rosettes = []
    for c in centers:
        leaves = []
        for i in range(rng.randint(14, 17)):
            leaves.append(dict(az=i * 2.39996 + rng.uniform(-0.28, 0.28), length=rng.uniform(0.13, 0.23),
                               elev=rng.uniform(3, 23), keep=rng.random(), phase=rng.random()))
        rosettes.append(dict(center=c, leaves=leaves))
    flowers = []
    for i in range(11):
        c = centers[i % len(centers)] + Vector((rng.uniform(-0.035, 0.035), rng.uniform(-0.035, 0.035), 0))
        flowers.append(dict(base=c, height=rng.uniform(0.14, 0.25), az=rng.uniform(0, math.tau),
                            clock=i in (2, 8), phase=rng.random(), keep=rng.random()))
    plantain = [dict(base=Vector((0.18, 0.10, 0.0)), az=i * 2.39996 + rng.uniform(-0.2, 0.2),
                     length=rng.uniform(0.13, 0.23), elev=rng.uniform(12, 35), keep=rng.random(),
                     phase=rng.random()) for i in range(13)]
    butter = [dict(base=Vector((-0.20, 0.13, 0.0)), az=i * 2.39996 + rng.uniform(-0.25, 0.25),
                   length=rng.uniform(0.060, 0.105), elev=rng.uniform(8, 30), keep=rng.random(),
                   phase=rng.random()) for i in range(12)]
    return dict(rosettes=rosettes, flowers=flowers, plantain=plantain, butter=butter)


def emit_dandelion(desc, atlas, lod):
    b = F.Batch(height=MESHES["dandelion"][1])
    for ro in desc["rosettes"]:
        for lf in ro["leaves"]:
            if (lod == 1 and lf["keep"] > 0.78) or (lod == 2 and lf["keep"] > 0.50):
                continue
            _card(b, atlas, ro["center"] + Vector((0, 0, 0.004)), lf["az"], lf["length"], "dandelion_leaf",
                  lf["phase"], lod, elev=lf["elev"], droop=0.20, fold=0.05, curl=0.04,
                  scale=(1.0, 1.06, 1.20)[lod])
    for fl in desc["flowers"]:
        if (lod == 1 and fl["keep"] > 0.84) or (lod == 2 and fl["keep"] > 0.60):
            continue
        n = (5, 4, 2)[lod]
        pts = _stem_points(fl["base"], fl["height"], fl["az"], 0.012, n)
        b.tube(pts, [0.0020 * (1 - 0.25 * k / n) for k in range(n + 1)], (4, 3, 3)[lod],
               atlas.uv("stem_green"), v_length=0.3, phase=fl["phase"], flutter=0.22)
        top = pts[-1]
        key = "seed_clock" if fl["clock"] else "dandelion_flower"
        size = (0.055 if fl["clock"] else 0.050) * (1.0, 1.10, 1.24)[lod]
        if fl["clock"]:
            _cupped_head(b, atlas, top + UP * 0.004,
                         (UP * 0.9 + Vector((math.cos(fl["az"]), math.sin(fl["az"]), 0)) * 0.25).normalized(),
                         size, key, fl["phase"], lod, spin=fl["az"], cards=6, cup=0.12)
        else:
            _cupped_head(b, atlas, top + UP * 0.003, UP, size, key, fl["phase"], lod,
                         spin=fl["az"], cards=7, cup=0.11)
    for lf in desc["plantain"]:
        if (lod == 1 and lf["keep"] > 0.75) or (lod == 2 and lf["keep"] > 0.55):
            continue
        _card(b, atlas, lf["base"] + Vector((0, 0, 0.004)), lf["az"], lf["length"], "plantain_leaf",
              lf["phase"], lod, elev=lf["elev"], droop=0.28, fold=0.08, curl=0.02)
    for idx, az in enumerate((0.3, 2.1, 3.6, 5.2)):
        base = Vector((0.18, 0.10, 0.0)) + Vector((0.025 * idx, -0.015 * idx, 0))
        pts = _stem_points(base, 0.18 + idx * 0.025, az, 0.006, (5, 3, 2)[lod])
        b.tube(pts, [0.0016] * len(pts), (3, 3, 3)[lod], atlas.uv("stem_green"),
               v_length=0.25, phase=0.35 + idx * 0.2, flutter=0.25)
        b.flat(pts[-1], Vector((math.cos(az), math.sin(az), 0.45)).normalized(), 0.035, atlas.uv("plantain_seed"),
               spin=az, cup=0.06, phase=0.35 + idx * 0.2, flutter=0.35, segs=1)
    for lf in desc["butter"]:
        if (lod == 1 and lf["keep"] > 0.80) or (lod == 2 and lf["keep"] > 0.60):
            continue
        _card(b, atlas, lf["base"] + Vector((0, 0, 0.004)), lf["az"], lf["length"], "buttercup_leaf",
              lf["phase"], lod, elev=lf["elev"], droop=0.12, fold=0.05, curl=0.04)
    # Creeping buttercup runners and small yellow flowers skimming the grass.
    for k, az in enumerate((0.6, 2.4, 4.1)):
        base = Vector((-0.20, 0.13, 0.006))
        end = base + Vector((math.cos(az), math.sin(az), 0)) * (0.16 + 0.03 * k)
        mid = base.lerp(end, 0.5) + Vector((-math.sin(az), math.cos(az), 0)) * 0.025
        b.tube([base, mid, end], [0.0014, 0.0012, 0.0010], 3, atlas.uv("stolon"),
               v_length=0.25, phase=0.6 + 0.1 * k, flutter=0.20)
        if lod < 2 or k == 0:
            _cupped_head(b, atlas, end + UP * 0.045,
                         (UP * 0.85 + Vector((math.cos(az), math.sin(az), 0)) * 0.35).normalized(),
                         0.032 * (1.0, 1.08, 1.22)[lod], "buttercup_flower", 0.6 + 0.1 * k,
                         lod, spin=az, cards=3, cup=0.07)
    print("HOMESTEAD_TRIS", lod, "dandelion", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(1.02, 1.12, 0.62))
    rng = random.Random(SEED)
    out = []
    desc_dock = describe_dock(random.Random(rng.randrange(1 << 30)))
    batches = [emit_dock(desc_dock, atlas, lod) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESHES["dock"][0], material, smooth_angle=179.0)
    out.extend(objs)
    print("HOMESTEAD_LODS dock", F.lod_report(objs))
    desc_thistle = describe_thistle(random.Random(rng.randrange(1 << 30)))
    batches = [emit_thistle(desc_thistle, atlas, lod) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESHES["thistle"][0], material, smooth_angle=179.0)
    out.extend(objs)
    print("HOMESTEAD_LODS thistle", F.lod_report(objs))
    desc_dandelion = describe_dandelion(random.Random(rng.randrange(1 << 30)))
    batches = [emit_dandelion(desc_dandelion, atlas, lod) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESHES["dandelion"][0], material, smooth_angle=179.0)
    out.extend(objs)
    print("HOMESTEAD_LODS dandelion", F.lod_report(objs))
    return out
