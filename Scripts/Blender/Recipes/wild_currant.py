"""Wild red currant (Ribes rubrum): a tidy, rounded, upright, thornless fruiting shrub.

    .\\Scripts\\Blender\\New-Prop.ps1 Scripts\\Blender\\Recipes\\wild_currant.py

The real plant (what the geometry and textures follow):
- A hedgerow and damp-woodland shrub of western Europe (Cornish lanes and old kitchen gardens),
  0.8-1.5 m tall and about as wide: a compact vase of 6-10 smooth upright stems from one crown
  with short side spurs, the tips arching slightly under their leaves. No prickles at all.
- Bark smooth and pale grey-brown, older stems shedding papery strips that show darker
  red-brown beneath; young shoots straw-green.
- Leaves alternate on 3-6 cm petioles, 4-7 cm across, palmately 3-5 lobed with broad shallow
  rounded lobes, a heart-shaped base and blunt rounded (crenate) teeth; soft fresh mid-green,
  paler and brighter at the shoot tips, the lowest ones yellowing.
- Fruit hangs in "strigs" (racemes) of 6-15 glossy translucent berries that ripen from green
  through pink to bright red. The ripe red strigs are a separate mesh (SM_CurrantProduce) so the
  game can hide them as she picks; this bush carries only a few green, pink-flushed unripe strigs.

Game notes: walk-through (no collision). One 4K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G per-stem phase, B leaf flutter).
Designed to read nothing like SM_BlackberryBramble: a round compact upright bush of bright
lobed leaves on smooth pale stems, against the bramble's dark thorny arching cane tangle.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "WildCurrant"
DESCRIPTION = ("Wild red currant bush, 0.95 m tall and 1.1 m across: smooth pale upright thornless stems, "
               "small fresh-green lobed leaves and a few green unripe strigs (ripe fruit is "
               "SM_CurrantProduce). Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 25000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.24, -0.40, 0.60)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on stems",
             "A": "1"},
    "material_notes": ("One material M_WildCurrant: T_WildCurrant_basecolor (sRGB, alpha = opacity mask, clip "
                       "0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 5531
LEAF_TILES = ("mid", "mid2", "light", "young", "dark", "yellow", "spots")
SPRAYS = ("spray_a", "spray_b")
CENTRE = 0.36        # palmate leaf: vein origin in tile units above the petiole end
LEAF_SIZE = 0.072    # m, card length (petiole stub to lobe tip); the blade itself is ~5.5 cm across
BERRY_R = 0.0042     # m, unripe currant radius (real 8-10 mm diameter berries, still small and green)

# (angle from +Y in radians, reach in tile units): terminal lobe, two laterals, two small basal lobes.
LOBES = [(0.0, 0.54), (-1.12, 0.47), (1.12, 0.47), (-2.15, 0.31), (2.15, 0.31)]


def _palettes():
    base = dict(yellow=(0.25, 0.22, 0.05), brown=(0.09, 0.05, 0.02), stalk=(0.16, 0.12, 0.06))
    return {
        "mid": dict(base, base=(0.098, 0.178, 0.035), tip=(0.108, 0.186, 0.037), vein=(0.15, 0.21, 0.07),
                    margin=(0.078, 0.140, 0.028)),
        "mid2": dict(base, base=(0.086, 0.160, 0.031), tip=(0.096, 0.168, 0.033), vein=(0.135, 0.19, 0.06),
                     margin=(0.068, 0.124, 0.025)),
        "light": dict(base, base=(0.115, 0.195, 0.042), tip=(0.13, 0.205, 0.046), vein=(0.18, 0.24, 0.09),
                      margin=(0.10, 0.175, 0.038)),
        "young": dict(base, base=(0.150, 0.230, 0.050), tip=(0.17, 0.235, 0.055), vein=(0.21, 0.27, 0.10),
                      margin=(0.15, 0.17, 0.05)),
        "dark": dict(base, base=(0.070, 0.132, 0.026), tip=(0.076, 0.138, 0.028), vein=(0.11, 0.16, 0.05),
                     margin=(0.055, 0.104, 0.022)),
        "yellow": dict(base, base=(0.17, 0.18, 0.04), tip=(0.24, 0.21, 0.05), vein=(0.22, 0.21, 0.08),
                       margin=(0.17, 0.13, 0.04)),
        "spots": dict(base, base=(0.084, 0.150, 0.031), tip=(0.092, 0.156, 0.033), vein=(0.14, 0.19, 0.065),
                      margin=(0.09, 0.11, 0.03), brown=(0.085, 0.062, 0.026)),
    }


PARAMS = {
    "mid": dict(hair=0.05, gloss=0.05),
    "mid2": dict(hair=0.05, gloss=0.05, damage=0.03),
    "light": dict(hair=0.06, gloss=0.0, trans=0.72),
    "young": dict(hair=0.1, gloss=0.0, trans=0.78),
    "dark": dict(hair=0.04, gloss=0.1),
    "yellow": dict(hair=0.08, yellow=0.65, damage=0.5, edge_burn=0.3),
    "spots": dict(hair=0.08, damage=0.5, holes=0.25, edge_burn=0.12),
}


def _crenate(phi, count, depth, phase):
    """Blunt rounded teeth with narrow notches between them, around the whole margin."""
    ph = (phi / math.tau + 0.5) * count + phase
    return (np.power(np.abs(np.sin(math.pi * ph)), 0.55) - 0.6) * depth


def _palmate(rng):
    phase = rng.uniform(0, 1)
    jit = [rng.uniform(0.93, 1.05) for _ in LOBES]
    count = rng.choice((34, 36, 38))

    def shape(X, Y):
        dx, dy = X, Y - CENTRE
        r = np.hypot(dx, dy)
        phi = np.arctan2(dx, dy)          # 0 toward the tip, +/-pi toward the petiole
        reach = np.full(X.shape, 0.33)
        for (a, rr), j in zip(LOBES, jit):
            diff = np.angle(np.exp(1j * (phi - a)))
            # Broad, blunt lobes: wide gaussian, flattened tip, so the sinuses stay shallow.
            reach = np.maximum(reach, rr * j * np.exp(-(diff / 0.62) ** 2 * 1.2) ** 0.24 *
                               (1 - 0.18 * np.abs(diff)))
        notch = np.exp(-((np.abs(phi) - math.pi) / 0.30) ** 2) * 0.12
        reach = reach - notch
        reach = reach * (1 + _crenate(phi, count, 0.06, phase))
        inside = reach - r
        return inside, np.clip(r / 0.5, 0, 1), reach

    return shape


def _veins(nrng):
    veins = []
    for a, rr in LOBES:
        reach = rr * 0.88
        pts = [(math.sin(a) * reach * t, CENTRE + math.cos(a) * reach * t) for t in np.linspace(0, 1, 10)]
        veins.append((pts, 1.0 if a == 0 else (0.85 if abs(a) < 2 else 0.6)))
        for k in range(3 if abs(a) < 2 else 1):
            t0 = 0.28 + 0.2 * k + nrng.uniform(-0.03, 0.03)
            px_, py_ = math.sin(a) * reach * t0, CENTRE + math.cos(a) * reach * t0
            for side in (-1, 1):
                b = a + side * 0.85
                ln = rr * 0.30 * (1 - 0.45 * t0)
                veins.append(([(px_, py_), (px_ + math.sin(b) * ln, py_ + math.cos(b) * ln)], 0.38))
    veins.append(([(0.0, 0.0), (0.0, CENTRE)], 1.0))
    return veins


def _blade(X, Y, px, nrng, rng, key, pals, vein_scale=1.0):
    return F.paint_blade(X, Y, nrng, _palmate(rng), _veins(nrng), pals[key], px, vein_width=0.0075 * vein_scale,
                         vein_depth=0.00022, puff=0.00016, tertiary=0.4, stalk=0.014, **PARAMS[key])


def _paint_berry(atlas, nrng):
    """Unripe currant skin: translucent green with faint meridian veins and a pink sun flush;
    v runs from the stalk pole (0) to the dried calyx at the free pole (1)."""
    X, Y, px = atlas.grid("berry")
    layer = F.Layer(X.shape)
    u = (X - X.min()) / max(X.max() - X.min(), 1e-6)
    n = F.noise(X.shape, nrng, freq=6.0, beta=2.4)
    green = np.asarray((0.20, 0.30, 0.06))
    blush = np.asarray((0.42, 0.16, 0.07))
    flush = F.smoothstep(0.45, 0.85, n) * F.smoothstep(0.2, 0.6, np.sin(math.pi * u) ** 2)
    color = F.lerp(green, blush, flush * 0.8)
    stripes = F.smoothstep(0.86, 0.98, np.abs(np.sin(u * math.pi * 8)))
    color = F.lerp(color, color * 1.25, stripes * 0.5)
    calyx = F.smoothstep(0.90, 0.95, Y)
    color = F.lerp(color, (0.05, 0.035, 0.02), calyx)
    layer.color = color
    layer.alpha[...] = 1
    layer.height = 0.00005 * stripes - 0.0002 * calyx
    layer.rough = 0.25 + 0.4 * calyx
    layer.trans[...] = 0.55
    atlas.put("berry", layer, meters_per_px=0.012 / X.shape[0], opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("cane", 128)
    atlas.column("twig", 96)
    atlas.column("petiole", 64)
    for key in LEAF_TILES:
        atlas.tile(key, 680, 680)
    for key in SPRAYS:
        atlas.tile(key, 1100, 1400)
    atlas.tile("berry", 128, 128)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAF_TILES:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade(X, Y, px, nrng, rng, key, pals), meters_per_px=LEAF_SIZE / X.shape[0])
    mixes = {"spray_a": ["mid", "light", "mid2", "mid", "young", "dark"],
             "spray_b": ["mid2", "mid", "dark", "light", "mid", "spots"]}
    for key in SPRAYS:
        layer = S.paint_spray(atlas, key, nrng, rng,
                              lambda Xl, Yl, pxl, i, k: _blade(Xl, Yl, pxl, nrng, rng, k, pals, 1.3),
                              7, leaf_len=0.30, angle=(45, 70), twig_color=(0.17, 0.14, 0.09), twig_width=0.007,
                              twig_top=0.74, keys=mixes[key], squash=(0.8, 1.0), taper=0.35)
        atlas.put(key, layer, meters_per_px=0.24 / layer.color.shape[0])
    _paint_berry(atlas, nrng)
    # Pale grey-brown stems shedding papery strips over darker red-brown underbark.
    S.paint_bark(atlas, "cane", nrng, (0.20, 0.165, 0.125), (0.105, 0.072, 0.050), blotch=0.75, stri=0.55,
                 aniso=22.0, cracks=0.8, crack_color=(0.07, 0.045, 0.030), lenticels=0.35,
                 lent_color=(0.26, 0.23, 0.19), rough=0.6, v_len=0.5)
    S.paint_bark(atlas, "twig", nrng, (0.17, 0.15, 0.10), (0.13, 0.13, 0.07), blotch=0.5, stri=0.25,
                 lenticels=0.3, lent_color=(0.24, 0.22, 0.17), rough=0.5, v_len=0.5, relief=0.00012)
    S.paint_bark(atlas, "petiole", nrng, (0.14, 0.17, 0.06), (0.20, 0.12, 0.07), blotch=0.45, stri=0.2,
                 rough=0.5, v_len=0.5, relief=0.00008)
    atlas.save()
    return atlas


SPEC = dict(
    height=0.86, rx=0.53, ry=0.50,
    profile=[(0.0, 0.32), (0.22, 0.70), (0.5, 0.96), (0.75, 1.0), (0.9, 0.80), (1.0, 0.38)],
    crowns=[(0.0, 0.0), (0.05, 0.03), (-0.04, 0.04), (0.02, -0.05)], crown_jitter=0.035,
    stem_radius=0.0095, step=0.025, bark="cane", clumps=False, lobes=0.08,
    levels=[
        dict(count=9, elev=(64, 86), length=(0.85, 1.12), tropism=0.30, outward=0.30, wander=0.35, stop=1.0,
             gravity=0.14, bark="cane", tip_radius=0.3),
        dict(per_m=5.0, start=0.28, end=0.95, angle=(28, 52), length=(0.30, 0.55), radius=(0.45, 0.6), up=0.3,
             out=0.35, tropism=0.2, outward=0.35, wander=0.6, stop=1.0, max=6, gravity=0.12, bark="twig",
             tip_radius=0.4),
        dict(per_m=8.0, start=0.2, angle=(35, 60), abs_length=(0.05, 0.13), radius=(0.5, 0.7), up=0.25,
             out=0.3, tropism=0.2, outward=0.3, wander=1.0, stop=1.05, inner=0.55, inner_skip=0.7, max=5,
             step=0.015, bark="twig", tip_radius=0.5),
    ],
    shell=dict(count=230, depth=(0.74, 1.0), max_len=0.28, z_min=0.15, radius=0.0022),
    leaf=dict(level=1, zone=0.19, spacing=(0.022, 0.036), angle=(48, 72), size=LEAF_SIZE, petiole=0.55,
              arrangement="alternate", face_up=1.1, face_out=0.45, hang=0.05, inner=0.58, inner_density=0.5,
              tip_min=0.55, tip_len=0.06, fold=(0.05, 0.22), droop=(0.05, 0.25), curl=(0.0, 0.1), jitter=0.3,
              petiole_tube="petiole", petiole_radius=0.0009),
    full=0.52, lod_levels=(9, 9, 1), lod_step=(0.05, 0.1, 0.2),
    sides=[(5, 4, 3), (4, 3, 3), (3, 3, 3)], leaf_grid=((2, 2), (1, 1), (1, 1)), spray_grid=((2, 2), (1, 1), (1, 1)),
    spray_cross=True, spray_overhang=0.8, v_bark=0.5, seed=SEED, min_radius=0.0010,
)


def _strigs(desc, krng):
    """A few short unripe strigs hanging from leaf nodes on the outer shell."""
    out = []
    cands = [(tw, lf) for tw in desc["twigs"] if tw["outer"] > 0.62
             for lf in tw["leaves"] if 0.25 < lf["node"].z < 0.8]
    krng.shuffle(cands)
    for tw, lf in cands[:9]:
        node = lf["node"]
        o = Vector((node.x, node.y, 0.0))
        o = o.normalized() if o.length > 1e-3 else Vector((1, 0, 0))
        length = krng.uniform(0.035, 0.06)
        bend = o * krng.uniform(0.010, 0.018)
        rachis = [node + bend * (t ** 0.6) - Vector((0, 0, length * t)) for t in np.linspace(0, 1, 4)]
        berries = []
        count = krng.randint(5, 8)
        for k in range(count):
            t = 0.25 + 0.75 * k / max(count - 1, 1)
            p = rachis[0].lerp(rachis[-1], t) + bend * 0.3 * t
            az = k * 2.4 + krng.uniform(-0.4, 0.4)
            side = Vector((math.cos(az), math.sin(az), 0.0))
            r = BERRY_R * krng.uniform(0.75, 1.05) * (1.0 - 0.25 * t)
            d = (side * 0.8 - Vector((0, 0, 0.6))).normalized()
            stalk = p + d * (r + 0.004)
            berries.append(dict(base=p, stalk=stalk, center=stalk + d * r * 0.9, r=r, dir=d))
        out.append(dict(rachis=rachis, berries=berries, phase=tw["phase"]))
    return out


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    rng = random.Random(SEED)
    desc = S.grow(SPEC, rng)
    krng = random.Random(SEED + 5)
    for tw in desc["twigs"]:
        for lf in tw["leaves"]:
            if lf["node"].z < 0.28:
                lf["key"] = krng.choices(["mid", "mid2", "yellow", "spots", "dark"], [25, 25, 22, 13, 15])[0]
            elif lf["from_tip"] < 0.035:
                lf["key"] = krng.choices(["young", "light"], [60, 40])[0]
            elif lf["outer"] < 0.58:
                lf["key"] = krng.choices(["mid2", "dark", "mid", "yellow"], [35, 35, 25, 5])[0]
            else:
                lf["key"] = krng.choices(["mid", "mid2", "light", "dark", "spots"], [38, 28, 16, 12, 6])[0]
        tw["spray_key"] = SPRAYS[tw["spray"] % len(SPRAYS)]
    strigs = _strigs(desc, krng)
    print(f"HOMESTEAD_PLANT branches={len(desc['branches'])} twigs={len(desc['twigs'])} "
          f"leaves={sum(len(t['leaves']) for t in desc['twigs'])} full={sum(t['full'] for t in desc['twigs'])} "
          f"strigs={len(strigs)}")
    batches = []
    for lod in range(3):
        b = S.emit(desc, atlas, lod, SPEC, lambda lf, tw: lf["key"], lambda tw: tw["spray_key"])
        if lod < 2:
            for sg in strigs:
                b.tube(sg["rachis"] if lod == 0 else [sg["rachis"][0], sg["rachis"][-1]],
                       [0.0008, 0.0007, 0.0006, 0.0005][:4 if lod == 0 else 2], 3, atlas.uv("petiole"),
                       v_length=0.5, phase=sg["phase"], flutter=0.3)
                for be in sg["berries"]:
                    if lod == 0:
                        b.tube([be["base"], be["stalk"]], [0.0004, 0.0003], 3, atlas.uv("petiole"), v_length=0.5,
                               phase=sg["phase"], flutter=0.4)
                    b.sphere(be["center"], be["r"], atlas.uv("berry", inset=False), segs=(7, 5)[lod],
                             rings=(5, 3)[lod], stretch=1.08, axis=be["dir"], phase=sg["phase"])
        batches.append(b)
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
