"""Cord belt: an 8 mm two-ply plant-fibre cord tied round the waist with a reef knot
in front, the two ends hanging as short tails with stopper knots. It carries the
forage pouch (whose hanging loop is tied onto it at the right hip).

Real-object research (written before modeling):
- Primitive belts are commonly a length of hand-laid two-ply cordage (dogbane, nettle,
  basswood or yucca fibre) 6-10 mm thick, simply wrapped around the waist and tied off
  with a reef (square) knot, tails left 8-12 cm long and finished with overhand knots
  so they do not unravel.
- Adult waist ~75-85 cm around; seen from above the waist is an ellipse roughly
  28-30 cm wide by 21-23 cm deep.

Rigid static mesh. Units are meters, Z up, -Y forward.
PIVOT: the origin is the centre of the loop at the height of the cord's centreline
(the waist centre), so it can sit on a pelvis socket. The loop lies in the XY plane:
the ellipse is 28.4 cm across X (hip to hip) and 22 cm along Y (belly to back), the
cord centreline measures ~80 cm around, the knot is at the front (-Y) and the tails
hang down (-Z) from it.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math

from mathutils import Vector, noise

NAME = "CordBelt"
DESCRIPTION = ("Two-ply plant-fibre cord belt, ~80 cm loop of 8 mm cord tied with a reef knot in front "
               "(original). Pivot = loop centre at cord height; knot faces -Y, tails hang -Z.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
BAKE = {"size": 2048, "samples": 64}
# Seen from above-front like a coiled belt on the ground; close-up on the knot.
BEAUTY = {"pose": (0, 0, 20), "focus": (0.0, -0.11, -0.02)}

A = 0.142          # half width (X)
B = 0.110          # half depth (Y)
CORD_R = 0.004
SIDES = 10

# Fitted path round her shorts' waistband (cord_belt_fit.py); the ellipse is the fallback.
import json
import os
_CONTOUR_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..',
                             'Assets', 'Props', 'CordBelt', 'cord_belt_contour.json')
CONTOUR = None
if os.path.exists(_CONTOUR_FILE):
    with open(_CONTOUR_FILE) as _f:
        CONTOUR = [Vector(p) for p in json.load(_f)['points']]


def ellipse(theta):
    if CONTOUR:
        f = (theta % (2 * math.pi)) / (2 * math.pi) * len(CONTOUR)
        i = int(f) % len(CONTOUR)
        p = CONTOUR[i].lerp(CONTOUR[(i + 1) % len(CONTOUR)], f - int(f))
        # Only ever wobble outward: the fit already sits the cord on the cloth.
        out = Vector((p.x, p.y, 0.0)).normalized()
        wobble = 0.0008 * (1 + noise.noise(Vector((math.cos(theta) * 1.7, math.sin(theta) * 1.7, 0.5))))
        lift = 0.0015 * noise.noise(Vector((math.cos(theta) * 2.3, math.sin(theta) * 2.3, 3.0)))
        return p + out * wobble + Vector((0, 0, lift))
    wobble = 0.0025 * noise.noise(Vector((math.cos(theta) * 1.7, math.sin(theta) * 1.7, 0.5)))
    lift = 0.003 * noise.noise(Vector((math.cos(theta) * 2.3, math.sin(theta) * 2.3, 3.0)))
    return Vector(((A + wobble) * math.cos(theta), (B + wobble) * math.sin(theta), lift))


def build_cord(kit, cord):
    """One continuous cord: tail A up into the knot, round the waist, back through the
    knot and down as tail B."""
    front = ellipse(-0.5 * math.pi)
    gap = 0.075                                    # radians taken up by the knot
    pts, radii = [], []

    def tail(side, length, seed):
        out = []
        base = front + Vector((0.009 * side, -0.006, -0.008))
        for i in range(14):
            t = i / 13
            sway = 0.012 * side * t + 0.004 * noise.noise(Vector((t * 2.0, seed, 1.0)))
            out.append((base + Vector((sway, -0.004 * t - 0.003 * t * t, -length * t)), t))
        return out

    def tail_radius(t):
        r = CORD_R * (1 + 0.6 * math.exp(-((t - 0.83) / 0.065) ** 2) ** 0.6)   # overhand stopper knot
        return r * (0.8 + 0.2 * (1 - smoothstep(0.93, 1.0, t)))        # frayed, untwisting end

    for p, t in reversed(tail(-1, 0.095, 1.0)):
        pts.append(p)
        radii.append(tail_radius(t))
    # Rise out of the knot and follow the waist all the way round.
    count = 88
    for i in range(count + 1):
        theta = -0.5 * math.pi + gap + (2 * math.pi - 2 * gap) * i / count
        pts.append(ellipse(theta))
        radii.append(CORD_R * (1 + 0.04 * noise.noise(Vector((theta * 3.0, 7.0, 0.0)))))
    for p, t in tail(1, 0.11, 2.0)[1:]:
        pts.append(p)
        radii.append(tail_radius(t))
    return kit.tube("Belt", pts, radii=radii, sides=SIDES, material=cord)


def build_knot(kit, cord):
    """Reef knot: two flattened bights interlocked side by side, each passing in front
    of and then behind the other, as it reads from the front."""
    centre = ellipse(-0.5 * math.pi) + Vector((0.0, -0.0045, -0.0015))
    parts = []
    for side in (-1, 1):
        points = []
        for i in range(29):
            t = 2 * math.pi * i / 28
            x = side * (0.0055 + 0.0095 * math.cos(t))
            z = 0.0062 * math.sin(t)
            y = 0.0042 * math.sin(t) * side - 0.0015 * math.cos(t)
            points.append(centre + Vector((x, y, z)))
        parts.append(kit.tube("KnotBight", points, radius=CORD_R * 0.97, sides=SIDES, material=cord))
    return parts


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def build(kit):
    mats = kit.mats
    cord = mats.rawhide("M_BeltCord", color=(0.21, 0.165, 0.10), strands=2, twist=48.0)
    parts = [build_cord(kit, cord)] + build_knot(kit, cord)
    return kit.join(parts, "SM_CordBelt", pivot=None, unwrap=False, reshade=True, smooth_angle=180)
