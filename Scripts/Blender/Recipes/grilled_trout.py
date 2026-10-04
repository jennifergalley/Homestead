"""Original fire-grilled trout fillet/platter and a separate edible piece.

Usage: New-Prop.ps1 grilled_trout -Live -NoBeauty -OutDirectory <E: scratch>
Held source only; no art/import/grip/bake or real food-safety acceptance.

References read: https://en.wikipedia.org/wiki/Trout (oily food fish) and
https://en.wikipedia.org/wiki/Fish_as_food (cooking preparations). Myomere
reference from the raw-food study informs folded muscle partitions, not a
borrowed fish mesh. Catalogue: RiverTrout1/Kindling1, no oil/butter/herbs/
lemon/batter/garnish. One authored 14cm boneless skin-on fillet, three loose
flakes and a new 22x15.6cm oval platter; portion is a new smaller broken piece.
Dimensions/flake count are presentation, not a stock-weight gameplay rule.
"""
import importlib
import math

import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "GrilledTroutSource"
DESCRIPTION = "Held original fire-browned trout fillet and edible piece."
PROVENANCE = "Original closed cooked-fillet loft, muscle relief, separate flakes, oval platter and food PBR; no caught-fish or meal mesh reused."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 65000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "GrilledTrout",
          "ingredients": {"RiverTrout": 1, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, -9), "meshes": {
    "SM_GrilledTrout": {"focus": (0, -.010, .018), "detail_distance": .33, "detail_fstop": 64},
    "SM_GrilledTroutPortion": {"focus": (0, 0, .007), "detail_distance": .24, "detail_fstop": 64},
}}
SEED = 31913
MYOMERE_SPACING_M = .0095
MYOSEPTUM_DEPTH_M = .00065
FILLET_ROWS, FILLET_SIDES = 72, 80
EDIBLE_AXIAL_LENGTH_M = .0332
PLATTER_PROFILE = ((0, .001), (.047, .001), (.064, .003),
                   (.074, .007), (.078, .011), (.0775, .0145),
                   (.075, .016), (.070, .012), (.060, .008),
                   (.043, .005), (0, .005))


def cooked_fillet(kit, name: str, index: int, length: float, width: float,
                  height: float, rows: int, skin: bool = True) -> bpy.types.Object:
    return shapes.cooked_fillet(
        kit, name, index, length, width, height, rows,
        food.grilled_trout_flesh("M_" + name + "Flesh", index),
        food.grilled_trout_skin("M_" + name + "Skin", index) if skin else None,
        SEED, index == 0, sides=FILLET_SIDES,
        spacing_m=MYOMERE_SPACING_M, depth_m=MYOSEPTUM_DEPTH_M)


def build(kit) -> list:
    platter = shapes.vessel(kit, "OriginalGrilledTroutPlatter", PLATTER_PROFILE,
                            food.grilled_trout_platter("M_GrilledTroutPlatter"))
    for vertex in platter.data.vertices:
        vertex.co.x *= .110 / .078
    kit.tag_coords(platter.data)
    shapes.closed_normals(platter)
    supports, parts = [shapes.world_surface(platter)], [platter]
    fillet = cooked_fillet(kit, "CookedTroutFillet", 0, .140, .050, .013, FILLET_ROWS)
    fillet.rotation_euler.z = math.pi / 2 - .13
    shapes.seat_on_surfaces(fillet, 0, 0, supports, maximum_radius=.098)
    supports.append(shapes.world_surface(fillet))
    parts.append(fillet)
    for index, (x, y, angle) in enumerate(((-.027, .035, .2), (.014, .033, -.3), (.033, -.034, .4))):
        flake = cooked_fillet(kit, "CookedTroutFlake" + str(index), index + 3,
                              .015 + index * .001, .013, .0026, 20, skin=False)
        flake.rotation_euler = (.015, -.015 + .012 * index, math.pi / 2 + angle)
        shapes.seat_on_surfaces(flake, x, y, supports, maximum_radius=.098)
        supports.append(shapes.world_surface(flake))
        parts.append(flake)
    serving = kit.join(parts, "SM_GrilledTrout", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    bite = cooked_fillet(kit, "CookedTroutEdiblePiece", 9, EDIBLE_AXIAL_LENGTH_M, .025, .009, 32)
    portion = kit.join([bite], "SM_GrilledTroutPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "GrilledTrout", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fillets"], serving["food_flake_count"] = 1, 3
    portion["food_grip_not_verified"] = True
    return [serving, portion]
