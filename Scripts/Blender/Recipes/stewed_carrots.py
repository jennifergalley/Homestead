"""Original two-carrot stew serving and independent edible carrot half segment.

Usage: New-Prop.ps1 stewed_carrots -Live -OutDirectory <E: scratch>
Source WIP only. No existing crop/food mesh or photographic texture is reused.

Observation: https://en.wikipedia.org/wiki/Carrot, selectively bred fleshy
taproot with a reduced woody core; the catalogue's orange-root crop is retained.
Author two trimmed 13.8cm roots, each cut into three lengths and split in half
(twelve pieces total), cooked without added oil/herbs/milk. Exact Carrot2 and
Kindling1. A little cooking-water surface is presentation, not a new ingredient.
New 18.4cm hand-thrown bowl; the held portion has no bowl/plate/spoon attached.
"""
import hashlib
import importlib
import math
from pathlib import Path

import homestead_food_geometry as shapes
import homestead_food_materials as food
from mathutils import Vector, noise

shapes = importlib.reload(shapes)
food = importlib.reload(food)

NAME = "StewedCarrotsSource"
DESCRIPTION = "Held original stewed carrot serving and edible portion."
PROVENANCE = "Original tapered root halves, vascular cut fields, hand-thrown bowl and cooking-liquid volume; procedural PBR."
COLLISION = "none"
TRIANGLE_BUDGET = 65000
BAKE = None
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "StewedCarrots",
          "ingredients": {"Carrot": 2, "Kindling": 1}, "original_food": True,
          "serving_carrot_units": 2, "not_import_or_art_acceptance": True,
          "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 12), "meshes": {
    "SM_StewedCarrots": {"focus": (0, -.020, .042), "detail_distance": .34},
    "SM_StewedCarrotsPortion": {"focus": (0, 0, .008), "detail_distance": .23},
}}
ROOT_LENGTH_M = .138
ROOT_RINGS = 32
ROOT_SIDES = 32
COOKED_EDGE_ROUNDING_M = .00035
CUT_COLUMNS = 18
BOWL_FLOOR_M = .008
LIQUID_SURFACE_M = .014
BOWL_PROFILE = ((0, .001), (.045, .001), (.053, .003), (.061, .006),
                (.071, .012), (.082, .024), (.089, .040), (.092, .049),
                (.092, .053), (.090, .055), (.087, .053), (.085, .046),
                (.079, .032), (.070, .022), (.059, .012), (.046, BOWL_FLOOR_M),
                (0, BOWL_FLOOR_M))
PLACEMENTS = ((-.025, -.026, 90), (.025, -.026, 90),
              (-.023, 0, 90), (.023, 0, 90),
              (-.020, -.053, 82), (.025, -.049, 101),
              (-.025, .026, 90), (.025, .026, 90),
              (-.024, .004, 74), (.024, .004, 104),
              (-.024, .051, 98), (.025, .047, 84))
SEATING_ORDER = (0, 1, 6, 7, 2, 3, 8, 9, 4, 5, 10, 11)


def carrot_point(t: float, theta: float, root: int) -> Vector:
    radius = .0015 + .0155 * (1 - t) ** .82
    ridge = 1 + .014 * math.sin(theta * 9 + t * 17 + root)
    point = Vector((radius * math.cos(theta) * ridge,
                    ROOT_LENGTH_M * (t - .5),
                    radius * math.sin(theta) * ridge))
    point.x += .0011 * math.sin(math.pi * t) * (1 if root == 0 else -1)
    return point


def segment(kit, name: str, root: int, section: int, upper: bool):
    vertices, rows, faces = [], [], []
    for ring in range(ROOT_RINGS + 1):
        t = (section + ring / ROOT_RINGS) / 3
        row = []
        for side in range(ROOT_SIDES + 1):
            theta = (0 if upper else math.pi) + math.pi * side / ROOT_SIDES
            point = carrot_point(t, theta, root)
            if upper:
                point.z *= -1
            row.append(len(vertices))
            vertices.append(point)
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(ROOT_SIDES):
            faces.append((first[side], first[side + 1], second[side + 1], second[side]))
    flesh_start = len(faces)
    cuts = []
    for ring, row in enumerate(rows):
        left, right = (row[0], row[-1]) if not upper else (row[-1], row[0])
        cut = [left]
        for column in range(1, CUT_COLUMNS):
            fraction = column / CUT_COLUMNS
            point = vertices[left].lerp(vertices[right], fraction)
            point.z += (.00010 * noise.noise(point * 600 + Vector((root, 3.8, 7.2)))
                        * math.sin(math.pi * fraction) * math.sin(math.pi * ring / ROOT_RINGS))
            cut.append(len(vertices))
            vertices.append(point)
        cut.append(right)
        cuts.append(cut)
    for first, second in zip(cuts, cuts[1:]):
        for column in range(CUT_COLUMNS):
            faces.append((first[column], second[column], second[column + 1], first[column + 1]))
    for row, cut in ((rows[0], cuts[0]), (rows[-1], cuts[-1])):
        if cut[0] != row[0]:
            cut = list(reversed(cut))
        faces.append(tuple(row + list(reversed(cut[1:-1]))))
    seed = root * 7 + section + 51
    obj = kit.mesh(name, vertices, faces, food.stewed_carrot("M_" + name + "Skin", seed, True))
    obj.data.materials.append(food.stewed_carrot("M_" + name + "Flesh", seed, False))
    for face in list(obj.data.polygons)[flesh_start:]:
        face.material_index = 1
    shapes.closed_normals(obj)
    bevel = obj.modifiers.new("CookedCarrotKnifeEdgeRounding", "BEVEL")
    bevel.width = COOKED_EDGE_ROUNDING_M
    bevel.segments = 2
    bevel.limit_method = "ANGLE"
    bevel.angle_limit = .40
    kit.apply_modifiers(obj)
    shapes.closed_normals(obj)
    return obj


def liquid(kit):
    profile = ((0, BOWL_FLOOR_M), (.045, BOWL_FLOOR_M), (.058, .012),
               (.061, LIQUID_SURFACE_M), (.051, LIQUID_SURFACE_M - .0003),
               (0, LIQUID_SURFACE_M - .0003))
    return shapes.vessel(kit, "OriginalCarrotCookingLiquid", profile,
                         food.cooking_liquid("M_OriginalCarrotCookingLiquid"), sides=160)


def build(kit) -> list:
    bowl = shapes.vessel(kit, "OriginalCarrotStewBowl", BOWL_PROFILE,
                         food.earthenware("M_OriginalCarrotStewEarthenware", 37))
    parts = [bowl, liquid(kit)]
    supports = [shapes.world_surface(bowl)]
    for index in SEATING_ORDER:
        x, y, angle = PLACEMENTS[index]
        root, section, upper = index // 6, (index % 6) // 2, bool(index % 2)
        obj = segment(kit, "StewedCarrotSegment" + str(index), root, section, upper)
        centre_y = sum(v.co.y for v in obj.data.vertices) / len(obj.data.vertices)
        for vertex in obj.data.vertices:
            vertex.co.y -= centre_y
        obj.rotation_euler.z = math.radians(angle)
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.0865)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_StewedCarrots", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    serving["food_carrot_units"] = 2
    serving["food_piece_count"] = 12
    portion = kit.join([segment(kit, "HandheldStewedCarrot", 0, 1, False)],
                       "SM_StewedCarrotsPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"] = True
        obj["food_source_only"] = True
        obj["food_item"] = "StewedCarrots"
        obj["food_role"] = role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    return [serving, portion]
