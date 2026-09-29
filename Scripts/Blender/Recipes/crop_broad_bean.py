"""Garden broad bean crop plot (Vicia faba, Windsor/Longpod type), five growth stages.

Real plant research / art targets (written before modelling):
- Broad beans were a dependable cottage-garden pulse in nineteenth-century Britain; Windsor and
  Longpod forms are upright, hollow/square-stemmed annuals, commonly 60-90 cm tall, with thick
  glaucous grey-green foliage.
- Leaves are alternate and pinnate, without tendrils: 2-6 broad oval, smooth-edged leaflets on a
  short rachis. The lower leaves weather, tear and grey as the plant carries a pod load.
- Flowers form in leaf axils in short clusters: white standards/wings with a conspicuous black or
  purple-black blotch. Pods are large, green, slightly downy, 12-18 cm long and about 2 cm thick,
  hanging or angled from the axils with visible bean lumps.
- Homestead crop plot: six plants (2 per ridge) on the tilled-bed ridge tops y=-0.3, 0, +0.3, base
  z=0.045. Everything remains inside the 1 m square and is walk-through.

One numpy-painted 2K atlas/material shared by all five stages. Alpha-masked two-sided foliage with
Wind vertex colours from homestead_foliage.py. No collision.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F

NAME = "CropBroadBean"
DESCRIPTION = ("One 1 m tilled-bed plot of six Windsor/Longpod broad bean plants across five crop "
               "stages, with glaucous pinnate leaves, black-blotched flowers and large readable pods.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1850_441

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 4.5,
          "meshes": {
              "SM_CropBroadBean_Growing": {"focus": (0.0, 0.0, 0.32), "eye_distance": 4.0},
              "SM_CropBroadBean_Ripe": {"focus": (0.02, 0.0, 0.44), "eye_distance": 4.8},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / crop height",
             "G": "per-plant phase", "B": "leaf flutter, low on stems/pods", "A": "1"},
    "crop_layout": "Six plants at x=-0.22,+0.22 on tilled-bed ridges y=-0.3,0,+0.3; base z=0.045 m.",
    "material_notes": "One M_CropBroadBean atlas: alpha in basecolor; roughness R, translucency G, AO B.",
}
NOTES = {
    "SM_CropBroadBean_Ripe": "Pick-and-regrow stage: 70-80 cm plants, fat 12-18 cm green pods with lumpy geometry.",
    "SM_CropBroadBean_Mature": "White axillary flowers carry dark blotches plus young green pods.",
    "SM_CropBroadBean_Harvest": "Hand-held produce: small bunch of three pods, grip at stalk top/origin, pods hang down -Z.",
}

RIDGES = (-0.30, 0.0, 0.30)
XS = (-0.22, 0.22)
BASE_Z = 0.045
STAGES = {
    "Sprout": dict(height=0.105, plant_height=0.060, leaves=1, side=0, pods=0, flowers=0),
    "Young": dict(height=0.225, plant_height=0.180, leaves=4, side=0, pods=0, flowers=0),
    "Growing": dict(height=0.535, plant_height=0.490, leaves=8, side=2, pods=0, flowers=0),
    "Mature": dict(height=0.755, plant_height=0.710, leaves=6, side=1, pods=1, flowers=2),
    "Ripe": dict(height=0.825, plant_height=0.780, leaves=6, side=1, pods=4, flowers=1),
}


def _plants():
    out = []
    for j, y in enumerate(RIDGES):
        for i, x in enumerate(XS):
            rng = random.Random(SEED + j * 17 + i * 101)
            out.append(dict(pos=Vector((x + rng.uniform(-0.015, 0.015), y + rng.uniform(-0.010, 0.010), BASE_Z)),
                            heading=rng.uniform(0, math.tau), phase=rng.random(), seed=rng.randrange(1 << 20)))
    return out


def _palettes():
    return {
        "leaf": dict(base=(0.055, 0.085, 0.052), tip=(0.075, 0.105, 0.067),
                     vein=(0.115, 0.145, 0.096), margin=(0.045, 0.071, 0.048),
                     yellow=(0.16, 0.14, 0.055), brown=(0.065, 0.040, 0.024)),
        "leaf_pale": dict(base=(0.070, 0.105, 0.068), tip=(0.088, 0.123, 0.078),
                          vein=(0.130, 0.160, 0.110), margin=(0.060, 0.090, 0.060)),
        "tatty": dict(base=(0.060, 0.077, 0.046), tip=(0.082, 0.083, 0.044),
                      vein=(0.120, 0.120, 0.074), margin=(0.050, 0.053, 0.034),
                      yellow=(0.15, 0.12, 0.045), brown=(0.055, 0.032, 0.018), dry=(0.13, 0.08, 0.035)),
    }


def _leaf_shape(rng, tatty=False):
    return F.ovate(width=0.30 if not tatty else 0.29, widest=0.52, tip_sharp=0.70, base_round=0.85,
                   base=0.025, tip=0.97, teeth=0, phase=rng.uniform(0, 1))


def _leaf_layer(X, Y, px, nrng, rng, key, pals):
    tatty = key == "leaf_tatty"
    veins = F.pinnate_veins(count=7, angle=0.62, curve=0.18, reach=0.74, width=0.30,
                            widest=0.52, start=0.12, stop=0.86, rng=nrng)
    pal = pals["tatty" if tatty else ("leaf_pale" if key == "leaf_pale" else "leaf")]
    return F.paint_blade(X, Y, nrng, _leaf_shape(rng, tatty), veins, pal, px,
                         vein_width=0.0075, vein_depth=0.00016, puff=0.00018, tertiary=0.18,
                         damage=0.20 if tatty else 0.02, holes=0.10 if tatty else 0.0,
                         yellow=0.30 if tatty else 0.05, edge_burn=0.22 if tatty else 0.02,
                         gloss=0.05, hair=0.05, trans=0.50 if not tatty else 0.32)


def _paint_flower(atlas, nrng, rng):
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    petal = F.ovate(width=0.46, widest=0.58, tip_sharp=0.42, base_round=1.1, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(72 * k + (12 if k in (1, 4) else 0) + rng.uniform(-5, 5))
        sc = 0.52 if k == 0 else 0.44
        dx, dy = X, Y - 0.50
        c, s = math.cos(th), math.sin(th)
        Xl = (dx * c - dy * s) / sc
        Yl = (dx * s + dy * c) / sc + 0.05
        pal = dict(base=(0.58, 0.56, 0.48), tip=(0.72, 0.71, 0.64), vein=(0.62, 0.61, 0.54),
                   margin=(0.76, 0.74, 0.68))
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, [([(0, 0.05), (0, 0.95)], 1.0)], pal, px / sc,
                                 vein_width=0.018, puff=0.00012, tertiary=0.0, trans=0.82, gloss=-0.1))
    for cx in (-0.13, 0.13):
        r = ((X - cx) / 0.090) ** 2 + ((Y - 0.44) / 0.105) ** 2
        blotch = 1 - F.smoothstep(0.70, 1.12, r)
        layer.color = F.lerp(layer.color, (0.020, 0.014, 0.020), blotch * 0.94)
        layer.height -= 0.00008 * blotch
    r = np.hypot(X, Y - 0.50)
    disc = 1 - F.smoothstep(0.08, 0.12, r)
    n = F.noise(X.shape, nrng, freq=45.0, beta=1.4)
    layer.color = F.lerp(layer.color, F.lerp((0.34, 0.32, 0.070), (0.50, 0.46, 0.105), n), disc)
    layer.height += 0.00025 * disc
    atlas.put("flower", layer, meters_per_px=0.035 / X.shape[0])


def _paint_columns(atlas, nrng):
    for key, base, hi, rough, scale in (
        ("stem", (0.050, 0.082, 0.055), (0.095, 0.125, 0.086), 0.66, 95.0),
        ("pod", (0.075, 0.145, 0.055), (0.135, 0.205, 0.082), 0.58, 34.0),
    ):
        U, V = atlas.column_grid(key)
        n = F.noise(U.shape, nrng, freq=scale, beta=1.8, aniso=(1.0, 5.0 if key == "stem" else 1.5))
        ridges = F.ridged(F.noise(U.shape, nrng, freq=18.0 if key == "pod" else 55.0, beta=2.0, aniso=(2, 1)))
        layer = F.Layer(U.shape)
        layer.color = F.lerp(base, hi, 0.35 * n + 0.22 * ridges)
        if key == "pod":
            bean_lumps = F.smoothstep(0.62, 0.90, np.sin(V * math.tau * 5.4 + 0.8 * np.sin(U * math.tau)))
            layer.color = F.lerp(layer.color, (0.16, 0.22, 0.08), bean_lumps * 0.28)
            layer.height = 0.00010 * n + 0.00030 * bean_lumps
            layer.trans[...] = 0.18
        else:
            layer.height = 0.00006 * n
            layer.trans[...] = 0.32
        layer.alpha[...] = 1
        layer.rough = rough + 0.06 * n
        atlas.put(key, layer, meters_per_px=0.50 / U.shape[0], wrap=True, opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=12)
    atlas.column("stem", 64)
    atlas.column("pod", 112)
    for key in ("leaf", "leaf_pale", "leaf_tatty"):
        atlas.tile(key, 360, 520)
    atlas.tile("flower", 260, 260)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in ("leaf", "leaf_pale", "leaf_tatty"):
        X, Y, px = atlas.grid(key)
        atlas.put(key, _leaf_layer(X, Y, px, nrng, rng, key, pals), meters_per_px=0.072 / X.shape[0])
    _paint_flower(atlas, nrng, rng)
    _paint_columns(atlas, nrng)
    atlas.save()
    return atlas


def _arc(p0, heading, elev, length, droop, n=4):
    pts = [Vector(p0)]
    side = heading.cross(Vector((0, 0, 1))).normalized()
    d = (heading * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    for _ in range(n):
        d = (Matrix.Rotation(-droop / n, 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def _stem_path(base, height, rng, lean=0.0, n=6):
    az = rng.uniform(0, math.tau)
    lean_vec = Vector((math.cos(az), math.sin(az), 0)) * lean
    pts = []
    for i in range(n):
        t = i / (n - 1)
        wig = Vector((math.sin(t * math.pi * 1.4 + az), math.cos(t * math.pi * 1.1 + az * 0.7), 0)) * 0.010 * t
        pts.append(base + lean_vec * t + wig + Vector((0, 0, height * t)))
    return pts


def _add_leaf(b, atlas, node, az, length, leaflets, phase, lod, rng, tatty=False):
    heading = Vector((math.cos(az), math.sin(az), 0))
    elev = math.radians(rng.uniform(18, 42) if not tatty else rng.uniform(4, 18))
    petiole_len = length * rng.uniform(0.25, 0.38)
    pts = _arc(node, heading, elev, petiole_len, elev * 0.40 + (0.25 if tatty else 0.08), n=2 if lod else 3)
    stem_pts = pts if lod == 0 else [pts[0], pts[-1]]
    b.tube(stem_pts, [0.0015, 0.0010, 0.0008, 0.0007][:len(stem_pts)], 3, atlas.uv("stem"),
           v_length=0.35, phase=phase, flutter=0.12)
    top = pts[-1]
    # Broad-bean leaflets are canted outward, not a flat horizontal canopy; the horizontal
    # normal component keeps the pinnate leaves readable from third-person side views.
    up = (Vector((0, 0, 0.68)) + heading * 0.74).normalized()
    key = "leaf_tatty" if tatty else rng.choices(["leaf", "leaf_pale"], [72, 28])[0]
    if lod == 2:
        for side in (-1, 1):
            d = (heading + Vector((-heading.y, heading.x, 0)) * side * 0.30 + Vector((0, 0, 0.10))).normalized()
            b.card(top + d * length * 0.05, d, up, length * 0.82, length * 0.34, atlas.uv(key),
                   rows=1, cols=1, fold=0.0, droop=0.12 + 0.18 * tatty, phase=phase,
                   flutter=0.85, flutter_base=0.25)
        return
    count = max(2, min(6, leaflets))
    sidev = Vector((-heading.y, heading.x, 0))
    positions = np.linspace(0.24, 1.0, count)
    for k, t in enumerate(positions):
        side = -1 if k % 2 == 0 else 1
        if k == count - 1 and count % 2 == 1:
            side = 0
        d = (heading * (0.80 + 0.20 * t) + sidev * side * 0.62 + Vector((0, 0, 0.08))).normalized()
        base = top + heading * length * 0.18 * t + sidev * side * length * 0.025
        ln = length * rng.uniform(0.62, 0.82) * (0.82 if side else 0.92)
        rect = atlas.uv(key)
        if side < 0:
            rect = (rect[1], rect[0], rect[2], rect[3])
        b.card(base, d, up, ln, ln * 0.42, rect, rows=2 if lod == 0 else 1, cols=1,
               fold=0.018 if lod == 0 else 0.0, curl=rng.uniform(-0.03, 0.06),
               droop=rng.uniform(0.04, 0.18) + (0.22 if tatty else 0.0), twist=rng.uniform(-0.08, 0.08),
               phase=phase, flutter=0.95, flutter_base=0.28)


def _add_flower_cluster(b, atlas, node, az, phase, lod, rng, count=3):
    if lod == 2:
        count = min(count, 1)
    heading = Vector((math.cos(az), math.sin(az), 0))
    stalk = _arc(node, heading, math.radians(40), rng.uniform(0.030, 0.050), 0.35, n=2)
    b.tube(stalk, [0.00075] * len(stalk), 3, atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.25)
    top = stalk[-1]
    for j in range(count):
        hd = Matrix.Rotation((j - (count - 1) / 2) * 0.45 + rng.uniform(-0.18, 0.18), 3, Vector((0, 0, 1))) @ heading
        ped = _arc(top, hd, math.radians(25), rng.uniform(0.016, 0.026), 0.20, n=1 if lod else 2)
        if lod == 0:
            b.tube(ped, [0.00050] * len(ped), 3, atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.35)
        normal = (Vector((0, 0, 0.55)) + hd * 0.75).normalized()
        b.flat(ped[-1] + normal * 0.002, normal, rng.uniform(0.026, 0.034), atlas.uv("flower"),
               spin=rng.uniform(0, math.tau), cup=0.08 if lod == 0 else 0.0, phase=phase,
               flutter=0.45, segs=2 if lod == 0 else 1)


def _add_pod(b, atlas, node, az, length, radius, phase, lod, rng, young=False):
    heading = Vector((math.cos(az), math.sin(az), 0))
    stalk = _arc(node, heading, math.radians(12), 0.030, 0.55, n=2)
    if lod < 2:
        b.tube(stalk, [0.0009, 0.00075, 0.00065], 3, atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.10)
    axis = (heading * rng.uniform(0.65, 0.95) + Vector((0, 0, -rng.uniform(0.30, 0.65)))).normalized()
    side = axis.cross(Vector((0, 0, 1))).normalized() if abs(axis.z) < 0.95 else Vector((1, 0, 0))
    pts = []
    rings = 3 if lod else 5
    for i in range(rings):
        t = i / (rings - 1)
        bow = side * math.sin(t * math.pi) * rng.uniform(-0.010, 0.010)
        droop = Vector((0, 0, -0.025 * math.sin(t * math.pi)))
        pts.append(stalk[-1] + axis * (length * t) + bow + droop)
    radii = []
    for i in range(rings):
        t = i / (rings - 1)
        envelope = math.sin(math.pi * (0.10 + 0.80 * t)) ** (0.42 if young else 0.32)
        lump = 1.0 if young else 1.0 + 0.20 * max(math.exp(-((t - c) / 0.105) ** 2) for c in (0.25, 0.43, 0.61, 0.78))
        radii.append(radius * envelope * lump)
    b.tube(pts, radii, 4 if lod else 5, atlas.uv("pod"), v_length=max(length, 0.06),
           phase=phase, flutter=0.04, cap=True, flatten=0.82, roll=rng.uniform(0, math.tau))


def _emit_stage(stage, atlas, lod):
    cfg = STAGES[stage]
    b = F.Batch(height=cfg["height"])
    for plant in _plants():
        rng = random.Random(plant["seed"] + lod * 2003 + sum(ord(c) for c in stage))
        base = plant["pos"]
        phase = plant["phase"]
        if stage == "Sprout":
            hd = Vector((math.cos(plant["heading"]), math.sin(plant["heading"]), 0))
            pts = [base, base + hd * 0.010 + Vector((0, 0, 0.022)),
                   base + hd * 0.018 + Vector((0, 0, 0.050)), base + hd * 0.006 + Vector((0, 0, 0.060))]
            b.tube(pts, [0.0048, 0.0045, 0.0038, 0.0025], 4, atlas.uv("stem"), v_length=0.25,
                   phase=phase, flutter=0.08, roll=math.pi / 4, cap=True)
            _add_leaf(b, atlas, pts[-1], plant["heading"] + math.pi * 0.5, 0.035, 2, phase, lod, rng)
            continue
        H = cfg["plant_height"] * rng.uniform(0.94, 1.04)
        stem = _stem_path(base, H, rng, lean=0.018 if stage in ("Mature", "Ripe") else 0.010, n=6)
        b.tube(stem, [0.0048, 0.0046, 0.0041, 0.0036, 0.0030, 0.0024], 4, atlas.uv("stem"),
               v_length=0.42, phase=phase, flutter=0.04, roll=math.pi / 4, cap=True)
        for l in range(cfg["leaves"]):
            t = 0.16 + 0.76 * l / max(cfg["leaves"] - 1, 1)
            seg = min(int(t * (len(stem) - 1)), len(stem) - 2)
            f = t * (len(stem) - 1) - seg
            node = stem[seg].lerp(stem[seg + 1], f)
            az = plant["heading"] + l * 2.39996 + rng.uniform(-0.28, 0.28)
            leaf_len = (0.044 + 0.046 * t) * (0.75 if stage == "Young" else 1.0)
            _add_leaf(b, atlas, node, az, leaf_len, rng.choice([2, 4, 4, 6]), phase, lod, rng,
                      tatty=(stage == "Ripe" and l < 2))
        if cfg["side"] and lod == 0:
            for s in range(cfg["side"]):
                t = 0.32 + 0.20 * s
                node = stem[int(t * (len(stem) - 1))]
                az = plant["heading"] + (s * math.pi * 0.82) + rng.uniform(-0.4, 0.4)
                hd = Vector((math.cos(az), math.sin(az), 0))
                branch = _arc(node, hd, math.radians(40), H * 0.20, 0.15, n=3)
                b.tube(branch, [0.0022, 0.0018, 0.0014, 0.0010], 4, atlas.uv("stem"), v_length=0.35,
                       phase=phase, flutter=0.06, roll=math.pi / 4, cap=True)
                _add_leaf(b, atlas, branch[-1], az, 0.058, rng.choice([4, 6]), phase, lod, rng)
        for k in range(cfg["flowers"]):
            if lod == 2 and k >= 1:
                break
            node = stem[int(rng.uniform(0.48, 0.86) * (len(stem) - 1))]
            _add_flower_cluster(b, atlas, node, plant["heading"] + k * 1.9 + rng.uniform(-0.3, 0.3), phase, lod, rng,
                                count=2 if stage in ("Mature", "Ripe") else rng.choice([2, 3]))
        for k in range(cfg["pods"]):
            if lod == 2 and k >= 1:
                break
            if lod == 1 and stage == "Ripe" and k >= 3:
                break
            node = stem[int(rng.uniform(0.40, 0.82) * (len(stem) - 1))]
            if stage == "Mature":
                _add_pod(b, atlas, node, plant["heading"] + k * 2.1 + rng.uniform(-0.35, 0.35),
                         rng.uniform(0.055, 0.085), rng.uniform(0.0045, 0.0065), phase, lod, rng, young=True)
            else:
                _add_pod(b, atlas, node, plant["heading"] + k * 1.35 + rng.uniform(-0.45, 0.45),
                         rng.uniform(0.120, 0.175), rng.uniform(0.0088, 0.0115), phase, lod, rng, young=False)
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _emit_harvest(atlas):
    b = F.Batch(height=0.25)
    rng = random.Random(SEED + 9001)
    origin = Vector((0, 0, 0))
    for k, az in enumerate((-0.55, 0.20, 0.85)):
        heading = Vector((math.sin(az) * 0.55, math.cos(az) * 0.55, -1.0)).normalized()
        side = heading.cross(Vector((0, 0, 1))).normalized() if abs(heading.z) < 0.96 else Vector((1, 0, 0))
        stalk_end = origin + heading * rng.uniform(0.022, 0.034) + side * rng.uniform(-0.004, 0.004)
        b.tube([origin, stalk_end], [0.0014, 0.0010], 5, atlas.uv("stem"), v_length=0.35, phase=0.17, flutter=0.02)
        length = rng.uniform(0.125, 0.165)
        radius = rng.uniform(0.0090, 0.0112)
        pts = []
        rings = 8
        for i in range(rings):
            t = i / (rings - 1)
            pts.append(stalk_end + heading * length * t + side * math.sin(t * math.pi) * rng.uniform(-0.008, 0.008))
        radii = []
        for i in range(rings):
            t = i / (rings - 1)
            envelope = math.sin(math.pi * (0.08 + 0.84 * t)) ** 0.32
            lump = 1.0 + 0.18 * max(math.exp(-((t - c) / 0.11) ** 2) for c in (0.25, 0.45, 0.65, 0.82))
            radii.append(radius * envelope * lump)
        b.tube(pts, radii, 10, atlas.uv("pod"), v_length=length, phase=0.17, flutter=0.02,
               cap=True, flatten=0.82, roll=rng.uniform(0, math.tau))
    print("HOMESTEAD_TRIS Harvest 0", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    objs = []
    for stage in ("Sprout", "Young", "Growing", "Mature", "Ripe"):
        batches = [_emit_stage(stage, atlas, lod) for lod in range(3)]
        made = F.finish_lods(kit, batches, f"SM_{NAME}_{stage}", material, smooth_angle=179.0)
        print("HOMESTEAD_LODS", stage, F.lod_report(made))
        objs.extend(made)
    harvest = _emit_harvest(atlas).build(f"SM_{NAME}_Harvest", material)
    harvest = kit.join([harvest], f"SM_{NAME}_Harvest", pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    harvest.data.color_attributes.active_color = harvest.data.color_attributes["Wind"]
    objs.append(harvest)
    return objs
