"""Stone hoe: a knapped chert hoe blade lashed with rawhide into the natural crook of
an elbow haft, a bark-stripped sapling that grew out of a thicker parent limb.

Real-object research (written before modeling):
- Eastern Woodlands and Mississippian farmers tilled with chipped-stone hoes: bifacially
  flaked chert (Mill Creek, Dover) or ground slate blades, 12-30 cm long and 7-12 cm
  wide, ovate to spade-shaped, narrower at the poll where they were hafted, often with
  shallow side notches that keep the lashing from slipping. The working bit carries a
  famous use-wear gloss: soil silica polishes the last few centimetres glassy, with
  fine striations running in the direction of the stroke, while soil packs into the
  flake-scar hollows above it.
- Hafts were ~1.0-1.5 m saplings (ash, hickory, oak) cut where they branched from a
  thicker limb, so a stub of the parent limb forms a "foot" at 60-80 degrees to the
  handle. The blade's poll is seated in that crook on a flattened face of the foot and
  bound on with wet rawhide strips, which shrink rock-hard as they dry; the far side
  of the parent limb is cut off short as a heel.
- Hafts are stripped of bark and knife-shaved, keeping their natural bows and trimmed
  knot stubs; bark clings in the crook where it is hard to strip. Hands polish and
  darken the grips (a hoe is worked with one hand near the top and the other a third
  of the way down), and the head is caked with soil.
- Here: haft 125 cm to the crook, foot 4.7-5.3 cm thick; chert blade 16.8 cm long
  (edge arc included), 4.2 cm wide at the poll, 9.5 cm across the bit, 1.4 cm thick; the
  blade stands at 75 degrees to the haft. 15.7k triangles, one 2K texture set.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the centre of the main (right) hand's grip on the haft, 42 cm below
the haft top (one third of the way down). The haft runs along +Z (top at z +0.42); the
crook, foot and blade are at the far -Z end (about z -0.83), the blade pointing -Y
(75 degrees from the haft) with its cutting edge along X at y -0.19. The blade's inner
face looks back up the haft (+Z, tilted toward +Y); the foot lies under its outer face.
``REPORT["attach"]`` gives the offsets (cm) from the pivot to the left-hand grip (upper
choke), the centre of the cutting edge and the bit corners, the haft top, the crook and
the blade end (the -Z end of the head: the cut heel of the crook, its lowest point).
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib.util
import math
from pathlib import Path

from mathutils import Vector, noise

_spec = importlib.util.spec_from_file_location("homestead_flint_hatchet", Path(__file__).with_name("flint_hatchet.py"))
hatchet = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(hatchet)

NAME = "StoneHoe"
DESCRIPTION = ("Two-handed stone hoe: knapped chert blade rawhide-lashed into the crook of a bark-stripped "
               "elbow haft (original). Pivot = right-hand grip 1/3 down from the top; handle +Z, blade -Z end "
               "pointing -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "repack": False}

Z_TOP = 0.42             # haft top (the pivot is the right-hand grip)
Z_LEFT = 0.32            # left-hand grip, the upper choke
Z_E = -0.83              # where the haft meets the parent limb's axis
HAFT_LENGTH = Z_TOP - Z_E
HOE_ANGLE = math.radians(75.0)
X = Vector((1.0, 0.0, 0.0))
D = Vector((0.0, -math.sin(HOE_ANGLE), math.cos(HOE_ANGLE)))     # along the foot and blade
N = Vector((0.0, math.cos(HOE_ANGLE), math.sin(HOE_ANGLE)))      # blade inner-face normal

L_FOOT = 0.118           # foot length beyond the junction, along D
L_HEEL = 0.036           # heel stub on the other side
SEAT = 0.0172            # flattened blade seat on the foot, above its axis (along N)
A0 = 0.037               # blade poll, along D from the junction
BL = 0.158               # blade length (poll to bit, before the edge arc)
ARC = 0.010              # the middle of the bit reaches this much further
STRIP = 0.0012           # rawhide strip thickness


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# ----------------------------------------------------------------------- haft

def _raw_axis(z):
    s = (Z_TOP - z) / HAFT_LENGTH
    env = 1 - smoothstep(0.84, 1.0, s)
    x = 0.010 * math.sin(2.3 * s + 0.5) + 0.004 * math.sin(6.1 * s + 1.2) + 0.0025 * math.exp(-((s - 0.56) / 0.05) ** 2)
    y = 0.007 * math.sin(3.1 * s + 2.0) + 0.003 * math.sin(7.3 * s) - 0.002 * math.exp(-((s - 0.3) / 0.06) ** 2)
    return Vector((x * env, y * env, z))


_OFFSET = _raw_axis(0.0)


def haft_axis(z):
    """Irregular centreline through the pivot, straightening into the crook."""
    return _raw_axis(z) - Vector((_OFFSET.x, _OFFSET.y, 0.0))


E = haft_axis(Z_E)


def haft_radius(s):
    """s = arclength fraction from the top (0) to the crook (1); the base swells into a
    branch collar where the sapling grew out of the parent limb."""
    r = 0.0152 + 0.0022 * s + 0.0041 * smoothstep(0.88, 0.985, s) ** 1.5
    r *= 1 + 0.025 * noise.noise(Vector((s * 9.0, 0.7, 2.0)))
    top = min(s / 0.01, 1.0)
    return r * (0.7 + 0.3 * math.sqrt(1 - (1 - top) ** 2))


# Trimmed twig stubs: (arclength fraction from the top, angle around the haft, height m).
KNOTS = [(0.16, 1.4, 0.0034), (0.37, 4.3, 0.0042), (0.52, 0.2, 0.0028), (0.71, 2.6, 0.0045),
         (0.86, 5.3, 0.0036)]


def build_haft(kit, wood):
    zs = [Z_TOP - d for d in (0.0, 0.0015, 0.004, 0.008, 0.0125)]
    n = 74
    zs += [Z_TOP - 0.018 - (Z_TOP - 0.018 - Z_E) * i / n for i in range(n + 1)]
    haft = kit.tube("Haft", [haft_axis(z) for z in zs], radius=haft_radius, sides=24, material=wood)

    def collar(co):
        # A branch collar is drawn out along the parent limb: stretch the haft base along
        # the limb's run (Y) so it spreads into the limb instead of butting against it.
        k = smoothstep(Z_E + 0.075, Z_E + 0.014, co.z)
        c = haft_axis(co.z)
        dy = co.y - c.y
        # Mostly on the heel side; the crotch side stays clear of the blade poll.
        return Vector((co.x, c.y + dy * (1 + (0.75 if dy > 0 else 0.22) * k), co.z))

    kit.warp(haft, collar)
    haft["tube_rings"], haft["tube_sides"] = len(zs), 24

    def relief(co, pco):
        s = pco.z / HAFT_LENGTH
        angle = math.atan2(pco.y, pco.x)
        bump = noise.noise(Vector((pco.x * 70, pco.y * 70, pco.z * 6))) * 0.0005
        # Long shallow knife facets from shaving the stick.
        facet = 7 * angle + 1.9 * noise.noise(Vector((0.0, 0.0, pco.z * 2.6)))
        bump -= 0.00048 * abs(math.cos(facet / 2))
        for ks, ka, h in KNOTS:
            wrap = math.atan2(math.sin(angle - ka), math.cos(angle - ka))
            d = ((s - ks) * HAFT_LENGTH / 0.011) ** 2 + (wrap / 0.38) ** 2
            bump += h * min(1.0, 1.5 * math.exp(-d))                    # flat-topped where it was cut
        # Battered top: a few flattened chips.
        bump -= 0.0008 * smoothstep(0.012, 0.0, s) * (0.5 + 0.5 * noise.noise(Vector((pco.x * 400, pco.y * 400, 1))))
        return bump

    return kit.displace(haft, relief)


# ---------------------------------------------------------------- crook / foot

def limb_axis(a):
    span = (a + L_HEEL) / (L_FOOT + L_HEEL)
    return E + D * a + N * (0.002 * math.sin(math.pi * span)) + X * (0.0012 * math.sin(2.1 * span + 0.4))


def limb_radius(a):
    r = 0.0252 - 0.013 * a
    end = min((L_FOOT - a) / 0.004, (a + L_HEEL) / 0.004, 1.0)
    return r * (0.86 + 0.14 * math.sqrt(max(0.0, 1 - (1 - max(end, 0.0)) ** 2)))


def build_limb(kit, wood):
    span = L_FOOT + L_HEEL
    ts = [0.0, 0.004, 0.012, 0.024] + [0.04 + 0.92 * i / 26 for i in range(27)] + [0.976, 0.988, 0.996, 1.0]
    points = [limb_axis(-L_HEEL + span * t) for t in ts]
    limb = kit.tube("Limb", points, radius=lambda t: limb_radius(-L_HEEL + span * t), sides=24, material=wood)
    limb["tube_rings"], limb["tube_sides"] = len(points), 24

    def relief(co, pco):
        angle = math.atan2(pco.y, pco.x)
        bump = noise.noise(Vector((pco.x * 60, pco.y * 60, pco.z * 20))) * 0.0006
        bump -= 0.0003 * abs(math.cos((5 * angle + 1.3 * noise.noise(Vector((0, 0, pco.z * 12)))) / 2))
        # The limb swells up around the branch collar, so haft and limb grow into each
        # other instead of meeting in a hard crease.
        a = (co - E).dot(D)
        radial = co - limb_axis(a)
        radial = radial - D * radial.dot(D)
        up = max(0.0, radial.normalized().dot(N)) if radial.length > 1e-6 else 0.0
        bump += 0.0065 * math.exp(-((a - 0.003) / 0.021) ** 2) * up ** 1.6
        return bump

    kit.displace(limb, relief)

    def shape(co):
        a = (co - E).dot(D)
        v = (co - E).dot(N)
        u = (co - limb_axis(a)).dot(X)
        r = limb_radius(a)
        # Adzed flat seat for the blade on the inner side of the foot.
        fade = smoothstep(0.012, 0.028, a)
        if v > SEAT:
            co = co - N * ((v - SEAT) * fade)
        # Both ends were hacked off at a slant with a stone blade.
        co = co + D * (-0.007 * (v / r) * smoothstep(L_FOOT - 0.012, L_FOOT, a))
        co = co + D * (0.006 * (u / r) + 0.003 * (v / r)) * smoothstep(-L_HEEL + 0.012, -L_HEEL, a)
        return co

    return kit.warp(limb, shape)


# ---------------------------------------------------------------------- blade

FS = (-1.0, -0.985, -0.955, -0.91, -0.85, -0.77, -0.67, -0.55, -0.41, -0.26, -0.09, 0.09, 0.26, 0.41, 0.55, 0.67,
      0.77, 0.85, 0.91, 0.955, 0.985, 1.0)


def half_width(s, side):
    w = 0.021 + 0.027 * smoothstep(0.05, 0.9, s) ** 1.1
    w -= 0.005 * math.exp(-((s - 0.22) / 0.06) ** 2)                     # lashing notches
    w *= 1 - 0.2 * smoothstep(0.88, 1.0, s) ** 2
    w += 0.0017 * noise.noise(Vector((s * 5.0, 1.3 if side > 0 else 4.1, 0.0)))
    if s < 0.1:
        w *= math.sqrt(max(0.0, 1 - ((0.1 - s) / 0.1) ** 2))
    return max(w, 0.0004)


def thickness(s):
    t = 0.0128 * (1 - s ** 2.1) ** 0.85 + 0.0014
    if s < 0.1:
        t *= math.sqrt(max(0.0, 1 - ((0.1 - s) / 0.1) ** 2)) ** 0.5
    return max(t, 0.0006)


H = SEAT + 0.42 * thickness(0.25) + 0.0003     # mid-plane height above the foot line


def blade_local(s, f, top):
    """(along, across, thickness) of a blade surface point; the inner (top) face is
    domed, the outer (ventral) face flatter, as on a tool knapped from a big flake."""
    hw = half_width(s, 1 if f > 0 else -1)
    t_total = thickness(s)
    if top:
        t = 0.58 * t_total * (1 - abs(f) ** 2.4) ** 0.5
    else:
        t = -0.42 * t_total * (1 - abs(f) ** 3.2) ** 0.5
    along = s * BL + ARC * (1 - f * f) * s ** 6
    # Small use nicks along the bit.
    along -= 0.0011 * abs(noise.noise(Vector((f * 6.0, 2.2, 0.0)))) * smoothstep(0.93, 1.0, s)
    return along, f * hw, t


def blade_point(along, across, t):
    return E + D * (A0 + along) + X * across + N * (H + t)


def blade_section(s):
    ring = [blade_local(s, f, True) for f in FS]
    ring += [blade_local(s, f, False) for f in reversed(FS[1:-1])]
    return ring


BLADE_S = ([0.003, 0.012, 0.026, 0.045, 0.07, 0.1] + [0.125 + 0.785 * i / 36 for i in range(37)] +
           [0.925, 0.94, 0.955, 0.968, 0.979, 0.988, 0.995, 1.0])


def build_blade(kit, stone):
    rows, coords = [], []
    for s in BLADE_S:
        ring = blade_section(s)
        rows.append([blade_point(*p) for p in ring])
        coords.append([(al, t * 10.0, ac) for al, ac, t in ring])
    blade = kit.loft("Blade", rows, material=stone, coords=coords, cap_start=True, cap_end=True)

    def scars(co, pco):
        along, t, across = pco.x, pco.y / 10.0, pco.z
        s = along / BL

        def pattern(side, sa, sw, seed, soft=0.75):
            # Flakes are struck from the side edges, so their scars run across the blade.
            p = Vector((along * sa, across * sw, side * 7.3 + seed))
            d, _ = noise.voronoi(p, distance_metric="DISTANCE", exponent=2.5)
            return -(min((d[1] - d[0]) / 0.5, 1.0) ** soft)

        blend = smoothstep(-0.0012, 0.0012, t)
        big = pattern(1, 32, 20, 0, 0.9) * blend + pattern(-1, 32, 20, 0, 0.9) * (1 - blend)
        fine = pattern(1, 95, 55, 3, 0.6) * blend + pattern(-1, 95, 55, 3, 0.6) * (1 - blend)
        hw = max(half_width(s, 1 if across > 0 else -1), 1e-5)
        edge = smoothstep(0.62, 0.95, abs(across) / hw) + smoothstep(0.84, 0.97, s)
        body = 0.6 + 0.4 * smoothstep(0.0, 0.06, s)
        depth = 0.0024 * big * (1 - 0.5 * min(edge, 1.0)) + 0.0008 * fine * min(edge, 1.0)
        depth *= body
        return max(depth, -0.3 * abs(t))

    return kit.displace(blade, scars)


# -------------------------------------------------------------------- lashing

def _hull(points):
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]          # counter-clockwise


def lash_section(a):
    """Counter-clockwise outline (x along X, v along N) around the foot and the blade at
    distance ``a`` along the foot, starting under the foot, resampled for a wrap."""
    c = limb_axis(a) - (E + D * a)
    cx, cv, r = c.dot(X), c.dot(N), limb_radius(a) + 0.0003
    pts = []
    for k in range(36):
        ang = 2 * math.pi * k / 36
        pts.append((cx + r * math.cos(ang), min(cv + r * math.sin(ang), SEAT)))
    s = (a - A0) / BL
    if 0.0 < s < 1.0:
        for along, across, t in blade_section(s):
            pts.append((across * 1.03 + math.copysign(0.0004, across), H + t * 1.08 + math.copysign(0.0003, t)))
    hull = _hull([(round(x, 6), round(v, 6)) for x, v in pts])
    start = min(range(len(hull)), key=lambda i: (hull[i][1], abs(hull[i][0])))
    hull = hull[start:] + hull[:start]
    out = []
    for i, p in enumerate(hull):
        q = hull[(i + 1) % len(hull)]
        seg = math.hypot(q[0] - p[0], q[1] - p[1])
        pieces = max(1, int(seg / 0.011))
        out += [(p[0] + (q[0] - p[0]) * k / pieces, p[1] + (q[1] - p[1]) * k / pieces) for k in range(pieces)]
    return out


STRIP_SECTION = ((-0.5, 0.0), (-0.42, 0.8), (0.0, 1.0), (0.42, 0.8), (0.5, 0.0), (0.0, -0.15))


def lash_band(kit, name, mat, a0, a1, turns, width, seed):
    """Wet-rawhide strip wound ``turns`` times around foot and blade, each turn lapping
    the last like shingles, the ends tapered and tucked."""
    pitch = (a1 - a0) / turns
    path = []
    for k in range(int(turns)):
        outline = lash_section(a0 + pitch * (k + 0.5))
        perim = [0.0]
        for p, q in zip(outline, outline[1:] + outline[:1]):
            perim.append(perim[-1] + math.hypot(q[0] - p[0], q[1] - p[1]))
        for i, p in enumerate(outline):
            prev, nxt = outline[i - 1], outline[(i + 1) % len(outline)]
            tx, tv = nxt[0] - prev[0], nxt[1] - prev[1]
            ln = math.hypot(tx, tv) or 1.0
            path.append((p, (tv / ln, -tx / ln), a0 + pitch * (k + perim[i] / perim[-1])))
    first = path[0]
    path.append((first[0], first[1], a1))
    rows, coords, length = [], [], 0.0
    previous = None
    lift_step = STRIP * 1.15 * width / pitch        # each lap sits on the one before it
    for i, ((x, v), (nx, nv), a) in enumerate(path):
        tau = i / (len(path) - 1)
        taper = smoothstep(0.0, 0.05, tau) * smoothstep(1.0, 0.95, tau)
        wob = noise.noise(Vector((tau * 17.0, seed, 0.0)))
        sw = width * (0.55 + 0.45 * taper) * (1 + 0.1 * wob)
        th = STRIP * (0.45 + 0.55 * taper)
        a += 0.0016 * noise.noise(Vector((tau * turns * 1.3, seed, 5.0)))
        base = E + D * a + X * x + N * v
        o = X * nx + N * nv
        if previous is not None:
            length += (base - previous).length
        previous = base
        laps = smoothstep(0.0, 1.0, tau * turns)          # the first lap lies flat on the wood
        ring, pco = [], []
        for sa, b in STRIP_SECTION:
            lift = lift_step * (0.5 - sa) * laps + 0.0002
            ring.append(base + o * (th * (0.1 + 0.9 * b) + lift) + D * (sa * sw))
            pco.append((sa * 0.02, b * 0.002, length))
        rows.append(ring)
        coords.append(pco)
    return kit.loft(name, rows, material=mat, coords=coords, cap_start=True, cap_end=True)


# ------------------------------------------------------------------ materials

def _adopt(kit, mat):
    """A Graph over an existing material, so overlays can extend it."""
    g = kit.mats.Graph.__new__(kit.mats.Graph)
    g.mat, g.tree = mat, mat.node_tree
    g.bsdf = mat.node_tree.nodes["Principled BSDF"]
    return g


def _input(g, socket_name):
    socket = g.bsdf.inputs[socket_name]
    return socket.links[0].from_socket if socket.links else socket.default_value


def _fmix(g, a, b, fac):
    node = g.node("ShaderNodeMix", data_type="FLOAT")
    g.link(fac, node.inputs["Factor"])
    for index, value in ((2, a), (3, b)):
        if isinstance(value, float):
            node.inputs[index].default_value = value
        else:
            g.link(value, node.inputs[index])
    return node.outputs[0]


def _chain_normal(g, height, strength, distance):
    normal_in = g.bsdf.inputs["Normal"].links[0].from_socket if g.bsdf.inputs["Normal"].links else None
    g.set("Normal", g.bump(height, strength=strength, distance=distance, normal=normal_in))


def bark_and_soil(kit, mat, z_from, z_full, bark=0.35, soil=0.5, soil_from=None, seed=0.0):
    """Remnant sheets of thin sapling bark that cling toward part-local z ``z_full`` (the
    crook, where it is hardest to strip): grey-brown with fine splits and lenticels, a
    thin tan edge of dried inner bark where it was peeled. Soil smeared on below
    ``soil_from``."""
    g = _adopt(kit, mat)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.31, seed * 0.17, seed * 0.11))
    region = g.remap(z, z_from, z_full)
    patches = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.6)), scale=12.0, detail=3.0,
                      roughness=0.55).outputs["Fac"]
    ragged = g.noise(seeded, scale=190.0, detail=3.0).outputs["Fac"]
    v = g.math("ADD", g.math("ADD", patches, g.math("MULTIPLY", g.math("SUBTRACT", ragged, 0.5), 0.09)),
               g.math("MULTIPLY", region, bark))
    live = g.remap(region, 0.0, 0.2)
    mask = g.math("MULTIPLY", g.remap(v, 0.64, 0.648), live)
    rim = g.math("MULTIPLY", g.math("SUBTRACT", g.remap(v, 0.628, 0.64), mask), live)
    splits = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.08)), scale=420.0, detail=3.0).outputs["Fac"]
    split = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", splits, 0.5)), 0.03, 0.0)
    lent = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 3.5)), scale=170.0, feature="F1").outputs["Distance"]
    lenticel = g.remap(lent, 0.14, 0.06)
    mottle = g.noise(seeded, scale=60.0, detail=5.0).outputs["Fac"]
    bark_col = g.mix((0.042, 0.035, 0.028), (0.098, 0.083, 0.066), g.remap(mottle, 0.35, 0.68))
    bark_col = g.mix(bark_col, (0.14, 0.118, 0.09), g.math("MULTIPLY", lenticel, 0.8))
    bark_col = g.mix(bark_col, (0.02, 0.016, 0.012), g.math("MULTIPLY", split, 0.8))
    color = g.mix(_input(g, "Base Color"), (0.13, 0.085, 0.047), rim)
    color = g.mix(color, bark_col, mask)
    rough = _fmix(g, _input(g, "Roughness"), 0.86, mask)
    cover = g.math("MULTIPLY", z, 0.0)
    if soil and soil_from is not None:
        dirt = g.noise(g.vmath("ADD", seeded, (3.0, 1.0, 7.0)), scale=40.0, detail=6.0, roughness=0.7).outputs["Fac"]
        low = g.remap(z, soil_from, z_full)
        cover = g.math("MULTIPLY", g.remap(dirt, 0.62 - 0.25 * soil, 0.72 - 0.25 * soil), low)
        grit = g.noise(p, scale=1800.0, detail=2.0).outputs["Fac"]
        earth = g.mix((0.05, 0.037, 0.026), (0.11, 0.085, 0.06), g.remap(grit, 0.4, 0.62))
        color = g.mix(color, earth, cover)
        rough = g.math("MAXIMUM", rough, g.math("MULTIPLY", cover, 0.93))
    g.set("Base Color", color)
    g.set("Roughness", rough)
    height = g.math("ADD", mask, g.math("MULTIPLY", rim, 0.4))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", g.math("ADD", split, g.math("MULTIPLY", lenticel, 0.4)),
                                               g.math("MULTIPLY", mask, 0.5)))
    height = g.math("ADD", height, g.math("MULTIPLY", cover, g.noise(p, scale=900.0, detail=3.0).outputs["Fac"]))
    _chain_normal(g, height, 0.6, 0.0011)
    return mat


def knot_marks(kit, mat, knots, length):
    """The cut faces of trimmed twig stubs (end grain, dark) and the darker swirl of
    grain around each, at the same places the geometry has its knot bumps."""
    g = _adopt(kit, mat)
    p = g.coord()
    x, y, z = g.separate(p)
    radial = g.vmath("LENGTH", g.combine(x, y, 0.0))
    nx, ny = g.math("DIVIDE", x, radial), g.math("DIVIDE", y, radial)
    near = None
    for ks, ka, _ in knots:
        dz = g.math("DIVIDE", g.math("SUBTRACT", z, ks * length), 0.011)
        cosd = g.math("ADD", g.math("MULTIPLY", nx, math.cos(ka)), g.math("MULTIPLY", ny, math.sin(ka)))
        da = g.math("DIVIDE", g.math("MULTIPLY", g.math("SUBTRACT", 1.0, cosd), 2.0), 0.38 * 0.38)
        k = g.math("EXPONENT", g.math("MULTIPLY", g.math("ADD", g.math("MULTIPLY", dz, dz), da), -1.0))
        near = k if near is None else g.math("MAXIMUM", near, k)
    wobble = g.noise(p, scale=500.0, detail=3.0).outputs["Fac"]
    near = g.math("ADD", near, g.math("MULTIPLY", g.math("SUBTRACT", wobble, 0.5), 0.12))
    face = g.remap(near, 0.62, 0.7)
    halo = g.remap(near, 0.12, 0.6)
    rings = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", near, 40.0)))
    color = g.mix(_input(g, "Base Color"), (0.62, 0.5, 0.42), g.math("MULTIPLY", halo, 0.8), blend="MULTIPLY")
    end_grain = g.mix((0.06, 0.037, 0.02), (0.11, 0.07, 0.038), rings)
    color = g.mix(color, end_grain, face)
    g.set("Base Color", color)
    g.set("Roughness", _fmix(g, _input(g, "Roughness"), 0.78, face))
    _chain_normal(g, g.math("MULTIPLY", g.math("MULTIPLY", rings, face), 0.4), 0.3, 0.0005)
    return mat


def use_wear(kit, mat):
    """Hoe-blade use-wear: soil-silica gloss with stroke striations on the last few cm of
    the bit, and soil packed into the flake-scar hollows above it.
    pcoord = (along, thickness x 10, across)."""
    g = _adopt(kit, mat)
    p = g.coord()
    along, t10, across = g.separate(p)
    s = g.math("DIVIDE", along, BL)
    ragged = g.noise(p, scale=70.0, detail=4.0).outputs["Fac"]
    reach = g.math("ADD", s, g.math("MULTIPLY", g.math("SUBTRACT", ragged, 0.5), 0.12))
    gloss = g.remap(reach, 0.76, 0.93)
    striae = g.noise(g.combine(g.math("MULTIPLY", along, 4.0), g.math("MULTIPLY", across, 700.0),
                               g.math("MULTIPLY", t10, 3.0)), scale=1.0, detail=6.0, roughness=0.7).outputs["Fac"]
    scratch = g.remap(striae, 0.56, 0.66)
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.006, samples=16))
    patches = g.noise(g.vmath("ADD", p, (2.0, 5.0, 1.0)), scale=45.0, detail=6.0, roughness=0.7).outputs["Fac"]
    upper = g.remap(s, 0.3, 0.62)
    dirt = g.math("ADD", g.remap(cavity, 0.1, 0.35), g.remap(patches, 0.66, 0.74, 0.0, 0.7))
    dirt = g.math("MULTIPLY", g.math("MINIMUM", dirt, 1.0),
                  g.math("MULTIPLY", upper, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", gloss, 0.85))))
    grit = g.noise(p, scale=2200.0, detail=2.0).outputs["Fac"]
    earth = g.mix((0.048, 0.036, 0.025), (0.12, 0.092, 0.064), g.remap(grit, 0.4, 0.62))
    color = g.mix(_input(g, "Base Color"), (0.2, 0.185, 0.165), g.math("MULTIPLY", gloss, 0.28))
    color = g.mix(color, earth, dirt)
    # The hafted end is pecked and ground dull for the lashing and grimed where it sits in the crook.
    poll = g.remap(s, 0.42, 0.04)
    color = g.mix(color, (0.07, 0.055, 0.04), g.math("MULTIPLY", poll, 0.65))
    g.set("Base Color", color)
    # Soil-worn chert is waxy, not glassy; only the polished bit is glossy.
    waxy = g.math("ADD", 0.4, g.math("MULTIPLY", g.noise(p, scale=160.0, detail=3.0).outputs["Fac"], 0.14))
    rough = g.math("MAXIMUM", _input(g, "Roughness"), g.math("ADD", waxy, g.math("MULTIPLY", poll, 0.3)))
    rough = _fmix(g, rough, g.math("ADD", 0.14, g.math("MULTIPLY", scratch, 0.22)), gloss)
    rough = g.math("MAXIMUM", rough, g.math("MULTIPLY", dirt, 0.9))
    g.set("Roughness", rough)
    height = g.math("ADD", g.math("MULTIPLY", g.math("MULTIPLY", scratch, gloss), -0.6),
                    g.math("MULTIPLY", dirt, g.math("MULTIPLY", grit, 0.8)))
    _chain_normal(g, height, 0.35, 0.0005)
    return mat

def rawhide_strip(kit, name, color=(0.2, 0.125, 0.06), seed=0.0):
    """Dried rawhide strip: hard, horn-like and translucent amber, darker along the thin
    ragged edges, faint fibre streaks along its length, hair-follicle pits on the grain
    side and soil worked into it. pcoord = (across, thickness, length)."""
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.3, seed * 0.2, seed * 0.1))
    fibre = g.noise(g.combine(g.math("MULTIPLY", x, 60.0), g.math("MULTIPLY", y, 60.0), g.math("MULTIPLY", z, 2.0)),
                    scale=12.0, detail=6.0, roughness=0.6).outputs["Fac"]
    blotch = g.noise(seeded, scale=35.0, detail=4.0).outputs["Fac"]
    edge = g.remap(g.math("ABSOLUTE", x), 0.006, 0.0098)
    pits = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 1.0)), scale=900.0, feature="F1").outputs["Distance"]
    pit = g.remap(pits, 0.14, 0.05)
    dark = tuple(c * 0.45 for c in color)
    base = g.ramp(g.math("ADD", g.math("MULTIPLY", blotch, 0.6), g.math("MULTIPLY", fibre, 0.4)),
                  [(0.35, dark), (0.55, color), (0.7, tuple(min(1.0, c * 1.2) for c in color))])
    base = g.mix(base, dark, g.math("MAXIMUM", g.math("MULTIPLY", edge, 0.7), g.math("MULTIPLY", pit, 0.5)))
    dirt = g.noise(g.vmath("ADD", seeded, (4.0, 2.0, 9.0)), scale=60.0, detail=5.0).outputs["Fac"]
    grime = g.remap(dirt, 0.55, 0.7, 0.0, 0.6)
    base = g.mix(base, (0.05, 0.037, 0.026), grime)
    g.set("Base Color", base)
    g.set("Roughness", g.math("ADD", g.remap(fibre, 0.3, 0.7, 0.4, 0.55), g.math("MULTIPLY", grime, 0.35)))
    g.set("Subsurface Weight", 0.05)
    g.set("Subsurface Radius", (0.0012, 0.0006, 0.0003))
    height = g.math("ADD", g.math("MULTIPLY", fibre, 0.5), g.math("MULTIPLY", pit, -0.4))
    g.set("Normal", g.bump(height, strength=0.35, distance=0.0008))
    return g.mat


# ------------------------------------------------------------------------- UVs

def tube_uvs(obj, segments=1, lane=0.0):
    """Cylindrical UVs for an un-joined ``kit.tube`` from its rest pcoord (angle x mean
    radius, arclength), in meters, cut into ``segments`` islands along its length so a
    long haft doesn't pack as one diagonal sliver. End caps get planar islands."""
    mesh = obj.data
    raw = [0.0] * (len(mesh.vertices) * 3)
    mesh.attributes["pcoord"].data.foreach_get("vector", raw)
    pts = [Vector(raw[i * 3:i * 3 + 3]) for i in range(len(mesh.vertices))]
    length = max(p.z for p in pts)
    radius = sum(math.hypot(p.x, p.y) for p in pts) / len(pts)
    seg = length / segments
    uv = mesh.uv_layers["UVMap"].data
    for poly in mesh.polygons:
        vs = [pts[i] for i in poly.vertices]
        zs = [p.z for p in vs]
        if max(zs) - min(zs) < 1e-7:
            end = 1 if zs[0] > length / 2 else 0
            for loop, p in zip(poly.loop_indices, vs):
                uv[loop].uv = (lane + p.x, -6 * radius - end * 4 * radius + p.y)
            continue
        k = min(int((sum(zs) / len(zs)) / seg), segments - 1)
        angles = [math.atan2(p.y, p.x) for p in vs]
        if max(angles) - min(angles) > math.pi:
            angles = [a + 2 * math.pi if a < 0 else a for a in angles]
        for loop, p, a in zip(poly.loop_indices, vs, angles):
            uv[loop].uv = (lane + k * 12 * radius + a * radius, p.z - k * seg)
    return obj


def unwrap(obj, dense_materials, density=1.6, margin=0.003):
    """Keep the hand-laid tube UVs, smart-project the blade and lashing, even out texel
    density, give the blade and lashing ``density`` x more, and pack everything."""
    import bpy
    mesh = obj.data
    names = [slot.material.name for slot in obj.material_slots]
    dense = [names[p.material_index] in dense_materials for p in mesh.polygons]
    import bmesh
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.context.scene.tool_settings.use_uv_select_sync = True
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_mode(type="FACE")
    bpy.ops.mesh.select_all(action="DESELECT")
    bm = bmesh.from_edit_mesh(mesh)
    bm.faces.ensure_lookup_table()
    for face in bm.faces:
        face.select_set(dense[face.index])
    bmesh.update_edit_mesh(mesh)
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=margin, scale_to_bounds=False)
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = mesh.uv_layers["UVMap"].data
    loops = [loop for poly, flag in zip(mesh.polygons, dense) if flag for loop in poly.loop_indices]
    centre = sum((Vector(uv[i].uv) for i in loops), Vector((0.0, 0.0))) / len(loops)
    for i in loops:
        uv[i].uv = centre + (Vector(uv[i].uv) - centre) * density
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, margin=margin)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


# ---------------------------------------------------------------------- report

def _cm(v):
    return [round(c * 100, 2) for c in v]


def edge_centre():
    along, across, _ = blade_local(1.0, 0.0, True)
    return blade_point(along, 0.0, 0.0)


def bit_corners():
    """Where the bit edge meets the two side edges (-X, +X)."""
    return [blade_point(*blade_local(1.0, f, True)[:2], 0.0) for f in (-1.0, 1.0)]

REPORT = {"attach": {
    "units": "cm, from the mesh pivot (right-hand grip centre), mesh space",
    "pivot": "main (right) hand grip centre on the haft, 42 cm below the haft top (one third down)",
    "handle_axis": "+Z (haft top); the crook and blade are at the -Z end",
    "edge_direction": "-Y (blade 75 degrees from the haft, cutting edge along X)",
    "hoe_angle_deg": 75.0,
    "left_hand_grip": _cm(haft_axis(Z_LEFT)),
    "edge_centre": _cm(edge_centre()),
    "haft_top": _cm(haft_axis(Z_TOP)),
    "bit_corners": [_cm(p) for p in bit_corners()],
    "crook": _cm(E),
}}

# Review lying on the ground as a dropped hoe lands, on the haft with the blade propped
# 20 degrees up, inner face half turned to the camera; close-up on the lashing.
BEAUTY = {"pose": (-90, 70, 80), "focus": tuple(blade_point(0.28 * BL, 0.012, 0.58 * thickness(0.28)))}


def build(kit):
    m = kit.mats
    haft_wood = m.wood("M_StoneHoeHaft", light=(0.16, 0.105, 0.058), dark=(0.07, 0.042, 0.022), grain=0.9,
                       roughness=0.72, weathering=0.16, grime=0.6, seed=3.0,
                       polish=0.9, polish_center=(Z_TOP - Z_LEFT, Z_TOP), polish_length=0.075, relief=0.9)
    knot_marks(kit, haft_wood, KNOTS, HAFT_LENGTH)
    bark_and_soil(kit, haft_wood, HAFT_LENGTH - 0.17, HAFT_LENGTH - 0.01, bark=0.28, soil=0.45,
                  soil_from=HAFT_LENGTH - 0.12, seed=1.0)
    limb_wood = m.wood("M_StoneHoeCrook", light=(0.18, 0.122, 0.07), dark=(0.08, 0.05, 0.027), grain=0.8,
                       roughness=0.66, weathering=0.12, grime=0.6, seed=9.0, relief=0.9)
    bark_and_soil(kit, limb_wood, L_HEEL + 0.07, 0.0, bark=0.3, soil=0.7, soil_from=0.3, seed=2.0)
    stone = m.flint("M_StoneHoeBlade", body=(0.1, 0.092, 0.08), light=(0.21, 0.192, 0.165),
                    cortex_amount=0.0, cortex_below_x=-1.0)
    hatchet.add_scar_bump(stone, scale=30.0, strength=0.3, distance=0.0025, start=0.004, end=0.97 * BL, fade=0.03)
    use_wear(kit, stone)
    hide = rawhide_strip(kit, "M_StoneHoeLashing", color=(0.13, 0.08, 0.038), seed=1.0)
    hide_b = rawhide_strip(kit, "M_StoneHoeLashingB", color=(0.115, 0.07, 0.034), seed=4.0)
    # The baked set carries the largest Subsurface Weight to the whole mesh (with a ~5 cm
    # radius); a 1.4 cm chert blade is opaque, so keep the set free of it.
    for mat in (haft_wood, limb_wood, stone, hide, hide_b):
        mat.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value = 0.0
    blade = build_blade(kit, stone)
    haft = tube_uvs(build_haft(kit, haft_wood), segments=4)
    limb = tube_uvs(build_limb(kit, limb_wood), lane=1.0)
    parts = [haft, limb, blade,
             lash_band(kit, "LashA", hide, A0 - 0.004, A0 + 0.044, 8, 0.0078, 1.0),
             lash_band(kit, "LashB", hide_b, A0 + 0.056, A0 + 0.077, 4, 0.0068, 2.0)]
    hoe = kit.join(parts, "SM_StoneHoe", pivot=None, unwrap=False, reshade=True, smooth_angle=50)
    unwrap(hoe, {stone.name, hide.name, hide_b.name})
    lowest = min((v.co for v in hoe.data.vertices), key=lambda co: co.z)
    # The -Z end of the whole head (the cut heel of the crook), e.g. for ground clearance.
    REPORT["attach"]["blade_end"] = _cm(lowest)
    return hoe
