"""Old hand-forged machete: leaf-tapered carbon-steel blade on a full tang with two
riveted hardwood scales, three peened brass rivets and a brass-lined lanyard hole.

Real-object research (written before modeling):
- Classic Latin/Collins-pattern 18" machetes: 18 in (45.7 cm) blade, 23-24 in (58-61 cm)
  overall, grip ~13-15 cm. Tramontina's 18" is 1070 carbon steel, 2.2 mm at the heel
  tapering to ~1.5 mm at the tip (distal taper); older hand-forged blades run ~3 mm.
- Blade width 4-6 cm; a leaf/"bolo-ish" pattern widens towards the front third then
  sweeps up to a drop point. The edge carries a short secondary bevel (~3-4 mm wide,
  ~25-30 deg inclusive) which is the only bright, honed steel on a used blade.
- The first 1.5-2.5 cm ahead of the handle is an unsharpened ricasso/heel.
- Handles: two hardwood scales (~9-10 mm each) on a full tang, three rivets (brass,
  steel or aluminium), Collins-style with a lanyard hole at the butt, often metal-lined.
  The butt hooks slightly forward ("bird's head") so the hand cannot slip off.
- Wear on an old carbon-steel blade: black forge scale left on the spine half, a grey
  ground flat with a mottled grey-blue-brown plant-sap patina, light rust in pits and
  under the scales, a bright recently-honed bevel, edge dished a little where it has
  been sharpened most, the odd rolled nick, and hand-polished, oil-darkened scales.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters. Orientation (for hand sockets): pivot at the grip centre (where the
palm closes, ~1/3 of the way up from the butt), blade pointing +Z, handle down,
cutting edge facing -Y, spine +Y, flats facing +-X.
"""
import math

from mathutils import Vector, noise

NAME = "Machete"
DESCRIPTION = ("Old hand-forged carbon-steel machete: leaf-tapered blade, riveted hardwood "
               "scales, brass rivets and lanyard liner (original). Pivot = grip centre; "
               "blade +Z, edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 8000
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
# Review lying flat on the ground (flat face up); close-up on the ricasso, front rivet and edge heel.
BEAUTY = {"pose": (0, 90, 24), "focus": (0.0, -0.006, 0.085)}

Z_BUTT = -0.052          # butt of the handle (grip centre is the origin)
Z_HILT = 0.103           # front end of the scales
BLADE_LENGTH = 0.46
Z_TIP = Z_HILT + BLADE_LENGTH
TANG = 0.0015            # half thickness of the tang / spine at the hilt (3 mm stock)
EDGE_ANGLE = math.radians(13.5)   # per side of the honed secondary bevel
LANYARD_Z = -0.038
RIVETS_Z = (-0.019, 0.031, 0.080)


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# --------------------------------------------------------------------- handle

def handle_v(z):
    return (z - Z_BUTT) / (Z_HILT - Z_BUTT)


def y_back(v):
    """Spine-side outline of the handle (+Y)."""
    return 0.0132 + 0.0010 * math.sin(math.pi * v) - 0.0035 * max(0.0, (0.14 - v) / 0.14) ** 2


def y_front(v):
    """Edge-side outline: palm swell, bird's-head butt hook and a small finger lip."""
    return -(0.0132 + 0.0024 * math.exp(-((v - 0.45) / 0.22) ** 2)
             + 0.0058 * math.exp(-((v - 0.035) / 0.075) ** 2)
             + 0.0020 * math.exp(-((v - 0.95) / 0.03) ** 2))


def scale_thickness(v):
    """Outer thickness of one scale beyond the tang, before end rounding."""
    return 0.0080 + 0.0013 * math.sin(math.pi * v)


def end_factors(v):
    # Scale fronts are bevelled back to the tang rather than domed.
    front = max(0.0, 1 - ((v - 0.93) / 0.07) ** 2) ** 0.7 if v > 0.93 else 1.0
    butt = math.sqrt(max(0.0, 1 - ((0.075 - v) / 0.075) ** 2)) if v < 0.075 else 1.0
    return front, butt


def scale_face_x(v):
    front, butt = end_factors(v)
    return TANG + scale_thickness(v) * front * butt + 0.0005 * front * butt


def handle_section(v):
    """Closed ring (x, y) around the handle: +X scale, front tang strip, -X scale, back strip."""
    yb, yf = y_back(v), y_front(v)
    yc, half = (yb + yf) / 2, (yb - yf) / 2
    front, butt = end_factors(v)
    a = scale_thickness(v) * front * butt
    half *= max(butt, 0.0)
    top, bot = yc + half, yc - half
    r = min(0.7 * a, 0.9 * half)
    groove = 0.0004 * front * butt
    dome = 0.0005 * front * butt
    t = TANG
    pts = [(t, top), (t + groove, top - groove), (t + 2 * groove, top - 0.25 * groove)]
    cx = max(t + a - r, t + 2 * groove)
    for k in range(5):
        ang = math.radians(90 - 22.5 * k)
        pts.append((cx + r * math.cos(ang), top - r + r * math.sin(ang)))
    for s in (0.25, 0.5, 0.75):
        y = (top - r) + ((bot + r) - (top - r)) * s
        span = max(half - r, 1e-6)
        pts.append((t + a + dome * (1 - ((y - yc) / span) ** 2), y))
    for k in range(5):
        ang = math.radians(-22.5 * k)
        pts.append((cx + r * math.cos(ang), bot + r + r * math.sin(ang)))
    pts += [(t + 2 * groove, bot + 0.25 * groove), (t + groove, bot + groove), (t, bot)]
    return pts + [(-x, y) for x, y in reversed(pts)]


def build_handle(kit, wood, steel):
    vs = [0.004, 0.012, 0.024, 0.04, 0.058, 0.075, 0.095]
    vs += [0.095 + (0.92 - 0.095) * i / 25 for i in range(1, 26)]
    vs += [0.935, 0.95, 0.965, 0.978, 0.989, 1.0]
    rows, coords = [], []
    for v in vs:
        z = Z_BUTT + (Z_HILT - Z_BUTT) * v
        ring = [(x, y, z) for x, y in handle_section(v)]
        rows.append(ring)
        coords.append([(x, y + 0.05, z) for x, y, z in ring])
    butt = (0.0, (y_back(0) + y_front(0)) / 2, Z_BUTT)
    handle = kit.loft("Handle", rows, material=[wood, steel], coords=coords, cap_start=butt, cap_end=True)
    mesh = handle.data
    for poly in mesh.polygons:
        if all(abs(mesh.vertices[i].co.x) <= TANG + 1e-6 for i in poly.vertices):
            poly.material_index = 1

    # Lanyard hole through scales and tang.
    v = handle_v(LANYARD_Z)
    yc = (y_back(v) + y_front(v)) / 2
    cutter = kit.cylinder("LanyardCutter", 0.0029, 0.06, (0, yc, LANYARD_Z), rotation=(0, 90, 0), sides=16)
    mod = handle.modifiers.new("Lanyard", "BOOLEAN")
    mod.operation, mod.solver, mod.object = "DIFFERENCE", "EXACT", cutter
    kit.apply_modifiers(handle)
    import bpy
    bpy.data.objects.remove(cutter)
    return handle


def revolve_x(kit, name, profile, center, material, sides=16):
    """Closed (x, radius) profile revolved around the X axis through ``center`` (y, z)."""
    rows = []
    for x, radius in profile:
        rows.append([(x, center[0] + radius * math.cos(2 * math.pi * j / sides),
                      center[1] + radius * math.sin(2 * math.pi * j / sides)) for j in range(sides)])
    return kit.loft(name, rows, material=material, cyclic=True)


def build_rivet(kit, name, z, mat):
    v = handle_v(z)
    yc = (y_back(v) + y_front(v)) / 2
    fx = scale_face_x(v)
    profile = []
    for side in (-1, 1):
        # Peened head: sits in a shallow counterbore, domed just proud of the worn scale.
        head = [(fx - 0.0012, 0.0031), (fx + 0.00015, 0.0032), (fx + 0.00045, 0.0027), (fx + 0.0006, 0.0015)]
        profile += [(side * x, r) for x, r in (head if side > 0 else reversed(head))]
    rows = [[(x, yc + r * math.cos(2 * math.pi * j / 14), z + r * math.sin(2 * math.pi * j / 14))
             for j in range(14)] for x, r in profile]
    return kit.loft(name, rows, material=mat, cap_start=(-(fx + 0.00065), yc, z), cap_end=(fx + 0.00065, yc, z))


def build_liner(kit, mat):
    v = handle_v(LANYARD_Z)
    yc = (y_back(v) + y_front(v)) / 2
    fx = scale_face_x(v)
    profile = [(-(fx + 0.0003), 0.0024), (-(fx + 0.0004), 0.0033), (-(fx + 0.0001), 0.0039),
               (-(fx - 0.0007), 0.0030), ((fx - 0.0007), 0.0030), ((fx + 0.0001), 0.0039),
               ((fx + 0.0004), 0.0033), ((fx + 0.0003), 0.0024)]
    return revolve_x(kit, "LanyardLiner", profile, (yc, LANYARD_Z), mat, sides=16)


# ---------------------------------------------------------------------- blade

SPINE_Y = y_back(1.0)
HEEL_WIDTH = SPINE_Y - y_front(1.0)
NICKS = ((0.262, 0.0008, 0.0022), (0.371, 0.0005, 0.0016))    # z, depth, half-length


def spine_y(u):
    return SPINE_Y - 0.0045 * u * u - 0.019 * smoothstep(0.76, 1.0, u) ** 1.4


def blade_width(z):
    u = (z - Z_HILT) / BLADE_LENGTH
    body = 0.040 + 0.0185 * math.sin(0.5 * math.pi * min(max(u, 0.0) / 0.70, 1.0)) ** 1.5
    tip = max(0.0, 1 - (max(0.0, u - 0.60) / 0.40) ** 2.3) ** 0.85
    heel = min(max((z - (Z_HILT - 0.003)) / 0.017, 0.0), 1.0)
    width = HEEL_WIDTH + (body - HEEL_WIDTH) * math.sqrt(1 - (1 - heel) ** 2)
    width *= tip
    # Edge dished by years of sharpening where it does most of the work, and two old nicks.
    width -= 0.0011 * math.exp(-((u - 0.30) / 0.13) ** 2) * tip
    for nz, depth, half in NICKS:
        width -= depth * math.exp(-((z - (Z_HILT + nz)) / half) ** 2)
    return max(width, 0.0)


def blade_rows():
    zs = [Z_HILT - 0.003 + 0.002 * i for i in range(18)]                 # heel / ricasso
    z = zs[-1]
    while z < Z_HILT + 0.83 * BLADE_LENGTH:
        step = 0.0045
        for nz, _, half in NICKS:
            if abs(z - (Z_HILT + nz)) < 3 * half:
                step = 0.0014
        z += step
        zs.append(z)
    start = zs[-1]
    for i in range(1, 25):
        s = math.sin(0.5 * math.pi * i / 24)
        zs.append(start + (Z_TIP - 0.0008 - start) * s)
    return zs


def blade_section(z):
    u = (z - Z_HILT) / BLADE_LENGTH
    ys = spine_y(max(u, 0.0))
    w = blade_width(z)
    ye = ys - w
    ts = (2 * TANG - 0.0013 * max(u, 0.0)) * min(1.0, w / 0.010) ** 0.5
    sharp = smoothstep(Z_HILT + 0.010, Z_HILT + 0.027, z)
    wobble = noise.noise(Vector((z * 70.0, 0.3, 1.1)))
    bevel = (0.0010 + 0.0026 * sharp) * (1 + 0.14 * wobble)
    bevel = min(bevel, 0.45 * w)
    tb = ts * 0.9 + (2 * bevel * math.tan(EDGE_ANGLE) - ts * 0.9) * sharp
    tb = min(tb, ts)
    chamfer = min(0.0007, 0.15 * w)
    y_top = ys - chamfer
    yb = ye + bevel
    pts = [(ts * 0.30, ys), (ts * 0.5, y_top)]
    for s in (0.35, 0.7):
        y = y_top + (yb - y_top) * s
        # Hand-forged: full thickness towards the spine, hammered convex taper to the edge.
        x = ts * 0.5 + (tb * 0.5 - ts * 0.5) * s ** 1.6
        pts.append((x, y))
    pts.append((tb * 0.5, yb))
    edge = [(0.0, ye)]
    ring = pts + edge + [(-x, y) for x, y in reversed(pts)]
    return ring, ye


def build_blade(kit, steel):
    rows, coords = [], []
    for z in blade_rows():
        ring, ye = blade_section(z)
        rows.append([(x, y, z) for x, y in ring])
        # pcoord: (thickness x 12 so the two faces decorrelate, distance from edge, length)
        coords.append([(x * 12.0, y - ye, z) for x, y in ring])
    tip_y = spine_y(1.0)
    return kit.loft("Blade", rows, material=steel, coords=coords, cap_start=True,
                    cap_end=(0.0, tip_y - 0.0004, Z_TIP))


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_MacheteScales", light=(0.15, 0.088, 0.048), dark=(0.055, 0.032, 0.017),
                     grain=0.7, roughness=0.58, weathering=0.15, grime=0.4, seed=4.0,
                     polish=0.85, polish_center=0.012, polish_length=0.055, relief=0.7)
    steel = mats.steel("M_MacheteSteel", bevel=0.0034, scale_from=0.024, patina=0.65, rust=0.4, seed=2.0)
    brass = mats.brass("M_MacheteBrass", wear=0.55)
    parts = [build_blade(kit, steel), build_handle(kit, wood, steel), build_liner(kit, brass)]
    parts += [build_rivet(kit, f"Rivet{i}", z, brass) for i, z in enumerate(RIVETS_Z)]
    return kit.join(parts, "SM_Machete", pivot=None, unwrap=False, reshade=True, smooth_angle=35)
