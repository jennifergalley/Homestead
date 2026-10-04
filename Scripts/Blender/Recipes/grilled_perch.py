"""Original pair of grilled perch fillets, new platter and edible piece.

Usage: New-Prop.ps1 grilled_perch -Live -NoBeauty -OutDirectory <E: scratch>
Held source only, not art/import/bake/eating or real food-safety acceptance.

References read: https://en.wikipedia.org/wiki/European_perch (species/bands)
and https://en.wikipedia.org/wiki/Fish_as_food. Exact LakePerch1/Kindling1,
no oil/butter/batter/herbs/lemon/garnish. Two authored 10.6-11cm boneless
skin-on fillets from one fish, white cooked muscle and browned barred skin
on a new 19.8x14.4cm oval ceramic platter. Smaller broken edible piece is
new geometry. Shared original muscle-loft construction, not a reused mesh;
no catch/raw-food source or shader is changed. Dimensions are presentation,
not a stock-weight or yield change.
"""
import importlib
import math

import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "GrilledPerchSource"
DESCRIPTION = "Held original perch fillet pair, ceramic platter and edible piece."
PROVENANCE = "Original closed cooked-fillet profiles, fresh lofts/ceramic vessel and food PBR; no catch or exported meal mesh reused."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 65000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "GrilledPerch",
          "ingredients": {"LakePerch": 1, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 9), "meshes": {
    "SM_GrilledPerch": {"focus": (0, -.014, .016), "detail_distance": .30, "detail_fstop": 64},
    "SM_GrilledPerchPortion": {"focus": (0, 0, .005), "detail_distance": .24, "detail_fstop": 64},
}}
SEED = 32117
COOKED_SKIN_RELIEF_M = .00026
COOKED_FLESH_RELIEF_M = .00011
PLATTER_PROFILE = ((0, .001), (.043, .001), (.058, .003),
                   (.069, .007), (.072, .0105), (.0715, .0135),
                   (.0695, .015), (.065, .0115), (.056, .007),
                   (.039, .005), (0, .005))


def cooked_surface_relief(kit, obj: bpy.types.Object, index: int) -> None:
    totals, skin_totals = [0] * len(obj.data.vertices), [0] * len(obj.data.vertices)
    for face in obj.data.polygons:
        for vertex_index in face.vertices:
            totals[vertex_index] += 1
            skin_totals[vertex_index] += int(face.material_index == 1)
    if any(total == 0 for total in totals):
        raise ValueError("Cooked perch loft contains an isolated surface vertex")
    normals = [vertex.normal.copy() for vertex in obj.data.vertices]
    for vertex, normal, total, count in zip(obj.data.vertices, normals, totals, skin_totals):
        weight = count / total
        point = vertex.co * 390 + Vector((index * 2.1, 3.7, 5.9))
        fibres = Vector((vertex.co.x * 1300, (vertex.co.y + abs(vertex.co.x) * .42) * 240,
                         vertex.co.z * 1300)) + Vector((index * 3.1, 8.3, 2.9))
        relief = (weight * COOKED_SKIN_RELIEF_M * noise.noise(point)
                  + (1 - weight) * COOKED_FLESH_RELIEF_M * noise.noise(fibres))
        vertex.co += normal * relief
    shapes.closed_normals(obj)
    kit.tag_coords(obj.data)


def build(kit) -> list:
    platter = shapes.vessel(kit, "OriginalGrilledPerchPlatter", PLATTER_PROFILE,
                            food.grilled_perch_platter("M_GrilledPerchPlatter"))
    for vertex in platter.data.vertices:
        vertex.co.x *= .099 / .072
    kit.tag_coords(platter.data)
    shapes.closed_normals(platter)
    supports, parts = [shapes.world_surface(platter)], [platter]
    for index in range(2):
        name = "CookedPerchFillet" + str(index)
        obj = shapes.cooked_fillet(
            kit, name, index, .110 - index * .004, .040, .0095, 64,
            food.grilled_white_fish_flesh("M_" + name + "Flesh", index),
            food.grilled_perch_skin("M_" + name + "Skin", index),
            SEED, True, depth_m=.00035)
        cooked_surface_relief(kit, obj, index)
        obj.rotation_euler = (math.pi if index else 0, 0, math.pi / 2 + (-.08 if index else .07))
        shapes.seat_on_surfaces(obj, -.004 if index else .005, .023 if index else -.023,
                                supports, maximum_radius=.089)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_GrilledPerch", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    bite = shapes.cooked_fillet(
        kit, "CookedPerchEdiblePiece", 9, .0298, .022, .0075, 32,
        food.grilled_white_fish_flesh("M_CookedPerchEdiblePieceFlesh", 9),
        food.grilled_perch_skin("M_CookedPerchEdiblePieceSkin", 9),
        SEED, False, depth_m=.00035)
    cooked_surface_relief(kit, bite, 9)
    portion = kit.join([bite], "SM_GrilledPerchPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "GrilledPerch", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fillets"] = 2
    portion["food_grip_not_verified"] = True
    return [serving, portion]
