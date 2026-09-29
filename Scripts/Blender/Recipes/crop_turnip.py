"""One 1 m plot of Early Snowball / purple-crowned turnips in five growth stages.

Real-object research (written before modeling):
- White globe turnips common to British cottage gardens are a cool-season Brassica rapa root:
  leaves grow as basal rosettes, rough and bristly, lyrate/pinnatifid with a large rounded
  terminal lobe and smaller side lobes. Young leaves are clear yellow-green; older outer leaves
  dull, yellow, and lie outward.
- "Early Snowball" reads as a white globe; purple-topped historic types such as Purple Top Milan
  put a violet crown at soil level. Harvest roots are about 6-8 cm diameter and sit with the
  shoulder/globe half out of the ridge, while the leafy rosette reaches 25-30 cm.
- Seedlings first show paired heart-shaped cotyledons 2-3 cm tall, then a tiny rough true leaf.
- Homestead plot layout follows SM_TilledBed: 1 m square, pivot bottom-centre, ridge tops along X at
  y=-0.3,0,+0.3 m, z about 4.5-5.5 cm. Six turnips are planted, x=-0.22,+0.22 on each ridge.

Game notes: walk-through (no collision). One 2K atlas and one material for all five stages
(alpha-masked, two-sided foliage). Wind vertex colours per homestead_foliage.py. The plant-stage
meshes carry only leaves/stems; the game places SM_CropTurnip_Produce on report anchors and scales
it from sunk pale-green young roots up to the ripe white/purple globe.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "CropTurnip"
DESCRIPTION = ("Six Early Snowball / purple-crowned turnips on a 1 m tilled-bed plot, authored as "
               "five growth-stage meshes from sprout to ripe with shared atlas/material.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.16), "views": ("hero", "detail"),
          "detail_distance": 1.7,
          "meshes": {
              "SM_CropTurnip_Growing": {"focus": (0.0, 0.0, 0.13), "detail_distance": 1.45},
              "SM_CropTurnip_Ripe": {"focus": (0.0, -0.02, 0.15), "eye_distance": 2.5,
                                      "detail_distance": 1.25},
              "SM_CropTurnip_Harvest": {"focus": (0.0, 0.0, -0.035), "pose": (0, 0, 0),
                                         "detail_distance": 0.55},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / stage height",
             "G": "per-plant random phase", "B": "leaf flutter 0 at petiole -> 1 at blade tip",
             "A": "1"},
    "material_notes": ("One material M_CropTurnip: T_CropTurnip_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency, B AO). "
                       "Two-sided masked foliage; turnip root and soil tiles are opaque in the same atlas."),
    "harvest_mesh": "SM_CropTurnip_Harvest pivot is the grip point at the leaf base/top of root; root hangs down -Z.",
}

SEED = 185006
RIDGES = (-0.30, 0.0, 0.30)
PLANT_X = (-0.22, 0.22)
GROUND_Z = 0.045
STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")


def _stage_name(stage):
    return "SM_" + NAME + "_" + stage


def _palettes():
    return {
        "leaf": dict(base=(0.060, 0.100, 0.030), tip=(0.070, 0.115, 0.034),
                     vein=(0.092, 0.135, 0.046), margin=(0.046, 0.078, 0.023),
                     yellow=(0.23, 0.20, 0.06), brown=(0.075, 0.045, 0.020)),
        "leaf_dark": dict(base=(0.045, 0.080, 0.025), tip=(0.055, 0.094, 0.027),
                          vein=(0.078, 0.112, 0.040), margin=(0.038, 0.064, 0.020),
                          yellow=(0.22, 0.18, 0.055), brown=(0.065, 0.040, 0.018)),
        "leaf_old": dict(base=(0.105, 0.100, 0.034), tip=(0.135, 0.120, 0.040),
                         vein=(0.145, 0.135, 0.055), margin=(0.078, 0.066, 0.025),
                         yellow=(0.28, 0.23, 0.075), brown=(0.090, 0.055, 0.023)),
        "cotyledon": dict(base=(0.078, 0.128, 0.040), tip=(0.088, 0.142, 0.045),
                          vein=(0.115, 0.160, 0.060), margin=(0.060, 0.100, 0.030),
                          yellow=(0.22, 0.20, 0.07), brown=(0.07, 0.04, 0.02)),
    }


def _turnip_leaf_shape(width=0.36, teeth=19, phase=0.0):
    """Lyrate brassica leaf silhouette: narrow basal wing, side lobes, large terminal lobe."""
    def shape(X, Y):
        s = np.clip(Y, 0.0, 1.0)
        terminal = width * 1.08 * np.exp(-((s - 0.74) / 0.23) ** 2)
        basal = width * 0.23 * np.sin(np.pi * np.clip((s - 0.03) / 0.72, 0, 1)) ** 0.7
        lobes = width * (0.18 * np.exp(-((s - 0.24) / 0.055) ** 2) +
                         0.24 * np.exp(-((s - 0.40) / 0.065) ** 2) +
                         0.18 * np.exp(-((s - 0.54) / 0.060) ** 2))
        necks = 1.0 - (0.34 * np.exp(-((s - 0.32) / 0.050) ** 2) +
                       0.28 * np.exp(-((s - 0.49) / 0.055) ** 2))
        hw = np.maximum(basal + lobes, terminal) * necks
        hw *= (1.0 + F.serration(s, X, teeth, 0.10, double=0.12, phase=phase) *
               F.smoothstep(0.08, 0.20, s) * (1 - F.smoothstep(0.92, 1.0, s)))
        inside = np.minimum(hw - np.abs(X), np.minimum(Y - 0.015, 0.98 - Y) * 3.0)
        return inside, s, hw
    return shape


def _turnip_leaf_veins(rng):
    veins = [([(0.0, 0.02), (0.0, 0.98)], 1.0)]
    for y0, reach, sc in ((0.20, 0.48, 0.72), (0.33, 0.70, 0.62), (0.46, 0.74, 0.52),
                          (0.60, 0.58, 0.43), (0.73, 0.42, 0.35)):
        for side in (-1, 1):
            dy = rng.uniform(-0.018, 0.018)
            veins.append(([(0.0, y0 + dy), (side * 0.12, y0 + 0.05 + dy),
                           (side * reach * 0.35, y0 + 0.14 + dy)], sc))
    return veins


def _paint_root_tile(atlas, key, nrng, ripe=True):
    X, Y, px = atlas.grid(key)
    U = X / (2 * X.max()) + 0.5
    V = np.clip(Y, 0, 1)
    n = F.noise(X.shape, nrng, freq=18.0, beta=1.8)
    freckles = F.smoothstep(0.84, 0.97, F.noise(X.shape, nrng, freq=92.0, beta=1.2))
    layer = F.Layer(X.shape)
    white = F.lerp((0.47, 0.43, 0.33), (0.66, 0.61, 0.47), n)
    crown = F.lerp((0.33, 0.070, 0.125), (0.18, 0.045, 0.090), n)
    crown_mask = F.smoothstep(0.60 if ripe else 0.72, 0.88, V + 0.05 * np.sin(U * math.tau * 2.0))
    layer.color = F.lerp(white, crown, crown_mask * (0.88 if ripe else 0.42))
    layer.color = F.lerp(layer.color, (0.25, 0.19, 0.12), freckles * 0.18)
    layer.alpha[...] = 1
    layer.height = 0.00018 * n - 0.00008 * freckles
    layer.rough = 0.62 + 0.10 * freckles
    layer.trans[...] = 0.0
    atlas.put(key, layer, meters_per_px=0.08 / X.shape[0], opaque=True)


def _paint_stem_column(atlas, nrng):
    U, V = atlas.column_grid("stem")
    n = F.noise(U.shape, nrng, freq=70.0, beta=1.4, aniso=(1.0, 7.0))
    hairs = F.smoothstep(0.80, 0.94, F.noise(U.shape, nrng, freq=155.0, beta=1.1, aniso=(1.0, 4.0)))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.072, 0.104, 0.038), (0.120, 0.135, 0.055), n * 0.55)
    layer.color = F.lerp(layer.color, (0.25, 0.27, 0.18), hairs * 0.35)
    layer.height = 0.00008 * n + 0.00005 * hairs
    layer.rough = 0.57 + 0.12 * hairs
    layer.trans[...] = 0.25
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
    atlas.column("stem", 54)
    atlas.tile("cotyledon", 260, 320)
    atlas.tile("true_tiny", 300, 440)
    atlas.tile("leaf", 430, 760)
    atlas.tile("leaf_dark", 430, 760)
    atlas.tile("leaf_old", 430, 760)
    atlas.tile("root_small", 220, 220)
    atlas.tile("root_ripe", 300, 300)
    atlas.tile("soil", 160, 160)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()

    X, Y, px = atlas.grid("cotyledon")
    atlas.put("cotyledon", F.paint_blade(X, Y, nrng,
              F.ovate(width=0.33, widest=0.47, tip_sharp=0.45, base_round=1.4, cordate=0.18,
                      base=0.02, tip=0.94),
              [([(0, 0.03), (0, 0.92)], 1.0)], pals["cotyledon"], px, vein_width=0.010,
              vein_depth=0.00006, puff=0.00010, tertiary=0.15, trans=0.65), meters_per_px=0.030 / X.shape[0])
    X, Y, px = atlas.grid("true_tiny")
    atlas.put("true_tiny", F.paint_blade(X, Y, nrng, _turnip_leaf_shape(width=0.28, teeth=13, phase=0.2),
              _turnip_leaf_veins(nrng), pals["leaf"], px, vein_width=0.009, vein_depth=0.00010,
              puff=0.00011, tertiary=0.20, hair=0.15, trans=0.62), meters_per_px=0.060 / X.shape[0])
    for key, pal, params in (("leaf", pals["leaf"], dict(yellow=0.03, damage=0.04)),
                             ("leaf_dark", pals["leaf_dark"], dict(yellow=0.02, damage=0.06)),
                             ("leaf_old", pals["leaf_old"], dict(yellow=0.38, damage=0.22, edge_burn=0.20))):
        X, Y, px = atlas.grid(key)
        atlas.put(key, F.paint_blade(X, Y, nrng,
                  _turnip_leaf_shape(width=0.37, teeth=21, phase=rng.random()),
                  _turnip_leaf_veins(nrng), pal, px, vein_width=0.010, vein_depth=0.00016,
                  puff=0.00016, tertiary=0.38, hair=0.22, gloss=0.0, trans=0.58, **params),
                  meters_per_px=0.205 / X.shape[0])
    _paint_root_tile(atlas, "root_small", nrng, ripe=False)
    _paint_root_tile(atlas, "root_ripe", nrng, ripe=True)
    _paint_soil_tile(atlas, nrng)
    _paint_stem_column(atlas, nrng)
    atlas.save()
    return atlas


def _plot_positions(rng):
    plants = []
    for y in RIDGES:
        for x in PLANT_X:
            plants.append(Vector((x + rng.uniform(-0.012, 0.012),
                                  y + rng.uniform(-0.010, 0.010),
                                  GROUND_Z + rng.uniform(-0.002, 0.002))))
    return plants


def _produce_anchor_rows(shift=(0.0, 0.0, 0.0)):
    rng = random.Random(SEED + 704)
    offset = Vector(shift)
    rows = []
    for base in _plot_positions(random.Random(SEED + 42)):
        local = base - offset
        rows.append([round(local.x, 4), round(local.y, 4), round(local.z, 4),
                     round(rng.uniform(0.0, 360.0), 1), round(rng.uniform(0.88, 1.08), 3)])
    return rows


REPORT["produce"] = {
    "mesh": _stage_name("Produce"),
    "anchors": {stage: [list(row) for row in _produce_anchor_rows()]
                for stage in ("Young", "Growing", "Mature", "Ripe")},
}


STAGE_PARAMS = {
    "Sprout": dict(height=0.052, leaves=1, leaf_len=0.034, petiole=0.012, root=0.0),
    "Young": dict(height=0.135, leaves=5, leaf_len=0.094, petiole=0.040, root=0.0),
    "Growing": dict(height=0.235, leaves=8, leaf_len=0.155, petiole=0.062, root=0.018),
    "Mature": dict(height=0.305, leaves=10, leaf_len=0.190, petiole=0.070, root=0.027),
    "Ripe": dict(height=0.285, leaves=9, leaf_len=0.170, petiole=0.060, root=0.039),
}


def _edge_scale(base, heading, reach, margin=0.455):
    """Shorten leaves aimed out of the one-metre plot while preserving inward rosette fullness."""
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


def _emit_leaf(b, atlas, base, heading, phase, rng, stage, lod, old=False):
    p = STAGE_PARAMS[stage]
    if stage == "Sprout":
        for side_sign in (-1, 1):
            d = Matrix.Rotation(side_sign * 0.95 + rng.uniform(-0.12, 0.12), 3, Vector((0, 0, 1))) @ heading
            top = base + Vector((0, 0, 0.010))
            up = (Vector((0, 0, 1)) + d * 0.20).normalized()
            b.card(top, d, up, 0.027, 0.027 * S.tile_aspect(atlas, "cotyledon"), atlas.uv("cotyledon"),
                   rows=(2, 1, 1)[lod], cols=1, fold=0.03, droop=0.05, phase=phase, flutter=1.0,
                   flutter_base=0.15)
        d = (heading + Vector((0, 0, 0.7))).normalized()
        b.card(base + Vector((0, 0, 0.012)), d, (Vector((0, 0, 1)) - heading * 0.3).normalized(),
               0.026, 0.026 * S.tile_aspect(atlas, "true_tiny"), atlas.uv("true_tiny"),
               rows=1, cols=1, fold=0.04, droop=0.05, phase=phase, flutter=1.0, flutter_base=0.2)
        return

    elev = math.radians(rng.uniform(25, 55) if old or stage == "Ripe" else rng.uniform(45, 72))
    if lod == 2:
        elev = max(elev, math.radians(30))
    pet = p["petiole"] * rng.uniform(0.78, 1.10)
    edge_scale = _edge_scale(base, heading, p["petiole"] + p["leaf_len"])
    pet *= edge_scale
    droop = rng.uniform(0.15, 0.38) + (0.28 if old or stage == "Ripe" else 0.0)
    pts = _arc(base + Vector((0, 0, 0.003)), heading, elev, pet, droop, (3, 2, 1)[lod])
    b.tube(pts, [0.0019 * (1 - 0.35 * i / max(len(pts) - 1, 1)) for i in range(len(pts))], 4 if lod == 0 else 3,
           atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.12)
    key = "leaf_old" if old else rng.choices(["leaf", "leaf_dark"], [65, 35])[0]
    if stage == "Young":
        key = "true_tiny" if rng.random() < 0.25 else "leaf"
    top = pts[-1]
    if stage == "Ripe":
        leaf_elev = math.radians(rng.uniform(28, 55))
    elif stage == "Mature":
        leaf_elev = math.radians(rng.uniform(34, 64))
    else:
        leaf_elev = math.radians(rng.uniform(18, 48))
    d = (heading * math.cos(leaf_elev) + Vector((0, 0, math.sin(leaf_elev)))).normalized()
    up = (Vector((0, 0, 1)) - heading * rng.uniform(0.2, 0.7)).normalized()
    length = p["leaf_len"] * rng.uniform(0.78, 1.08) * (0.72 if stage == "Young" else 1.0) * edge_scale
    width = length * S.tile_aspect(atlas, key) * (1.05 if lod == 2 else 1.0)
    b.card(top, d, up, length, width, atlas.uv(key), rows=(4, 2, 1)[lod], cols=(2, 1, 1)[lod],
           fold=0.13 if lod == 0 else 0.02, curl=rng.uniform(-0.04, 0.09) if lod == 0 else 0.0,
           droop=rng.uniform(0.08, 0.24) + ((0.10 if stage == "Ripe" else 0.30) if stage == "Ripe" or old else 0.0),
           twist=rng.uniform(-0.25, 0.25) if lod == 0 else 0.0, phase=phase, flutter=1.0, flutter_base=0.25)


def emit(stage, atlas, lod):
    rng = random.Random(SEED + 100 * STAGES.index(stage) + lod)
    p = STAGE_PARAMS[stage]
    b = F.Batch(height=GROUND_Z + p["height"])
    stats = {"leaves": 0, "roots": 0}
    for pi, base in enumerate(_plot_positions(random.Random(SEED + 42))):
        phase = (pi * 0.137 + 0.21) % 1.0
        az0 = rng.uniform(0, math.tau)
        t0 = b.triangles
        if stage == "Sprout":
            _emit_leaf(b, atlas, base, Vector((math.cos(az0), math.sin(az0), 0)), phase, rng, stage, lod)
        else:
            leaf_count = p["leaves"]
            if lod == 1:
                leaf_count = max(3, int(leaf_count * 0.58))
            elif lod == 2:
                leaf_count = max(2, int(leaf_count * 0.32))
            for i in range(leaf_count):
                az = az0 + i * 2.39996 + rng.uniform(-0.22, 0.22)
                heading = Vector((math.cos(az), math.sin(az), 0))
                old = stage in ("Mature", "Ripe") and i >= leaf_count - (2 if lod < 2 else 1)
                _emit_leaf(b, atlas, base, heading, phase, rng, stage, lod, old=old)
        stats["leaves"] += b.triangles - t0
    print("HOMESTEAD_TRIS", stage, lod, stats)
    return b


def _zero_wind(obj):
    attr = obj.data.attributes.get("Wind")
    if attr:
        values = []
        for i in range(len(attr.data)):
            g = ((i * 37) % 251) / 251.0
            values.extend((0.0, g, 0.0, 1.0))
        attr.data.foreach_set("color_srgb", values)
        obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]


def emit_produce(atlas):
    """Ripe turnip globe used by the game as a separately scaled/tinted instance."""
    rng = random.Random(SEED + 808)
    b = F.Batch(height=1.0)
    phase = 0.0
    # Ten-centimetre exaggerated globe: origin/soil line crosses 60% up from the buried bottom.
    b.sphere(Vector((0, 0, -0.010)), 0.050, atlas.uv("root_ripe", inset=False),
             segs=18, rings=9, stretch=1.0, axis=Vector((0, 0, 1)), phase=phase)
    for i in range(6):
        az = i * 2.39996 + rng.uniform(-0.22, 0.22)
        radial = Vector((math.cos(az), math.sin(az), 0))
        start = radial * rng.uniform(0.004, 0.014) + Vector((0, 0, 0.034 + rng.uniform(-0.002, 0.003)))
        mid = start + radial * rng.uniform(0.003, 0.008) + Vector((0, 0, rng.uniform(0.006, 0.010)))
        end = mid + radial * rng.uniform(0.006, 0.012) + Vector((0, 0, rng.uniform(0.005, 0.010)))
        radius = rng.uniform(0.0017, 0.0026)
        b.tube([start, mid, end], [radius, radius * 0.82, radius * 0.52], 5,
               atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.0, cap=True)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(atlas):
    """Hand-held pulled turnip: grip at origin, globe/root hangs down -Z, trimmed leaves above."""
    rng = random.Random(SEED + 909)
    b = F.Batch(height=0.24)
    phase = 0.42
    # Globe: 7.5 cm white root with a violet crown just below the fist.
    b.sphere(Vector((0, 0, -0.039)), 0.039, atlas.uv("root_ripe", inset=False),
             segs=12, rings=7, stretch=0.98, axis=Vector((0, 0, 1)), phase=phase)
    # Small dirty root tip and clods: enough to read "freshly pulled", not a muddy blob.
    b.sphere(Vector((0.004, -0.003, -0.081)), 0.0045, atlas.uv("soil", inset=False),
             segs=5, rings=3, stretch=1.0, axis=Vector((0, 0, 1)), phase=phase)
    for k in range(4):
        az = k * 1.72 + rng.uniform(-0.25, 0.25)
        r = rng.uniform(0.010, 0.025)
        c = Vector((math.cos(az) * r, math.sin(az) * r, -0.076 + rng.uniform(-0.004, 0.004)))
        b.sphere(c, rng.uniform(0.0025, 0.0045), atlas.uv("soil", inset=False),
                 segs=4, rings=3, stretch=rng.uniform(0.7, 1.3), axis=Vector((0, 0, 1)), phase=phase)
    # Hair roots hanging from the dirty tip.
    for k in range(7):
        az = k * 2.39996 + rng.uniform(-0.25, 0.25)
        start = Vector((math.cos(az) * 0.010, math.sin(az) * 0.010, -0.073))
        end = start + Vector((math.cos(az) * rng.uniform(0.006, 0.018),
                              math.sin(az) * rng.uniform(0.006, 0.018),
                              -rng.uniform(0.020, 0.040)))
        mid = start.lerp(end, 0.5) + Vector((0, 0, rng.uniform(-0.004, 0.003)))
        b.tube([start, mid, end], [0.00045, 0.00032, 0.00016], 3, atlas.uv("stem"),
               v_length=0.35, phase=phase, flutter=0.05)
    # Trimmed leaf tuft: individual petioles plus shortened 12-16 cm blades for a hand-held pull.
    for i in range(6):
        az = i * 2.39996 + rng.uniform(-0.22, 0.22)
        heading = Vector((math.cos(az), math.sin(az), 0))
        old = i >= 4
        elev = math.radians(rng.uniform(48, 72) if not old else rng.uniform(36, 56))
        pts = _arc(Vector((0, 0, 0.002)), heading, elev, rng.uniform(0.025, 0.042),
                   rng.uniform(0.06, 0.16), 2)
        b.tube(pts, [0.0016, 0.0013, 0.0010], 4, atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.12)
        key = "leaf_old" if old else rng.choice(["leaf", "leaf_dark"])
        d = (heading * math.cos(math.radians(rng.uniform(38, 62))) +
             Vector((0, 0, math.sin(math.radians(rng.uniform(38, 62)))))).normalized()
        up = (Vector((0, 0, 1)) - heading * 0.35).normalized()
        length = rng.uniform(0.115, 0.155)
        b.card(pts[-1], d, up, length, length * S.tile_aspect(atlas, key), atlas.uv(key),
               rows=3, cols=2, fold=0.10, droop=rng.uniform(0.04, 0.16), twist=rng.uniform(-0.12, 0.12),
               phase=phase, flutter=1.0, flutter_base=0.25)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    produce_material = material.copy()
    produce_material.name = "M_CropTurnipProduce"
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
