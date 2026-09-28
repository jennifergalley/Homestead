"""Salvaged estate axe: a rusted English-pattern forged iron head she finds in the ruin, wedged on
a fresh ash haft she cut herself. It replaces the flint hatchet on the fixed estate.

Real-object research (written before modeling):
- 19th-century English woodsman's and "Kent"/"Yankee"-pattern axes: a 2.5-3.5 lb forged head
  12-16 cm from poll to edge, the bit flaring from a 6 cm tall eye to a 9-11 cm convex edge,
  the cheeks hollow-ground so the bit thins to under 1 mm; a flat, slightly mushroomed poll.
  The eye is teardrop-shaped, about 6 x 2.5 cm, with the haft wedged in from the top.
- A short haft (a "boy's" or half axe, 55-65 cm) of straight-grained ash, oval, swelling at the
  eye and knobbed at the butt.
- Wear: black forge scale and orange-brown rust, heaviest on the cheeks and poll; the edge is
  ground bright where she's sharpened it on a stone; the new haft is pale and clean.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters, Z up. Frame matches SM_FlintHatchet (so the felling and strike clips fit): pivot
at the lower (dominant) hand's grip 7 cm above the butt, haft along +Z (butt z -0.07, top z +0.53),
edge facing -Y with its centre at about (0, -13.9, 43) cm, poll toward +Y, cheeks toward +-X.
"""
import math

from mathutils import Vector, noise

NAME = "EstateAxe"
DESCRIPTION = ("Salvaged English-pattern axe: rusted forged iron head on a new ash haft (original). "
               "Pivot = lower-hand grip 7 cm above the butt; head +Z, edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
BEAUTY = {"pose": (0, 90, 28), "focus": (0.0, -0.07, 0.45)}

Z_BUTT = -0.07
HAFT = 0.585
Z_TOP = Z_BUTT + HAFT
Z_EYE = 0.462            # centre of the eye
EDGE_Y = -0.139          # the edge centre's reach toward -Y
POLL_Y = 0.034
EDGE_DROP = 0.028        # the beard sweeps down: the edge centre sits this far below the eye centre


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def haft_section(v):
    """Oval half-widths (x across the cheeks, y along the head) at v (0 butt, 1 top)."""
    swell = 0.0145 + 0.0035 * smoothstep(0.6, 0.93, v)
    knob = 0.0045 * math.exp(-((v - 0.03) / 0.03) ** 2)
    neck = -0.0015 * math.exp(-((v - 0.1) / 0.04) ** 2)
    r = swell + knob + neck
    return r * 0.82, r


def build_haft(kit, wood):
    rows, coords = [], []
    sides = 28
    vs = [0.0, 0.005, 0.015, 0.03] + [0.03 + 0.97 * i / 40 for i in range(1, 41)]
    for v in vs:
        z = Z_BUTT + HAFT * v
        rx, ry = haft_section(v)
        if v < 0.03:
            f = math.sqrt(max(0.0, 1 - ((0.03 - v) / 0.03) ** 2)) ** 0.6
            rx, ry = rx * f, ry * f
        # The haft passes through the teardrop eye: narrower toward the bit.
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            wob = 1 + 0.012 * noise.noise(Vector((math.cos(a) * 3, math.sin(a) * 3, z * 25)))
            ring.append((rx * math.cos(a) * wob, ry * math.sin(a) * wob, z))
            co.append((math.cos(a) * 0.02, math.sin(a) * 0.02, z))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Haft", rows, material=wood, coords=coords, cap_start=(0, 0, Z_BUTT - 0.001), cap_end=True)


def head_section(y):
    """(half-thickness in x, half-height in z, centre z, t) at y along the head."""
    # t: 0 at the poll, 1 at the edge.
    t = (POLL_Y - y) / (POLL_Y - EDGE_Y)
    eye = math.exp(-(((y - 0.002) / 0.03) ** 2))
    # Thickness: a stout poll and eye, then hollow-ground cheeks drawing down to a fine edge.
    w_body = 0.017 * (1 - smoothstep(0.35, 1.0, t)) ** 1.25 + 0.0007
    w = w_body + 0.0045 * eye
    # The top line runs nearly straight; the beard flares down to a 10.5 cm edge.
    top = Z_EYE + 0.029 + 0.004 * eye - 0.006 * t
    bot = Z_EYE - 0.029 - 0.004 * eye - 0.05 * smoothstep(0.3, 1.0, t) ** 1.5
    # Mushroomed poll.
    poll = smoothstep(0.05, 0.0, t)
    top += 0.0025 * poll
    bot -= 0.0025 * poll
    w += 0.0015 * poll
    return w, 0.5 * (top - bot), 0.5 * (top + bot), t


def build_head(kit, iron):
    rows, coords = [], []
    sides = 28
    count = 56
    for i in range(count + 1):
        u = i / count
        y = POLL_Y + (EDGE_Y - POLL_Y) * (u ** 0.85)
        w, h, cz, t = head_section(y)
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            c, s = math.cos(a), math.sin(a)
            # Rounded rectangle, squarer toward the poll, lens-like toward the bit.
            k = 3.4 - 1.2 * t
            px = w * math.copysign(abs(c) ** (2 / k), c)
            pz = h * math.copysign(abs(s) ** (2 / k), s)
            # Convex edge: the heel and toe of the bit sit back from the edge centre.
            yy = y + 0.013 * (t ** 3) * (pz / max(h, 1e-6)) ** 2
            # The toe (top corner) sweeps slightly further forward than the heel.
            yy -= 0.004 * (t ** 3) * max(0.0, pz / max(h, 1e-6))
            ring.append((px, yy, cz + pz))
            # pcoord: (thickness, distance from the edge, along the edge)
            co.append((px, (1 - t) * (POLL_Y - EDGE_Y), pz))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Head", rows, material=iron, coords=coords, cap_start=True, cap_end=True)


def build_wedge(kit, iron):
    """An iron wedge driven into the split haft end, standing just proud of the eye."""
    return kit.box("Wedge", (0.0035, 0.024, 0.009), location=(0, 0, Z_TOP - 0.003), material=iron, bevel=0.0008)


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_EstateAxeAsh", light=(0.26, 0.19, 0.11), dark=(0.135, 0.088, 0.047),
                     grain=0.9, roughness=0.66, weathering=0.06, grime=0.25, seed=21.0,
                     polish=0.25, polish_center=[0.0, 0.0], polish_length=0.07, relief=0.6)
    # Rust and scale everywhere except the freshly ground edge bevel.
    iron = mats.steel("M_EstateAxeIron", bevel=0.009, scale_from=0.03, patina=0.85, rust=0.85, seed=23.0)
    parts = [build_haft(kit, wood), build_head(kit, iron), build_wedge(kit, iron)]
    return kit.join(parts, "SM_EstateAxe", pivot=None, unwrap=False, reshade=True, smooth_angle=40)


REPORT = {"attach": {
    "units": "cm, from the mesh pivot (lower-hand grip centre), mesh space",
    "pivot": "lower (dominant) hand grip centre, 7 cm above the butt",
    "handle_axis": "+Z (head end)",
    "edge_direction": "-Y",
    "upper_hand_choke": [0.0, 0.0, 18.0],
    "edge_centre": [0.0, round(EDGE_Y * 100, 2), round((Z_EYE - EDGE_DROP) * 100, 2)],
    "butt_end": [0.0, 0.0, Z_BUTT * 100],
    "haft_top": [0.0, 0.0, round(Z_TOP * 100, 2)],
}}
