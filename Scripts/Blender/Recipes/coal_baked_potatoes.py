"""Original coal-roasted potatoes: two-tuber serving and edible half portion.

Usage: New-Prop.ps1 coal_baked_potatoes -Live -OutDirectory <E: scratch>
Source-only WIP; no prior crop/root/food mesh or photo texture is reused.

Observation: Wikimedia Commons File:Baked potato, baked sweet potato, and
steamed kale.jpg, O'Dea, CC BY-SA 4.0 (photo not included or used as texture).
Only potato skin/starchy flesh are studied; no pictured kale or toppings.
Cooking dehydrates/creases the skin, leaves localized charcoal scorch and
exposes crumbly pale starch. The serving contains exactly two potato units:
one whole and a second split into two halves, without oil/butter/garnish.
Original 23.2cm turned elm platter; tubers 9.6-10.4cm, portion 7.8cm long.
"""
import hashlib
import math
import random
from pathlib import Path

import bmesh
import homestead_food_materials as food
import homestead_materials as materials
from mathutils import Vector, noise

NAME = "PreparedPotatoesSource"
DESCRIPTION = "Held original coal-baked potato serving and edible portion."
PROVENANCE = "Original custom tuber, eye-pit, split crumb and turned-platter geometry; procedural PBR. O'Dea CC BY-SA 4.0 photo observed only."
COLLISION = "none"
TRIANGLE_BUDGET = 65000
BAKE = None
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "BakedPotatoes",
          "ingredients": {"Potato": 2, "Kindling": 1},
          "original_food": True, "serving_potato_units": 2,
          "food_material_source_sha256": hashlib.sha256(
              Path(food.__file__).read_bytes()).hexdigest(),
          "not_import_or_art_acceptance": True}
BEAUTY = {"pose": (0, 0, 12), "meshes": {
    "SM_BakedPotatoes": {"focus": (0, -.015, .045), "detail_distance": .40},
    "SM_BakedPotatoesPortion": {"focus": (0, 0, .018), "detail_distance": .27},
}}
TUBER_RINGS = 72
TUBER_SIDES = 96
CRUMB_COLUMNS = 80
CRUMB_CELL_SCALE_PER_M = 280
CRUMB_FRACTURE_DEPTH_M = .0011
EYE_PIT_RADIUS_M = .0023
EYE_PIT_DEPTH_M = .0009
PLATTER_FLOOR_M = .009
PLATTER_PROFILE = ((0, .001), (.07, .001), (.096, .002), (.108, .005),
                   (.115, .008), (.116, .011), (.115, .014), (.112, .016),
                   (.108, .0165), (.104, .014), (.101, .012),
                   (.097, .010), (.089, PLATTER_FLOOR_M), (0, PLATTER_FLOOR_M))


def close_normals(obj) -> None:
    mesh = bmesh.new()
    mesh.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
    mesh.to_mesh(obj.data)
    mesh.free()
    obj.data.update()


def tuber_point(phi: float, theta: float, size: tuple, seed: int, eyes: list) -> Vector:
    width, length, height = size
    radial = math.sin(phi)
    shape = 1 + .047 * math.sin(3 * theta + seed) * radial + .034 * math.sin(5 * phi + theta)
    point = Vector((width * .5 * radial * math.cos(theta) * shape,
                    length * .5 * math.cos(phi) + .002 * math.sin(2 * phi + seed) * radial,
                    height * .5 * radial * math.sin(theta) * shape))
    normal = Vector((point.x / width ** 2, point.y / length ** 2, point.z / height ** 2)).normalized()
    wrinkles = (.00055 * math.sin(theta * 31 + 1.7 * math.sin(phi * 7 + seed))
                * (.55 + .35 * noise.noise(point * 160 + Vector((seed, 0, 0)))) * radial ** 3)
    pits = sum(-EYE_PIT_DEPTH_M * math.exp(-(point - eye).length_squared / EYE_PIT_RADIUS_M ** 2)
               for eye in eyes)
    return point + normal * (wrinkles + pits)


def potato(kit, name: str, size: tuple, seed: int, split: bool, upper_half: bool = False):
    generator = random.Random(seed)
    eyes = [tuber_point(generator.uniform(.35, math.pi - .35),
                        generator.uniform(0, 2 * math.pi), size, seed, [])
            for _ in range(19)]
    sides = TUBER_SIDES // 2 if split else TUBER_SIDES
    columns = sides + 1 if split else sides
    angle = lambda side: ((0 if upper_half else math.pi) + math.pi * side / sides if split
                          else 2 * math.pi * side / sides)
    def point_at(phi, theta):
        point = tuber_point(phi, theta, size, seed, eyes)
        if upper_half:
            point.z *= -1
        if split:
            edge = abs(math.cos(theta)) ** 24 * math.sin(phi)
            point.z += .0012 * edge * noise.noise(point * 850 + Vector((seed, 3.1, 8.7)))
        return point
    vertices = [point_at(0, 0)]
    for ring in range(1, TUBER_RINGS):
        vertices.extend(point_at(math.pi * ring / TUBER_RINGS, angle(side))
                        for side in range(columns))
    end = len(vertices)
    vertices.append(point_at(math.pi, 0))
    faces = []
    for side in range(sides):
        next_side = side + 1 if split else (side + 1) % sides
        faces.append((0, 1 + next_side, 1 + side))
        last = 1 + (TUBER_RINGS - 2) * columns
        faces.append((end, last + side, last + next_side))
    for ring in range(TUBER_RINGS - 2):
        row, next_row = 1 + ring * columns, 1 + (ring + 1) * columns
        for side in range(sides):
            next_side = side + 1 if split else (side + 1) % sides
            faces.append((row + side, row + next_side, next_row + next_side, next_row + side))
    flesh_faces = []
    if split:
        rows = []
        for ring in range(TUBER_RINGS - 1):
            first, last = 1 + ring * columns, 1 + ring * columns + sides
            left, right = (first, last) if not upper_half else (last, first)
            row = [left]
            for column in range(1, CRUMB_COLUMNS):
                t = column / CRUMB_COLUMNS
                point = vertices[left].lerp(vertices[right], t)
                point.z *= (1 - math.sin(math.pi * t)) ** 2
                grain = noise.noise(point * 640 + Vector((seed, seed * .31, 0)))
                distances = noise.voronoi(point * CRUMB_CELL_SCALE_PER_M
                                          + Vector((seed, seed * .31, 0)))[0]
                fracture = CRUMB_FRACTURE_DEPTH_M * math.exp(-((distances[1] - distances[0]) / .15) ** 2)
                fade = math.sin(math.pi * (ring + 1) / TUBER_RINGS) * math.sin(math.pi * t)
                point.z += (.0042 + .0012 * grain - fracture) * fade
                row.append(len(vertices))
                vertices.append(point)
            row.append(right)
            rows.append(row)
        for column in range(CRUMB_COLUMNS):
            faces.append((0, rows[0][column], rows[0][column + 1]))
            faces.append((end, rows[-1][column + 1], rows[-1][column]))
            if 0 < column < CRUMB_COLUMNS - 1:
                flesh_faces.extend((len(faces) - 2, len(faces) - 1))
        for first, second in zip(rows, rows[1:]):
            for column in range(CRUMB_COLUMNS):
                faces.append((first[column], second[column], second[column + 1], first[column + 1]))
                if 0 < column < CRUMB_COLUMNS - 1:
                    flesh_faces.append(len(faces) - 1)
    obj = kit.mesh(name, vertices, faces, food.potato_skin("M_" + name + "Skin", seed))
    if split:
        obj.data.materials.append(food.potato_flesh("M_" + name + "Flesh", seed))
        for index in flesh_faces:
            obj.data.polygons[index].material_index = 1
    close_normals(obj)
    obj["potato_units"] = .5 if split else 1
    return obj


def platter(kit):
    sides = 128
    vertices, rows, faces = [], [], []
    for radius, height in PLATTER_PROFILE:
        if radius == 0:
            rows.append([len(vertices)])
            vertices.append(Vector((0, 0, height)))
        else:
            rows.append(list(range(len(vertices), len(vertices) + sides)))
            for side in range(sides):
                theta = side * 2 * math.pi / sides
                irregular = .00022 * math.sin(theta * 9 + radius * 100) * (radius / .116) ** 5
                vertices.append(Vector(((radius + irregular) * math.cos(theta),
                                        (radius + irregular) * math.sin(theta), height)))
    for first, second in zip(rows, rows[1:]):
        for side in range(sides):
            following = (side + 1) % sides
            if len(first) == 1:
                faces.append((first[0], second[following], second[side]))
            elif len(second) == 1:
                faces.append((first[side], first[following], second[0]))
            else:
                faces.append((first[side], first[following], second[following], second[side]))
    material = materials.wood("M_OriginalPotatoElmPlatter", light=(.22, .145, .073),
                              dark=(.15, .09, .041), grain=.80, roughness=.69,
                              weathering=.12, grime=.06, seed=13, relief=.22)
    obj = kit.mesh("OriginalPotatoElmPlatter", vertices, faces, material)
    kit.tag_coords(obj.data, [(point.y, point.z, point.x) for point in vertices])
    close_normals(obj)
    return obj


def seat(obj, x: float, y: float, angle: float) -> None:
    obj.location = (x, y, PLATTER_FLOOR_M - min(v.co.z for v in obj.data.vertices))
    obj.rotation_euler.z = math.radians(angle)


def build(kit) -> list:
    dish = platter(kit)
    whole = potato(kit, "WholeCoalPotato", (.062, .104, .059), 17, False)
    first = potato(kit, "SplitCoalPotatoLeft", (.066, .096, .057), 29, True)
    second = potato(kit, "SplitCoalPotatoRight", (.066, .096, .057), 29, True, upper_half=True)
    seat(whole, -.020, .054, -24)
    seat(first, -.037, -.027, -13)
    seat(second, .038, -.021, 18)
    serving = kit.join([dish, whole, first, second], "SM_BakedPotatoes", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    serving["food_potato_units"] = 2
    portion = potato(kit, "HandheldCoalPotato", (.053, .078, .046), 41, True)
    portion = kit.join([portion], "SM_BakedPotatoesPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"] = True
        obj["food_item"] = "BakedPotatoes"
        obj["food_role"] = role
        obj["food_source_only"] = True
    return [serving, portion]
