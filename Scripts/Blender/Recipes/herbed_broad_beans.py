"""Original cooked broad beans with Meadow Herbs, bowl and edible bean.

Usage: New-Prop.ps1 herbed_broad_beans -Live -OutDirectory <E: scratch>
Source WIP only; no existing crop, fish, meal mesh or photographic texture.

Observation: https://en.wikipedia.org/wiki/Vicia_faba, broad beans are shelled
then steamed/boiled; young seeds have a flattened, slightly kidney-like outline.
Author 1.9-2.5cm cooked seeds with fine seed-coat wrinkles and a lateral hilum.
The three catalogue crop units are represented by a small serving of 24 seeds,
not a new conversion rule. Exact BroadBeans3/Flowers(Meadow Herbs)1/Kindling1;
no oil, butter, salt or uncatalogued garnish. New 13.4cm earthenware bowl.
"""
import importlib
import math
import random

import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "HerbedBroadBeansSource"
DESCRIPTION = "Held original herbed broad bean serving and edible bean."
PROVENANCE = "Original seed coats/hila, chopped Meadow Herbs and hand-thrown bowl; procedural PBR."
COLLISION = "none"
TRIANGLE_BUDGET = 65000
BAKE = None
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "HerbedBroadBeans",
          "ingredients": {"BroadBeans": 3, "Flowers": 1, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 12), "meshes": {
    "SM_HerbedBroadBeans": {"focus": (0, -.013, .026), "detail_distance": .26,
                           "detail_fstop": 64},
    "SM_HerbedBroadBeansPortion": {"focus": (0, -.004, .006), "detail_distance": .15,
                                  "detail_fstop": 64},
}}
SEED = 6841
BEAN_RINGS = 20
BEAN_SIDES = 64
BEAN_COUNT = 24
HERB_FLAKES = 36
BOWL_INNER_RADIUS_M = .060
BOWL_PROFILE = tuple((radius * .8, height * .8) for radius, height in
               ((0, .001), (.044, .001), (.051, .004), (.063, .010),
                (.075, .023), (.083, .035), (.084, .040), (.082, .043),
                (.079, .042), (.076, .036), (.071, .025), (.060, .015),
                (.050, .008), (0, .008)))
PLACEMENTS = tuple((x, y) for y in (-.037, -.012, .013, .038)
                   for x in (-.036, -.012, .012, .036)) + tuple(
                       (x, y) for y in (-.023, .023) for x in (-.031, -.010, .011, .032))
PLACEMENTS = tuple((x * .78, y * .78) for x, y in PLACEMENTS)


def bean(kit, name: str, index: int) -> bpy.types.Object:
    rng = random.Random(SEED + index)
    width, length, thickness = .0077 * rng.uniform(.92, 1.08), .0117 * rng.uniform(.9, 1.07), .0040
    vertices, rows, faces = [], [], []
    for ring in range(BEAN_RINGS + 1):
        phi = math.pi * ring / BEAN_RINGS
        radius = math.sin(phi)
        row = []
        for side in range(BEAN_SIDES if 0 < ring < BEAN_RINGS else 1):
            theta = side * 2 * math.pi / BEAN_SIDES
            x, y = width * radius * math.cos(theta), length * radius * math.sin(theta)
            x += .0022 * math.exp(-(y / .0045) ** 2) * radius * max(0, -math.cos(theta))
            z = thickness * math.cos(phi)
            z *= 1 - .085 * math.exp(-(x / .0015) ** 2) * radius
            point = Vector((x, y, z))
            point.x += .00022 * math.exp(-((x + .0061) / .0014) ** 2
                                         - ((y - .0006) / .0045) ** 2
                                         - ((z - .0016) / .0022) ** 2)
            point *= 1 + .018 * noise.noise(point * 480 + Vector((index, 2.1, 4.7)))
            row.append(len(vertices))
            vertices.append(point)
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(BEAN_SIDES):
            following = (side + 1) % BEAN_SIDES
            if len(first) == 1:
                faces.append((first[0], second[side], second[following]))
            elif len(second) == 1:
                faces.append((first[side], second[0], first[following]))
            else:
                faces.append((first[side], second[side], second[following], first[following]))
    obj = kit.mesh(name, vertices, faces, food.cooked_broad_bean("M_" + name, index + 19))
    shapes.closed_normals(obj)
    return obj


def herb_flake(kit, name: str, index: int, material) -> bpy.types.Object:
    rng = random.Random(SEED + 100 + index)
    length, width = rng.uniform(.0023, .0043), rng.uniform(.0007, .0014)
    outline = ((-.48, -.46), (.22, -.51), (.49, -.28),
               (.40, .50), (-.12, .42), (-.51, .17))
    vertices = [Vector((x * width, y * length, height + .000055 * math.sin(y * math.pi)))
                for height in (0, .00008) for x, y in outline]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    obj = kit.mesh(name, vertices, faces, material)
    shapes.closed_normals(obj)
    return obj


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl = shapes.vessel(kit, "OriginalBroadBeanBowl", BOWL_PROFILE,
                         food.earthenware("M_OriginalBroadBeanEarthenware", 43))
    parts, supports = [bowl], [shapes.world_surface(bowl)]
    for index, (x, y) in enumerate(PLACEMENTS):
        obj = bean(kit, "OriginalCookedBroadBean" + str(index), index)
        obj.rotation_euler = tuple(math.radians(value) for value in
                                   (rng.uniform(-12, 12), rng.uniform(-12, 12), rng.uniform(-45, 45)))
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=BOWL_INNER_RADIUS_M)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    herb_material = food.chopped_meadow_herb("M_OriginalBroadBeanMeadowHerbs")
    for index in range(HERB_FLAKES):
        x, y = PLACEMENTS[index % BEAN_COUNT]
        obj = herb_flake(kit, "BroadBeanHerbFlake" + str(index), index, herb_material)
        obj.rotation_euler.z = rng.uniform(-math.pi, math.pi)
        shapes.seat_on_surfaces(obj, x + rng.uniform(-.003, .003),
                                y + rng.uniform(-.004, .004), supports,
                                maximum_radius=BOWL_INNER_RADIUS_M)
        parts.append(obj)
    serving = kit.join(parts, "SM_HerbedBroadBeans", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    portion = kit.join([bean(kit, "EdibleHerbedBroadBean", 8)],
                       "SM_HerbedBroadBeansPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    serving["food_bean_count"] = BEAN_COUNT
    serving["food_herb_flakes"] = HERB_FLAKES
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"] = True
        obj["food_source_only"] = True
        obj["food_item"] = "HerbedBroadBeans"
        obj["food_role"] = role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    return [serving, portion]
