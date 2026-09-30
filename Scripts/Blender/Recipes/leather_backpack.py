"""Leather knapsack: a mid-19th-century country rucksack of oak-bark tanned harness leather, worn high
on her back on two shoulder straps. The General Store's one-time pack upgrade (120 to 240 units);
hideable in Appearance.

Real-object research (written before modeling):
- Before the 1880s "rucksack" was still a German word; in England a walker, pedlar, soldier or
  gamekeeper carried a knapsack: a flat-backed bag of leather (or painted canvas on a leather frame)
  about 30-35 cm wide, 35-40 cm tall and 10-15 cm deep, worn between the shoulder blades.
- Construction: a gusseted body sewn with waxed linen thread in a saddle stitch (about 5 stitches
  per inch, so 4-5 mm apart, set 5-7 mm in from the edge); a stiff flap of heavier leather cut in
  one with the back panel, falling two-thirds down the front with rounded corners; two flap straps
  riveted to the flap, closing through iron roller buckles on short chapes sewn to the body; a
  carrying handle on top.
- Shoulder straps (about 3-4 cm wide) are riveted or sewn at the top of the back panel, run over the
  shoulders and down the chest, and pass under the arms to buckles at the bottom corners, where they
  adjust. Brass or copper rivets at stress points, iron buckles.
- Wear: dubbined leather goes dark and waxy where handled, scuffs pale and dry on the corners and
  the flap edge, and creases across the straps where they bend over the shoulders.
Size here: body 0.32 m wide x 0.36 m tall x up to 0.13 m deep; flap falls 0.20 m; straps 34 mm.

Fitted to her: leather_backpack_fit.py samples her back, shoulders and chest in the bind pose
(Assets/Props/LeatherBackpack/leather_backpack_fit.json). The back panel spans her shoulder blades
taut (bridging the hollow of her spine) with CLEARANCE for the garments over her skin, and the
shoulder straps lie over her shoulders and chest. Without the fit file it uses a measured-average
torso (``_fallback_*``) so the recipe still builds. Units are meters, Z up, -Y forward, X her left
(the body's frame).
PIVOT: the back panel's centre between her shoulder blades (ATTACH_Z), in her reference pose; build
with -KeepPivot. AHomesteadCharacter attaches it to ATTACH_BONE with the reference-pose transform
recorded in the report (as it does the cord belt), so it rides her spine. Original procedural
geometry and materials only: no scanned or downloaded geometry or textures.
"""
import json
import math
import os

from mathutils import Matrix, Vector, noise

NAME = "LeatherBackpack"
DESCRIPTION = ("1850s leather knapsack of dubbined harness leather: flap with two iron roller-buckle straps, "
               "top handle, shoulder straps fitted over her shoulders (original). Pivot = her back between the "
               "shoulder blades in the reference pose; attach to spine_05.")
COLLISION = "none"
TRIANGLE_BUDGET = 18000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 180), "focus": (0.085, 0.14, -0.18)}

_FIT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..',
                         'Assets', 'Props', 'LeatherBackpack', 'leather_backpack_fit.json')
FIT = None
if os.path.exists(_FIT_FILE):
    with open(_FIT_FILE) as _f:
        FIT = json.load(_f)

ATTACH_BONE = "spine_05"
# Body of the bag.
Z_BOT = 0.98
Z_TOP = 1.34
HALF_W = 0.16
BOTTOM_R = 0.025
SUPER = 5.0                 # superellipse exponent of the body's cross-section: flat faces, round corners
RING = 48
# Clearance off her skin for the shirt, tunic or coat under it.
CLEARANCE = 0.03
ATTACH_Z = 1.26
# Flap and straps.
FLAP_DROP = 0.20
FLAP_T = 0.0055
FLAP_R = 0.045
STRAP_W = 0.034
STRAP_T = 0.004
FLAP_STRAP_X = 0.085
BUCKLE_Z = Z_TOP - FLAP_DROP - 0.075
STITCH_IN = 0.0065
STITCH_PITCH = 0.0048


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# --- Her torso -----------------------------------------------------------------------------------

def _fallback_back(z):
    # Waist 7.5 cm behind her centre line, shoulder blades 11 cm.
    return 0.075 + 0.035 * smoothstep(1.00, 1.28, z) - 0.01 * smoothstep(1.36, 1.44, z)


def _fallback_shoulder(y):
    return 1.425 - 6.0 * (y - 0.01) ** 2


def _fallback_chest(z):
    return -0.09 - 0.035 * math.exp(-((z - 1.26) / 0.06) ** 2)


def _interp(xs, values, x):
    pairs = [(a, v) for a, v in zip(xs, values) if v is not None]
    if not pairs:
        return None
    if x <= pairs[0][0]:
        return pairs[0][1]
    for (a0, v0), (a1, v1) in zip(pairs, pairs[1:]):
        if x <= a1:
            return v0 + (v1 - v0) * (x - a0) / (a1 - a0)
    return pairs[-1][1]


def _taut_back_rows():
    """The back panel's Y at each fit height: taut across the pack's width, so it rests on her
    shoulder blades and bridges the hollow of her spine, then smoothed up and down."""
    if not FIT:
        return None
    zs = FIT['z']
    raw = []
    for z, row in zip(zs, FIT['back']):
        near = [v for x, v in zip(FIT['x'], row) if v is not None and abs(x) <= HALF_W - 0.02]
        raw.append(max(near) if near else None)
    # Leather under a load hangs straight between the high points: a running max, then a mean.
    held = []
    for i in range(len(zs)):
        window = [v for v in raw[max(0, i - 3):i + 4] if v is not None]
        held.append(max(window) if window else None)
    smooth = []
    for i in range(len(zs)):
        window = [v for v in held[max(0, i - 2):i + 3] if v is not None]
        smooth.append(sum(window) / len(window) if window else None)
    return zs, smooth


_BACK = _taut_back_rows()


def back_y(z):
    """Y of the back panel (her side of the bag) at height z."""
    skin = _interp(_BACK[0], _BACK[1], z) if _BACK else None
    return (skin if skin is not None else _fallback_back(z)) + CLEARANCE


def shoulder_z(side, y):
    if FIT:
        z = _interp(FIT['shoulder_y'], FIT['shoulder'][0 if side > 0 else 1], y)
        if z is not None:
            return z
    return _fallback_shoulder(y)


def chest_y(side, z):
    if FIT:
        y = _interp(FIT['z_chest'], [pair[0 if side > 0 else 1] for pair in FIT['chest']], z)
        if y is not None:
            return y
    return _fallback_chest(z)


# --- The bag's shape -----------------------------------------------------------------------------

def depth(z):
    t = (z - Z_BOT) / (Z_TOP - Z_BOT)
    belly = 0.085 + 0.045 * math.sin(math.pi * min(1.0, t * 1.1)) ** 0.7
    return belly * (1.0 - 0.3 * smoothstep(0.85, 1.0, t))


def half_w(z):
    t = (z - Z_BOT) / (Z_TOP - Z_BOT)
    return HALF_W * (1.0 - 0.05 * smoothstep(0.8, 1.0, t))


def _se(c, n):
    return math.copysign(abs(c) ** (2.0 / n), c)


def outer_y(x, z, inset=0.0):
    """Y of the bag's outer face (away from her) at (x, z)."""
    hw, a = half_w(z) - inset, depth(z) * 0.5 - inset
    cy = back_y(z) + depth(z) * 0.5
    r = min(1.0, abs(x) / hw) if hw > 0 else 1.0
    return cy + a * max(0.0, 1.0 - r ** SUPER) ** (1.0 / SUPER)


def body_ring(z, inset):
    hw, a = half_w(z) - inset, depth(z) * 0.5 - inset
    cy = back_y(z) + depth(z) * 0.5
    ring = []
    for i in range(RING):
        th = 2 * math.pi * i / RING
        ring.append((hw * _se(math.cos(th), SUPER), cy + a * _se(math.sin(th), SUPER), z))
    return ring


def build_body(kit, leather):
    rows = []
    # Rounded bottom edge, then the body up to its gathered top under the flap.
    for k in range(5):
        phi = 0.5 * math.pi * k / 4
        rows.append(body_ring(Z_BOT + BOTTOM_R * (1 - math.cos(phi)), BOTTOM_R * (1 - math.sin(phi))))
    steps = 28
    for k in range(1, steps + 1):
        rows.append(body_ring(Z_BOT + BOTTOM_R + (Z_TOP - Z_BOT - BOTTOM_R) * k / steps, 0.0))
    coords = [[tuple(p) for p in row] for row in rows]
    body = kit.loft("Body", rows, material=leather, coords=coords, cap_start=True, cap_end=True)

    # Loaded leather: soft irregular sag and a couple of broad creases across the front.
    def sag(co, pcoord):
        n = noise.noise(Vector((co.x * 9.0, co.y * 9.0, co.z * 7.0)))
        crease = math.sin((co.z - Z_BOT) * 38.0 + co.x * 6.0) * 0.0008
        return 0.0018 * n + crease
    kit.displace(body, sag)
    return body


# --- Flap ----------------------------------------------------------------------------------------

def flap_path():
    """(y, z, front weight) along the flap's centre line: up off the back panel, over the top and
    down the outer face. The front weight lets the stiff flap follow the bag's round sides a little."""
    pts = []
    yb, yo = back_y(Z_TOP), outer_y(0.0, Z_TOP)
    for k in range(7):
        s = k / 6.0
        a = math.pi * s
        # A half-round roll over the top, a little flattened.
        y = (yb + yo) * 0.5 - (yo - yb) * 0.5 * math.cos(a)
        z = Z_TOP + 0.018 * math.sin(a) + FLAP_T * 0.5
        pts.append((y, z, s * 0.5))
    steps = 16
    for k in range(1, steps + 1):
        z = Z_TOP - FLAP_DROP * k / steps
        pts.append((outer_y(0.0, z) + FLAP_T * 0.5 + 0.0015, z, 1.0))
    return pts


def _flap_centre():
    path = flap_path()
    lengths = [0.0]
    for (y0, z0, _), (y1, z1, _) in zip(path, path[1:]):
        lengths.append(lengths[-1] + math.hypot(y1 - y0, z1 - z0))
    return path, lengths


_FLAP, _FLAP_LEN = _flap_centre()
FLAP_HW = HALF_W + 0.006


def flap_point(u, v, lift=0.0):
    """Point on the flap's outer surface at across-offset u and path length v, lifted along its normal."""
    total = _FLAP_LEN[-1]
    v = min(max(v, 0.0), total)
    for i in range(1, len(_FLAP_LEN)):
        if v <= _FLAP_LEN[i] or i == len(_FLAP_LEN) - 1:
            f = (v - _FLAP_LEN[i - 1]) / max(1e-6, _FLAP_LEN[i] - _FLAP_LEN[i - 1])
            y0, z0, w0 = _FLAP[i - 1]
            y1, z1, w1 = _FLAP[i]
            y, z, w = y0 + (y1 - y0) * f, z0 + (z1 - z0) * f, w0 + (w1 - w0) * f
            ty, tz = y1 - y0, z1 - z0
            break
    length = math.hypot(ty, tz) or 1.0
    ny, nz = tz / length, -ty / length          # the flap's outward normal in the YZ plane
    wrap = w * 0.6 * (outer_y(u, z) - outer_y(0.0, z)) if w > 0.5 else 0.0
    return Vector((u, y + wrap + ny * (FLAP_T * 0.5 + lift), z + nz * (FLAP_T * 0.5 + lift)))


def flap_half_width(v):
    """Rounded bottom corners: the half-width narrows over the last FLAP_R of the flap."""
    remaining = _FLAP_LEN[-1] - v
    if remaining >= FLAP_R:
        return FLAP_HW
    d = FLAP_R - remaining
    return FLAP_HW - FLAP_R + math.sqrt(max(0.0, FLAP_R * FLAP_R - d * d))


def build_flap(kit, leather):
    across = 18
    rows, coords = [], []
    steps = 40
    total = _FLAP_LEN[-1]
    for k in range(steps + 1):
        v = total * (k / steps) ** 0.85
        hw = max(0.004, flap_half_width(v))
        top = [flap_point(-hw + 2 * hw * j / across, v) for j in range(across + 1)]
        under = [flap_point(hw - 2 * hw * j / across, v, lift=-FLAP_T) for j in range(across + 1)]
        ring = [tuple(p) for p in top + under]
        rows.append(ring)
        coords.append([(p[0], v * 0.3, p[2]) for p in ring])
    flap = kit.loft("Flap", rows, material=leather, coords=coords, cap_start=True, cap_end=True)

    def edge_wear(co, pcoord):
        return 0.0006 * noise.noise(Vector((co.x * 40.0, co.y * 40.0, co.z * 40.0)))
    kit.displace(flap, edge_wear)
    return flap


def flap_outline():
    """Stitch line points and their normals round the flap's sides and rounded bottom."""
    total = _FLAP_LEN[-1]
    pts = []
    # Down one side, round the bottom, up the other.
    v = _FLAP_LEN[6]
    side = []
    while v < total - FLAP_R:
        side.append(v)
        v += STITCH_PITCH
    for s in (-1.0, 1.0):
        chain = side if s < 0 else list(reversed(side))
        if s > 0:
            pts += _bottom_edge()
        for vv in chain:
            pts.append((s * (FLAP_HW - STITCH_IN), vv))
    return pts


def _bottom_edge():
    total = _FLAP_LEN[-1]
    pts = []
    r = FLAP_R - STITCH_IN
    arc = 0.5 * math.pi * r
    count = max(2, int(arc / STITCH_PITCH))
    for k in range(count):
        a = 0.5 * math.pi * k / count
        pts.append((-(FLAP_HW - FLAP_R) - r * math.cos(a), total - FLAP_R + r * math.sin(a)))
    x = -(FLAP_HW - FLAP_R)
    while x < FLAP_HW - FLAP_R:
        pts.append((x, total - STITCH_IN))
        x += STITCH_PITCH
    for k in range(count):
        a = 0.5 * math.pi * (1 - k / count)
        pts.append(((FLAP_HW - FLAP_R) + r * math.cos(a), total - FLAP_R + r * math.sin(a)))
    return pts


def build_stitches(kit, thread):
    """Saddle stitches: short slanted lengths of waxed thread pressed into the flap."""
    parts = []
    outline = flap_outline()
    for i, (u, v) in enumerate(outline[:-1]):
        a = flap_point(u, v, lift=-0.0004)
        b_u, b_v = outline[i + 1]
        b = flap_point(u + (b_u - u) * 0.62, v + (b_v - v) * 0.62, lift=-0.0004)
        parts.append(kit.tube("Stitch_%03d" % i, [tuple(a), tuple(b)], radius=0.0007, sides=5, material=thread))
    return parts


# --- Straps and hardware -------------------------------------------------------------------------

def band(kit, name, points, outward, width, thick, material, chamfer=0.25):
    """A flat leather strap along ``points``; ``outward(i)`` gives the face normal at each point.
    Eight-point section with chamfered edges; pcoord runs along the strap."""
    pts = [Vector(p) for p in points]
    rows, coords = [], []
    length = 0.0
    for i, p in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        n = Vector(outward(i))
        n = (n - t * n.dot(t)).normalized()
        s = t.cross(n).normalized()
        if i:
            length += (p - pts[i - 1]).length
        hw, ht, c = width * 0.5, thick * 0.5, thick * chamfer
        section = [(-hw + c, ht), (hw - c, ht), (hw, ht - c), (hw, -ht + c),
                   (hw - c, -ht), (-hw + c, -ht), (-hw, -ht + c), (-hw, ht - c)]
        ring = [tuple(p + s * a + n * b) for a, b in section]
        rows.append(ring)
        coords.append([(a, b, length) for a, b in section])
    return kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)


def roller_buckle(kit, name, centre, across, up, out, iron, width):
    """An iron roller buckle: a rectangular frame, a roller on its lower bar and a prong."""
    across, up, out = Vector(across).normalized(), Vector(up).normalized(), Vector(out).normalized()
    c = Vector(centre)
    hw, hh, r = width * 0.5 + 0.004, 0.017, 0.0026

    def at(a, b, lift=0.0):
        return tuple(c + across * a + up * b + out * lift)
    corners, frame = 6, []
    for (a, b) in ((-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh), (-hw, -hh)):
        frame.append((a, b))
    loop = []
    for (a0, b0), (a1, b1) in zip(frame, frame[1:]):
        for k in range(corners):
            f = k / corners
            loop.append(at(a0 + (a1 - a0) * f, b0 + (b1 - b0) * f, r))
    loop.append(loop[0])
    parts = [kit.tube(name + "_Frame", loop, radius=r, sides=10, material=iron)]
    roller = [at(-hw + 0.003, -hh, r + 0.001), at(hw - 0.003, -hh, r + 0.001)]
    parts.append(kit.tube(name + "_Roller", roller, radius=r * 1.5, sides=12, material=iron))
    prong = [at(0.0, -hh + 0.002, r), at(0.0, 0.0, r + 0.0035), at(0.0, hh - 0.001, r + 0.0045), at(0.0, hh + 0.002, r + 0.002)]
    parts.append(kit.tube(name + "_Prong", prong, radius=r * 0.75, sides=8, material=iron))
    return parts


def rivet(kit, name, at, normal, brass):
    n = Vector(normal).normalized()
    rot = Vector((0, 0, 1)).rotation_difference(n).to_euler()
    deg = tuple(math.degrees(a) for a in rot)
    obj = kit.cylinder(name, 0.0042, 0.0025, location=tuple(Vector(at) + n * 0.001), rotation=deg,
                       material=brass, sides=14, radius_top=0.0032, bevel=0.0006)
    # Bake the placement into the mesh: build() moves every part to the pivot by warping mesh-local
    # vertices, which a rotated object transform would otherwise swing metres away.
    obj.data.transform(obj.matrix_basis)
    obj.matrix_basis = Matrix.Identity(4)
    return obj


def front_point(x, z, lift):
    """On the bag's outer face (or the flap, where it covers it), lifted by ``lift``."""
    y = outer_y(x, z)
    # Over the flap it rides the flap's thickness, easing off its edge rather than stepping.
    y += (FLAP_T + 0.0015) * smoothstep(Z_TOP - FLAP_DROP - 0.02, Z_TOP - FLAP_DROP + 0.005, z)
    return Vector((x, y + lift, z))


def build_flap_straps(kit, strap, iron, brass):
    parts = []
    for s in (-1.0, 1.0):
        x = s * FLAP_STRAP_X
        # Riveted to the flap near its bottom, down over the face into the buckle, tail through the keeper.
        zs = [Z_TOP - FLAP_DROP + 0.055 - k * 0.012 for k in range(12)]
        pts = [front_point(x, z, STRAP_T * 0.5 + 0.0008) for z in zs]
        pts.append(front_point(x, BUCKLE_Z - 0.03, STRAP_T * 1.6))
        parts.append(band(kit, "FlapStrap_%d" % (s > 0), pts, lambda i: (0, 1, 0), STRAP_W - 0.006, STRAP_T, strap))
        parts.append(rivet(kit, "FlapRivet_%d" % (s > 0), front_point(x, zs[0] - 0.008, STRAP_T), (0, 1, 0), brass))
        # The chape: a short tab sewn to the body below, folded round the buckle's roller bar.
        chape = [front_point(x, BUCKLE_Z - 0.06 + k * 0.012, STRAP_T * 0.5) for k in range(6)]
        parts.append(band(kit, "Chape_%d" % (s > 0), chape, lambda i: (0, 1, 0), STRAP_W - 0.004, STRAP_T, strap))
        parts += roller_buckle(kit, "FlapBuckle_%d" % (s > 0), front_point(x, BUCKLE_Z, STRAP_T * 0.9),
                               (1, 0, 0), (0, 0, 1), (0, 1, 0), iron, STRAP_W - 0.006)
        # The keeper: a leather loop just below the buckle holding the tail flat.
        keeper = [front_point(x - (STRAP_W * 0.5 + 0.001), BUCKLE_Z - 0.03, STRAP_T * 0.4),
                  front_point(x - STRAP_W * 0.5, BUCKLE_Z - 0.03, STRAP_T * 2.8),
                  front_point(x + STRAP_W * 0.5, BUCKLE_Z - 0.03, STRAP_T * 2.8),
                  front_point(x + (STRAP_W * 0.5 + 0.001), BUCKLE_Z - 0.03, STRAP_T * 0.4)]
        hub = front_point(x, BUCKLE_Z - 0.03, STRAP_T * 1.2)

        def away(i, keeper=keeper, hub=hub):
            d = keeper[i] - hub
            return (d.x, d.y, 0.0)
        parts.append(band(kit, "Keeper_%d" % (s > 0), keeper, away, 0.011, 0.0025, strap))
    return parts


def build_handle(kit, strap, brass):
    """A short carrying loop riveted to the top of the flap."""
    v = _FLAP_LEN[3]
    base_l, base_r = flap_point(-0.045, v, 0.002), flap_point(0.045, v, 0.002)
    mid = (base_l + base_r) * 0.5
    pts = []
    for k in range(13):
        a = math.pi * k / 12
        pts.append(Vector((-0.045 * math.cos(a), mid.y, mid.z + 0.045 * math.sin(a) ** 0.8)))
    pts[0], pts[-1] = base_l, base_r

    def away(i):
        d = pts[i] - Vector((0.0, mid.y, mid.z))
        return (d.x, 0.0, d.z) if d.length > 1e-4 else (0.0, 0.0, 1.0)
    handle = band(kit, "Handle", pts, away, 0.024, 0.006, strap)
    return [handle, rivet(kit, "HandleRivetL", base_l, (0, 0, 1), brass), rivet(kit, "HandleRivetR", base_r, (0, 0, 1), brass)]


def shoulder_path(side):
    """From the top of the back panel, over her shoulder and down her chest, then under her arm
    and back to the bag's lower corner, lying on her (with CLEARANCE) all the way."""
    lie = CLEARANCE * 0.5 + STRAP_T * 0.5
    x0 = side * 0.055
    pts = [Vector((x0, back_y(Z_TOP - 0.03) - STRAP_T, Z_TOP - 0.03))]
    # Rising off the pack to the back of her shoulder.
    ys = [0.10, 0.08, 0.06, 0.04, 0.02, 0.0, -0.02, -0.04, -0.06]
    for i, y in enumerate(ys):
        x = side * (0.075 + (0.105 - 0.075) * min(1.0, i / 3.0))
        pts.append(Vector((x, y, shoulder_z(side, y) + lie)))
    # Down her chest, taut: it bridges between the shoulder and the bust rather than dipping in.
    zc = [1.36, 1.32, 1.28, 1.24, 1.20]
    for i, z in enumerate(zc):
        x = side * (0.105 + 0.012 * i)
        pts.append(Vector((x, min(chest_y(side, z), chest_y(side, 1.26)) - lie, z)))
    # Under the arm, round her side and back to the bag's lower corner.
    pts.append(Vector((side * 0.155, -0.06, 1.16)))
    pts.append(Vector((side * 0.172, 0.0, 1.12)))
    pts.append(Vector((side * 0.17, 0.07, 1.07)))
    pts.append(Vector((side * (HALF_W - 0.015), back_y(Z_BOT + 0.05) + 0.02, Z_BOT + 0.05)))
    # Resample evenly so the strap's section doesn't pinch at the joins, then relax the polyline's
    # corners (and the fit's sampling noise) into one smooth run, as a strap under tension lies.
    pts = _resample(pts, 0.012)
    return _relax(pts, 10, keep=2)


def _relax(pts, iterations, keep):
    """Laplacian smoothing with the first and last ``keep`` points pinned."""
    pts = [Vector(p) for p in pts]
    for _ in range(iterations):
        pts = pts[:keep] + [pts[i - 1] * 0.25 + pts[i] * 0.5 + pts[i + 1] * 0.25
                            for i in range(keep, len(pts) - keep)] + pts[len(pts) - keep:]
    return pts


def _resample(pts, step):
    out = [pts[0]]
    carry = 0.0
    for a, b in zip(pts, pts[1:]):
        seg = (b - a).length
        d = step - carry
        while d < seg:
            out.append(a + (b - a) * (d / seg))
            d += step
        carry = seg - (d - step)
    if (out[-1] - pts[-1]).length > 1e-4:
        out.append(pts[-1])
    return out


def build_shoulder_straps(kit, strap, iron, brass):
    parts = []
    for side in (-1.0, 1.0):
        pts = shoulder_path(side)
        centre = Vector((0.0, 0.0, 1.25))

        def raw_outward(p):
            # Away from her torso's axis (a vertical line through her chest), so the strap lies flat on
            # her, turning face-up over the shoulder with a smooth blend rather than a twist.
            away = Vector((p.x - centre.x, p.y - centre.y, 0.0)).normalized()
            w = smoothstep(1.33, 1.40, p.z)
            return (away * (1.0 - 0.7 * w) + Vector((0, 0, 1)) * w).normalized()
        normals = [raw_outward(p) for p in pts]
        for _ in range(6):
            normals = [(normals[max(i - 1, 0)] + normals[i] * 2 + normals[min(i + 1, len(normals) - 1)]).normalized()
                       for i in range(len(normals))]

        def outward(i, normals=normals):
            return tuple(normals[i])
        parts.append(band(kit, "ShoulderStrap_%d" % (side > 0), pts, outward, STRAP_W, STRAP_T, strap))
        parts.append(rivet(kit, "YokeRivet_%d" % (side > 0), pts[0] + Vector((0, -0.002, 0.012)), (0, -1, 0), brass))
        # Adjusting buckle where the strap meets the bag's lower corner.
        end, before = pts[-1], pts[-4]
        up = (before - end).normalized()
        out = Vector((side, 0.3, 0.0)).normalized()
        across = up.cross(out).normalized()
        parts += roller_buckle(kit, "CornerBuckle_%d" % (side > 0), tuple(end + up * 0.03 + out * 0.004),
                               tuple(across), tuple(up), tuple(out), iron, STRAP_W - 0.006)
    return parts


def build(kit):
    mats = kit.mats
    leather = mats.harness_leather("M_KnapsackLeather", color=(0.19, 0.105, 0.05), dark=(0.06, 0.034, 0.018),
                                   roughness=0.56, scuff=0.45, dubbin=0.5, grime=0.3, seed=11.0)
    strap = mats.harness_leather("M_KnapsackStrap", color=(0.15, 0.08, 0.038), dark=(0.05, 0.028, 0.015),
                                 roughness=0.5, scuff=0.55, dubbin=0.7, grime=0.4, seed=12.0)
    thread = mats.rawhide("M_KnapsackThread", color=(0.30, 0.25, 0.16), strands=2, twist=120.0)
    iron = mats.wrought_iron("M_KnapsackIron", rust=0.45, wear=0.4, seed=13.0)
    brass = mats.brass("M_KnapsackBrass", wear=0.4)
    for mat in (leather, strap, thread, iron, brass):
        if mat and mat.node_tree and "Principled BSDF" in mat.node_tree.nodes:
            bsdf = mat.node_tree.nodes["Principled BSDF"]
            if "Subsurface Weight" in bsdf.inputs:
                bsdf.inputs["Subsurface Weight"].default_value = 0.0
    parts = [build_body(kit, leather), build_flap(kit, leather)] + build_stitches(kit, thread)
    parts += build_flap_straps(kit, strap, iron, brass)
    parts += build_handle(kit, strap, brass)
    parts += build_shoulder_straps(kit, strap, iron, brass)
    pivot = Vector((0.0, back_y(ATTACH_Z), ATTACH_Z))
    for part in parts:
        kit.warp(part, lambda co: co - pivot)
    # Unreal's component space: centimetres, Y flipped (the export mirrors Y).
    REPORT["attach"] = {"bone": ATTACH_BONE,
                        "pivot_reference_pose_cm": [round(pivot.x * 100, 2), round(-pivot.y * 100, 2), round(pivot.z * 100, 2)],
                        "fitted": FIT is not None}
    print("LEATHER_BACKPACK pivot (m) = (%.4f, %.4f, %.4f); fitted to her: %s" % (pivot.x, pivot.y, pivot.z, FIT is not None))
    return kit.join(parts, "SM_LeatherBackpack", pivot=None, unwrap=True, reshade=True, smooth_angle=45)


REPORT = {"dimensions_m": {"width": 0.32, "height": 0.36, "depth": 0.13},
          "front": "the flap faces +Y (away from her back); her forward is -Y"}
