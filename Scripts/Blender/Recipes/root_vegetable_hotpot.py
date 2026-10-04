"""Original wild-root/turnip hotpot, handled crock and eating spoon.

Usage: New-Prop.ps1 root_vegetable_hotpot -Live -NoBeauty -OutDirectory <E: scratch>
Held source only: no baked material, import, grip or new animation acceptance.

References read: https://en.wikipedia.org/wiki/Root_vegetable and
https://en.wikipedia.org/wiki/Turnip. Generic fibrous brown-skinned wild taproot
cuts do not assert a species absent from the catalogue. Author ten 1.2-1.9cm
root segments, six soft white turnip sectors and twenty-four Meadow Herb flecks
in a new 16.6cm, two-lug crock with cooking broth. Exact Roots2/Turnip1/Flowers1/
Kindling1, no potato/carrot/meat/oil/milk. Counts are presentation, not stock
weight. New 17cm generated maple spoon carries root and turnip with seasoning.
No existing game/crop/meal mesh or photo reused. Source -Y spoon tip mirrors
in FBX; bottom pivot is not an approved grip anchor.
"""
import importlib
import math
import random

import bmesh
import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "RootVegetableHotpotSource"
DESCRIPTION = "Held original root-and-turnip hotpot and food-bearing maple spoon."
PROVENANCE = "Original root/quarter-sector turnip lofts, closed herb patches, generated crock/lugs/broth/spoon; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 80000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "RootVegetableHotpot",
          "ingredients": {"Roots": 2, "Turnip": 1, "Flowers": 1, "Kindling": 1},
          "original_food": True, "not_import_or_art_acceptance": True,
          "eating_grip_not_verified": True, "no_new_animation": True,
          "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 14), "meshes": {
    "SM_RootVegetableHotpot": {"focus": (0, -.02, .036), "detail_distance": .34, "detail_fstop": 64},
    "SM_RootVegetableHotpotPortion": {"focus": (0, -.064, .011), "detail_distance": .27, "detail_fstop": 64},
}}
SEED = 21906
ROOT_ROWS, ROOT_COLUMNS = 20, 40
TURNIP_ROWS, TURNIP_COLUMNS, CUT_COLUMNS = 22, 24, 8
COOKED_EDGE_RADIUS_M = .00080
SPOON_LENGTH_M = .170
BROTH_SURFACE_M = .032
FOOD_MEAN_HEIGHT_M = .027
CROCK_PROFILE = ((0, .001), (.043, .001), (.054, .006), (.066, .018),
                 (.075, .033), (.080, .047), (.082, .049), (.083, .052),
                 (.082, .056), (.079, .058), (.077, .056), (.076, .049),
                 (.071, .037), (.063, .024), (.052, .014), (.041, .008), (0, .008))
ROOT_PLACEMENTS = ((-.041, -.030), (-.013, -.042), (.015, -.038), (.040, -.023),
                   (-.043, -.006), (-.013, -.014), (.025, .002), (-.029, .029),
                   (-.001, .036), (.029, .029))
TURNIP_PLACEMENTS = ((-.024, -.028), (.012, -.018), (.043, .006),
                     (-.031, .009), (.001, .015), (.026, .037))


def round_cut_edges(kit, obj, size: float) -> None:
    shapes.closed_normals(obj)
    bevel = obj.modifiers.new("HotpotSoftCookedCutEdge", "BEVEL")
    bevel.width, bevel.segments = COOKED_EDGE_RADIUS_M * size, 3
    bevel.limit_method, bevel.angle_limit = "ANGLE", .60
    kit.apply_modifiers(obj)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        bmesh.ops.dissolve_degenerate(mesh, dist=1e-8, edges=list(mesh.edges))
        mesh.to_mesh(obj.data)
    finally:
        mesh.free()
    shapes.closed_normals(obj)


def root_segment(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    length = (.0155 + .003 * math.sin(index * 1.7)) * size
    radius = (.0068 + .001 * math.cos(index * 2.3)) * size
    vertices, rows, faces = [], [], []
    for row in range(ROOT_ROWS + 1):
        t, ring = row / ROOT_ROWS, []
        for column in range(ROOT_COLUMNS):
            theta = column * 2 * math.pi / ROOT_COLUMNS
            reach = radius * (1 + .035 * math.sin(theta * 3 + index)
                              + .035 * math.sin(t * math.pi + index * .3))
            reach *= 1 + .014 * math.sin(theta * 14 + index + t * .8)
            point = Vector((reach * math.cos(theta) + .0006 * size * math.sin(t * math.pi),
                            (t - .5) * length, reach * math.sin(theta) * .91))
            point.x += .00010 * size * noise.noise(point * 620 + Vector((index, 6, 2)))
            ring.append(len(vertices))
            vertices.append(point)
        rows.append(ring)
    for first, second in zip(rows, rows[1:]):
        for column in range(ROOT_COLUMNS):
            following = (column + 1) % ROOT_COLUMNS
            faces.append((first[column], first[following], second[following], second[column]))
    skin_count = len(faces)
    for cap, boundary in enumerate((rows[0], rows[-1])):
        faces += shapes.cut_food_patch(vertices, boundary, SEED + index * 2 + cap, normal_axis=1)
    obj = kit.mesh(name, vertices, faces, food.stewed_root("M_" + name + "Skin", index, False))
    obj.data.materials.append(food.stewed_root("M_" + name + "Flesh", index, True))
    for face in obj.data.polygons[skin_count:]:
        face.material_index = 1
    round_cut_edges(kit, obj, size)
    return obj


def turnip_sector(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    length = (.022 + .0017 * math.sin(index)) * size
    radius = .016 * size
    vertices, rows, axes, faces = [], [], [], []
    for row in range(TURNIP_ROWS + 1):
        t, ring = row / TURNIP_ROWS, []
        y = (t - .5) * length
        axes.append(len(vertices))
        vertices.append(Vector((0, y, 0)))
        for column in range(TURNIP_COLUMNS + 1):
            theta = math.pi * .5 * column / TURNIP_COLUMNS
            reach = radius * math.sqrt(1 - 1.5 * (t - .5) ** 2) * (1 + .045 * math.sin(t * 4 + index)
                              + .022 * math.sin(theta * 3 + index))
            ring.append(len(vertices))
            vertices.append(Vector((reach * math.cos(theta), y, reach * math.sin(theta) * .91)))
        rows.append(ring)
    for first, second in zip(rows, rows[1:]):
        for column in range(TURNIP_COLUMNS):
            faces.append((first[column], first[column + 1], second[column + 1], second[column]))
    cut_grids = {}
    for column in (0, TURNIP_COLUMNS):
        grid = []
        for row, axis in zip(rows, axes):
            line = [axis]
            for step in range(1, CUT_COLUMNS):
                fraction = step / CUT_COLUMNS
                point = vertices[axis].lerp(vertices[row[column]], fraction)
                relief = .00015 * size * math.sin(math.pi * fraction) * noise.noise(
                    point * 980 + Vector((index, 1.7, 5.1)))
                point.z += relief if column == 0 else 0
                point.x += relief if column == TURNIP_COLUMNS else 0
                line.append(len(vertices))
                vertices.append(point)
            line.append(row[column])
            grid.append(line)
        for first, second in zip(grid, grid[1:]):
            for step in range(CUT_COLUMNS):
                faces.append((first[step], first[step + 1], second[step + 1], second[step]))
        cut_grids[column] = grid
    for cap, (row, axis) in enumerate(((rows[0], axes[0]), (rows[-1], axes[-1]))):
        # Cap boundaries include the subdivided radial cuts, not just their corners.
        end = 0 if cap == 0 else -1
        boundary = row + list(reversed(cut_grids[TURNIP_COLUMNS][end][1:-1])) + [axis]
        boundary += cut_grids[0][end][1:-1]
        faces += shapes.cut_food_patch(vertices, boundary, SEED + 100 + index * 2 + cap, normal_axis=1)
    for point in vertices:
        point.x -= radius * .45
        point.z -= radius * .42
    obj = kit.mesh(name, vertices, faces, food.stewed_root("M_" + name, index + 51, True, True))
    round_cut_edges(kit, obj, size)
    return obj


def herb_patch(kit, name: str, index: int, material) -> bpy.types.Object:
    rng = random.Random(SEED + 1000 + index)
    length, width = rng.uniform(.0026, .004), rng.uniform(.0011, .0017)
    outline = ((-.45, -.48), (.19, -.50), (.5, -.15), (.43, .38), (-.19, .49), (-.5, .12))
    vertices = [Vector((x * width, y * length, .00025 * math.sin(y * 3) + layer * .00010))
                for layer in range(2) for x, y in outline]
    count = len(outline)
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    obj = kit.mesh(name, vertices, faces, material)
    shapes.closed_normals(obj)
    return obj


def place_in_broth(obj, x: float, y: float, supports: list) -> None:
    shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.070)
    rotation = obj.rotation_euler.to_matrix()
    mean = sum((rotation @ vertex.co).z for vertex in obj.data.vertices) / len(obj.data.vertices)
    obj.location.z = max(obj.location.z, FOOD_MEAN_HEIGHT_M - mean)
    supports.append(shapes.world_surface(obj))


def build(kit) -> list:
    rng = random.Random(SEED)
    clay = food.earthenware("M_RootHotpotCrock", 61)
    crock = shapes.vessel(kit, "OriginalRootHotpotCrock", CROCK_PROFILE, clay)
    parts, supports = [crock], [shapes.world_surface(crock)]
    for sign in (-1, 1):
        points = [(sign * (.075 + .019 * math.sin(math.pi * step / 32)),
                   .016 * math.cos(math.pi * step / 32), .044 + .001 * math.sin(math.pi * step / 32))
                  for step in range(33)]
        lug = kit.tube("OriginalRootHotpotCrockLug" + str(sign), points, radius=.0028, sides=24, material=clay)
        shapes.closed_normals(lug)
        parts.append(lug)
    liquid = shapes.vessel(kit, "OriginalRootHotpotBroth",
                          ((0, .0081), (.0408, .0081), (.0518, .0141),
                           (.0628, .0241), (.0673, BROTH_SURFACE_M),
                           (.04, BROTH_SURFACE_M), (0, BROTH_SURFACE_M)),
                          food.vegetable_broth("M_RootHotpotBroth"))
    parts.append(liquid)
    for index, (x, y) in enumerate(ROOT_PLACEMENTS):
        obj = root_segment(kit, "RootHotpotWildRoot" + str(index), index)
        obj.rotation_euler = (rng.uniform(-.55, .55), rng.uniform(-.24, .24),
                              rng.uniform(-math.pi, math.pi))
        place_in_broth(obj, x, y, supports)
        parts.append(obj)
    for index, (x, y) in enumerate(TURNIP_PLACEMENTS):
        obj = turnip_sector(kit, "RootHotpotTurnip" + str(index), index)
        obj.rotation_euler = (rng.uniform(-.3, .3), rng.uniform(-.3, .3),
                              rng.uniform(-math.pi, math.pi))
        place_in_broth(obj, x, y, supports)
        parts.append(obj)
    herbs = food.chopped_meadow_herb("M_RootHotpotMeadowHerbs")
    for index in range(24):
        x, y = TURNIP_PLACEMENTS[index % 6] if index % 2 else ROOT_PLACEMENTS[index % 10]
        obj = herb_patch(kit, "RootHotpotHerb" + str(index), index, herbs)
        obj.rotation_euler.z = rng.uniform(-math.pi, math.pi)
        shapes.seat_on_surfaces(obj, x + rng.uniform(-.006, .006), y + rng.uniform(-.006, .006),
                               supports, maximum_radius=.070)
        parts.append(obj)
    serving = kit.join(parts, "SM_RootVegetableHotpot", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    utensil = shapes.carved_spoon(kit, "OriginalRootHotpotMapleSpoon",
                                  food.maple_eating_spoon("M_RootHotpotMapleSpoon"), SPOON_LENGTH_M)
    supports = [shapes.world_surface(utensil)]
    root = root_segment(kit, "RootHotpotSpoonWildRoot", 11, .72)
    turnip = turnip_sector(kit, "RootHotpotSpoonTurnip", 7, .62)
    portion_parts = [utensil]
    for obj, x in ((root, -.006), (turnip, .0055)):
        shapes.seat_on_surfaces(obj, x, -.0775 * SPOON_LENGTH_M / .182,
                               supports, maximum_radius=.12)
        supports.append(shapes.world_surface(obj))
        portion_parts.append(obj)
    for index, x in enumerate((-.003, .004)):
        obj = herb_patch(kit, "RootHotpotSpoonHerb" + str(index), 31 + index, herbs)
        shapes.seat_on_surfaces(obj, x, -.0775 * SPOON_LENGTH_M / .182,
                               supports, maximum_radius=.12)
        portion_parts.append(obj)
    portion = kit.join(portion_parts, "SM_RootVegetableHotpotPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "RootVegetableHotpot", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_root_pieces"], serving["food_turnip_pieces"] = 10, 6
    serving["food_herb_fragments"] = 24
    portion["food_eating_utensil"] = "original generated 17cm carved maple spoon"
    portion["food_grip_not_verified"] = True
    return [serving, portion]
