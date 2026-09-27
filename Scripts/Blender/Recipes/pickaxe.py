"""Salvaged navvy's pick: a forged iron head with a long point and a flat chisel, on a new ash haft
wedged through the eye. The head is the rusted one she finds in the ruin; the haft is fresh.

Real-object research (written before modeling):
- 19th-century English railway and quarry picks ("navvy picks", "grubbing picks"): a 5-7 lb head
  about 17-20 in (43-50 cm) tip to tip, one arm drawn to a square point, the other to a flat
  chisel about 2 in (5 cm) wide; both arms curve gently down toward the ground. The eye is an oval
  swelling (6-7 cm deep) in the middle of the head.
- Hafts are ash (or hickory later), 30-36 in (76-91 cm), oval, swelling toward the eye end and
  wedged at the top so the head can't fly off; a knob at the butt.
- Wear: the head is black and brown with heavy rust scale, the point and chisel ground bright and
  burred from use; the haft is pale and clean because it is new.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters. Orientation matches SM_FlintHatchet so the felling clip's two-handed grip fits:
pivot at the lower (dominant) hand's grip 7 cm above the butt, haft along +Z, head at the top with
its point toward -Y (where the axe's edge faces) and chisel toward +Y, flats toward +-X.
"""
import math

from mathutils import Vector, noise

NAME = "Pickaxe"
DESCRIPTION = ("Salvaged navvy's pick: rusted point-and-chisel iron head on a new ash haft (original). "
               "Pivot = lower-hand grip 7 cm above the butt; head +Z, point -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
BEAUTY = {"pose": (0, 90, 28), "focus": (0.0, 0.0, 0.66)}

Z_BUTT = -0.07
HAFT = 0.78
Z_HEAD = Z_BUTT + HAFT - 0.035     # centre of the eye
HEAD_HALF = 0.23                   # point and chisel reach from the eye


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def haft_section(v):
    """Oval section half-widths (x across the flats, y along the head) at v (0 butt, 1 top)."""
    swell = 0.0165 + 0.004 * smoothstep(0.55, 0.95, v)
    knob = 0.004 * math.exp(-((v - 0.02) / 0.025) ** 2)
    neck = -0.0015 * math.exp(-((v - 0.08) / 0.04) ** 2)
    return (swell + knob + neck) * 0.86, swell + knob + neck


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
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            wob = 1 + 0.01 * noise.noise(Vector((math.cos(a) * 3, math.sin(a) * 3, z * 25)))
            ring.append((rx * math.cos(a) * wob, ry * math.sin(a) * wob, z))
            co.append((math.cos(a) * 0.02, math.sin(a) * 0.02, z))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Haft", rows, material=wood, coords=coords, cap_start=(0, 0, Z_BUTT - 0.001), cap_end=True)


def head_profile(s):
    """Head section half-widths (x across, z depth) at s in [-1, 1] (-1 point end at -Y, +1 chisel)."""
    t = abs(s)
    eye = 0.034 * math.exp(-(t / 0.2) ** 2)
    if s < 0:
        # Square-sectioned point: tapers steadily to a blunted tip.
        w = 0.013 * (1 - t) ** 0.8 + 0.0018
        d = 0.016 * (1 - t) ** 0.9 + 0.002
    else:
        # Chisel: stays deep-ish but flattens to a 5 cm wide, thin edge.
        w = 0.013 + (0.025 - 0.013) * smoothstep(0.55, 1.0, t)
        d = 0.016 * (1 - 0.88 * smoothstep(0.35, 1.0, t)) + 0.0012
    return w + eye * 0.35, d + eye


def build_head(kit, iron):
    rows, coords = [], []
    sides = 20
    ss = [-1.0 + 2.0 * i / 60 for i in range(61)]
    for s in ss:
        y = s * HEAD_HALF
        # Both arms curve down toward the ground.
        droop = -0.045 * (abs(s) ** 1.8)
        cz = Z_HEAD + droop
        w, d = head_profile(s)
        ring, co = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            # Rounded-square section: forged, not turned.
            c, sn = math.cos(a), math.sin(a)
            k = 3.2
            px = w * math.copysign(abs(c) ** (2 / k), c)
            pz = d * math.copysign(abs(sn) ** (2 / k), sn)
            ring.append((px, y, cz + pz))
            # pcoord: (across, distance from the nearest working end, along the head)
            co.append((px * 12.0, (1 - abs(s)) * HEAD_HALF, y))
        rows.append(ring)
        coords.append(co)
    return kit.loft("Head", rows, material=iron, coords=coords, cap_start=True, cap_end=True)


def build_wedge(kit, iron):
    """The haft end standing just proud of the eye, split by an iron wedge."""
    top = Z_HEAD + 0.036
    wedge = kit.box("Wedge", (0.004, 0.03, 0.012), location=(0, 0, top + 0.004), material=iron)
    return wedge


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_PickaxeAsh", light=(0.25, 0.18, 0.105), dark=(0.13, 0.085, 0.045),
                     grain=0.9, roughness=0.66, weathering=0.08, grime=0.3, seed=11.0,
                     polish=0.3, polish_center=[0.0, 0.25], polish_length=0.07, relief=0.6)
    # Rusted head: scale and rust everywhere except the ground point and chisel (bevel from the ends).
    iron = mats.steel("M_PickaxeIron", bevel=0.025, scale_from=0.04, patina=0.9, rust=0.9, seed=13.0)
    parts = [build_haft(kit, wood), build_head(kit, iron), build_wedge(kit, iron)]
    return kit.join(parts, "SM_Pickaxe", pivot=None, unwrap=False, reshade=True, smooth_angle=40)
