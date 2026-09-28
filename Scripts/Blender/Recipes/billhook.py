"""Worn West Country billhook: a hand-forged, single-edged carbon-steel blade that sweeps into a
forward hook, its tang driven through a turned ash handle with an iron ferrule and a peened
butt washer. The head is the rusted one she salvages from the ruin, freshly hafted: old pitted
steel with a newly honed edge, on a clean pale handle.

Real-object research (written before modeling):
- English regional billhooks (Morris of Dunsford, Elwell, Brades catalogues of the 1850s-1900s
  list dozens of county patterns). The West Country / Devon pattern is single-edged, with a
  straight belly that curls into a pronounced forward hook ("beak") for pulling brash and
  hooking bramble toward the cut. Blades run 8-10 in (20-25 cm), 2-2.5 in (5-6 cm) deep.
- Stock is heavier than a machete: about 4-5 mm at the spine near the handle, tapering to
  ~2.5 mm at the beak, ground convex to an edge honed at roughly 25-30 deg inclusive.
- A round or oval turned handle, 4.5-5 in (11-13 cm), of ash or beech, often with a slight
  swell at the palm and a flared butt; a whittle tang runs through it and is riveted over a
  washer at the butt. An iron or steel ferrule (1.5-2 cm) binds the front of the handle.
- The blade narrows at the heel into a short shank ("shoulder") that enters the ferrule.
- Wear: forge scale and brown rust in pits on the flats and spine, a grey ground zone, and a
  bright honed bevel with the odd nick where it has met stone.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters. Orientation matches SM_Machete so she swings it with the machete's hack:
pivot at the grip centre, blade +Z, handle down, cutting edge facing -Y (the hook curls
toward -Y), spine +Y, flats facing +-X.
"""
import math

from mathutils import Vector, noise

NAME = "Billhook"
DESCRIPTION = ("Worn West Country billhook: hooked single-edged blade, turned ash handle, iron "
               "ferrule and peened butt washer (original). Pivot = grip centre; blade +Z, edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
# Review lying on the ground, flat face up; close-up on the hook and its edge.
BEAUTY = {"pose": (0, 90, 24), "focus": (0.0, -0.02, 0.27)}

Z_BUTT = -0.058          # butt of the handle (grip centre is the origin)
Z_FERRULE = 0.070        # handle wood ends inside the ferrule
Z_SHANK = 0.088          # ferrule front; the blade's shank emerges here
STRAIGHT = 0.150         # straight belly length, from the shank
HOOK_RADIUS = 0.085
HOOK_ANGLE = math.radians(72)
SPINE_T = 0.0046         # full spine thickness at the heel
TIP_T = 0.0024
EDGE_ANGLE = math.radians(14)
NICKS = ((0.09, 0.0007, 0.002), (0.19, 0.0005, 0.0016))


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# ---------------------------------------------------------------------- blade

LENGTH = STRAIGHT + HOOK_RADIUS * HOOK_ANGLE


def spine_frame(s):
    """Point on the spine, its tangent and the edge-ward normal at arc length ``s`` from the shank.
    The spine runs up the +Y side, then bends forward over the edge."""
    spine_y = 0.012
    if s <= STRAIGHT:
        return Vector((0.0, spine_y, Z_SHANK + s)), Vector((0, 0, 1)), Vector((0, -1, 0))
    a = (s - STRAIGHT) / HOOK_RADIUS
    centre = Vector((0.0, spine_y - HOOK_RADIUS, Z_SHANK + STRAIGHT))
    point = centre + Vector((0.0, HOOK_RADIUS * math.cos(a), HOOK_RADIUS * math.sin(a)))
    tangent = Vector((0.0, -math.sin(a), math.cos(a)))
    return point, tangent, Vector((0.0, -tangent.z, tangent.y))


def depth(s):
    """Spine-to-edge depth: a narrow shank, the deep belly, then the tapering beak."""
    u = s / LENGTH
    shank = 0.022 + (0.050 - 0.022) * smoothstep(0.0, 0.10, u) ** 0.8
    belly = 0.006 * math.exp(-((u - 0.40) / 0.2) ** 2)
    # The belly sweeps smoothly up into a stout beak: the depth falls away well before the spine
    # starts to bend, so the inside of the hook stays a broad curve rather than a notch.
    beak = 1 - 0.78 * smoothstep(0.42, 1.0, u)
    tip = max(0.0, 1 - (max(0.0, u - 0.9) / 0.1) ** 2) ** 0.6
    d = (shank + belly) * beak * tip
    # Sharpening has dished the working belly a little; two old nicks.
    d -= 0.0012 * math.exp(-((u - 0.42) / 0.16) ** 2) * beak
    for ns, dn, half in NICKS:
        d -= dn * math.exp(-((s - ns) / half) ** 2)
    return max(d, 0.0)


def blade_stations():
    ss = [0.002 * i for i in range(12)]
    s = ss[-1]
    while s < LENGTH * 0.9:
        step = 0.004
        for ns, _, half in NICKS:
            if abs(s - ns) < 3 * half:
                step = 0.0012
        s += step
        ss.append(s)
    start = ss[-1]
    for i in range(1, 16):
        ss.append(start + (LENGTH - 0.0006 - start) * math.sin(0.5 * math.pi * i / 15))
    return ss


def blade_ring(s):
    """Closed section from the spine across to the edge: (x across the flats, n toward the edge)."""
    u = s / LENGTH
    d = depth(s)
    ts = SPINE_T + (TIP_T - SPINE_T) * u
    ts *= min(1.0, d / 0.008) ** 0.5
    sharp = smoothstep(0.012, 0.03, s)
    wobble = noise.noise(Vector((s * 60.0, 0.7, 2.3)))
    bevel = min((0.0012 + 0.003 * sharp) * (1 + 0.15 * wobble), 0.45 * d)
    tb = min(ts * 0.9 + (2 * bevel * math.tan(EDGE_ANGLE) - ts * 0.9) * sharp, ts)
    chamfer = min(0.0009, 0.18 * max(d, 1e-5))
    n_bevel = d - bevel
    pts = [(ts * 0.32, 0.0), (ts * 0.5, chamfer)]
    for f in (0.3, 0.6, 0.85):
        n = chamfer + (n_bevel - chamfer) * f
        pts.append((ts * 0.5 + (tb * 0.5 - ts * 0.5) * f ** 1.7, n))
    pts.append((tb * 0.5, n_bevel))
    ring = pts + [(0.0, d)] + [(-x, n) for x, n in reversed(pts)]
    return ring, d


def build_blade(kit, steel):
    rows, coords = [], []
    for s in blade_stations():
        spine, _, edgeward = spine_frame(s)
        ring, d = blade_ring(s)
        rows.append([tuple(spine + edgeward * n + Vector((x, 0, 0))) for x, n in ring])
        # pcoord: (thickness x 12 so the faces decorrelate, distance from edge, length along the blade)
        coords.append([(x * 12.0, d - n, s) for x, n in ring])
    tip = spine_frame(LENGTH)[0] + spine_frame(LENGTH)[2] * 0.0004
    return kit.loft("Blade", rows, material=steel, coords=coords, cap_start=True, cap_end=tuple(tip))


# --------------------------------------------------------------------- handle

def handle_radius(v):
    """Turned profile along the handle (v 0 = butt, 1 = ferrule): flared butt, palm swell, neck."""
    return (0.0128 + 0.0022 * math.exp(-((v - 0.03) / 0.07) ** 2)
            + 0.0016 * math.exp(-((v - 0.48) / 0.25) ** 2) - 0.0012 * smoothstep(0.8, 1.0, v))


def build_handle(kit, wood):
    rows, coords = [], []
    sides = 28
    vs = [0.0, 0.006, 0.016, 0.03] + [0.03 + 0.97 * i / 30 for i in range(1, 31)]
    for v in vs:
        z = Z_BUTT + (Z_FERRULE - Z_BUTT) * v
        r = handle_radius(v)
        # Rounded butt edge, and a slightly oval section (deeper toward the edge and spine).
        r *= math.sqrt(max(0.0, 1 - ((0.03 - v) / 0.03) ** 2)) ** 0.5 if v < 0.03 else 1.0
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            wob = 1 + 0.012 * noise.noise(Vector((math.cos(a) * 2, math.sin(a) * 2, z * 40)))
            ring.append((r * 0.93 * math.cos(a) * wob, r * 1.06 * math.sin(a) * wob + 0.0005, z))
            co.append((math.cos(a) * 0.02, math.sin(a) * 0.02, z * 1.0))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Handle", rows, material=wood, coords=coords, cap_start=(0.0, 0.0005, Z_BUTT - 0.0004), cap_end=True)


def build_ferrule(kit, iron):
    rows, sides = [], 28
    r_wood = handle_radius(1.0)
    profile = [(Z_FERRULE - 0.004, r_wood * 0.99), (Z_FERRULE - 0.0035, r_wood + 0.0011),
               (Z_FERRULE + 0.004, r_wood + 0.0013), (Z_SHANK - 0.004, r_wood + 0.0012),
               (Z_SHANK - 0.0012, r_wood + 0.0006), (Z_SHANK, r_wood * 0.72)]
    for z, r in profile:
        rows.append([(r * 0.95 * math.cos(2 * math.pi * j / sides), r * 1.05 * math.sin(2 * math.pi * j / sides) + 0.0005, z)
                     for j in range(sides)])
    return kit.loft("Ferrule", rows, material=iron, cap_start=False, cap_end=(0.0, 0.0005, Z_SHANK + 0.0005))


def build_butt(kit, iron):
    """Washer and the tang's peened end on the butt."""
    washer = kit.cylinder("Washer", 0.0085, 0.0018, (0, 0.0005, Z_BUTT - 0.0009), sides=24, material=iron)
    rows = []
    for z, r in ((Z_BUTT - 0.0016, 0.0042), (Z_BUTT - 0.0028, 0.0040), (Z_BUTT - 0.0038, 0.0029), (Z_BUTT - 0.0043, 0.0012)):
        rows.append([(r * math.cos(2 * math.pi * j / 16), r * math.sin(2 * math.pi * j / 16) + 0.0005, z) for j in range(16)])
    peen = kit.loft("Peen", rows, material=iron, cap_start=True, cap_end=True)
    return [washer, peen]


def build(kit):
    mats = kit.mats
    # Freshly hafted: pale seasoned ash, only lightly handled.
    wood = mats.wood("M_BillhookAsh", light=(0.25, 0.18, 0.105), dark=(0.13, 0.085, 0.045),
                     grain=0.9, roughness=0.68, weathering=0.1, grime=0.35, seed=7.0,
                     polish=0.35, polish_center=0.0, polish_length=0.07, relief=0.6)
    # The salvaged head: heavy old rust and scale, but a newly honed bright edge.
    steel = mats.steel("M_BillhookSteel", bevel=0.0032, scale_from=0.011, patina=0.9, rust=0.95, seed=5.0)
    # Scale everywhere on the ferrule and butt (no honed zone): pcoord there is rest position.
    iron = mats.steel("M_BillhookIron", bevel=-1.0, scale_from=-0.05, patina=0.9, rust=0.85, seed=9.0)
    parts = [build_blade(kit, steel), build_handle(kit, wood), build_ferrule(kit, iron)] + build_butt(kit, iron)
    return kit.join(parts, "SM_Billhook", pivot=None, unwrap=False, reshade=True, smooth_angle=35)
