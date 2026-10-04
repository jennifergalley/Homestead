"""Original browned mackerel fillet, new platter and edible portion.

Usage: New-Prop.ps1 grilled_mackerel -Live -NoBeauty -OutDirectory <E: scratch>
Held source only, not art/import/bake/eating or real food-safety acceptance.

References read: https://en.wikipedia.org/wiki/Mackerel_as_food and the earlier
myomere study. Exact SeaMackerel1/Kindling1; no salt/oil/butter/herbs/lemon/
batter/garnish. One authored 13.5cm skin-up boneless fillet, two broken
flesh-up pieces and a new 20.8x15.2cm ceramic platter. Cooked cream tissue,
darker lateral muscle and fine wavy cooked-skin bars. New smaller edible
piece, no catch/raw-food/exported-meal mesh reused. Dimensions/piece count
are presentation, not stock weight or an inventory yield change.
"""
import importlib
import math

import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "GrilledMackerelSource"
DESCRIPTION = "Held original grilled mackerel fillet, broken pieces and edible portion."
PROVENANCE = "Fresh original cooked-muscle lofts, custom ceramic profile and cooked-mackerel food PBR; no catch/raw/exported meal mesh reused."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 65000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "GrilledMackerel",
          "ingredients": {"SeaMackerel": 1, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, -7), "meshes": {
    "SM_GrilledMackerel": {"focus": (0, -.013, .017), "detail_distance": .32, "detail_fstop": 64},
    "SM_GrilledMackerelPortion": {"focus": (0, 0, .005), "detail_distance": .24, "detail_fstop": 64},
}}
SEED = 32339
SKIN_PUCKER_DEPTH_M = .00024
CUT_TISSUE_RELIEF_M = .00023
PLATTER_PROFILE = ((0, .001), (.045, .001), (.062, .003),
                   (.073, .007), (.076, .011), (.0755, .0135),
                   (.0735, .015), (.069, .0115), (.060, .0075),
                   (.041, .0055), (0, .0055))


def heat_pucker(kit, obj: bpy.types.Object, index: int, length_m: float) -> None:
    def relief(co: Vector, pcoord: Vector) -> float:
        weight = min(1.0, max(0.0, (.0001 - pcoord.z) / .0013))
        weight = weight * weight * (3 - 2 * weight)
        skin = weight * SKIN_PUCKER_DEPTH_M * noise.noise(
            co * 450 + Vector((index * 2.1, 8.3, 1.9)))
        return skin
    kit.displace(obj, relief)
    shapes.closed_normals(obj)
    normals = [vertex.normal.copy() for vertex in obj.data.vertices]
    # Keep the softened boundary intact; roughen only the interior cut tissue.
    for vertex, normal in zip(obj.data.vertices, normals):
        cut_distance = abs(vertex.co.y + shapes.cooked_muscle_fold(vertex.co.x))
        if (abs(normal.y) > .88 and abs(normal.z) < .18
                and cut_distance > length_m * .5 - .0003):
            vertex.co += normal * CUT_TISSUE_RELIEF_M * noise.noise(
                vertex.co * 1900 + Vector((index * 3.1, 7.7, 4.9)))
    shapes.closed_normals(obj)
    kit.tag_coords(obj.data)


def build(kit) -> list:
    platter = shapes.vessel(kit, "OriginalGrilledMackerelPlatter", PLATTER_PROFILE,
                            food.grilled_mackerel_platter("M_GrilledMackerelPlatter"))
    for vertex in platter.data.vertices:
        vertex.co.x *= .104 / .076
    kit.tag_coords(platter.data)
    shapes.closed_normals(platter)
    supports, parts = [shapes.world_surface(platter)], [platter]
    main_name = "CookedMackerelFillet"
    fillet = shapes.cooked_fillet(
        kit, main_name, 0, .135, .043, .011, 72,
        food.grilled_mackerel_flesh("M_" + main_name + "Flesh", 0),
        food.grilled_mackerel_skin("M_" + main_name + "Skin", 0),
        SEED, True, depth_m=.00035)
    heat_pucker(kit, fillet, 0, .135)
    fillet.rotation_euler = (math.pi, 0, math.pi / 2 - .10)
    shapes.seat_on_surfaces(fillet, 0, .014, supports, maximum_radius=.094)
    supports.append(shapes.world_surface(fillet))
    parts.append(fillet)
    for index, (x, angle) in enumerate(((-.025, -.3), (.023, .25))):
        name = "CookedMackerelBrokenPiece" + str(index)
        piece = shapes.cooked_fillet(
            kit, name, index + 3, .031, .024, .008, 32,
            food.grilled_mackerel_flesh("M_" + name + "Flesh", index + 3),
            food.grilled_mackerel_skin("M_" + name + "Skin", index + 3),
            SEED, False, depth_m=.00035)
        heat_pucker(kit, piece, index + 3, .031)
        piece.rotation_euler.z = angle
        shapes.seat_on_surfaces(piece, x, -.036, supports, maximum_radius=.094)
        supports.append(shapes.world_surface(piece))
        parts.append(piece)
    serving = kit.join(parts, "SM_GrilledMackerel", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    bite = shapes.cooked_fillet(
        kit, "CookedMackerelEdiblePiece", 9, .0295, .022, .008, 32,
        food.grilled_mackerel_flesh("M_CookedMackerelEdiblePieceFlesh", 9),
        food.grilled_mackerel_skin("M_CookedMackerelEdiblePieceSkin", 9),
        SEED, False, depth_m=.00035)
    heat_pucker(kit, bite, 9, .0295)
    portion = kit.join([bite], "SM_GrilledMackerelPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "GrilledMackerel", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fillets"], serving["food_broken_pieces"] = 1, 2
    portion["food_grip_not_verified"] = True
    return [serving, portion]
