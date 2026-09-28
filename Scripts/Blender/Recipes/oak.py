"""Pedunculate oak (Quercus robur): a mature Cornish woodland oak.

The real tree (what the geometry and textures follow):
- The commonest tree of Cornish valley woods. Mature woodland/parkland oaks stand 14-18 m with a
  crown about as wide as tall: a short, stout bole (3-5 m, 0.9-1.2 m diameter) that splits into
  a few heavy, sinuous limbs spreading wide and low, then crooked, zig-zag branches and twigs.
  The crown is broad and domed, lumpy and irregular (lost limbs, wind shaping).
- Bark grey-brown, rugged: deep vertical fissures between long ridges broken into blocks; in the
  mild wet Cornish air it carries pale lichen crusts on the ridges and moss in the fissures,
  thickest near the base.
- Leaves 7-12 cm, obovate with 4-5 pairs of rounded lobes, small "ear" lobes (auricles) at the
  base and almost no stalk; crowded in rosettes at the shoot tips. Upper face dark to mid green
  and slightly glossy, underside paler and bluish; by midsummer many are nibbled, galled or
  spotted.

Game notes: trunk-only capsule collision; four LODs (see homestead_tree.py) with two material
slots (0 bark, 1 leaves); wind in the Wind vertex colour (R height, G branch phase, B flutter).
"""
import math
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "Oak"
DESCRIPTION = ("Mature pedunculate oak, about 16 m tall with a broad 17 m domed crown on a short "
               "fissured bole; painted leaf-cluster canopy, four LODs, trunk capsule.")
COLLISION = "capsule"
TRIANGLE_BUDGET = 90000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 4101
BEAUTY = {"pose": (0, 0, 0), "focus": (1.5, -3.0, 5.0), "views": ("hero", "detail", "eye"),
          "eye_distance": 16.0, "detail_distance": 6.0, "detail_fstop": 11.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "dark", "dark", "light", "under", "yellow", "holes")

SPEC = dict(
    height=16.0, rx=8.6, ry=8.0, seed=SEED, crown_center=0.62, crown_base=0.22,
    profile=[(0.0, 0.08), (0.17, 0.14), (0.24, 0.62), (0.36, 0.93), (0.52, 1.0), (0.70, 0.9),
             (0.86, 0.62), (1.0, 0.18)],
    lobes=0.16, stem_radius=0.56, step=0.3, min_radius=0.004, bark="bark",
    flare=dict(amount=0.5, height=0.6, lobe_height=1.0, rough=0.035,
               lobes=[(5, 0.14, 0.4), (3, 0.07, 1.3), (7, 0.045, 2.1)]),
    skirt=0.3, bark_size=0.8, capsule_height=3.0,
    levels=[
        dict(count=1, elev=(86, 89), length=(0.62, 0.68), tropism=0.9, outward=0.0, wander=0.3,
             stop=0.97, tip_radius=0.12, step=0.3),
        dict(per_m=1.35, start=0.3, end=0.9, angle=(34, 58), abs_length=(7.0, 10.5), radius=(0.62, 0.8),
             up=0.25, out=0.45, tropism=0.3, outward=0.16, wander=0.5, stop=1.0, gravity=0.012,
             max=8, step=0.25, tip_radius=0.1),
        dict(per_m=1.5, start=0.12, end=0.97, angle=(30, 60), abs_length=(1.8, 4.2), radius=(0.42, 0.58),
             up=0.3, out=0.4, tropism=0.25, outward=0.3, wander=0.9, stop=1.02, max=12, step=0.15,
             tip_radius=0.2),
        dict(per_m=2.8, start=0.15, end=0.98, angle=(35, 70), abs_length=(0.5, 1.2), radius=(0.45, 0.62),
             up=0.2, out=0.3, tropism=0.1, outward=0.3, wander=1.1, stop=1.05, max=10, step=0.1,
             inner=0.5, inner_skip=0.6, tip_radius=0.35),
    ],
    shell=dict(count=520, depth=(0.72, 1.0), max_len=1.6, z_min=0.22, radius=0.012, max_level=2),
    foliage=dict(level=3, zone=1.0, spacing=(0.22, 0.34), size=0.85, inner=0.62, inner_density=0.3,
                 on_parents=0.45, along=0.55, face_out=0.7, face_up=0.45, droop=0.12, min_z=0.2),
    normal_blend=0.6,
    lods=[
        dict(levels=9, sides=[24, 12, 5, 3], trunk_sides=28, step=0.2, grid=(2, 2), fold=0.18,
             droop=0.14, back=0.08),
        dict(levels=2, sides=[14, 8, 4], trunk_sides=14, step=0.5, cell=1.25, card_scale=1.5,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=2, sides=[8, 4, 3], trunk_sides=8, step=1.0, min_radius=0.04, cell=2.4,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[6, 3], trunk_sides=6, step=2.0, min_radius=0.11, cell=4.0,
             card_scale=1.45, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.05), brown=(0.075, 0.04, 0.016), stalk=(0.07, 0.06, 0.03),
                dry=(0.15, 0.09, 0.04))
    return {
        "mid": dict(base, base=(0.040, 0.068, 0.018), tip=(0.046, 0.074, 0.020), vein=(0.075, 0.100, 0.035),
                    margin=(0.036, 0.058, 0.016)),
        "dark": dict(base, base=(0.028, 0.050, 0.014), tip=(0.032, 0.055, 0.015), vein=(0.058, 0.082, 0.030),
                     margin=(0.026, 0.044, 0.012)),
        "light": dict(base, base=(0.070, 0.105, 0.028), tip=(0.080, 0.112, 0.030), vein=(0.10, 0.13, 0.045),
                      margin=(0.062, 0.092, 0.025)),
        "under": dict(base, base=(0.058, 0.078, 0.040), tip=(0.062, 0.082, 0.042), vein=(0.085, 0.10, 0.06),
                      margin=(0.052, 0.07, 0.036)),
        "yellow": dict(base, base=(0.070, 0.090, 0.024), tip=(0.13, 0.13, 0.035), vein=(0.13, 0.13, 0.05),
                       margin=(0.10, 0.09, 0.03)),
        "holes": dict(base, base=(0.044, 0.070, 0.019), tip=(0.050, 0.075, 0.021), vein=(0.078, 0.10, 0.036),
                      margin=(0.050, 0.058, 0.02)),
    }


PARAMS = {
    "mid": dict(gloss=0.25, damage=0.08),
    "dark": dict(gloss=0.35),
    "light": dict(gloss=0.1, trans=0.7),
    "under": dict(gloss=-0.3, hair=0.25, trans=0.5),
    "yellow": dict(yellow=0.45, damage=0.6, edge_burn=0.3),
    "holes": dict(holes=1.0, damage=0.35, edge_burn=0.12),
}


def oak_shape(rng):
    """Obovate blade with 4-5 pairs of rounded lobes and basal auricles."""
    core = F.ovate(width=0.13, base=0.04, tip=1.0, widest=0.66, tip_sharp=0.6, base_round=0.9)
    parts = [core]
    n = rng.choice((4, 4, 5))
    lobes = []
    for k in range(n):
        s = 0.17 + 0.62 * k / (n - 1) + rng.uniform(-0.025, 0.025)
        L = 0.10 + 0.11 * math.sin(math.pi * (k + 0.7) / (n + 0.5))
        for side in (-1, 1):
            ang = side * math.radians(60 - 8 * k + rng.uniform(-7, 7))
            lobe = F.ovate(width=0.52, base=0.0, tip=1.0, widest=0.6, tip_sharp=0.5, base_round=0.7)
            ln = L * rng.uniform(0.85, 1.12)
            parts.append(T.placed(lobe, (0.0, s + (0.035 if side > 0 else 0.0)), ang, ln))
            lobes.append((s, ang, ln))
    for side in (-1, 1):
        ear = F.ovate(width=0.45, base=0.0, tip=1.0, widest=0.5, tip_sharp=0.6, base_round=0.7)
        parts.append(T.placed(ear, (0.0, 0.075), side * math.radians(118), 0.06))
    veins = [([(0.0, 0.04 + 0.95 * t) for t in np.linspace(0, 1, 12)], 1.0)]
    for s, ang, ln in lobes:
        veins.append(([(0.0, s), (math.sin(ang) * ln * 0.85, s + math.cos(ang) * ln * 0.85)], 0.5))
    return T.union(*parts), veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [oak_shape(random.Random(SEED + 7 * i)) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.009,
                             vein_depth=0.0003, puff=0.0003, tertiary=0.45, stalk=0.012, **PARAMS[key])
    return blade


def paint_leaves():
    atlas = F.Atlas(NAME + "Leaves", size=2048, seed=SEED, folder=F.props_folder(NAME) / "Textures")
    for key in CLUSTERS + MASS:
        atlas.tile(key, 1024, 1024)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    blade = make_blade(nrng, rng)
    for i, key in enumerate(CLUSTERS):
        lrng = random.Random(SEED + 31 * i)
        layout = T.cluster_layout(lrng, pattern="tips", side_twigs=8, side_len=(0.22, 0.42),
                                  side_angle=(28, 65), leaf_len=0.26, tip_fan=(6, 9), twig_width=0.011,
                                  curve=0.18, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.06, 0.05, 0.036), meters=0.85)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=950, leaf_len=0.045,
                 keys=LEAF_KEYS, core=(0.012, 0.021, 0.007), holes=0.5, meters=4.0)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=2048, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 2048)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"], style="fissured",
                 base=(0.070, 0.058, 0.044), ridge=(0.118, 0.104, 0.084), fissure=(0.014, 0.011, 0.008),
                 depth=0.032, ridges=7.0, breaks=0.75, lichen=0.28, lichen_color=(0.19, 0.20, 0.15),
                 moss=0.35, moss_color=(0.04, 0.065, 0.016), algae=0.3, rough=0.86)
    atlas.save()
    return atlas


def build(kit):
    bark = paint_bark()
    leaves = paint_leaves()
    objs, desc, cap = T.build(kit, SPEC, bark, leaves, CLUSTERS, MASS, "SM_" + NAME)
    REPORT["capsule"] = T.capsule_report(cap)
    return objs


REPORT = {
    "blocking": True,
    "unreal_folder": "Trees",
    "nanite": True,
    "tree": {"height_m": SPEC["height"], "crown_m": [SPEC["rx"] * 2, SPEC["ry"] * 2],
             "ground_line_cm": SPEC["skirt"] * 100,
             "note": "Pivot is the bottom of a 30 cm skirt under the trunk flare; the ground line is 30 cm up."},
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / tree height",
             "G": "per-branch random phase", "B": "leaf flutter 0 at card base -> 1 at tip, 0 on wood", "A": "1",
             "strength": 14.0, "flutter": 0.9},
    "material_notes": ("Slot 0 M_OakBark: tiling bark T_OakBark_* (0.8 m square). Slot 1 M_OakLeaves: leaf-cluster "
                       "atlas T_OakLeaves_* (basecolor alpha = mask, clip 0.5; roughness R rough, G translucency, "
                       "B AO; normal OpenGL). Two-sided foliage, masked."),
}
