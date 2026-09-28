"""Himalayan blackberry (Rubus armeniacus) bramble thickets: a medium and a large mound.

The real plant (what the geometry and textures follow):
- The dominant bramble of Sierra Nevada foothill creeks, roadsides and clearings. Arching canes
  build impenetrable mounds 1-3 m tall and several metres across; cane tips root where they touch.
- Canes are five-angled, 1-3 cm thick at the base, green flushed purple-red and glaucous when
  young, brown-purple when old, arching up 1-2.5 m then down. Stout hooked prickles 5-10 mm long
  with broad red bases sit mostly on the angles and point back toward the crown.
- First-year canes carry palmately compound leaves of five leaflets on 4-8 cm petioles; second-year
  canes throw short lateral shoots with three-leaflet leaves ending in clusters of pale pink-white
  flowers (2-2.5 cm) and fruit.
- Leaflets are ovate, 6-12 cm, doubly serrate with forward teeth, dark green with impressed veins
  above and grey-felted beneath; the terminal leaflet is largest on the longest stalk.
- Fruit: aggregates of glossy drupelets 1.5-2 cm long, green -> red -> black; late summer carries
  all three at once, held on a star of reflexed sepals.
- Canes die after fruiting and stay in the thicket grey, brittle and leafless, with dry leaves.

Game notes: both mounds BLOCK the player (convex collision) until cleared with a machete. One
shared 4K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py (R height, G branch phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F

NAME = "BlackberryBramble"
DESCRIPTION = ("Himalayan blackberry bramble mounds (medium 1.2 m and large 2 m) with arching prickly "
               "canes, five-leaflet leaves, dead canes, flowers and green/red/black berries. Blocking.")
COLLISION = "convex"
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.10, -0.55, 0.80),
          "meshes": {"SM_BlackberryBrambleLarge": {"focus": (0.2, -0.95, 1.30)}}}
REPORT = {
    "blocking": True,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-cane random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on canes",
             "A": "1"},
    "material_notes": ("One material M_BlackberryBramble: T_BlackberryBramble_basecolor (sRGB, alpha = "
                       "opacity mask, clip 0.5), _normal (OpenGL, flip green for Unreal), _roughness "
                       "(R roughness, G translucency mask, B AO). Two-sided foliage, masked."),
}

SEED = 7717
V_CANE = 0.70            # metres of cane per vertical repeat of a bark column
LEAFLET_TILES = ("dark", "dark2", "mid", "young", "yellow", "holes", "dry", "under")


# ------------------------------------------------------------------ textures

def _palettes():
    return {
        "dark": dict(base=(0.034, 0.060, 0.013), tip=(0.038, 0.066, 0.014), vein=(0.064, 0.090, 0.024),
                     margin=(0.024, 0.042, 0.014), yellow=(0.20, 0.17, 0.04), brown=(0.055, 0.028, 0.012)),
        "dark2": dict(base=(0.040, 0.068, 0.015), tip=(0.046, 0.076, 0.016), vein=(0.074, 0.100, 0.028),
                      margin=(0.026, 0.046, 0.016), yellow=(0.20, 0.17, 0.04), brown=(0.055, 0.028, 0.012)),
        "mid": dict(base=(0.052, 0.086, 0.018), tip=(0.060, 0.094, 0.020), vein=(0.090, 0.118, 0.034),
                    margin=(0.034, 0.058, 0.018), yellow=(0.22, 0.19, 0.05), brown=(0.055, 0.028, 0.012)),
        "young": dict(base=(0.070, 0.112, 0.028), tip=(0.090, 0.080, 0.030), vein=(0.105, 0.135, 0.045),
                      margin=(0.085, 0.050, 0.030), yellow=(0.20, 0.17, 0.04), brown=(0.055, 0.028, 0.012)),
        "yellow": dict(base=(0.060, 0.090, 0.022), tip=(0.110, 0.105, 0.028), vein=(0.120, 0.125, 0.040),
                       margin=(0.090, 0.070, 0.020), yellow=(0.23, 0.19, 0.045), brown=(0.085, 0.040, 0.016)),
        "holes": dict(base=(0.036, 0.066, 0.019), tip=(0.044, 0.074, 0.021), vein=(0.070, 0.100, 0.034),
                      margin=(0.050, 0.045, 0.016), yellow=(0.20, 0.17, 0.04), brown=(0.085, 0.038, 0.015)),
        "dry": dict(base=(0.120, 0.070, 0.032), tip=(0.100, 0.055, 0.026), vein=(0.150, 0.100, 0.050),
                    margin=(0.070, 0.038, 0.018), dry=(0.125, 0.068, 0.030), brown=(0.060, 0.030, 0.014)),
        "under": dict(base=(0.078, 0.092, 0.058), tip=(0.082, 0.095, 0.060), vein=(0.115, 0.125, 0.085),
                      margin=(0.065, 0.078, 0.048), yellow=(0.20, 0.18, 0.08), brown=(0.10, 0.05, 0.02)),
    }


def _leaflet_shape(rng):
    return F.ovate(width=0.285, widest=0.40, tip_sharp=1.25, base_round=0.55, teeth=15,
                   tooth_depth=0.12, double=0.35, phase=rng.uniform(0, 1))


def _leaflet_veins(nrng):
    return F.pinnate_veins(count=9, angle=1.05, curve=0.55, reach=0.92, width=0.285, widest=0.40,
                           start=0.07, stop=0.80, rng=nrng)


LEAFLET_PARAMS = {
    "dark": dict(gloss=0.4),
    "dark2": dict(gloss=0.3, damage=0.04, edge_burn=0.10),
    "mid": dict(gloss=0.25, damage=0.05),
    "young": dict(gloss=0.1, trans=0.7),
    "yellow": dict(yellow=0.65, damage=1.0, edge_burn=0.5),
    "holes": dict(holes=1.0, damage=0.5, edge_burn=0.3),
    "dry": dict(dry=1.0, edge_burn=0.6, holes=0.4, trans=0.25),
    "under": dict(hair=0.8, trans=0.5, gloss=-0.4),
}


def _paint_leaflets(atlas, nrng, rng):
    pals = _palettes()
    for key in LEAFLET_TILES:
        X, Y, px = atlas.grid(key)
        under = key == "under"
        layer = F.paint_blade(X, Y, nrng, _leaflet_shape(rng), _leaflet_veins(nrng), pals[key], px,
                              vein_width=0.0070 if under else 0.0060,
                              vein_depth=-0.00025 if under else 0.00026,
                              puff=0.00010 if under else 0.00020, tertiary=0.35, stalk=0.011,
                              **LEAFLET_PARAMS[key])
        h = atlas.rects[key][3] - 2 * atlas.pad
        atlas.put(key, layer, normal_strength=1.0, meters_per_px=0.085 / h)


def _rotated(X, Y, origin, theta, scale):
    dx, dy = X - origin[0], Y - origin[1]
    c, s = math.cos(theta), math.sin(theta)
    return (dx * c - dy * s) / scale, (dx * s + dy * c) / scale


COMPOUND = [  # (angle from the petiole axis, length, attach offset along the axis) in tile units
    (math.radians(-100), 0.27, 0.0), (math.radians(100), 0.27, 0.0),
    (math.radians(-50), 0.40, 0.02), (math.radians(50), 0.40, 0.02),
    (0.0, 0.54, 0.045),
]
COMPOUND_ATTACH = (0.0, 0.21)


def _paint_compound(atlas, nrng, rng):
    """Whole five-leaflet leaf seen from above, for the far LOD's single-card leaves."""
    pals = _palettes()
    X, Y, px = atlas.grid("compound")
    layer = F.Layer(X.shape)
    layer.color[...] = pals["dark"]["base"]
    d, _ = F.polyline_distance(X, Y, [(0.0, 0.0), COMPOUND_ATTACH])
    stalk = np.clip(-(d - 0.008) / px + 0.5, 0, 1)
    keys = ["dark", "mid", "dark", "mid", "dark"]
    for (theta, length, offset), key in zip(COMPOUND, keys):
        origin = (COMPOUND_ATTACH[0] + math.sin(theta) * offset, COMPOUND_ATTACH[1] + math.cos(theta) * offset)
        Xl, Yl = _rotated(X, Y, origin, theta, length)
        leaf = F.paint_blade(Xl, Yl, nrng, _leaflet_shape(rng), _leaflet_veins(nrng), pals[key], px / length,
                             vein_width=0.0075, vein_depth=0.00026, puff=0.0002, tertiary=0.3, stalk=0.013,
                             **LEAFLET_PARAMS[key])
        layer.over(leaf)
    stalk_layer = F.Layer(X.shape)
    stalk_layer.color[...] = (0.07, 0.05, 0.025)
    stalk_layer.alpha = stalk * (1 - layer.alpha)
    stalk_layer.trans[...] = 0.1
    layer.over(stalk_layer)
    atlas.put("compound", layer, meters_per_px=0.2 / X.shape[0])


def _paint_flower(atlas, nrng, rng):
    X, Y, px = atlas.grid("flower")
    cy = 0.5
    layer = F.Layer(X.shape)
    layer.color[...] = (0.62, 0.52, 0.50)
    sepal = F.ovate(width=0.16, widest=0.30, tip_sharp=2.2, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(36 + 72 * k + rng.uniform(-6, 6))
        Xl, Yl = _rotated(X, Y, (0.0, cy), th, 0.36)
        leaf = F.paint_blade(Xl, Yl, nrng, sepal, [([(0, 0), (0, 1)], 0.6)],
                             dict(base=(0.085, 0.105, 0.055), tip=(0.10, 0.12, 0.07), vein=(0.10, 0.12, 0.06),
                                  margin=(0.14, 0.15, 0.11)), px / 0.36, hair=1.0, tertiary=0.0, trans=0.3)
        layer.over(leaf)
    petal = F.ovate(width=0.40, widest=0.70, tip_sharp=0.35, base_round=1.4, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(72 * k + rng.uniform(-9, 9))
        length = 0.43 * rng.uniform(0.92, 1.05)
        Xl, Yl = _rotated(X, Y, (0.0, cy), th, length)
        fan = [([(0, 0.05), (0.18 * s, 0.95)], 0.35) for s in (-1, -0.5, 0, 0.5, 1)]
        leaf = F.paint_blade(Xl, Yl, nrng, petal, fan,
                             dict(base=(0.66, 0.50, 0.52), tip=(0.78, 0.70, 0.70), vein=(0.62, 0.46, 0.49),
                                  margin=(0.80, 0.74, 0.74)), px / length,
                             vein_width=0.012, vein_depth=0.0002, puff=0.0004, tertiary=0.0, trans=0.85,
                             gloss=-0.2)
        layer.over(leaf)
    r = np.hypot(X, Y - cy)
    th = np.arctan2(Y - cy, X)
    n = F.noise(X.shape, nrng, freq=60.0, beta=1.5)
    filament = np.exp(-(((th * 70 / (2 * math.pi)) % 1.0 - 0.5) / 0.18) ** 2)
    ring = F.smoothstep(0.075, 0.09, r) * (1 - F.smoothstep(0.19, 0.215, r)) * filament
    anther = F.smoothstep(0.17, 0.19, r) * (1 - F.smoothstep(0.20, 0.22, r)) * filament
    disc = 1 - F.smoothstep(0.07, 0.085, r)
    color = layer.color
    color = F.lerp(color, (0.62, 0.62, 0.52), ring)
    color = F.lerp(color, (0.50, 0.36, 0.08), anther)
    color = F.lerp(color, F.lerp((0.20, 0.24, 0.06), (0.34, 0.36, 0.10), n), disc)
    layer.color = color
    layer.height = layer.height + 0.0004 * disc * n + 0.00015 * ring
    layer.trans = np.where(disc > 0.5, 0.2, layer.trans)
    layer.rough = np.where(disc + ring > 0.5, 0.6, layer.rough)
    atlas.put("flower", layer, meters_per_px=0.025 / X.shape[0])

    # Calyx star behind each berry.
    X, Y, px = atlas.grid("calyx")
    layer = F.Layer(X.shape)
    layer.color[...] = (0.09, 0.11, 0.05)
    for k in range(5):
        th = math.radians(72 * k + rng.uniform(-8, 8))
        Xl, Yl = _rotated(X, Y, (0.0, 0.5), th, 0.46)
        leaf = F.paint_blade(Xl, Yl, nrng, sepal, [([(0, 0), (0, 1)], 0.6)],
                             dict(base=(0.075, 0.095, 0.045), tip=(0.10, 0.10, 0.06), vein=(0.10, 0.12, 0.06),
                                  margin=(0.13, 0.13, 0.09), brown=(0.09, 0.05, 0.02)), px / 0.46,
                             hair=1.0, tertiary=0.0, trans=0.3, edge_burn=0.4)
        layer.over(leaf)
    atlas.put("calyx", layer, meters_per_px=0.02 / X.shape[0])


def _paint_berries(atlas, nrng):
    looks = {
        "berry_black": ((0.012, 0.006, 0.014), (0.030, 0.006, 0.012), 0.20),
        "berry_red": ((0.20, 0.012, 0.020), (0.10, 0.006, 0.010), 0.30),
        "berry_green": ((0.13, 0.17, 0.045), (0.070, 0.090, 0.025), 0.40),
    }
    for key, (dome, crevice, rough) in looks.items():
        U, V = atlas.column_grid(key)
        rows, around = 7, 9
        rv = V * rows
        ri = np.floor(rv)
        fv = rv - ri - 0.5
        cu = U * around + (ri % 2) * 0.5
        fu = cu - np.floor(cu) - 0.5
        d = np.hypot(fu * 1.1, fv)
        dome_h = np.sqrt(np.clip(0.22 - d * d, 0, None))
        cell_rand = (np.sin(np.floor(cu) * 12.9898 + ri * 78.233) * 43758.5453) % 1.0
        tint = 0.75 + 0.5 * cell_rand
        layer = F.Layer(U.shape)
        k = F.smoothstep(0.05, 0.40, dome_h)
        layer.color = F.lerp(crevice, np.asarray(dome) * tint[..., None], k)
        style = np.exp(-(d / 0.07) ** 2)
        layer.color = F.lerp(layer.color, (0.12, 0.10, 0.07), style * 0.6)
        poles = F.smoothstep(0.0, 0.08, V) * (1 - F.smoothstep(0.94, 1.0, V))
        layer.color = F.lerp((0.06, 0.07, 0.03), layer.color, poles)
        layer.height = dome_h * 0.0016
        layer.rough = rough + 0.35 * (1 - k)
        layer.trans[...] = 0.05
        layer.ao = 0.55 + 0.45 * k
        h, w = U.shape
        atlas.put(key, layer, meters_per_px=0.05 / h, opaque=True)


def _paint_bark(atlas, nrng):
    for key in ("cane_green", "cane_old", "cane_dead", "prickle"):
        U, V = atlas.column_grid(key)
        shp = U.shape
        ridges = np.abs(np.cos(U * 5 * math.pi)) ** 3
        stri = F.noise(shp, nrng, freq=40.0, beta=1.8, aniso=(1.0, 14.0))
        blot = F.noise(shp, nrng, freq=5.0, beta=2.6, aniso=(1.0, 3.0))
        fine = F.noise(shp, nrng, freq=150.0, beta=1.2)
        layer = F.Layer(shp)
        if key == "cane_green":
            base = F.lerp((0.058, 0.082, 0.026), (0.085, 0.036, 0.034), F.smoothstep(0.45, 0.80, blot) * 0.75)
            base = F.lerp(base, (0.11, 0.115, 0.10), F.smoothstep(0.55, 0.85, 1 - blot) * 0.35 * (1 - ridges))
            base = base * (0.85 + 0.3 * stri)[..., None]
            base = F.lerp(base, (0.16, 0.15, 0.11), F.smoothstep(0.93, 1.0, fine) * 0.6)
            rough = 0.45 + 0.15 * (1 - ridges) + 0.1 * stri
            height = 0.00035 * ridges + 0.00008 * stri
        elif key == "cane_old":
            base = F.lerp((0.075, 0.034, 0.024), (0.105, 0.075, 0.055), F.smoothstep(0.45, 0.75, blot))
            base = base * (0.8 + 0.4 * stri)[..., None]
            cracks = F.smoothstep(0.90, 0.98, F.ridged(F.noise(shp, nrng, freq=25.0, beta=2.2, aniso=(1.0, 6.0))))
            base = F.lerp(base, (0.03, 0.018, 0.012), cracks * 0.7)
            rough = 0.62 + 0.15 * stri
            height = 0.0003 * ridges + 0.0001 * stri - 0.00015 * cracks
        elif key == "cane_dead":
            base = F.lerp((0.135, 0.118, 0.092), (0.085, 0.072, 0.056), F.smoothstep(0.35, 0.75, blot))
            base = base * (0.78 + 0.45 * stri)[..., None]
            splits = F.smoothstep(0.92, 0.985, F.ridged(F.noise(shp, nrng, freq=18.0, beta=2.0, aniso=(1.0, 20.0))))
            base = F.lerp(base, (0.030, 0.024, 0.018), splits * 0.8)
            lichen = F.smoothstep(0.80, 0.9, F.noise(shp, nrng, freq=20.0, beta=2.2))
            base = F.lerp(base, (0.19, 0.20, 0.14), lichen * 0.5)
            rough = 0.82 + 0.1 * stri
            height = 0.00025 * ridges + 0.00012 * stri - 0.0002 * splits
        else:  # prickle: v runs base -> tip
            base = F.lerp((0.120, 0.026, 0.032), (0.36, 0.27, 0.13), F.smoothstep(0.35, 0.95, V))
            base = base * (0.9 + 0.2 * stri)[..., None]
            rough = 0.38 + 0.1 * stri
            height = 0.00005 * stri
        layer.color = base
        layer.rough = rough
        layer.height = height
        layer.trans[...] = 0.05
        layer.ao = 0.85 + 0.15 * ridges
        atlas.put(key, layer, meters_per_px=V_CANE / shp[0], wrap=True, opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    for key in ("cane_green", "cane_old", "cane_dead"):
        atlas.column(key, 224)
    atlas.column("prickle", 48)
    for key in LEAFLET_TILES:
        atlas.tile(key, 800, 1200)
    atlas.tile("compound", 1152, 1152)
    atlas.tile("flower", 512, 512)
    atlas.tile("calyx", 320, 320)
    for key in ("berry_black", "berry_red", "berry_green"):
        atlas.tile(key, 256, 256)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    _paint_leaflets(atlas, nrng, rng)
    _paint_compound(atlas, nrng, rng)
    _paint_flower(atlas, nrng, rng)
    _paint_berries(atlas, nrng)
    _paint_bark(atlas, nrng)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ plant description

MEDIUM = dict(name="SM_BlackberryBramble", height=1.22, reach=1.0, canes=34, dead=5, laterals=8,
              crowns=[(0.0, 0.0), (0.16, 0.10), (-0.14, 0.12), (0.05, -0.15)], radius=0.0085,
              branch=0.30, shoots=0.05, leaf_spacing=(0.055, 0.075), full=0.25, leaf_scale=1.2, cane_length=1.55, suckers=10,
              cane_step=0.072,
              seed=SEED)
LARGE = dict(name="SM_BlackberryBrambleLarge", height=2.02, reach=1.25, canes=46, dead=9, laterals=14,
             crowns=[(0.0, 0.0), (0.42, 0.18), (-0.38, 0.22), (0.18, -0.38), (-0.28, -0.28), (0.55, -0.16)],
             radius=0.0115, branch=0.35, shoots=0.04, leaf_spacing=(0.060, 0.080), full=0.16, cane_length=2.45, suckers=18, cane_step=0.085,
             leaf_scale=1.45, seed=SEED + 101)


def _bezier(p0, p1, p2, p3, t):
    u = 1 - t
    return p0 * u ** 3 + p1 * 3 * u * u * t + p2 * 3 * u * t * t + p3 * t ** 3


def _resample(points, step):
    out = [points[0]]
    acc = 0.0
    for a, b in zip(points, points[1:]):
        acc += (b - a).length
        if acc >= step:
            out.append(b)
            acc = 0.0
    if out[-1] is not points[-1]:
        out.append(points[-1])
    return out


def _cane_path(rng, p0, horiz, reach, peak, tip_z, elev, drift=0.0, wobble=0.035):
    """Arching cane: leaves ``p0`` at elevation ``elev``, tops out at ``peak`` and comes down to
    ``tip_z`` at horizontal distance ``reach`` (drifting sideways by ``drift`` radians)."""
    p0 = Vector(p0)
    up = Vector((0, 0, 1))
    side = Vector((-horiz.y, horiz.x, 0))
    rise = max(peak - p0.z, 0.05)
    d0 = horiz * math.cos(elev) + up * math.sin(elev)
    p1 = p0 + d0 * (rise * 0.62 / max(math.sin(elev), 0.2))
    p2 = p0 + horiz * reach * rng.uniform(0.48, 0.62) + up * rise * 1.12
    p3 = p0 + horiz * reach + up * (tip_z - p0.z)
    pts = [_bezier(p0, p1, p2, p3, t) for t in np.linspace(0, 1, 140)]
    top = max(p.z for p in pts)
    scale = rise / max(top - p0.z, 1e-3)
    phase = rng.uniform(0, math.tau)
    out = []
    for i, p in enumerate(pts):
        t = i / (len(pts) - 1)
        rel = Vector((p.x - p0.x, p.y - p0.y, (p.z - p0.z) * scale))
        rel = Matrix.Rotation(drift * t * t, 3, "Z") @ rel
        q = p0 + rel + side * (wobble * reach * math.sin(phase + t * 5.0) * t)
        q.z = max(q.z, 0.012)
        out.append(q)
    return _resample(out, 0.012)


def _arc_path(rng, p0, horiz, length, e0, e_end, power, drift=0.0, wobble=0.03, root_tip=False):
    """Cane that bends over under its own weight: the heading falls from ``e0`` (elevation, rad)
    to ``e_end`` along its ``length`` (theta = e0 - (e0 - e_end) * s^power). With ``root_tip`` the
    tip keeps descending until it reaches the soil, as blackberry tips do."""
    p = Vector(p0)
    side = Vector((-horiz.y, horiz.x, 0))
    step = 0.012
    n = max(8, int(length / step))
    phase = rng.uniform(0, math.tau)
    out = [p.copy()]
    s = 0.0
    while True:
        s += step
        t = min(s / length, 1.0)
        theta = e0 - (e0 - e_end) * t ** power
        az_turn = drift * t * t
        h = Matrix.Rotation(az_turn, 3, "Z") @ horiz
        d = h * math.cos(theta) + Vector((0, 0, math.sin(theta)))
        d += side * (wobble * math.cos(phase + s * 3.3))
        p = p + d.normalized() * step
        if p.z < 0.012 and s > 0.3:
            p.z = 0.012
            out.append(p.copy())
            break
        out.append(p.copy())
        if s >= length and not (root_tip and p.z > 0.03) or s > length * 1.35:
            break
    return out


def _arclength(pts):
    acc = [0.0]
    for a, b in zip(pts, pts[1:]):
        acc.append(acc[-1] + (b - a).length)
    return acc


def _at(pts, acc, s):
    for i in range(1, len(pts)):
        if acc[i] >= s:
            f = (s - acc[i - 1]) / max(acc[i] - acc[i - 1], 1e-9)
            return pts[i - 1].lerp(pts[i], f), (pts[i] - pts[i - 1]).normalized(), i
    return pts[-1], (pts[-1] - pts[-2]).normalized(), len(pts) - 1


def _zigzag(pts, acc, spacing, amount):
    """Blackberry canes kink slightly at every node."""
    chord = pts[-1] - pts[0]
    lateral = Vector((0, 0, 1)).cross(chord)
    lateral = lateral.normalized() if lateral.length > 1e-4 else Vector((1, 0, 0))
    return [p + lateral * (amount * math.sin(math.pi * s / spacing)) for p, s in zip(pts, acc)]


def _leaf(rng, node, tangent, outward, surface, size, key, leaflets=5, variant=None, full=True, around=None):
    """One compound leaf: petiole plus leaflet placements, facing the mound surface. Petioles
    spiral round the cane (2/5 phyllotaxy), so leaves stand off to alternating sides."""
    up = Vector((0, 0, 1))
    if around is None:
        around = rng.uniform(0, math.tau)
    radial = Matrix.Rotation(around, 3, tangent) @ tangent.orthogonal().normalized()
    lateral = radial - up * radial.dot(up)
    lateral = lateral.normalized() if lateral.length > 0.2 else outward
    pet_dir = (lateral * 0.55 + up * 0.55 + surface * 0.35).normalized()
    pet_len = rng.uniform(0.040, 0.068) * size
    mid = node + pet_dir * pet_len * 0.55
    jitter = Vector((rng.uniform(-.3, .3), rng.uniform(-.3, .3), rng.uniform(-.15, .15)))
    leaf_up = (up * 0.9 + surface * 0.4 + jitter).normalized()
    present = (lateral * 0.75 + outward * 0.3 + jitter * 0.8).normalized()
    end = mid + (pet_dir * 0.4 + present * 0.6).normalized() * pet_len * 0.45
    axis = present - leaf_up * present.dot(leaf_up)
    if axis.length < 1e-3:
        axis = leaf_up.orthogonal()
    axis.normalize()
    layout = COMPOUND if leaflets == 5 else [COMPOUND[2], COMPOUND[3], COMPOUND[4]]
    items = []
    base_size = 0.085 * size
    for theta, length, offset in layout:
        d = Matrix.Rotation(-theta + rng.uniform(-0.12, 0.12), 3, leaf_up) @ axis
        rel = length / 0.54 if leaflets == 5 else max(length / 0.54, 0.8)
        k = variant or ("under" if rng.random() < 0.03 else key)
        lk = base_size * rel * rng.uniform(0.9, 1.08)
        upk = Matrix.Rotation(rng.uniform(-0.3, 0.3), 3, d) @ leaf_up
        dry = k == "dry"
        items.append(dict(base=end + axis * (offset * base_size / 0.54), dir=d, up=upk, length=lk,
                          width=lk * 0.667, key=k, fold=rng.uniform(0.15, 0.35) + (0.25 if dry else 0),
                          droop=rng.uniform(0.10, 0.38) + (0.35 if dry else 0),
                          curl=rng.uniform(0.0, 0.1) + (0.45 if dry else 0),
                          twist=rng.uniform(-0.25, 0.25) + (rng.uniform(-0.7, 0.7) if dry else 0)))
    card_key = "compound" if (variant or key) not in ("dry", "yellow") else (variant or key)
    return dict(petiole=[node, mid, end], leaflets=items, axis=axis, up=leaf_up, end=end,
                size=base_size * rng.uniform(0.95, 1.05), key=card_key, full=full,
                droop=rng.uniform(0.05, 0.25))


def describe(spec):
    """Random decisions for one mound, shared by every LOD."""
    rng = random.Random(spec["seed"])
    H, R = spec["height"], spec["reach"]
    crowns = [Vector((x, y, -0.01)) for x, y in spec["crowns"]]
    centre = sum(crowns, Vector()) / len(crowns)
    extent = R + max((Vector((c.x, c.y, 0)) - Vector((centre.x, centre.y, 0))).length for c in crowns)
    canes = []

    def outward_at(p):
        o = Vector((p.x - centre.x, p.y - centre.y, 0))
        return o.normalized() if o.length > 1e-3 else Vector((1, 0, 0))

    def surface_at(p):
        g = Vector(((p.x - centre.x) / extent ** 2, (p.y - centre.y) / extent ** 2, max(p.z, 0.05) / H ** 2))
        return g.normalized()

    def outerness(p):
        dxy = Vector((p.x - centre.x, p.y - centre.y)).length
        return math.sqrt((dxy / extent) ** 2 + (p.z / H) ** 2)

    def leaf_key():
        return rng.choices(["dark", "dark2", "mid", "holes", "yellow"], weights=[34, 30, 24, 7, 5])[0]

    def add_cane(kind, pts, radius, phase, leafy=True, zig=True):
        # Keep the footprint and height inside the mound envelope.
        base = pts[0]
        far = max(Vector((q.x - base.x, q.y - base.y)).length for q in pts)
        limit = spec["reach"] * 1.1
        if kind in ("primo", "flori", "dead") and far > limit:
            k = limit / far
            pts = [Vector((base.x + (q.x - base.x) * k, base.y + (q.y - base.y) * k, q.z)) for q in pts]
        top = max(q.z for q in pts)
        if top > H:
            pts = [Vector((q.x, q.y, 0.012 + (q.z - 0.012) * (H - 0.012) / (top - 0.012))) for q in pts]
        acc = _arclength(pts)
        L = acc[-1]
        radii = [radius * 0.28 + radius * 0.72 * (1 - s / L) ** 0.85 for s in acc]
        cane = dict(kind=kind, pts=pts, acc=acc, radii=radii, phase=phase, leaves=[], prickles=[],
                    fruit=[], flowers=[])
        canes.append(cane)
        spacing = spec["leaf_spacing"]
        s = L * (rng.uniform(0.05, 0.14) if kind != "shoot" else 0.2)
        while leafy and s < L - 0.02:
            node, tan, _ = _at(pts, acc, s)
            o = outerness(node)
            young = s > L - 0.25 and kind in ("primo", "shoot")
            size = (0.6 + 0.4 * min(1.0, (L - s) / 0.35)) * spec["leaf_scale"]
            full = o >= spec["full"] and kind != "dead"
            if kind == "dead":
                variant = "dry"
            elif young:
                variant = "young"
            elif o < 0.5:
                if rng.random() < 0.5:
                    s += rng.uniform(*spacing)
                    continue
                variant = rng.choice(["yellow", "dry", "holes", "mid", "dark2"])
            else:
                variant = None
            cane["leaves"].append(_leaf(rng, node, tan, outward_at(node), surface_at(node), size,
                                        leaf_key(), leaflets=3 if kind == "lateral" else 5,
                                        variant=variant, full=full,
                                        around=len(cane["leaves"]) * 2.513 + rng.uniform(-0.35, 0.35)))
            if kind == "primo" and L * 0.35 < s < L * 0.85 and rng.random() < spec["shoots"]:
                sd = (outward_at(node) * 0.6 + Vector((0, 0, 0.5)) + tan * 0.3).normalized()
                ln = rng.uniform(0.12, 0.28)
                spts = [node + sd * ln * t + Vector((0, 0, -0.05 * t * t)) for t in np.linspace(0, 1, 7)]
                pending.append(("shoot", spts, radius * 0.38, phase))
            s += rng.uniform(*spacing) if kind != "dead" else rng.uniform(0.3, 0.6)
        if kind in ("primo", "flori", "dead"):
            s = 0.02
            while s < L - 0.05:
                node, tan, i = _at(pts, acc, s)
                if outerness(node) < 0.62:        # hidden inside the mound
                    s += 0.05
                    continue
                r = radii[i]
                ang = rng.randrange(5) * math.tau / 5 + rng.uniform(-0.15, 0.15)
                ring_dir = Matrix.Rotation(ang, 3, tan) @ tan.orthogonal().normalized()
                d = (ring_dir * 0.78 - tan * 0.62).normalized()
                length = rng.uniform(0.0045, 0.0075) * (0.6 + 0.4 * min(1.0, r / 0.008))
                cane["prickles"].append((node + ring_dir * r * 0.85, d, length, max(0.0012, r * 0.32), tan))
                s += rng.uniform(0.035, 0.060)
        return cane

    pending = []
    for c in range(spec["canes"]):
        crown = crowns[c % len(crowns)] + Vector((rng.uniform(-0.05, 0.05), rng.uniform(-0.05, 0.05), 0))
        az = rng.uniform(0, math.tau)
        horiz = Vector((math.cos(az), math.sin(az), 0))
        reach_frac = rng.uniform(0.35, 1.0)
        reach = R * reach_frac
        if rng.random() < 0.6:
            # Vigorous central canes: climb steeply, then crook over and hang (shepherd's crook).
            e0, power = math.radians(rng.uniform(72, 86)), rng.uniform(1.7, 2.3)
            length = spec["cane_length"] * rng.uniform(0.75, 1.1)
        else:
            # Rim canes: low wide arches that run out over the ground and root.
            e0, power = math.radians(rng.uniform(38, 62)), rng.uniform(1.1, 1.5)
            length = spec["cane_length"] * rng.uniform(0.40, 0.62)
        pts = _arc_path(rng, crown, horiz, length, e0, math.radians(-rng.uniform(40, 85)), power,
                        drift=rng.uniform(-1.1, 1.1), root_tip=rng.random() < 0.45)
        top = max(q.z for q in pts)
        if top > H:
            pts = [Vector((q.x, q.y, 0.012 + (q.z - 0.012) * H / top)) for q in pts]
        kind = "flori" if c % 3 == 1 else "primo"
        radius = spec["radius"] * rng.uniform(0.75, 1.15)
        cane = add_cane(kind, pts, radius, rng.random())
        if rng.random() < spec["branch"]:
            s = cane["acc"][-1] * rng.uniform(0.35, 0.55)
            node, tan, _ = _at(cane["pts"], cane["acc"], s)
            haz = az + rng.choice([-1, 1]) * rng.uniform(0.5, 1.0)
            bh = Vector((math.cos(haz), math.sin(haz), 0))
            bpts = _cane_path(rng, node, bh, reach * rng.uniform(0.4, 0.65),
                              min(H, node.z + rng.uniform(0.05, 0.22)), rng.choice([0.02, node.z * 0.5]),
                              elev=math.radians(rng.uniform(10, 35)), drift=rng.uniform(-0.5, 0.5))
            add_cane("primo", bpts, radius * 0.6, (cane["phase"] + 0.3) % 1.0)
        while pending:
            kind_s, spts, rad, ph = pending.pop()
            add_cane(kind_s, spts, rad, (ph + rng.uniform(0.05, 0.2)) % 1.0, zig=False)

    # Short leafy suckers around the skirt close the gaps under the arches.
    for c in range(spec.get("suckers", 0)):
        ang = rng.uniform(0, math.tau)
        dist = extent * rng.uniform(0.25, 0.7)
        base = centre + Vector((math.cos(ang) * dist, math.sin(ang) * dist, -0.01))
        az = ang + rng.uniform(-1.2, 1.2)
        pts = _arc_path(rng, base, Vector((math.cos(az), math.sin(az), 0)), rng.uniform(0.35, 0.8),
                        math.radians(rng.uniform(50, 78)), math.radians(-rng.uniform(20, 60)),
                        rng.uniform(1.2, 1.6), drift=rng.uniform(-0.8, 0.8))
        add_cane("primo", pts, spec["radius"] * rng.uniform(0.45, 0.65), rng.random())
        while pending:
            pending.pop()

    flori = [c for c in canes if c["kind"] == "flori"]
    for k in range(spec["laterals"]):
        if not flori:
            break
        parent = flori[k % len(flori)]
        s = parent["acc"][-1] * rng.uniform(0.35, 0.75)
        node, tan, _ = _at(parent["pts"], parent["acc"], s)
        direction = (outward_at(node) * 0.55 + Vector((0, 0, 1)) * 0.75 + tan * 0.2).normalized()
        length = rng.uniform(0.14, 0.30) * (H / 1.2) ** 0.3
        pts = [node + direction * length * t + Vector((0, 0, -0.04 * t * t)) +
               outward_at(node) * 0.03 * math.sin(math.pi * t) for t in np.linspace(0, 1, 9)]
        lat = add_cane("lateral", pts, 0.0032, parent["phase"], zig=False)
        # Spring canes carry no flowers or fruit yet (blackberry flowers from late May).
        if not spec.get("fruit", True):
            continue
        end, tan_end, _ = _at(lat["pts"], lat["acc"], lat["acc"][-1])
        for j in range(rng.randint(4, 10)):
            az = j * 2.39996 + rng.uniform(-0.4, 0.4)
            spread = rng.uniform(0.35, 1.1)
            radial = Matrix.Rotation(az, 3, tan_end) @ tan_end.orthogonal().normalized()
            ped_dir = (tan_end * math.cos(spread) + radial * math.sin(spread)).normalized()
            ped_dir = (ped_dir + Vector((0, 0, -0.35))).normalized()
            start = end - tan_end * rng.uniform(0.0, 0.05)
            tip = start + ped_dir * rng.uniform(0.012, 0.028)
            kind = rng.choices(["berry_black", "berry_red", "berry_green", "flower"], weights=[48, 26, 16, 10])[0]
            item = dict(start=start, tip=tip, dir=ped_dir, kind=kind, scale=rng.uniform(0.85, 1.12),
                        phase=parent["phase"], spin=rng.uniform(0, math.tau))
            (lat["flowers"] if kind == "flower" else lat["fruit"]).append(item)

    for c in range(spec["dead"]):
        crown = crowns[c % len(crowns)] + Vector((rng.uniform(-0.06, 0.06), rng.uniform(-0.06, 0.06), 0))
        az = rng.uniform(0, math.tau)
        horiz = Vector((math.cos(az), math.sin(az), 0))
        peak = H * rng.uniform(0.35, 0.75)
        pts = _cane_path(rng, crown, horiz, R * rng.uniform(0.35, 0.85), peak, rng.uniform(0.02, peak * 0.6),
                         elev=math.radians(rng.uniform(30, 60)), drift=rng.uniform(-0.6, 0.6), wobble=0.06)
        if rng.random() < 0.55:
            pts = pts[: max(6, int(len(pts) * rng.uniform(0.45, 0.8)))]
        add_cane("dead", pts, spec["radius"] * rng.uniform(0.9, 1.25), rng.random())

    litter = []
    for _ in range(int(14 * (extent / 0.95) ** 2)):
        ang = rng.uniform(0, math.tau)
        dist = extent * 0.75 * math.sqrt(rng.random())
        pos = centre + Vector((math.cos(ang) * dist, math.sin(ang) * dist, 0.0))
        pos.z = 0.006
        yaw = rng.uniform(0, math.tau)
        litter.append(dict(base=pos, dir=Vector((math.cos(yaw), math.sin(yaw), 0)), up=Vector((0, 0, 1)),
                           length=rng.uniform(0.055, 0.085), fold=0.35, droop=-0.1, curl=0.5,
                           twist=rng.uniform(-0.6, 0.6)))
    # The outermost leaves (what the eye sees) get individual leaflets; the rest are cards.
    ranked = sorted((l for c in canes for l in c["leaves"] if l["key"] == "compound"),
                    key=lambda l: -outerness(l["end"]))
    for i, leaf in enumerate(ranked):
        leaf["full"] = i < int(len(ranked) * spec["full"])
    return dict(canes=canes, litter=litter, height=H, cane_step=spec["cane_step"])


# ------------------------------------------------------------------ geometry per LOD

CANE_COLUMN = {"primo": "cane_green", "shoot": "cane_green", "lateral": "cane_green", "flori": "cane_old",
               "dead": "cane_dead"}
CANE_SIDES = {"primo": (5, 4, 3), "flori": (5, 4, 3), "dead": (5, 4, 3), "shoot": (4, 3, 3), "lateral": (4, 3, 3)}


def _compound_card(b, leaf, uv, cols, rows, scale, phase, fold=0.08):
    L = leaf["size"] / 0.54 * scale
    key = leaf["key"]
    rect = uv(key)
    if key != "compound":       # dry/yellow leaves use a single leaflet tile, sized to match
        L = leaf["size"] * scale * 1.05
        b.card(leaf["end"], leaf["axis"], leaf["up"], L, L * 0.667, rect, rows=rows, cols=cols, fold=0.3,
               droop=leaf["droop"] + 0.2, phase=phase, flutter=1.0, flutter_base=0.3)
        return
    b.card(leaf["end"] - leaf["axis"] * COMPOUND_ATTACH[1] * L, leaf["axis"], leaf["up"], L, L, rect,
           rows=rows, cols=cols, fold=fold, droop=leaf["droop"], phase=phase, flutter=1.0, flutter_base=0.3)


def emit(desc, atlas, lod):
    b = F.Batch(height=desc["height"])
    uv = atlas.uv
    stats = {"canes": 0, "leaves": 0}
    for cane in desc["canes"]:
        t_start = b.triangles
        pts, radii, kind = cane["pts"], cane["radii"], cane["kind"]
        if kind in ("lateral", "shoot"):
            stride = (1, 2, 3)[lod]
        else:
            stride = max(1, int(round((desc["cane_step"], 0.12, 0.22)[lod] / 0.012)))
        idx = list(range(0, len(pts), stride))
        if idx[-1] != len(pts) - 1:
            idx.append(len(pts) - 1)
        if lod == 2 and kind in ("shoot", "lateral") and cane["acc"][-1] < 0.2:
            idx = [0, len(pts) - 1]
        b.tube([pts[i] for i in idx], [radii[i] for i in idx], CANE_SIDES[kind][lod], uv(CANE_COLUMN[kind]),
               v_length=V_CANE, v_offset=cane["phase"], phase=cane["phase"], cap=lod == 0,
               roll=cane["phase"] * 6.28)
        if lod == 0:
            for base, d, length, rad, tan in cane["prickles"]:
                b.cone(base - d * rad * 0.4, d, length, rad, uv("prickle"), sides=3, phase=cane["phase"],
                       hook=0.35, up=tan)
        t_cane = b.triangles
        stats["canes"] += t_cane - t_start
        for li, leaf in enumerate(cane["leaves"]):
            ph = cane["phase"]
            if lod == 2:
                _compound_card(b, leaf, uv, 1, 1, 1.12, ph, fold=0.0)
                continue
            if lod == 1:
                if leaf["full"]:
                    for lf in leaf["leaflets"]:
                        b.card(lf["base"], lf["dir"], lf["up"], lf["length"], lf["width"], uv(lf["key"]),
                               rows=1, cols=2, fold=lf["fold"], droop=lf["droop"], twist=lf["twist"],
                               phase=ph, flutter=1.0, flutter_base=0.3)
                else:
                    _compound_card(b, leaf, uv, 1, 1, 1.05, ph)
                continue
            if not leaf["full"]:
                b.tube([leaf["petiole"][0], leaf["end"]], [0.0019, 0.0013], 3, uv("cane_green"),
                       v_length=V_CANE, phase=ph, flutter=0.25)
                _compound_card(b, leaf, uv, 2, 2, 1.0, ph)
                continue
            b.tube(leaf["petiole"], [0.0019, 0.0016, 0.0013], 3, uv("cane_green"), v_length=V_CANE, phase=ph,
                   flutter=0.25)
            for lf in leaf["leaflets"]:
                rows = 3 if lf is leaf["leaflets"][-1] else 2
                b.card(lf["base"], lf["dir"], lf["up"], lf["length"], lf["width"], uv(lf["key"]), rows=rows,
                       cols=2, fold=lf["fold"], curl=lf["curl"], droop=lf["droop"], twist=lf["twist"],
                       phase=ph, flutter=1.0, flutter_base=0.3)
        t_leaf = b.triangles
        stats["leaves"] += t_leaf - t_cane
        for item in cane["fruit"] + cane["flowers"]:
            if lod == 0:
                b.tube([item["start"], item["tip"]], [0.0010, 0.0008], 3, uv("cane_green"), v_length=V_CANE,
                       phase=item["phase"], flutter=0.3)
            if item["kind"] == "flower":
                if lod < 2:
                    b.flat(item["tip"], item["dir"], 0.024 * item["scale"], uv("flower"), spin=item["spin"],
                           cup=0.12, phase=item["phase"], segs=2 if lod == 0 else 1)
                continue
            r = 0.0082 * item["scale"] * (0.8 if item["kind"] == "berry_green" else 1.0)
            centre = item["tip"] + item["dir"] * r * 1.05
            segs, rings = ((6, 4), (4, 3), (3, 2))[lod]
            b.sphere(centre, r, uv(item["kind"], inset=False), segs=segs, rings=rings, stretch=1.12,
                     axis=item["dir"], phase=item["phase"])
            if lod == 0:
                b.flat(item["tip"] + item["dir"] * 0.0015, item["dir"], 0.021 * item["scale"], uv("calyx"),
                       spin=item["spin"], cup=-0.25, phase=item["phase"], segs=1)
    stats["fruit"] = b.triangles - stats["canes"] - stats["leaves"]
    print("HOMESTEAD_TRIS", lod, stats)
    if lod < 2:
        for lf in desc["litter"]:
            b.card(lf["base"], lf["dir"], lf["up"], lf["length"], lf["length"] * 0.667, uv("dry"),
                   rows=(2, 1)[lod], cols=2, fold=lf["fold"], curl=lf["curl"] if lod == 0 else 0,
                   droop=lf["droop"], twist=lf["twist"], flutter=0.1)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    meshes = []
    for spec in (MEDIUM, LARGE):
        desc = describe(spec)
        leaves = sum(len(c["leaves"]) for c in desc["canes"])
        full = sum(1 for c in desc["canes"] for l in c["leaves"] if l["full"])
        print(f"HOMESTEAD_PLANT {spec['name']} canes={len(desc['canes'])} leaves={leaves} full={full}")
        batches = [emit(desc, atlas, lod) for lod in range(3)]
        objs = F.finish_lods(kit, batches, spec["name"], material)
        print("HOMESTEAD_LODS", F.lod_report(objs))
        meshes.extend(objs)
    return meshes
