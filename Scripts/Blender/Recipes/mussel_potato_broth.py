"""Original mussel, potato and laver broth in a banded bowl, and a spoonful to eat.

Usage: New-Prop.ps1 mussel_potato_broth -NoPreview -OutDirectory Assets\\Props\\PreparedFood\\MusselPotatoBrothSourceProof
then render_beauty.py on the saved blend (see docs\\blender-assets.md, prepared food).
Held source only: no baked material, import, grip or new animation acceptance.

References read: https://en.wikipedia.org/wiki/Blue_mussel, https://en.wikipedia.org/wiki/Laverbread
and https://en.wikipedia.org/wiki/Porphyra (laver blades, cooking colour). Exact Mussels3/Potato1/
Laver1/Kindling1 simmered into a briny broth; no milk, cream, butter, onion or herbs. Presentation,
not stock weight: a new 16cm, 6.2cm-deep creamware bowl with an ochre mocha band and brown slip lines holds cloudy,
potato-starched broth flecked with laver; six soft potato chunks, three open mussels still in their
blue-black shells, three shelled orange meats and six slick olive-black laver ribbons float at the
surface. Portion: the generated 17cm maple eating spoon carrying broth, a meat, a potato morsel and a
laver strip (export keeps only the food). No existing game/catch/meal mesh or photo reused.
"""
import importlib
import math
import random

import homestead_beach_food as beach
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Matrix, Vector

beach = importlib.reload(beach)
food = importlib.reload(food)

NAME = "MusselPotatoBrothSource"
DESCRIPTION = "Held original mussel, potato and laver broth and a food-bearing maple spoon."
PROVENANCE = "Original creamware bowl, broth volume, potato chunks, solidified mussels, laver blades and spoon lens; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 80000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "MusselPotatoBroth",
          "ingredients": {"Mussels": 3, "Potato": 1, "Laver": 1, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "eating_grip_not_verified": True, "no_new_animation": True,
          "food_geometry_source_sha256": beach.SOURCE_SHA256,
          "food_material_source_sha256": beach.SOURCE_SHA256,
          "shared_food_geometry_sha256": shapes.SOURCE_SHA256,
          "shared_food_material_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 14), "meshes": {
    "SM_MusselPotatoBroth": {"focus": (0, -.018, .048), "detail_distance": .32, "detail_fstop": 64},
    "SM_MusselPotatoBrothPortion": {"focus": (0, -.068, .010), "detail_distance": .25, "detail_fstop": 64},
}}
SEED = 41311
BOWL_SIDES = 144
BROTH_LEVEL_M = .0465
FLOAT_RADIUS_M = .061
SPOON_LENGTH_M = .170
BOWL_PROFILE = ((0, .0030), (.030, .0030), (.034, .0005), (.038, 0), (.041, .003),
                (.050, .010), (.060, .020), (.069, .033), (.075, .045), (.0785, .055),
                (.0798, .0605), (.0794, .0622), (.0780, .0621), (.0766, .0598),
                (.0745, .054), (.0705, .044), (.064, .032), (.055, .021), (.045, .0125),
                (.035, .0080), (.020, .0070), (0, .0068))
POTATOES = ((.046, -.016), (-.046, -.024), (-.002, -.048), (.040, .032), (-.040, .030), (.004, .048))
MUSSELS = ((-.016, -.014, 172, .050), (.018, .008, 168, .048), (-.020, .026, 176, .046))
MEATS = ((.026, -.040), (-.024, -.044), (-.052, .002))
LAVER = ((-.004, -.018, .046, .6), (.030, .026, .040, 2.1), (-.036, -.034, .036, 1.2),
         (.044, -.030, .032, -.4), (-.022, .044, .040, .2), (-.046, .010, .034, 1.6))

def float_on(obj, x: float, y: float, sink: float, others: list, bowl_surface, rng, heading=None,
             max_tilt: float = .35) -> None:
    supports = [bowl_surface, beach.float_plane(BROTH_LEVEL_M - sink, FLOAT_RADIUS_M + .004)] + others
    beach.settle(obj, x, y, supports, rng, FLOAT_RADIUS_M, tries=36, jitter=.006,
                 max_tilt=max_tilt, heading=heading)


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl, interior = beach.vessel(kit, "OriginalMusselBrothBowl", BOWL_PROFILE,
                                  beach.banded_creamware("M_MusselBrothCreamware"), BOWL_SIDES, smoothing=1)
    broth_material = beach.mussel_broth("M_MusselBrothLiquid")
    broth = shapes.vessel(kit, "OriginalMusselBroth", beach.fill_profile(interior, BROTH_LEVEL_M),
                          broth_material, BOWL_SIDES)
    bowl_surface = shapes.world_surface(bowl)
    parts, others = [bowl, broth], []
    laver_material = beach.cooked_laver("M_MusselBrothLaver")
    for index, (x, y, length, heading) in enumerate(LAVER):
        obj = beach.laver_ribbon(kit, "MusselBrothLaver" + str(index), index, laver_material,
                                 length, .017 + .005 * math.sin(index * 2.1))
        beach.drape(obj, x, y, heading, [bowl_surface], BROTH_LEVEL_M + .0001,
                    FLOAT_RADIUS_M + .006, reach=.0015, relief=.70)
        others.append(shapes.world_surface(obj))
        parts.append(obj)
    potato_material = beach.boiled_potato("M_MusselBrothPotato")
    for index, (x, y) in enumerate(POTATOES):
        obj = beach.potato_chunk(kit, "MusselBrothPotato" + str(index), index, potato_material, 1.0)
        float_on(obj, x, y, .0070, others, bowl_surface, rng, max_tilt=.6)
        others.append(shapes.world_surface(obj))
        parts.append(obj)
    meat_material = beach.steamed_mussel_meat("M_MusselBrothMeat")
    for index, (x, y) in enumerate(MEATS):
        obj = beach.loose_meat(kit, "MusselBrothMeat" + str(index), SEED + 301 + index * 5,
                               meat_material, .046 + .003 * index)
        float_on(obj, x, y, .0040, others, bowl_surface, rng, max_tilt=.3)
        others.append(shapes.world_surface(obj))
        parts.append(obj)
    shell, nacre = beach.mussel_shell("M_MusselBrothShell"), beach.mussel_nacre("M_MusselBrothNacre")
    for index, (x, y, gape, length) in enumerate(MUSSELS):
        seed = SEED + 101 + index * 7
        obj = beach.open_mussel(kit, "MusselBrothMussel" + str(index), seed,
                                {"shell": shell, "nacre": nacre, "meat": meat_material}, length, gape)
        xs = [vertex.co.x for vertex in obj.data.vertices]
        ys = [vertex.co.y for vertex in obj.data.vertices]
        obj.data.transform(Matrix.Translation((-(min(xs) + max(xs)) / 2, -(min(ys) + max(ys)) / 2, 0)))
        beach._offset_coords(obj, Vector(((seed * .0731) % 1, (seed * .0377) % 1, (seed * .0519) % 1)))
        float_on(obj, x, y, .0085, others, bowl_surface, rng, max_tilt=.12)
        others.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_MusselPotatoBroth", unwrap=False, smooth_angle=80)
    kit.pack_uvs(serving)

    utensil = shapes.carved_spoon(kit, "OriginalMusselBrothMapleSpoon",
                                  food.maple_eating_spoon("M_MusselBrothMapleSpoon"), SPOON_LENGTH_M)
    lens, _ = beach.broth_lens(kit, "MusselBrothSpoonful", utensil, beach.mussel_broth("M_MusselBrothSpoonful", thin=True))
    supports = [shapes.world_surface(utensil)]
    centre_y = -.0775 * SPOON_LENGTH_M / .182
    meat = beach.loose_meat(kit, "MusselBrothSpoonMeat", SEED + 777, beach.steamed_mussel_meat("M_MusselBrothSpoonMeat"), .038)
    meat.rotation_euler.z = math.pi / 2 + .12
    shapes.seat_on_surfaces(meat, -.0012, centre_y - .0045, supports, maximum_radius=.12)
    supports.append(shapes.world_surface(meat))
    morsel = beach.potato_chunk(kit, "MusselBrothSpoonPotato", 11, beach.boiled_potato("M_MusselBrothSpoonPotato"), .48)
    morsel.rotation_euler.z = .4
    shapes.seat_on_surfaces(morsel, .0035, centre_y + .0135, supports, maximum_radius=.12)
    supports.append(shapes.world_surface(morsel))
    strip = beach.laver_ribbon(kit, "MusselBrothSpoonLaver", 21, beach.cooked_laver("M_MusselBrothSpoonLaver"),
                               .024, .0075, rows=22, columns=5)
    beach.drape(strip, .0005, centre_y + .003, .5, supports + [shapes.world_surface(lens)], None,
                .12, reach=.0012, relief=.5)
    portion = kit.join([utensil, lens, meat, morsel, strip], "SM_MusselPotatoBrothPortion",
                       unwrap=False, smooth_angle=80)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "MusselPotatoBroth", role
        obj["food_geometry_source_sha256"] = beach.SOURCE_SHA256
    serving["food_potato_pieces"], serving["food_mussels_in_shell"] = len(POTATOES), len(MUSSELS)
    serving["food_loose_meats"], serving["food_laver_ribbons"] = len(MEATS), len(LAVER)
    portion["food_eating_utensil"] = "original generated 17cm carved maple spoon"
    portion["food_grip_not_verified"] = True
    return [serving, portion]
