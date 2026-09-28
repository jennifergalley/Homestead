"""Salvaged swan-neck draw hoe: a rusted forged iron blade on a swan neck and socket, fitted with a
fresh ash haft she cut herself. It replaces the stone hoe on the fixed estate.

Real-object research (written before modeling):
- Victorian English garden "draw" and "swan-neck" hoes: a thin forged blade 13-16 cm wide and
  8-11 cm deep, its lower edge ground sharp, on a round iron neck (9-12 mm) that curves out of a
  conical socket or ferrule and meets the blade at the top centre of its back. The blade stands at
  about 70-80 degrees to the haft, so the edge sits flat on the soil with the user upright and
  pulling it toward her.
- Hafts are 120-150 cm, straight ash, round (3-3.5 cm), driven into the socket and pinned.
- Wear: dark scale and rust on the neck and socket, rust blooms on the back of the blade; the lower
  2 cm of the blade is scoured bright and scratched by soil; soil clings in the neck's crook.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters, Z up. Frame matches SM_StoneHoe (so the tilling clip fits): pivot at the main
(right) hand's grip, haft along +Z (top z +0.42); the socket and neck are at the -Z end (about
z -0.83); the blade points -Y from the neck, about 70 degrees from the haft, with its cutting edge
along X centred near (-0.9, -19.3, -75.5) cm.
"""
import math

from mathutils import Vector, noise

NAME = "DrawHoe"
DESCRIPTION = ("Salvaged swan-neck draw hoe: rusted iron blade, neck and socket on a new ash haft (original). "
               "Pivot = right-hand grip; haft +Z, blade at the -Z end pointing -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}

Z_TOP = 0.42
Z_LEFT = 0.32
Z_SOCKET_TOP = -0.70        # where the haft enters the socket
Z_SOCKET_END = -0.80
X0 = -0.009                 # the head's slight offset, matching the StoneHoe's edge centre
EDGE = Vector((X0, -0.193, -0.7545))
# Direction from the blade's back (top edge) to its cutting edge: about 69 degrees off the haft.
BLADE_DIR = Vector((0.0, -0.932, 0.362)).normalized()
BLADE_DEPTH = 0.095
BLADE_HALF_WIDTH = 0.072
ROOT = EDGE - BLADE_DIR * BLADE_DEPTH


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def build_haft(kit, wood):
    rows, coords = [], []
    sides = 28
    z0, z1 = Z_SOCKET_TOP - 0.03, Z_TOP
    dome = 0.012
    zs = [z0 + (z1 - dome - z0) * i / 44 for i in range(45)]
    zs += [z1 - dome + dome * math.sin(0.5 * math.pi * k / 8) for k in range(1, 9)]
    for z in zs:
        v = (z - z0) / (z1 - z0)
        r = 0.0158 - 0.0012 * v
        # A rounded, slightly flattened top end.
        e = z - (z1 - dome)
        if e > 0:
            r *= math.sqrt(max(0.0, 1 - (e / dome) ** 2)) ** 0.7 * 0.85 + 0.15 * (1 - e / dome)
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            wob = 1 + 0.01 * noise.noise(Vector((math.cos(a) * 3, math.sin(a) * 3, z * 20)))
            ring.append((r * math.cos(a) * wob, r * math.sin(a) * wob, z))
            co.append((math.cos(a) * 0.02, math.sin(a) * 0.02, z))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Haft", rows, material=wood, coords=coords, cap_start=True, cap_end=(0, 0, Z_TOP + 0.0005))


def socket_radius(t):
    """Conical socket from the haft (t 0) down to where the neck leaves it (t 1)."""
    return 0.0185 - 0.0085 * smoothstep(0.0, 1.0, t) ** 0.8 + 0.0012 * math.exp(-((t - 0.02) / 0.03) ** 2)


def neck_points():
    """The swan neck: out of the socket, down and round, then forward and up into the blade's back."""
    start = Vector((X0, 0.0, Z_SOCKET_END))
    ctrl = [start, Vector((X0, -0.004, -0.828)), Vector((X0, -0.022, -0.853)), Vector((X0, -0.055, -0.858)),
            Vector((X0, -0.082, -0.84)), ROOT + Vector((0, 0.004, 0.006))]
    pts = []
    for i in range(len(ctrl) - 1):
        p0 = ctrl[max(i - 1, 0)]
        p1, p2 = ctrl[i], ctrl[i + 1]
        p3 = ctrl[min(i + 2, len(ctrl) - 1)]
        for k in range(8):
            s = k / 8
            s2, s3 = s * s, s * s * s
            pts.append(0.5 * ((2 * p1) + (-p0 + p2) * s + (2 * p0 - 5 * p1 + 4 * p2 - p3) * s2
                              + (-p0 + 3 * p1 - 3 * p2 + p3) * s3))
    pts.append(ctrl[-1])
    return pts


def build_socket_and_neck(kit, iron):
    rows, coords = [], []
    sides = 24
    count = 24
    for i in range(count + 1):
        t = i / count
        z = Z_SOCKET_TOP + (Z_SOCKET_END - Z_SOCKET_TOP) * t
        r = socket_radius(t)
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            ring.append((X0 + r * math.cos(a), r * math.sin(a), z))
            co.append((r * math.cos(a), 0.2 + (z - Z_SOCKET_END), r * math.sin(a)))
        rows.append(ring)
        coords.append(co)
    socket = kit.loft("Socket", rows, material=iron, coords=coords, cap_start=True, cap_end=True)
    pts = neck_points()
    n = len(pts)
    radii = [0.0098 - 0.0034 * smoothstep(0.0, 0.5, i / (n - 1)) + 0.0012 * smoothstep(0.85, 1.0, i / (n - 1))
             for i in range(n)]
    # Sweep the neck by hand so the iron's pcoord reads it as forged stock, not a honed edge.
    rows, coords = [], []
    sides = 18
    across = Vector((1.0, 0.0, 0.0))
    length = 0.0
    for i, p in enumerate(pts):
        tangent = (pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized()
        if i:
            length += (pts[i] - pts[i - 1]).length
        up = tangent.cross(across).normalized()
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            off = across * (math.cos(a) * radii[i]) + up * (math.sin(a) * radii[i])
            ring.append(tuple(p + off))
            co.append((math.cos(a) * radii[i], 0.12 + 0.3 * length, math.sin(a) * radii[i]))
        rows.append(ring)
        coords.append(co)
    neck = kit.loft("Neck", rows, material=iron, coords=coords, cap_start=True, cap_end=True)
    return [socket, neck]


def build_blade(kit, iron):
    """A thin plate: the back (top) edge at ROOT, the cutting edge at EDGE, width along X."""
    side = Vector((1.0, 0.0, 0.0))
    normal = side.cross(BLADE_DIR).normalized()   # the blade's face normal
    rows, coords = [], []
    cols = 24
    depth_steps = 22
    radius = 0.013                              # corner rounding
    for i in range(depth_steps + 1):
        u = 0.5 - 0.5 * math.cos(math.pi * i / depth_steps)   # denser rows at the back and edge
        along = u * BLADE_DEPTH
        # Thickness: 3.2 mm at the back, a 1.8 cm ground bevel down to the edge.
        th = 0.0016 * (1 - smoothstep(0.72, 1.0, u)) + 0.00035
        # The blade widens a little toward the edge; the corners are rounded.
        half = BLADE_HALF_WIDTH * (0.93 + 0.07 * u)
        e = min(along, BLADE_DEPTH - along)
        if e < radius:
            half -= radius - math.sqrt(max(0.0, radius * radius - (radius - e) ** 2))
        ring, co = [], []
        # Walk round the section: across the front face, then back across the rear face.
        for side_sign, face in ((1, 1), (-1, -1)):
            for j in range(cols + 1):
                s = (-1 + 2 * j / cols) * side_sign
                # The faces close over the side edges.
                th_here = th * math.sqrt(max(0.0, 1 - abs(s) ** 8)) + 0.0002
                # A slight dish across the width, deepest at the middle.
                dish = 0.0025 * (1 - s * s)
                p = ROOT + BLADE_DIR * along + side * (s * half) + normal * (face * th_here + dish)
                ring.append(tuple(p))
                co.append((face * th_here, BLADE_DEPTH - along, s * half))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Blade", rows, material=iron, coords=coords, cap_start=True, cap_end=True)


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_DrawHoeAsh", light=(0.26, 0.19, 0.11), dark=(0.135, 0.088, 0.047),
                     grain=0.9, roughness=0.66, weathering=0.06, grime=0.3, seed=31.0,
                     polish=0.25, polish_center=[0.0, 0.0], polish_length=0.08, relief=0.6)
    iron = mats.steel("M_DrawHoeIron", bevel=0.02, scale_from=0.05, patina=0.9, rust=0.95, seed=33.0)
    parts = [build_haft(kit, wood), build_blade(kit, iron)] + build_socket_and_neck(kit, iron)
    return kit.join(parts, "SM_DrawHoe", pivot=None, unwrap=False, reshade=True, smooth_angle=40)


def _cm(v):
    return [round(c * 100, 2) for c in v]


_corner = EDGE + Vector((BLADE_HALF_WIDTH, 0, 0))
REPORT = {"attach": {
    "units": "cm, from the mesh pivot (right-hand grip centre), mesh space",
    "pivot": "main (right) hand grip centre on the haft, 42 cm below the haft top",
    "handle_axis": "+Z (haft top); the socket, neck and blade are at the -Z end",
    "edge_direction": "-Y (blade about 69 degrees from the haft, cutting edge along X)",
    "left_hand_grip": [0.0, 0.0, Z_LEFT * 100],
    "edge_centre": _cm(EDGE),
    "haft_top": [0.0, 0.0, Z_TOP * 100],
    "bit_corners": [_cm(EDGE - Vector((BLADE_HALF_WIDTH, 0, 0))), _cm(_corner)],
    "crook": _cm(Vector((X0, 0.0, Z_SOCKET_END))),
    "blade_end": _cm(Vector((X0, -0.055, -0.868))),
}}
BEAUTY = {"pose": (-90, 70, 80), "focus": tuple(ROOT + BLADE_DIR * 0.05)}
