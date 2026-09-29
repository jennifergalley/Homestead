"""Garden pea crop plot (Pisum sativum, tall marrowfat type on hazel pea-sticks), five stages.

Real plant research / art targets (written before modelling):
- Victorian kitchen gardens grew tall marrowfat and "Champion of England" peas up brushy hazel
  pea-sticks pushed in beside the rows as the seedlings came up; the twiggy tops (about 1 m) give
  the tendrils something to catch.
- Pea plants are glaucous (waxy, blue-grey-green) climbers with thin weak stems. At each node:
  a pair of large leafy stipules clasping the stem (bigger than the leaflets, faintly mottled),
  a leaf of 1-3 pairs of smooth oval leaflets, and branched tendrils at the leaf tip.
- Flowers come from the upper axils in pairs: white, sweet-pea shaped (standard, wings, keel).
  Pods are 7-10 cm, green, slightly curved, laterally flattened, with a short beak; they swell
  with the peas and are picked green, so a picked plant flowers and pods again.
- Homestead crop plot: six pea-sticks at x=-0.22,+0.22 on the ridge tops y=-0.3,0,+0.3, two vines
  sown at each; base z=0.045. Everything stays inside the 1 m square and is walk-through.

The pods are separate instanced produce (SM_CropPea_Produce, hanging -Z from the stalk top) on the
report anchors, so the game swells and greens them; the plant meshes carry no pods.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_crop as C
import homestead_foliage as F
import homestead_shrub as S

NAME = "CropPea"
DESCRIPTION = ("One 1 m tilled-bed plot of tall garden peas climbing six brushy hazel pea-sticks, "
               "five crop stages; the pods are separate instanced produce.")
COLLISION = "none"
TRIANGLE_BUDGET = 7500
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1850_617

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 4.5,
          "meshes": {
              "SM_CropPea_Sprout": {"focus": (0.0, 0.0, 0.30), "eye_distance": 3.6},
              "SM_CropPea_Growing": {"focus": (0.0, 0.0, 0.40), "eye_distance": 4.0},
              "SM_CropPea_Ripe": {"focus": (0.0, 0.0, 0.48), "eye_distance": 4.6},
              "SM_CropPea_Produce": {"focus": (0.0, 0.0, -0.05), "eye_distance": 0.6,
                                     "detail_distance": 0.25, "detail_fstop": 32.0},
              "SM_CropPea_Harvest": {"focus": (0.0, 0.0, -0.06), "eye_distance": 0.9},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / crop height",
             "G": "per-plant phase", "B": "leaf flutter; the pea-sticks are nearly still", "A": "1"},
    "crop_layout": "Six hazel pea-sticks at x=-0.22,+0.22 on ridges y=-0.3,0,+0.3, two vines each; base z=0.045 m.",
    "material_notes": "One M_CropPea atlas: alpha in basecolor; roughness R, translucency G, AO B. "
                      "Pods use M_CropPeaProduce (re-parented to M_CropProduce in Unreal).",
    "harvest_mesh": "SM_CropPea_Harvest: three pods on their stalks, grip at the origin, hanging -Z.",
}

XS = (-0.22, 0.22)
STICK_H = 0.98
UP = C.UP
STAGES = {
    "Sprout": dict(height=0.07, nodes=2, flowers=0, yellow=0),
    "Young": dict(height=0.22, nodes=4, flowers=0, yellow=0),
    "Growing": dict(height=0.52, nodes=7, flowers=0, yellow=0),
    "Mature": dict(height=0.80, nodes=10, flowers=3, yellow=1),
    "Ripe": dict(height=0.88, nodes=10, flowers=1, yellow=3),
}


def _sticks():
    out = []
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate(XS):
            rng = random.Random(SEED + j * 31 + i * 7)
            out.append(dict(pos=Vector((x + rng.uniform(-0.02, 0.02), y + rng.uniform(-0.015, 0.015), C.BASE_Z - 0.03)),
                            lean=C.heading(rng.uniform(0, math.tau)) * rng.uniform(0.02, 0.05),
                            az=rng.uniform(0, math.tau), phase=rng.random(), seed=rng.randrange(1 << 20),
                            idx=j * 10 + i))
    return out


def _stick_axis(stick, z):
    s = max(0.0, min(1.0, (z - stick["pos"].z) / STICK_H))
    return stick["pos"] + stick["lean"] * s + UP * (STICK_H * s)


# ------------------------------------------------------------------ painting

def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=10)
    atlas.column("stem", 48)
    atlas.column("bark", 96)
    atlas.column("pod", 128)
    for key in ("leaflet", "leaflet_blue", "leaflet_yellow"):
        atlas.tile(key, 300, 440)
    atlas.tile("stipule", 380, 380)
    atlas.tile("stipule_yellow", 380, 380)
    atlas.tile("flower", 240, 240)
    atlas.tile("calyx", 160, 160)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    pal = dict(base=(0.070, 0.115, 0.075), tip=(0.085, 0.130, 0.090), vein=(0.125, 0.165, 0.120),
               margin=(0.060, 0.098, 0.066), yellow=(0.20, 0.18, 0.06), brown=(0.08, 0.05, 0.02))
    blue = dict(pal, base=(0.068, 0.110, 0.090), tip=(0.080, 0.125, 0.105))
    for key, p, extra in (("leaflet", pal, {}), ("leaflet_blue", blue, {}),
                          ("leaflet_yellow", pal, dict(yellow=0.65, edge_burn=0.3, damage=0.2))):
        X, Y, px = atlas.grid(key)
        shape = F.ovate(width=0.30, widest=0.46, tip_sharp=0.55, base_round=0.75, base=0.03, tip=0.97)
        veins = F.pinnate_veins(count=6, angle=0.8, curve=0.3, reach=0.78, width=0.30, widest=0.46,
                                start=0.10, stop=0.84, rng=nrng)
        layer = F.paint_blade(X, Y, nrng, shape, veins, p, px, vein_width=0.006, vein_depth=0.00012,
                              puff=0.00015, tertiary=0.25, gloss=-0.05, trans=0.55, **extra)
        # Waxy bloom: a faint pale-blue sheen blotched over the blade.
        bloom = F.smoothstep(0.45, 0.8, F.noise(X.shape, nrng, freq=6.0, beta=2.0))
        layer.color = F.lerp(layer.color, (0.105, 0.145, 0.130), bloom * 0.30 * layer.alpha)
        atlas.put(key, layer, meters_per_px=0.035 / X.shape[0])
    for key, extra in (("stipule", {}), ("stipule_yellow", dict(yellow=0.55, edge_burn=0.35))):
        X, Y, px = atlas.grid(key)
        # Broad, heart-based stipule, toothed near its base, with the pale-grey mottle peas show.
        shape = F.ovate(width=0.42, widest=0.36, tip_sharp=0.7, base_round=0.5, base=0.03, tip=0.96,
                        cordate=0.15, teeth=5, tooth_depth=0.05)
        veins = F.pinnate_veins(count=5, angle=0.9, curve=0.4, reach=0.8, width=0.42, widest=0.36,
                                start=0.08, stop=0.8, rng=nrng)
        layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.006, vein_depth=0.0001,
                              puff=0.00012, tertiary=0.2, trans=0.5, **extra)
        mottle = F.smoothstep(0.62, 0.78, F.noise(X.shape, nrng, freq=14.0, beta=1.8))
        layer.color = F.lerp(layer.color, (0.16, 0.19, 0.17), mottle * 0.45)
        atlas.put(key, layer, meters_per_px=0.045 / X.shape[0])
    # White pea flower seen from the front: round standard over two wings and the keel.
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    white = dict(base=(0.66, 0.66, 0.60), tip=(0.78, 0.78, 0.73), vein=(0.62, 0.64, 0.58), margin=(0.80, 0.80, 0.76))
    for cx, cy, sx, sy in ((0.0, 0.52, 0.44, 0.44), (-0.17, 0.30, 0.22, 0.26), (0.17, 0.30, 0.22, 0.26)):
        petal = F.ovate(width=0.46, widest=0.55, tip_sharp=0.5, base_round=1.0, base=0.0, tip=1.0)
        Xl, Yl = (X - cx) / sx, (Y - cy) / sy + 0.5
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, [([(0, 0.05), (0, 0.9)], 1.0)], white, px / sx,
                                 vein_width=0.02, puff=0.0001, tertiary=0.0, trans=0.85, gloss=-0.1))
    keel = np.exp(-((X / 0.05) ** 2 + ((Y - 0.18) / 0.10) ** 2))
    layer.color = F.lerp(layer.color, (0.40, 0.46, 0.30), keel * 0.5)
    atlas.put("flower", layer, meters_per_px=0.025 / X.shape[0])
    X, Y, px = atlas.grid("calyx")
    calyx = F.Layer(X.shape)
    for k in range(5):
        th = math.tau * k / 5
        c, s = math.cos(th), math.sin(th)
        Xl, Yl = (X * c - (Y - 0.5) * s) / 0.5, (X * s + (Y - 0.5) * c) / 0.5
        calyx.over(F.paint_blade(Xl, Yl, nrng, F.ovate(width=0.18, widest=0.3, tip_sharp=1.2), [], pal, px / 0.5,
                                 tertiary=0.0, trans=0.4))
    atlas.put("calyx", calyx, meters_per_px=0.015 / X.shape[0])
    # Columns: glaucous stem, hazel bark, pod skin.
    for key, base, hi, freq, aniso in (("stem", (0.075, 0.115, 0.080), (0.115, 0.150, 0.115), 90.0, 6.0),
                                       ("bark", (0.062, 0.046, 0.034), (0.115, 0.090, 0.066), 60.0, 3.0),
                                       ("pod", (0.090, 0.180, 0.060), (0.160, 0.260, 0.090), 30.0, 1.5)):
        U, V = atlas.column_grid(key)
        n = F.noise(U.shape, nrng, freq=freq, beta=1.8, aniso=(1.0, aniso))
        layer = F.Layer(U.shape)
        layer.color = F.lerp(base, hi, np.clip(0.3 + 0.5 * n, 0, 1))
        layer.height = 0.00006 * n
        if key == "bark":
            lent = F.smoothstep(0.82, 0.9, F.noise(U.shape, nrng, freq=120.0, beta=1.2, aniso=(3.0, 1.0)))
            layer.color = F.lerp(layer.color, (0.20, 0.18, 0.14), lent * 0.8)
            grey = F.smoothstep(0.4, 0.8, F.noise(U.shape, nrng, freq=10.0, beta=2.0))
            layer.color = F.lerp(layer.color, (0.15, 0.15, 0.125), grey * 0.45)
            layer.height += 0.00015 * lent
            layer.rough[...] = 0.8
        elif key == "pod":
            peas = F.smoothstep(0.55, 0.95, np.sin(V * math.tau * 3.5) * 0.5 + 0.5) * \
                (1 - F.smoothstep(0.2, 0.45, np.abs(U - 0.5)))
            suture = np.exp(-((U - 0.0) / 0.02) ** 2) + np.exp(-((U - 1.0) / 0.02) ** 2)
            layer.color = F.lerp(layer.color, (0.14, 0.25, 0.08), peas * 0.35)
            layer.color = F.lerp(layer.color, (0.06, 0.12, 0.04), suture * 0.6)
            layer.height += 0.00035 * peas
            layer.rough[...] = 0.45
            layer.trans[...] = 0.15
        else:
            layer.rough[...] = 0.55
            layer.trans[...] = 0.3
        layer.alpha[...] = 1
        atlas.put(key, layer, meters_per_px=0.3 / U.shape[0], wrap=True, opaque=True)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ geometry

def _emit_stick(b, atlas, stick, lod):
    """A brushy hazel pea-stick: a leaning main stem forking into twiggy side shoots."""
    rng = random.Random(stick["seed"])
    base = stick["pos"]
    main = [_stick_axis(stick, base.z + STICK_H * s) for s in (0.0, 0.3, 0.6, 0.85, 1.0)]
    if lod == 2:
        main = [main[0], main[2], main[4]]
    b.tube(main, [0.0070, 0.0058, 0.0045, 0.0032, 0.0018][:len(main)] if lod < 2 else [0.007, 0.0045, 0.0018],
           (5, 4, 3)[lod], atlas.uv("bark"), v_length=0.35, phase=stick["phase"], flutter=0.0, cap=True)
    branches = (9, 5, 2)[lod]
    for k in range(9):
        t = 0.24 + 0.70 * k / 8 + rng.uniform(-0.04, 0.04)
        az = stick["az"] + k * 2.39996 + rng.uniform(-0.3, 0.3)
        length = rng.uniform(0.14, 0.30) * (1.1 - 0.4 * t)
        elev = math.radians(rng.uniform(40, 68))
        if k >= branches:
            continue
        node = _stick_axis(stick, base.z + STICK_H * t)
        hd = C.heading(az)
        length *= C.edge_scale(node, hd, length * 0.7)
        pts = C.arc(node, hd, elev, length, 0.15, (2, 2, 1)[lod])
        b.tube(pts, [0.0024, 0.0015, 0.0008][:len(pts)], 3, atlas.uv("bark"), v_length=0.35,
               phase=stick["phase"], flutter=0.02)
        if lod == 0 or (lod == 1 and t > 0.6):
            # Twiggy brush: the fine tips fork again, thickest near the top of the stick.
            for f in ((-1, 1) if t > 0.5 else (1,)):
                tw = Matrix.Rotation(f * rng.uniform(0.35, 0.7), 3, UP) @ hd
                tip = C.arc(pts[1], tw, elev + 0.25, length * rng.uniform(0.35, 0.55), 0.1, 1)
                b.tube(tip, [0.0010, 0.0004], 3, atlas.uv("bark"), v_length=0.35, phase=stick["phase"], flutter=0.03)


def _vine_path(stick, v, stage):
    """Points up one vine, twining loosely round its pea-stick, stable across LODs."""
    cfg = STAGES[stage]
    rng = C.plant_rng(SEED, "vine", stick["idx"], v)
    H = cfg["height"] * rng.uniform(0.92, 1.04)
    az0 = stick["az"] + math.pi * v + rng.uniform(-0.4, 0.4)
    n = max(3, cfg["nodes"] + 1)
    base = stick["pos"] + C.heading(az0) * 0.035 + UP * (C.BASE_Z - stick["pos"].z + 0.002)
    pts = [base]
    for k in range(1, n):
        s = k / (n - 1)
        z = C.BASE_Z + H * s
        a = az0 + s * H * 5.0 + rng.uniform(-0.15, 0.15)
        r = 0.030 + 0.065 * s + rng.uniform(-0.008, 0.008)
        pts.append(_stick_axis(stick, z) + C.heading(a) * r + UP * 0.0)
    return pts, rng.random(), rng.randrange(1 << 20)


def _leaf(b, atlas, node, az, phase, lod, rng, stage, scale, yellow=False, tendril=True):
    hd = C.heading(az)
    key_leaf = "leaflet_yellow" if yellow else rng.choice(["leaflet", "leaflet", "leaflet_blue"])
    key_stip = "stipule_yellow" if yellow else "stipule"
    # Stipule pair clasping the node.
    for side in (-1, 1):
        d = (Matrix.Rotation(side * 0.9, 3, UP) @ hd + UP * 0.35).normalized()
        ln = 0.050 * scale * rng.uniform(0.85, 1.1)
        rect = atlas.uv(key_stip)
        if side < 0:
            rect = (rect[1], rect[0], rect[2], rect[3])
        b.card(node - d * ln * 0.25, d, (UP - hd * 0.3).normalized(), ln, ln * S.tile_aspect(atlas, key_stip),
               rect, rows=1, cols=1, fold=0.0, droop=0.1 + 0.3 * yellow, phase=phase, flutter=0.6, flutter_base=0.1)
    if lod == 2:
        return
    # The leaf: a short petiole, one or two pairs of leaflets, then the tendrils.
    elev = math.radians(rng.uniform(15, 40))
    length = 0.075 * scale
    pts = C.arc(node, hd, elev, length, 0.3 + 0.4 * yellow, 2)
    b.tube(pts, [0.0011, 0.0008, 0.0005], 3, atlas.uv("stem"), v_length=0.3,
           phase=phase, flutter=0.3)
    pairs = 2 if (lod == 0 and scale > 0.7) else 1
    side_v = hd.cross(UP).normalized()
    for p in range(pairs):
        t = 0.45 + 0.4 * p
        seg = t * (len(pts) - 1)
        i = min(int(seg), len(pts) - 2)
        at = pts[i].lerp(pts[i + 1], seg - i)
        for side in (-1, 1):
            d = (hd * 0.45 + side_v * side * 0.9 + UP * 0.15).normalized()
            ln = 0.050 * scale * rng.uniform(0.85, 1.1)
            rect = atlas.uv(key_leaf)
            if side < 0:
                rect = (rect[1], rect[0], rect[2], rect[3])
            b.card(at, d, (UP * 0.8 + hd * 0.4).normalized(), ln, ln * S.tile_aspect(atlas, key_leaf), rect,
                   rows=2 if lod == 0 else 1, cols=1, fold=0.006 if lod == 0 else 0.0,
                   droop=rng.uniform(0.05, 0.2) + 0.3 * yellow, twist=rng.uniform(-0.2, 0.2),
                   phase=phase, flutter=0.95, flutter_base=0.3)
    if lod == 0 and stage != "Sprout" and tendril:
        # A tendril reaching for the twigs, curling at its tip.
        curl = C.arc(pts[-1], hd, math.radians(40), 0.040 * scale, -1.6, 3)
        b.tube(curl, [0.0005, 0.0004, 0.0003, 0.0002], 3, atlas.uv("stem"), v_length=0.3, phase=phase, flutter=0.5)


def _flower(b, atlas, node, az, phase, lod, rng):
    hd = C.heading(az)
    ped = C.arc(node, hd, math.radians(55), 0.04, 0.4, 2 if lod == 0 else 1)
    b.tube(ped, [0.0006] * len(ped), 3, atlas.uv("stem"), v_length=0.3, phase=phase, flutter=0.35)
    for k in range(2 if lod == 0 else 1):
        d = (Matrix.Rotation((k - 0.5) * 0.8, 3, UP) @ hd + UP * 0.2).normalized()
        at = ped[-1] + d * 0.008
        b.flat(at, (d + UP * 0.3).normalized(), rng.uniform(0.024, 0.030), atlas.uv("flower"),
               spin=rng.uniform(-0.3, 0.3), cup=0.10 if lod == 0 else 0.0, phase=phase, flutter=0.45,
               segs=2 if lod == 0 else 1)


def _node_at(pts, t):
    seg = t * (len(pts) - 1)
    i = min(int(seg), len(pts) - 2)
    return pts[i].lerp(pts[i + 1], seg - i)


def emit(stage, lod, atlas):
    cfg = STAGES[stage]
    b = F.Batch(height=STICK_H)
    for stick in _sticks():
        _emit_stick(b, atlas, stick, lod)
        for v in range(2):
            pts, phase, seed = _vine_path(stick, v, stage)
            rng = random.Random(seed)
            draw = pts if lod == 0 else pts[::2] + ([pts[-1]] if (len(pts) - 1) % 2 else [])
            radii = [0.0024 * (1 - 0.45 * i / (len(draw) - 1)) for i in range(len(draw))]
            b.tube(draw, radii, (4, 3, 3)[lod], atlas.uv("stem"), v_length=0.3, phase=phase, flutter=0.15, cap=True)
            nodes = cfg["nodes"]
            for k in range(nodes):
                t = 0.16 + 0.82 * k / max(nodes - 1, 1) if nodes > 1 else 0.9
                az = stick["az"] + math.pi * v + k * 2.2 + rng.uniform(-0.4, 0.4)
                yellow = k < cfg["yellow"]
                scale = (0.55 if stage == "Sprout" else 0.8 if stage == "Young" else 1.0) * (0.8 + 0.3 * t)
                skip = (lod == 1 and k % 2 == 1 and k != nodes - 1) or (lod == 2 and k % 3 != 0)
                if not skip:
                    _leaf(b, atlas, _node_at(pts, t), az, phase, lod, rng, stage, scale, yellow, tendril=k % 2 == 1)
                else:
                    rng.random()
            for f in range(cfg["flowers"]):
                t = 0.72 + 0.22 * f / max(cfg["flowers"], 1)
                if lod == 2 and f:
                    continue
                _flower(b, atlas, _node_at(pts, t), stick["az"] + math.pi * v + f * 2.4 + 1.0, phase, lod, rng)
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _anchors():
    rows = {}
    for stage in C.PRODUCE_STAGES:
        out = []
        for stick in _sticks():
            for v in range(2):
                pts, _, seed = _vine_path(stick, v, stage)
                rng = random.Random(seed + 4409)
                for slot, t in enumerate((0.42, 0.58, 0.74)):
                    az = stick["az"] + math.pi * v + slot * 2.1 + rng.uniform(-0.3, 0.3)
                    at = _node_at(pts, t) + C.heading(az) * 0.012
                    out.append((at, math.degrees(az - math.pi * 0.5), rng.uniform(0.88, 1.12)))
        rows[stage] = out
    return C.anchors_report(f"SM_{NAME}_Produce", rows)


def _pod(b, atlas, top, axis, side, length, radius, phase, rings=10, sides=10):
    """A pea pod hanging from ``top`` along ``axis``: flattened, gently curved, with a short beak."""
    pts, radii = [], []
    for i in range(rings):
        t = i / (rings - 1)
        bow = side * (0.010 * math.sin(t * math.pi))
        pts.append(top + axis * (length * t) + bow + side * (0.006 * t * t))
        env = math.sin(math.pi * (0.06 + 0.88 * t)) ** 0.28
        lump = 1.0 + 0.10 * max(math.exp(-((t - c) / 0.07) ** 2) for c in (0.2, 0.34, 0.48, 0.62, 0.76))
        radii.append(radius * env * lump)
    b.tube(pts, radii, sides, atlas.uv("pod"), v_length=length, phase=phase, flutter=0.0, cap=True,
           flatten=0.62, roll=0.0)
    return pts


def emit_produce(atlas):
    b = F.Batch(height=0.12)
    stalk = [Vector((0, 0, 0)), Vector((0, 0.006, -0.010)), Vector((0, 0.010, -0.018))]
    b.tube(stalk, [0.0010, 0.0008, 0.0007], 5, atlas.uv("stem"), v_length=0.3, phase=0.0, flutter=0.0)
    b.flat(stalk[-1] + Vector((0, 0.002, -0.003)), Vector((0, 0.4, 1.0)).normalized(), 0.016,
           atlas.uv("calyx"), spin=0.3, cup=-0.15, phase=0.0, flutter=0.0, segs=2)
    axis = Vector((0, 0.30, -1.0)).normalized()
    side = axis.cross(Vector((1, 0, 0))).normalized()
    _pod(b, atlas, stalk[-1], axis, side, 0.085, 0.0068, 0.0, rings=12, sides=12)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(atlas):
    b = F.Batch(height=0.2)
    rng = random.Random(SEED + 9001)
    for k, az in enumerate((-0.6, 0.15, 0.9)):
        hd = Vector((math.sin(az) * 0.5, math.cos(az) * 0.5, -1.0)).normalized()
        stalk_end = hd * rng.uniform(0.018, 0.028)
        b.tube([Vector((0, 0, 0)), stalk_end], [0.0011, 0.0008], 5, atlas.uv("stem"), v_length=0.3, phase=0.1)
        side = hd.cross(Vector((0, 0, 1))).normalized()
        _pod(b, atlas, stalk_end, hd, side, rng.uniform(0.075, 0.092), rng.uniform(0.0062, 0.0072), 0.1)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    objs = C.stage_meshes(kit, NAME, material, lambda stage, lod: emit(stage, lod, atlas))
    REPORT["produce"] = _anchors()
    produce_material = material.copy()
    produce_material.name = f"M_{NAME}Produce"
    objs.append(C.single(kit, emit_produce(atlas), f"SM_{NAME}_Produce", produce_material, still_wind=True))
    objs.append(C.single(kit, emit_harvest(atlas), f"SM_{NAME}_Harvest", material, still_wind=True))
    return objs
