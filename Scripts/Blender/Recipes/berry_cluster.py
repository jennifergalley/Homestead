"""Hand-picked sprig of ripe wild blackberries: five berries on a short bramble stem
with one small trifoliate leaf, as she holds it pinched between finger and thumb.

Real-object research (written before modeling):
- Native Pacific/California blackberry (Rubus ursinus) and the naturalised Himalayan
  blackberry grow along Sierra foothill creeks and clearings. Wild fruit is smaller than
  cultivated: ~1.2-2.0 cm long, ~1.0-1.5 cm across, oblong-conical with a blunt tip.
- Each berry is an aggregate of ~20-50 drupelets, 3-4 mm each: glossy, near-black when
  ripe (reflects only a few percent), deep purple-red in the gaps; each drupelet keeps a
  tiny dry style. Five small hairy sepals stay reflexed back against the pedicel.
- Fruit hangs in loose terminal clusters (cymes) on 0.5-2 cm prickly pedicels; the cane
  and pedicels are green flushed purple-red with small backward-hooked prickles.
- Leaves are compound, usually 3 (or 5) ovate, pointed, sharply double-toothed
  leaflets, dark green and quilted above with sunken veins, paler and felted beneath.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the pinch point on the main stem, 1.2 cm below its cut end. The
stem runs along +Z above the pivot (cut end at z +0.012); the berries hang below it
(-Z, lowest at about z -0.039), the leaf reaches out along +X. Hold it by the origin.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import random

from mathutils import Matrix, Vector, noise

NAME = "BerryCluster"
DESCRIPTION = ("Hand-picked sprig of five ripe wild blackberries with one trifoliate leaf "
               "(original). Pivot = pinch point on the stem; stem +Z, berries hang -Z.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
# Lay it on its side for review; close-up on the berries.
BEAUTY = {"pose": (90, 0, 25), "focus": (0.0, 0.0, -0.022)}

SEED = 7711
UP = Vector((0, 0, 1))


def frame(axis):
    """Two unit vectors perpendicular to ``axis``."""
    axis = axis.normalized()
    helper = Vector((1, 0, 0)) if abs(axis.x) < 0.8 else Vector((0, 1, 0))
    e1 = axis.cross(helper).normalized()
    return e1, axis.cross(e1).normalized()


def berry(kit, name, material, top, axis, length, width, seed, sides=12, rows=10):
    """One aggregate berry hanging from ``top`` along ``axis``: oblong with a blunt tip
    and a flatter shoulder, gently lumpy (drupelet domes live in the material)."""
    axis = axis.normalized()
    e1, e2 = frame(axis)
    offset = Vector((seed * 0.071, seed * 0.043, seed * 0.029))
    rings, coords = [], []
    for i in range(rows):
        t = 0.04 + 0.92 * (i / (rows - 1))
        t = 0.5 - 0.5 * math.cos(math.pi * t)                     # denser at both ends
        h = length * t
        profile = math.sin(math.pi * (t ** 0.85)) ** 0.5
        radius = 0.5 * width * profile * (1.0 - 0.1 * t)
        ring, local = [], []
        for j in range(sides):
            a = 2 * math.pi * (j + 0.5 * (i % 2)) / sides
            d = math.cos(a) * e1 + math.sin(a) * e2
            probe = Vector((math.cos(a) * 1.7, math.sin(a) * 1.7, t * 3.0)) + offset * 40
            r = radius * (1.0 + 0.055 * noise.noise(probe))
            ring.append(tuple(top + axis * h + d * r))
            local.append(tuple(Vector((math.cos(a) * r, math.sin(a) * r, h)) + offset))
        rings.append(ring)
        coords.append(local)
    obj = kit.loft(name, rings, material=material, coords=coords)
    return kit.recalc_normals(obj)


def sepals(kit, name, material, top, axis, width, seed, count=5):
    """Small pointed sepals reflexed back up the pedicel from the berry's shoulder."""
    rng = random.Random(seed)
    axis = axis.normalized()
    e1, e2 = frame(axis)
    parts = []
    for k in range(count):
        a = 2 * math.pi * (k + rng.uniform(-0.15, 0.15)) / count
        d = math.cos(a) * e1 + math.sin(a) * e2
        base = top + axis * (0.05 * width) + d * (0.14 * width)
        mid = base + d * (0.09 * width) - axis * (0.06 * width)
        tip = mid + d * (0.04 * width) - axis * (0.17 * width) * rng.uniform(0.7, 1.1)
        parts.append(kit.tube(f"{name}_{k}", [base, mid, tip], radius=lambda t: 0.001 * (1 - t) ** 0.8 + 0.00005,
                              sides=3, material=material, roll=a))
    return parts


def stalk(kit, name, material, points, r0, r1, sides=5):
    return kit.tube(name, points, radius=lambda t: r0 + (r1 - r0) * t, sides=sides, material=material)


def prickle(kit, name, material, base, out, length=0.0022):
    """A small backward-hooked bramble prickle."""
    down = Vector((0, 0, -1))
    mid = base + out * (0.55 * length) + down * (0.15 * length)
    tip = base + out * (0.8 * length) + down * (0.65 * length)
    return kit.tube(name, [base - out * 0.0003, mid, tip], radius=lambda t: 0.00055 * (1 - t) + 0.00003,
                    sides=4, material=material)


def berry_on_pedicel(kit, name, mats, node, out, reach, drop, length, width, seed, lowres=False):
    """Pedicel from ``node`` arching out along ``out`` then down; berry and sepals at its end."""
    out = out.normalized()
    end = node + out * reach + Vector((0, 0, -drop))
    mid = node + out * (0.55 * reach) + Vector((0, 0, -0.25 * drop))
    axis = (out * (reach / max(drop + reach, 1e-6)) * 1.1 + Vector((0, 0, -1))).normalized()
    parts = [stalk(kit, name + "Ped", mats["stem"], [node, mid, end], 0.00062, 0.0005,
                   sides=4 if lowres else 5)]
    parts.append(berry(kit, name, mats["berry"], end, axis, length, width, seed,
                       sides=10 if lowres else 12, rows=8 if lowres else 10))
    if not lowres:
        parts += sepals(kit, name + "Sep", mats["sepal"], end, axis, width, seed)
    return parts


def leaflet(kit, name, front, back, base, direction, normal, length, width, seed, cols=5, rows=11):
    """Double-sided ovate, pointed, toothed leaflet (a thin closed shell so it never
    shows a back-face hole). pcoord = (u across, v base->tip, 0) for leaf_pcoord."""
    rng = random.Random(seed)
    direction = direction.normalized()
    side = normal.cross(direction).normalized()
    normal = direction.cross(side).normalized()
    thickness = 0.00028

    def point(u, v):
        half = 0.5 * width * math.sin(math.pi * v ** 0.72) ** 0.85
        across = (u - 0.5) * 2.0
        if abs(across) > 0.99 and 0.06 < v < 0.97:
            tooth = (v * 7.5 + 0.3 * rng.random()) % 1.0
            half *= 0.87 + 0.13 * tooth                             # forward-pointing teeth
        lateral = across * half
        lift = abs(lateral) * 0.42 + 0.0012 * noise.noise(Vector((u * 3, v * 4, seed)))
        droop = -0.22 * length * v ** 2.4
        return base + direction * (v * length) + side * lateral + normal * (lift + droop)

    grid, coords = [], []
    for i in range(1, rows):
        v = i / rows
        for j in range(cols):
            u = j / (cols - 1)
            grid.append(point(u, v))
            coords.append((u, v, 0.0))
    verts = [base] + grid + [point(0.5, 1.0)]
    coords = [(0.5, 0.0, 0.0)] + coords + [(0.5, 1.0, 0.0)]
    tip = len(verts) - 1

    def g(i, j):
        return 1 + i * cols + j

    faces = []
    for j in range(cols - 1):
        faces.append((0, g(0, j + 1), g(0, j)))
        faces.append((tip, g(rows - 2, j), g(rows - 2, j + 1)))
    for i in range(rows - 2):
        for j in range(cols - 1):
            faces.append((g(i, j), g(i, j + 1), g(i + 1, j + 1), g(i + 1, j)))
    n = len(verts)
    shifted = [vv - normal * thickness for vv in verts]
    back_faces = [tuple(reversed([f + n for f in face])) for face in faces]
    loop = [0] + [g(i, 0) for i in range(rows - 1)] + [tip] + [g(i, cols - 1) for i in reversed(range(rows - 1))]
    rim = [(a, b, b + n, a + n) for a, b in zip(loop, loop[1:] + loop[:1])]
    obj = kit.mesh(name, verts + shifted, faces + back_faces + rim)
    kit.tag_coords(obj.data, [Vector(c) for c in coords + coords])
    obj.data.materials.append(front)
    obj.data.materials.append(back)
    front_count = len(faces)
    for index, poly in enumerate(obj.data.polygons):
        poly.material_index = 0 if index < front_count else 1
    return kit.recalc_normals(obj)


def trifoliate(kit, name, mats, node, out, seed):
    """Short petiole from ``node`` with a terminal and two lateral leaflets."""
    out = out.normalized()
    tip = node + out * 0.011 + Vector((0, 0, 0.004))
    mid = node + out * 0.005 + Vector((0, 0, 0.0028))
    parts = [stalk(kit, name + "Petiole", mats["stem"], [node, mid, tip], 0.0007, 0.0005)]
    parts.append(prickle(kit, name + "Pr", mats["stem"], node + out * 0.006 + Vector((0, 0, 0.0028)),
                         Vector((0, 0, -1)).cross(out).normalized(), 0.0016))
    tilt = Vector((0, 0, 1))
    heading = out + Vector((0, 0, 0.12))
    parts.append(leaflet(kit, name + "T", mats["leaf"], mats["leaf_under"], tip + out * 0.0015,
                         heading, tilt, 0.026, 0.017, seed))
    for sign, k in ((1, 1), (-1, 2)):
        sideways = Matrix.Rotation(sign * math.radians(62), 3, "Z") @ out
        parts.append(leaflet(kit, name + f"L{k}", mats["leaf"], mats["leaf_under"], tip + sideways * 0.0008,
                             sideways + Vector((0, 0, -0.05)), tilt + sideways * 0.15 * sign, 0.020, 0.0135,
                             seed + k))
    return parts


def materials(kit):
    m = kit.mats
    return {
        "berry": m.blackberry("M_Blackberry", seed=0.0),
        "berry_purple": m.blackberry("M_BlackberryPurple", red=0.35, seed=1.0),
        "stem": m.stem("M_BrambleStem", color=(0.06, 0.08, 0.026), dark=(0.07, 0.022, 0.02), roughness=0.72),
        "cut": m.stem("M_BrambleCut", color=(0.34, 0.33, 0.18), dark=(0.22, 0.22, 0.11), roughness=0.6),
        "sepal": m.stem("M_BrambleSepal", color=(0.07, 0.085, 0.04), dark=(0.05, 0.035, 0.022), roughness=0.8),
        "leaf": m.leaf_pcoord("M_BrambleLeaf", color=(0.035, 0.075, 0.018), vein=(0.07, 0.12, 0.035),
                              tip=(0.05, 0.09, 0.025), roughness=0.48, serrate_dark=0.4),
        "leaf_under": m.leaf_pcoord("M_BrambleLeafUnder", color=(0.11, 0.14, 0.075), vein=(0.15, 0.17, 0.10),
                                    tip=(0.12, 0.14, 0.08), roughness=0.78, rugose=0.4),
    }


def build(kit):
    mats = materials(kit)
    stem = kit.tube("MainStem", [(0.0, 0.0, 0.012), (0.0003, 0.0, 0.004), (0.0, 0.0001, -0.004),
                                 (-0.0005, 0.0003, -0.012), (-0.0008, 0.0005, -0.0165)],
                    radius=lambda t: 0.0013 - 0.0004 * t, sides=7)
    stem.data.materials.append(mats["stem"])
    stem.data.materials.append(mats["cut"])
    body_faces = 4 * 7
    for index, poly in enumerate(stem.data.polygons):
        poly.material_index = 1 if body_faces <= index < body_faces + 7 else 0   # top cap = cut face
    parts = [stem]
    for k, (z, az) in enumerate(((0.007, 200), (-0.001, 95), (0.003, 300))):
        a = math.radians(az)
        out = Vector((math.cos(a), math.sin(a), 0))
        parts.append(prickle(kit, f"Prickle{k}", mats["stem"], Vector((0, 0, z)) + out * 0.0011, out))

    parts += trifoliate(kit, "Leaf", mats, Vector((0.0004, 0.0, -0.005)), Vector((1.0, -0.25, 0.0)), SEED)

    parts += berry_on_pedicel(kit, "BerryC", mats, Vector((-0.0008, 0.0005, -0.0162)), Vector((0.2, -0.1, 0)),
                              0.001, 0.0045, 0.0185, 0.0145, 1)
    laterals = ((-0.0085, 45, 0.0085, 0.0035, 0.0165, 0.0135, "berry_purple"),
                (-0.0100, 140, 0.0095, 0.0040, 0.0175, 0.0140, "berry"),
                (-0.0115, 235, 0.0090, 0.0038, 0.0160, 0.0130, "berry"),
                (-0.0130, 320, 0.0080, 0.0042, 0.0170, 0.0138, "berry"))
    for k, (z, az, reach, drop, length, width, material) in enumerate(laterals):
        a = math.radians(az)
        out = Vector((math.cos(a), math.sin(a), 0))
        local = dict(mats, berry=mats[material])
        parts += berry_on_pedicel(kit, f"Berry{k}", local, Vector((-0.0006, 0.0004, z)), out,
                                  reach, drop, length, width, 2 + k)
    return kit.join(parts, "SM_BerryCluster", pivot=None, unwrap=False, reshade=True, smooth_angle=60)
