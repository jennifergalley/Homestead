"""Torch geometry: a knife-shaved hazel stake, a head of pitch-soaked linen strips wound
in overlapping laps, a twine lashing under the head, pitch drips, and (for the wall
torch) a forged wrought-iron sconce.

Torch-local frame: the stake runs up +Z from s = 0 (spike tip or butt); ``place`` moves
the finished parts into the asset frame. Units are meters.
"""
import math

from mathutils import Matrix, Vector, noise

import bpy


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def _noise(x, y, z):
    return noise.noise(Vector((x, y, z)))


class Spec:
    """Dimensions of one torch (meters, torch-local)."""

    def __init__(self, radius, head_z0, head_length, head_radius, spike=0.0, spent=False, seed=0):
        self.radius = radius              # stake radius
        self.head_z0 = head_z0            # bottom edge of the wrapping
        self.head_length = head_length    # cloth body length (before the domed top)
        self.head_radius = head_radius
        self.spike = spike                # length of the knife-cut point (0 = rounded butt)
        self.spent = spent
        self.seed = seed
        if spent:
            # The pitch has burnt off and the upper laps have gone to ash: a shrunken
            # crust of charred cloth round a black, burnt-down stake end.
            self.body = head_length * 0.62
            self.top = head_z0 + self.body + 0.034
        else:
            self.body = head_length
            self.top = head_z0 + head_length + head_radius * 0.52

    def stake_radius(self, s):
        wobble = 0.035 * _noise(s * 9.0, self.seed, 0.4)
        return self.radius * (1 + wobble)


# ---------------------------------------------------------------------- stake

KNOTS = ((0.37, 1.1), (0.58, 4.0), (0.81, 2.3))   # (fraction of stake length, angle)


def stake(kit, spec, mat, name="Stake"):
    """Hazel rod, bark stripped and knife-shaved: 15 sides so a five-facet point cuts on
    exact edges, low nodes where side shoots were trimmed, and either a carved spike or a
    chamfered butt at s = 0. Spent stakes end in a burnt-down, split charcoal cone."""
    top = spec.top + (0.004 if spec.spent else -0.03)
    ss = [0.0, 0.002, 0.005, 0.009]
    s = ss[-1]
    while s < top:
        s += 0.008 if s < max(spec.spike, 0.02) + 0.01 else 0.022
        ss.append(min(s, top))
    cone = spec.head_z0 + spec.body + 0.022
    if spec.spent:
        ss = [v for v in ss if v < cone] + [cone + (top - cone) * t for t in (0.0, 0.25, 0.5, 0.7, 0.85, 0.95, 1.0)]
        ss = sorted(set(round(v, 5) for v in ss))
    radii = []
    for v in ss:
        r = spec.stake_radius(v)
        if not spec.spike:
            r *= 0.72 + 0.28 * math.sqrt(smoothstep(0.0, 0.012, v))
        if spec.spent and v > cone:
            u = (v - cone) / (top - cone)
            r *= max(0.12, 1 - u ** 1.4 * 0.95) * 0.82
        radii.append(r)
    points = [(0.0, 0.0, v) for v in ss]
    roll = math.radians(7)
    obj = kit.tube(name, points, radii=radii, sides=15, material=mat, roll=roll)
    length = top
    facets = [math.pi / 2 + roll + 2 * math.pi * (k + 0.5) / 5 for k in range(5)]
    jitter = [1 + 0.1 * _noise(k * 1.7, spec.seed, 2.0) for k in range(5)]

    def shape(co):
        r = math.hypot(co.x, co.y)
        if r < 1e-7:
            return co
        a = math.atan2(co.y, co.x)
        s = co.z
        if spec.spike and s < spec.spike:
            # Five flat knife facets meeting in a point.
            t = s / spec.spike
            limit = r
            for centre, j in zip(facets, jitter):
                d = spec.radius * (0.06 + 0.84 * t ** 0.92) * j
                c = math.cos(a - centre)
                if c > 0.05:
                    limit = min(limit, d / c)
            r = limit
        for frac, angle in KNOTS:
            dz = (s - frac * length) / 0.009
            da = math.atan2(math.sin(a - angle), math.cos(a - angle)) / 0.45
            r += 0.0034 * math.exp(-dz * dz - da * da)
        if spec.spent and s > cone:
            # Burnt end: checked, split and uneven.
            r *= 1 + 0.18 * _noise(math.cos(a) * 3, math.sin(a) * 3, s * 90 + spec.seed)
        return Vector((math.cos(a) * r, math.sin(a) * r, s))

    kit.warp(obj, shape)
    kit.recalc_normals(obj)
    return obj


# ----------------------------------------------------------------------- head

def _lap(z, angle, pitch, seed):
    """Height of the wound strip at (z, angle): each turn rises across its width and
    drops at the edge where the next turn laps over it. Hand winding is uneven, so the
    edge wanders and each turn sits proud by a different amount."""
    wander = 0.22 * _noise(math.cos(angle) * 1.3, math.sin(angle) * 1.3, z * 20.0 + seed)
    phase = z / pitch - angle / (2 * math.pi) + wander
    turn = math.floor(phase)
    u = phase - turn
    proud = 0.65 + 0.7 * (0.5 + 0.5 * _noise(turn * 1.37, seed, 0.7))
    return proud * (u / 0.84 if u < 0.84 else (1.0 - u) / 0.16)


def head(kit, spec, mat, name="Head", sides=32):
    """Linen strips wound up the stake in overlapping laps and soaked in pitch; the top
    strip is folded down over the end in pleats. Spent heads are a shrunken, cracked
    crust with a ragged burnt edge round the charred stake end."""
    z0, body = spec.head_z0, spec.body
    rh = spec.head_radius * (0.82 if spec.spent else 1.0)
    seed = spec.seed * 3.1
    lap_depth = 0.0042 if not spec.spent else 0.0022
    pitch = 0.042
    rows, coords = [], []
    zs = [z0 - 0.001, z0 + 0.001, z0 + 0.004, z0 + 0.008]
    z = zs[-1]
    step = body / 34
    while z < z0 + body - step * 0.5:
        z += step
        zs.append(z)
    profile = []
    for z in zs:
        t = (z - z0) / body
        bottom = smoothstep(-0.02, 0.09, t) ** 0.5
        # Club-shaped: fattest two thirds up, where the strips were wound back over themselves.
        belly = 0.9 + 0.14 * math.sin(math.pi * min(max(t, 0.0), 1.0) ** 1.6)
        r = spec.radius * 0.96 + (rh * belly - spec.radius * 0.96) * bottom
        profile.append((z, r, 1.0 if t > 0.03 else 0.0, 0.0))
    top_r = profile[-1][1]
    if not spec.spent:
        dome = rh * 0.52
        for k in range(1, 9):
            phi = math.radians(90 * k / 9)
            profile.append((z0 + body + dome * math.sin(phi), top_r * math.cos(phi) + 0.0015 * math.sin(phi),
                            1.0 - k / 9, math.sin(phi) * math.cos(phi) * 2))
    else:
        # Burnt top edge falls in to hug the charred stake.
        for k in range(1, 7):
            u = k / 6
            profile.append((z0 + body + 0.024 * math.sin(u * math.pi / 2),
                            top_r + (spec.radius * 0.62 - top_r) * u ** 0.8, 1.0 - u, 0.0))
    for index, (z, r, lap_weight, pleat) in enumerate(profile):
        ring, pco = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            ca, sa = math.cos(a), math.sin(a)
            lumps = _noise(ca * 1.4 + seed, sa * 1.4, z * 14.0) * 0.0042 \
                + _noise(ca * 4.0, sa * 4.0 + seed, z * 45.0) * 0.0013
            if spec.spent:
                lumps = lumps * 1.5 - 0.004 * max(0.0, _noise(ca * 3.0, sa * 3.0, z * 30.0 + seed))
            rr = r + lap_weight * (lap_depth * _lap(z - z0, a, pitch, seed) + lumps)
            # Pleats where the top strip is folded down over the end.
            rr += pleat * 0.0024 * (0.5 + 0.5 * math.sin(a * 7 + 1.7 * _noise(ca, sa, seed + 2.0)))
            if index == 0:
                rr = r
            zz = z
            if spec.spent and index >= len(profile) - 7:
                # Ragged burnt edge: the crust has flaked away lower on some sides.
                fall = max(0.0, _noise(ca * 2.2, sa * 2.2, seed + 5.0) + 0.15)
                zz -= 0.011 * fall * (index - (len(profile) - 7)) / 6
            ring.append((ca * rr, sa * rr, zz))
            pco.append((ca * rr, sa * rr, zz - z0))
        rows.append(ring)
        coords.append(pco)
    cap = (0.0, 0.0, spec.top) if not spec.spent else False
    obj = kit.loft(name, rows, material=mat, coords=coords, cap_start=False, cap_end=cap)
    return obj


def lashing(kit, spec, mat, name="Lashing", turns=5):
    """Twine lashed tight round the stake just under the wrapping so it cannot slide."""
    cord = 0.0021
    z1 = spec.head_z0 - 0.004
    z0 = z1 - turns * cord * 2.1
    points, radii = [], []
    steps = int(turns * 16)
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        a = 0.6 + 2 * math.pi * turns * t
        r = spec.stake_radius(z) + cord * (0.78 + 0.2 * _noise(t * 9.0, spec.seed, 1.0))
        points.append((math.cos(a) * r, math.sin(a) * r, z))
        radii.append(cord * (0.92 + 0.15 * _noise(t * 27.0, spec.seed, 4.0)))
    # Tucked ends run a little way along the stake.
    return kit.tube(name, points, radii=radii, sides=6, material=mat)


def drips(kit, spec, mat, name="Drip"):
    """Pitch that ran down from the head in the first minutes of burning and set."""
    parts = []
    for k, (angle, length) in enumerate(((0.5, 0.035), (2.6, 0.02), (4.3, 0.05))):
        points, radii = [], []
        for i in range(9):
            t = i / 8
            z = spec.head_z0 + 0.002 - length * t
            r = spec.stake_radius(z) + 0.0009
            a = angle + 0.05 * math.sin(t * 5 + k)
            points.append((math.cos(a) * r, math.sin(a) * r, z))
            radii.append(0.0034 * (1 - 0.65 * t) + 0.0014 * math.exp(-((t - 0.93) / 0.07) ** 2))
        parts.append(kit.tube(f"{name}{k}", points, radii=radii, sides=6, material=mat))
    return parts


def torch(kit, spec, mats, polish=None):
    """All parts of one torch in the torch-local frame."""
    parts = [stake(kit, spec, mats["wood"]), head(kit, spec, mats["head"]),
             lashing(kit, spec, mats["cord"])]
    if not spec.spent:
        parts += drips(kit, spec, mats["head"])
    return parts


def place(objs, matrix):
    for obj in objs:
        obj.data.transform(matrix)
        obj.data.update()


def bow(objs, amount, length, seed=0):
    """A hazel rod is never quite straight: bend everything gently in X."""
    for obj in objs:
        kit_warp = [v for v in obj.data.vertices]
        for v in kit_warp:
            t = v.co.z / length
            v.co.x += amount * math.sin(math.pi * t) + amount * 0.3 * math.sin(2.1 * math.pi * t + seed)
        obj.data.update()


# ------------------------------------------------------------------- materials

def materials(kit, spec, suffix=""):
    m = kit.mats
    char_top = spec.head_z0
    if spec.spent:
        wood = m.wood("M_TorchStake" + suffix, light=(0.25, 0.19, 0.125), dark=(0.11, 0.075, 0.042), grain=0.55,
                      roughness=0.72, weathering=0.4, grime=0.5, seed=3.0 + spec.seed,
                      char_above=char_top - 0.012, char_band=0.03, soot_band=0.14)
        cord = m.rawhide("M_TorchCord" + suffix, color=(0.05, 0.035, 0.022), strands=3, twist=70.0)
    else:
        wood = m.wood("M_TorchStake" + suffix, light=(0.28, 0.21, 0.135), dark=(0.12, 0.08, 0.045), grain=0.55,
                      roughness=0.7, weathering=0.35, grime=0.45, seed=3.0 + spec.seed,
                      char_above=char_top + 0.004, char_band=0.008, soot_band=0.1)
        cord = m.rawhide("M_TorchCord" + suffix, color=(0.26, 0.19, 0.11), strands=3, twist=70.0)
    rag = m.pitch_rag("M_TorchHead" + suffix, spent=spec.spent, seed=spec.seed)
    return {"wood": wood, "head": rag, "cord": cord}


# ---------------------------------------------------------------------- iron

def _strap_rows(path, width, thickness, x_scale=None):
    """Rectangular (slightly rounded) sections swept along a 2D path in the Y-Z plane."""
    rows = []
    count = len(path)
    for i, (y, z) in enumerate(path):
        a = Vector(path[max(i - 1, 0)])
        b = Vector(path[min(i + 1, count - 1)])
        tangent = (b - a).normalized()
        normal = Vector((-tangent.y, tangent.x))
        w = width(i / (count - 1)) if callable(width) else width
        h = thickness / 2
        corner = min(0.0012, w * 0.3, h * 0.6)
        ring = []
        for sx, sn in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
            # Two points per corner give a small hammered chamfer.
            first = ((w - corner) * sx, h * sn) if sx * sn > 0 else (w * sx, (h - corner) * sn)
            second = (w * sx, (h - corner) * sn) if sx * sn > 0 else ((w - corner) * sx, h * sn)
            for px, pn in (first, second):
                off = normal * pn
                ring.append((px, y + off.x, z + off.y))
        rows.append(ring)
    return rows


def sconce(kit, iron, ring_centres, ring_radius=0.0215):
    """Forged sconce, back face on the wall plane y = 0 (wall on -Y): a nailed strap with a
    ram's-horn scroll at the top and a drawn point below, a twisted square arm to a collar
    ring, and a shorter arm to the lower ring the butt rests in."""
    parts = []
    thick = 0.0068
    # Back strap, from the drawn bottom point up to the scroll.
    path = [(thick / 2 + 0.0035 * smoothstep(-0.215, -0.235, z), z)
            for z in [-0.238 + 0.004 * i for i in range(6)]]
    path += [(thick / 2, z) for z in [-0.215 + 0.0145 * i for i in range(26)]]
    top = path[-1][1]
    centre = (thick / 2 + 0.0165, top)
    for k in range(1, 30):
        phi = math.pi - k * math.radians(12)
        r = 0.0165 * (1 - 0.42 * k / 29)
        path.append((centre[0] + r * math.cos(phi), centre[1] + r * math.sin(phi)))

    def width(t):
        base = 0.0215 + 0.0012 * math.sin(t * 17.0)
        point = smoothstep(0.0, 0.2, t) ** 0.6
        scroll = 1 - 0.45 * smoothstep(0.72, 1.0, t)
        return base * (0.25 + 0.75 * point) * scroll

    rows = _strap_rows(path, width, thick)
    plate = kit.loft("SconcePlate", rows, material=iron, cap_start=True, cap_end=True)
    kit.displace(plate, lambda co, pco: 0.00045 * _noise(co.x * 90, co.y * 90, co.z * 90))
    parts.append(plate)

    # Rose-head nails.
    for index, z in enumerate((0.125, -0.182)):
        rows = []
        for y, half in ((thick - 0.0005, 0.0068), (thick + 0.0022, 0.0056), (thick + 0.0042, 0.0026)):
            ring = []
            for j in range(4):
                a = math.radians(45 + 90 * j + 9 * index)
                ring.append((half * 1.41 * math.cos(a), y, z + half * 1.41 * math.sin(a)))
            rows.append(ring)
        parts.append(kit.loft(f"Nail{index}", rows, material=iron, cap_start=True,
                              cap_end=(0.0, thick + 0.0052, z)))

    # Arms: square bar forged into the plate, twisted in the middle, riveted to the rings.
    for index, (cy, cz) in enumerate(ring_centres):
        near = cy - ring_radius + 0.0018
        half = 0.0058 if index == 0 else 0.005
        rows = []
        count = 22
        for i in range(count + 1):
            t = i / count
            y = 0.003 + (near - 0.003) * t
            twist = math.radians(45) + (math.radians(360) * smoothstep(0.25, 0.75, t) if index == 0 else 0.0)
            # Fillet where the arm is fire-welded to the strap.
            flare = 1 + 0.5 * math.exp(-(y - 0.003) / 0.004)
            ring = []
            for j in range(4):
                a = twist + math.pi / 2 * j
                ring.append((half * flare * 1.2 * math.cos(a), y, cz + half * flare * math.sin(a)))
            rows.append(ring)
        parts.append(kit.loft(f"Arm{index}", rows, material=iron, cap_start=True, cap_end=True))

        # Collar ring: a flat band bent round, its section swept round the circle.
        band = 0.015 if index == 0 else 0.011
        wall = 0.0025
        sides = 36
        rows = []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            wobble = 0.0003 * _noise(math.cos(a) * 4, math.sin(a) * 4, index + 0.5)
            section = [(ring_radius + wall - 0.0006, -band / 2), (ring_radius + wall, -band / 2 + 0.0008),
                       (ring_radius + wall, band / 2 - 0.0008), (ring_radius + wall - 0.0006, band / 2),
                       (ring_radius - wall, band / 2), (ring_radius - wall, -band / 2)]
            rows.append([(math.cos(a) * (r + wobble), cy + math.sin(a) * (r + wobble), cz + zz)
                         for r, zz in section])
        parts.append(kit.loft(f"Ring{index}", rows, material=iron, cyclic=True))
    return parts


# --------------------------------------------------------------------- finish

def finish(kit, obj, lod_ratios=(0.5, 0.2), margin=0.004, smooth_angle=50.0):
    """Unwrap LOD0 once (the bake keeps these UVs) and decimate LODs that share them."""
    import homestead_rocks as rocks
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle)
    kit.pack_uvs(obj, margin=margin)
    meshes = [obj]
    for index, ratio in enumerate(lod_ratios, start=1):
        lod = rocks.lod(kit, obj, f"{obj.name}_LOD{index}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle))
    return meshes


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def head_top(spec, matrix=Matrix.Identity(4)):
    return matrix @ Vector((0.0, 0.0, spec.top))
