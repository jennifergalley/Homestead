"""Forager's thigh pouch: a slim, flat pouch of soft smoke-tanned buckskin that hangs from her cord
belt on a short thong and lies against the outside of her right hip and thigh, cinched at the mouth
with a two-ply plant-fibre drawstring tied off on the outer face.

Real-object research (written before modeling):
- Flat primitive pouches ("possibles" or forage bags) are two panels of brain- or smoke-tanned
  hide, often a U cut from one folded piece, whip-stitched round the edge with a rawhide or
  sinew thong. Stitches run over the edge every ~7-10 mm.
- A drawstring through slits ~2-3 cm below the mouth gathers the top into radial pleats and a
  small ruffled crown; the thong ends carry overhand stopper knots.
- Worn at the side, a flat pouch rides on the outer thigh: the back panel lies against the leg
  and the front bulges only as far as its contents (roots, berries) push it, ~2 cm here.
- Size here: ~13 cm body x 11 cm wide, 16.5 cm to the top of the crown, ~2.6 cm thick when
  holding a few roots; the hanging thong is ~6.5 cm and wraps once round the belt cord.

Fitted: Scripts/Blender/Recipes/forage_pouch_fit.py samples her bind-pose hip, thigh and shorts
(Assets/Props/ForagePouch/forage_pouch_fit.json). The back panel follows that surface the way
taut leather does (bridging hollows and the shorts' hem, never dipping into them), standing off
it by CLEARANCE. Units are meters, Z up, -Y forward, X her left (the body's frame).
PIVOT: the origin is the belt cord's centreline where the thong wraps it; the geometry keeps its
place relative to that point in her reference pose, so AHomesteadCharacter can put the origin
back on the belt (build with -KeepPivot). Everything is generated here: no scanned or downloaded
geometry or textures.
"""
import json
import math
import os

from mathutils import Vector, noise
from mathutils.bvhtree import BVHTree

NAME = "ForagePouch"
DESCRIPTION = ("Forager's flat drawstring thigh pouch in smoke-tanned buckskin, whip-stitched with rawhide, "
               "on a plant-fibre thong (original). Fitted to the heroine's right hip; pivot = the belt cord "
               "where the thong wraps it, in her reference pose.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
BAKE = {"size": 2048, "samples": 96}
BEAUTY = {"pose": (0, 0, 90), "focus": (-0.02, 0.0, -0.12)}

_FIT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..',
                         'Assets', 'Props', 'ForagePouch', 'forage_pouch_fit.json')
with open(_FIT_FILE) as _f:
    FIT = json.load(_f)

SIDES = 48
PLEATS = 9
F0 = 0.040              # centre, forward of her pelvis (the outermost line of her thigh)
HALF_W = 0.055
Z_BOT = 0.811
BOTTOM_R = 0.045
Z_BODY_TOP = 0.941      # the gathers start here
Z_NECK = 0.963          # drawstring
CROWN_H = 0.014
NECK_HW = 0.016
T_BELLY = 0.024
T_TOP = 0.014
T_NECK = 0.009
SEAM = 0.38             # the edge seam's height across the thickness (the back panel is flat)
CLEARANCE = 0.003
FLATTEN = 0.2           # 0 = wraps the thigh exactly, 1 = a flat board
CORD_R = 0.0017
LOOP_R = 0.0021
BELT_R = 0.004


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def _upper_hull(xs, ys):
    """Concave envelope over ``ys`` (outwardness): what a sheet under light tension lies on."""
    pts = list(zip(xs, ys))
    hull = []
    for p in pts:
        while len(hull) >= 2:
            (x1, y1), (x2, y2) = hull[-2], hull[-1]
            if (x2 - x1) * (p[1] - y1) - (y2 - y1) * (p[0] - x1) >= 0:
                hull.pop()
            else:
                break
        hull.append(p)
    out = []
    k = 0
    for x in xs:
        while k < len(hull) - 2 and hull[k + 1][0] < x:
            k += 1
        (x1, y1), (x2, y2) = hull[k], hull[min(k + 1, len(hull) - 1)]
        out.append(y1 if x2 == x1 else y1 + (y2 - y1) * (x - x1) / (x2 - x1))
    return out


class Surface:
    """Outwardness O(f, z) = -x of her hip/thigh/shorts, and the leather that rests on it."""

    def __init__(self, fit, f_span=(-0.03, 0.115), z_span=(0.78, 1.07)):
        zs, fs = fit['z'], fit['f']
        self.i0 = max(i for i, z in enumerate(zs) if z <= z_span[0])
        self.i1 = min(i for i, z in enumerate(zs) if z >= z_span[1])
        self.j0 = max(j for j, f in enumerate(fs) if f <= f_span[0])
        self.j1 = min(j for j, f in enumerate(fs) if f >= f_span[1])
        self.zs = zs[self.i0:self.i1 + 1]
        self.fs = fs[self.j0:self.j1 + 1]
        raw = []
        for i in range(self.i0, self.i1 + 1):
            row = [None if v is None else -v for v in fit['x'][i][self.j0:self.j1 + 1]]
            known = [(j, v) for j, v in enumerate(row) if v is not None]
            raw.append([v if v is not None else min(known, key=lambda q: abs(q[0] - j))[1] for j, v in enumerate(row)])
        self.raw = raw
        # Taut leather: bridge hollows down each column, then across each row, then soften.
        cols = [_upper_hull(self.zs, [raw[i][j] for i in range(len(self.zs))]) for j in range(len(self.fs))]
        tense = [[cols[j][i] for j in range(len(self.fs))] for i in range(len(self.zs))]
        tense = [_upper_hull(self.fs, row) for row in tense]
        for _ in range(3):
            soft = [row[:] for row in tense]
            for i in range(1, len(self.zs) - 1):
                for j in range(1, len(self.fs) - 1):
                    soft[i][j] = sum(tense[i + a][j + b] for a in (-1, 0, 1) for b in (-1, 0, 1)) / 9
            tense = [[max(s, r) for s, r in zip(srow, rrow)] for srow, rrow in zip(soft, raw)]
        self.leather = tense

    def _sample(self, grid, f, z):
        step_f = self.fs[1] - self.fs[0]
        step_z = self.zs[1] - self.zs[0]
        u = min(max((f - self.fs[0]) / step_f, 0.0), len(self.fs) - 1.001)
        v = min(max((z - self.zs[0]) / step_z, 0.0), len(self.zs) - 1.001)
        j, i = int(u), int(v)
        a, b = u - j, v - i
        return ((grid[i][j] * (1 - a) + grid[i][j + 1] * a) * (1 - b)
                + (grid[i + 1][j] * (1 - a) + grid[i + 1][j + 1] * a) * b)

    def raw_at(self, f, z):
        return self._sample(self.raw, f, z)

    def at(self, f, z, flatten=FLATTEN):
        o = self._sample(self.leather, f, z)
        if flatten:
            o += flatten * (self._sample(self.leather, F0, z) - o)
        return o

    def point(self, f, z, d, flatten=FLATTEN):
        """The point ``d`` out from the leather's rest surface at (f, z), along its normal."""
        e = 0.002
        o = self.at(f, z, flatten)
        of = (self.at(f + e, z, flatten) - self.at(f - e, z, flatten)) / (2 * e)
        oz = (self.at(f, z + e, flatten) - self.at(f, z - e, flatten)) / (2 * e)
        n = Vector((-1.0, of, -oz)).normalized()
        return Vector((-o, -f, z)) + n * d, n


SURF = Surface(FIT)


def _belt_point():
    best = min(FIT['belt'], key=lambda p: abs(-p[1] - F0))
    return Vector(best)


PIVOT = _belt_point()


def pleat(theta, amount, s, soft=False):
    """Radial gathers under the drawstring: rounded outer folds, tight valleys, irregular."""
    jitter = 1.1 * noise.noise(Vector((math.cos(theta) * 1.3, math.sin(theta) * 1.3, 4.0)))
    wave = math.cos(PLEATS * theta + jitter + 0.35 * s)
    shaped = wave if soft else (wave ** 0.8 if wave >= 0 else -(abs(wave) ** 1.8))
    depth = 0.62 + 0.45 * noise.noise(Vector((math.cos(theta) * 2.1, math.sin(theta) * 2.1, 9.0)))
    return 1 + amount * depth * shaped


def section(theta, t):
    """(across in -1..1, out from the rest surface) round the pouch at angle ``theta``:
    a flat back panel on the leg (sin < 0), a bulging front panel (sin > 0), seam between."""
    u, s = math.cos(theta), math.sin(theta)
    across = math.copysign(abs(u) ** 0.75, u)
    if s >= 0:
        out = t * (SEAM + (1 - SEAM) * s ** 0.65)
    else:
        out = t * SEAM * (1 - abs(s) ** 0.3)
    return across, out


def ring(z, hw, t, amount=0.0, s=0.0, soft=False, drop=0.0):
    pts = []
    for j in range(SIDES):
        theta = 2 * math.pi * j / SIDES
        across, out = section(theta, t)
        k = pleat(theta, amount, s, soft) if amount else 1.0
        mid = 0.5 * t
        d = max(mid + (out - mid) * k, 0.0)
        p, _ = SURF.point(F0 + hw * across * k, z - drop, CLEARANCE + d)
        pts.append(tuple(p))
    return pts


def body_rows():
    """(z, half width, thickness, pleat amount, s) from the bottom up to the neck."""
    rows = []
    zc = Z_BOT + BOTTOM_R
    for c in (0.03, 0.09, 0.18, 0.3, 0.45, 0.62, 0.8, 1.0):
        z = Z_BOT + BOTTOM_R * c
        across = (1 - ((zc - z) / BOTTOM_R) ** 2.4) ** (1 / 2.4)
        belly = T_BELLY * across ** 0.6
        rows.append((z, HALF_W * across, max(belly, 0.002), 0.0, 0.0))
    steps = 10
    for i in range(1, steps + 1):
        c = i / steps
        z = zc + (Z_BODY_TOP - zc) * c
        full = math.sin(math.pi * min(1.0, 0.35 + 0.65 * c)) ** 0.5 if c < 0.5 else 1.0
        t = T_BELLY + (T_TOP - T_BELLY) * smoothstep(0.1, 1.0, c)
        rows.append((z, HALF_W * (1.0 - 0.05 * c) * (0.985 + 0.015 * full), t, 0.0, 0.0))
    for i in range(1, 7):
        c = i / 6
        z = Z_BODY_TOP + (Z_NECK - Z_BODY_TOP) * c
        hw = NECK_HW + (HALF_W * 0.95 - NECK_HW) * math.cos(0.5 * math.pi * c) ** 1.4
        t = T_NECK + (T_TOP - T_NECK) * math.cos(0.5 * math.pi * c)
        rows.append((z, hw, t, 0.3 * smoothstep(0.1, 1.0, c), c))
    return rows


def build_body(kit, hide):
    specs = body_rows()
    rows = [ring(z, hw, t, a, s) for z, hw, t, a, s in specs]
    for i in range(1, 7):
        c = i / 6
        hw = NECK_HW + (0.024 - NECK_HW) * math.sin(0.5 * math.pi * c) ** 0.8
        t = T_NECK + (0.013 - T_NECK) * c
        rows.append(ring(Z_NECK + CROWN_H * c, hw, t, 0.16 + 0.1 * c, 1.0 + 0.3 * c, soft=True,
                         drop=0.004 * c * c))
    # The gathered rim rolls over and funnels into the closed mouth.
    rows.append(ring(Z_NECK + CROWN_H - 0.003, 0.021, 0.011, 0.22, 1.3, soft=True))
    rows.append(ring(Z_NECK + CROWN_H - 0.011, 0.010, 0.006, 0.22, 1.3, soft=True))
    bottom, _ = SURF.point(F0, Z_BOT - 0.0012, CLEARANCE + 0.3 * SEAM * 0.004)
    top, _ = SURF.point(F0, Z_NECK + CROWN_H - 0.02, CLEARANCE + 0.5 * T_NECK)
    body = kit.loft("Body", rows, material=hide, cap_start=tuple(bottom), cap_end=tuple(top))

    def lumps(co, pco):
        # Roots and berries push the front panel out; the back lies flat on her leg.
        out = -co.x - SURF.at(-co.y, co.z)
        weight = smoothstep(CLEARANCE + 0.006, CLEARANCE + 0.016, out) * (1 - smoothstep(0.885, 0.92, co.z))
        big = noise.noise(pco * 30.0 + Vector((3.0, 1.0, 0.0)))
        small = noise.noise(pco * 75.0 + Vector((0.0, 7.0, 2.0)))
        return weight * (0.0026 * big + 0.0008 * small)
    kit.displace(body, lumps)
    return body, specs, rows


def outer_probe(body):
    """Point on the pouch's outer face at (f, z), lifted off it: for cords laid over it."""
    mesh = body.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in mesh.vertices], [p.vertices[:] for p in mesh.polygons])

    def probe(f, z, lift):
        hit, normal, _, _ = tree.ray_cast(Vector((-0.5, -f, z)), Vector((1.0, 0.0, 0.0)), 0.5)
        if hit is None:
            p, n = SURF.point(f, z, CLEARANCE + T_NECK)
            return p + n * lift
        if normal.x > 0:
            normal = -normal
        return hit + normal * lift
    return probe


def build_stitches(kit, cord, specs, rows):
    """Whip stitch over the seam: down the back edge, round the bottom, up the front edge."""
    last = len([s for s in specs if s[3] == 0.0])
    rear, front = [], []
    for spec, pts in zip(specs[:last], rows[:last]):
        centre, n = SURF.point(F0, spec[0], CLEARANCE + SEAM * spec[2])
        for j, side in ((SIDES // 2, rear), (0, front)):
            edge = Vector(pts[j])
            side.append((edge + (centre - edge).normalized() * 0.0022, n))
    path = list(reversed(rear)) + front
    points = []
    pitch, radius = 0.009, 0.0034
    length = 0.0
    for (a, na), (b, nb) in zip(path, path[1:]):
        seg = (b - a).length
        steps = max(1, int(seg / (pitch / 6)))
        for k in range(steps):
            w = k / steps
            p = a.lerp(b, w)
            n = na.lerp(nb, w).normalized()
            tangent = (b - a).normalized()
            side = tangent.cross(n).normalized()
            phase = 2 * math.pi * (length + seg * w) / pitch
            points.append(p + (n * math.cos(phase) + side * math.sin(phase)) * radius)
        length += seg
    return kit.tube("Stitches", points, radius=0.0009, sides=4, material=cord)


def build_drawstring(kit, cord, neck):
    centre = sum((Vector(p) for p in neck), Vector()) / len(neck)
    points = []
    for j in range(0, SIDES + 1, 2):
        p = Vector(neck[j % SIDES])
        # Over and under through nine slits: dips below the leather between them.
        m = smoothstep(-0.45, -0.1, math.cos(PLEATS * 2 * math.pi * j / SIDES + math.pi / 9))
        points.append(p + (p - centre).normalized() * CORD_R * (0.9 * m - 0.9 * (1 - m)))
    return kit.tube("Drawstring", points, radius=CORD_R, sides=7, material=cord)


def knot_frame():
    p, n = SURF.point(F0, Z_NECK - 0.002, CLEARANCE + T_NECK + 0.0048)
    return p, n, Vector((0.0, -1.0, 0.0)), Vector((0.0, 0.0, 1.0))


def build_knot(kit, cord):
    at, n, ef, ez = knot_frame()
    s = 0.0023
    points = []
    for i in range(45):
        t = 2 * math.pi * i / 44
        a = s * (math.sin(t) + 2 * math.sin(2 * t))
        b = s * (math.cos(t) - 2 * math.cos(2 * t)) * 0.8
        c = s * 0.9 * -math.sin(3 * t)
        points.append(at + ef * a + ez * b + n * c)
    return kit.tube("Knot", points, radius=CORD_R * 0.95, sides=7, material=cord)


def build_tail(kit, name, cord, probe, df, length, seed):
    at, n, ef, ez = knot_frame()
    start = at - ez * 0.004 + ef * 0.003 * math.copysign(1, df)
    points, radii = [], []
    steps = 20
    for i in range(steps + 1):
        t = i / steps
        z = start.z - length * t
        f = -start.y + df * smoothstep(0.0, 1.0, t) + 0.004 * noise.noise(Vector((t * 3.0, seed, 0.0)))
        on_surface = probe(f, z, CORD_R * 1.05)
        points.append(start.lerp(on_surface, smoothstep(0.0, 0.25, t)) if i else start)
        radius = CORD_R
        radius *= 1 + 0.75 * math.exp(-((t - 0.86) / 0.045) ** 2)       # overhand stopper knot
        radius *= 0.55 + 0.45 * (1 - smoothstep(0.9, 1.0, t))             # frayed end
        radii.append(radius)
    return kit.tube(name, points, radii=radii, sides=7, material=cord)


def build_loop(kit, cord, neck_specs):
    """The thong: up from both sides of the neck behind the crown (against her shorts), over the
    belt cord and down behind it, once round, and back."""
    _, n_b = SURF.point(F0, PIVOT.z, 0.0, flatten=0.0)
    up = (Vector((0, 0, 1)) - n_b * n_b.z).normalized()
    rho = BELT_R + LOOP_R + 0.0004
    z_top = PIVOT.z - rho - 0.002
    _, neck_hw, neck_t, _, _ = neck_specs

    def base(sign):
        return PIVOT + Vector((0.0, -sign * 0.007, 0.0))

    def leg(sign):
        pts = []
        count = 14
        for i in range(count):
            c = i / count
            z = Z_NECK + (z_top - Z_NECK) * c
            w = neck_hw + 0.001 + (0.007 - neck_hw - 0.001) * smoothstep(0.0, 1.0, c)
            tuck = smoothstep(0.0, 0.2, c)
            d = (CLEARANCE + 0.5 * neck_t) * (1 - tuck) + (LOOP_R + 0.0003) * tuck
            p, _ = SURF.point(F0 + sign * w, z, d, flatten=0.0)
            pts.append(p)
        # Rise off the shorts to meet the outside of the belt cord.
        end = base(sign) + n_b * rho - up * 0.002
        pts[-1] = pts[-1].lerp(end, 0.5)
        pts.append(end)
        return pts

    def over(sign, forward):
        arc = [base(sign) + n_b * (rho * math.cos(a)) + up * (rho * math.sin(a))
               for a in [math.pi * k / 10 for k in range(11)]]
        arc.append(base(sign) - n_b * rho * 0.9 - up * 0.006)
        return arc if forward else list(reversed(arc))

    a = leg(-1)
    b = leg(1)
    path = a + over(-1, True) + over(1, False) + list(reversed(b))
    wobble = [p + Vector((0.0006 * noise.noise(Vector((p.z * 60.0, 1.0, 0.0))), 0.0, 0.0)) for p in path]
    return kit.tube("HangingLoop", wobble, radius=LOOP_R, sides=7, material=cord)


def report_clearance(body):
    worst = min(-v.co.x - SURF.raw_at(-v.co.y, v.co.z) for v in body.data.vertices
                if Z_BOT - 0.01 < v.co.z < Z_NECK + CROWN_H + 0.01)
    print("FORAGE_POUCH min clearance above her body/shorts: %.1f mm; pivot (m) = (%.4f, %.4f, %.4f)"
          % (worst * 1000, PIVOT.x, PIVOT.y, PIVOT.z))


def build(kit):
    mats = kit.mats
    hide = mats.leather("M_PouchHide", color=(0.275, 0.195, 0.11), dark=(0.10, 0.072, 0.044),
                        roughness=0.76, soil_below=Z_BOT + 0.045, soil=0.5,
                        handled=(Z_NECK, 0.03), stains=0.35, seed=3.0)
    cord = mats.rawhide("M_PouchCord", color=(0.21, 0.165, 0.10), strands=2, twist=110.0)
    body, specs, rows = build_body(kit, hide)
    report_clearance(body)
    probe = outer_probe(body)
    neck_index = len(specs) - 1
    parts = [body, build_stitches(kit, cord, specs, rows), build_drawstring(kit, cord, rows[neck_index]),
             build_knot(kit, cord), build_loop(kit, cord, specs[neck_index])]
    parts.append(build_tail(kit, "TailA", cord, probe, -0.012, 0.064, 1.0))
    parts.append(build_tail(kit, "TailB", cord, probe, 0.009, 0.080, 2.0))
    for part in parts:
        kit.warp(part, lambda co: co - PIVOT)
    return kit.join(parts, "SM_ForagePouch", pivot=None, unwrap=False, reshade=True, smooth_angle=180)
