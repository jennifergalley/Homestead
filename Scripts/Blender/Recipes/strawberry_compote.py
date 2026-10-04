"""Original unsweetened strawberry reduction, ceramic dish and eating spoon.

Usage: New-Prop.ps1 strawberry_compote -Live -NoBeauty -OutDirectory <E: scratch>
Source WIP only; no baked/imported/hand-seated or action-animation acceptance.

References read: https://en.wikipedia.org/wiki/Strawberry (fleshy receptacle/
external achenes) and https://en.wikipedia.org/wiki/Compote (unsweetened puree).
Authored eight softened stemless halves of four 2.4-3.2cm fruits, external
achenes and bounded juice in a new 13.2cm dish. Exact Strawberries2/Kindling1:
no sugar, cream, spice or leaf garnish. Count is presentation, not stock weight.
Original asymmetric shoulder-to-tip half lofts/seed pits, closed nonradial
flesh sections and separate intrinsic achenes; no prior game/crop mesh or photo.
New 15.2cm maple spoon is generated from original authored family construction.
-Y tip mirrors in FBX; bottom-centre pivot is not an approved grip anchor.
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

NAME = "StrawberryCompoteSource"
DESCRIPTION = "Held original unsweetened strawberry reduction and eating spoon."
PROVENANCE = "Original closed cooked strawberry halves/pitted skin/intrinsic achenes, juice volume and generated dish/utensil; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 60000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "StrawberryCompote",
          "ingredients": {"Strawberries": 2, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 11), "meshes": {
    "SM_StrawberryCompote": {"focus": (0, -.015, .021), "detail_distance": .29, "detail_fstop": 64},
    "SM_StrawberryCompotePortion": {"focus": (0, -.052, .010), "detail_distance": .25, "detail_fstop": 64},
}}
SEED = 21905
FRUIT_ROWS, FRUIT_COLUMNS = 32, 48
SPOON_LENGTH_M = .152
FRUIT_MEAN_HEIGHT_M = .020
COOKED_EDGE_RADIUS_M = .00035
SEED_ROWS, SEED_COLUMNS = 4, 5
BOWL_PROFILE = ((0, .001), (.037, .001), (.046, .004), (.056, .014),
                (.064, .028), (.066, .034), (.065, .037), (.062, .0375),
                (.060, .032), (.056, .023), (.048, .013), (.037, .006), (0, .006))
PLACEMENTS = ((-.024, -.025), (.005, -.029), (.027, -.014), (-.030, .003),
              (-.007, -.006), (.013, .010), (-.012, .022), (.014, .032))


def fruit_point(t: float, theta: float, index: int, size: float) -> Vector:
    shoulder = max(0, math.sin(math.pi * t)) ** .55 * (1.5 - t)
    radius = (.0096 + .0006 * math.sin(index * 2.3)) * shoulder * size
    radius *= 1 + .035 * math.sin(theta * 3 + index) * math.sin(math.pi * t)
    x, y = radius * math.cos(theta), radius * math.sin(theta) * .72
    z = (.5 - t) * (.028 + .002 * math.sin(index)) * size
    return Vector((x + .0006 * math.sin(t * 4 + index) * size, y, z))


def achene(kit, name: str, material) -> bpy.types.Object:
    vertices, rows, faces = [], [], []
    for ring in range(7):
        phi = math.pi * ring / 6
        row = []
        for side in range(12 if ring not in (0, 6) else 1):
            theta = side * 2 * math.pi / 12
            row.append(len(vertices))
            vertices.append(Vector((.00019 * math.sin(phi) * math.cos(theta),
                                    .00012 * math.sin(phi) * math.sin(theta),
                                    .00046 * math.cos(phi))))
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(12):
            following = (side + 1) % 12
            if len(first) == 1:
                faces.append((first[0], second[side], second[following]))
            elif len(second) == 1:
                faces.append((first[side], second[0], first[following]))
            else:
                faces.append((first[side], second[side], second[following], first[following]))
    obj = kit.mesh(name, vertices, faces, material)
    shapes.closed_normals(obj)
    return obj


def cooked_half(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    sites = [(.18 + row * .16, (.17 + column * .14) * math.pi)
             for row in range(SEED_ROWS) for column in range(SEED_COLUMNS)]
    seed_points = [fruit_point(t, theta, index, size) for t, theta in sites]
    vertices, rows, faces = [], [], []
    for row in range(FRUIT_ROWS + 1):
        ring = []
        t = row / FRUIT_ROWS
        for column in range(FRUIT_COLUMNS + 1 if row not in (0, FRUIT_ROWS) else 1):
            theta = math.pi * column / FRUIT_COLUMNS
            point = fruit_point(t, theta, index, size)
            pit = sum(.00013 * math.exp(-(point - seed).length_squared / (.00065 * size) ** 2)
                      for seed in seed_points)
            point.y -= pit * math.sin(theta)
            point.y += .00016 * noise.noise(point * 670 + Vector((index, 2.9, 6.3))) * math.sin(theta)
            ring.append(len(vertices))
            vertices.append(point)
        rows.append(ring)
    for first, second in zip(rows, rows[1:]):
        for column in range(FRUIT_COLUMNS):
            if len(first) == 1:
                faces.append((first[0], second[column], second[column + 1]))
            elif len(second) == 1:
                faces.append((first[column], second[0], first[column + 1]))
            else:
                faces.append((first[column], second[column], second[column + 1], first[column + 1]))
    boundary = ([row[0] for row in rows] +
                [row[-1] for row in reversed(rows[1:-1])])
    skin_faces = len(faces)
    faces += shapes.cut_food_patch(vertices, boundary, SEED + index, normal_axis=1)
    fruit = kit.mesh(name, vertices, faces, food.stewed_strawberry("M_" + name, index))
    fruit.data.materials.append(food.stewed_strawberry("M_" + name + "Flesh", index, flesh=True))
    for face in fruit.data.polygons[skin_faces:]:
        face.material_index = 1
    shapes.closed_normals(fruit)
    bevel = fruit.modifiers.new("CookedStrawberryCutEdge", "BEVEL")
    bevel.width, bevel.segments, bevel.limit_method, bevel.angle_limit = COOKED_EDGE_RADIUS_M * size, 3, "ANGLE", .6
    kit.apply_modifiers(fruit)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(fruit.data)
        bmesh.ops.dissolve_degenerate(mesh, dist=1e-8, edges=list(mesh.edges))
        mesh.to_mesh(fruit.data)
    finally:
        mesh.free()
    shapes.closed_normals(fruit)
    parts = [fruit]
    material = food.strawberry_achene("M_StrawberryCompoteAchene" + str(index))
    for number, ((t, theta), point) in enumerate(zip(sites, seed_points)):
        seed = achene(kit, name + "Achene" + str(number), material)
        seed.rotation_euler.z = theta - math.pi * .5
        seed.location = point
        parts.append(seed)
    return kit.join(parts, "SM_" + name, pivot="center", unwrap=False, smooth_angle=65)


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl = shapes.vessel(kit, "OriginalStrawberryReductionDish", BOWL_PROFILE,
                         food.fruit_bowl_glaze("M_StrawberryReductionGlazedDish"))
    juice = shapes.vessel(kit, "OriginalStrawberryCookingJuice",
                          ((0, .0061), (.0368, .0061), (.0478, .0131),
                           (.0513, .018), (.032, .018), (0, .018)),
                          food.stewed_strawberry("M_StrawberryReducedJuice", 37))
    supports, parts = [shapes.world_surface(bowl)], [bowl, juice]
    for index, (x, y) in enumerate(PLACEMENTS):
        obj = cooked_half(kit, "StrawberryReductionHalf" + str(index), index)
        obj.rotation_euler = (math.pi * (.5 if index % 2 else -.5) + rng.uniform(-.24, .24),
                              rng.uniform(-.2, .2), rng.uniform(-math.pi, math.pi))
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.057)
        rotation = obj.rotation_euler.to_matrix()
        mean_height = sum((rotation @ vertex.co).z for vertex in obj.data.vertices) / len(obj.data.vertices)
        obj.location.z = max(obj.location.z, FRUIT_MEAN_HEIGHT_M - mean_height)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_StrawberryCompote", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    utensil = shapes.carved_spoon(kit, "OriginalStrawberryReductionMapleSpoon",
                                  food.maple_eating_spoon("M_StrawberryReductionMapleSpoon"),
                                  SPOON_LENGTH_M)
    bite = cooked_half(kit, "StrawberryReductionSpoonHalf", 10, .95)
    bite.rotation_euler.x = -math.pi * .5 + .30
    shapes.seat_on_surfaces(bite, 0, -.0775 * SPOON_LENGTH_M / .182,
                           [shapes.world_surface(utensil)], maximum_radius=.1)
    portion = kit.join([utensil, bite], "SM_StrawberryCompotePortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "StrawberryCompote", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fruit_halves"], serving["food_achenes"] = 8, 160
    portion["food_eating_utensil"] = "original generated 15.2cm carved maple spoon"
    portion["food_grip_not_verified"] = True
    return [serving, portion]
