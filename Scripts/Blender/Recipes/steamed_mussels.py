"""Original steamed blue mussels in their liquor, and one open mussel to eat.

Usage: New-Prop.ps1 steamed_mussels -NoPreview -OutDirectory Assets\\Props\\PreparedFood\\SteamedMusselsSourceProof
then render_beauty.py on the saved blend (see docs\\blender-assets.md, prepared food).
Held source only: no baked material, import, grip or new animation acceptance.

References read: https://en.wikipedia.org/wiki/Blue_mussel and
https://en.wikipedia.org/wiki/Mussel (anatomy, shell, cooking). Exact Mussels3/Kindling1:
mussels steamed open over the fire in their own juices, nothing else added. The serving
is presentation, not stock weight: nine 5.3-6.4cm Mytilus edulis, every one opened 95-175
degrees about its dorsal hinge, two valves of 0.9mm shell, orange (and two cream, male)
meat moulded into the lower valve with a dark frilled mantle. They lie heaped in a new
19.7cm cream-slip, honey-glazed earthenware bowl dipped in treacle glaze outside over a
bare red foot, in a 7mm pool of cloudy liquor. Portion: one open 6cm mussel, no utensil.
No existing game/catch/meal mesh or photo reused. Bottom pivot is not a grip anchor.
"""
import importlib
import math
import random

import homestead_beach_food as beach
import homestead_food_geometry as shapes
from mathutils import Matrix, Vector

beach = importlib.reload(beach)

NAME = "SteamedMusselsSource"
DESCRIPTION = "Held original steamed mussels in liquor and one open mussel to eat."
PROVENANCE = "Original solidified mussel valves, valve-moulded meats, slipware bowl and liquor; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 80000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "SteamedMussels",
          "ingredients": {"Mussels": 3, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "eating_grip_not_verified": True, "no_new_animation": True,
          "food_geometry_source_sha256": beach.SOURCE_SHA256,
          "food_material_source_sha256": beach.SOURCE_SHA256,
          "shared_food_geometry_sha256": shapes.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 14), "meshes": {
    "SM_SteamedMussels": {"focus": (-.012, -.040, .040), "detail_distance": .34, "detail_fstop": 64},
    "SM_SteamedMusselsPortion": {"focus": (0, -.008, .012), "detail_distance": .26, "detail_fstop": 64},
}}
SEED = 41207
BOWL_SIDES = 144
LIQUOR_LEVEL_M = .0170
BOWL_PROFILE = ((0, .0030), (.040, .0030), (.046, .0006), (.050, 0), (.054, .003),
                (.066, .010), (.080, .022), (.090, .035), (.095, .045), (.098, .052),
                (.0985, .0545), (.097, .0556), (.0950, .0541), (.091, .046), (.083, .034),
                (.070, .020), (.055, .011), (.040, .0075), (0, .0068))
# (x, y, gape degrees, length m, pale meat); settled in this order, outer ring first.
MUSSELS = (
    (-.040, -.022, 140, .061, False), (.004, -.046, 115, .058, False),
    (.044, -.018, 160, .064, False), (.040, .030, 105, .057, True),
    (-.004, .046, 130, .060, False), (-.044, .020, 150, .055, False),
    (-.010, -.010, 100, .062, False), (.016, .006, 145, .056, True),
    (-.012, .016, 120, .053, False),
)


def centred_mussel(kit, name: str, seed: int, materials: dict, length: float, gape: float):
    obj = beach.open_mussel(kit, name, seed, materials, length, gape)
    xs = [vertex.co.x for vertex in obj.data.vertices]
    ys = [vertex.co.y for vertex in obj.data.vertices]
    obj.data.transform(Matrix.Translation((-(min(xs) + max(xs)) / 2, -(min(ys) + max(ys)) / 2, 0)))
    beach._offset_coords(obj, Vector(((seed * .0731) % 1, (seed * .0377) % 1, (seed * .0519) % 1)))
    return obj


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl, interior = beach.vessel(kit, "OriginalSteamedMusselsBowl", BOWL_PROFILE,
                                  beach.slipware_bowl("M_SteamedMusselsSlipware"),
                                  BOWL_SIDES, smoothing=1)
    liquor = shapes.vessel(kit, "OriginalSteamedMusselsLiquor",
                           beach.fill_profile(interior, LIQUOR_LEVEL_M),
                           beach.mussel_liquor("M_SteamedMusselsLiquor"), BOWL_SIDES)
    shell, nacre = beach.mussel_shell("M_SteamedMusselShell"), beach.mussel_nacre("M_SteamedMusselNacre")
    meats = {False: beach.steamed_mussel_meat("M_SteamedMusselMeat"),
             True: beach.steamed_mussel_meat("M_SteamedMusselMeatPale", pale=True)}
    parts, supports = [bowl, liquor], [shapes.world_surface(bowl)]
    for index, (x, y, gape, length, pale) in enumerate(MUSSELS):
        materials = {"shell": shell, "nacre": nacre, "meat": meats[pale]}
        obj = centred_mussel(kit, "SteamedMussel" + str(index), SEED + index * 13, materials, length, gape)
        beach.settle(obj, x, y, supports, rng, maximum_radius=.092)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_SteamedMussels", unwrap=False, smooth_angle=80)
    kit.pack_uvs(serving)
    single = centred_mussel(kit, "SteamedMusselToEat", SEED + 997,
                            {"shell": beach.mussel_shell("M_SteamedMusselPortionShell"),
                             "nacre": beach.mussel_nacre("M_SteamedMusselPortionNacre"),
                             "meat": beach.steamed_mussel_meat("M_SteamedMusselPortionMeat")},
                            .060, 168)
    portion = kit.join([single], "SM_SteamedMusselsPortion", unwrap=False, smooth_angle=80)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "SteamedMussels", role
        obj["food_geometry_source_sha256"] = beach.SOURCE_SHA256
    serving["food_mussels"] = len(MUSSELS)
    portion["food_mussels"] = 1
    portion["food_grip_not_verified"] = True
    return [serving, portion]
