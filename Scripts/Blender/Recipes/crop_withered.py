"""Withered crop plots: the dead plants a plot shows after its crop's season ends, four families.

Real-object research (written before modelling):
- Frost or the turn of the season kills an out-of-season crop in place. Root crops (carrots,
  turnips, potatoes, leeks) keep a low collapse of yellow-brown, papery tops lying on the soil,
  with a stub or two still half standing. Brassicas (cabbage, winter broccoli) slump: the big
  leaves go tan and grey, flop outward flat and rot at their bases around a bare stump.
- Grain left standing past its season lodges: grey, bleached straw snapped at the nodes, the tops
  folded over or lying flat, the shattered ears dark and empty.
- Peas and beans die back to a tangle of brown, brittle stems draped over broken, leaning sticks
  and slumped onto the ground, their small leaves shrivelled to curls.
- Every family is one 1 m plot, pivot at the bottom centre of the tilled bed (ridges at y=-0.3,
  0, +0.3; base z=0.045), walk-through. Colours stay a light tan-grey so a dead plot reads at the
  gameplay camera against dark wet soil.

Meshes: SM_CropWithered_Root, _Leafy, _Stalk, _Vine (each with LOD1/LOD2), one shared atlas.
HomesteadWorld draws the family for the plot's crop (see CropWitheredFamily) in place of the stage
mesh and produce.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_crop as C
import homestead_foliage as F
import homestead_shrub as S

NAME = "CropWithered"
DESCRIPTION = ("Four 1 m withered-crop plots (root, leafy, stalk and vine silhouette families) of dead, "
               "collapsed plants for crops that die at the change of season.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1854_101
FAMILIES = ("Root", "Leafy", "Stalk", "Vine")
LODGE_AZ = math.radians(35)  # the prevailing direction lodged straw lies

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"], "eye_distance": 4.0,
          "meshes": {f"SM_CropWithered_{f}": {"focus": (0.0, 0.0, 0.08), "eye_distance": 3.6} for f in FAMILIES}}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plot height",
             "G": "per-plant phase", "B": "a little flutter at dead leaf tips", "A": "1"},
    "material_notes": "One M_CropWithered atlas: alpha in basecolor; roughness R, translucency G, AO B.",
    "families": {"Root": "turnips, carrots, potatoes, leeks, legacy roots",
                 "Leafy": "cabbage, winter broccoli",
                 "Stalk": "wheat, barley",
                 "Vine": "broad beans, peas, strawberries, legacy berries"},
}
UP = C.UP


# ------------------------------------------------------------------ painting

def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=10)
    atlas.column("stem", 48)
    atlas.column("straw", 48)
    atlas.column("bark", 80)
    atlas.column("rot", 64)
    for key in ("broad", "broad_grey"):
        atlas.tile(key, 460, 620)
    for key in ("strap", "strap_grey"):
        atlas.tile(key, 110, 900)
    atlas.tile("frond", 300, 520)
    atlas.tile("curl", 240, 280)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    tan = dict(base=(0.20, 0.14, 0.075), tip=(0.26, 0.19, 0.10), vein=(0.30, 0.24, 0.14),
               margin=(0.12, 0.08, 0.045), yellow=(0.28, 0.22, 0.10), brown=(0.07, 0.045, 0.025),
               dry=(0.24, 0.17, 0.09))
    grey = dict(tan, base=(0.19, 0.17, 0.14), tip=(0.23, 0.21, 0.17), vein=(0.26, 0.24, 0.19),
                margin=(0.11, 0.09, 0.07), dry=(0.21, 0.19, 0.15))
    for key, pal in (("broad", tan), ("broad_grey", grey)):
        X, Y, px = atlas.grid(key)
        a = float(X.max())
        shape = F.ovate(width=a, widest=0.55, tip_sharp=0.5, base_round=0.7, base=0.0, tip=0.99,
                        teeth=14, tooth_depth=0.04, double=0.3)
        veins = F.pinnate_veins(count=8, angle=0.9, curve=0.3, reach=0.85, width=a, widest=0.55,
                                start=0.1, stop=0.85, rng=nrng)
        layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.014, vein_depth=0.0002,
                              puff=0.0002, tertiary=0.6, dry=0.8, damage=0.6, holes=0.35, edge_burn=0.6,
                              stalk=0.03, trans=0.15)
        atlas.put(key, layer, meters_per_px=0.30 / X.shape[0])
    for key, pal in (("strap", tan), ("strap_grey", grey)):
        X, Y, px = atlas.grid(key)
        a = float(X.max())
        shape = F.ovate(width=a * 0.85, widest=0.3, tip_sharp=0.8, base_round=0.2, base=0.0, tip=0.99)
        veins = [([(0, 0), (0, 0.97)], 1.0)] + [([(a * 0.25 * k, 0.0), (a * 0.05 * k, 0.95)], 0.3) for k in (-2, -1, 1, 2)]
        layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.006, puff=0.00005,
                              tertiary=0.0, dry=0.9, edge_burn=0.5, damage=0.3, trans=0.15)
        atlas.put(key, layer, meters_per_px=0.35 / X.shape[0])
    # Dead ferny root-crop top: a stalk with small shrivelled lobes.
    X, Y, px = atlas.grid("frond")
    layer = F.Layer(X.shape)
    rng = random.Random(SEED + 5)
    layer.over(F.paint_blade(X, Y, nrng, F.ovate(width=0.012, widest=0.2, tip_sharp=1.0), [], tan, px,
                             tertiary=0.0, dry=0.9, trans=0.1))
    for k in range(14):
        y0 = 0.12 + 0.8 * k / 13
        side = -1 if k % 2 else 1
        Xl, Yl = S.rotated(X, Y, (0.0, y0), side * rng.uniform(0.7, 1.1), 0.28 * (1.1 - 0.6 * y0))
        lobe = F.ovate(width=0.22, widest=0.4, tip_sharp=0.8, teeth=6, tooth_depth=0.25)
        layer.over(F.paint_blade(Xl, Yl, nrng, lobe, [], tan if k % 3 else grey, px / 0.2, tertiary=0.0,
                                 dry=0.85, holes=0.2, trans=0.1))
    atlas.put("frond", layer, meters_per_px=0.20 / X.shape[0])
    # Shrivelled leaflet, curled into a crumpled crescent.
    X, Y, px = atlas.grid("curl")
    Xw = X + 0.18 * (Y - 0.5) ** 2 * 4 - 0.09
    layer = F.paint_blade(Xw, Y, nrng, F.ovate(width=0.26, widest=0.45, tip_sharp=0.7, teeth=5, tooth_depth=0.08),
                          F.pinnate_veins(count=4, width=0.26, rng=nrng), tan, px, dry=0.9, damage=0.5,
                          holes=0.2, edge_burn=0.6, tertiary=0.3, trans=0.1)
    atlas.put("curl", layer, meters_per_px=0.04 / X.shape[0])
    for key, base, hi in (("stem", (0.15, 0.11, 0.07), (0.24, 0.19, 0.12)),
                          ("straw", (0.20, 0.17, 0.12), (0.29, 0.25, 0.18)),
                          ("bark", (0.07, 0.055, 0.042), (0.13, 0.11, 0.085)),
                          ("rot", (0.05, 0.038, 0.026), (0.11, 0.085, 0.055))):
        U, V = atlas.column_grid(key)
        n = F.noise(U.shape, nrng, freq=70.0, beta=1.8, aniso=(1.0, 6.0))
        layer = F.Layer(U.shape)
        layer.color = F.lerp(base, hi, np.clip(0.25 + 0.6 * n, 0, 1))
        if key == "straw":
            node = np.exp(-(((V / 0.18) % 1.0 - 0.5) / 0.03) ** 2)
            layer.color = F.lerp(layer.color, np.asarray(base) * 0.6, node * 0.7)
        layer.height = 0.00006 * n
        layer.alpha[...] = 1
        layer.rough[...] = 0.85
        layer.trans[...] = 0.05
        atlas.put(key, layer, meters_per_px=0.3 / U.shape[0], wrap=True, opaque=True)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ families

def _flop(b, atlas, key, base, az, length, rng, lod, phase, elev=(4, 18), droop=(0.6, 1.1)):
    """A dead strap or frond lying almost flat on the soil."""
    hd = C.heading(az)
    e = math.radians(rng.uniform(*elev))
    d = (hd * math.cos(e) + UP * math.sin(e)).normalized()
    length *= C.edge_scale(base, hd, length)
    rect = atlas.uv(key)
    if rng.random() < 0.5:
        rect = (rect[1], rect[0], rect[2], rect[3])
    C.strap(b, rect, base, d, length, length * S.tile_aspect(atlas, key), phase, rows=(4, 2, 1)[lod],
            droop=rng.uniform(*droop), twist=rng.uniform(-0.8, 0.8) if lod < 2 else 0.0,
            fold=0.2 if lod == 0 else 0.0, flutter=0.3, flutter_base=0.0, taper=0.2)


def _root(b, atlas, lod):
    rng = random.Random(SEED + 1)
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate((-0.33, -0.11, 0.11, 0.33)):
            base = Vector((x + rng.uniform(-0.02, 0.02), y + rng.uniform(-0.02, 0.02), C.BASE_Z))
            phase = rng.random()
            count = (6, 4, 2)[lod]
            az0 = rng.uniform(0, math.tau)
            for k in range(6):
                az = az0 + k * 2.39996 + rng.uniform(-0.3, 0.3)
                key = rng.choice(["strap", "strap_grey", "frond", "frond"])
                length = rng.uniform(0.12, 0.22)
                upright = k == 0 and rng.random() < 0.6
                if k >= count:
                    continue
                if upright:
                    # A stub still half standing, snapped over at the tip.
                    _flop(b, atlas, key, base, az, length * 0.9, rng, lod, phase, elev=(55, 70), droop=(1.8, 2.4))
                else:
                    _flop(b, atlas, key, base, az, length, rng, lod, phase)
            b.tube([base - UP * 0.01, base + UP * 0.012], [0.009, 0.006], (6, 4, 3)[lod], atlas.uv("rot"),
                   v_length=0.1, phase=phase, cap=True)


def _leafy(b, atlas, lod):
    rng = random.Random(SEED + 2)
    for pidx, (x, y) in enumerate([(-0.20, -0.30), (0.20, 0.0), (-0.20, 0.30)]):
        base = Vector((x, y, C.BASE_Z))
        phase = rng.random()
        b.tube([base - UP * 0.01, base + UP * 0.05, base + UP * 0.075 + C.heading(rng.uniform(0, 6.3)) * 0.01],
               [0.014, 0.012, 0.009], (7, 5, 4)[lod], atlas.uv("rot"), v_length=0.15, phase=phase, cap=True)
        count = (9, 6, 4)[lod]
        az0 = rng.uniform(0, math.tau)
        rows, cols = ((7, 5), (4, 3), (2, 2))[lod]
        for k in range(9):
            az = az0 + k * 2.39996 + rng.uniform(-0.2, 0.2)
            length = rng.uniform(0.20, 0.32)
            key = "broad" if rng.random() < 0.6 else "broad_grey"
            elev = math.radians(rng.uniform(0, 20))
            droop = rng.uniform(0.35, 0.7)
            if k >= count:
                continue
            at = base + UP * rng.uniform(0.02, 0.05) + C.heading(az) * 0.015
            length *= C.edge_scale(at, C.heading(az), length * 0.9)
            C.cupped_leaf(b, atlas.uv(key), at, C.heading(az), length, length * 0.62, phase, rng, elev=elev,
                          droop=droop, cup=0.012, rows=rows, cols=cols, flutter=0.25, wave=0.12 if lod == 0 else 0.0,
                          frill=0.015 if lod == 0 else 0.0, narrow=0.3)
        # Rotted heart leaves slumped over the stump.
        for k in range((3, 2, 1)[lod]):
            az = az0 + k * 2.1 + 0.5
            C.cupped_leaf(b, atlas.uv("broad_grey"), base + UP * 0.07, C.heading(az), 0.10, 0.08, phase, rng,
                          elev=math.radians(35), droop=1.2, cup=0.02, rows=max(2, rows - 2), cols=max(2, cols - 1),
                          flutter=0.1, narrow=0.5)


def _kinked(b, atlas, base, az, rng, lod, phase):
    """A lodged grain stem: upright to a node, snapped over there, the top hanging or lying."""
    hd = C.heading(az)
    h1 = rng.uniform(0.06, 0.30) if rng.random() < 0.75 else rng.uniform(0.30, 0.40)
    lean = rng.uniform(0.1, 0.5)
    kink = base + hd * h1 * lean + UP * h1
    fold = math.radians(rng.uniform(95, 160))
    d2 = (hd * math.sin(fold) + UP * math.cos(fold)).normalized()
    l2 = rng.uniform(0.18, 0.40)
    tip = kink + d2 * l2
    if tip.z < C.BASE_Z + 0.01:
        tip.z = C.BASE_Z + 0.01 + rng.uniform(0, 0.02)
    for axis in (0, 1):
        lim = C.EDGE
        tip[axis] = max(-lim, min(lim, tip[axis]))
    pts = [base, base.lerp(kink, 0.5), kink, kink.lerp(tip, 0.5) - UP * 0.02, tip]
    if lod == 2:
        pts = [base, kink, tip]
    b.tube(pts, [0.0032] * len(pts), 3, atlas.uv("straw"), v_length=0.35, phase=phase, flutter=0.02)
    if lod == 0 and rng.random() < 0.5:
        # The empty, dark ear remnant.
        b.tube([tip, tip + (tip - kink).normalized() * 0.06], [0.004, 0.002], 4, atlas.uv("rot"),
               v_length=0.1, phase=phase, cap=True)
    if lod < 2 and rng.random() < 0.7:
        _flop(b, atlas, "strap_grey", base.lerp(kink, rng.uniform(0.3, 0.8)), az + rng.uniform(1.5, 4.5),
              rng.uniform(0.12, 0.2), rng, lod, phase, elev=(-10, 20), droop=(0.9, 1.5))


def _stalk(b, atlas, lod):
    rng = random.Random(SEED + 3)
    xs = np.linspace(-0.40, 0.40, 9)
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate(xs):
            base = Vector((x + rng.uniform(-0.02, 0.02), y + rng.uniform(-0.03, 0.03), C.BASE_Z))
            phase = rng.random()
            az0 = rng.uniform(0, math.tau)
            for k in range(3):
                if lod == 2 and k:
                    continue
                if lod == 1 and k == 2:
                    continue
                # Grain lodges mostly one way, laid over by the same wind and rain.
                lodge = LODGE_AZ + rng.uniform(-0.7, 0.7) if rng.random() < 0.8 else rng.uniform(0, math.tau)
                _kinked(b, atlas, base + C.heading(az0 + k * 2.1) * 0.008, lodge, rng, lod, phase)
            if lod < 2:
                _flop(b, atlas, "strap", base, az0 + 1.0, rng.uniform(0.10, 0.16), rng, lod, phase)


def _vine(b, atlas, lod):
    rng = random.Random(SEED + 4)
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate((-0.22, 0.22)):
            base = Vector((x + rng.uniform(-0.02, 0.02), y + rng.uniform(-0.02, 0.02), C.BASE_Z - 0.02))
            phase = rng.random()
            hd = C.heading(rng.uniform(0, math.tau))
            has_stick = (i + j) % 2 == 0
            top = base + UP * rng.uniform(0.40, 0.58) + hd * rng.uniform(0.08, 0.16)
            if has_stick:
                # A leaning, broken pea-stick or bean pole.
                b.tube([base, base.lerp(top, 0.5), top], [0.007, 0.0055, 0.004], (5, 4, 3)[lod], atlas.uv("bark"),
                       v_length=0.35, phase=phase, cap=True)
                for k in range((3, 2, 0)[lod]):
                    at = base.lerp(top, rng.uniform(0.4, 0.95))
                    tw = C.arc(at, C.heading(rng.uniform(0, math.tau)), math.radians(rng.uniform(20, 50)),
                               rng.uniform(0.06, 0.14), 0.1, 1)
                    b.tube(tw, [0.0018, 0.0008], 3, atlas.uv("bark"), v_length=0.35, phase=phase)
            # Dead vines: up the stick (or a short sprawl) and slumped back over onto the soil.
            for v in range((4, 2, 1)[lod]):
                az = rng.uniform(0, math.tau)
                h = top.z - base.z if has_stick else rng.uniform(0.10, 0.18)
                crest = base + UP * (h * rng.uniform(0.6, 0.95)) + C.heading(az) * 0.03
                land = base + C.heading(az) * rng.uniform(0.18, 0.30)
                land.z = C.BASE_Z + 0.005
                for axis in (0, 1):
                    land[axis] = max(-C.EDGE, min(C.EDGE, land[axis]))
                pts = [base + UP * 0.02, base.lerp(crest, 0.5) + C.heading(az + 1.2) * 0.02, crest,
                       crest.lerp(land, 0.5) + UP * 0.03, land]
                if lod == 2:
                    pts = [pts[0], pts[2], pts[4]]
                b.tube(pts, [0.0022, 0.0019, 0.0016, 0.0013, 0.0010][:len(pts)], 3, atlas.uv("stem"), v_length=0.3,
                       phase=phase, flutter=0.05)
                # Shrivelled leaflets hanging along it.
                for k in range((9, 4, 1)[lod]):
                    seg = rng.uniform(0.1, 0.95) * (len(pts) - 1)
                    s = min(int(seg), len(pts) - 2)
                    at = pts[s].lerp(pts[s + 1], seg - s)
                    d = (C.heading(rng.uniform(0, math.tau)) + UP * rng.uniform(-1.0, 0.2)).normalized()
                    ln = rng.uniform(0.04, 0.065)
                    b.card(at, d, UP, ln, ln * S.tile_aspect(atlas, "curl"), atlas.uv("curl"), rows=1, cols=1,
                           fold=0.0, droop=0.3, phase=phase, flutter=0.3)
            # Fallen leaf litter under the plant.
            for k in range((5, 3, 1)[lod]):
                at = base + C.heading(rng.uniform(0, math.tau)) * rng.uniform(0.02, 0.16)
                at.z = C.BASE_Z + 0.004
                b.flat(at, (UP + C.heading(rng.uniform(0, 6.3)) * 0.3).normalized(), rng.uniform(0.03, 0.05),
                       atlas.uv("curl"), spin=rng.uniform(0, math.tau), cup=0.1, phase=phase, flutter=0.05, segs=1)


def emit(family, lod, atlas):
    b = F.Batch(height=0.6)
    {"Root": _root, "Leafy": _leafy, "Stalk": _stalk, "Vine": _vine}[family](b, atlas, lod)
    print("HOMESTEAD_TRIS", family, lod, b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(0.6, 0.5, 0.3))
    return C.stage_meshes(kit, NAME, material, lambda family, lod: emit(family, lod, atlas), stages=FAMILIES)
