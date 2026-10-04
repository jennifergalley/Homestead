"""Original six-wedge roasted turnip serving and separate edible wedge.

Usage: New-Prop.ps1 roasted_turnips -Live -OutDirectory <E: scratch>
Source WIP only, no baking/import or existing crop/meal mesh reuse.

Observation: https://en.wikipedia.org/wiki/Turnip describes a globular 5-20cm
white root with a coloured exposed shoulder and white interior. Author one
7.6cm trimmed purple-top root cut into six sectors, modest dry-heat browning,
no oil/herbs/butter. Exact catalogue ingredients: Turnip1 and Kindling1.
The new 20cm low earthenware dish is hand-thrown, not a reused food container.
"""
import hashlib
import importlib
import math
from pathlib import Path

import bmesh
import homestead_food_materials as food
from mathutils import Vector, noise

food = importlib.reload(food)

NAME = "RoastedTurnipsSource"
DESCRIPTION = "Held original dry-roasted turnip wedges and edible portion."
PROVENANCE = "Original sector-loft/cut-plane turnip and hand-thrown earthenware; procedural food PBR. Text anatomy reference only."
COLLISION = "none"
TRIANGLE_BUDGET = 65000
BAKE = None
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "RoastedTurnips",
          "ingredients": {"Turnip": 1, "Kindling": 1}, "original_food": True,
          "serving_turnip_units": 1, "not_import_or_art_acceptance": True,
          "food_material_source_sha256": hashlib.sha256(Path(food.__file__).read_bytes()).hexdigest()}
BEAUTY = {"pose": (0, 0, 8), "meshes": {
    "SM_RoastedTurnips": {"focus": (0, -.025, .037), "detail_distance": .34},
    "SM_RoastedTurnipsPortion": {"focus": (0, 0, .020), "detail_distance": .27},
}}
ROOT_RINGS = 48
RIND_COLUMNS = 26
CUT_COLUMNS = 16
ROOT_LENGTH_M = .076
ROOT_RADIUS_M = .039
COOKED_EDGE_ROUNDING_M = .00055
DISH_FLOOR_M = .008
DISH_PROFILE = ((0, .001), (.060, .001), (.076, .003), (.089, .007),
                (.098, .012), (.100, .016), (.100, .019), (.097, .020),
                (.092, .017), (.084, .012), (.073, .009), (.060, DISH_FLOOR_M),
                (0, DISH_FLOOR_M))


def repair_closed_normals(obj) -> None:
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        if any(len(edge.link_faces) != 2 for edge in mesh.edges):
            raise ValueError(obj.name + " is not a closed meal component")
        bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
        if mesh.calc_volume(signed=True) < 0:
            bmesh.ops.reverse_faces(mesh, faces=list(mesh.faces))
        if mesh.calc_volume(signed=True) <= 0:
            raise ValueError(obj.name + " has no positive closed volume")
        mesh.to_mesh(obj.data)
    finally:
        mesh.free()
    obj.data.update()


def root_point(phi: float, theta: float) -> Vector:
    radial = math.sin(phi)
    shoulder = 1 + .065 * math.cos(phi) + .027 * math.sin(3 * theta + phi)
    point = Vector((ROOT_RADIUS_M * radial * math.cos(theta) * shoulder,
                    ROOT_LENGTH_M * .5 * math.cos(phi),
                    ROOT_RADIUS_M * radial * math.sin(theta) * shoulder))
    normal = Vector((point.x, 0, point.z)).normalized()
    point += normal * .00010 * noise.noise(point * 420 + Vector((1.7, 3.1, 8.9)))
    return point


def wedge(kit, name: str, sector: int):
    offset = sector * math.pi / 3
    rotate = lambda point: Vector((math.cos(offset) * point.x + math.sin(offset) * point.z,
                                   point.y, -math.sin(offset) * point.x + math.cos(offset) * point.z))
    vertices = [Vector((0, ROOT_LENGTH_M * .5, 0))]
    rows = []
    for ring in range(1, ROOT_RINGS):
        phi = math.pi * ring / ROOT_RINGS
        row = list(range(len(vertices), len(vertices) + RIND_COLUMNS + 1))
        rows.append(row)
        vertices.extend(rotate(root_point(phi, offset + math.pi * side / (3 * RIND_COLUMNS)))
                        for side in range(RIND_COLUMNS + 1))
    end = len(vertices)
    vertices.append(Vector((0, -ROOT_LENGTH_M * .5, 0)))
    faces = []
    for side in range(RIND_COLUMNS):
        faces.extend(((0, rows[0][side + 1], rows[0][side]),
                      (end, rows[-1][side], rows[-1][side + 1])))
    for first, second in zip(rows, rows[1:]):
        for side in range(RIND_COLUMNS):
            faces.append((first[side], first[side + 1], second[side + 1], second[side]))
    flesh_start = len(faces)
    axes = []
    for row in rows:
        axes.append(len(vertices))
        vertices.append(Vector((0, vertices[row[0]].y, 0)))
    for side in (0, RIND_COLUMNS):
        cuts = []
        for ring, (row, axis) in enumerate(zip(rows, axes)):
            cut = [axis]
            for column in range(1, CUT_COLUMNS):
                fraction = column / CUT_COLUMNS
                point = vertices[axis].lerp(vertices[row[side]], fraction)
                normal = Vector((0, 0, -1)) if side == 0 else Vector((-.8660254, 0, .5))
                relief = .00014 * noise.noise(point * 290 + Vector((sector, 7.1, 3.7)))
                point += normal * relief * math.sin(math.pi * fraction)
                cut.append(len(vertices))
                vertices.append(point)
            cut.append(row[side])
            cuts.append(cut)
        for column in range(CUT_COLUMNS):
            faces.extend(((0, cuts[0][column], cuts[0][column + 1]),
                          (end, cuts[-1][column + 1], cuts[-1][column])))
        for first, second in zip(cuts, cuts[1:]):
            for column in range(CUT_COLUMNS):
                faces.append((first[column], second[column], second[column + 1], first[column + 1]))
    obj = kit.mesh(name, vertices, faces, food.roasted_turnip("M_" + name + "Skin", sector + 31, True))
    obj.data.materials.append(food.roasted_turnip("M_" + name + "Flesh", sector + 31, False))
    for face in list(obj.data.polygons)[flesh_start:]:
        face.material_index = 1
    for vertex in obj.data.vertices:
        vertex.co.x -= ROOT_RADIUS_M * .5
    repair_closed_normals(obj)
    bevel = obj.modifiers.new("CookedKnifeEdgeRounding", "BEVEL")
    bevel.width = COOKED_EDGE_ROUNDING_M
    bevel.segments = 3
    bevel.limit_method = "ANGLE"
    bevel.angle_limit = .40
    kit.apply_modifiers(obj)
    repair_closed_normals(obj)
    return obj


def dish(kit):
    sides = 128
    vertices, rows, faces = [], [], []
    for radius, height in DISH_PROFILE:
        row = []
        for side in range(sides if radius else 1):
            theta = side * 2 * math.pi / sides
            wobble = .00018 * math.sin(theta * 7 + .8) * (radius / .100) ** 4
            row.append(len(vertices))
            vertices.append(Vector((radius * math.cos(theta), radius * math.sin(theta), height + wobble)))
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(sides):
            following = (side + 1) % sides
            if len(first) == 1:
                faces.append((first[0], second[following], second[side]))
            elif len(second) == 1:
                faces.append((first[side], first[following], second[0]))
            else:
                faces.append((first[side], first[following], second[following], second[side]))
    obj = kit.mesh("OriginalRoastTurnipEarthenware", vertices, faces,
                   food.earthenware("M_OriginalRoastTurnipEarthenware", 19))
    repair_closed_normals(obj)
    return obj


def dish_surface_height(radius: float) -> float:
    profile = tuple(reversed(DISH_PROFILE[8:]))
    for (first_radius, first_height), (second_radius, second_height) in zip(profile, profile[1:]):
        if radius <= second_radius:
            fraction = max(0, (radius - first_radius) / (second_radius - first_radius))
            return first_height + fraction * (second_height - first_height)
    raise ValueError("Roasted turnip wedge extends beyond its dish's inner surface")


def build(kit) -> list:
    parts = [dish(kit)]
    for sector in range(6):
        obj = wedge(kit, "RoastedTurnipWedge" + str(sector), sector)
        theta = sector * math.pi / 3 + .14 + (.02, -.04, .05, -.03, .01, -.02)[sector]
        obj.rotation_euler.y = (0, -.07, .10, -.05, .09, -.06)[sector]
        obj.rotation_euler.z = theta - math.pi * .5
        reach = (.046, .043, .047, .044, .046, .043)[sector]
        x, y = reach * math.cos(theta), reach * math.sin(theta)
        rotation = obj.rotation_euler.to_matrix()
        offsets = []
        for vertex in obj.data.vertices:
            point = rotation @ vertex.co
            radius = math.hypot(point.x + x, point.y + y)
            offsets.append(dish_surface_height(radius) - point.z)
        obj.location = (x, y, max(offsets))
        parts.append(obj)
    serving = kit.join(parts, "SM_RoastedTurnips", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    serving["food_turnip_units"] = 1
    serving["food_piece_count"] = 6
    portion = kit.join([wedge(kit, "HandheldRoastedTurnipWedge", 2)],
                       "SM_RoastedTurnipsPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"] = True
        obj["food_source_only"] = True
        obj["food_item"] = "RoastedTurnips"
        obj["food_role"] = role
    return [serving, portion]
