"""California hazelnut (Corylus cornuta var. californica): a tall open multi-stem shrub.

The real plant (what the geometry and textures follow):
- Deciduous understorey shrub of shady Sierra Nevada foothill and Yosemite valley woods, 1.5-4 m,
  a vase of many straight, slender stems from the base that arch outward at the top.
- Bark smooth grey-brown with pale lenticels; twigs brown and glandular-hairy.
- Leaves alternate and two-ranked, laid out flat in layers to catch shade light; broadly ovate to
  round, 5-10 cm, doubly serrate, heart-shaped base, short-pointed tip, soft-hairy, mid green
  with 6-8 pairs of straight parallel veins, paler beneath, thin and translucent.
- Nuts in pairs or threes at twig ends, each wrapped in a green bristly husk drawn out into a
  tubular beak ("beaked hazel"), ripening late summer.

Game notes: walk-through (no collision). One 4K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G branch phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "Hazel"
DESCRIPTION = ("Open multi-stem California hazelnut shrub, 2.1 m tall and 2 m across, with two-ranked soft "
               "serrate leaves, lenticelled grey stems and beaked nut husks. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 25000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.4, 1.3)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on stems",
             "A": "1"},
    "material_notes": ("One material M_Hazel: T_Hazel_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 5150
LEAF_TILES = ("mid", "mid2", "dark", "light", "yellow", "holes", "dry", "under")
SPRAYS = ("spray_a", "spray_b")


def _palettes():
    base = dict(yellow=(0.19, 0.16, 0.04), brown=(0.07, 0.035, 0.014), stalk=(0.09, 0.07, 0.035),
                dry=(0.14, 0.08, 0.035))
    return {
        "mid": dict(base, base=(0.052, 0.086, 0.020), tip=(0.058, 0.092, 0.022), vein=(0.080, 0.115, 0.036),
                    margin=(0.044, 0.070, 0.018)),
        "mid2": dict(base, base=(0.060, 0.094, 0.022), tip=(0.068, 0.098, 0.024), vein=(0.090, 0.122, 0.040),
                     margin=(0.050, 0.075, 0.020)),
        "dark": dict(base, base=(0.040, 0.070, 0.017), tip=(0.044, 0.074, 0.018), vein=(0.066, 0.098, 0.030),
                     margin=(0.034, 0.056, 0.015)),
        "light": dict(base, base=(0.078, 0.118, 0.030), tip=(0.088, 0.122, 0.032), vein=(0.105, 0.14, 0.05),
                      margin=(0.070, 0.100, 0.028)),
        "yellow": dict(base, base=(0.090, 0.110, 0.028), tip=(0.16, 0.15, 0.04), vein=(0.15, 0.15, 0.06),
                       margin=(0.12, 0.10, 0.03)),
        "holes": dict(base, base=(0.056, 0.088, 0.022), tip=(0.062, 0.092, 0.024), vein=(0.085, 0.118, 0.04),
                      margin=(0.060, 0.070, 0.022)),
        "dry": dict(base, base=(0.13, 0.075, 0.032), tip=(0.11, 0.06, 0.028), vein=(0.16, 0.10, 0.05),
                    margin=(0.08, 0.04, 0.02)),
        "under": dict(base, base=(0.088, 0.108, 0.058), tip=(0.092, 0.112, 0.060), vein=(0.12, 0.13, 0.08),
                      margin=(0.08, 0.095, 0.05)),
    }


PARAMS = {
    "mid": dict(gloss=0.05, hair=0.25),
    "mid2": dict(gloss=0.0, hair=0.25, damage=0.02, edge_burn=0.05),
    "dark": dict(gloss=0.1, hair=0.2),
    "light": dict(gloss=-0.05, hair=0.3, trans=0.72),
    "yellow": dict(yellow=0.55, damage=0.7, edge_burn=0.35, hair=0.2),
    "holes": dict(holes=1.0, damage=0.4, edge_burn=0.2, hair=0.2),
    "dry": dict(dry=1.0, edge_burn=0.6, holes=0.3, trans=0.25),
    "under": dict(hair=0.9, gloss=-0.4, trans=0.5),
}


def _shape(rng):
    return F.ovate(width=0.37, widest=0.50, tip_sharp=1.5, base_round=0.75, teeth=24, tooth_depth=0.07,
                   double=0.55, cordate=0.14, phase=rng.uniform(0, 1))


def _veins(nrng):
    return F.pinnate_veins(count=7, angle=0.85, curve=0.45, reach=0.93, width=0.37, widest=0.50,
                           start=0.07, stop=0.78, rng=nrng)


def _blade(X, Y, px, nrng, rng, key, pals, vein_scale=1.0):
    under = key == "under"
    return F.paint_blade(X, Y, nrng, _shape(rng), _veins(nrng), pals[key], px,
                         vein_width=0.0080 * vein_scale, vein_depth=-0.0002 if under else 0.00032,
                         puff=0.00028, tertiary=0.55, stalk=0.012, **PARAMS[key])


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("bark", 160)
    atlas.column("twig", 64)
    for key in LEAF_TILES:
        atlas.tile(key, 900, 1080)
    for key in SPRAYS:
        atlas.tile(key, 1400, 1500)
    atlas.tile("husk", 320, 480)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAF_TILES:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade(X, Y, px, nrng, rng, key, pals), meters_per_px=0.09 / X.shape[0])
    mixes = {"spray_a": ["mid", "dark", "mid2", "mid", "holes", "dark", "light"],
             "spray_b": ["mid2", "mid", "dark", "yellow", "mid", "mid2", "light"]}
    for key in SPRAYS:
        layer = S.paint_spray(atlas, key, nrng, rng, lambda Xl, Yl, pxl, i, k: _blade(Xl, Yl, pxl, nrng, rng, k, pals, 1.3),
                              6, leaf_len=0.36, angle=(55, 78), twig_color=(0.09, 0.065, 0.04), twig_width=0.008,
                              twig_top=0.66, keys=mixes[key], squash=(0.75, 1.0), taper=0.35)
        atlas.put(key, layer, meters_per_px=0.40 / layer.color.shape[0])
    S.paint_bark(atlas, "bark", nrng, (0.105, 0.088, 0.072), (0.075, 0.062, 0.050), blotch=0.6, stri=0.18,
                 aniso=4.0, lenticels=1.0, lent_color=(0.21, 0.19, 0.15), cracks=0.08, lichen=0.3, rough=0.6,
                 v_len=0.5, relief=0.00015)
    S.paint_bark(atlas, "twig", nrng, (0.09, 0.062, 0.038), (0.07, 0.05, 0.03), blotch=0.6, stri=0.25,
                 lenticels=0.5, rough=0.55, v_len=0.5)
    # Nut husk: green bristly bract, v runs from the stalk to the frilled beak.
    X, Y, px = atlas.grid("husk")
    layer = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=30.0, beta=1.6)
    layer.color = F.lerp((0.07, 0.10, 0.03), (0.12, 0.14, 0.05), n)
    layer.color = F.lerp(layer.color, (0.13, 0.10, 0.04), F.smoothstep(0.75, 1.0, Y) * 0.6)
    layer.alpha[...] = 1.0
    layer.height = 0.0002 * n
    layer.rough[...] = 0.62
    layer.trans[...] = 0.3
    atlas.put("husk", layer, meters_per_px=0.04 / X.shape[0], opaque=True)
    atlas.save()
    return atlas


SPEC = dict(
    height=2.12, rx=1.02, ry=0.95,
    profile=[(0.0, 0.22), (0.2, 0.45), (0.5, 0.82), (0.75, 1.0), (0.92, 0.82), (1.0, 0.45)],
    crowns=[(0.0, 0.0), (0.12, 0.06), (-0.09, 0.08), (0.03, -0.11)], crown_jitter=0.05,
    stem_radius=0.017, step=0.03, bark="bark", clumps=False, lobes=0.12,
    levels=[
        dict(count=17, elev=(72, 88), length=(0.95, 1.2), tropism=0.15, outward=0.35, wander=0.35, stop=0.96,
             gravity=0.12, low_frac=0.12, low_elev=(45, 60), low_length=(0.55, 0.8), bark="bark", tip_radius=0.22),
        dict(per_m=5.0, start=0.38, end=0.97, angle=(40, 70), length=(0.3, 0.6), radius=(0.40, 0.55),
             up=0.0, out=0.5, tropism=0.05, outward=0.4, wander=0.8, stop=1.0, max=7, bark="twig", tip_radius=0.35),
        dict(per_m=6.0, start=0.2, angle=(45, 75), abs_length=(0.10, 0.24), radius=(0.5, 0.7), up=0.0,
             out=0.2, tropism=0.05, outward=0.2, wander=1.0, stop=1.05, inner=0.5, inner_skip=0.6, max=6,
             step=0.02, bark="twig", tip_radius=0.5),
    ],
    shell=dict(count=320, depth=(0.72, 1.0), max_len=0.4, z_min=0.3, radius=0.0028),
    leaf=dict(level=1, zone=0.20, spacing=(0.035, 0.05), angle=(55, 80), size=0.092, petiole=0.12,
              arrangement="alternate", divergence=math.pi, face_up=1.3, face_out=0.2, hang=0.05, inner=0.55,
              inner_density=0.45, tip_min=0.45, tip_len=0.08, fold=(0.05, 0.2), droop=(0.1, 0.35),
              curl=(0.0, 0.1), jitter=0.25),
    full=0.28, lod_levels=(9, 9, 1), lod_step=(0.07, 0.14, 0.3),
    sides=[(5, 4, 3), (4, 3, 3), (3, 3, 3)], leaf_grid=((2, 2), (1, 1), (1, 1)), spray_grid=((2, 2), (1, 1), (1, 1)),
    spray_cross=False, spray_overhang=0.9, v_bark=0.5, seed=SEED,
)


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    rng = random.Random(SEED)
    desc = S.grow(SPEC, rng)
    krng = random.Random(SEED + 5)
    for tw in desc["twigs"]:
        for lf in tw["leaves"]:
            if lf["outer"] < 0.55:
                lf["key"] = krng.choices(["mid", "mid2", "yellow", "holes", "dry", "under"], [30, 25, 15, 12, 6, 12])[0]
            elif lf["from_tip"] < 0.04:
                lf["key"] = "light"
            else:
                lf["key"] = krng.choices(["mid", "mid2", "dark", "holes", "yellow", "under"], [34, 26, 22, 7, 4, 7])[0]
        tw["spray_key"] = SPRAYS[tw["spray"] % 2]
    nuts = []
    cands = [tw for tw in desc["twigs"] if tw["outer"] > 0.7 and tw["end"].z > 0.9]
    krng.shuffle(cands)
    for tw in cands[:12]:
        base = tw["end"] - tw["dir"] * 0.01
        for j in range(krng.choice([1, 2, 2, 3])):
            az = j * 2.1 + krng.uniform(-0.3, 0.3)
            side = tw["up"].cross(tw["dir"]).normalized()
            d = (tw["dir"] * 0.4 + side * math.cos(az) * 0.6 + tw["up"] * math.sin(az) * 0.6 - Vector((0, 0, 0.4))).normalized()
            nuts.append(dict(base=base, dir=d, phase=tw["phase"], size=krng.uniform(0.9, 1.1)))
    print(f"HOMESTEAD_PLANT branches={len(desc['branches'])} twigs={len(desc['twigs'])} "
          f"leaves={sum(len(t['leaves']) for t in desc['twigs'])} full={sum(t['full'] for t in desc['twigs'])}")
    batches = []
    for lod in range(3):
        b = S.emit(desc, atlas, lod, SPEC, lambda lf, tw: lf["key"], lambda tw: tw["spray_key"])
        if lod < 2:
            for nut in nuts:
                r = 0.008 * nut["size"]
                centre = nut["base"] + nut["dir"] * r * 1.4
                segs, rings = ((6, 4), (4, 3))[lod]
                b.sphere(centre, r, atlas.uv("husk", inset=False), segs=segs, rings=rings, stretch=1.25,
                         axis=nut["dir"], phase=nut["phase"])
                if lod == 0:   # tubular beak of the husk
                    tip = centre + nut["dir"] * r * 0.9
                    b.cone(tip, nut["dir"], 0.024, r * 0.5, atlas.uv("husk"), sides=5, phase=nut["phase"],
                           hook=0.15)
        batches.append(b)
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
