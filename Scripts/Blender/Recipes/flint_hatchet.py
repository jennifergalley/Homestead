"""Game-ready flint hatchet: the FlintAxe (``flint_axe.py``) reworked to a hand-held
hotbar tool under 10k triangles, re-oriented for the heroine's hand sockets.

Same object as the FlintAxe: a knapped flint head socketed through a seasoned
wooden haft, bound with twisted rawhide above and below the head and crossed over
both faces. The shape functions (haft taper and bow, head loft, flake scars, lashing
path) are the FlintAxe's own; only the tessellation is reduced (haft 18 sides, head
36 x 52 with no subdivision, lashing 6 sides) and fine detail moves into the 2K bake.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the centre of the lower (dominant) hand's grip on the haft, 7 cm
above the butt. The haft runs along +Z (butt at z -0.07, top at z +0.53); the head sits
near the top with its cutting edge facing -Y, the poll/butt of the head toward +Y and
the broad faces toward +-X. ``REPORT["attach"]`` gives the offsets (cm) from the pivot
to the upper-hand choke grip and to the centre of the cutting edge.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib.util
import math
from pathlib import Path

from mathutils import Matrix, Vector, noise

NAME = "FlintHatchet"
DESCRIPTION = ("Game-ready hafted flint hatchet with rawhide lashing (original, reworked FlintAxe). "
               "Pivot = lower-hand grip 7 cm above the butt; head +Z, edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
# Review lying on the ground, broad face up; close-up on the socket and bit.
BEAUTY = {"pose": (0, 90, 28), "focus": (0.0, -0.075, 0.43)}

_spec = importlib.util.spec_from_file_location("homestead_flint_axe", Path(__file__).with_name("flint_axe.py"))
axe = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(axe)

GRIP_Z = 0.07            # dominant-hand grip centre above the butt (authoring frame)
CHOKE_Z = GRIP_Z + 0.18  # upper-hand choke grip, 18 cm further up the haft
# Authoring frame is the FlintAxe's (haft +Z from the butt, bit +X); the export frame
# turns the bit to -Y and drops the grip centre onto the origin.
_TURN = Matrix.Rotation(math.radians(-90), 4, "Z")
TO_EXPORT = Matrix.Translation(-(_TURN @ axe.haft_axis(GRIP_Z))) @ _TURN


def build_haft(kit, mat):
    points = [axe.haft_axis(axe.HAFT_LENGTH * i / 56) for i in range(57)]
    haft = kit.tube("Haft", points, radius=axe.haft_radius, sides=18, material=mat)
    knots = [(0.18, 1.2, 0.0035), (0.33, 4.1, 0.003), (0.41, 2.4, 0.0025)]

    def relief(co, pco):
        z = pco.z
        angle = math.atan2(pco.y, pco.x)
        bump = noise.noise(Vector((pco.x * 60, pco.y * 60, z * 9))) * 0.0006
        for kz, ka, height in knots:
            d = ((z - kz) / 0.014) ** 2 + ((math.atan2(math.sin(angle - ka), math.cos(angle - ka))) / 0.45) ** 2
            bump += height * math.exp(-d)
        return bump

    return kit.displace(haft, relief)


def build_head(kit, mat):
    rings, sides = 52, 36
    width_bit, thick_max = 0.072, 0.032
    butt, length, head_z = axe.HEAD_BUTT, axe.HEAD_LENGTH, axe.HEAD_Z
    # Rings bunch toward the bit so the edge arc and bevel stay smooth.
    points = [(butt + length * (0.5 - 0.5 * math.cos(math.pi * (i / (rings - 1)) ** 0.85)), 0.0, head_z)
              for i in range(rings)]
    head = kit.tube("Head", points, radius=1.0, sides=sides, material=mat, cap=True)

    def shape(co):
        s = min(max((co.x - butt) / length, 0.0), 1.0)
        cy, cz = co.y, co.z - head_z
        width = width_bit * (0.52 + 0.48 * s ** 0.75)
        thick = max(thick_max * (0.55 + 0.45 * math.sin(math.pi * min(1.0, s * 1.35 + 0.1))) * (1 - s ** 6), 0.0012)
        butt_round = math.sqrt(max(0.0, 1 - ((0.08 - s) / 0.08) ** 2)) if s < 0.08 else 1.0
        ey = math.copysign(abs(cy) ** 0.8, cy) * thick / 2 * butt_round
        ez = math.copysign(abs(cz) ** 0.6, cz) * width / 2 * butt_round
        x = co.x + 0.012 * (1 - (abs(cz) ** 2)) * s ** 5
        return (x, ey, head_z + ez)

    kit.warp(head, shape)
    kit.tag_coords(head.data)

    def flake_scars(co, pco):
        s = (pco.x - butt) / length

        def scar(side):
            point = Vector((pco.x * 55, pco.z * 55, side * 7.3))
            distances, _ = noise.voronoi(point, distance_metric="DISTANCE", exponent=2.5)
            edge = min((distances[1] - distances[0]) / 0.45, 1.0)
            return -(edge ** 0.55)

        blend = min(max((co.y + 0.004) / 0.008, 0.0), 1.0)
        blend = blend * blend * (3 - 2 * blend)
        value = scar(1.0) * blend + scar(-1.0) * (1 - blend)
        rim = min(abs(co.y) / 0.005, 1.0)
        body = min(max((s - 0.10) / 0.08, 0.0), 1.0)
        depth = 0.0030 * (1 - max(0.0, (s - 0.7) / 0.3)) ** 2 * (0.25 + 0.75 * rim) * body
        depth = min(depth, 0.3 * abs(co.y))
        lump = noise.noise(Vector((pco.x * 140, pco.y * 140, pco.z * 140))) * 0.0009 * (1 - body)
        return value * depth + lump

    return kit.displace(head, flake_scars)


def wrap_band(kit, name, mat, z0, z1, turns, phase=0.0, seed=0):
    points, radii = [], []
    steps = int(turns * 16)
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        angle = phase + 2 * math.pi * turns * t
        wobble = noise.noise(Vector((t * 9.0, seed, 0.0)))
        radius = axe.haft_radius(z / axe.HAFT_LENGTH) + axe.CORD * (0.8 + 0.25 * wobble)
        center = axe.haft_axis(z) + Vector((0, 0, 0.0012 * noise.noise(Vector((t * 23.0, seed, 3.0)))))
        points.append(center + Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0)))
        radii.append(axe.CORD * (0.9 + 0.18 * noise.noise(Vector((t * 31.0, seed, 7.0)))))
    return kit.tube(name, points, radii=radii, sides=6, material=mat)


def cross_cord(kit, name, mat, side, rising, offset):
    points = []
    for i in range(19):
        t = i / 18
        angle = side * math.pi / 2 + (t - 0.5) * math.radians(130) * (1 if rising else -1)
        z = axe.HEAD_Z - 0.036 + 0.072 * t + offset
        radius = axe.haft_radius(z / axe.HAFT_LENGTH) + axe.CORD * 0.9
        points.append(axe.haft_axis(z) + Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0)))
    return kit.tube(name, points, radius=axe.CORD * 0.95, sides=6, material=mat)


def export_point(p):
    return TO_EXPORT @ Vector(p)


def attach_offsets():
    choke = export_point(axe.haft_axis(CHOKE_Z))
    edge = export_point((axe.HEAD_BUTT + axe.HEAD_LENGTH + 0.012, 0.0, axe.HEAD_Z))
    butt = export_point(axe.haft_axis(0.0))
    top = export_point(axe.haft_axis(axe.HAFT_LENGTH))

    def cm(v):
        return [round(c * 100, 2) for c in v]

    return {
        "units": "cm, from the mesh pivot (lower-hand grip centre), mesh space",
        "pivot": "lower (dominant) hand grip centre, 7 cm above the butt",
        "handle_axis": "+Z (head end)",
        "edge_direction": "-Y",
        "upper_hand_choke": cm(choke),
        "edge_centre": cm(edge),
        "butt_end": cm(butt),
        "haft_top": cm(top),
    }


def add_scar_bump(mat, scale=55.0, strength=0.55, distance=0.0025, along="x", start=None, end=None,
                  fade=0.02):
    """Layer crisp conchoidal flake-scar relief over a flint material's normal: a dish
    per Voronoi cell of pcoord (along, z) with sharp ridges where scars meet, separate
    patterns on the two faces (sign of pcoord y or x). The low-poly geometry carries the
    broad scars; this puts the ridges into the baked normal map. ``start``/``end`` limit
    the scars along the ``along`` axis (fading over ``fade`` m)."""
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    attr = nodes.new("ShaderNodeAttribute")
    attr.attribute_name, attr.attribute_type = "pcoord", "GEOMETRY"
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(attr.outputs["Vector"], sep.inputs["Vector"])
    across = sep.outputs["Y"] if along == "x" else sep.outputs["X"]
    axis = sep.outputs["X"] if along == "x" else sep.outputs["Y"]

    def math_node(op, a, b=0.0):
        n = nodes.new("ShaderNodeMath")
        n.operation = op
        for sock, v in zip(n.inputs, (a, b)):
            if isinstance(v, float) or isinstance(v, int):
                sock.default_value = v
            else:
                links.new(v, sock)
        return n.outputs[0]

    side = math_node("MULTIPLY", math_node("SIGN", across), 7.3)
    comb = nodes.new("ShaderNodeCombineXYZ")
    links.new(math_node("MULTIPLY", axis, scale), comb.inputs["X"])
    links.new(math_node("MULTIPLY", sep.outputs["Z"], scale), comb.inputs["Y"])
    links.new(side, comb.inputs["Z"])
    vor = nodes.new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    links.new(comb.outputs["Vector"], vor.inputs["Vector"])
    dish = math_node("POWER", math_node("MINIMUM", math_node("DIVIDE", vor.outputs["Distance"], 0.22), 1.0), 0.55)
    if start is not None or end is not None:
        lo = nodes.new("ShaderNodeMapRange")
        links.new(axis, lo.inputs["Value"])
        lo.inputs["From Min"].default_value = (start if start is not None else -9.0)
        lo.inputs["From Max"].default_value = (start if start is not None else -9.0) + fade
        hi = nodes.new("ShaderNodeMapRange")
        links.new(axis, hi.inputs["Value"])
        hi.inputs["From Min"].default_value = (end if end is not None else 9.0)
        hi.inputs["From Max"].default_value = (end if end is not None else 9.0) - fade
        dish = math_node("MULTIPLY", dish, math_node("MULTIPLY", lo.outputs["Result"], hi.outputs["Result"]))
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = strength
    bump.inputs["Distance"].default_value = distance
    links.new(dish, bump.inputs["Height"])
    previous = bsdf.inputs["Normal"].links[0].from_socket if bsdf.inputs["Normal"].links else None
    if previous is not None:
        links.new(previous, bump.inputs["Normal"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


REPORT = {"attach": attach_offsets()}


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_HatchetHaft", light=(0.20, 0.135, 0.075), dark=(0.085, 0.05, 0.026),
                     grain=0.9, roughness=0.62, weathering=0.1, grime=0.55,
                     polish=0.6, polish_center=GRIP_Z, polish_length=0.07)
    stone = mats.flint("M_HatchetHead", cortex=(0.24, 0.2, 0.145), cortex_amount=0.0,
                       cortex_below_x=axe.HEAD_BUTT + 0.022)
    add_scar_bump(stone, scale=17.0, strength=0.4, distance=0.003, start=axe.HEAD_BUTT + 0.03,
                  end=axe.HEAD_BUTT + 0.97 * axe.HEAD_LENGTH, fade=0.05)
    hide = mats.rawhide("M_HatchetLashing", color=(0.155, 0.098, 0.05), strands=2, twist=38.0)
    parts = [build_haft(kit, wood), build_head(kit, stone)]
    parts.append(wrap_band(kit, "WrapLow", hide, axe.HEAD_Z - 0.062, axe.HEAD_Z - 0.030, 6.5, seed=1))
    parts.append(wrap_band(kit, "WrapHigh", hide, axe.HEAD_Z + 0.030, axe.HEAD_Z + 0.060, 6.0, phase=1.3, seed=2))
    for side in (1, -1):
        for rising, offset in ((True, 0.0), (False, 0.003)):
            parts.append(cross_cord(kit, f"Cross{side}{rising}", hide, side, rising, offset))
    for part in parts:
        part.data.transform(TO_EXPORT)
        part.data.update()
    return kit.join(parts, "SM_FlintHatchet", pivot=None, unwrap=False, reshade=True, smooth_angle=70)
