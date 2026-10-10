"""Ripe wild red currants to dress SM_WildCurrant: one SECTOR of three hanging strigs.

    .\\Scripts\\Blender\\New-Prop.ps1 Scripts\\Blender\\Recipes\\currant_produce.py

The game places three copies of this mesh on each currant bush, rotated 0 / 120 / 240
degrees about the vertical axis through the pivot, and hides one copy at a time as she
picks. So this mesh holds only the strigs of a ~100 degree arc (azimuths 20, 60 and 100
degrees from +X); the three copies interlock into nine evenly spaced strigs around the bush.

Real-object research (written before modeling):
- Red currant (Ribes rubrum) fruit hangs in "strigs": racemes 5-10 cm long on a thin
  green rachis flushed red, from short spurs on 2-3 year old wood, mostly on the outer,
  sunlit side of the bush. Each strig carries 6-15 berries on 3-6 mm pedicels; the
  biggest, ripest berries sit at the top of the strig, smaller ones toward its tip.
- Berries are near-spherical (slightly taller than wide), 8-10 mm across, with thin
  glossy translucent skin: bright scarlet-red when ripe, light glowing through them, faint
  pale meridian lines (the vascular strands) running pole to pole, and a tiny dark dried
  calyx at the free (bottom) end. Late berries stay orange-red or pinkish.
- Here the berries are scaled ~1.6x (radius 5.4-7.4 mm) so the strigs read at game
  distance, like the existing blackberry produce.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the bush base (ground level, bush centre), same as SM_WildCurrant.
Strigs hang from twig stubs at 42-60 cm height, 47-49 cm out from the pivot (the bush's
outer leaf shell), each stub
pointing back into the foliage, the berries hanging down and a little outward.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import random

from mathutils import Matrix, Vector

import homestead_materials as hm

NAME = "CurrantProduce"
DESCRIPTION = ("One sector (~100 degrees) of ripe red currant strigs for SM_WildCurrant (original): three "
               "strigs of 8-9 glossy berries. Place three copies at 0/120/240 degrees; pivot = bush base.")
COLLISION = "none"
TRIANGLE_BUDGET = 8000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 64}

SEED = 6127
UP = Vector((0, 0, 1))
SECTOR = (20.0, 60.0, 100.0)      # degrees from +X; copies at +120 / +240 fill the circle evenly
HEIGHTS = (0.42, 0.60, 0.50)      # m, twig-stub node height above the pivot (bush is ~0.95 m tall)
RADII = (0.48, 0.47, 0.485)       # m, node distance from the axis: just inside the leaf shell (r90 0.45-0.51)
COUNTS = (9, 8, 9)                  # berries per strig
STRIG_LEN = (0.115, 0.10, 0.11)  # m, rachis drop below the node
BERRY_R = (0.0074, 0.0054)        # m, radius at the top of a strig -> at its tip (real ~4.5 mm, x1.6)
SIDES, ROWS = 14, 9               # berry loft resolution (smooth shaded, reads round at 4K close-up)


def _node(k):
    a = math.radians(SECTOR[k])
    out = Vector((math.cos(a), math.sin(a), 0.0))
    return out * RADII[k] + UP * HEIGHTS[k], out


_n0, _o0 = _node(0)
BEAUTY = {"pose": (0, 0, 0), "focus": tuple(_n0 + _o0 * 0.02 - UP * 0.04)}


def _berry_material(name, deep, bright, glow, seed):
    """Glossy translucent currant skin. pcoord = unit-sphere berry-local position, z = +1 at
    the pedicel and -1 at the calyx. Light pools toward the lower half (it shines through the
    berry), pale meridian lines run pole to pole and the calyx is a tiny dark dry star."""
    g = hm.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 1.7, seed * 0.9, seed * 2.3))
    mottle = g.noise(seeded, scale=2.2, detail=3.0).outputs["Fac"]
    angle = g.math("ARCTAN2", y, x)
    wobble = g.math("MULTIPLY", g.noise(seeded, scale=4.0, detail=2.0).outputs["Fac"], 0.5)
    meridian = g.math("ABSOLUTE", g.math("SINE", g.math("ADD", g.math("MULTIPLY", angle, 4.0), wobble)))
    lines = g.math("MULTIPLY", g.remap(meridian, 0.982, 0.999),
                   g.remap(g.math("ABSOLUTE", z), 0.92, 0.55))
    body = g.mix(deep, bright, g.remap(g.math("ADD", g.math("MULTIPLY", z, -0.55), mottle), 0.05, 1.1))
    body = g.mix(body, glow, g.math("MULTIPLY", lines, 0.22))
    radial = g.vmath("LENGTH", g.combine(x, y, 0.0))
    below = g.remap(z, -0.6, -0.9)
    calyx = g.math("MULTIPLY", g.remap(radial, 0.20, 0.11), below)
    scar = g.math("MULTIPLY", g.remap(radial, 0.16, 0.08), g.remap(z, 0.7, 0.95))
    body = g.mix(body, (0.035, 0.016, 0.010), calyx)
    body = g.mix(body, (0.20, 0.05, 0.02), g.math("MULTIPLY", scar, 0.6))
    g.set("Base Color", body)
    g.set("Roughness", g.math("ADD", 0.17, g.math("MULTIPLY", calyx, 0.6)))
    g.set("Coat Weight", 0.35)
    g.set("Coat Roughness", 0.06)
    g.set("Subsurface Weight", 0.08)
    g.set("Subsurface Radius", (0.006, 0.0008, 0.0006))
    height = g.math("SUBTRACT", g.math("MULTIPLY", lines, 0.15), g.math("MULTIPLY", calyx, 0.8))
    g.set("Normal", g.bump(height, strength=0.35, distance=0.0004))
    return g.mat


def materials(kit):
    m = kit.mats
    return {
        "ripe": _berry_material("M_Currant", (0.30, 0.008, 0.013), (0.56, 0.028, 0.036), (0.75, 0.13, 0.10), 0.0),
        "ripe2": _berry_material("M_CurrantDeep", (0.26, 0.006, 0.012), (0.46, 0.020, 0.030), (0.66, 0.10, 0.09),
                                 1.0),
        "late": _berry_material("M_CurrantLate", (0.42, 0.045, 0.018), (0.62, 0.12, 0.035), (0.78, 0.30, 0.14),
                                2.0),
        "rachis": m.stem("M_CurrantStrig", color=(0.06, 0.15, 0.025), dark=(0.22, 0.04, 0.025), roughness=0.55),
        "twig": m.stem("M_CurrantTwig", color=(0.20, 0.165, 0.125), dark=(0.10, 0.07, 0.05), roughness=0.75),
    }


def berry(kit, name, material, top, axis, radius, roll):
    """Near-spherical berry hanging from ``top`` along ``axis``: a touch taller than wide,
    the calyx end slightly dimpled. pcoord holds the unit-sphere position (z = +1 at top)."""
    axis = axis.normalized()
    e1 = axis.orthogonal().normalized()
    e1 = Matrix.Rotation(roll, 3, axis) @ e1
    e2 = axis.cross(e1).normalized()
    stretch = 1.07
    centre = top + axis * radius * stretch
    rows, coords = [], []
    for i in range(ROWS):
        t = (i + 0.5) / ROWS                     # 0 at the pedicel pole -> 1 at the calyx
        th = math.pi * t
        rr = math.sin(th) * radius
        h = math.cos(th) * radius * stretch
        if t > 0.8:
            h += radius * 0.05 * ((t - 0.8) / 0.2) ** 2     # calyx dimple
        ring, local = [], []
        for j in range(SIDES):
            a = math.tau * j / SIDES
            d = e1 * math.cos(a) + e2 * math.sin(a)
            ring.append(tuple(centre - axis * h + d * rr))
            local.append((math.cos(a) * math.sin(th), math.sin(a) * math.sin(th), math.cos(th)))
        rows.append(ring)
        coords.append(local)
    top_pole = tuple(centre + axis * radius * stretch)
    bottom_pole = tuple(centre - axis * radius * (stretch - 0.06))
    obj = kit.loft(name, rows, material=material, coords=coords, cap_start=top_pole, cap_end=bottom_pole)
    return kit.recalc_normals(obj)


def strig(kit, name, mats, k, rng):
    node, out = _node(k)
    side = UP.cross(out).normalized()
    parts = []
    # Twig stub: a short spur running back into the foliage, the strig hanging from its end.
    inward = node - out * 0.075 + UP * 0.03 + side * rng.uniform(-0.01, 0.01)
    mid = node - out * 0.032 + UP * 0.016
    parts.append(kit.tube(name + "Twig", [inward, mid, node], radius=lambda t: 0.0021 - 0.0007 * t, sides=6,
                          material=mats["twig"]))
    length = STRIG_LEN[k]
    lean = out * rng.uniform(0.016, 0.024) + side * rng.uniform(-0.01, 0.01)
    rachis = []
    for i in range(9):
        t = i / 8
        rachis.append(node + out * 0.006 * math.sin(t * math.pi * 0.5) + lean * t ** 1.5 - UP * length * t ** 1.08)
    parts.append(kit.tube(name + "Rachis", rachis, radius=lambda t: 0.00085 - 0.00035 * t, sides=5,
                          material=mats["rachis"], cap=False))
    count = COUNTS[k]
    placed = []                                      # (centre, radius) of berries already hung
    for b in range(count):
        t = 0.14 + 0.86 * b / (count - 1)
        f = t * 8
        i0 = min(int(f), 7)
        base = rachis[i0].lerp(rachis[i0 + 1], f - i0)
        radius = (BERRY_R[0] + (BERRY_R[1] - BERRY_R[0]) * (b / (count - 1)) ** 1.2) * rng.uniform(0.92, 1.06)
        az0 = b * 2.39996 + rng.uniform(-0.3, 0.3)
        ped_len = radius * 0.55 + rng.uniform(0.0035, 0.005)
        best = None
        for attempt in range(12):
            az = az0 + attempt * 0.52
            radial = (out * math.cos(az) + side * math.sin(az)).normalized()
            if radial.dot(out) < -0.3:
                radial = (radial + out * 0.8).normalized()   # berries crowd to the lit, outer side
            length = ped_len * (1.0 + 0.12 * (attempt // 4))
            tip = base + radial * length * 0.8 - UP * length * 0.55
            axis = (radial * 0.45 - UP).normalized() if b < count - 1 else (-UP + radial * 0.15).normalized()
            centre = tip + axis * radius * 1.07
            gap = min([(centre - c).length - (radius + r) for c, r in placed] + [1.0])
            if best is None or gap > best[0]:
                best = (gap, radial, tip, axis, centre, length)
            if gap > 0.0004:
                break
        _, radial, tip, axis, centre, length = best
        placed.append((centre, radius))
        ctrl = base + radial * length * 0.62 - UP * length * 0.05
        stalk = [base * (1 - s) ** 2 + ctrl * (2 * s * (1 - s)) + tip * s * s for s in (0.0, 0.34, 0.67, 1.0)]
        parts.append(kit.tube(f"{name}P{b}", stalk, radius=lambda t: 0.00045 - 0.0001 * t, sides=4,
                              material=mats["rachis"], cap=False))
        roll = rng.uniform(0, math.tau)
        pick = rng.random()
        material = mats["late"] if (b >= count - 2 and pick < 0.5) or pick < 0.06 else \
            mats["ripe2"] if pick < 0.4 else mats["ripe"]
        parts.append(berry(kit, f"{name}B{b}", material, tip, axis, radius, roll))
    return parts


def build(kit):
    mats = materials(kit)
    rng = random.Random(SEED)
    parts = []
    for k in range(len(SECTOR)):
        parts += strig(kit, f"Strig{k}", mats, k, rng)
    return kit.join(parts, "SM_" + NAME, pivot=None, unwrap=False, reshade=True, smooth_angle=100)
