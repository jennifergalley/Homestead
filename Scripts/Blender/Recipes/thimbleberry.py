"""Thimbleberry (Rubus parviflorus): a knee-to-waist-high clump of unarmed canes with huge soft leaves.

The real plant (what the geometry and textures follow):
- A thornless raspberry of shady Sierra Nevada foothill and Yosemite streamsides, woodland edges
  and burns; forms loose patches 0.5-2 m tall from spreading rhizomes.
- Canes erect, sparsely branched, green-brown when young, later grey-brown with shreddy peeling
  bark; no prickles, glandular-hairy.
- Leaves alternate on long petioles, 10-20 cm across, palmately 5-lobed like a maple, heart-shaped
  at the base, coarsely and irregularly double-toothed, soft and velvety on both sides, mid green
  and held flat in a level layer; older ones yellow or develop brown spots.
- Flowers 3-5 cm, five crinkled white petals around a yellow centre, a few together at the cane
  tips; fruit a soft flattened red "thimble" 1.5 cm across.

Game notes: walk-through (no collision). One 4K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G per-cane phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "Thimbleberry"
DESCRIPTION = ("Thimbleberry clump, 0.9 m tall and 1.2 m across: thornless canes with big velvety maple-like "
               "leaves, white flowers and red berries. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 25000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.3, 0.55)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-cane random phase", "B": "leaf flutter 0 at leaf base -> 1 at tip, 0 on canes",
             "A": "1"},
    "material_notes": ("One material M_Thimbleberry: T_Thimbleberry_basecolor (sRGB, alpha = opacity mask, clip "
                       "0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 9091
LEAF_TILES = ("mid", "mid2", "light", "dark", "yellow", "spots")
CENTRE = 0.30        # palmate leaf: vein origin in tile units above the petiole end


def _palettes():
    base = dict(yellow=(0.21, 0.18, 0.04), brown=(0.07, 0.035, 0.014), stalk=(0.10, 0.09, 0.04))
    return {
        "mid": dict(base, base=(0.066, 0.104, 0.026), tip=(0.072, 0.108, 0.028), vein=(0.10, 0.135, 0.045),
                    margin=(0.056, 0.088, 0.022)),
        "mid2": dict(base, base=(0.058, 0.094, 0.023), tip=(0.064, 0.100, 0.025), vein=(0.092, 0.125, 0.042),
                     margin=(0.050, 0.080, 0.020)),
        "light": dict(base, base=(0.085, 0.125, 0.034), tip=(0.095, 0.130, 0.036), vein=(0.12, 0.155, 0.06),
                      margin=(0.075, 0.110, 0.030)),
        "dark": dict(base, base=(0.048, 0.080, 0.020), tip=(0.052, 0.084, 0.021), vein=(0.08, 0.11, 0.036),
                     margin=(0.042, 0.068, 0.018)),
        "yellow": dict(base, base=(0.12, 0.13, 0.032), tip=(0.18, 0.16, 0.04), vein=(0.17, 0.16, 0.06),
                       margin=(0.13, 0.11, 0.03)),
        "spots": dict(base, base=(0.064, 0.100, 0.026), tip=(0.070, 0.104, 0.027), vein=(0.10, 0.13, 0.045),
                      margin=(0.07, 0.08, 0.024), brown=(0.09, 0.045, 0.016)),
    }


PARAMS = {
    "mid": dict(hair=0.15, gloss=-0.1),
    "mid2": dict(hair=0.15, gloss=-0.1, damage=0.03, edge_burn=0.05),
    "light": dict(hair=0.18, gloss=-0.15, trans=0.72),
    "dark": dict(hair=0.12, gloss=-0.05),
    "yellow": dict(hair=0.3, yellow=0.6, damage=0.6, edge_burn=0.35),
    "spots": dict(hair=0.3, damage=1.0, holes=0.5, edge_burn=0.2),
}

LOBES = [(0.0, 0.52), (-0.95, 0.47), (0.95, 0.47), (-1.85, 0.33), (1.85, 0.33)]   # (angle from +Y, reach)


def _palmate(rng):
    phase = rng.uniform(0, 1)
    jit = [rng.uniform(0.92, 1.06) for _ in LOBES]

    def shape(X, Y):
        dx, dy = X, Y - CENTRE
        r = np.hypot(dx, dy)
        phi = np.arctan2(dx, dy)          # 0 toward the tip, +/-pi toward the petiole
        reach = np.full(X.shape, 0.16)
        for (a, rr), j in zip(LOBES, jit):
            diff = np.angle(np.exp(1j * (phi - a)))
            reach = np.maximum(reach, rr * j * np.exp(-(diff / 0.50) ** 2 * 1.6) ** 0.5 *
                               (1 - 0.25 * np.abs(diff)))
        # Heart-shaped base: shallow notch toward the petiole.
        notch = np.exp(-((np.abs(phi) - math.pi) / 0.35) ** 2) * 0.10
        reach = reach - notch
        teeth = S.F.serration(phi / math.tau + 0.5, np.ones_like(phi), 58, 0.045, double=0.6, phase=phase)
        reach = reach * (1 + teeth * 1.2)
        inside = reach - r
        return inside, np.clip(r / 0.5, 0, 1), reach

    return shape


def _veins(nrng):
    veins = []
    for a, rr in LOBES:
        pts = [(math.sin(a) * rr * 0.9 * t, CENTRE + math.cos(a) * rr * 0.9 * t) for t in np.linspace(0, 1, 10)]
        veins.append((pts, 1.0 if a == 0 else 0.85))
        for k in range(4):
            t0 = 0.22 + 0.17 * k
            px_, py_ = math.sin(a) * rr * 0.9 * t0, CENTRE + math.cos(a) * rr * 0.9 * t0
            for side in (-1, 1):
                b = a + side * 0.9
                ln = rr * 0.28 * (1 - 0.5 * t0)
                veins.append(([(px_, py_), (px_ + math.sin(b) * ln, py_ + math.cos(b) * ln)], 0.4))
    veins.append(([(0.0, 0.0), (0.0, CENTRE)], 1.0))
    return veins


def _blade(X, Y, px, nrng, rng, key, pals):
    return F.paint_blade(X, Y, nrng, _palmate(rng), _veins(nrng), pals[key], px, vein_width=0.0062,
                         vein_depth=0.0002, puff=0.00018, tertiary=0.45, stalk=0.012, **PARAMS[key])


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("cane", 128)
    atlas.column("petiole", 64)
    for key in LEAF_TILES:
        atlas.tile(key, 1100, 1100)
    atlas.tile("spray", 1300, 1500)
    atlas.tile("flower", 400, 400)
    atlas.tile("berry", 160, 160)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAF_TILES:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade(X, Y, px, nrng, rng, key, pals), meters_per_px=0.17 / X.shape[0])
    layer = S.paint_spray(atlas, "spray", nrng, rng, lambda Xl, Yl, pxl, i, k: _blade(Xl, Yl, pxl, nrng, rng, k, pals),
                          3, leaf_len=0.42, angle=(55, 75), twig_color=(0.10, 0.08, 0.04), twig_width=0.01,
                          twig_top=0.5, keys=["mid", "mid2", "dark", "mid"], squash=(0.7, 0.95), taper=0.25)
    atlas.put("spray", layer, meters_per_px=0.45 / layer.color.shape[0])
    # Flower: five broad crinkled white petals, yellow centre.
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    petal = F.ovate(width=0.42, widest=0.68, tip_sharp=0.4, base_round=1.3, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(72 * k + rng.uniform(-8, 8))
        Xl, Yl = S.rotated(X, Y, (0.0, 0.5), th, 0.46)
        crinkle = [([(0, 0.05), (0.2 * s, 0.95)], 0.3) for s in (-1, -0.4, 0.4, 1)]
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, crinkle,
                                 dict(base=(0.66, 0.66, 0.60), tip=(0.74, 0.74, 0.70), vein=(0.60, 0.60, 0.54),
                                      margin=(0.76, 0.76, 0.72)), px / 0.46, vein_width=0.02, vein_depth=0.0003,
                                 puff=0.0006, tertiary=0.0, trans=0.85, gloss=-0.2))
    r = np.hypot(X, Y - 0.5)
    disc = 1 - F.smoothstep(0.08, 0.11, r)
    n = F.noise(X.shape, nrng, freq=50.0, beta=1.4)
    layer.color = F.lerp(layer.color, F.lerp((0.40, 0.30, 0.05), (0.55, 0.45, 0.10), n), disc)
    layer.height = layer.height + 0.0005 * disc * n
    atlas.put("flower", layer, meters_per_px=0.045 / X.shape[0])
    X, Y, px = atlas.grid("berry")
    layer = F.Layer(X.shape)
    cells = np.abs(np.sin(X * 90) * np.sin(Y * 90))
    layer.color = F.lerp((0.16, 0.012, 0.015), (0.32, 0.03, 0.03), cells)
    layer.alpha[...] = 1
    layer.height = 0.0004 * cells
    layer.rough[...] = 0.45
    layer.trans[...] = 0.2
    atlas.put("berry", layer, meters_per_px=0.02 / X.shape[0], opaque=True)
    S.paint_bark(atlas, "cane", nrng, (0.11, 0.085, 0.055), (0.07, 0.075, 0.035), blotch=0.7, stri=0.5, aniso=16.0,
                 cracks=0.5, crack_color=(0.16, 0.13, 0.09), rough=0.65, v_len=0.5)
    S.paint_bark(atlas, "petiole", nrng, (0.085, 0.10, 0.04), (0.10, 0.08, 0.04), blotch=0.4, stri=0.2, rough=0.55,
                 v_len=0.5)
    atlas.save()
    return atlas


SPEC = dict(
    height=0.92, rx=0.62, ry=0.58,
    profile=[(0.0, 0.45), (0.3, 0.75), (0.6, 0.95), (0.85, 1.0), (1.0, 0.8)],
    crowns=[(0.0, 0.0), (0.22, 0.1), (-0.2, 0.14), (0.05, -0.22), (-0.18, -0.15), (0.28, -0.12)], crown_jitter=0.08,
    stem_radius=0.0065, step=0.025, bark="cane", clumps=False, lobes=0.12,
    levels=[
        dict(count=34, elev=(62, 86), length=(0.35, 1.05), tropism=0.25, outward=0.25, wander=0.4, stop=1.0,
             gravity=0.05, bark="cane", tip_radius=0.5),
        dict(per_m=1.6, start=0.5, end=0.9, angle=(28, 50), length=(0.35, 0.6), radius=(0.55, 0.7), up=0.3,
             out=0.3, tropism=0.25, outward=0.2, wander=0.5, stop=1.02, max=2, bark="cane", tip_radius=0.5),
    ],
    leaf=dict(level=0, zone=0.55, spacing=(0.06, 0.09), angle=(55, 80), size=0.27, petiole=0.5,
              arrangement="alternate", face_up=1.4, face_out=0.3, hang=0.0, inner=0.0, inner_density=1.0,
              tip_min=0.55, tip_len=0.15, fold=(0.0, 0.12), droop=(0.1, 0.35), curl=(0.0, 0.15), jitter=0.2,
              flat=0.6, petiole_tube="petiole", petiole_radius=0.0018),
    full=1.0, lod2_leaves=0.6, lod_levels=(9, 9, 1), lod_step=(0.06, 0.12, 0.3),
    sides=[(5, 4, 3), (4, 3, 3)], leaf_grid=((3, 3), (1, 2), (1, 1)), spray_grid=((2, 2), (1, 1), (1, 1)),
    spray_cross=False, spray_overhang=0.6, v_bark=0.5, seed=SEED,
)


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    rng = random.Random(SEED)
    desc = S.grow(SPEC, rng)
    krng = random.Random(SEED + 5)
    for tw in desc["twigs"]:
        for lf in tw["leaves"]:
            low = lf["node"].z < 0.35
            if low:
                lf["key"] = krng.choices(["mid", "mid2", "yellow", "spots", "dark"], [25, 25, 20, 20, 10])[0]
            elif lf["from_tip"] < 0.05:
                lf["key"] = "light"
            else:
                lf["key"] = krng.choices(["mid", "mid2", "dark", "spots", "yellow"], [36, 30, 22, 8, 4])[0]
    fruit = []
    tips = [tw for tw in desc["twigs"] if tw["end"].z > 0.5]
    krng.shuffle(tips)
    for tw in tips[:14]:
        for j in range(krng.randint(1, 3)):
            az = j * 2.3 + krng.uniform(0, 1)
            d = (Vector((math.cos(az), math.sin(az), 0)) * 0.6 + Vector((0, 0, 0.8))).normalized()
            kind = krng.choices(["flower", "berry"], [50, 50])[0]
            fruit.append(dict(base=tw["end"], tip=tw["end"] + d * krng.uniform(0.03, 0.06), dir=d, kind=kind,
                              phase=tw["phase"], spin=krng.uniform(0, 6.28)))
    print(f"HOMESTEAD_PLANT branches={len(desc['branches'])} twigs={len(desc['twigs'])} "
          f"leaves={sum(len(t['leaves']) for t in desc['twigs'])}")
    batches = []
    for lod in range(3):
        b = S.emit(desc, atlas, lod, SPEC, lambda lf, tw: lf["key"], lambda tw: "spray")
        for it in fruit:
            if lod == 0:
                b.tube([it["base"], it["tip"]], [0.0012, 0.0009], 3, atlas.uv("petiole"), v_length=0.5,
                       phase=it["phase"], flutter=0.3)
            if it["kind"] == "flower":
                if lod < 2:
                    b.flat(it["tip"], it["dir"], 0.045, atlas.uv("flower"), spin=it["spin"], cup=0.15,
                           phase=it["phase"], segs=2 if lod == 0 else 1)
            elif lod < 2:
                r = 0.0075 if it["kind"] == "berry" else 0.006
                b.sphere(it["tip"] + it["dir"] * r * 0.7, r, atlas.uv("berry", inset=False),
                         segs=(7, 5)[lod], rings=(4, 3)[lod], stretch=0.6, axis=it["dir"], phase=it["phase"])
        batches.append(b)
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
