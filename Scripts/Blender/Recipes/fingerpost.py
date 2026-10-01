"""Oak fingerpost reading "To the Cove" (add-cove-route-kit): one at the manor's front door (yaw 159), one at
the head of the cliff steps (yaw 153).

Real-object research (written before modeling):
- Estate and parish fingerposts of the period were oak: a square post about 5 in (12-13 cm) and 7 ft
  (2.1 m) high with a capped or pyramid finial, and a flat arm 1 in (2.5-3 cm) thick morticed through the
  post near the top, its outer end cut to a point. Letters were incised with a V-cut and painted white so
  they read on both faces of the arm.
- Weathering: silver-grey oak with dark checks, lichen on the cap, the paint in the letters chalky and
  broken, green algae low on the post.

Pivot at the post's foot on the ground; the arm points along +X (Water sets the yaw), 1.70 m up; the
lettering reads on both faces. The post is bedded 0.35 m below the pivot.
Lettering: Blender's bundled default font (DejaVu Sans, Bitstream Vera licence: free to use, modify and
embed), converted to mesh and standing 1.5 mm proud of each face of the arm, like thick paint in the cut letters. Everything else is original
procedural geometry and materials.
"""
import bpy
from mathutils import Vector

NAME = "Fingerpost"
DESCRIPTION = ("Silvered oak fingerpost with a pointed arm reading 'To the Cove' on both faces (original; "
               "lettering in Blender's bundled DejaVu Sans). Pivot = post foot; the arm points +X.")
COLLISION = "box"
TRIANGLE_BUDGET = 12000
PROVENANCE = ("Original project-authored procedural geometry and materials. Lettering: Blender's bundled "
              "default font (DejaVu Sans, Bitstream Vera licence, permits embedding and modification).")
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, -20), "focus": (0.35, 0.0, 1.70)}
REPORT = {"pivot": "foot of the post on the ground; the arm points along +X", "arm_height_m": 1.70,
          "text": "To the Cove", "yaws_deg": {"front door": 159, "head of the cliff steps": 153}}

POST = 0.125
HEIGHT = 2.10
BURY = 0.35
ARM_Z = 1.70
ARM_LEN = 0.62
ARM_H = 0.13
ARM_T = 0.03
TEXT = "To the Cove"
TEXT_SIZE = 0.068


def lettering(kit, name, face, paint):
    """The words as a mesh on one face of the arm (face +1: the +Y face, read from +Y; -1 the -Y face)."""
    curve = bpy.data.curves.new(name, type='FONT')
    curve.body = TEXT
    curve.size = TEXT_SIZE
    curve.extrude = 0.001
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
    # Text lies in XY facing +Z: stand it up on the arm's face, reading left to right as seen from that face. The
    # export converts Blender's right-handed frame to Unreal's without mirroring the object (the knapsack check,
    # 09-30), so text that reads true here reads true in the game.
    centre_x = POST * 0.5 + (ARM_LEN - POST * 0.5) * 0.45
    for v in mesh.vertices:
        x, y, z = v.co
        if face > 0:
            v.co = Vector((centre_x - x, face * (ARM_T * 0.5 + 0.0005) + z * face, ARM_Z + y))
        else:
            v.co = Vector((centre_x + x, face * (ARM_T * 0.5 + 0.0005) + z * face, ARM_Z + y))
    mesh.update()
    kit.recalc_normals(obj)
    mesh.materials.clear()
    mesh.materials.append(paint)
    kit.tag_coords(mesh)
    return obj


def build(kit):
    oak = kit.mats.wood("M_FingerpostOak", light=(0.30, 0.28, 0.25), dark=(0.10, 0.095, 0.085), grain=1.1,
                        roughness=0.8, weathering=0.8, grime=0.3, seed=71.0, relief=1.2)
    paint = kit.material("M_FingerpostPaint", (0.80, 0.79, 0.74), roughness=0.85)
    height = HEIGHT + BURY
    parts = [kit.box("Post", (POST, POST, height), location=(0.0, 0.0, HEIGHT - height * 0.5), material=oak,
                     bevel=0.01, bevel_segments=2)]
    parts.append(kit.cylinder("Finial", POST * 0.74, 0.07, location=(0.0, 0.0, HEIGHT + 0.035), rotation=(0, 0, 45),
                              material=oak, sides=4, radius_top=0.01))
    # The arm: a flat board through the post, its outer end cut to a point, its inner end a short stub.
    tip = ARM_LEN
    half = ARM_H * 0.5
    rows = []
    for x, h in ((-POST * 0.5 - 0.04, half), (tip - ARM_H * 0.45, half), (tip, 0.004)):
        rows.append([(x, -ARM_T * 0.5, ARM_Z - h), (x, ARM_T * 0.5, ARM_Z - h), (x, ARM_T * 0.5, ARM_Z + h),
                     (x, -ARM_T * 0.5, ARM_Z + h)])
    coords = [[(p[1], p[2] - ARM_Z, p[0]) for p in row] for row in rows]
    arm = kit.loft("Arm", rows, material=oak, coords=coords, cap_start=True, cap_end=True)
    parts.append(arm)
    parts.append(lettering(kit, "TextFront", 1.0, paint))
    parts.append(lettering(kit, "TextBack", -1.0, paint))
    return kit.join(parts, "SM_Fingerpost_ToTheCove", pivot=None, unwrap=True, reshade=True, smooth_angle=40)
