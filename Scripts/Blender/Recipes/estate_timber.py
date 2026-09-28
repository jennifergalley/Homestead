"""Estate clearing timber: three felled-tree stumps and three pieces of fallen wood, cleared with the
axe (add-overgrown-estate-clearing: small stump and fallen bough at worn tier, large stump and
fallen log at iron, ancient stump and giant log at steel).

Real-object research (written before modeling):
- Cornish estate woodland is mostly pedunculate oak, ash, sycamore and beech; a neglected estate
  has stumps from old fellings, windthrow and coppice stools. Felled stumps are cut 15-60 cm above
  ground (older axe work higher than saw work), the cut face slightly sloped, with a ragged strip
  of "hinge" fibres where the last wood tore rather than was cut.
- Root flare: the trunk swells over the last 20-40 cm into 4-7 buttress lobes that run into the
  soil. Old cut faces weather silver-grey with radial checks and dark growth-ring lines; ancient
  stumps rot from the heart, go soft and mossy, and lose bark in patches.
- Fallen timber: branches 5-10 cm through and 1.5-2.5 m long with side twigs; logs 25-45 cm
  through and 3-4 m; giant windthrown trunks 70-90 cm through. Bark on down wood splits and
  sloughs in patches, lichen and moss take the top, and cut ends show rings.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters, Z up, pivot at bottom centre.
"""
import math
import random

from mathutils import Vector, noise

NAME = "EstateTimber"
DESCRIPTION = ("Felled stumps (small, large, ancient) and fallen wood (bough, log, giant log) for estate "
               "clearing with the axe (original).")
COLLISION = "none"  # The world presents resources without collision.
TRIANGLE_BUDGET = 24000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.3),
          "meshes": {"SM_StumpAncient": {"focus": (0.0, 0.0, 0.5)},
                     "SM_FallenLog": {"focus": (0.0, 0.0, 0.2)},
                     "SM_GiantLog": {"focus": (0.0, 0.0, 0.4)},
                     "SM_FallenBough": {"focus": (0.0, 0.0, 0.05)}}}
SEED = 5151


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def fissures(u, v, seed):
    """Bark relief: deep, long vertical fissures between flat-topped ridges (0 in a fissure, 1 on a ridge)."""
    n = noise.noise(Vector((u, v, seed * 0.37)))
    n2 = noise.noise(Vector((u * 2.3 + 7.1, v * 1.7, seed * 0.11)))
    ridge = 1 - min(1.0, abs(n + 0.35 * n2) * 2.6)
    return smoothstep(0.1, 0.55, ridge)


def relieve(kit, obj, depth, radius, seed, along_axis, keep):
    """Cut the fissures into the bark along the part's length; ``keep(co)`` (0-1) fades them out
    toward cut faces."""
    kit.subdivide(obj, levels=1, smooth=False)

    def offset(co, _p):
        fade = 1.0 - keep(co)
        if fade <= 0.0:
            return 0.0
        if along_axis == 2:
            a, along = math.atan2(co.y, co.x), co.z
        else:
            a, along = math.atan2(co.z, co.y), co.x
        return -depth * fade * (1 - fissures(a * radius * 16.0, along * 3.0, seed))
    kit.displace(obj, offset)


def stump(kit, name, radius, height, lobes, flare, bark, wood, seed, rot=0.0, sides=40):
    """A felled stump: noisy trunk, buttress flare into the ground, sloped cut top with a hinge."""
    rng = random.Random(seed)
    phase = rng.uniform(0, math.tau)
    slope = rng.uniform(0.04, 0.09) * radius
    tilt = rng.uniform(0, math.tau)
    zs = [0.0, 0.02, 0.05, 0.09, 0.14, 0.2, 0.28, 0.37, 0.47, 0.6, 0.74, 0.88, 0.96, 1.0]
    rows, coords = [], []
    for k, t in enumerate(zs):
        z = t * height
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            lobe = max(0.0, math.cos(lobes * a + phase + 0.3 * math.sin(3 * a))) ** 1.6
            spread = flare * (1 - smoothstep(0.0, 0.45, t)) ** 1.8 * (0.35 + lobe)
            wob = 1 + 0.05 * noise.noise(Vector((math.cos(a) * 1.7, math.sin(a) * 1.7, z * 4 + seed)))
            r = radius * (1 + spread) * wob
            # The cut face slopes down toward the hinge side; rot softens and caves the rim.
            top = height - slope * (1 + math.cos(a - tilt)) if k == len(zs) - 1 else z
            r *= 1 - rot * 0.35 * smoothstep(0.85, 1.0, t) * (0.5 + 0.5 * noise.noise(Vector((a * 2, seed, 1.0))))
            ring.append((r * math.cos(a), r * math.sin(a), min(top, z) if k == len(zs) - 1 else z))
            co.append((r * math.cos(a), r * math.sin(a), z))
        rows.append(ring)
        coords.append(co)
    # Cut face: rings converge to the heart, dished where the heart has rotted.
    top_ring = rows[-1]
    face_rows, face_coords = [], []
    for f in (0.8, 0.55, 0.3, 0.1):
        ring = [(x * f, y * f, zz - rot * 0.12 * radius * (1 - f) ** 0.5 + 0.004 * noise.noise(Vector((x * 30, y * 30, seed))))
                for x, y, zz in top_ring]
        face_rows.append(ring)
        face_coords.append([(x, y, zz) for x, y, zz in ring])
    obj = kit.loft(name + "_body", rows + face_rows, material=[bark, wood], coords=coords + face_coords,
                   cap_start=True, cap_end=True)
    relieve(kit, obj, min(0.02, 0.06 * radius), radius, seed, 2,
            lambda co: smoothstep(height * 0.78 - slope * 2, height * 0.9 - slope * 2, co.z))
    mesh = obj.data
    for poly in mesh.polygons:
        if poly.normal.z > 0.6 and poly.center.z > height * 0.7:
            poly.material_index = 1
    parts = [obj]
    # Hinge: a ragged strip of torn fibres standing proud on the high side of the cut.
    hx, hy = math.cos(tilt + math.pi), math.sin(tilt + math.pi)
    for i in range(7):
        s = (i - 3) / 3.0
        base = Vector((hx * radius * 0.55 - hy * s * radius * 0.5, hy * radius * 0.55 + hx * s * radius * 0.5, height - 0.01))
        tip = base + Vector((rng.uniform(-0.01, 0.01), rng.uniform(-0.01, 0.01), rng.uniform(0.02, 0.06) * (radius / 0.2) ** 0.5))
        parts.append(kit.tube(f"{name}_hinge{i}", [base, (base + tip) / 2 + Vector((0, 0, 0.004)), tip],
                              radii=[0.012 * radius / 0.2, 0.008 * radius / 0.2, 0.002], sides=6, material=wood))
    return kit.join(parts, name, pivot="base", unwrap=True, reshade=True, smooth_angle=50)


def log(kit, name, length, radius, bark, wood, seed, stubs=2, bend=0.04, sides=28):
    """A fallen log lying along X: tapering, gently bowed, sawn/broken ends and branch stubs."""
    rng = random.Random(seed)
    steps = 26
    rows, coords = [], []
    for i in range(steps + 1):
        u = i / steps
        x = (u - 0.5) * length
        r = radius * (1.12 - 0.3 * u) * (1 + 0.04 * noise.noise(Vector((u * 6, seed, 0.3))))
        cz = bend * length * math.sin(math.pi * u) * 0.3
        cy = bend * length * math.sin(math.pi * u * 1.3 + 0.4) * 0.5
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            # It has settled: the underside flattens where it presses into the leaf litter.
            flat = 1 - 0.18 * max(0.0, -math.sin(a)) ** 3
            ring.append((x, cy + r * math.cos(a), cz + r * math.sin(a) * flat))
            co.append((r * math.cos(a), r * math.sin(a), x))
        rows.append(ring)
        coords.append(co)
    body = kit.loft(name + "_body", rows, material=[bark, wood], coords=coords, cap_start=True, cap_end=True)
    relieve(kit, body, min(0.02, 0.07 * radius), radius, seed, 0, lambda co: smoothstep(length * 0.5 - 0.06, length * 0.5 - 0.015, abs(co.x)))
    mesh = body.data
    for poly in mesh.polygons:
        if abs(poly.normal.x) > 0.85 and abs(poly.center.x) > length * 0.49:
            poly.material_index = 1
    parts = [body]
    for k in range(stubs):
        u = rng.uniform(0.2, 0.8)
        x = (u - 0.5) * length
        a = rng.uniform(0.3, math.pi - 0.3)
        r = radius * (1.12 - 0.3 * u)
        base = Vector((x, r * 0.8 * math.cos(a), r * 0.8 * math.sin(a)))
        direction = Vector((rng.uniform(-0.5, 0.5), math.cos(a), math.sin(a))).normalized()
        stub_len = rng.uniform(0.08, 0.25) * (radius / 0.2) ** 0.7
        parts.append(kit.tube(f"{name}_stub{k}", [base, base + direction * stub_len * 0.6, base + direction * stub_len],
                              radii=[r * 0.32, r * 0.24, r * 0.2], sides=12, material=bark))
    return kit.join(parts, name, pivot="base", unwrap=True, reshade=True, smooth_angle=45)


def bough(kit, name, bark, seed):
    """A fallen bough: a crooked branch with side twigs, lying on the ground."""
    rng = random.Random(seed)
    pts = []
    for i in range(12):
        u = i / 11
        pts.append(Vector(((u - 0.5) * 2.1, 0.12 * math.sin(u * 5.0 + 0.4) + rng.uniform(-0.02, 0.02),
                           0.04 + 0.05 * math.sin(math.pi * u) + rng.uniform(-0.005, 0.005))))
    radii = [0.045 * (1 - 0.7 * i / 11) + 0.006 for i in range(12)]
    parts = [kit.tube(name + "_main", pts, radii=radii, sides=16, material=bark)]
    for k in range(5):
        i = rng.randint(2, 9)
        base = pts[i]
        side = rng.choice((-1, 1))
        direction = Vector((rng.uniform(0.2, 0.8), side * rng.uniform(0.4, 1.0), rng.uniform(-0.05, 0.25))).normalized()
        length = rng.uniform(0.25, 0.6)
        twig = [base + direction * length * t + Vector((0, 0, -0.04 * t * t)) for t in (0.0, 0.35, 0.7, 1.0)]
        parts.append(kit.tube(f"{name}_twig{k}", twig, radii=[radii[i] * 0.55, radii[i] * 0.42, radii[i] * 0.3, 0.004],
                              sides=10, material=bark))
    return kit.join(parts, name, pivot="base", unwrap=True, reshade=True, smooth_angle=45)


def build(kit):
    mats = kit.mats
    # Oak bark measures about 0.08-0.12 linear albedo; the fissures read near black.
    bark = mats.bark("M_StumpBark", light=(0.115, 0.1, 0.08), dark=(0.022, 0.018, 0.014), scale=0.55, lichen=0.3, stretch=0.1)
    mossy = mats.bark("M_StumpBarkMossy", light=(0.1, 0.1, 0.07), dark=(0.02, 0.02, 0.015), scale=0.7, lichen=0.75, stretch=0.12)
    cut = mats.wood("M_StumpCut", light=(0.22, 0.19, 0.15), dark=(0.085, 0.07, 0.052), grain=1.4, roughness=0.84,
                    weathering=0.75, grime=0.35, seed=3.0, relief=1.2)
    fresh = mats.wood("M_LogEnd", light=(0.33, 0.25, 0.15), dark=(0.15, 0.10, 0.06), grain=1.2, roughness=0.78,
                      weathering=0.35, grime=0.25, seed=8.0, relief=1.0)
    twig = mats.bark("M_BoughBark", light=(0.13, 0.115, 0.095), dark=(0.035, 0.03, 0.025), scale=0.25, lichen=0.3, stretch=0.2)
    return [
        stump(kit, "SM_StumpSmall", 0.13, 0.32, 4, 0.35, bark, cut, SEED + 1),
        stump(kit, "SM_StumpLarge", 0.30, 0.48, 5, 0.55, bark, cut, SEED + 2),
        stump(kit, "SM_StumpAncient", 0.58, 0.62, 7, 0.6, mossy, cut, SEED + 3, rot=1.0, sides=56),
        bough(kit, "SM_FallenBough", twig, SEED + 4),
        log(kit, "SM_FallenLog", 3.2, 0.19, bark, fresh, SEED + 5, stubs=3),
        log(kit, "SM_GiantLog", 5.2, 0.42, mossy, fresh, SEED + 6, stubs=4, bend=0.03, sides=40),
    ]
