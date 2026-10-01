"""Cow parsley (Anthriscus sylvestris) in flower: frothy white umbels lining the lake trail.

The real plant (what the geometry and textures follow):
- The lace of Cornish lanes and hedge banks in April-June, 60-150 cm (here about 80 cm).
- Fresh green, fern-like leaves, two or three times pinnate with toothed, lobed leaflets, mostly from
  the base in a loose mound, a few smaller ones where the stems branch.
- Hollow, furrowed stems, often flushed purple low down, branching near the top into several stalks.
- Each stalk ends in a compound umbel 4-10 cm across: 6-12 rays spreading like a parasol, each
  carrying a small domed umbellet of tiny white five-petalled florets, so the whole head reads as
  a loose white froth.

Game notes: walk-through (no collision). About 80 cm tall and 55 cm across; the white umbels float
over the trail's edge. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py, the same sway as the bluebell clump (the umbels nod the most).
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "CowParsley"
MESH = "SM_CowParsley"
DESCRIPTION = ("Cow parsley in flower, 80 cm tall and 55 cm across: a mound of fresh fern-like leaves and "
               "branching hollow stems topped with lacy white umbels. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 14000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.72)}
REPORT = W.report(NAME)

SEED = 5547
HEIGHT = 0.82
GREEN = dict(base=(0.055, 0.105, 0.030), tip=(0.065, 0.115, 0.034), vein=(0.085, 0.13, 0.045),
             margin=(0.055, 0.095, 0.028), brown=(0.09, 0.06, 0.025))


def _leaflet(X, Y, px, nrng, rng, pal):
    # A lobed, sharply toothed leaflet (the frond's last division).
    shape = F.ovate(width=0.30, widest=0.40, tip_sharp=1.2, base_round=0.9, teeth=9, tooth_depth=0.14,
                    double=0.6, phase=rng.uniform(0, 1))
    veins = F.pinnate_veins(count=4, angle=0.8, curve=0.4, reach=0.85, width=0.30, widest=0.40,
                            start=0.12, stop=0.8, rng=nrng)
    return F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.008, vein_depth=0.00015,
                         puff=0.0001, tertiary=0.2, trans=0.5)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    for key in ("frond", "frond2", "frond_old"):
        atlas.tile(key, 560, 760)
    for key in ("umbellet", "umbellet2"):
        atlas.tile(key, 300, 300)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = {"frond": GREEN, "frond2": {k: tuple(c * f for c, f in zip(v, (1.08, 1.06, 0.92))) for k, v in GREEN.items()},
            "frond_old": dict(GREEN, base=(0.07, 0.10, 0.03))}
    for key, pal in pals.items():
        # A pinnate frond seen from above: opposite pairs of lobed leaflets along a green rachis.
        layer = S.paint_spray(atlas, key, nrng, rng, lambda Xl, Yl, pxl, i, k, pal=pal: _leaflet(Xl, Yl, pxl, nrng, rng, pal),
                              11, leaf_len=0.22, angle=(50, 70), arrangement="opposite",
                              twig_color=(0.07, 0.11, 0.035), twig_width=0.008, twig_top=0.86, squash=(0.55, 1.0),
                              taper=0.6)
        atlas.put(key, layer, meters_per_px=0.22 / layer.color.shape[0])
    W.paint_umbellet(atlas, "umbellet", nrng, rng, florets=26)
    W.paint_umbellet(atlas, "umbellet2", nrng, rng, florets=20, colour=(0.82, 0.82, 0.78))
    W.paint_stem(atlas, "stem", nrng, (0.12, 0.06, 0.08), (0.07, 0.11, 0.04), length_m=0.8)
    atlas.save()
    return atlas


def describe(rng):
    mound = []
    for i in range(10):
        mound.append(dict(head=i * 2.39996 + rng.uniform(-0.3, 0.3), length=rng.uniform(0.20, 0.30),
                          elev=math.radians(rng.uniform(25, 55)), droop=rng.uniform(0.4, 0.9),
                          key=rng.choices(("frond", "frond2", "frond_old"), (45, 40, 15))[0], keep=rng.random()))
    stems = []
    for i in range(5):
        az = i * 2.39996 + rng.uniform(-0.3, 0.3)
        branches = []
        for k in range(rng.randint(2, 3)):
            umbel_rays = []
            for r in range(rng.randint(10, 13)):
                umbel_rays.append(dict(az=r * math.tau / 12 + rng.uniform(-0.2, 0.2), length=rng.uniform(0.035, 0.055),
                                       rise=rng.uniform(0.35, 0.7), key=rng.choice(("umbellet", "umbellet2")),
                                       size=rng.uniform(0.024, 0.032), spin=rng.uniform(0, math.tau)))
            branches.append(dict(az=rng.uniform(0, math.tau), length=rng.uniform(0.08, 0.16),
                                 spread=rng.uniform(0.35, 0.7), rays=umbel_rays))
        stems.append(dict(base=Vector((math.cos(az) * 0.04, math.sin(az) * 0.04, 0.0)), length=rng.uniform(0.66, 0.80),
                          lean=az, tilt=rng.uniform(0.08, 0.2), branches=branches, phase=rng.random(),
                          frond_az=rng.uniform(0, math.tau)))
    return dict(mound=mound, stems=stems)


def _umbel(b, atlas, lod, top, axis, rays, phase):
    """A compound umbel: rays spreading like a parasol from ``top``, each ending in a domed umbellet."""
    side = axis.orthogonal().normalized()
    keep = rays if lod == 0 else rays[:: (2 if lod == 1 else 3)]
    for ray in keep:
        a = ray["az"]
        out = (side * math.cos(a) + axis.cross(side) * math.sin(a)).normalized()
        d = (out + axis * ray["rise"]).normalized()
        end = top + d * ray["length"]
        if lod < 2:
            b.tube([top, top + d * ray["length"] * 0.5, end], [0.0008, 0.0007, 0.0006], 3, atlas.uv("stem"),
                   v_length=0.1, phase=phase, flutter=0.5)
        b.flat(end + axis * 0.002, (axis + out * 0.3).normalized(), ray["size"] * (1.0, 1.15, 1.4)[lod],
               atlas.uv(ray["key"]), spin=ray["spin"], cup=-0.25, phase=phase, flutter=0.7, segs=1)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"fronds": 0, "stems": 0, "umbels": 0}
    base = Vector((0, 0, 0.004))
    for lf in desc["mound"]:
        if lod == 1 and lf["keep"] > 0.7 or lod == 2 and lf["keep"] > 0.45:
            continue
        h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        t0 = b.triangles
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.3).normalized(), lf["length"],
               lf["length"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]), rows=(4, 3, 2)[lod],
               cols=(2, 2, 1)[lod], fold=0.1, droop=lf["droop"], phase=lf["head"] / math.tau, flutter=0.8)
        stats["fronds"] += b.triangles - t0
    for st in desc["stems"]:
        lean = Vector((math.cos(st["lean"]), math.sin(st["lean"]), 0.0))
        n = (9, 6, 3)[lod]
        pts = [st["base"] + (Vector((0, 0, 1)) * t + lean * st["tilt"] * t * t) * st["length"]
               for t in np.linspace(0, 1, n + 1)]
        t0 = b.triangles
        b.tube(pts, [0.0035 * (1 - 0.5 * i / n) for i in range(n + 1)], (5, 4, 3)[lod], atlas.uv("stem"),
               v_length=0.8, phase=st["phase"], flutter=0.15)
        # A smaller frond where the stem branches.
        acc = S.arclength(pts)
        node, _, _ = S.at(pts, acc, acc[-1] * 0.72)
        h = Vector((math.cos(st["frond_az"]), math.sin(st["frond_az"]), 0))
        b.card(node, (h + Vector((0, 0, 0.6))).normalized(), Vector((0, 0, 1)), 0.12,
               0.12 * S.tile_aspect(atlas, "frond2"), atlas.uv("frond2"), rows=(3, 2, 1)[lod], cols=(2, 1, 1)[lod],
               fold=0.1, droop=0.5, phase=st["phase"], flutter=0.8)
        stats["stems"] += b.triangles - t0
        t0 = b.triangles
        top = pts[-1]
        axis = (Vector((0, 0, 1)) + lean * 0.15).normalized()
        _umbel(b, atlas, lod, top, axis, st["branches"][0]["rays"], st["phase"])
        # The side stalks fork off just below the main umbel, each with its own head.
        node, _, _ = S.at(pts, acc, acc[-1] * 0.9)
        for br in st["branches"][1:]:
            out = Vector((math.cos(br["az"]), math.sin(br["az"]), 0))
            d = (axis + out * br["spread"]).normalized()
            end = node + d * br["length"]
            b.tube([node, node + d * br["length"] * 0.5, end], [0.0018, 0.0015, 0.0013], (4, 3, 3)[lod],
                   atlas.uv("stem"), v_length=0.3, phase=st["phase"], flutter=0.4)
            _umbel(b, atlas, lod, end, (d + Vector((0, 0, 1))).normalized(), br["rays"], st["phase"])
        stats["umbels"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    desc = describe(random.Random(SEED))
    batches = [emit(desc, atlas, lod, HEIGHT) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESH, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
