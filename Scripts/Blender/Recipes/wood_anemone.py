"""Wood anemone (Anemone nemorosa) in flower: a white woodland spring flower along the lake trail.

The real plant (what the geometry and textures follow):
- Carpets the floor of old woods and shady hedge banks, flowering March-May before the canopy closes.
- Each slender stem, 6-30 cm (here 9-16 cm), carries a whorl of three stalked leaves about two-thirds
  of the way up, each deeply cut into three toothed lobes, and one flower above them.
- The flower is 2-4 cm across: six or seven white tepals (no true petals), often flushed pink or
  lilac on the back, round a boss of many yellow anthers. Flowers open toward the sun and nod
  closed in dull weather; young buds hang down.
- A clump is a loose patch of stems from a creeping rhizome, some flowering, some leaf-only.

Game notes: walk-through (no collision). About 15 cm tall and 35 cm across; the white stars are the
cue from a distance. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py, the same sway as the bluebell clump.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "WoodAnemone"
MESH = "SM_WoodAnemoneClump"
DESCRIPTION = ("A clump of wood anemones in flower, 15 cm tall and 35 cm across: slender stems, a whorl of "
               "three cut leaves each, and white six-tepalled flowers with yellow centres. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.08)}
REPORT = W.report(NAME)

SEED = 7231
HEIGHT = 0.16


def _paint_leaf(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.9
    # One of the three leaf segments: wedge-based and deeply, coarsely toothed toward the tip.
    shape = F.ovate(width=w, widest=0.66, tip_sharp=0.7, base_round=2.2, base=0.02, tip=0.98,
                    teeth=5, tooth_depth=0.30, double=0.6)
    veins = F.pinnate_veins(count=4, angle=0.7, curve=0.4, reach=0.85, base=0.02, tip=0.98, width=w,
                            widest=0.62, start=0.25, stop=0.85, rng=nrng)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.04, vein_depth=0.0002,
                          puff=0.0002, tertiary=0.3, trans=0.5, stalk=0.02, **extra)
    atlas.put(key, layer, meters_per_px=0.05 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    for key in ("leaf", "leaf2"):
        atlas.tile(key, 260, 520)
    for key in ("flower", "flower2", "bud"):
        atlas.tile(key, 360, 360)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    green = dict(base=(0.045, 0.085, 0.025), tip=(0.055, 0.095, 0.030), vein=(0.07, 0.11, 0.04),
                 margin=(0.050, 0.080, 0.026), brown=(0.08, 0.05, 0.02))
    _paint_leaf(atlas, "leaf", nrng, green)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (1.1, 1.05, 0.9))) for k, v in green.items()})
    W.paint_flower_face(atlas, "flower", nrng, petals=6, inner=(0.78, 0.78, 0.74), outer=(0.86, 0.85, 0.82),
                        eye=(0.80, 0.80, 0.70), centre=(0.70, 0.55, 0.08), size_m=0.035, rim=0.94, waist=0.30,
                        gap=0.05, veins=0.15, eye_radius=0.16, centre_radius=0.13, dots=14)
    W.paint_flower_face(atlas, "flower2", nrng, petals=7, inner=(0.78, 0.74, 0.76), outer=(0.84, 0.80, 0.82),
                        eye=(0.78, 0.76, 0.70), centre=(0.68, 0.52, 0.07), size_m=0.032, rim=0.92, waist=0.28,
                        gap=0.05, veins=0.15, eye_radius=0.16, centre_radius=0.13, dots=14)
    # A closed bud from the side: the tepals' pink-flushed backs.
    W.paint_flower_face(atlas, "bud", nrng, petals=3, inner=(0.62, 0.48, 0.55), outer=(0.74, 0.66, 0.70),
                        eye=(0.66, 0.55, 0.60), centre=(0.60, 0.50, 0.55), size_m=0.015, rim=0.9, waist=0.5,
                        eye_radius=0.0, centre_radius=0.0)
    W.paint_stem(atlas, "stem", nrng, (0.14, 0.06, 0.05), (0.08, 0.10, 0.04), length_m=0.15)
    atlas.save()
    return atlas


def describe(rng):
    stems = []
    for i in range(13):
        az = rng.uniform(0, math.tau)
        r = 0.13 * rng.random() ** 0.5
        flowering = rng.random() < 0.8
        stems.append(dict(base=Vector((math.cos(az) * r, math.sin(az) * r, 0.0)), length=rng.uniform(0.09, 0.16),
                          lean=rng.uniform(0, math.tau), tilt=rng.uniform(0.05, 0.25), flowering=flowering,
                          bud=flowering and rng.random() < 0.2, whorl=rng.uniform(0.6, 0.72),
                          leaf_spin=rng.uniform(0, math.tau), size=rng.uniform(0.026, 0.034),
                          key=rng.choice(("flower", "flower2")), face_tilt=rng.uniform(0.3, 0.8),
                          phase=rng.random(), keep=rng.random()))
    return dict(stems=stems)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"stems": 0, "leaves": 0, "flowers": 0}
    for st in desc["stems"]:
        if lod == 1 and st["keep"] > 0.75 or lod == 2 and st["keep"] > 0.5:
            continue
        lean = Vector((math.cos(st["lean"]), math.sin(st["lean"]), 0.0))
        n = (5, 3, 2)[lod]
        pts = [st["base"] + (Vector((0, 0, 1)) * t + lean * st["tilt"] * t * t) * st["length"]
               for t in np.linspace(0, 1, n + 1)]
        t0 = b.triangles
        b.tube(pts, [0.0012 * (1 - 0.3 * i / n) for i in range(n + 1)], (4, 3, 3)[lod], atlas.uv("stem"),
               v_length=0.15, phase=st["phase"], flutter=0.3)
        stats["stems"] += b.triangles - t0
        # The whorl: three stalked leaves spreading out level from the stem.
        t0 = b.triangles
        acc = S.arclength(pts)
        node, _, _ = S.at(pts, acc, acc[-1] * st["whorl"])
        for k in range(3):
            a = st["leaf_spin"] + k * math.tau / 3
            h = Vector((math.cos(a), math.sin(a), 0))
            d = (h + Vector((0, 0, 0.25))).normalized()
            b.card(node, d, Vector((0, 0, 1)), 0.045, 0.045 * S.tile_aspect(atlas, "leaf"),
                   atlas.uv("leaf" if k % 2 else "leaf2"), rows=(3, 2, 1)[lod], cols=(2, 1, 1)[lod], fold=0.15,
                   droop=0.25, phase=st["phase"], flutter=0.7, flutter_base=0.2)
        stats["leaves"] += b.triangles - t0
        if not st["flowering"]:
            continue
        t0 = b.triangles
        top = pts[-1]
        if st["bud"]:
            # Young buds hang their heads.
            face = (lean - Vector((0, 0, 0.6))).normalized()
            b.flat(top + face * 0.004, face, 0.013, atlas.uv("bud"), spin=st["leaf_spin"], cup=0.5,
                   phase=st["phase"], flutter=0.5, segs=1)
        else:
            face = (Vector((0, 0, 1)) + lean * st["face_tilt"]).normalized()
            b.flat(top + face * 0.002, face, st["size"], atlas.uv(st["key"]), spin=st["leaf_spin"], cup=0.18,
                   phase=st["phase"], flutter=0.6, segs=2 if lod == 0 else 1)
        stats["flowers"] += b.triangles - t0
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
