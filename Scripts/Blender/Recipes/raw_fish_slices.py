"""Original raw mackerel slices, oval dish and a separate edible slice.

Usage: New-Prop.ps1 raw_fish_slices -Live -NoBeauty -OutDirectory <E: scratch>
Held source only, never an artistic/import/eating-grip or bake acceptance claim.

References read: https://en.wikipedia.org/wiki/Myomere (folded fish muscle/
myosepta) and https://en.wikipedia.org/wiki/Mackerel_as_food. Catalogue's
RawFishSlices uses SeaMackerel1, no fuel/rice/vinegar/soy/citrus/herbs/garnish.
Authored six 3.6-4.4cm skin-on boneless slices with pale pink-white flesh,
dark lateral muscle and narrow myosepta on a new 22x16.4cm oval ceramic dish.
Piece count/size are presentation, not a stock-weight or real food-safety rule.
New smaller edible slice is not a reused catch/crop/meal mesh. No caught-fish
library, shader, bake, diagnostic or production budget is changed.
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

NAME = "RawFishSlicesSource"
DESCRIPTION = "Held original boneless mackerel slices and separate edible portion."
PROVENANCE = "Original closed fillet-cut lofts with real shallow myoseptal relief, independent oval dish and procedural food PBR; no catch mesh reused."
COLLISION = "none"
BAKE = None
TRIANGLE_BUDGET = 65000
REPORT = {"draft_only": True, "draft_mode": "source", "food_item": "RawFishSlices",
          "ingredients": {"SeaMackerel": 1}, "original_food": True,
          "not_import_or_art_acceptance": True, "eating_grip_not_verified": True,
          "no_new_animation": True, "food_geometry_source_sha256": shapes.SOURCE_SHA256,
          "food_material_source_sha256": food.SOURCE_SHA256}
BEAUTY = {"pose": (0, 0, 11), "meshes": {
    "SM_RawFishSlices": {"focus": (0, -.016, .014), "detail_distance": .33, "detail_fstop": 64},
    "SM_RawFishSlicesPortion": {"focus": (0, 0, .004), "detail_distance": .24, "detail_fstop": 64},
}}
SEED = 21907
SLICE_ROWS, SLICE_COLUMNS = 36, 80
SLICE_HEIGHT_M = .0075
MYOSEPTUM_RELIEF_M = .00008
PLATE_X_SCALE = .110 / .082
PLATE_PROFILE = ((0, .001), (.052, .001), (.069, .003), (.079, .007),
                 (.082, .011), (.0815, .014), (.079, .015),
                 (.074, .012), (.066, .008), (.05, .006), (0, .006))
PLACEMENTS = ((-.047, -.025, -.18), (0, -.027, -.08), (.047, -.022, .05),
              (-.040, .014, .12), (.003, .013, -.10), (.045, .016, .17))


def myoseptum_height(point: Vector, index: int) -> float:
    phase = (point.y + abs(point.x) * food.RAW_MYOMERE_SLOPE
             + food.RAW_MYOMERE_CURVE_M * math.sin(point.x * 260)
             + .001 * (index % 5)) / food.RAW_MYOMERE_SPACING_M
    distance = abs(phase - round(phase)) * food.RAW_MYOMERE_SPACING_M
    return MYOSEPTUM_RELIEF_M * math.exp(-(distance / food.RAW_FASCIA_WIDTH_M) ** 2)


def fillet_slice(kit, name: str, index: int, size: float = 1) -> bpy.types.Object:
    width = (.040 + .0034 * math.sin(index * 1.7)) * size
    length = (.018 + .0013 * math.cos(index)) * size
    vertices, rows, faces, skin_flags = [], [], [], []
    for row in range(SLICE_ROWS + 1):
        t, ring = row / SLICE_ROWS, []
        for column in range(SLICE_COLUMNS):
            theta = column * 2 * math.pi / SLICE_COLUMNS
            c, s = math.cos(theta), math.sin(theta)
            x = width * .5 * math.copysign(abs(c) ** .60, c)
            x *= 1 - .07 * t + .022 * math.sin(theta * 3 + index)
            y = length * (t - .5) + .10 * x
            across = x / (width * .5)
            thickness = .75 + .25 * (1 - across * across) + .12 * across
            z = SLICE_HEIGHT_M * .5 * size * thickness * math.copysign(abs(s) ** .78, s)
            point = Vector((x, y, z))
            edge = abs(s) ** 3
            point.z += edge * .00010 * size * noise.noise(point * 750 + Vector((index, 2.1, 6.3)))
            point.z += edge * myoseptum_height(point, index) * size
            ring.append(len(vertices))
            vertices.append(point)
        rows.append(ring)
    for first, second in zip(rows, rows[1:]):
        for column in range(SLICE_COLUMNS):
            following = (column + 1) % SLICE_COLUMNS
            faces.append((first[column], first[following], second[following], second[column]))
            skin_flags.append(math.sin((column + .5) * 2 * math.pi / SLICE_COLUMNS) < -.55)
    # The cut ends are oblique; keep their boundary and seal with interior tissue.
    for cap, boundary in enumerate((rows[0], rows[-1])):
        first_new = len(vertices)
        faces += shapes.cut_food_patch(vertices, boundary, SEED + index * 2 + cap, normal_axis=1)
        mean_x = sum(vertices[i].x for i in boundary) / len(boundary)
        for point in vertices[first_new:]:
            point.y += .10 * (point.x - mean_x)
    obj = kit.mesh(name, vertices, faces, food.raw_mackerel_flesh("M_" + name + "Flesh", index))
    obj.data.materials.append(food.prepared_mackerel_skin("M_" + name + "Skin"))
    for face, skin in zip(obj.data.polygons, skin_flags):
        face.material_index = int(skin)
    shapes.closed_normals(obj)
    bevel = obj.modifiers.new("RawMackerelKnifeEdge", "BEVEL")
    bevel.width, bevel.segments, bevel.limit_method, bevel.angle_limit = .00025 * size, 3, "ANGLE", .65
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
    rng = random.Random(SEED)
    plate = shapes.vessel(kit, "OriginalRawMackerelOvalPlate", PLATE_PROFILE,
                          food.raw_fish_plate("M_RawMackerelOvalPlate"))
    for vertex in plate.data.vertices:
        vertex.co.x *= PLATE_X_SCALE
    kit.tag_coords(plate.data)
    shapes.closed_normals(plate)
    supports, parts = [shapes.world_surface(plate)], [plate]
    for index, (x, y, rotation) in enumerate(PLACEMENTS):
        obj = fillet_slice(kit, "RawMackerelSlice" + str(index), index)
        obj.rotation_euler = (rng.uniform(-.08, .08), rng.uniform(-.08, .08), rotation)
        if index in (1, 4):
            obj.rotation_euler.x += math.pi
        shapes.seat_on_surfaces(obj, x, y, supports, maximum_radius=.098)
        supports.append(shapes.world_surface(obj))
        parts.append(obj)
    serving = kit.join(parts, "SM_RawFishSlices", unwrap=False, smooth_angle=65)
    kit.pack_uvs(serving)
    bite = fillet_slice(kit, "RawMackerelEdibleSlice", 9, .78)
    bite.rotation_euler.y = .055
    portion = kit.join([bite], "SM_RawFishSlicesPortion", unwrap=False, smooth_angle=65)
    kit.pack_uvs(portion)
    for obj, role in ((serving, "serving"), (portion, "handheld_portion")):
        obj["food_original"], obj["food_source_only"] = True, True
        obj["food_item"], obj["food_role"] = "RawFishSlices", role
        obj["food_geometry_source_sha256"] = shapes.SOURCE_SHA256
    serving["food_slice_count"] = 6
    portion["food_grip_not_verified"] = True
    return [serving, portion]
