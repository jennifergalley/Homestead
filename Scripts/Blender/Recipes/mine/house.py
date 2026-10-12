"""Mine house geometry (house frame of ``layout``; metres, Unreal axes; ``Masonry.build`` mirrors Y).

Walls are coursed granite rubble with dressed long-and-short quoins at the corners and jambs,
round arches turned in dressed voussoirs, granite sills, a lime-mortar core behind the joints, and,
on the ground floor, limewashed rubble inside. The north-west corner is the stack. The roof is
slate on a pitched deck; floors and joinery are timber.
"""
import math

import numpy as np

from mine import layout as L
from stone_building.masonry import Masonry

MAT_STONE, MAT_MORTAR, MAT_WASH = 0, 1, 2
JOINT = 0.018                 # joint between stones (m)
RING = 0.30                   # voussoir ring depth (m); the big bob-wall arch takes RING_BIG
RING_BIG = 0.42
JAMB = (0.26, 0.42)           # jamb quoins, short and long (m)
QUOIN = (0.42, 0.72)          # corner quoins, short and long (m)
QUOIN_PROUD = 0.02            # corner quoins stand this far proud of the wall plane (m)
SILL_H, SILL_PROUD = 0.12, 0.05
MORTAR_RECESS = 0.028
BOTTOM = -0.35                # buried footing course
WASH_TOP = L.CEILING_Z + 0.25 # limewash inside up to just above the ceiling joists


# --------------------------------------------------------------------------- wall frames

class Wall:
    """One wall: outer face plane, thickness, u axis along the face, its u range and its openings."""

    def __init__(self, key):
        self.key = key
        if key in ("+Y", "-Y"):
            s = 1.0 if key == "+Y" else -1.0
            self.outer, self.t = s * L.OUT_Y / 2, L.WALL
            self.n = np.array([0.0, s, 0.0])
            self.u0, self.u1 = -L.OUT_X / 2, L.OUT_X / 2
        else:
            s = 1.0 if key == "+X" else -1.0
            self.outer = s * L.OUT_X / 2
            self.t = L.WALL if key == "+X" else L.BOB_WALL
            self.n = np.array([s, 0.0, 0.0])
            self.u0, self.u1 = -L.OUT_Y / 2, L.OUT_Y / 2
        self.gable = key in ("+X", "-X")
        self.openings = [o for o in [L.DOOR] + L.WINDOWS if o["wall"] == key]

    def p(self, u, v, depth):
        """Point on the wall at u along it, height v, ``depth`` in from the outer face."""
        s = self.n[0] + self.n[1]
        if self.key in ("+Y", "-Y"):
            return np.array([u, self.outer - s * depth, v])
        return np.array([self.outer - s * depth, u, v])

    def u_dir(self):
        return np.array([1.0, 0.0, 0.0]) if self.key in ("+Y", "-Y") else np.array([0.0, 1.0, 0.0])

    def top(self, u):
        """Top of the wall at u: the eaves, or the gable's raking line."""
        if not self.gable:
            return L.EAVES_Z
        return L.EAVES_Z + max(0.0, L.OUT_Y / 2 - abs(u)) * math.tan(math.radians(L.ROOF_PITCH))


def stack_box():
    sx, sy = L.STACK["centre"]
    h = L.STACK["half"]
    return (sx - h, sx + h, sy - h, sy + h)


def corner_extent(wall, k, end):
    """How far the corner quoin (or the stack) at ``end`` (-1 or +1 along u) reaches along this
    wall's face in course k. N/S faces carry the long side on even courses, E/W on odd ones."""
    x0, x1, y0, y1 = stack_box()
    if wall.key == "+Y" and end < 0:
        return x1 - x0
    if wall.key == "-X" and end > 0:
        return y1 - y0
    long_here = (k % 2 == 0) == (wall.key in ("+Y", "-Y"))
    return QUOIN[1] if long_here else QUOIN[0]


# --------------------------------------------------------------------------- courses and openings

def courses(seed=7):
    rng = np.random.default_rng(seed)
    out, z = [], BOTTOM
    while z < L.RIDGE_Z + 0.2:
        h = rng.uniform(0.24, 0.36) if z < 1.2 else rng.uniform(0.17, 0.30)
        out.append((z, z + h))
        z += h
    return out


def ring_of(o):
    return RING_BIG if o.get("boarded") else RING


def opening_interval(o, z0, z1, k):
    """The u half-width an opening (with its jambs, sill and voussoirs) takes out of course z0..z1,
    or None. Jamb width alternates by course so the dressing toothes into the rubble."""
    half = o["width"] / 2
    sill, spring = o["sill"], o["spring"]
    ring = ring_of(o)
    lo = sill - (SILL_H if o is not L.DOOR else 0.0)
    if z1 <= lo or z0 >= spring + half + ring:
        return None
    zm = 0.5 * (z0 + z1)
    widths = []
    if z0 < spring:
        widths.append(half + JAMB[k % 2] + 0.06 if zm >= sill else half + 0.12)
    if z1 > spring:
        dz = max(z0, spring) - spring
        r = half + ring
        widths.append(math.sqrt(max(r * r - dz * dz, 0.0)) + 0.02)
    return max(widths)


def free_intervals(wall, k, z0, z1):
    """Intervals of u left for rubble in course k after corners, stack, openings and the gable rake."""
    a = wall.u0 + corner_extent(wall, k, -1) + JOINT
    b = wall.u1 - corner_extent(wall, k, +1) - JOINT
    if wall.gable and z0 > L.EAVES_Z:
        reach = L.OUT_Y / 2 - (z1 - L.EAVES_Z) / math.tan(math.radians(L.ROOF_PITCH))
        a, b = max(a, -reach), min(b, reach)
    if b - a < 0.12:
        return []
    cuts = []
    for o in wall.openings:
        w = opening_interval(o, z0, z1, k)
        if w is not None:
            cuts.append((o["centre"] - w, o["centre"] + w))
    out, cur = [], a
    for c0, c1 in sorted(cuts):
        if c0 > cur + 0.1:
            out.append((cur, min(c0, b)))
        cur = max(cur, c1)
    if b > cur + 0.1:
        out.append((cur, b))
    return [(x, y) for x, y in out if y - x > 0.1]


# --------------------------------------------------------------------------- masonry

def _cells(a, b, length, rng, joints_below, min_last=0.16):
    cells, x = [], a
    while b - x > 1e-6:
        end = x + length()
        for j in joints_below:
            if abs(end - j) < 0.07:
                end = j + (0.07 if end >= j else -0.07)
        if b - end < min_last:
            end = b
        cells.append((x, min(end, b)))
        x = min(end, b)
    if len(cells) > 1 and cells[-1][1] - cells[-1][0] < min_last * 0.85:
        cells[-2] = (cells[-2][0], cells[-1][1])
        cells.pop()
    return cells


# Jenny (review 3): "stones look like flat tiles stuck on smooth plaster, with far too much flat mortar between
# them. Real rubble walls are packed stone with thin, recessed mortar joints (stones should have depth/relief;
# use pinning stones in gaps)". So the stones fill their cells (squarer outlines, at most one spall cut, a finer
# outline), stand proud of the recessed joints by 2-5 cm, and pinning stones plug the gaps where joints meet.
STONE_BULGE = (0.014, 0.038)
STONE_OUT = 0.04
STONE_EXPONENT = (9.0, 16.0)
PIN_CHANCE = 0.55


def _stone(m, wall, ua, ub, za, zb, depth, normal, mat, rng, bulge=STONE_BULGE, max_out=STONE_OUT):
    angle = rng.normal(0.0, math.radians(1.5))
    hu = (ub - ua) / 2 - JOINT / 2 - abs(angle) * (zb - za) * 0.5
    hv = max((zb - za) / 2 - JOINT / 2 - rng.uniform(0.0, 0.005) - abs(angle) * (ub - ua) * 0.5, 0.03)
    cz = za + JOINT / 2 + hv + rng.uniform(0.0, 0.003)
    up = np.array([0.0, 0.0, 1.0]) * math.cos(angle) + wall.u_dir() * math.sin(angle)
    m.pillow(wall.p((ua + ub) / 2, cz, depth), normal, up, hu, hv, mat=mat, bulge=rng.uniform(*bulge),
             max_out=max_out, exponent=rng.uniform(*STONE_EXPONENT), cuts=(0, 1), sides=22, roll=0.008,
             relief=0.012)


def _pins(m, wall, joints, z0, z1, depth, normal, mat, rng):
    """Pinning stones (spalls) driven into the joints where a course's vertical joints meet the bed joints."""
    for u in joints:
        for z, sign in ((z0, 1.0), (z1, -1.0)):
            if rng.random() > PIN_CHANCE:
                continue
            hu, hv = rng.uniform(0.028, 0.055), rng.uniform(0.018, 0.034)
            centre = wall.p(u + rng.normal(0.0, 0.01), z + sign * rng.uniform(0.0, 0.02), depth)
            up = np.array([0.0, 0.0, 1.0])
            m.pillow(centre, normal, up, hu, hv, mat=mat, bulge=rng.uniform(0.004, 0.012), max_out=0.012,
                     recess=(-0.016, -0.006), cuts=(1, 2), sides=10, roll=0.004, skirt=0.03)


def rubble(m, wall, z0, z1, intervals, rng, joints_below, inner=False):
    """Random rubble brought to course on the outer (or inner) face; some cells are two stones stacked."""
    depth = wall.t if inner else 0.0
    normal = -wall.n if inner else wall.n
    mat = MAT_WASH if inner else MAT_STONE
    h = z1 - z0
    out = []
    for a, b in intervals:
        cells = _cells(a, b, lambda: float(np.clip(h * rng.uniform(1.3, 2.8), 0.24, 0.85)), rng,
                       [j for j in joints_below if a < j < b])
        for ua, ub in cells:
            if h >= 0.22 and ub - ua > 0.3 and rng.random() < 0.3:
                zm = z0 + h * rng.uniform(0.35, 0.65)
                for c, d in ((z0, zm), (zm, z1)):
                    for sa, sb in _cells(ua, ub, lambda: float(np.clip((d - c) * rng.uniform(1.8, 3.4), 0.18, 0.7)),
                                         rng, [], min_last=0.14):
                        _stone(m, wall, sa, sb, c, d, depth, normal, mat, rng)
            else:
                _stone(m, wall, ua, ub, z0, z1, depth, normal, mat, rng)
        _pins(m, wall, [ub for _, ub in cells[:-1]], z0, z1, depth, normal, mat, rng)
        out += [ub for _, ub in cells[:-1]]
    return out


def box_on(m, wall, ua, ub, za, zb, d0, d1, mat, radius=0.02, bulge=0.004, skip=(), relief=0.003):
    """An axis-aligned dressed block on a wall between u, z and depth ranges."""
    p0, p1 = wall.p(ua, za, d0), wall.p(ub, zb, d1)
    lo, hi = np.minimum(p0, p1), np.maximum(p0, p1)
    m.block(lo, hi, mat=mat, radius=radius, relief=relief, step=0.2, bulge=bulge, skip=skip)


def corner_quoins(m, wall, k, z0, z1, rng):
    """N/S walls carry the corner quoins (both faces); the stack replaces the north-west one."""
    if wall.key not in ("+Y", "-Y"):
        return
    for end in (-1, 1):
        if wall.key == "+Y" and end < 0:
            continue
        x_out = L.OUT_X / 2 * end
        along = corner_extent(wall, k, end)
        across = QUOIN[1] if k % 2 else QUOIN[0]
        s = wall.n[1]
        # Dressed quoins stand a little proud of the rubble's general face, as the long-and-short work does.
        x0, x1 = sorted((x_out + end * QUOIN_PROUD, x_out - end * along))
        y0, y1 = sorted((wall.outer + s * QUOIN_PROUD, wall.outer - s * across))
        m.block(np.array([x0, y0, z0 + JOINT / 2]), np.array([x1, y1, z1 - JOINT / 2]), mat=MAT_STONE,
                radius=0.02, relief=0.003, step=0.2, bulge=0.006 * rng.uniform(0.5, 1.0),
                skip=(("z", -1), ("z", 1)))


def opening_dressing(m, wall, o, cs, rng):
    """Jamb quoins through the wall, the sill, the voussoir ring and the arch's soffit band."""
    half, sill, spring, c = o["width"] / 2, o["sill"], o["spring"], o["centre"]
    ring = ring_of(o)
    for k, (z0, z1) in enumerate(cs):
        za, zb = max(z0, sill), min(z1, spring)
        if zb - za < 0.05:
            continue
        w = JAMB[k % 2]
        for side in (-1, 1):
            ua, ub = sorted((c + side * half, c + side * (half + w)))
            box_on(m, wall, ua, ub, za + JOINT / 2, zb - JOINT / 2, -0.004, wall.t + 0.004, MAT_STONE,
                   skip=(("z", -1), ("z", 1)))
    if o is not L.DOOR:
        box_on(m, wall, c - half - 0.14, c + half + 0.14, sill - SILL_H, sill, -SILL_PROUD, wall.t * 0.55, MAT_STONE,
               radius=0.015, bulge=0.003)
    else:
        box_on(m, wall, c - half - 0.06, c + half + 0.06, BOTTOM, sill, -0.10, wall.t, MAT_STONE, radius=0.012,
               bulge=0.002)
    # Voussoirs: an odd number of wedge faces round the arch, keystone at the crown.
    rm = half + ring / 2
    n = int(round(math.pi * rm / 0.21))
    n += 1 - n % 2
    for i in range(n):
        th = math.pi * (i + 0.5) / n
        radial = wall.u_dir() * math.cos(th) + np.array([0.0, 0.0, math.sin(th)])
        centre = wall.p(c, spring, 0.0) + radial * rm
        hu = math.pi * rm / n / 2 - JOINT / 2
        hv = ring / 2 - JOINT / 2 + (0.03 if i == n // 2 else 0.0)
        m.pillow(centre, wall.n, radial, hu, hv, mat=MAT_STONE, bulge=0.006, max_out=0.02, cuts=(0, 1),
                 exponent=14.0)
    arch_band(m, wall, c, spring, half, half + ring, MAT_STONE)


def arch_band(m, wall, c, spring, r0, r1, mat, segs=28):
    """The arch's soffit (r0) through the wall, with an annulus at the back (inner face) and one just
    behind the voussoirs at the front (covers the joints between them)."""
    verts, faces = [], []
    for i in range(segs + 1):
        th = math.pi * i / segs
        radial = wall.u_dir() * math.cos(th) + np.array([0.0, 0.0, math.sin(th)])
        base = wall.p(c, spring, 0.0)
        for r, d in ((r0, MORTAR_RECESS), (r0, wall.t), (r1, wall.t), (r1, MORTAR_RECESS)):
            q = base + radial * r
            q = q - wall.n * d
            verts.append(q)
    for i in range(segs):
        a, b = i * 4, (i + 1) * 4
        faces.append((a + 0, b + 0, b + 1, a + 1))   # soffit
        faces.append((a + 1, b + 1, b + 2, a + 2))   # back annulus
        faces.append((a + 2, b + 2, b + 3, a + 3))   # outer (hidden)
        faces.append((a + 3, b + 3, b + 0, a + 0))   # front annulus
    orient = 1 if wall.key in ("-Y", "+X") else -1
    if orient < 0:
        faces = [tuple(reversed(f)) for f in faces]
    m.add(np.array(verts), faces, mat)


def mortar_core(m, wall):
    """Lime-mortar core between the faces, recessed behind them, with the openings left open."""
    d0, d1 = MORTAR_RECESS, wall.t - MORTAR_RECESS
    # The N/S cores stop the recess short of the corners: flush ends showed flat mortar through the quoins
    # (Jenny: "the corners have no quoins: there's flat render where big dressed corner stones should be").
    u0 = wall.u0 + (MORTAR_RECESS if wall.key in ("+Y", "-Y") else L.WALL)
    u1 = wall.u1 - (MORTAR_RECESS if wall.key in ("+Y", "-Y") else L.WALL)
    if wall.key == "-X":
        u1 = stack_box()[2]
    if wall.key == "+Y":
        u0 = stack_box()[1]
    holes = sorted((o["centre"] - o["width"] / 2, o["centre"] + o["width"] / 2,
                    o["sill"], o["spring"] + o["width"] / 2) for o in wall.openings)
    edges = sorted({u0, u1, *[h[0] for h in holes], *[h[1] for h in holes]})
    edges = [e for e in edges if u0 <= e <= u1]
    for a, b in zip(edges[:-1], edges[1:]):
        mid = 0.5 * (a + b)
        top = L.EAVES_Z if not wall.gable else min(wall.top(a), wall.top(b))
        spans, cur = [], BOTTOM
        for h in holes:
            if h[0] <= mid <= h[1]:
                spans.append((cur, h[2]))
                cur = h[3]
        spans.append((cur, top))
        for za, zb in spans:
            if zb - za > 0.02:
                box_on(m, wall, a, b, za, zb, d0, d1, MAT_MORTAR, radius=0.004, bulge=0.0, relief=0.0,
                       skip=(("z", -1),))
    if wall.gable:
        rise = L.RIDGE_Z - L.EAVES_Z
        half = L.OUT_Y / 2 - L.WALL
        v = [wall.p(-half, L.EAVES_Z, d0), wall.p(half, L.EAVES_Z, d0), wall.p(0.0, L.EAVES_Z + rise * half / (L.OUT_Y / 2), d0),
             wall.p(-half, L.EAVES_Z, d1), wall.p(half, L.EAVES_Z, d1), wall.p(0.0, L.EAVES_Z + rise * half / (L.OUT_Y / 2), d1)]
        faces = [(0, 2, 1), (3, 4, 5), (0, 1, 4, 3), (1, 2, 5, 4), (2, 0, 3, 5)]
        if wall.key == "+X":
            faces = [tuple(reversed(f)) for f in faces]
        m.add(np.array(v), faces, MAT_MORTAR)


def inner_intervals(wall, k, z0, z1):
    """The room's face of a wall: corner to corner of the room, less the openings."""
    a, b = (L.IN_W, L.IN_E) if wall.key in ("+Y", "-Y") else (-L.IN_Y, L.IN_Y)
    cuts = []
    for o in wall.openings:
        w = opening_interval(o, z0, z1, k)
        if w is not None:
            cuts.append((o["centre"] - w, o["centre"] + w))
    out, cur = [], a
    for c0, c1 in sorted(cuts):
        if c0 > cur + 0.1:
            out.append((cur, min(c0, b)))
        cur = max(cur, c1)
    if b > cur + 0.1:
        out.append((cur, b))
    return [(x, y) for x, y in out if y - x > 0.1]


def verge(m, wall, rng):
    """Dressed verge stones along both gable rakes, laid with the slope (they hide the stepped rubble)."""
    pitch = math.radians(L.ROOF_PITCH)
    length = (L.OUT_Y / 2) / math.cos(pitch)
    for side in (-1, 1):
        start = np.array([side * L.OUT_Y / 2, L.EAVES_Z])               # (u, z) at the eaves
        rake = np.array([-side * math.cos(pitch), math.sin(pitch)])      # up the rake
        down = np.array([side * math.sin(pitch), -math.cos(pitch)])      # into the wall, across the rake
        across = wall.u_dir() * down[0] + np.array([0.0, 0.0, down[1]])
        s = 0.0
        while s < length - 0.05:
            seg = min(rng.uniform(0.38, 0.6), length - s)
            mid = start + rake * (s + seg / 2) + down * 0.13
            # pillow's 'up' is the stone's short axis: across the rake.
            m.pillow(wall.p(mid[0], mid[1], 0.0), wall.n, -across, seg / 2 - JOINT / 2, 0.12, mat=MAT_STONE,
                     bulge=0.006, max_out=0.02, cuts=(0, 1), exponent=12.0)
            s += seg


def build_wall(key, seed):
    """One wall's masonry (outer rubble, corners, opening dressing, ground-floor inner wash, core)."""
    rng = np.random.default_rng(seed)
    wall = Wall(key)
    m = Masonry(seed, tint_mean=0.74)
    cs = courses()
    joints_out, joints_in = [], []
    for k, (z0, z1) in enumerate(cs):
        if z0 >= wall.top(0.0) - 0.05:
            break
        corner_quoins(m, wall, k, z0, max(z0 + 0.1, min(z1, wall.top(0.0) if wall.gable else L.EAVES_Z)), rng)
        if not wall.gable and z0 >= L.EAVES_Z - 0.05:
            break
        free = free_intervals(wall, k, z0, z1)
        joints_out = rubble(m, wall, z0, z1, free, rng, joints_out)
        if z1 <= WASH_TOP and z0 >= L.FLOOR_Z - 0.2:
            inner = inner_intervals(wall, k, z0, z1)
            joints_in = rubble(m, wall, z0, z1, inner, rng, joints_in, inner=True)
            # Limewash runs over the joints too: a washed skin just behind the stone faces.
            for a, b in inner:
                box_on(m, wall, a, b, z0, z1, wall.t - 0.022, wall.t - 0.013, MAT_WASH, radius=0.002, bulge=0.0,
                       relief=0.0)
    for o in wall.openings:
        opening_dressing(m, wall, o, cs, rng)
    if wall.gable:
        verge(m, wall, rng)
    mortar_core(m, wall)
    return m, wall

STACK_QUOIN = (0.30, 0.52)


def build_stack(seed=31):
    """The stack in the north-west corner: coursed granite with long-and-short quoins up to the brick,
    then a brick shaft with a corbelled head and a granite cap. Its outer faces are flush with the
    walls'; its two inner faces only show above the roof."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.82)
    x0, x1, y0, y1 = stack_box()
    top = L.STACK["brick_from_z"]
    for k, (z0, z1) in enumerate(courses(seed=11)):
        if z0 >= top - 0.05:
            break
        z1 = min(z1, top)
        lx = STACK_QUOIN[k % 2]
        ly = STACK_QUOIN[(k + 1) % 2]
        for cx_, sx in ((x0, 1), (x1, -1)):
            for cy_, sy in ((y0, 1), (y1, -1)):
                if z1 < L.EAVES_Z - 0.3 and not (cx_ == x0 or cy_ == y1):
                    continue
                lo = np.array([min(cx_, cx_ + sx * lx), min(cy_, cy_ + sy * ly), z0 + JOINT / 2])
                hi = np.array([max(cx_, cx_ + sx * lx), max(cy_, cy_ + sy * ly), z1 - JOINT / 2])
                m.block(lo, hi, mat=MAT_STONE, radius=0.02, relief=0.003, step=0.2,
                        bulge=0.006 * rng.uniform(0.5, 1.0), skip=(("z", -1), ("z", 1)))
        for name, n in (("-X", (-1.0, 0, 0)), ("+X", (1.0, 0, 0)), ("+Y", (0, 1.0, 0)), ("-Y", (0, -1.0, 0))):
            if name in ("+X", "-Y") and z1 < L.EAVES_Z - 0.3:
                continue
            if name in ("-X", "+X"):
                plane, a, b = (x0 if name == "-X" else x1), y0 + ly + JOINT, y1 - ly - JOINT
            else:
                plane, a, b = (y1 if name == "+Y" else y0), x0 + lx + JOINT, x1 - lx - JOINT
            if b - a < 0.1:
                continue
            for sa, sb in _cells(a, b, lambda: float(np.clip((z1 - z0) * rng.uniform(1.4, 2.6), 0.2, 0.5)), rng, [],
                                 min_last=0.12):
                hu, hv = (sb - sa) / 2 - JOINT / 2, max((z1 - z0) / 2 - JOINT / 2, 0.03)
                c = (sa + sb) / 2
                centre = np.array([plane, c, (z0 + z1) / 2]) if name in ("-X", "+X") else np.array([c, plane, (z0 + z1) / 2])
                m.pillow(centre, np.array(n), np.array([0.0, 0.0, 1.0]), hu, hv, mat=MAT_STONE,
                         bulge=rng.uniform(0.004, 0.016))
    m.box(np.array([x0 + MORTAR_RECESS, y0 + MORTAR_RECESS, BOTTOM]),
          np.array([x1 - MORTAR_RECESS, y1 - MORTAR_RECESS, top + 0.02]), MAT_MORTAR, skip=(("z", -1),))
    bh = L.STACK["brick_half"]
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    m.block(np.array([x0 + 0.02, y0 + 0.02, top]), np.array([x1 - 0.02, y1 - 0.02, top + 0.16]), mat=MAT_STONE,
            radius=0.02, relief=0.003, step=0.2, bulge=0.004)   # dressed weathering course under the brick
    z, course = top + 0.16, 0
    brick_h = 0.075
    while z < L.STACK["top_z"] - 0.12:
        corbel = 0.05 * max(0, min(3, int((z - (L.STACK["top_z"] - 0.45)) / brick_h) + 1))
        brick_course(m, cx, cy, bh + corbel, z, brick_h, course, rng)
        z += brick_h
        course += 1
    m.block(np.array([cx - bh - 0.2, cy - bh - 0.2, z]), np.array([cx + bh + 0.2, cy + bh + 0.2, z + 0.11]),
            mat=MAT_STONE, radius=0.015, relief=0.003, step=0.2)
    m.box(np.array([cx - 0.2, cy - 0.2, z + 0.06]), np.array([cx + 0.2, cy + 0.2, z + 0.111]), MAT_MORTAR,
          skip=(("z", -1),))
    return m

MAT_BRICK = 3


def brick_course(m, cx, cy, half, z, h, course, rng):
    """One course of English-bond-ish brick round a square shaft (headers and stretchers alternate)."""
    length = 0.115 if course % 2 else 0.23
    for side in range(4):
        n = [np.array([1.0, 0, 0]), np.array([0, 1.0, 0]), np.array([-1.0, 0, 0]), np.array([0, -1.0, 0])][side]
        u = np.array([-n[1], n[0], 0.0])
        a, x = -half, -half + (0.0 if course % 2 == 0 else length / 2)
        cells = [(-half, x)] if x > -half + 0.02 else []
        while x < half - 1e-6:
            end = min(x + length, half)
            cells.append((x, end))
            x = end
        for ua, ub in cells:
            if ub - ua < 0.02:
                continue
            centre = np.array([cx, cy, z + h / 2]) + n * half + u * (ua + ub) / 2
            m.pillow(centre, n, np.array([0.0, 0.0, 1.0]), (ub - ua) / 2 - 0.005, h / 2 - 0.006, mat=MAT_BRICK,
                     bulge=0.002, max_out=0.004, skirt=0.02, roll=0.003, relief=0.0015, cuts=(0, 0), exponent=16.0,
                     tilt=0.008)
    m.box(np.array([cx - half + 0.012, cy - half + 0.012, z]), np.array([cx + half - 0.012, cy + half - 0.012, z + h]),
          MAT_MORTAR, skip=(("z", -1), ("z", 1)))


# --------------------------------------------------------------------------- roof

MAT_SLATE, MAT_TIMBER, MAT_RIDGE = 0, 1, 2
GAUGE, SLATE_LEN, SLATE_T = 0.20, 0.46, 0.011
DECK_T = 0.03


def build_roof(seed=41):
    """Delabole-style slates in diminishing-width courses on a boarded deck over both pitches, clay ridge
    tiles, and the deck's underside and fascia as timber."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=1.0)
    pitch = math.radians(L.ROOF_PITCH)
    x_lo, x_hi = -L.OUT_X / 2 - L.VERGE_OVERHANG, L.OUT_X / 2 + L.VERGE_OVERHANG
    run = L.OUT_Y / 2 + L.EAVES_OVERHANG
    slope = run / math.cos(pitch)
    rise = SLATE_T * SLATE_LEN / GAUGE
    for s in (-1.0, 1.0):
        up = np.array([0.0, -s * math.cos(pitch), math.sin(pitch)])           # eaves to ridge
        nrm = np.array([0.0, s * math.sin(pitch), math.cos(pitch)])           # out of the roof
        eave = np.array([0.0, s * run, L.EAVES_Z - L.EAVES_OVERHANG * math.tan(pitch)])
        # Deck: boards' underside and top as one slab (seen from inside through the upper windows).
        corners = [eave + np.array([x_lo, 0, 0]), eave + np.array([x_hi, 0, 0]),
                   eave + np.array([x_hi, 0, 0]) + up * slope, eave + np.array([x_lo, 0, 0]) + up * slope]
        v = [c for c in corners] + [c + nrm * DECK_T for c in corners]
        faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (3, 2, 6, 7), (0, 3, 7, 4), (1, 5, 6, 2)]
        faces = [tuple(reversed(f)) for f in faces] if s > 0 else faces
        m.add(np.array(v), faces, MAT_TIMBER, pcoord=np.array(v) * np.array([1.0, 1.0, 1.0]))
        courses = int(slope / GAUGE)
        joints_below = []
        for j in range(courses):
            butt = j * GAUGE - (0.0 if j else 0.0)
            # Diminishing courses: wider, longer slates at the eaves.
            wmin, wmax = (0.30, 0.46) if j < courses * 0.4 else (0.22, 0.36)
            joints, x = [], x_lo
            while x < x_hi - 1e-6:
                end = x + rng.uniform(wmin, wmax)
                for jt in joints_below:
                    if abs(end - jt) < 0.07:
                        end = jt + (0.07 if end >= jt else -0.07)
                if x_hi - end < 0.12:
                    end = x_hi
                gap = rng.uniform(0.003, 0.008)
                width = end - x - gap
                length = min(SLATE_LEN + rng.uniform(-0.02, 0.005), slope - butt + 0.02)
                if length < GAUGE * 1.2:
                    x = end
                    continue
                th = SLATE_T * rng.uniform(0.8, 1.25)
                lift = rise + rng.uniform(-0.002, 0.002)
                cx = 0.5 * (x + end) + rng.uniform(-0.003, 0.003)
                yaw = rng.normal(0.0, math.radians(0.7))
                b = butt + rng.uniform(-0.006, 0.006)

                def frame(p, cx=cx, b=b, lift=lift, length=length, yaw=yaw, up=up, nrm=nrm, eave=eave):
                    p = np.asarray(p, dtype=np.float64)
                    xx = p[:, 0] * math.cos(yaw) - p[:, 1] * math.sin(yaw)
                    yy = p[:, 0] * math.sin(yaw) + p[:, 1] * math.cos(yaw)
                    h = DECK_T + lift * (1.0 - p[:, 1] / length) + p[:, 2]
                    return (eave[None, :] + np.outer(cx + xx, [1.0, 0.0, 0.0]) + np.outer(b + yy, up)
                            + np.outer(h, nrm))

                m.plate(frame, width, length, th, mat=MAT_SLATE, sides=12, relief=0.0012,
                        extra={"slate_tail": lambda local, length=length: np.clip(1.0 - local[:, 1] / (GAUGE * 1.1), 0, 1)})
                joints.append(end)
                x = end
            joints_below = joints[:-1]
        # Fascia board along the eaves.
        fa = eave + nrm * 0.0
        lo = np.array([x_lo, fa[1] - 0.02 if s > 0 else fa[1] - 0.005, fa[2] - 0.20])
        hi = np.array([x_hi, fa[1] + 0.005 if s > 0 else fa[1] + 0.02, fa[2] + 0.02])
        m.block(lo, hi, mat=MAT_TIMBER, radius=0.006, relief=0.001, step=0.5, pcoord_axis=0)
    # Ridge: half-round clay tiles with mortar beds.
    x = x_lo - 0.02
    while x < x_hi:
        length = min(0.32, x_hi + 0.02 - x)
        ridge_tile(m, x, x + length - 0.006, L.RIDGE_Z + DECK_T + rise * 0.5 + 0.005, rng)
        x += length
    return m


def ridge_tile(m, x0, x1, z, rng, radius=0.13, segs=12):
    verts, faces = [], []
    for x in (x0, x1):
        for i in range(segs + 1):
            th = math.pi * i / segs
            r = radius * (1.0 + rng.uniform(-0.01, 0.01))
            verts.append((x, r * math.cos(th), z + r * math.sin(th) * 0.8))
        for i in range(segs + 1):
            th = math.pi * i / segs
            r = radius - 0.016
            verts.append((x, r * math.cos(th), z + r * math.sin(th) * 0.8))
    n = segs + 1
    o0, i0, o1, i1 = 0, n, 2 * n, 3 * n
    for i in range(segs):
        faces.append((o0 + i, o0 + i + 1, o1 + i + 1, o1 + i))       # outer
        faces.append((i0 + i + 1, i0 + i, i1 + i, i1 + i + 1))       # inner
        faces.append((o0 + i + 1, o0 + i, i0 + i, i0 + i + 1))       # end x0
        faces.append((o1 + i, o1 + i + 1, i1 + i + 1, i1 + i))       # end x1
    faces.append((o0, i0, i1, o1))
    faces.append((o0 + segs, o1 + segs, i1 + segs, i0 + segs))
    m.add(np.array(verts), [tuple(reversed(f)) for f in faces], MAT_RIDGE)


# --------------------------------------------------------------------------- floors and shaft

MAT_BOARD, MAT_BEAM, MAT_DECK = 0, 1, 2


def build_floors(seed=51):
    """Ground-floor boards along X with the trapdoor's trimmed opening and collar, the first floor's
    joists and boards overhead, and the bob wall's beam-loft floor."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=1.0)
    tx, ty = L.TRAPDOOR["centre"]
    hx, hy = L.TRAPDOOR["size"][0] / 2, L.TRAPDOOR["size"][1] / 2
    trim = 0.10
    z0, z1 = L.FLOOR_Z - 0.028, L.FLOOR_Z
    y = -L.IN_Y
    while y < L.IN_Y - 1e-6:
        w = min(rng.uniform(0.17, 0.23), L.IN_Y - y)
        edge = L.TRAPDOOR_LAP + 0.003          # the boards stop at the closed lid's edge: no gap round it
        lo_y, hi_y = ty - hy - edge, ty + hy + edge
        # A board straddling the lid's side edge is ripped along it: the part beside the lid runs full
        # length, the part alongside it stops at the lid's ends.
        strips = []
        for sa, sb in ((y, min(y + w, lo_y)), (max(y, lo_y), min(y + w, hi_y)), (max(y, hi_y), y + w)):
            if sb - sa > 0.004:
                strips.append((sa, sb, lo_y <= sa and sb <= hi_y))
        for sa, sb, beside in strips:
            ya, yb = sa + 0.002, sb - 0.002
            cuts = [(L.IN_W, tx - hx - edge), (tx + hx + edge, L.IN_E)] if beside else [(L.IN_W, L.IN_E)]
            board_cuts(m, rng, cuts, ya, yb, z0, z1)
        y += w
    floors_rest(m, rng, tx, ty, hx, hy, trim, z0, z1)
    return m


def board_cuts(m, rng, cuts, ya, yb, z0, z1):
    """Boards between ya and yb laid end to end along each x range in ``cuts``."""
    for a, b in cuts:
        x = a
        while x < b - 1e-6:
            end = min(x + rng.uniform(2.2, 4.2), b)
            if b - end < 0.6:
                end = b
            m.block(np.array([x + 0.002, ya, z0 + rng.uniform(-0.002, 0.0)]), np.array([end - 0.002, yb, z1]),
                    mat=MAT_BOARD, radius=0.004, relief=0.0008, step=0.4, pcoord_axis=0, relief_scale=4.0)
            x = end


def floors_rest(m, rng, tx, ty, hx, hy, trim, z0, z1):
    # Trimmers round the opening, rebated: their tops sit a lid's thickness below the boards, so the closed
    # lid (TRAPDOOR_LAP wider than the opening all round) lies flush with the floor with no gap at its edges.
    for lo, hi in (((tx - hx - trim, ty - hy - trim), (tx + hx + trim, ty - hy)),
                   ((tx - hx - trim, ty + hy), (tx + hx + trim, ty + hy + trim)),
                   ((tx - hx - trim, ty - hy), (tx - hx, ty + hy)),
                   ((tx + hx, ty - hy), (tx + hx + trim, ty + hy))):
        m.block(np.array([lo[0], lo[1], z0 - 0.2]), np.array([hi[0], hi[1], z1 - 0.032]), mat=MAT_BEAM,
                radius=0.008, relief=0.001, step=0.4, pcoord_axis=0 if hi[0] - lo[0] > hi[1] - lo[1] else 1)
    # Under the boards a sealed deck of dark tarred planking over the whole room, open only over the shaft:
    # the hairline gaps between the boards (and round the lid) look into darkness, never through to the
    # holed landscape and the sky beneath it.
    deck = (z0 - 0.07, z0 - 0.025)
    for lo, hi in (((L.IN_W, -L.IN_Y), (tx - hx, L.IN_Y)), ((tx + hx, -L.IN_Y), (L.IN_E, L.IN_Y)),
                   ((tx - hx, -L.IN_Y), (tx + hx, ty - hy)), ((tx - hx, ty + hy), (tx + hx, L.IN_Y))):
        m.block(np.array([lo[0], lo[1], deck[0]]), np.array([hi[0], hi[1], deck[1]]), mat=MAT_DECK, radius=0.002,
                relief=0.0, step=0.8)
    # First floor: joists across the room (along Y) on the side walls, boards over them.
    x = L.IN_W + 0.35
    while x < L.IN_E - 0.2:
        m.block(np.array([x - 0.075, -L.IN_Y - 0.15, L.CEILING_Z]), np.array([x + 0.075, L.IN_Y + 0.15, L.CEILING_Z + 0.23]),
                mat=MAT_BEAM, radius=0.01, relief=0.0015, step=0.4, pcoord_axis=1)
        x += rng.uniform(0.52, 0.6)
    y = -L.IN_Y
    while y < L.IN_Y - 1e-6:
        w = min(rng.uniform(0.18, 0.24), L.IN_Y - y)
        m.block(np.array([L.IN_W, y + 0.002, L.CEILING_Z + 0.23]), np.array([L.IN_E, y + w - 0.002, L.UPPER_FLOOR_Z]),
                mat=MAT_BOARD, radius=0.004, relief=0.0008, step=0.5, pcoord_axis=0)
        y += w
    # A second floor deck higher up (seen only through the upper windows).
    m.block(np.array([L.IN_W, -L.IN_Y, 7.35]), np.array([L.IN_E, L.IN_Y, 7.42]), mat=MAT_BOARD, radius=0.004,
            relief=0.0005, step=0.6, pcoord_axis=0)
    return m


def build_shaft(seed=61):
    """The shaft under the trapdoor: four granite-lined faces in rough courses, wet and darkening with
    depth, down to where it fades out of sight; a timber bearer set across each corner."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.7)
    tx, ty = L.TRAPDOOR["centre"]
    hx, hy = L.TRAPDOOR["size"][0] / 2, L.TRAPDOOR["size"][1] / 2
    top, bottom = L.FLOOR_Z - 0.23, -L.SHAFT_DEPTH_SHOWN
    z, k = top, 0
    while z > bottom:
        h = rng.uniform(0.2, 0.32)
        za = max(z - h, bottom)
        for name, n, plane, a, b in (("-X", (1, 0, 0), tx - hx, ty - hy, ty + hy), ("+X", (-1, 0, 0), tx + hx, ty - hy, ty + hy),
                                      ("-Y", (0, 1, 0), ty - hy, tx - hx, tx + hx), ("+Y", (0, -1, 0), ty + hy, tx - hx, tx + hx)):
            off = 0.08 * (k % 2)
            for sa, sb in _cells(a - off, b + 0.2, lambda: float(np.clip(h * rng.uniform(1.4, 2.6), 0.22, 0.6)), rng, []):
                sa, sb = max(sa, a - 0.02), min(sb, b + 0.02)
                if sb - sa < 0.08:
                    continue
                hu, hv = (sb - sa) / 2 - JOINT / 2, max(h / 2 - JOINT / 2, 0.03)
                c = (sa + sb) / 2
                centre = np.array([plane, c, (z + za) / 2]) if name in ("-X", "+X") else np.array([c, plane, (z + za) / 2])
                m.pillow(centre, np.array(n, float), np.array([0.0, 0.0, 1.0]), hu, hv, mat=MAT_STONE,
                         bulge=rng.uniform(0.004, 0.02))
        z, k = za, k + 1
    # Mortar liner behind the stones, facing into the shaft (an open-topped box turned inside out).
    r = MORTAR_RECESS
    lo, hi = np.array([tx - hx - r, ty - hy - r, bottom - 0.1]), np.array([tx + hx + r, ty + hy + r, top])
    corners = np.array([[x, y, z] for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])
    outward = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4)]
    m.add(corners, [tuple(reversed(f)) for f in outward], MAT_MORTAR)
    return m
