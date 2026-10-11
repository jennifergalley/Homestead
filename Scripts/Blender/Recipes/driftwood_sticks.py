"""Driftwood sticks: a small cluster of thin, silver, sea-bleached sticks and twigs with a little wrack (scenery).

The real thing (what the geometry and textures follow):
- Among the wrack on any strandline lie sticks and twigs from cliff-top scrub, hedges and the odd
  broken branch brought down a stream: 20-60 cm long, 1.5-4.5 cm thick, bark long gone, the wood
  bleached silver-grey (palest of all the beach wood, ~0.35-0.45 linear at most), ends rounded by
  sand and the grain raised in fine ribs, a few short seasoning checks, a side twig snapped to a
  stub, one end often split or frayed.
- They lie loosely crossed where the tide dropped them, settled a few millimetres into the sand,
  with a few strands of dried bladderwrack (olive-brown going black, bladders paired along the
  midrib) caught over and under them.

Geometry: tubes along crooked paths for the sticks and side-twig stubs, and flattened forked straps
with bladders for the wrack; a dense displaced high copy (ribs, checks, frayed ends) shaded with
the DriftwoodLog material (loaded from driftwood_log.py) and a vertex-coloured wrack material is
baked onto the light game mesh (homestead_highbake, 2K). Pivot bottom centre; sticks bed 3-6 mm
below z = 0. Original procedural geometry and materials.
"""
import importlib.util
import math
import os
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_highbake as hb  # noqa: E402
import homestead_rocks as rocks  # noqa: E402

_spec = importlib.util.spec_from_file_location("homestead_driftwood_log", Path(__file__).with_name("driftwood_log.py"))
log = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(log)

NAME = "DriftwoodSticks"
DESCRIPTION = ("A loose cluster of five thin sea-bleached sticks and twigs 21-58 cm with a few strands of dried "
               "bladderwrack caught over them. Ground-hugging scenery, no collision.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = None  # baked high-to-low inside build() (homestead_highbake)
BAKE_SIZE = 1024 if DRAFT else 2048
BEAUTY = {"pose": (0, 0, 0), "focus": (0.06, 0.0, 0.02), "ground": "origin", "detail_distance": 0.45,
          "detail_fstop": 11.0, "views": ["hero", "detail", "eye"], "eye_distance": 1.6}
REPORT = {"placement": "Ground line at z = 0 (pivot, bottom centre); sticks bed 3-6 mm below it. Random yaw, "
                       "0.8-1.2 scale."}
NOTES = {"sink_depth_m": 0.006}
SEED = 61851
LOD_RATIOS = (0.5, 0.25)

# (length m, base radius m, tip radius m, yaw deg, x, y, crook, twig stubs)
STICKS = [(0.58, 0.018, 0.011, 12, -0.02, 0.00, 0.06, 2), (0.45, 0.014, 0.009, -28, 0.03, 0.05, 0.04, 1),
          (0.36, 0.011, 0.007, 61, 0.08, -0.07, 0.05, 1), (0.27, 0.010, 0.0065, -70, -0.12, 0.07, 0.03, 0),
          (0.21, 0.009, 0.006, 150, 0.18, 0.09, 0.03, 0)]


def stick_paths(rng):
    """Centre lines settled on the sand and on each other (later sticks lie across earlier ones)."""
    out = []
    for i, (L, r0, r1, yaw, x, y, crook, stubs) in enumerate(STICKS):
        n = 40
        # Walk the centre line: a gentle bow plus one or two kinks where a side shoot or a knot
        # deflected the grain - beach sticks are never dowel-straight.
        heading = math.radians(yaw)
        kinks = {int(n * rng.uniform(0.25, 0.75)): rng.choice((-1, 1)) * rng.uniform(0.12, 0.26)
                 for _ in range(rng.randint(1, 2))}
        bow = rng.choice((-1, 1)) * crook * 1.6
        walk = [Vector((0.0, 0.0, 0.0))]
        for k in range(n):
            heading += bow / n * 2.0 + kinks.get(k, 0.0) + rng.gauss(0.0, 0.012)
            walk.append(walk[-1] + Vector((math.cos(heading), math.sin(heading), 0.0)) * L / n)
        mid = (walk[0] + walk[-1]) * 0.5
        pts, radii = [], []
        ph = rng.uniform(0, 6)
        for k in range(n + 1):
            t = k / n
            p = walk[k] - mid + Vector((x, y, 0.0))
            r = r0 + (r1 - r0) * t
            r *= 1.0 + 0.08 * math.sin(t * 9.0 + ph) + 0.05 * math.sin(t * 23.0)
            # Sand-rounded ends: the radius closes over about one radius at each end.
            e = min(t, 1.0 - t) * L / (1.1 * r)
            r *= max(0.3, math.sqrt(max(0.0, 1.0 - (1.0 - min(e, 1.0)) ** 2)))
            pts.append(p)
            radii.append(r)
        out.append(dict(pts=pts, radii=radii, stubs=stubs, L=L, seed=101 + i))
    # Settle: each point rests on the ground (bedded 3-6 mm) or on a stick already placed.
    placed = []
    for st in out:
        bed = rng.uniform(0.003, 0.006)
        for k, p in enumerate(st["pts"]):
            r = st["radii"][k]
            z = r - bed
            for other in placed:
                for q, rq in zip(other["pts"], other["radii"]):
                    dxy = math.hypot(p.x - q.x, p.y - q.y)
                    if dxy < r + rq:
                        z = max(z, q.z + math.sqrt(max((r + rq) ** 2 - dxy * dxy, 0.0)) - 0.002)
            p.z = z
        # Smooth the settled heights (a rigid stick bridges, it doesn't follow every bump).
        zs = [p.z for p in st["pts"]]
        for _ in range(30):
            zs = [zs[0]] + [max(st["radii"][k] - bed, (zs[k - 1] + zs[k + 1]) * 0.5) for k in range(1, len(zs) - 1)] + [zs[-1]]
        for p, z in zip(st["pts"], zs):
            p.z = z
        placed.append(st)
    return out


def twig_stubs(st, rng):
    """Side twigs snapped to short stubs, angled forward along the stick."""
    out = []
    for k in range(st["stubs"]):
        i = int(len(st["pts"]) * rng.uniform(0.25, 0.7))
        p, q = st["pts"][i], st["pts"][i + 1]
        t = (q - p).normalized()
        side = t.cross(Vector((0, 0, 1))).normalized() * rng.choice((-1, 1))
        d = (t * 0.8 + side * 0.6 + Vector((0, 0, rng.uniform(0.1, 0.5)))).normalized()
        L = rng.uniform(0.04, 0.09)
        r = st["radii"][i] * rng.uniform(0.45, 0.6)
        pts = [p + d * L * u for u in np.linspace(0.0, 1.0, 8)]
        out.append(dict(pts=pts, radii=[r * (1 - 0.45 * u) * (0.55 if u > 0.99 else 1.0) for u in np.linspace(0, 1, 8)], seed=st["seed"] * 7 + k))
    return out


def tube_mesh(name, pts, radii, sides, cap=True):
    from homestead_kit import tube
    return tube(name, [tuple(p) for p in pts], radii=list(radii), sides=sides, cap=cap)


def resample(pts, radii, n):
    acc = [0.0]
    for a, b in zip(pts, pts[1:]):
        acc.append(acc[-1] + (b - a).length)
    out_p, out_r = [], []
    for s in np.linspace(0, acc[-1], n):
        i = min(max(np.searchsorted(acc, s) - 1, 0), len(pts) - 2)
        f = (s - acc[i]) / max(acc[i + 1] - acc[i], 1e-9)
        out_p.append(pts[i].lerp(pts[i + 1], f))
        out_r.append(radii[i] + (radii[i + 1] - radii[i]) * f)
    return out_p, out_r


def stick_detail(obj, seed, length, rng, frayed):
    """Displace a dense stick tube in its grain frame (pcoord = (r cos, r sin, s)) and write the
    DriftwoodLog material attributes."""
    mesh = obj.data
    P = rocks.coords(obj)
    N = rocks.normals(obj)
    pc = np.empty(len(P) * 3)
    mesh.attributes["pcoord"].data.foreach_get("vector", pc)
    pc = pc.reshape(-1, 3)
    r = np.maximum(np.hypot(pc[:, 0], pc[:, 1]), 1e-5)
    th = np.arctan2(pc[:, 1], pc[:, 0])
    s = pc[:, 2]
    twist = rng.uniform(-1.5, 1.5)
    tg = th + twist * s
    cu, su = np.cos(tg), np.sin(tg)
    stripes = max(6, round(2 * math.pi * float(np.median(r)) / 0.006))
    warp = log.pfbm(np.stack([cu, su, s / 0.2], axis=1), 2, seed)
    ridge = (0.5 + 0.5 * np.cos(2 * np.pi * (tg * stripes / (2 * np.pi) + 1.5 * warp))) ** 3
    dips = log.pfbm(np.stack([cu * r / 0.01, su * r / 0.01, s / 0.08], axis=1), 2, seed + 1)
    ends = np.clip(np.minimum(s, length - s) / 0.02, 0.0, 1.0)
    fine = (0.0013 * ridge + 0.0010 * dips - 0.0006) * ends
    check = np.zeros(len(P))
    for c in range(rng.randint(1, 2)):
        phi = rng.uniform(-np.pi, np.pi)
        s0 = rng.uniform(0.05, length * 0.5)
        s1 = min(length - 0.03, s0 + rng.uniform(0.08, 0.2))
        e = np.clip((s - s0) / (s1 - s0), 0, 1)
        env = np.sin(np.pi * e) ** 0.6
        arc = np.abs(log._wrap(tg - phi)) * r
        w = rng.uniform(0.0015, 0.0025) * (0.4 + 0.6 * env) + 1e-5
        check = np.maximum(check, rng.uniform(0.002, 0.004) * env * np.clip(1 - arc / w, 0, 1) ** 1.5)
    check *= np.clip(r / 0.015, 0.3, 1.0)
    fray = np.zeros(len(P))
    if frayed:
        tip = np.clip((s - (length - 0.035)) / 0.035, 0, 1)
        fray = tip * (0.0025 * (0.5 + 0.5 * np.cos(tg * 7 + 2.0 * log.pfbm(np.stack([cu, su, s * 30], 1), 2, seed + 3))))
        fray *= np.clip(r / 0.012, 0.0, 1.0)  # never more than ~20 % of the local radius (folds on the rounded tip)
    rocks.set_coords(obj, P + N * (fine - check - fray)[:, None])
    log._set_attr(mesh, "dw_check", np.clip(check / 0.003, 0, 1))
    log._set_attr(mesh, "dw_ridge", ridge * ends)
    log._set_attr(mesh, "dw_knot", np.zeros(len(P)))
    log._set_attr(mesh, "dw_cav", np.clip(-dips * 1.2, 0, 1) * ends)
    log._set_attr(mesh, "dw_cap", 1.0 - ends)
    log._set_attr(mesh, "dw_calm", np.ones(len(P)))


def wrack_strand(rng, over, z_of):
    """A forked bladderwrack strap draped over the sticks: list of (points, width) segments plus
    bladder centres."""
    segs, bladders = [], []

    def grow(p, ang, length, w, level):
        n = 10
        pts = []
        for k in range(n + 1):
            t = k / n
            a = ang + 0.4 * math.sin(t * 3 + level)
            q = p + Vector((math.cos(a), math.sin(a), 0.0)) * length * t
            q.z = z_of(q.x, q.y) + 0.0012
            pts.append(q)
        segs.append((pts, w))
        k = rng.randint(3, 6)
        tdir = (pts[k + 1] - pts[k]).normalized()
        side = tdir.cross(Vector((0, 0, 1))).normalized()
        for sgn in (-1, 1):
            c = pts[k] + side * sgn * w * 0.55
            c.z = z_of(c.x, c.y) + w * 0.2
            bladders.append((c, w * 0.36, tdir))
        if level < 2:
            for sgn in (-1, 1):
                grow(pts[-1], ang + sgn * rng.uniform(0.3, 0.6), length * rng.uniform(0.6, 0.8), w * 0.85, level + 1)
    grow(over, rng.uniform(0, 2 * math.pi), rng.uniform(0.08, 0.12), rng.uniform(0.010, 0.013), 0)
    return segs, bladders


def strap_mesh(name, pts, w, thick, sides):
    """A flattened tube (ellipse cross-section w x thick) along pts, for wrack straps."""
    from homestead_kit import loft
    rows = []
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        side = t.cross(Vector((0, 0, 1))).normalized()
        up = side.cross(t).normalized()
        taper = 1.0 - 0.3 * i / (len(pts) - 1)
        ring = []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            ring.append(tuple(p + side * math.cos(a) * w * 0.5 * taper + up * math.sin(a) * thick * 0.5))
        rows.append(ring)
    return loft(name, rows, cap_start=True, cap_end=True)


def wrack_material(kit, name):
    g = kit.mats.Graph(name)
    pos = g.node("ShaderNodeTexCoord").outputs["Object"]
    n1 = g.noise(pos, scale=40.0, detail=4.0).outputs["Fac"]
    n2 = g.noise(pos, scale=300.0, detail=3.0).outputs["Fac"]
    blad = g.node("ShaderNodeAttribute", attribute_name="wk_bladder", attribute_type="GEOMETRY").outputs["Fac"]
    color = g.mix((0.020, 0.016, 0.010), (0.050, 0.038, 0.016), g.remap(n1, 0.3, 0.7))
    color = g.mix(color, (0.052, 0.040, 0.016), g.math("MULTIPLY", blad, 0.8))
    salt = g.math("MULTIPLY", g.remap(n1, 0.65, 0.8), g.remap(n2, 0.5, 0.8))
    color = g.mix(color, (0.17, 0.165, 0.155), g.math("MULTIPLY", salt, 0.4))
    g.set("Base Color", color)
    g.set("Roughness", g.math("SUBTRACT", 0.72, g.math("MULTIPLY", blad, 0.3)))
    g.set("Normal", g.bump(g.math("ADD", n2, g.math("MULTIPLY", n1, 0.5)), strength=0.3, distance=0.0004))
    g.bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return g.mat


def build(kit):
    rng = random.Random(SEED)
    sticks = stick_paths(rng)
    wood = log.driftwood_material(kit, "M_DriftwoodSticksProcedural", height_scale=0.08)
    weed = wrack_material(kit, "M_DriftwoodSticksWrack")
    highs, lows = [], []

    def add_wood(name, pts, radii, seed, length, frayed):
        hp, hr = resample(pts, radii, max(24, int(length / 0.004)))
        hi = tube_mesh(name + "Hi", hp, hr, 24)
        stick_detail(hi, seed, length, rng, frayed)
        hi.data.materials.append(wood)
        lp, lr = resample(pts, radii, max(8, int(length / 0.024)))
        lo = tube_mesh(name + "Lo", lp, lr, 8)
        highs.append(hi)
        lows.append(lo)

    for i, st in enumerate(sticks):
        add_wood(f"Stick{i}", st["pts"], st["radii"], st["seed"], st["L"], frayed=i in (0, 2))
        for k, tw in enumerate(twig_stubs(st, rng)):
            L = (tw["pts"][-1] - tw["pts"][0]).length
            hp, hr = resample(tw["pts"], tw["radii"], 20)
            hi = tube_mesh(f"Twig{i}_{k}Hi", hp, hr, 16)
            stick_detail(hi, tw["seed"], L, rng, False)
            hi.data.materials.append(wood)
            lo = tube_mesh(f"Twig{i}_{k}Lo", tw["pts"][::3] + [tw["pts"][-1]], tw["radii"][::3] + [tw["radii"][-1]], 6)
            highs.append(hi)
            lows.append(lo)

    def z_of(x, y):
        z = 0.0005
        for st in sticks:
            for q, rq in zip(st["pts"], st["radii"]):
                dxy = math.hypot(x - q.x, y - q.y)
                if dxy < rq * 1.4:
                    z = max(z, q.z + math.sqrt(max((rq * 1.05) ** 2 - min(dxy, rq) ** 2, 0.0)))
        return z

    for i, over in enumerate((sticks[0]["pts"][14], sticks[1]["pts"][28], sticks[2]["pts"][10])):
        segs, bladders = wrack_strand(rng, over.copy(), z_of)
        for k, (pts, w) in enumerate(segs):
            hi = strap_mesh(f"Wrack{i}_{k}Hi", pts, w, 0.0016, 10)
            log._set_attr(hi.data, "wk_bladder", np.zeros(len(hi.data.vertices)))
            hi.data.materials.append(weed)
            highs.append(hi)
            lows.append(strap_mesh(f"Wrack{i}_{k}Lo", pts[::2] + ([pts[-1]] if len(pts) % 2 == 0 else []), w,
                                   0.0016, 4))
        for k, (c, rad, tdir) in enumerate(bladders):
            # Air bladders are flattened ovals lying along the strap, not beads.
            rot = Matrix.Rotation(math.atan2(tdir.y, tdir.x), 4, "Z")
            shape = Matrix.Translation(c) @ rot @ Matrix.Diagonal((1.5, 1.0, 0.6, 1.0))
            b = kit.sphere(f"Bladder{i}_{k}Hi", rad, segments=12, rings=8)
            b.data.transform(shape)
            log._set_attr(b.data, "wk_bladder", np.ones(len(b.data.vertices)))
            b.data.materials.append(weed)
            highs.append(b)
            lb = kit.sphere(f"Bladder{i}_{k}Lo", rad, segments=5, rings=3)
            lb.data.transform(shape)
            lows.append(lb)

    hi_obj = rocks.join(highs, "DriftwoodSticksHigh")
    lo_obj = rocks.join(lows, "SM_" + NAME)
    lo_obj.data.materials.clear()
    for obj in (hi_obj, lo_obj):
        bpy.context.view_layer.objects.active = obj
        for o in bpy.context.scene.objects:
            o.select_set(o is obj)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    hi_obj.data.shade_smooth()
    lo, hi = kit.bounds(lo_obj)
    shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 0.0))
    for obj in (lo_obj, hi_obj):
        obj.data.transform(Matrix.Translation(-shift))
        obj.data.update()
    kit.pack_uvs(lo_obj, margin=0.004)
    kit.finalize(lo_obj, pivot=None, unwrap=False, reshade=True, smooth_angle=70.0)
    lo_obj["homestead_shift"] = list(shift)
    folder = Path(__file__).resolve().parents[3] / "Assets" / "Props" / NAME / "Textures"
    material, info = hb.bake(kit, hi_obj, lo_obj, folder, NAME, size=BAKE_SIZE, samples=16 if DRAFT else 64,
                             extrusion=0.005, distance=0.014)
    bpy.data.objects.remove(hi_obj)
    meshes = [lo_obj]
    for index, ratio in enumerate(LOD_RATIOS, start=1):
        lod = rocks.lod(kit, lo_obj, f"SM_{NAME}_LOD{index}", ratio)
        kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=70.0)
        lod.data.materials.clear()
        lod.data.materials.append(material)
        lod["homestead_shift"] = list(shift)
        meshes.append(lod)
    NOTES.update({"bake": info, "lods": {m.name: rocks.triangles(m) for m in meshes},
                  "material": "Baked from a dense displaced high mesh with the DriftwoodLog wood material and a "
                              "wrack material (homestead_highbake); M_PropTextured."})
    print("HOMESTEAD_STICKS", NOTES["lods"], flush=True)
    return meshes
