"""Original pan-crisped laver on a blue shell-edged plate, and one crisp piece to eat.

Usage: New-Prop.ps1 fried_laver -NoPreview -OutDirectory Assets\\Props\\PreparedFood\\FriedLaverSourceProof
then render_beauty.py on the saved blend (see docs\\blender-assets.md, prepared food).
Held source only: no baked material, import, grip or new animation acceptance.

References read: https://en.wikipedia.org/wiki/Porphyra and https://en.wikipedia.org/wiki/Laverbread
(laver blades, thin one-cell membranes that darken to green-black when cooked and crisp when
fried dry). Exact Laver2/Kindling1: laver crisped in the pan until it smells of the sea; no oats,
fat, bacon or salt added. Presentation, not stock weight: ten crumpled, edge-curled 3.5-6cm
sheets and six crumbled flakes heaped on a new 18cm pearlware plate with a moulded, feathered
cobalt shell edge (common English tableware c.1780-1860). Portion: one crisp 4.5cm piece.
No existing game/catch/meal mesh or photo reused. Bottom pivot is not a grip anchor.
"""
import importlib
import math
import random

import homestead_beach_food as beach
import homestead_food_geometry as shapes

beach = importlib.reload(beach)

NAME = "FriedLaverSource"
DESCRIPTION = "Held original crisp fried laver on a shell-edged plate and one piece to eat."
PROVENANCE = "Original crumpled laver sheets, crumbled flakes and scalloped shell-edge plate; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 80000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "FriedLaver",
          "ingredients": {"Laver": 2, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "eating_grip_not_verified": True, "no_new_animation": True,
          "food_geometry_source_sha256": beach.SOURCE_SHA256,
          "food_material_source_sha256": beach.SOURCE_SHA256,
          "shared_food_geometry_sha256": shapes.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 14), "meshes": {
    "SM_FriedLaver": {"focus": (0, -.02, .022), "detail_distance": .30, "detail_fstop": 64},
    "SM_FriedLaverPortion": {"focus": (0, 0, .006), "detail_distance": .22, "detail_fstop": 64},
}}
SEED = 41421
PLATE_SIDES = 288
SCALLOPS = 48
SCALLOP_DEPTH = .011
PLATE_RIM_M = .0905
PLATE_PROFILE = ((0, .0025), (.045, .0025), (.050, .0004), (.054, 0), (.058, .002),
                 (.066, .008), (.074, .014), (.083, .019), (.089, .0215), (.0905, .0228),
                 (.0895, .0240), (.082, .0215), (.072, .0175), (.063, .0105), (.056, .0058),
                 (.040, .0049), (0, .0047))
# (x, y, radius m) for the crumpled sheets, settled in order.
SHEETS = ((-.022, .012, .022), (.020, -.010, .021), (.002, -.030, .019), (-.026, -.022, .018),
          (.024, .024, .020), (-.002, .030, .017), (.034, -.002, .016), (-.036, -.004, .017),
          (-.006, .002, .019), (.010, .010, .016))
FLAKES = ((.050, -.022, .0105), (-.048, .022, .0090), (.034, .044, .0110),
          (-.032, -.048, .0085), (.054, .018, .0095), (-.012, .052, .0088))


def scalloped(plate) -> None:
    """Pinch the marly into the moulded shell edge: shallow cusps between rounded scallops."""
    for vertex in plate.data.vertices:
        radius = vertex.co.xy.length
        if radius < .078:
            continue
        weight = min(1.0, (radius - .078) / .010)
        weight = weight * weight * (3 - 2 * weight)
        angle = math.atan2(vertex.co.y, vertex.co.x)
        cusp = abs(math.sin(angle * SCALLOPS / 2)) ** .5
        vertex.co.x *= 1 - SCALLOP_DEPTH * weight * (1 - cusp)
        vertex.co.y *= 1 - SCALLOP_DEPTH * weight * (1 - cusp)
    shapes.closed_normals(plate)


def build(kit) -> list:
    rng = random.Random(SEED)
    plate, _ = beach.vessel(kit, "OriginalShellEdgePlate", PLATE_PROFILE,
                            beach.shell_edge_pearlware("M_FriedLaverPearlware", SCALLOPS, PLATE_RIM_M),
                            PLATE_SIDES, smoothing=1)
    scalloped(plate)
    parts, supports = [plate], [shapes.world_surface(plate)]
    laver = beach.fried_laver("M_FriedLaverSheets")
    for index, (x, y, radius) in enumerate(SHEETS):
        obj = beach.fried_laver_sheet(kit, "FriedLaverSheet" + str(index), index, laver, radius,
                                      rings=19, sides=64)
        beach.drape(obj, x, y, rng.uniform(0, 2 * math.pi), supports, None, .074,
                    reach=.0030, clearance=.00008, relief=1.0)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    for index, (x, y, radius) in enumerate(FLAKES):
        obj = beach.fried_laver_sheet(kit, "FriedLaverFlake" + str(index), 40 + index, laver, radius,
                                      rings=8, sides=24)
        beach.drape(obj, x, y, rng.uniform(0, 2 * math.pi), supports, None, .074,
                    reach=.0015, clearance=.00008, relief=1.0)
        parts.append(obj)
    serving = kit.join(parts, "SM_FriedLaver", unwrap=False, smooth_angle=80)
    kit.pack_uvs(serving)
    piece = beach.fried_laver_sheet(kit, "FriedLaverPiece", 77, beach.fried_laver("M_FriedLaverPiece"),
                                    .0205, rings=18, sides=64)
    portion = kit.join([piece], "SM_FriedLaverPortion", unwrap=False, smooth_angle=80)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "FriedLaver", role
        obj["food_geometry_source_sha256"] = beach.SOURCE_SHA256
    serving["food_laver_sheets"], serving["food_laver_flakes"] = len(SHEETS), len(FLAKES)
    portion["food_grip_not_verified"] = True
    return [serving, portion]
