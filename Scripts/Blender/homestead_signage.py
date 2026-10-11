"""The estate's fingerpost template: one definition of the look (post, board, ink, font, letter size, arm shape)
and one builder, so every fingerpost matches and a restyle is a one-file edit (Jenny, 2026-10-11).

Make a new fingerpost: add a recipe under Scripts/Blender/Recipes/ (copy fingerpost_road.py) whose build(kit)
returns one `fingerpost(...)` per mesh:

    from homestead_signage import Arm, fingerpost
    fingerpost(kit, "SM_Fingerpost_ToTheQuarry", [Arm("To the quarry", UPPER_Z)])
    fingerpost(kit, "SM_Fingerpost_Fork", [Arm("Mill", UPPER_Z), Arm("Chapel", LOWER_Z, turn=-1.0)])
    fingerpost(kit, "SM_Fingerpost_ToTheMineArm", [Arm("To the mine", 1.43)], with_post=False)   # an arm to hang on an existing post

then build it with Scripts/Blender/New-Prop.ps1 <recipe> and import it with import_props.py. The arm's length
follows its words, so every sign has the same letter size and board; nothing is sized per sign.

Restyle every fingerpost at once: change the constants below (board and ink colours, post wood, font, LETTER_SIZE,
BOARD_HEIGHT, margins) and rebuild the recipes that use this module (fingerpost, fingerpost_road, and any added).
In the game the arms are placed by HomesteadRoadSign (road and cove posts) and HomesteadWorldMine (the mine arm).

Font: Abril Fatface (TypeTogether, SIL Open Font License 1.1) in Assets/Fonts/AbrilFatface, a Victorian fat face,
converted to mesh and standing proud of each face of the board like thick paint.
"""
from dataclasses import dataclass
from pathlib import Path

import bpy
from mathutils import Vector

REPO = Path(__file__).resolve().parents[2]
FONT_PATH = REPO / "Assets" / "Fonts" / "AbrilFatface" / "AbrilFatface-Regular.ttf"

# Colours (linear albedo). Near-black ink on a pale weathered board: about 1:20, so it reads from a distance.
INK_COLOUR = (0.012, 0.011, 0.010)
BOARD_LIGHT = (0.50, 0.47, 0.40)
BOARD_DARK = (0.33, 0.30, 0.25)
POST_LIGHT = (0.30, 0.28, 0.25)
POST_DARK = (0.10, 0.095, 0.085)

# Dimensions in metres. The post is bedded BURY below the pivot (the ground line); arms are keyed by their centre height.
POST_SIZE = 0.125
POST_HEIGHT = 2.10
POST_BURY = 0.35
BOARD_HEIGHT = 0.21
BOARD_THICKNESS = 0.03
LETTER_SIZE = 0.12          # Blender font size: capitals about 8 cm tall on the 21 cm board
LETTER_EXTRUDE = 0.002      # the paint stands this proud of the board face
LETTER_MARGIN = 0.05        # clear board either side of the words
STUB = 0.04                 # the board's inner end juts this far past the post's far face
TIP_TAPER = 0.45            # the pointed end is this fraction of the board height long
SMOOTH_ANGLE = 40


@dataclass
class Arm:
    """One arm: its words, the height of its centre above the ground, and its direction (+X by default; turn=-1.0 for -X)."""
    words: str
    z: float
    turn: float = 1.0


def board_material(kit, name="M_FingerpostBoard"):
    """Weathered painted board: pale, with a little grain and grime left showing."""
    return kit.mats.wood(name, light=BOARD_LIGHT, dark=BOARD_DARK, grain=1.0, roughness=0.78, weathering=0.35,
                         grime=0.18, seed=71.0, relief=0.8)


def ink_material(kit, name="M_FingerpostPaint"):
    return kit.material(name, INK_COLOUR, roughness=0.85)


def post_material(kit, name="M_FingerpostOak"):
    """Silvered oak with dark checks, for the post."""
    return kit.mats.wood(name, light=POST_LIGHT, dark=POST_DARK, grain=1.1, roughness=0.8, weathering=0.8,
                         grime=0.3, seed=71.0, relief=1.2)


def _text_mesh(name, words):
    """The words as a mesh lying in XY facing +Z, centred on the origin."""
    curve = bpy.data.curves.new(name, type='FONT')
    curve.font = bpy.data.fonts.load(str(FONT_PATH))
    curve.body = words
    curve.size = LETTER_SIZE
    curve.extrude = LETTER_EXTRUDE
    curve.align_x = 'CENTER'
    curve.align_y = 'CENTER'
    obj = bpy.data.objects.new(name, curve)
    bpy.context.scene.collection.objects.link(obj)
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target='MESH')
    return bpy.context.view_layer.objects.active


def text_width(words):
    obj = _text_mesh("Measure", words)
    xs = [v.co.x for v in obj.data.vertices]
    width = max(xs) - min(xs)
    mesh = obj.data
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    return width


def arm_length(words):
    """From the post's axis to the point: the post's half-width, margin, the words, margin, then the taper."""
    return POST_SIZE * 0.5 + LETTER_MARGIN + text_width(words) + LETTER_MARGIN + BOARD_HEIGHT * TIP_TAPER


def board(kit, name, material, arm):
    """A flat board through the post, its outer end cut to a point, its inner end a short stub."""
    tip = arm_length(arm.words)
    half = BOARD_HEIGHT * 0.5
    turn = arm.turn
    t = BOARD_THICKNESS * 0.5
    rows = []
    for x, h in ((-POST_SIZE * 0.5 - STUB, half), (tip - BOARD_HEIGHT * TIP_TAPER, half), (tip, 0.004)):
        rows.append([(x * turn, -t * turn, arm.z - h), (x * turn, t * turn, arm.z - h),
                     (x * turn, t * turn, arm.z + h), (x * turn, -t * turn, arm.z + h)])
    coords = [[(p[1] * turn, p[2] - arm.z, p[0] * turn) for p in row] for row in rows]
    return kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)


def lettering(kit, name, arm, face, ink):
    """The words on one face of an arm (face +1: the +Y face, read from +Y; -1 the -Y face). The export converts
    Blender's frame to Unreal's without mirroring the object, so text that reads true here reads true in the game."""
    obj = _text_mesh(name, arm.words)
    mesh = obj.data
    centre_x = POST_SIZE * 0.5 + LETTER_MARGIN + text_width(arm.words) * 0.5
    for v in mesh.vertices:
        x, y, z = v.co
        px = centre_x - x if face > 0 else centre_x + x
        py = face * (BOARD_THICKNESS * 0.5 + 0.0005) + z * face
        v.co = Vector((px * arm.turn, py * arm.turn, arm.z + y))
    mesh.update()
    kit.recalc_normals(obj)
    mesh.materials.clear()
    mesh.materials.append(ink)
    kit.tag_coords(mesh)
    return obj


def post(kit, oak):
    height = POST_HEIGHT + POST_BURY
    return [kit.box("Post", (POST_SIZE, POST_SIZE, height), location=(0.0, 0.0, POST_HEIGHT - height * 0.5),
                    material=oak, bevel=0.01, bevel_segments=2),
            kit.cylinder("Finial", POST_SIZE * 0.74, 0.07, location=(0.0, 0.0, POST_HEIGHT + 0.035),
                         rotation=(0, 0, 45), material=oak, sides=4, radius_top=0.01)]


def fingerpost(kit, mesh_name, arms, with_post=True):
    """One mesh: the post (unless with_post is False: an arm to hang on an existing post) and each arm with its
    words on both faces. Pivot is the post's foot on the ground."""
    oak, wood, ink = post_material(kit), board_material(kit), ink_material(kit)
    parts = post(kit, oak) if with_post else []
    for index, arm in enumerate(arms):
        parts.append(board(kit, f"Arm{index}", wood, arm))
        parts.append(lettering(kit, f"Front{index}", arm, 1.0, ink))
        parts.append(lettering(kit, f"Back{index}", arm, -1.0, ink))
    return kit.join(parts, mesh_name, pivot=None, unwrap=True, reshade=True, smooth_angle=SMOOTH_ANGLE)
