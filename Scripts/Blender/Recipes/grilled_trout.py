"""Original fire-grilled trout fillet/platter and a separate edible piece.

Usage: New-Prop.ps1 grilled_trout -Live -NoBeauty -OutDirectory <E: scratch>
Held source only; no art/import/grip/bake or real food-safety acceptance.

References read: https://en.wikipedia.org/wiki/Trout (oily food fish) and
https://en.wikipedia.org/wiki/Fish_as_food (cooking preparations). Myomere
reference from the raw-food study informs folded muscle partitions, not a
borrowed fish mesh. Catalogue: RiverTrout1/Kindling1, no oil/butter/herbs/
lemon/batter/garnish. One authored 14cm boneless skin-on fillet, three loose
flakes and a new 22x15.6cm oval platter; portion is a new smaller broken piece.
Dimensions/flake count are presentation, not a stock-weight gameplay rule.
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

NAME = "GrilledTroutSource"
DESCRIPTION = "Held original fire-browned trout fillet and edible piece."
PROVENANCE = "Original closed cooked-fillet loft, muscle relief, separate flakes, oval platter and food PBR; no caught-fish or meal mesh reused."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 65000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "GrilledTrout",
          "ingredients": {"RiverTrout": 1, "Kindling": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, -9), "meshes": {
    "SM_GrilledTrout": {"focus": (0, -.010, .018), "detail_distance": .33, "detail_fstop": 64},
    "SM_GrilledTroutPortion": {"focus": (0, 0, .007), "detail_distance": .24, "detail_fstop": 64},
}}
SEED = 31913
MYOMERE_SPACING_M = .0095
MYOSEPTUM_DEPTH_M = .00065
FILLET_ROWS, FILLET_SIDES = 72, 80
EDIBLE_AXIAL_LENGTH_M = .0332
PLATTER_PROFILE = ((0, .001), (.047, .001), (.064, .003),
                   (.074, .007), (.078, .011), (.0775, .0145),
                   (.075, .016), (.070, .012), (.060, .008),
                   (.043, .005), (0, .005))


def muscle_fold(x: float) -> float:
    return .42 * (math.sqrt(x * x + .000003) - math.sqrt(.000003)) + .0008 * math.sin(x * 190)


def cooked_fillet(kit, name: str, index: int, length: float, width: float,
                  height: float, rows: int, skin: bool = True) -> bpy.types.Object:
    vertices, rings, faces, skin_flags = [], [], [], []
    rng = random.Random(SEED + index)
    partitions = []
    for partition in range(-20, 21):
        partitions.append((partition * MYOMERE_SPACING_M + rng.uniform(-.0012, .0012),
                           rng.uniform(.60, 1.0)))
    positions = {row / rows for row in range(rows + 1)}
    # Add samples at muscle partitions; uniform rows alone miss their narrow troughs.
    for centre, _ in partitions:
        for offset in (-.0013, -.00052, 0, .00052, .0013):
            t = (centre + offset + length * .5) / length
            if 0 < t < 1:
                positions.add(t)
    for t in sorted(positions):
        ring = []
        fullness = ((.27 + .73 * math.sin(.35 + t * math.pi * .81) ** .6) * (1 - .65 * t)
                    if index == 0 else .80 + .20 * math.sin(t * math.pi))
        for side in range(FILLET_SIDES):
            theta = side * 2 * math.pi / FILLET_SIDES
            c, s = math.cos(theta), math.sin(theta)
            x = width * .5 * fullness * c
            x += .00065 * math.sin(t * 11 + index) * abs(s)
            fold = muscle_fold(x)
            y = length * (t - .5) - fold
            top = max(0, s) ** .72
            bottom = max(0, -s) ** .80
            z = height * (.70 * top * (1 + .15 * c) - .30 * bottom) * (.7 + .3 * math.sin(t * math.pi))
            point = Vector((x, y, z))
            centre, depth = min(partitions, key=lambda entry: abs(y + fold - entry[0]))
            distance = abs(y + fold - centre)
            groove = depth * math.exp(-(distance / .00052) ** 2)
            point.z -= top * min(MYOSEPTUM_DEPTH_M, height * .12) * groove
            point.z += top * .00018 * noise.noise(point * 930 + Vector((index, 4.9, 7.1)))
            point.z += bottom * .00022 * noise.noise(point * 440 + Vector((index, 8.1, 3.9)))
            ring.append(len(vertices))
            vertices.append(point)
        rings.append(ring)
    for first, second in zip(rings, rings[1:]):
        for side in range(FILLET_SIDES):
            following = (side + 1) % FILLET_SIDES
            faces.append((first[side], first[following], second[following], second[side]))
            angle = (side + .5) * 2 * math.pi / FILLET_SIDES
            skin_flags.append(skin and (math.sin(angle) < .06 or math.cos(angle) < -.76))
    for cap, boundary in enumerate((rings[0], rings[-1])):
        first_new = len(vertices)
        faces += shapes.cut_food_patch(vertices, boundary, SEED + index * 2 + cap, normal_axis=1)
        mean_fold = sum(muscle_fold(vertices[i].x) for i in boundary) / len(boundary)
        for point in vertices[first_new:]:
            point.y += mean_fold - muscle_fold(point.x)
    obj = kit.mesh(name, vertices, faces, food.grilled_trout_flesh("M_" + name + "Flesh", index))
    if skin:
        obj.data.materials.append(food.grilled_trout_skin("M_" + name + "Skin", index))
        for face, is_skin in zip(obj.data.polygons, skin_flags):
            face.material_index = int(is_skin)
    shapes.closed_normals(obj)
    bevel = obj.modifiers.new("GrilledTroutSoftCutEdges", "BEVEL")
    bevel.width, bevel.segments, bevel.limit_method, bevel.angle_limit = .00035, 3, "ANGLE", .65
    kit.apply_modifiers(obj)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        bmesh.ops.dissolve_degenerate(mesh, dist=1e-8, edges=list(mesh.edges))
        mesh.to_mesh(obj.data)
    finally:
        mesh.free()
    shapes.closed_normals(obj)
    return obj


def build(kit) -> list:
    platter = shapes.vessel(kit, "OriginalGrilledTroutPlatter", PLATTER_PROFILE,
                            food.grilled_trout_platter("M_GrilledTroutPlatter"))
    for vertex in platter.data.vertices:
        vertex.co.x *= .110 / .078
    kit.tag_coords(platter.data)
    shapes.closed_normals(platter)
    supports, parts = [shapes.world_surface(platter)], [platter]
    fillet = cooked_fillet(kit, "CookedTroutFillet", 0, .140, .050, .013, FILLET_ROWS)
    fillet.rotation_euler.z = math.pi / 2 - .13
    shapes.seat_on_surfaces(fillet, 0, 0, supports, maximum_radius=.098)
    supports.append(shapes.world_surface(fillet))
    parts.append(fillet)
    for index, (x, y, angle) in enumerate(((-.027, .035, .2), (.014, .033, -.3), (.033, -.034, .4))):
        flake = cooked_fillet(kit, "CookedTroutFlake" + str(index), index + 3,
                              .015 + index * .001, .013, .0026, 20, skin=False)
        flake.rotation_euler = (.015, -.015 + .012 * index, math.pi / 2 + angle)
        shapes.seat_on_surfaces(flake, x, y, supports, maximum_radius=.098)
        supports.append(shapes.world_surface(flake))
        parts.append(flake)
    serving = kit.join(parts, "SM_GrilledTrout", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    bite = cooked_fillet(kit, "CookedTroutEdiblePiece", 9, EDIBLE_AXIAL_LENGTH_M, .025, .009, 32)
    portion = kit.join([bite], "SM_GrilledTroutPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "GrilledTrout", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_fillets"], serving["food_flake_count"] = 1, 3
    portion["food_grip_not_verified"] = True
    return [serving, portion]
