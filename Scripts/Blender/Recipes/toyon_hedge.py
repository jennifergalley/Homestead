"""Toyon (Heteromeles arbutifolia) hedge thicket: a dense evergreen shrub mass that blocks passage.

The real plant (what the geometry and textures follow):
- Evergreen shrub of Sierra Nevada foothill chaparral and oak woodland edges, 2-5 m, many stems
  from a burl; in the understorey it forms dense rounded masses that are hard to push through.
- Stems smooth grey to grey-brown with pale lenticels; young twigs reddish-green.
- Leaves alternate, 5-10 cm, oblong-elliptic, thick and leathery, dark glossy green above with a
  pale yellow-green midrib, paler beneath, margins with small sharp forward teeth; petioles
  1-2 cm, often red. The oldest leaves turn orange-red or yellow before they drop.
- Flat clusters of small pomes (5-8 mm) at the twig ends: green in summer, orange-red by winter.

Game notes: BLOCKING (convex collision) until cleared with a machete. Elongated so several can be
lined up as a hedge. One 4K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py (R height, G branch phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "ToyonHedge"
DESCRIPTION = ("Dense evergreen toyon hedge mass, 3.1 x 1.7 m and 1.9 m tall, with leathery toothed leaves, "
               "grey stems, reddening old leaves and berry clusters. Blocking.")
COLLISION = "convex"
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.5, 1.0)}
REPORT = {
    "blocking": True,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on stems",
             "A": "1"},
    "material_notes": ("One material M_ToyonHedge: T_ToyonHedge_basecolor (sRGB, alpha = opacity mask, "
                       "clip 0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, "
                       "G translucency mask, B AO). Two-sided foliage, masked."),
}

SEED = 3301
LEAF_TILES = ("dark", "dark2", "mid", "young", "red", "yellow", "under")
SPRAYS = ("spray_a", "spray_b", "spray_c")


def _palettes():
    base = dict(yellow=(0.20, 0.15, 0.03), brown=(0.06, 0.03, 0.012), stalk=(0.12, 0.05, 0.03))
    return {
        "dark": dict(base, base=(0.026, 0.046, 0.013), tip=(0.030, 0.050, 0.014), vein=(0.085, 0.100, 0.035),
                     margin=(0.020, 0.034, 0.012)),
        "dark2": dict(base, base=(0.032, 0.054, 0.015), tip=(0.036, 0.058, 0.016), vein=(0.095, 0.110, 0.040),
                      margin=(0.024, 0.040, 0.013)),
        "mid": dict(base, base=(0.044, 0.070, 0.019), tip=(0.050, 0.076, 0.020), vein=(0.110, 0.125, 0.045),
                    margin=(0.032, 0.050, 0.016)),
        "young": dict(base, base=(0.070, 0.100, 0.028), tip=(0.085, 0.085, 0.030), vein=(0.12, 0.14, 0.05),
                      margin=(0.090, 0.060, 0.028)),
        "red": dict(base, base=(0.15, 0.045, 0.018), tip=(0.19, 0.07, 0.020), vein=(0.18, 0.10, 0.04),
                    margin=(0.12, 0.03, 0.012), yellow=(0.28, 0.13, 0.02), brown=(0.07, 0.025, 0.01)),
        "yellow": dict(base, base=(0.13, 0.12, 0.025), tip=(0.19, 0.14, 0.025), vein=(0.20, 0.17, 0.05),
                       margin=(0.10, 0.07, 0.02), brown=(0.08, 0.035, 0.012)),
        "under": dict(base, base=(0.070, 0.086, 0.040), tip=(0.074, 0.090, 0.042), vein=(0.10, 0.11, 0.05),
                      margin=(0.060, 0.070, 0.035)),
    }


PARAMS = {
    "dark": dict(gloss=0.55),
    "dark2": dict(gloss=0.45, damage=0.05),
    "mid": dict(gloss=0.4, damage=0.08, edge_burn=0.05),
    "young": dict(gloss=0.2, trans=0.7),
    "red": dict(gloss=0.3, damage=0.4, edge_burn=0.3, yellow=0.4),
    "yellow": dict(gloss=0.2, damage=0.7, edge_burn=0.35, holes=0.4),
    "under": dict(gloss=-0.3, trans=0.5, hair=0.2),
}


def _shape(rng):
    return F.ovate(width=0.165, widest=0.52, tip_sharp=0.9, base_round=0.85, teeth=24, tooth_depth=0.075,
                   phase=rng.uniform(0, 1))


def _veins(nrng):
    return F.pinnate_veins(count=12, angle=1.1, curve=0.35, reach=0.88, width=0.165, widest=0.52,
                           start=0.06, stop=0.88, rng=nrng)


def _blade(X, Y, px, nrng, rng, key, pals, vein_scale=1.0):
    under = key == "under"
    return F.paint_blade(X, Y, nrng, _shape(rng), _veins(nrng), pals[key], px,
                         vein_width=0.0065 * vein_scale if key != "under" else 0.0075,
                         vein_depth=-0.0002 if under else 0.00022, puff=0.00012, tertiary=0.25,
                         stalk=0.014, **PARAMS[key])


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("bark", 192)
    atlas.column("twig", 96)
    for key in LEAF_TILES:
        atlas.tile(key, 480, 1200)
    for key in SPRAYS:
        atlas.tile(key, 1024, 1536)
    atlas.tile("berries", 128, 128)
    atlas.tile("berries_ripe", 128, 128)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAF_TILES:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade(X, Y, px, nrng, rng, key, pals), meters_per_px=0.08 / X.shape[0])
    mixes = {"spray_a": ["dark", "dark2", "dark", "mid"], "spray_b": ["dark2", "mid", "dark", "dark2", "mid", "dark", "red"],
             "spray_c": ["mid", "dark", "dark2", "dark", "yellow", "mid", "young"]}
    for key in SPRAYS:
        layer = S.paint_spray(atlas, key, nrng, rng, lambda Xl, Yl, pxl, i, k: _blade(Xl, Yl, pxl, nrng, rng, k, pals, 1.4),
                              11, leaf_len=0.30, angle=(35, 62), twig_color=(0.10, 0.05, 0.03), twig_width=0.009,
                              twig_top=0.78, keys=mixes[key], squash=(0.6, 1.0), taper=0.45)
        atlas.put(key, layer, meters_per_px=0.35 / layer.color.shape[0])
    S.paint_bark(atlas, "bark", nrng, (0.12, 0.105, 0.09), (0.07, 0.06, 0.05), blotch=0.7, stri=0.25,
                 lenticels=0.8, lent_color=(0.22, 0.20, 0.17), cracks=0.25, lichen=0.35, rough=0.66, v_len=0.6)
    S.paint_bark(atlas, "twig", nrng, (0.10, 0.045, 0.03), (0.06, 0.07, 0.03), blotch=0.8, stri=0.2,
                 lenticels=0.4, rough=0.5, v_len=0.6)
    for key, (c0, c1) in {"berries": ((0.12, 0.14, 0.03), (0.20, 0.17, 0.035)),
                          "berries_ripe": ((0.40, 0.06, 0.02), (0.55, 0.16, 0.03))}.items():
        X, Y, px = atlas.grid(key)
        layer = F.Layer(X.shape)
        n = F.noise(X.shape, nrng, freq=8.0, beta=2.4)
        layer.color = F.lerp(c0, c1, n)
        tip = F.smoothstep(0.85, 0.95, Y)
        layer.color = F.lerp(layer.color, (0.05, 0.03, 0.015), tip)
        layer.color = F.lerp((0.08, 0.06, 0.02), layer.color, F.smoothstep(0.0, 0.1, Y))
        layer.alpha[...] = 1.0
        layer.rough[...] = 0.35
        layer.trans[...] = 0.1
        atlas.put(key, layer, meters_per_px=0.01 / X.shape[0], opaque=True)
    atlas.save()
    return atlas


SPEC = dict(
    height=1.92, rx=1.55, ry=0.84,
    profile=[(0.0, 0.55), (0.12, 0.86), (0.35, 1.0), (0.70, 0.97), (0.88, 0.78), (1.0, 0.38)],
    crowns=[(-1.0, 0.05), (-0.45, -0.10), (0.10, 0.08), (0.62, -0.05), (1.05, 0.10)],
    stem_radius=0.022, step=0.03, bark="bark", clumps=True, lobes=0.16,
    levels=[
        dict(count=40, elev=(60, 88), length=(0.85, 1.25), tropism=0.45, outward=0.3, wander=0.6, stop=0.82,
             low_frac=0.3, low_elev=(18, 40), low_length=(0.45, 0.75), bark="bark", tip_radius=0.25),
        dict(per_m=9.0, start=0.15, end=0.97, angle=(40, 75), length=(0.4, 0.8), radius=(0.45, 0.62),
             up=0.2, out=0.45, tropism=0.3, outward=0.5, wander=0.9, stop=1.0, inner=0.45, inner_skip=0.3,
             max=16, bark="bark", tip_radius=0.3),
        dict(per_m=7.0, start=0.15, angle=(35, 65), abs_length=(0.12, 0.30), radius=(0.5, 0.7), up=0.3,
             out=0.3, tropism=0.5, outward=0.5, wander=1.2, stop=1.04, inner=0.58, inner_skip=0.8, max=10,
             step=0.02, bark="twig", tip_radius=0.5),
    ],
    leaf=dict(level=1, zone=0.15, spacing=(0.016, 0.026), angle=(40, 65), size=0.078, petiole=0.16,
              arrangement="alternate", face_out=0.45, hang=0.12, inner=0.62, inner_density=0.3, tip_min=0.5,
              tip_len=0.06, fold=(0.12, 0.32), droop=(0.05, 0.3), curl=(0.0, 0.12)),
    shell=dict(count=1000, depth=(0.72, 1.0), max_len=0.45, z_min=0.03, radius=0.0035),
    full=0.15, lod_levels=(9, 9, 1), lod_step=(0.07, 0.14, 0.25),
    sides=[(5, 4, 3), (4, 3, 3), (3, 3, 3)], leaf_grid=((2, 2), (1, 1), (1, 1)), spray_grid=((2, 2), (1, 1), (1, 1)),
    spray_cross=True, spray_overhang=0.9, v_bark=0.6, seed=SEED,
)


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    rng = random.Random(SEED)
    desc = S.grow(SPEC, rng)
    krng = random.Random(SEED + 5)
    for tw in desc["twigs"]:
        for lf in tw["leaves"]:
            if lf["outer"] < 0.62:
                lf["key"] = krng.choices(["dark2", "mid", "yellow", "red", "under"], [36, 34, 8, 5, 17])[0]
            elif lf["from_tip"] < 0.035:
                lf["key"] = "young"
            else:
                lf["key"] = krng.choices(["dark", "dark2", "mid", "red", "yellow", "under"], [36, 30, 22, 3, 2, 7])[0]
        tw["spray_key"] = SPRAYS[tw["spray"] % 3]
    # Berry clusters on a few outer twig ends.
    clusters = []
    outer = [tw for tw in desc["twigs"] if tw["outer"] > 0.8 and tw["end"].z > 0.6]
    krng.shuffle(outer)
    for tw in outer[:16]:
        ripe = krng.random() < 0.3
        centre = tw["end"] + tw["up"] * 0.02
        berries = []
        for j in range(krng.randint(8, 16)):
            a = j * S.GOLDEN
            r = 0.012 * math.sqrt(j + 0.5)
            off = Vector((math.cos(a) * r, math.sin(a) * r, -0.004 * j ** 0.5))
            berries.append((centre + off, krng.uniform(0.0033, 0.0043)))
        clusters.append(dict(berries=berries, ripe=ripe, phase=tw["phase"], stem=tw["end"]))
    print(f"HOMESTEAD_PLANT branches={len(desc['branches'])} twigs={len(desc['twigs'])} "
          f"leaves={sum(len(t['leaves']) for t in desc['twigs'])} full={sum(t['full'] for t in desc['twigs'])}")
    batches = []
    for lod in range(3):
        b = S.emit(desc, atlas, lod, SPEC, lambda lf, tw: lf["key"], lambda tw: tw["spray_key"])
        for cl in clusters:
            key = "berries_ripe" if cl["ripe"] else "berries"
            segs, rings = ((5, 3), (4, 2), (3, 2))[lod]
            for i, (pos, r) in enumerate(cl["berries"]):
                if lod == 2 and i % 3:
                    continue
                if lod == 0:
                    b.tube([cl["stem"], pos], [0.0012, 0.0008], 3, atlas.uv("twig"), v_length=0.6,
                           phase=cl["phase"], flutter=0.3)
                b.sphere(pos, r * (1.3 if lod == 2 else 1.0), atlas.uv(key, inset=False), segs=segs, rings=rings,
                         phase=cl["phase"])
        batches.append(b)
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
