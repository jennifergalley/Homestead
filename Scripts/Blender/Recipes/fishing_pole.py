"""Original rural hazel fishing pole.

Usage: Scripts\\Blender\\New-Prop.ps1 fishing_pole -Live

Research: https://www.fishingmuseum.org.uk/rods_overview.html describes six-foot
hazel shoots with a tip-fixed horsehair line, wound around the pole for travel.
This is a reel-less 1.95 m hazel pole with an individually swept flax line,
whipped tip loop, carved wood float and forged iron hook. No modern spinning
reel, carbon blank, plastic bobber or borrowed geometry.

Authored dimensions: butt diameter 29 mm, working tip diameter 3.2 mm, grip
diameter about 29 mm with a small linen binding, float 75 x 16 mm, hook 21 mm.
The grip's centre is the origin; butt -Z, working tip +Z. A natural taper and
small bow are intentional, not an attachment-scale correction. The wrapped
line/secured tackle is the travel presentation, not an animated deployed line.
"""
import math

from mathutils import Vector, noise

NAME = "FishingPole"
DESCRIPTION = "Original tapered hazel pole, wound flax line, wooden float and forged hook."
PROVENANCE = "Original procedural geometry and PBR materials; no borrowed mesh or image."
COLLISION = "none"
TRIANGLE_BUDGET = 50000
BAKE = {"size": 4096, "samples": 96, "repack": False,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 90, 24), "focus": (0.025, -0.018, 0.24),
          "detail_distance": 0.85}
REPORT = {"attach": {"grip_cm": [0.0, 0.0, 0.0],
                     "butt_cm": [0.0, 0.0, -20.0],
                     "tip_cm": [3.6, -1.0, 175.0]},
          "presentation": "secured travel tackle; no deployed-line animation"}

BUTT_Z = -0.20
TIP_Z = 1.75
LENGTH = TIP_Z - BUTT_Z
ROD_SIDES = 40
ROD_RINGS = 161
LINE_RADIUS = 0.00045


def axis(z: float) -> Vector:
    """Hazel centreline, passing through the hand's pivot."""
    t = z / TIP_Z
    return Vector((0.036 * t * t + 0.002 * math.sin(t * 8.0),
                   -0.01 * t * t + 0.0013 * math.sin(t * 5.0), z))


def radius(z: float) -> float:
    t = min(1.0, max(0.0, (z - BUTT_Z) / LENGTH))
    return 0.0016 + 0.0129 * (1.0 - t) ** 1.08


def rod_uvs(obj) -> None:
    """Four straight shaft islands reserve texels rather than a diagonal sliver."""
    layer = obj.data.uv_layers["UVMap"].data
    body_faces = (ROD_RINGS - 1) * ROD_SIDES
    for index, poly in enumerate(obj.data.polygons):
        segment = min(3, index // ROD_SIDES // 40)
        seam = index < body_faces and index % ROD_SIDES == ROD_SIDES - 1
        for loop, vertex in zip(poly.loop_indices, poly.vertices):
            ring, side = divmod(min(vertex, ROD_RINGS * ROD_SIDES - 1), ROD_SIDES)
            u = 1.0 if seam and side == 0 else side / ROD_SIDES
            if index < body_faces:
                v = (ring - segment * 40) / 40.0
                layer[loop].uv = (0.015 + segment * 0.16 + 0.14 * u,
                                 0.04 + 0.91 * v)
            else:
                cap = 0 if index < body_faces + ROD_SIDES else 1
                if vertex >= ROD_RINGS * ROD_SIDES:
                    layer[loop].uv = (0.70 + cap * 0.09, 0.94)
                else:
                    angle = side * 2.0 * math.pi / ROD_SIDES
                    layer[loop].uv = (0.70 + cap * 0.09 + 0.025 * math.cos(angle),
                                     0.94 + 0.025 * math.sin(angle))


def build_rod(kit, wood):
    zs = [BUTT_Z + LENGTH * i / (ROD_RINGS - 1) for i in range(ROD_RINGS)]
    obj = kit.tube("HazelBlank", [axis(z) for z in zs],
                   radii=[radius(z) for z in zs], sides=ROD_SIDES, material=wood)
    knots = [(0.39, 0.4), (0.84, 3.1), (1.26, 1.7)]

    def relief(co, pco):
        z = co.z
        angle = math.atan2(pco.y, pco.x)
        value = noise.noise(Vector((pco.x * 130, pco.y * 130, z * 11))) * 0.00010
        value -= abs(math.cos(angle * 4.0 + z * 0.7)) * 0.00013
        for knot_z, knot_angle in knots:
            delta = math.atan2(math.sin(angle - knot_angle), math.cos(angle - knot_angle))
            value += 0.00065 * math.exp(-((z - knot_z) / 0.009) ** 2
                                       - (delta / 0.35) ** 2)
        return value * min(1.0, radius(z) / 0.006)

    kit.displace(obj, relief)
    rod_uvs(obj)
    return obj


def cord(kit, name, points, mat, rect, thickness=LINE_RADIUS):
    obj = kit.tube(name, points, radius=thickness, sides=12, material=mat)
    kit.assign_tube_uvs(obj, rect, 12, len(points))
    return obj


def whipping(kit, name, z0, z1, turns, mat, rect, thickness=LINE_RADIUS):
    points = []
    steps = int(turns * 24)
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        angle = 2.0 * math.pi * turns * t
        r = radius(z) + thickness * 0.95
        points.append(axis(z) + Vector((r * math.cos(angle), r * math.sin(angle), 0)))
    return cord(kit, name, points, mat, rect, thickness)


def build_float(kit, wood):
    points = [leader_axis(0.215 + 0.075 * i / 48) for i in range(49)]
    obj = kit.tube("CarvedFloat", points, radius=lambda t:
                   0.0013 + 0.0067 * math.sin(math.pi * t) ** 0.8,
                   sides=32, material=wood)
    kit.displace(obj, lambda co, pco:
                 0.000055 * noise.noise(Vector((pco.x * 600, pco.y * 600, pco.z * 110))))
    kit.assign_tube_uvs(obj, (0.67, 0.78, 0.40, 0.82), 32, len(points))
    return obj


def leader_axis(z: float) -> Vector:
    t = (0.339 - z) / 0.203
    start = axis(0.339) + Vector((radius(0.339) + LINE_RADIUS * 0.95, 0, 0))
    end = Vector((0.022, -0.037, 0.136))
    point = start.lerp(end, t)
    point.x += 0.032 * math.sin(math.pi * t)
    return point


def build_hook(kit, iron):
    points = [Vector((0.022, -0.037, 0.135 - 0.012 * i / 12)) for i in range(13)]
    centre = Vector((0.022, -0.037, 0.123))
    for i in range(1, 33):
        angle = -math.pi / 2.0 + math.pi * 1.22 * i / 32
        points.append(centre + Vector((0.0045 + 0.0045 * math.sin(angle), 0.0,
                                       0.0045 * math.cos(angle))))
    radii = [0.00048] * len(points)
    for i in range(1, 9):
        radii[-i] *= max(0.12, i / 9)
    obj = kit.tube("ForgedHook", points, radii=radii, sides=12, material=iron)
    kit.assign_tube_uvs(obj, (0.81, 0.88, 0.05, 0.20), 12, len(points))
    barb = cord(kit, "HookBarb", [points[-5], points[-5] + Vector((-0.0014, 0, -0.002))],
                iron, (0.91, 0.94, 0.05, 0.20), 0.00019)
    eye_points = [Vector((0.022 + 0.0013 * math.sin(i * math.pi / 12), -0.037,
                         0.1363 + 0.0013 * math.cos(i * math.pi / 12)))
                  for i in range(25)]
    eye = cord(kit, "HookEye", eye_points, iron, (0.95, 0.98, 0.05, 0.20), 0.00035)
    return [obj, barb, eye]


def build(kit):
    wood = kit.mats.wood("M_FishingPoleHazel", light=(0.27, 0.183, 0.102),
                         dark=(0.125, 0.079, 0.041), roughness=0.60,
                         weathering=0.08, grime=0.08, seed=17.0, grain=0.65,
                         polish=0.65, polish_center=0.20, polish_length=0.07, relief=0.35)
    flax = kit.mats.linen_cord("M_FishingPoleFlax", radius=LINE_RADIUS)
    iron = kit.mats.steel("M_FishingPoleHook", patina=0.75, rust=0.08, seed=23.0)
    float_wood = kit.mats.wood("M_FishingPoleFloat", light=(0.30, 0.22, 0.12),
                               dark=(0.17, 0.11, 0.053), grain=0.45,
                               roughness=0.66, char_above=0.062, char_band=0.004)
    parts = [build_rod(kit, wood)]
    parts.append(whipping(kit, "GripBinding", -0.060, -0.036, 12.0, flax,
                          (0.67, 0.78, 0.04, 0.20), 0.00065))
    parts.append(whipping(kit, "TipWhipping", 1.708, 1.732, 14.0, flax,
                          (0.81, 0.89, 0.23, 0.39)))
    loop_points = [axis(1.739) + Vector((0.0032 * math.sin(i * math.pi / 20),
                                       -0.0005, 0.011 * math.cos(i * math.pi / 20)))
                   for i in range(41)]
    parts.append(cord(kit, "TipLineLoop", loop_points, flax, (0.91, 0.98, 0.23, 0.39)))
    line = []
    for i in range(85):
        t = i / 84
        z = 1.738 - 1.417 * t
        line.append(axis(z) + Vector((0.007 * math.sin(math.pi * t),
                                      -radius(z) - LINE_RADIUS, 0)))
    parts.append(cord(kit, "LineAlongBlank", line, flax, (0.81, 0.88, 0.43, 0.82)))
    turn = [axis(0.321) + Vector(((radius(0.321) + LINE_RADIUS) * math.cos(a),
                                  (radius(0.321) + LINE_RADIUS) * math.sin(a), 0))
            for a in [-math.pi / 2 + math.pi * i / 48 for i in range(25)]]
    parts.append(cord(kit, "LineTurn", turn, flax, (0.81, 0.88, 0.84, 0.90)))
    parts.append(whipping(kit, "StoredLine", 0.321, 0.339, 9.0, flax,
                          (0.91, 0.98, 0.43, 0.82)))
    line_end = []
    for i in range(37):
        t = i / 36
        line_end.append(leader_axis(0.339 - 0.203 * t))
    parts.append(cord(kit, "TackleLeader", line_end, flax, (0.67, 0.75, 0.24, 0.37)))
    parts.append(build_float(kit, float_wood))
    parts.extend(build_hook(kit, iron))
    obj = kit.join(parts, "SM_FishingPole", pivot=None, unwrap=False,
                   reshade=True, smooth_angle=65)
    return obj


REPORT["attach"]["tip_cm"] = [100.0 * value for value in axis(TIP_Z)]
