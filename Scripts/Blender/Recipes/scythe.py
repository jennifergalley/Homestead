"""English scythe: a salvaged, rusted blade on a new crooked ash snath with two wooden nibs.

Real-object research (written before modeling):
- 19th-century English grass scythes: a long, heavy riveted-back blade of 30-40 in (76-100 cm),
  about 3-4 in (8-10 cm) deep at the heel tapering to the point, with a stiffening rib along the
  back and a tang (the "heel") bent up at the snath. It is held on by an iron ring and wedge.
- The snath ("sned") is ash or willow, 5 ft 6 in to 6 ft (1.6-1.8 m), with the English S-curve, and
  two nibs (grips) wedged into it: the upper about 60 cm below the top end, the lower 40-50 cm
  below that, each about 12-15 cm long. Mowing is a hip-driven swing; the blade travels flat to
  the ground, point forward, at roughly a right angle to the snath.
- Wear: this blade is from the ruin: scale and rust, pitted, with a newly ground edge. The snath
  and nibs are pale new wood.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters. Orientation (for the two-handed sweep): the pivot is the centre of the lower
(right-hand) nib's grip. The snath rises roughly along +Z to its top end and falls to the heel
at the bottom; the nibs stick out along -Y (toward her); the blade runs from the heel along +X
with its cutting edge toward -Y, hung LAY_DEGREES off flat (the edge raised toward the nibs) so
it lies flat on the ground when the snath leans back toward her by that angle.
"""
import math

from mathutils import Vector, noise

NAME = "Scythe"
DESCRIPTION = ("English scythe: rusted riveted-back blade on a crooked ash snath with two nibs (original). "
               "Pivot = lower nib grip; snath +Z, nibs -Y, blade +X with its edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ["basecolor", "roughness", "normal", "metallic", "ao"]}
BEAUTY = {"pose": (90, 0, 0), "focus": (0.3, 0.0, -1.05)}

SNATH_TOP = 0.62
SNATH_BOTTOM = -0.98
# The angle between blade and snath is set ("hung") so the blade lies flat at the mowing lean.
LAY_DEGREES = 30.0


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def snath_point(t):
    """Snath centreline from the heel (t=0) to the top end (t=1), with the English double crook."""
    z = SNATH_BOTTOM + (SNATH_TOP - SNATH_BOTTOM) * t
    y = 0.05 * math.sin(math.pi * (t * 1.6 - 0.1)) + 0.03 * smoothstep(0.75, 1.0, t)
    x = -0.03 * math.sin(math.pi * t)
    return Vector((x, y, z))


def build_snath(kit, wood):
    pts = [snath_point(i / 24) for i in range(25)]
    radii = [0.019 - 0.004 * (i / 24) for i in range(25)]
    return kit.tube("Snath", pts, radii=radii, sides=24, material=wood)


def nib(kit, name, at_z, wood):
    base = min((snath_point(i / 200) for i in range(201)), key=lambda p: abs(p.z - at_z))
    grip = base + Vector((0.0, -0.13, 0.0))
    pts = [base + (grip - base) * f + Vector((0, 0, 0.01 * math.sin(math.pi * f))) for f in (0.0, 0.25, 0.55, 0.85, 1.05)]
    radii = [0.016, 0.014, 0.0155, 0.016, 0.012]
    return kit.tube(name, pts, radii=radii, sides=18, material=wood), grip


def lay(p, heel):
    """The blade's lay: turned about the heel's X axis so that it lies flat on the ground when the
    snath leans back toward the mower by LAY_DEGREES (Content/Python/homestead_agent/scythe_mow.py LEAN)."""
    a = math.radians(LAY_DEGREES)
    c, s = math.cos(a), math.sin(a)
    y, z = p[1] - heel.y, p[2] - heel.z
    return (p[0], heel.y + y * c + z * s, heel.z - y * s + z * c)


def blade_rows():
    """Loft of the blade from the heel along +X: a curved, tapering plate with a rib on its back."""
    heel = snath_point(0.0) + Vector((0.0, 0.0, -0.02))
    rows, coords = [], []
    length = 0.86
    steps = 48
    for i in range(steps + 1):
        u = i / steps
        # Gently curved: the point sweeps back toward the mower (toward -Y).
        cx = heel.x + length * u
        cy = heel.y + 0.02 - 0.16 * u ** 2.2
        depth = (0.095 * (1 - u) ** 0.85 + 0.004) * smoothstep(-0.02, 0.05, u)
        rib = 0.0045 * (1 - 0.7 * u)
        edge_t = 0.0006
        z = heel.z + 0.01 * math.sin(math.pi * u)
        # Section across the blade (spine at +Y side, edge at -Y): rib, plate, bevel.
        profile = [(0.0, rib), (0.012, rib * 0.7), (0.02, 0.0016), (depth * 0.7, 0.0012),
                   (depth - 0.004, 0.0009), (depth, edge_t)]
        ring = []
        co = []
        for d, th in profile:
            ring.append(lay((cx, cy - d, z + th * 0.5), heel))
            co.append((th * 12.0, depth - d, cx))
        for d, th in reversed(profile):
            ring.append(lay((cx, cy - d, z - th * 0.5), heel))
            co.append((-th * 12.0, depth - d, cx))
        rows.append(ring)
        coords.append(co)
    return rows, coords


def build_blade(kit, steel):
    rows, coords = blade_rows()
    blade = kit.loft("Blade", rows, material=steel, coords=coords, cap_start=True, cap_end=True)
    # Heel tang bent up into the snath, and the fixing ring with its wedge.
    heel = snath_point(0.0)
    tang = kit.tube("Tang", [heel + Vector((0.0, 0.0, -0.03)), heel + Vector((0.0, 0.0, 0.02)), heel + Vector((0.0, 0.0, 0.09))],
                    radii=[0.007, 0.0065, 0.006], sides=10, material=steel)
    ring_z = heel.z + 0.06
    ring = kit.cylinder("Ring", 0.024, 0.022, (heel.x, heel.y, ring_z), sides=24, material=steel)
    return [blade, tang, ring]


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_ScytheAsh", light=(0.25, 0.18, 0.105), dark=(0.13, 0.085, 0.045),
                     grain=0.9, roughness=0.66, weathering=0.08, grime=0.25, seed=21.0,
                     polish=0.25, polish_center=[0.0, 0.42], polish_length=0.06, relief=0.6)
    steel = mats.steel("M_ScytheSteel", bevel=0.005, scale_from=0.03, patina=0.85, rust=0.9, seed=17.0)
    lower, grip = nib(kit, "NibLower", 0.0, wood)
    upper, _ = nib(kit, "NibUpper", 0.42, wood)
    parts = [build_snath(kit, wood), lower, upper]
    parts += build_blade(kit, steel)
    obj = kit.join(parts, "SM_Scythe", pivot=None, unwrap=False, reshade=True, smooth_angle=40)
    # Put the pivot at the lower nib's grip.
    for vert in obj.data.vertices:
        vert.co -= grip
    obj.data.update()
    return obj
