"""Deer brush (Ceanothus integerrimus): a small, loose, arching foothill shrub, about waist high.

The real plant (what the geometry and textures follow):
- A deciduous-to-semi-evergreen ceanothus of Sierra Nevada foothill and lower montane woods and
  burns around Yosemite, 1-3 m; young plants are loose mounds of slender arching stems.
- Stems and twigs smooth, yellow-green to olive with a waxy sheen, becoming grey-brown and
  finely lenticelled on old wood; no thorns.
- Leaves alternate, 2.5-7 cm, ovate to elliptic, margin entire, thin, light to mid green, with
  three prominent veins from the base (the ceanothus hallmark), paler beneath.
- Flowers in fluffy panicles 5-15 cm long at the branch ends, white to pale blue; by summer many
  have faded to cream and started to set small three-lobed capsules.

Game notes: walk-through (no collision). One 4K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G branch phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "DeerBrush"
DESCRIPTION = ("Young deer brush (Ceanothus integerrimus): loose arching mound 1.1 m tall and 1.5 m across with "
               "three-veined leaves and fading white-blue flower plumes. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 25000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.3, 0.65)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on stems",
             "A": "1"},
    "material_notes": ("One material M_DeerBrush: T_DeerBrush_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency mask, "
                       "B AO). Two-sided foliage, masked."),
}

SEED = 6401
LEAF_TILES = ("mid", "mid2", "light", "dark", "yellow", "holes", "under")
SPRAYS = ("spray_a", "spray_b", "spray_c")


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.04), brown=(0.07, 0.035, 0.014), stalk=(0.10, 0.10, 0.04))
    return {
        "mid": dict(base, base=(0.060, 0.094, 0.024), tip=(0.066, 0.098, 0.026), vein=(0.10, 0.13, 0.045),
                    margin=(0.050, 0.078, 0.020)),
        "mid2": dict(base, base=(0.052, 0.086, 0.022), tip=(0.058, 0.090, 0.024), vein=(0.092, 0.12, 0.042),
                     margin=(0.044, 0.070, 0.019)),
        "light": dict(base, base=(0.082, 0.120, 0.032), tip=(0.092, 0.126, 0.034), vein=(0.12, 0.15, 0.06),
                      margin=(0.072, 0.105, 0.028)),
        "dark": dict(base, base=(0.042, 0.072, 0.019), tip=(0.046, 0.076, 0.020), vein=(0.08, 0.105, 0.036),
                     margin=(0.036, 0.060, 0.016)),
        "yellow": dict(base, base=(0.11, 0.12, 0.03), tip=(0.17, 0.15, 0.04), vein=(0.17, 0.16, 0.06),
                       margin=(0.12, 0.10, 0.03)),
        "holes": dict(base, base=(0.058, 0.090, 0.024), tip=(0.064, 0.094, 0.025), vein=(0.10, 0.12, 0.045),
                      margin=(0.06, 0.07, 0.022)),
        "under": dict(base, base=(0.092, 0.112, 0.062), tip=(0.095, 0.115, 0.064), vein=(0.12, 0.13, 0.08),
                      margin=(0.085, 0.10, 0.055)),
    }


PARAMS = {
    "mid": dict(gloss=0.2),
    "mid2": dict(gloss=0.15, damage=0.03),
    "light": dict(gloss=0.1, trans=0.72),
    "dark": dict(gloss=0.25),
    "yellow": dict(yellow=0.5, damage=0.6, edge_burn=0.3),
    "holes": dict(holes=0.9, damage=0.3, edge_burn=0.15),
    "under": dict(gloss=-0.3, trans=0.5, hair=0.3),
}


def _shape(rng):
    return F.ovate(width=0.25, widest=0.42, tip_sharp=0.9, base_round=0.7, phase=rng.uniform(0, 1))


def _veins(nrng):
    """Midrib plus the two strong basal veins that arc up toward the tip, with faint cross veins."""
    veins = [([(0.0, 0.03 + 0.95 * t) for t in np.linspace(0, 1, 12)], 1.0)]
    for side in (-1, 1):
        pts = [(side * 0.20 * math.sin(math.pi * 0.95 * t) ** 0.8 * (1 - 0.3 * t), 0.05 + 0.80 * t)
               for t in np.linspace(0, 1, 14)]
        veins.append((pts, 0.9))
        for k in range(4):
            y0 = 0.25 + 0.15 * k + nrng.uniform(-0.02, 0.02)
            x1 = side * 0.18 * math.sin(math.pi * min((y0 - 0.05) / 0.8, 1.0) * 0.95)
            veins.append(([(side * 0.006, y0), (x1 * 0.6, y0 + 0.07), (x1 * 1.05, y0 + 0.12)], 0.35))
    return veins


def _blade(X, Y, px, nrng, rng, key, pals, vein_scale=1.0):
    under = key == "under"
    return F.paint_blade(X, Y, nrng, _shape(rng), _veins(nrng), pals[key], px,
                         vein_width=0.0085 * vein_scale, vein_depth=-0.0002 if under else 0.00030,
                         puff=0.00022, tertiary=0.3, stalk=0.014, **PARAMS[key])


def _paint_plume(atlas, key, nrng, rng, faded):
    """Fluffy panicle of tiny five-parted flowers on a branched stalk, v from stalk base to tip."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    layer.color[...] = (0.30, 0.30, 0.26)
    axis = [(0.01 * math.sin(3 * t), 0.02 + 0.95 * t) for t in np.linspace(0, 1, 10)]
    d, along = F.polyline_distance(X, Y, axis)
    stalk = F.Layer(X.shape)
    stalk.color[...] = (0.12, 0.12, 0.05)
    stalk.alpha = np.clip(-(d - 0.012 * (1 - 0.6 * along)) / px + 0.5, 0, 1)
    layer.over(stalk)
    a = X.shape[1] * px * 0.5
    blossoms = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=10.0, beta=2.2)
    tone = (0.54, 0.54, 0.49) if not faded else (0.44, 0.41, 0.32)
    blue = (0.46, 0.50, 0.62)
    cov = np.zeros(X.shape)
    col = np.zeros(X.shape + (3,))
    xs, ys = X[0], Y[:, 0]
    for i in range(2600):
        t = rng.random() ** 0.8
        y = 0.08 + 0.9 * t
        half = 0.36 * math.sin(math.pi * min(t * 1.1, 1.0)) ** 0.7 * (1 - 0.35 * t) + 0.04
        x = rng.uniform(-half, half) * a / 0.5 * math.sqrt(rng.random())
        r = rng.uniform(0.006, 0.011)
        i0, i1 = np.searchsorted(ys, y - 2 * r), np.searchsorted(ys, y + 2 * r)
        j0, j1 = np.searchsorted(xs, x - 2 * r), np.searchsorted(xs, x + 2 * r)
        win = (slice(i0, i1), slice(j0, j1))
        m = np.zeros(X.shape)
        # Five tiny petals plus a stamen tuft: a soft star rather than a dot.
        dx, dy = X[win] - x, Y[win] - y
        ang = np.arctan2(dy, dx) + rng.uniform(0, 6.28)
        star = r * (0.75 + 0.25 * np.cos(5 * ang))
        m[win] = np.clip(1.3 - np.hypot(dx, dy) / star, 0, 1)
        if not m.any():
            continue
        c = np.asarray(tone) * rng.uniform(0.8, 1.1)
        if rng.random() < 0.35 and not faded:
            c = np.asarray(blue) * rng.uniform(0.85, 1.1)
        if faded and rng.random() < 0.25:
            c = np.asarray((0.16, 0.15, 0.07))
        upd = m > cov
        col[upd] = c
        cov = np.maximum(cov, m)
    blossoms.color = col * (0.8 + 0.3 * n)[..., None]
    blossoms.alpha = F.smoothstep(0.15, 0.4, cov)
    blossoms.height = 0.0006 * cov
    blossoms.trans[...] = 0.6
    blossoms.rough[...] = 0.7
    blossoms.ao = 0.6 + 0.4 * cov
    layer.over(blossoms)
    atlas.put(key, layer, meters_per_px=0.14 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("bark", 128)
    atlas.column("twig", 96)
    for key in LEAF_TILES:
        atlas.tile(key, 560, 1100)
    for key in SPRAYS:
        atlas.tile(key, 1100, 1400)
    atlas.tile("plume", 600, 900)
    atlas.tile("plume_faded", 600, 900)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAF_TILES:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade(X, Y, px, nrng, rng, key, pals), meters_per_px=0.05 / X.shape[0])
    mixes = {"spray_a": ["mid", "mid2", "dark", "mid", "light"], "spray_b": ["mid2", "mid", "holes", "dark", "mid"],
             "spray_c": ["mid", "yellow", "mid2", "dark", "mid", "light"]}
    for key in SPRAYS:
        layer = S.paint_spray(atlas, key, nrng, rng, lambda Xl, Yl, pxl, i, k: _blade(Xl, Yl, pxl, nrng, rng, k, pals, 1.4),
                              9, leaf_len=0.26, angle=(35, 60), twig_color=(0.10, 0.10, 0.04), twig_width=0.008,
                              twig_top=0.78, keys=mixes[key], squash=(0.6, 1.0), taper=0.4)
        atlas.put(key, layer, meters_per_px=0.22 / layer.color.shape[0])
    _paint_plume(atlas, "plume", nrng, rng, False)
    _paint_plume(atlas, "plume_faded", nrng, rng, True)
    S.paint_bark(atlas, "bark", nrng, (0.085, 0.075, 0.055), (0.055, 0.060, 0.035), blotch=0.6, stri=0.2,
                 lenticels=0.6, lent_color=(0.17, 0.16, 0.12), lichen=0.2, rough=0.55, v_len=0.5)
    S.paint_bark(atlas, "twig", nrng, (0.075, 0.095, 0.035), (0.10, 0.10, 0.045), blotch=0.5, stri=0.15,
                 rough=0.42, v_len=0.5, relief=0.0001)
    atlas.save()
    return atlas


SPEC = dict(
    height=1.08, rx=0.78, ry=0.72,
    profile=[(0.0, 0.35), (0.2, 0.7), (0.5, 0.97), (0.75, 1.0), (0.92, 0.75), (1.0, 0.35)],
    crowns=[(0.0, 0.0), (0.06, -0.05), (-0.05, 0.04)], crown_jitter=0.04,
    stem_radius=0.011, step=0.025, bark="bark", clumps=False, lobes=0.14,
    levels=[
        dict(count=13, elev=(48, 82), length=(1.0, 1.4), tropism=0.1, outward=0.5, wander=0.7, stop=0.95,
             gravity=0.35, bark="bark", tip_radius=0.25),
        dict(per_m=7.0, start=0.3, end=0.97, angle=(35, 60), length=(0.35, 0.65), radius=(0.45, 0.6),
             up=0.15, out=0.35, tropism=0.1, outward=0.4, wander=0.9, stop=1.0, max=9, bark="twig",
             gravity=0.3, tip_radius=0.35),
        dict(per_m=9.0, start=0.2, angle=(35, 60), abs_length=(0.06, 0.16), radius=(0.5, 0.7), up=0.2,
             out=0.3, tropism=0.2, outward=0.3, wander=1.1, stop=1.05, inner=0.55, inner_skip=0.7, max=7,
             step=0.015, bark="twig", tip_radius=0.5),
    ],
    shell=dict(count=240, depth=(0.72, 1.0), max_len=0.3, z_min=0.12, radius=0.0022),
    leaf=dict(level=1, zone=0.12, spacing=(0.016, 0.026), angle=(40, 65), size=0.052, petiole=0.2,
              arrangement="alternate", face_up=1.0, face_out=0.4, hang=0.1, inner=0.58, inner_density=0.35,
              tip_min=0.5, tip_len=0.05, fold=(0.1, 0.3), droop=(0.05, 0.3), curl=(0.0, 0.1)),
    full=0.3, lod_levels=(9, 9, 1), lod_step=(0.05, 0.1, 0.2),
    sides=[(5, 4, 3), (4, 3, 3), (3, 3, 3)], leaf_grid=((2, 2), (1, 1), (1, 1)), spray_grid=((2, 2), (1, 1), (1, 1)),
    spray_cross=True, spray_overhang=0.9, v_bark=0.5, seed=SEED, min_radius=0.0011,
)


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    rng = random.Random(SEED)
    desc = S.grow(SPEC, rng)
    krng = random.Random(SEED + 5)
    for tw in desc["twigs"]:
        for lf in tw["leaves"]:
            if lf["outer"] < 0.58:
                lf["key"] = krng.choices(["mid", "mid2", "yellow", "holes", "under"], [30, 30, 15, 10, 15])[0]
            elif lf["from_tip"] < 0.03:
                lf["key"] = "light"
            else:
                lf["key"] = krng.choices(["mid", "mid2", "dark", "holes", "yellow", "under"], [34, 28, 20, 6, 4, 8])[0]
        tw["spray_key"] = SPRAYS[tw["spray"] % 3]
    plumes = []
    cands = [tw for tw in desc["twigs"] if tw["outer"] > 0.72 and tw["end"].z > 0.4]
    krng.shuffle(cands)
    for tw in cands[:40]:
        d = (tw["dir"] * 0.6 + Vector((0, 0, 0.5))).normalized()
        plumes.append(dict(base=tw["end"] - tw["dir"] * 0.01, dir=d, up=tw["up"], phase=tw["phase"],
                           faded=krng.random() < 0.55, size=krng.uniform(0.08, 0.15), spin=krng.uniform(0, 3)))
    print(f"HOMESTEAD_PLANT branches={len(desc['branches'])} twigs={len(desc['twigs'])} "
          f"leaves={sum(len(t['leaves']) for t in desc['twigs'])} full={sum(t['full'] for t in desc['twigs'])}")
    batches = []
    for lod in range(3):
        b = S.emit(desc, atlas, lod, SPEC, lambda lf, tw: lf["key"], lambda tw: tw["spray_key"])
        for pl in plumes:
            key = "plume_faded" if pl["faded"] else "plume"
            w = pl["size"] * S.tile_aspect(atlas, key)
            from mathutils import Matrix
            for c in range((2, 2, 1)[lod]):
                up = Matrix.Rotation(pl["spin"] + c * math.pi / 2, 3, pl["dir"]) @ pl["up"]
                b.card(pl["base"], pl["dir"], up, pl["size"], w, atlas.uv(key), rows=(2, 1, 1)[lod], cols=1,
                       droop=0.15, phase=pl["phase"], flutter=0.6, flutter_base=0.2)
        batches.append(b)
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
