"""Original cabbage-and-potato stew and a food-bearing carved maple spoon.

Usage: New-Prop.ps1 cabbage_potato_stew -Live -OutDirectory <E: scratch>
Source WIP, never an accepted grip/animation/import or baked-material receipt.

Author chopped, wilted cabbage lamina/ribs, six soft 2cm potato pieces and
Meadow Herbs in a new 17.6cm earthenware bowl. Exact Cabbage1/Potato1/Flowers1/
Kindling1, with cooking water as presentation; no milk, oil, meat or garnish.
New 18.2cm spoon has a 2.8cm oval hollow head and a 9mm-wide precision-grip
handle. Eating needs item-specific seating on the existing clip; no new clip
or carry/grip pose is authored here. Model -Y points toward the spoon tip
(Blender FBX mirrors Y); the exported pivot is bottom centre, NOT the grip.
No earlier crop/food/fish/spoon mesh or photographic texture is reused.
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

NAME = "CabbagePotatoStewSource"
DESCRIPTION = "Held original cabbage/potato stew and food-bearing maple spoon."
PROVENANCE = "Original closed cooked leaf grids, softened potato cuts, bowl, broth volume and hollow carved spoon; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 50000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "CabbagePotatoStew",
          "ingredients": {"Cabbage": 1, "Potato": 1, "Flowers": 1, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "eating_grip_not_verified": True, "no_new_animation": True,
          "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 12), "meshes": {
    "SM_CabbagePotatoStew": {"focus": (0, -.017, .040), "detail_distance": .33,
                            "detail_fstop": 64},
    "SM_CabbagePotatoStewPortion": {"focus": (0, -.065, .010), "detail_distance": .28,
                                   "detail_fstop": 64},
}}
SEED = 21903
LEAF_ROWS, LEAF_COLUMNS = 28, 20
LEAF_THICKNESS_M = .00030
COOKED_LEAF_FOLD_M = .0014
COOKED_LEAF_BEND_M = .0012
POTATO_FACE_RELIEF_M = .00022
POTATO_GRID = 9
SPOON_SIDES = 64
BROTH_SURFACE_M = .034
BOWL_PROFILE = ((0, .001), (.047, .001), (.056, .005), (.066, .016),
                (.077, .032), (.086, .049), (.088, .055), (.087, .060),
                (.084, .061), (.081, .057), (.074, .041), (.065, .025),
                (.055, .013), (.043, .009), (0, .009))
POTATO_PLACEMENTS = ((-.027, -.028), (.023, -.030), (-.037, .004),
                     (-.028, -.012), (.034, .017), (.028, .030))
LEAF_PLACEMENTS = ((-.038, -.028), (-.005, -.033), (.030, -.025),
                   (-.028, -.007), (.010, -.009), (.038, .002),
                   (-.033, .022), (0, .029), (.023, .030),
                   (-.012, -.014), (.018, .012), (-.016, .014))


def cabbage_leaf(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    rng = random.Random(SEED + index)
    length, width = rng.uniform(.032, .046) * size, rng.uniform(.014, .024) * size
    vertices, faces = [], []
    for layer in range(2):
        for row in range(LEAF_ROWS + 1):
            t = row / LEAF_ROWS
            half_width = width * (.32 + .68 * math.sin(math.pi * t)) * .5
            for column in range(LEAF_COLUMNS + 1):
                u = 2 * column / LEAF_COLUMNS - 1
                scallop = 1 + .06 * math.sin(t * 28 + index) * abs(u) ** 5
                x, y = u * half_width * scallop, length * (t - .5)
                fold = COOKED_LEAF_FOLD_M * size * (1 - math.cos(u * 2.2 + .4 * math.sin(t * 7 + index)))
                bend = COOKED_LEAF_BEND_M * size * math.sin(t * math.pi * 1.35 + index * .7)
                rib = .00045 * size * math.exp(-(u / .18) ** 2)
                branches = sum(.00015 * size * math.exp(
                    -((y - length * position - abs(x) * .72) / (.00065 * size)) ** 2)
                    for position in (-.35, -.18, 0, .18, .35))
                vertices.append(Vector((x, y, fold + bend + branches
                                        + layer * (LEAF_THICKNESS_M + rib))))
    stride, layer_size = LEAF_COLUMNS + 1, (LEAF_COLUMNS + 1) * (LEAF_ROWS + 1)
    for layer in range(2):
        for row in range(LEAF_ROWS):
            for column in range(LEAF_COLUMNS):
                a = layer * layer_size + row * stride + column
                face = (a, a + 1, a + stride + 1, a + stride)
                faces.append(face if layer else tuple(reversed(face)))
    boundary = (list(range(stride)) +
                [row * stride + LEAF_COLUMNS for row in range(1, LEAF_ROWS + 1)] +
                [LEAF_ROWS * stride + column for column in range(LEAF_COLUMNS - 1, -1, -1)] +
                [row * stride for row in range(LEAF_ROWS - 1, 0, -1)])
    for a, b in zip(boundary, boundary[1:] + boundary[:1]):
        faces.append((a, b, b + layer_size, a + layer_size))
    obj = kit.mesh(name, vertices, faces, food.stewed_cabbage("M_" + name, index + 31))
    shapes.closed_normals(obj)
    return obj


def potato_piece(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    vertices, faces, lookup = [], [], {}
    dimensions = (.020 * size, .022 * size, .018 * size)
    for axis in range(3):
        others = [value for value in range(3) if value != axis]
        for sign in (-1, 1):
            grid = []
            for row in range(POTATO_GRID + 1):
                line = []
                for column in range(POTATO_GRID + 1):
                    key = [0, 0, 0]
                    key[axis] = sign * POTATO_GRID
                    key[others[0]], key[others[1]] = 2 * row - POTATO_GRID, 2 * column - POTATO_GRID
                    key = tuple(key)
                    if key not in lookup:
                        point = Vector(tuple(key[i] * dimensions[i] / (2 * POTATO_GRID) for i in range(3)))
                        point *= 1 + .045 * noise.noise(point * 190 + Vector((index, 2.1, 5.3)))
                        face_normal = Vector(tuple(
                            math.copysign(1, value) if abs(value) == POTATO_GRID else 0 for value in key))
                        falloff = math.prod(1 - (abs(key[i]) / POTATO_GRID) ** 6 for i in others)
                        point += face_normal.normalized() * POTATO_FACE_RELIEF_M * size * falloff * noise.noise(
                            point * 950 + Vector((index, 6.1, 1.7)))
                        lookup[key] = len(vertices)
                        vertices.append(point)
                    line.append(lookup[key])
                grid.append(line)
            for first, second in zip(grid, grid[1:]):
                for column in range(POTATO_GRID):
                    faces.append((first[column], first[column + 1], second[column + 1], second[column]))
    obj = kit.mesh(name, vertices, faces, food.stewed_potato("M_" + name, index + 53))
    shapes.closed_normals(obj)
    bevel = obj.modifiers.new("StewedPotatoCookedEdge", "BEVEL")
    bevel.width, bevel.segments, bevel.limit_method, bevel.angle_limit = .0007 * size, 3, "ANGLE", .6
    kit.apply_modifiers(obj)
    shapes.closed_normals(obj)
    return obj


def spoon(kit) -> bpy.types.Object:
    profile = []
    for step in range(41):
        y = -.105 + .055 * step / 40
        relative = (y + .0775) / .0275
        fullness = max(0, 1 - relative * relative)
        width = .0006 + .0134 * math.sqrt(fullness) + .0039 * (step / 40) ** 8
        if step / 40 > .72:
            blend = (step / 40 - .72) / .28
            blend = blend * blend * (3 - 2 * blend)
            shoulder = .0006 + .0134 * math.sqrt(1 - .44 ** 2) + .0039 * .72 ** 8
            width = shoulder * (1 - blend) + .0045 * blend
        profile.append((y, width, .009, .0048 * fullness))
    profile += [(-.047, .0047, .0095, 0), (-.042, .0046, .010, 0)]
    profile += [(y, .0045 + .0003 * math.sin(y * 38), .010, 0)
                for y in (.0, .02, .04, .06, .074, .077)]
    vertices, rows, faces = [], [], []
    for y, width, rim, hollow in profile:
        row = []
        for side in range(SPOON_SIDES):
            theta = side * 2 * math.pi / SPOON_SIDES
            if y <= -.05:
                depth = (hollow * math.sin(theta) ** 2 if math.sin(theta) >= 0 else
                         (hollow + .0015) * (-math.sin(theta)) ** .8)
                z = rim - depth
            else:
                z = rim + .0018 * math.sin(theta)
            row.append(len(vertices))
            vertices.append(Vector((width * math.cos(theta), y, z)))
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(SPOON_SIDES):
            following = (side + 1) % SPOON_SIDES
            faces.append((first[side], second[side], second[following], first[following]))
    faces.extend((tuple(reversed(rows[0])), tuple(rows[-1])))
    obj = kit.mesh("OriginalCabbageStewMapleSpoon", vertices, faces,
                   food.maple_eating_spoon("M_CabbageStewMapleSpoon"))
    shapes.closed_normals(obj)
    return obj


def herb_fragment(kit, name: str, index: int, material) -> bpy.types.Object:
    rng = random.Random(SEED + 1000 + index)
    length = rng.uniform(.0028, .0035)
    outline = ((-.5, -.45), (.2, -.5), (.48, -.2), (.4, .48), (-.3, .38))
    vertices = [Vector((x * .0012, y * length, height))
                for height in (0, .0001) for x, y in outline]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    obj = kit.mesh(name, vertices, faces, material)
    shapes.closed_normals(obj)
    return obj


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl = shapes.vessel(kit, "OriginalCabbageStewBowl", BOWL_PROFILE,
                         food.earthenware("M_CabbageStewEarthenware", 51))
    liquid = shapes.vessel(kit, "OriginalCabbageCookingBroth",
                           ((0, .0091), (.0428, .0091), (.0548, .0131),
                            (.0648, .0251), (.0698, BROTH_SURFACE_M),
                            (.057, BROTH_SURFACE_M), (0, BROTH_SURFACE_M)),
                           food.vegetable_broth("M_CabbageStewBroth"))
    parts, supports = [bowl, liquid], [shapes.world_surface(bowl)]
    for index, (x, y) in enumerate(POTATO_PLACEMENTS):
        obj = potato_piece(kit, "CabbageStewPotato" + str(index), index)
        obj.rotation_euler.z = rng.uniform(-.5, .5)
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.076)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    for index, (x, y) in enumerate(LEAF_PLACEMENTS):
        obj = cabbage_leaf(kit, "CabbageStewLeaf" + str(index), index)
        obj.rotation_euler.z = rng.uniform(-math.pi, math.pi)
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.076)
        fluid_height = BROTH_SURFACE_M - sum(vertex.co.z for vertex in obj.data.vertices) / len(obj.data.vertices)
        obj.location.z = max(obj.location.z, fluid_height)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    herbs = food.chopped_meadow_herb("M_CabbageStewMeadowHerbs")
    for index in range(18):
        x, y = LEAF_PLACEMENTS[index % 12]
        obj = herb_fragment(kit, "CabbageStewHerb" + str(index), index, herbs)
        obj.rotation_euler.z = rng.uniform(-math.pi, math.pi)
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.076)
        parts.append(obj)
    serving = kit.join(parts, "SM_CabbagePotatoStew", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    utensil = spoon(kit)
    supports = [shapes.world_surface(utensil)]
    bite = potato_piece(kit, "CabbageStewSpoonPotato", 7, .55)
    shapes.seat_on_surfaces(bite, 0, -.079, supports, maximum_radius=.12)
    supports.append(shapes.world_surface(bite))
    leaf = cabbage_leaf(kit, "CabbageStewSpoonLeaf", 13, .37)
    shapes.seat_on_surfaces(leaf, 0, -.078, supports, maximum_radius=.12)
    portion = kit.join([utensil, bite, leaf], "SM_CabbagePotatoStewPortion",
                       unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "CabbagePotatoStew", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_potato_pieces"], serving["food_cabbage_pieces"] = 6, 12
    portion["food_eating_utensil"] = "original carved maple spoon"
    portion["food_grip_not_verified"] = True
    return [serving, portion]
