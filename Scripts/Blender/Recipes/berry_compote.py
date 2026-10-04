"""Original unsweetened cooked bramble reduction, glazed dish and eating spoon.

Usage: New-Prop.ps1 berry_compote -Live -NoBeauty -OutDirectory <E: scratch>
Source WIP, no bake/import/grip/animation acceptance.

References: https://en.wikipedia.org/wiki/Compote (cooked fruit/unsweetened puree),
https://en.wikipedia.org/wiki/Blackberry (aggregate drupelet fruit). Author twelve
softened 11-16mm bramble fruits, ruptured/collapsed lobes and juice in a new
11.8cm shallow glazed bowl; no sugar, spice, cream, leaves or invented garnish.
Exact Berries3/Kindling1, with water as presentation rather than new inventory.
Each fruit is one closed, non-spherical loft with seeded angular drupelet relief,
not a stack of intersecting sphere primitives. Original spoon construction is
shared authored geometry, generated at 14.6cm before UV/material coordinates;
no prior static mesh/photographic texture/game asset is reused. -Y tip mirrors
on FBX export; bottom-centre pivot is not an approved hand attachment.
"""
import importlib
import math
import random

import bpy
import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise
from mathutils.geometry import delaunay_2d_cdt

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "BerryCompoteSource"
DESCRIPTION = "Held original unsweetened cooked bramble reduction and eating spoon."
PROVENANCE = "Original closed aggregate-fruit lofts, reduced-juice volume, glazed dish and generated maple utensil; procedural PBR."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 60000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "BerryCompote",
          "ingredients": {"Berries": 3, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 9), "meshes": {
    "SM_BerryCompote": {"focus": (0, -.009, .022), "detail_distance": .26, "detail_fstop": 64},
    "SM_BerryCompotePortion": {"focus": (0, -.048, .010), "detail_distance": .23, "detail_fstop": 64},
}}
SEED = 21904
FRUIT_ROWS, FRUIT_SIDES = 32, 64
SPOON_LENGTH_M = .146
JUICE_SURFACE_M = .014
BOWL_PROFILE = ((0, .001), (.032, .001), (.040, .003), (.050, .010),
                (.057, .022), (.059, .029), (.058, .033), (.055, .0335),
                (.054, .029), (.050, .021), (.043, .011), (.034, .006), (0, .006))
FRUIT_PLACEMENTS = ((-.030, -.022), (-.009, -.031), (.018, -.027),
                    (.032, -.008), (-.030, .006), (-.011, -.007),
                    (.008, -.007), (-.014, .017), (.010, .016),
                    (.026, .022), (-.007, .033), (-.034, .026))


def cooked_fruit(kit, name: str, index: int, squash: float = 1,
                 torn: bool = False) -> bpy.types.Object:
    rng = random.Random(SEED + index)
    radius, length = rng.uniform(.0054, .0069), rng.uniform(.012, .016)
    centres = []
    for row in range(1, 9):
        phi = math.pi * row / 9
        count = max(4, round(13 * math.sin(phi)))
        for side in range(count):
            theta = (side + .35 * (row % 2) + rng.uniform(-.1, .1)) * 2 * math.pi / count
            centres.append(Vector((math.sin(phi) * math.cos(theta),
                                   math.sin(phi) * math.sin(theta), math.cos(phi))))
    vertices, rows, faces = [], [], []
    for row in range(FRUIT_ROWS + 1):
        ring = []
        count = 1 if row == 0 or (row == FRUIT_ROWS and not torn) else FRUIT_SIDES
        for side in range(count):
            theta = side * 2 * math.pi / FRUIT_SIDES
            maximum_phi = 2.05 + .13 * math.sin(theta * 5 + index) if torn else math.pi
            phi = maximum_phi * row / FRUIT_ROWS
            direction = Vector((math.sin(phi) * math.cos(theta),
                                math.sin(phi) * math.sin(theta), math.cos(phi)))
            drupelet = .0007 * sum(math.exp((direction.dot(centre) - 1) * 110)
                                  for centre in centres)
            radial = radius + drupelet
            point = Vector((direction.x * radial, direction.y * radial,
                            direction.z * (length * .5 + drupelet) * squash))
            collapse = .00038 * noise.noise(point * 360 + Vector((index, 4.7, 9.2)))
            point.x += collapse * math.sin(phi) ** 2
            point.y += .0007 * direction.z ** 2 * math.sin(index + phi)
            ring.append(len(vertices))
            vertices.append(point)
        rows.append(ring)
    for first, second in zip(rows, rows[1:]):
        for side in range(FRUIT_SIDES):
            following = (side + 1) % FRUIT_SIDES
            if len(first) == 1:
                faces.append((first[0], second[side], second[following]))
            elif len(second) == 1:
                faces.append((first[side], second[0], first[following]))
            else:
                faces.append((first[side], second[side], second[following], first[following]))
    skin_face_count = len(faces)
    if torn:
        boundary = rows[-1]
        outline = [Vector((vertices[index].x, vertices[index].y)) for index in boundary]
        points = list(outline)
        xs, ys = [point.x for point in outline], [point.y for point in outline]
        for row in range(1, 11):
            for column in range(1, 11):
                x = min(xs) + (max(xs) - min(xs)) * (column + rng.uniform(-.15, .15)) / 11
                y = min(ys) + (max(ys) - min(ys)) * (row + rng.uniform(-.15, .15)) / 11
                inside = False
                for a, b in zip(outline, outline[1:] + outline[:1]):
                    if (a.y > y) != (b.y > y) and x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x:
                        inside = not inside
                if inside:
                    points.append(Vector((x, y)))
        cap_points, _, cap_faces, origins, _, _ = delaunay_2d_cdt(
            points, [(i, (i + 1) % FRUIT_SIDES) for i in range(FRUIT_SIDES)],
            [tuple(range(FRUIT_SIDES))], 1, 1e-8)
        height = sum(vertices[index].z for index in boundary) / len(boundary)
        indices = []
        for point, source in zip(cap_points, origins):
            existing = next((i for i in source if i < FRUIT_SIDES), None)
            if existing is not None:
                indices.append(boundary[existing])
            else:
                indices.append(len(vertices))
                relief = .00014 * noise.noise(Vector((point.x * 2600, point.y * 2600, index)))
                vertices.append(Vector((point.x, point.y, height + relief)))
        faces += [tuple(indices[i] for i in face) for face in cap_faces]
    obj = kit.mesh(name, vertices, faces, food.reduced_blackberry("M_" + name, index + 3))
    if torn:
        obj.data.materials.append(food.reduced_blackberry("M_" + name + "Pulp", index + 3, pulp=True))
        for face in obj.data.polygons[skin_face_count:]:
            face.material_index = 1
    shapes.closed_normals(obj)
    return obj


def reduced_juice(kit) -> bpy.types.Object:
    radius = .0447
    vertices, rows, faces = [], [], []
    for ring in range(25):
        r = radius * ring / 24
        row = []
        for side in range(128 if ring else 1):
            theta = side * 2 * math.pi / 128
            x, y = r * math.cos(theta), r * math.sin(theta)
            field = noise.noise(Vector((x * 270, y * 270, 8.7)))
            height = JUICE_SURFACE_M + .0013 * field * (1 - r / radius)
            row.append(len(vertices))
            vertices.append(Vector((x, y, height)))
        rows.append(row)
    for r, height in ((.0428, .0111), (.0338, .0061), (0, .0061)):
        row = []
        for side in range(128 if r else 1):
            theta = side * 2 * math.pi / 128
            row.append(len(vertices))
            vertices.append(Vector((r * math.cos(theta), r * math.sin(theta), height)))
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(128):
            following = (side + 1) % 128
            if len(first) == 1:
                faces.append((first[0], second[side], second[following]))
            elif len(second) == 1:
                faces.append((first[side], second[0], first[following]))
            else:
                faces.append((first[side], second[side], second[following], first[following]))
    obj = kit.mesh("OriginalReducedBrambleJuice", vertices, faces,
                   food.reduced_blackberry("M_BrambleReducedJuice", 37, juice=True))
    shapes.closed_normals(obj)
    return obj


def build(kit) -> list:
    rng = random.Random(SEED)
    bowl = shapes.vessel(kit, "OriginalBerryReductionDish", BOWL_PROFILE,
                         food.fruit_bowl_glaze("M_BerryReductionGlazedDish"))
    juice = reduced_juice(kit)
    supports, parts = [shapes.world_surface(bowl)], [bowl, juice]
    for index, (x, y) in enumerate(FRUIT_PLACEMENTS):
        x, y = x * .73, y * .73
        obj = cooked_fruit(kit, "BrambleReductionFruit" + str(index), index,
                           .39 + .09 * (index % 4), torn=index in (3, 6, 9, 11))
        obj.rotation_euler = (rng.uniform(-.7, .7), rng.uniform(-.7, .7),
                              rng.uniform(-math.pi, math.pi))
        if index in (3, 6, 9, 11):
            obj.rotation_euler.x += math.pi
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.052)
        mean = sum((obj.rotation_euler.to_matrix() @ vertex.co).z
                   for vertex in obj.data.vertices) / len(obj.data.vertices)
        obj.location.z = max(obj.location.z, JUICE_SURFACE_M + .001 - mean)
        parts.append(obj)
        supports.append(shapes.world_surface(obj))
    serving = kit.join(parts, "SM_BerryCompote", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    utensil = shapes.carved_spoon(kit, "OriginalBerryReductionMapleSpoon",
                                  food.maple_eating_spoon("M_BerryReductionMapleSpoon"),
                                  SPOON_LENGTH_M)
    bite = cooked_fruit(kit, "BrambleReductionSpoonFruit", 15, .85, torn=True)
    bite.rotation_euler.x = math.pi
    shapes.seat_on_surfaces(bite, 0, -.0775 * SPOON_LENGTH_M / .182,
                           [shapes.world_surface(utensil)], maximum_radius=.1)
    portion = kit.join([utensil, bite], "SM_BerryCompotePortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "BerryCompote", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fruit_pieces"] = 12
    portion["food_eating_utensil"] = "original generated 14.6cm carved maple spoon"
    portion["food_grip_not_verified"] = True
    return [serving, portion]
