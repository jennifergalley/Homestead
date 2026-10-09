"""Original painted cork float for the deployed fishing line (harder-bob-fishing).

Usage: Scripts\\Blender\\New-Prop.ps1 fishing_float

Research: nineteenth-century coarse anglers fished "pike bungs" and Thames floats: a turned
cork body, painted bright above the waterline so it shows at distance and left pale or
cream below, threaded on a goose quill whose painted tip stands clear of the water. The line
runs through a whipped loop at the quill's foot. This is an original turned body on a quill,
red above the waterline and cream below, with a red-tipped quill and a flax whipping where
the quill leaves the cork. No plastic bobber or borrowed geometry.

Authored dimensions: cork body 48 mm across and 60 mm tall, quill 4.4 mm across running
15 mm below the cork and 70 mm above it. The engine shows it larger than life
(HomesteadCharacterFishing.cpp FloatDisplayScale) so it reads at the gameplay camera.
The base pivot is the quill's foot; the waterline (the paint seam) is REPORT waterline_cm
above it.
"""
import math

from mathutils import Vector

NAME = "FishingFloat"
DESCRIPTION = "Original painted cork fishing float on a red-tipped quill."
PROVENANCE = "Original procedural geometry and PBR materials; no borrowed mesh or image."
COLLISION = "none"
TRIANGLE_BUDGET = 8000
BAKE = {"size": 2048, "samples": 96,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.055), "detail_distance": 0.35}

QUILL_BELOW = 0.015
CORK_HEIGHT = 0.060
CORK_RADIUS = 0.024
CORK_WIDEST = 0.42   # fraction of the height where the body is widest
SEAM = 0.50          # paint seam (waterline) as a fraction of the cork's height
QUILL_ABOVE = 0.070
QUILL_RADIUS = 0.0022
TIP_LENGTH = 0.024
SIDES = 36
RINGS = 40
REPORT = {"waterline_cm": round(100.0 * (QUILL_BELOW + SEAM * CORK_HEIGHT), 2),
          "top_cm": round(100.0 * (QUILL_BELOW + CORK_HEIGHT + QUILL_ABOVE), 2),
          "cork_radius_cm": 100.0 * CORK_RADIUS}


def cork_radius(f: float) -> float:
    """Turned body: a full shoulder above a slightly longer taper to the foot."""
    if f <= CORK_WIDEST:
        u = f / CORK_WIDEST
        shape = math.sin(0.5 * math.pi * u) ** 0.75
    else:
        u = (f - CORK_WIDEST) / (1.0 - CORK_WIDEST)
        shape = math.cos(0.5 * math.pi * u) ** 0.62
    return max(QUILL_RADIUS * 1.15, CORK_RADIUS * shape)


def body(kit, name, f0, f1, mat):
    rings = max(4, int(RINGS * (f1 - f0)) + 1)
    fs = [f0 + (f1 - f0) * i / (rings - 1) for i in range(rings)]
    points = [Vector((0.0, 0.0, QUILL_BELOW + f * CORK_HEIGHT)) for f in fs]
    return kit.tube(name, points, radii=[cork_radius(f) for f in fs], sides=SIDES,
                    material=mat, cap=True)


def quill(kit, name, z0, z1, r0, r1, mat, tip=False):
    count = 16
    points = [Vector((0.0, 0.0, z0 + (z1 - z0) * i / (count - 1))) for i in range(count)]
    radii = []
    for i in range(count):
        t = i / (count - 1)
        r = r0 + (r1 - r0) * t
        if tip:
            r *= math.sqrt(max(0.0, 1.0 - t ** 6))
        radii.append(max(r, 0.0003))
    return kit.tube(name, points, radii=radii, sides=16, material=mat, cap=True)


def whipping(kit, name, z0, z1, turns, mat, radius):
    count = int(turns * 10)
    points = []
    for i in range(count + 1):
        t = i / count
        angle = 2.0 * math.pi * turns * t
        points.append(Vector((radius * math.cos(angle), radius * math.sin(angle), z0 + (z1 - z0) * t)))
    return kit.tube(name, points, radius=0.00042, sides=8, material=mat, cap=True)


def build(kit):
    red = kit.mats.painted_wood("M_FishingFloatRed", paint=(0.40, 0.032, 0.020),
                                under=(0.33, 0.22, 0.12), wear=0.18, grime=0.12, seed=3.0)
    cream = kit.mats.painted_wood("M_FishingFloatCream", paint=(0.46, 0.41, 0.31),
                                  under=(0.33, 0.22, 0.12), wear=0.22, grime=0.30, seed=7.0)
    quill_mat = kit.mats.wood("M_FishingFloatQuill", light=(0.50, 0.41, 0.25),
                              dark=(0.30, 0.23, 0.13), grain=0.25, roughness=0.42)
    flax = kit.mats.linen_cord("M_FishingFloatWhip", radius=0.00042)
    cork_top = QUILL_BELOW + CORK_HEIGHT
    top = cork_top + QUILL_ABOVE
    parts = [
        body(kit, "CorkBelow", 0.0, SEAM, cream),
        body(kit, "CorkAbove", SEAM, 1.0, red),
        quill(kit, "QuillFoot", 0.0, QUILL_BELOW + 0.004, QUILL_RADIUS * 0.75, QUILL_RADIUS, quill_mat),
        quill(kit, "QuillShaft", cork_top - 0.004, top - TIP_LENGTH, QUILL_RADIUS, QUILL_RADIUS * 0.82, quill_mat),
        quill(kit, "QuillTip", top - TIP_LENGTH, top, QUILL_RADIUS * 0.82, QUILL_RADIUS * 0.55, red, tip=True),
        whipping(kit, "TopWhipping", cork_top - 0.0005, cork_top + 0.004, 7.0, flax, QUILL_RADIUS + 0.0004),
        whipping(kit, "FootWhipping", 0.002, 0.007, 8.0, flax, QUILL_RADIUS * 0.8 + 0.0004),
    ]
    return kit.join(parts, "SM_FishingFloat", unwrap=True, reshade=True, smooth_angle=60)
