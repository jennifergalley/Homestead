"""Oak fingerposts for the public road (replace-road-sign-teleport): "To the village", "To the manor" and the
two-armed "Village / Manor" at the gateway. They are the cove's fingerpost (fingerpost.py) with the road's
words, and they only point the way: walking up to one no longer teleports her anywhere.

Real-object research: see fingerpost.py (oak post about 12.5 cm square, 2.1 m high, a flat pointed arm
morticed through it, V-cut painted letters on both faces, silvered oak, chalky paint). A two-way post carries
two arms at different heights, each pointing its own way, the upper one the nearer destination.

Pivot at the post's foot on the ground; the arm (the upper arm on the two-armed post) points along +X
(HomesteadRoadSign sets the yaw); the second arm points along -X. Lettering reads on both faces of each arm.
Lettering: Blender's bundled default font (DejaVu Sans, Bitstream Vera licence), converted to mesh. Everything
else is original procedural geometry and materials.
"""
import bpy
from mathutils import Vector

NAME = "FingerpostRoad"
DESCRIPTION = ("Silvered oak fingerposts for the public road: 'To the village', 'To the manor' and a two-armed "
               "'Village / Manor' (original; lettering in Blender's bundled DejaVu Sans). Pivot = post foot; "
               "the (upper) arm points +X, the second arm -X.")
COLLISION = "box"
TRIANGLE_BUDGET = 16000
PROVENANCE = ("Original project-authored procedural geometry and materials. Lettering: Blender's bundled "
              "default font (DejaVu Sans, Bitstream Vera licence, permits embedding and modification).")
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, -20), "focus": (0.35, 0.0, 1.70)}
REPORT = {"pivot": "foot of the post on the ground; the upper arm points along +X", "arm_height_m": 1.70,
          "texts": ["To the village", "To the manor", "Village", "Manor"]}

POST = 0.125
HEIGHT = 2.10
BURY = 0.35
UPPER_Z = 1.70
LOWER_Z = 1.40
ARM_LEN = 1.25
ARM_H = 0.22
ARM_T = 0.03
# Letters about 9 cm tall on a 22 cm board: readable from 4-5 m at 1080p (the first 5 cm letters were not).
TEXT_SIZE = 0.13
TEXT_WEIGHT = 0.006
# The lettering stays inside this much of the arm, from the post's face toward the tip's taper.
TEXT_FIELD = 0.95


def lettering(kit, name, text, face, paint, arm_z, turn):
    """The words on one face of an arm (face +1: the +Y face; -1 the -Y face); turn -1 for the arm along -X."""
    curve = bpy.data.curves.new(name, type='FONT')
    curve.body = text
    curve.size = TEXT_SIZE
    curve.extrude = 0.002
    curve.offset = TEXT_WEIGHT
    curve.align_x = 'CENTER'
    curve.align_y = 'CENTER'
    obj = bpy.data.objects.new(name, curve)
    bpy.context.scene.collection.objects.link(obj)
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    mesh = obj.data
    # Shrink a long phrase to fit the arm.
    width = max(v.co.x for v in mesh.vertices) - min(v.co.x for v in mesh.vertices)
    fit = min(1.0, TEXT_FIELD / width) if width > 0 else 1.0
    centre_x = POST * 0.5 + (ARM_LEN - POST * 0.5) * 0.47
    for v in mesh.vertices:
        x, y, z = v.co * fit
        if face > 0:
            px, py = centre_x - x, face * (ARM_T * 0.5 + 0.0005) + z * face
        else:
            px, py = centre_x + x, face * (ARM_T * 0.5 + 0.0005) + z * face
        v.co = Vector((px * turn, py * turn, arm_z + y))
    mesh.update()
    kit.recalc_normals(obj)
    mesh.materials.clear()
    mesh.materials.append(paint)
    kit.tag_coords(mesh)
    return obj


def arm(kit, name, oak, arm_z, turn):
    """A flat board through the post, its outer end cut to a point, its inner end a short stub."""
    tip = ARM_LEN
    half = ARM_H * 0.5
    rows = []
    for x, h in ((-POST * 0.5 - 0.04, half), (tip - ARM_H * 0.45, half), (tip, 0.004)):
        rows.append([(x * turn, -ARM_T * 0.5 * turn, arm_z - h), (x * turn, ARM_T * 0.5 * turn, arm_z - h),
                     (x * turn, ARM_T * 0.5 * turn, arm_z + h), (x * turn, -ARM_T * 0.5 * turn, arm_z + h)])
    coords = [[(p[1] * turn, p[2] - arm_z, p[0] * turn) for p in row] for row in rows]
    return kit.loft(name, rows, material=oak, coords=coords, cap_start=True, cap_end=True)


def post(kit, oak, height_over=0.0):
    height = HEIGHT + BURY + height_over
    parts = [kit.box("Post", (POST, POST, height), location=(0.0, 0.0, HEIGHT + height_over - height * 0.5),
                     material=oak, bevel=0.01, bevel_segments=2)]
    parts.append(kit.cylinder("Finial", POST * 0.74, 0.07, location=(0.0, 0.0, HEIGHT + height_over + 0.035),
                              rotation=(0, 0, 45), material=oak, sides=4, radius_top=0.01))
    return parts


def build_one(kit, oak, paint, mesh_name, arms):
    """`arms`: (words, arm height, turn) per arm."""
    parts = post(kit, oak)
    for index, (words, arm_z, turn) in enumerate(arms):
        parts.append(arm(kit, f"Arm{index}", oak, arm_z, turn))
        parts.append(lettering(kit, f"Front{index}", words, 1.0, paint, arm_z, turn))
        parts.append(lettering(kit, f"Back{index}", words, -1.0, paint, arm_z, turn))
    return kit.join(parts, mesh_name, pivot=None, unwrap=True, reshade=True, smooth_angle=40)


def build(kit):
    oak = kit.mats.wood("M_FingerpostOak", light=(0.26, 0.20, 0.14), dark=(0.08, 0.06, 0.042), grain=1.1,
                        roughness=0.8, weathering=0.8, grime=0.3, seed=71.0, relief=1.2)
    # Weathered brown-grey oak like the cove's post, with near-black letters: chalk-white vanished against it.
    paint = kit.material("M_FingerpostPaint", (0.025, 0.023, 0.02), roughness=0.9)
    return [
        build_one(kit, oak, paint, "SM_Fingerpost_ToTheVillage", [("To the village", UPPER_Z, 1.0)]),
        build_one(kit, oak, paint, "SM_Fingerpost_ToTheManor", [("To the manor", UPPER_Z, 1.0)]),
        build_one(kit, oak, paint, "SM_Fingerpost_VillageManor",
                  [("Village", UPPER_Z, 1.0), ("Manor", LOWER_Z, -1.0)]),
    ]
