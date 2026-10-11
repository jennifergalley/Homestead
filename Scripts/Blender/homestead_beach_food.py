"""Original beach-meal geometry and shaders: blue mussels, laver, potato and crockery.

Usage: import homestead_beach_food as beach; beach.open_mussel(kit, "Name", seed, materials)
Kept apart from homestead_food_geometry/homestead_food_materials so the source hashes the
eleven frozen meal blends pin (checked by Tests\\BlenderPreparedFoodChecks) stay valid.
Recipes reload it explicitly in live Blender.

Real-object notes (Mytilus edulis, Porphyra laver, 1851 rural English crockery):
- Blue mussel: 5-7cm long, height ~0.5 x length, each valve 0.8-1.1cm deep, shell 0.6-1.2mm.
  Umbo (beak) at the pointed anterior tip; straight-ish ventral margin; dorsal margin rises to a
  shoulder ~60% along, posterior end broadly rounded. Exterior: glossy blue-black periostracum
  with concentric growth lines about the umbo, brown/olive young margins and a chalky violet
  eroded beak. Interior: porcelain white-blue with a dark violet-blue margin band. Cooked meat:
  plump apricot-orange (female) or cream (male) with a dark brown frilled mantle edge.
- Laver (Porphyra umbilicalis): a one-cell-thin ruffled purple-green blade; cooked it turns
  olive-black and slick, fried it crisps into crinkled green-black sheets with bronze edges.
- Crockery: honey-glazed cream-slip earthenware with an unglazed red foot, banded creamware
  ("mocha" style slip bands) and blue shell-edged pearlware, all common rural tableware c.1850.
"""
import hashlib
import math
import random
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import homestead_food_geometry as shapes
from homestead_materials import Graph

SOURCE_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()

MUSSEL_SIDES, MUSSEL_RINGS = 64, 9
MUSSEL_SHELL_THICKNESS_M = .0007
MEAT_SIDES = 52
MEAT_RING_STEPS = (.10, .22, .34, .46, .56, .64, .70, .75, .83, .91, .96)
MEAT_BODY_EXTENT = .72
MEAT_GAP_M = .00025
LAVER_THICKNESS_M = .00022
FRIED_LAVER_THICKNESS_M = .00030
PROFILE_SMOOTHING = 2


# ---------------------------------------------------------------- shared tools

def _attribute(mesh, name: str, values: list) -> None:
    attr = mesh.attributes.get(name) or mesh.attributes.new(name, "FLOAT_VECTOR", "POINT")
    attr.data.foreach_set("vector", [component for value in values for component in value])


def _object(kit, name: str, vertices: list, faces: list, materials: list,
            face_materials: list | None = None, attributes: dict | None = None):
    obj = kit.mesh(name, vertices, faces, materials[0])
    for material in materials[1:]:
        obj.data.materials.append(material)
    if face_materials:
        for face, index in zip(obj.data.polygons, face_materials):
            face.material_index = index
    for attr_name, values in (attributes or {}).items():
        _attribute(obj.data, attr_name, values)
    return obj


def _offset_coords(obj, offset: Vector) -> None:
    coords = obj.data.attributes["pcoord"]
    values = [0.0] * (len(coords.data) * 3)
    coords.data.foreach_get("vector", values)
    for index in range(0, len(values), 3):
        values[index] += offset.x
        values[index + 1] += offset.y
        values[index + 2] += offset.z
    coords.data.foreach_set("vector", values)


def merge(kit, parts: list, name: str):
    """Join helper objects (identity transforms) into one movable food object."""
    for part in parts:
        if part.location.length > 1e-9 or any(abs(v) > 1e-9 for v in part.rotation_euler):
            raise ValueError(part.name + " must be built in its group's local frame")
    kit._select_only(parts, parts[0])
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    return obj


def settle(obj, x: float, y: float, supports: list, rng: random.Random, maximum_radius: float,
           tries: int = 40, jitter: float = .008, max_tilt: float = .45, heading=None,
           sample_step: int = 4) -> None:
    """Rest ``obj`` like a dropped piece: of random headings/tilts/nudges, keep the one whose
    centroid ends lowest (it would slide into that pose), then seat it exactly."""
    every = [vertex.co.copy() for vertex in obj.data.vertices]
    points = every[::sample_step]
    best = None
    for attempt in range(tries):
        turn = rng.uniform(0, 2 * math.pi) if heading is None else heading + rng.uniform(-.35, .35)
        lean = rng.uniform(0, 2 * math.pi)
        rotation = (Matrix.Rotation(rng.uniform(0, max_tilt), 3, Vector((math.cos(lean), math.sin(lean), 0)))
                    @ Matrix.Rotation(turn, 3, "Z"))
        px, py = (x, y) if attempt == 0 else (x + rng.uniform(-jitter, jitter), y + rng.uniform(-jitter, jitter))
        if any(math.hypot(p.x + px, p.y + py) > maximum_radius * .995 for p in (rotation @ co for co in every)):
            continue
        lift, total, valid = None, 0.0, True
        for co in points:
            point = rotation @ co
            wx, wy = point.x + px, point.y + py
            if math.hypot(wx, wy) > maximum_radius:
                valid = False
                break
            supported = False
            for surface in supports:
                hit, _, _, _ = surface.ray_cast(Vector((wx, wy, 1)), Vector((0, 0, -1)), 1.0)
                if hit is not None:
                    supported = True
                    offset = hit.z - point.z
                    lift = offset if lift is None else max(lift, offset)
            if not supported:
                valid = False
                break
            total += point.z
        if not valid or lift is None:
            continue
        score = total / len(points) + lift + .4 * math.hypot(px - x, py - y)
        if best is None or score < best[0]:
            best = (score, rotation, px, py)
    if best is None:
        raise ValueError(obj.name + " found no supported resting pose")
    obj.rotation_euler = best[1].to_euler()
    shapes.seat_on_surfaces(obj, best[2], best[3], supports, maximum_radius)


def drape(obj, x: float, y: float, heading: float, supports: list, base_level: float | None,
          maximum_radius: float, reach: float = .0025, clearance: float = .00012,
          relief: float = 1.0, sag: float = .00003, iterations: int = 1600) -> None:
    """Lay a thin sheet over whatever is below it, like wet or crisp laver.

    Samples the highest support near each vertex, then solves a hanging membrane over those
    obstacles (Jacobi: h = max(support, neighbour mean - sag)), so the sheet bridges gaps and
    droops between bumps instead of stepping into cliffs. ``relief`` keeps that fraction of
    the sheet's own authored crumple on top. Baked into the mesh at the identity transform.
    """
    import numpy as np
    rotation = Matrix.Rotation(heading, 3, "Z")
    low = min(vertex.co.z for vertex in obj.data.vertices)
    flat = [(rotation @ Vector((v.co.x, v.co.y, 0))).xy for v in obj.data.vertices]
    for _ in range(40):
        if max((point + Vector((x, y))).length for point in flat) <= maximum_radius:
            break
        x, y = x * .93, y * .93
    ring = [Vector((math.cos(a), math.sin(a), 0)) * reach for a in
            (i * 2 * math.pi / 6 for i in range(6))] + [Vector()]
    keys, key_of, world = {}, [], []
    for vertex in obj.data.vertices:
        local = rotation @ Vector((vertex.co.x, vertex.co.y, 0))
        wx, wy = local.x + x, local.y + y
        if math.hypot(wx, wy) > maximum_radius:
            raise ValueError(obj.name + " drapes beyond its vessel's aperture")
        key = (round(wx, 7), round(wy, 7))
        key_of.append(keys.setdefault(key, len(keys)))
        world.append((wx, wy))
    support = np.full(len(keys), -1.0)
    for (wx, wy), index in keys.items():
        height = base_level if base_level is not None else -1.0
        for offset in ring:
            for surface in supports:
                hit, _, _, _ = surface.ray_cast(Vector((wx + offset.x, wy + offset.y, 1)),
                                                Vector((0, 0, -1)), 1.0)
                if hit is not None:
                    height = max(height, hit.z)
        if height < -.5:
            raise ValueError(obj.name + " has a vertex with nothing beneath it")
        support[index] = height
    pairs = {(min(a, b), max(a, b)) for a, b in
             ((key_of[e.vertices[0]], key_of[e.vertices[1]]) for e in obj.data.edges) if a != b}
    first = np.array([a for a, _ in pairs] + [b for _, b in pairs])
    second = np.array([b for _, b in pairs] + [a for a, _ in pairs])
    degree = np.bincount(first, minlength=len(keys)).astype(float)
    height = support.copy()
    for _ in range(iterations):
        total = np.bincount(first, weights=height[second], minlength=len(keys))
        height = np.maximum(support, total / np.maximum(degree, 1) - sag)
    for vertex, index, (wx, wy) in zip(obj.data.vertices, key_of, world):
        vertex.co = Vector((wx, wy, float(height[index]) + clearance + relief * (vertex.co.z - low)))
    obj.location, obj.rotation_euler = (0, 0, 0), (0, 0, 0)
    obj.data.update()
    shapes.closed_normals(obj)


def smooth_profile(profile: tuple, iterations: int = PROFILE_SMOOTHING) -> list:
    """Chaikin-rounded revolve profile; the two axis points stay on the axis."""
    points = [Vector(point) for point in profile]
    for _ in range(iterations):
        refined = [points[0]]
        for a, b in zip(points, points[1:]):
            refined += [a.lerp(b, .25), a.lerp(b, .75)]
        refined.append(points[-1])
        refined[1] = Vector((max(refined[1].x, 1e-4), refined[1].y))
        refined[-2] = Vector((max(refined[-2].x, 1e-4), refined[-2].y))
        points = refined
    points[0].x = points[-1].x = 0
    return [(point.x, point.y) for point in points]


def vessel(kit, name: str, profile: tuple, material, sides: int = 160,
           smoothing: int = PROFILE_SMOOTHING):
    """Smoothed revolve with a per-vertex ``vside`` (0 outside, 1 inside, 0.5 on the rim)."""
    smooth = smooth_profile(profile, smoothing)
    obj = shapes.vessel(kit, name, tuple(smooth), material, sides)
    apex = max(range(len(smooth)), key=lambda index: smooth[index][1])
    sides_values = []
    for row, (radius, _) in enumerate(smooth):
        side = 0.0 if row < apex else 1.0 if row > apex else .5
        sides_values += [(side, row / (len(smooth) - 1), 0.0)] * (sides if radius else 1)
    if len(sides_values) != len(obj.data.vertices):
        raise ValueError(name + " vessel rows do not match its vertices")
    _attribute(obj.data, "vside", sides_values)
    interior = [point for point in smooth[apex:]]
    return obj, interior


def fill_profile(interior: list, level: float, lift: float = .0004) -> tuple:
    """A liquid volume resting on a vessel's smoothed interior up to ``level``."""
    rim, floor = None, []
    for (r0, z0), (r1, z1) in zip(interior, interior[1:]):
        if rim is None and z0 >= level > z1:
            t = (level - z0) / (z1 - z0)
            rim = (r0 + (r1 - r0) * t - lift, level)
        if rim is not None and r1 > 1e-6 and z1 < level - lift:
            floor.append((r1 - lift * .5, z1 + lift))
    if rim is None:
        raise ValueError("Liquid level is not inside the vessel")
    floor = sorted(point for point in floor if point[0] < rim[0] - 1e-5)
    return tuple([(0, interior[-1][1] + lift)] + floor + [rim, (rim[0] * .55, level), (0, level)])


def outline_resample(points: list, count: int) -> list:
    lengths = [0.0]
    closed = points + points[:1]
    for a, b in zip(closed, closed[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    total, result, index = lengths[-1], [], 0
    for step in range(count):
        target = total * step / count
        while lengths[index + 1] < target:
            index += 1
        span = lengths[index + 1] - lengths[index] or 1
        result.append(closed[index].lerp(closed[index + 1], (target - lengths[index]) / span))
    return result


# --------------------------------------------------------------------- mussels

def mussel_outline(length: float, height: float, seed: int) -> list:
    """Closed tip-first outline: umbo at the origin, dorsal edge +Y, posterior toward +X."""
    rng = random.Random(seed)
    def dorsal(t):
        return .68 * height * t ** .45 * math.sqrt(max(0, 1 - t ** 3)) / .705
    def ventral(t):
        concave = .03 * height * math.sin(math.pi * t) * (1 - t) ** 1.5
        return -.32 * height * t ** .62 * math.sqrt(max(0, 1 - t ** 3.2)) / .69 + concave
    samples = 260
    params = [(1 - math.cos(math.pi * i / samples)) / 2 for i in range(samples + 1)]
    points = [Vector((t * length, dorsal(t))) for t in params]
    points += [Vector((t * length, ventral(t))) for t in reversed(params[1:-1])]
    phase = rng.uniform(0, 10)
    wobble = []
    for point in points:
        amount = 1 + .014 * noise.noise(Vector((point.x * 90 + phase, point.y * 90, seed * .37)))
        wobble.append(Vector((point.x * amount, point.y * amount)))
    return wobble


def _umbo_table(outline: list) -> list:
    table = sorted((math.atan2(p.y, p.x), p.length) for p in outline if p.length > 1e-6)
    return table


def _umbo_reach(table: list, angle: float) -> float:
    if angle <= table[0][0]:
        return table[0][1]
    for (a0, d0), (a1, d1) in zip(table, table[1:]):
        if a0 <= angle <= a1:
            return d0 + (d1 - d0) * (angle - a0) / ((a1 - a0) or 1)
    return table[-1][1]


def mussel_valve(kit, name: str, length: float, seed: int, exterior, interior):
    """One closed valve, cup opening +Z, umbo at the origin, rim on z = 0."""
    height = length * (.49 + .03 * math.sin(seed * 1.3))
    depth = length * (.165 + .012 * math.cos(seed * .9))
    outline = outline_resample(mussel_outline(length, height, seed), MUSSEL_SIDES)
    centre = Vector((.50 * length, .15 * height))
    table = _umbo_table(outline)
    vertices, coords, faces = [Vector((centre.x, centre.y, 0))], [], []
    rings = []
    for ring in range(1, MUSSEL_RINGS + 1):
        s = (ring / MUSSEL_RINGS) ** .78
        row = []
        for point in outline:
            xy = centre.lerp(point, s)
            row.append(len(vertices))
            vertices.append(Vector((xy.x, xy.y, 0)))
        rings.append(row)
    for vertex_index, vertex in enumerate(vertices):
        xy = vertex.xy
        s = 0 if vertex_index == 0 else (((vertex_index - 1) // MUSSEL_SIDES + 1) / MUSSEL_RINGS) ** .78
        dome = (1 - s ** 2.4) ** .55
        keel = 1 + .10 * (xy.y - centre.y) / height
        beak = .30 * math.exp(-(xy.length / (.20 * length)) ** 2) * (1 - s ** 8)
        vertex.z = -depth * (dome * keel + beak)
        vertex.z += .00012 * noise.noise(Vector((xy.x * 260, xy.y * 260, seed)))
        reach = _umbo_reach(table, math.atan2(xy.y, xy.x)) if xy.length > 1e-6 else 1
        growth = min(1.0, xy.length / max(reach, 1e-6))
        coords.append((growth, math.atan2(xy.y, xy.x) / math.pi if xy.length > 1e-6 else 0.0, (seed % 17) / 17))
    for index in range(MUSSEL_SIDES):
        following = (index + 1) % MUSSEL_SIDES
        faces.append((0, rings[0][following], rings[0][index]))
    for first, second in zip(rings, rings[1:]):
        for index in range(MUSSEL_SIDES):
            following = (index + 1) % MUSSEL_SIDES
            faces.append((first[index], first[following], second[following], second[index]))
    obj = _object(kit, name, vertices, faces, [exterior, interior],
                  attributes={"shellcoord": coords})
    mesh = obj.data
    mesh.update()
    if sum(face.normal.z for face in mesh.polygons) > 0:
        mesh.flip_normals()
    solid = obj.modifiers.new("MusselShell", "SOLIDIFY")
    solid.thickness, solid.offset = MUSSEL_SHELL_THICKNESS_M, -1
    solid.use_even_offset, solid.use_rim, solid.use_rim_only = True, True, False
    solid.material_offset, solid.material_offset_rim = 1, 0
    kit.apply_modifiers(obj)
    shapes.closed_normals(obj)
    obj["mussel_length"], obj["mussel_height"], obj["mussel_depth"] = length, height, depth
    obj["mussel_centre"] = (centre.x, centre.y)
    return obj


def meat_in_valve(kit, name: str, valve, seed: int, material, plump: float = 1.0,
                  frill_reach: float = .87):
    """Cooked meat moulded onto the valve's interior, mantle frill on its rim.

    Frill and lump frequencies stay well under MEAT_SIDES / 4 so the ring samples them
    smoothly (higher harmonics alias into spiky triangles along the mantle edge)."""
    length, height = valve["mussel_length"], valve["mussel_height"]
    centre = Vector(valve["mussel_centre"])
    surface = BVHTree.FromPolygons([v.co for v in valve.data.vertices],
                                   [tuple(f.vertices) for f in valve.data.polygons])
    outline = outline_resample(mussel_outline(length, height, valve.get("mussel_seed", seed)), MEAT_SIDES)
    axis = Vector((1, .12)).normalized()
    thickness = valve["mussel_depth"] * 1.22 * plump
    rng = random.Random(seed)
    phase = rng.uniform(0, 6.28)

    def interior_z(x, y):
        hit, _, _, _ = surface.ray_cast(Vector((x, y, .05)), Vector((0, 0, -1)), 1.0)
        if hit is None:
            raise ValueError(name + " mantle extends beyond its valve")
        return hit.z + MEAT_GAP_M

    vertices, coords, faces = [], [], []
    rim = []
    for index, point in enumerate(outline):
        angle = index * 2 * math.pi / MEAT_SIDES
        frill = frill_reach + .014 * math.sin(angle * 11 + phase) + .008 * math.sin(angle * 5 + phase * 2)
        xy = centre.lerp(point, frill)
        z = interior_z(xy.x, xy.y) + .00025 * (1 + math.sin(angle * 9 + phase))
        rim.append(len(vertices))
        vertices.append(Vector((xy.x, xy.y, z)))
        coords.append((1.0, 0.0, .5))
    groove_depth, groove_width = .0011, .0038

    def lump(xy):
        return 1 + .07 * noise.noise(Vector((xy.x * 140, xy.y * 140, seed * .71)))

    for layer in ("top", "bottom"):
        centre_index = len(vertices)
        z = interior_z(centre.x, centre.y)
        if layer == "top":
            z += max(.0009, thickness * lump(centre) - groove_depth * plump)
        vertices.append(Vector((centre.x, centre.y, z)))
        coords.append((0.0, 0.0, 1.0 if layer == "top" else 0.0))
        rings = [[centre_index] * MEAT_SIDES]
        for s in MEAT_RING_STEPS:
            row = []
            for index, rim_index in enumerate(rim):
                xy = centre.lerp(vertices[rim_index].xy, s)
                base = interior_z(xy.x, xy.y)
                if layer == "top":
                    angle = index * 2 * math.pi / MEAT_SIDES
                    if s < MEAT_BODY_EXTENT:
                        offset = xy - centre
                        across = offset.x * -axis.y + offset.y * axis.x
                        groove = (groove_depth * math.exp(-(across / groove_width) ** 2)
                                  * (1 - (s / MEAT_BODY_EXTENT) ** 2))
                        body = (1 - (s / MEAT_BODY_EXTENT) ** 2.2) ** .42
                        base += max(.0009, thickness * body * lump(xy) - groove * plump)
                    else:
                        reach = (s - MEAT_BODY_EXTENT) / (1 - MEAT_BODY_EXTENT)
                        ruffle = .00042 * (1 + math.sin(angle * 7 + phase + reach * 2)) * reach
                        base += .0008 * (1 - reach) + ruffle
                row.append(len(vertices))
                vertices.append(Vector((xy.x, xy.y, base)))
                coords.append((s, (xy - centre).dot(Vector((-axis.y, axis.x))) / height,
                               1.0 if layer == "top" else 0.0))
            rings.append(row)
        rings.append(rim)
        for first, second in zip(rings, rings[1:]):
            for index in range(MEAT_SIDES):
                following = (index + 1) % MEAT_SIDES
                if first[index] == first[following]:
                    face = (first[index], second[index], second[following])
                else:
                    face = (first[index], second[index], second[following], first[following])
                faces.append(face if layer == "top" else tuple(reversed(face)))
    obj = _object(kit, name, vertices, faces, [material], attributes={"meatcoord": coords})
    shapes.closed_normals(obj)
    return obj


def open_mussel(kit, name: str, seed: int, materials: dict, length: float,
                gape_degrees: float, meat: bool = True, plump: float = 1.0):
    """Valve A cup-up holding the meat, valve B hinged open about the dorsal ligament."""
    valve = mussel_valve(kit, name + "ValveA", length, seed, materials["shell"], materials["nacre"])
    valve["mussel_seed"] = seed
    parts = [valve]
    if meat:
        parts.append(meat_in_valve(kit, name + "Meat", valve, seed, materials["meat"], plump))
    other = valve.copy()
    other.data = valve.data.copy()
    other.name = name + "ValveB"
    bpy.context.scene.collection.objects.link(other)
    height = valve["mussel_height"]
    hinge_end = Vector((.36 * length, .68 * height * .36 ** .45 * math.sqrt(1 - .36 ** 3) / .705, 0))
    axis = hinge_end.normalized()
    transform = Matrix.Rotation(-math.radians(gape_degrees), 4, axis) @ Matrix.Scale(-1, 4, Vector((0, 0, 1)))
    other.data.transform(transform)
    other.data.flip_normals()
    _offset_coords(other, Vector((.013, .021, .007 + seed * .0003)))
    shapes.closed_normals(other)
    parts.append(other)
    obj = merge(kit, parts, name)
    obj["food_components"] = len(parts)
    return obj


def loose_meat(kit, name: str, seed: int, material, length: float, plump: float = 1.0):
    """A shelled meat keeps the convex underside its valve gave it; the valve is discarded."""
    valve = mussel_valve(kit, name + "Mould", length, seed, material, material)
    valve["mussel_seed"] = seed
    meat = meat_in_valve(kit, name, valve, seed, material, plump, frill_reach=.80)
    data = valve.data
    bpy.data.objects.remove(valve)
    bpy.data.meshes.remove(data)
    xs = [vertex.co.x for vertex in meat.data.vertices]
    ys = [vertex.co.y for vertex in meat.data.vertices]
    low = min(vertex.co.z for vertex in meat.data.vertices)
    meat.data.transform(Matrix.Translation((-(min(xs) + max(xs)) / 2, -(min(ys) + max(ys)) / 2, -low)))
    _offset_coords(meat, Vector(((seed * .0613) % 1, (seed * .0291) % 1, (seed * .0457) % 1)))
    return meat


def float_plane(height: float, radius: float, sides: int = 96) -> BVHTree:
    """A flat support disc: pieces rest on it as if buoyed at that depth in a broth."""
    vertices = [Vector((0, 0, height))] + [
        Vector((radius * math.cos(i * 2 * math.pi / sides), radius * math.sin(i * 2 * math.pi / sides), height))
        for i in range(sides)]
    return BVHTree.FromPolygons(vertices, [(0, 1 + i, 1 + (i + 1) % sides) for i in range(sides)])


# ------------------------------------------------------------- potato & laver

def potato_chunk(kit, name: str, index: int, material, size: float = 1.0, grid: int = 9):
    """Boiled potato chunk: rounded superellipsoid cube, softened and crumbled."""
    rng = random.Random(7301 + index)
    dims = Vector((rng.uniform(.016, .021), rng.uniform(.015, .019), rng.uniform(.013, .017))) * size
    exponent = rng.uniform(2.8, 3.8)
    vertices, faces, lookup = [], [], {}
    for axis in range(3):
        others = [value for value in range(3) if value != axis]
        for sign in (-1, 1):
            rows = []
            for row in range(grid + 1):
                line = []
                for column in range(grid + 1):
                    key = [0, 0, 0]
                    key[axis] = sign * grid
                    key[others[0]], key[others[1]] = 2 * row - grid, 2 * column - grid
                    key = tuple(key)
                    if key not in lookup:
                        cube = Vector(tuple(value / grid for value in key))
                        norm = sum(abs(value) ** exponent for value in cube) ** (1 / exponent)
                        point = Vector(tuple(cube[i] / norm * dims[i] * .5 for i in range(3)))
                        point *= 1 + .085 * noise.noise(point * 120 + Vector((index, 2.3, 5.1)))
                        point *= 1 + .022 * noise.noise(point * 700 + Vector((index, 7.7, 1.9)))
                        lookup[key] = len(vertices)
                        vertices.append(point)
                    line.append(lookup[key])
                rows.append(line)
            for first, second in zip(rows, rows[1:]):
                for column in range(grid):
                    faces.append((first[column], first[column + 1], second[column + 1], second[column]))
    obj = _object(kit, name, vertices, faces, [material])
    shapes.closed_normals(obj)
    return obj


def laver_ribbon(kit, name: str, index: int, material, length: float, width: float,
                 rows: int = 36, columns: int = 8):
    """Cooked laver: a limp, torn, one-cell-thin blade, ruffled right across its width."""
    rng = random.Random(9101 + index)
    phase, twist = rng.uniform(0, 6.28), rng.uniform(-.6, .6)
    top, bottom, coords = [], [], []
    for row in range(rows + 1):
        t = row / rows
        taper = (.55 + .45 * math.sin(math.pi * (.04 + .92 * t)) ** .5
                 + .22 * noise.noise(Vector((t * 3.1, phase, index * .7))))
        shift = .25 * width * noise.noise(Vector((t * 2.3, phase + 4, index * .3)))
        for column in range(columns + 1):
            u = 2 * column / columns - 1
            x, y = u * width * .5 * max(.25, taper) + shift, (t - .5) * length
            ruffle = (.0013 * math.sin(y / .0021 + phase + 2.4 * u) * (.35 + .65 * abs(u) ** 1.1)
                      + .0006 * math.sin(y / .0011 + phase * 2 - 3.1 * u) * abs(u))
            drape = -.0030 * (2 * t - 1) ** 2 + .0012 * math.sin(t * 5 + phase)
            angle = twist * (t - .5)
            z = drape + ruffle + x * math.sin(angle)
            x *= math.cos(angle)
            top.append(Vector((x, y, z + LAVER_THICKNESS_M)))
            bottom.append(Vector((x, y, z)))
            coords.append((abs(u), t, index * .13))
    stride, layer = columns + 1, (columns + 1) * (rows + 1)
    vertices = top + bottom
    faces = []
    for row in range(rows):
        for column in range(columns):
            a = row * stride + column
            face = (a, a + 1, a + stride + 1, a + stride)
            faces.append(face)
            faces.append(tuple(reversed(tuple(value + layer for value in face))))
    boundary = (list(range(stride)) + [row * stride + columns for row in range(1, rows + 1)] +
                [rows * stride + column for column in range(columns - 1, -1, -1)] +
                [row * stride for row in range(rows - 1, 0, -1)])
    for a, b in zip(boundary, boundary[1:] + boundary[:1]):
        faces.append((b, a, a + layer, b + layer))
    obj = _object(kit, name, vertices, faces, [material], attributes={"lavercoord": coords + coords})
    shapes.closed_normals(obj)
    return obj


def fried_laver_sheet(kit, name: str, index: int, material, radius: float,
                      rings: int = 20, sides: int = 72):
    """Pan-crisped laver: a crumpled, edge-curled sheet with a torn outline."""
    rng = random.Random(11701 + index)
    phase = rng.uniform(0, 50)
    elongation = rng.uniform(1.0, 1.6)
    breaks = sorted(rng.uniform(0, 2 * math.pi) for _ in range(rng.randint(7, 11)))
    spans = [(angle, rng.uniform(.62, 1.12)) for angle in breaks]
    spans = [(spans[-1][0] - 2 * math.pi, spans[-1][1])] + spans + [(spans[0][0] + 2 * math.pi, spans[0][1])]

    def fracture(angle: float) -> float:
        for (a0, r0), (a1, r1) in zip(spans, spans[1:]):
            if a0 <= angle <= a1:
                return r0 + (r1 - r0) * (angle - a0) / ((a1 - a0) or 1)
        return 1.0

    outline = []
    for side in range(sides):
        angle = side * 2 * math.pi / sides
        reach = radius * fracture(angle) * (
            1 + .06 * noise.noise(Vector((math.cos(angle) * 1.7, math.sin(angle) * 1.7, phase)))
            + .035 * noise.noise(Vector((math.cos(angle) * 9, math.sin(angle) * 9, phase + 3))))
        outline.append(Vector((math.cos(angle) * reach * elongation, math.sin(angle) * reach)))
    size = min(1.0, radius / .026)
    creases = []
    for _ in range(15):
        origin = Vector((rng.uniform(-.7, .7) * radius, rng.uniform(-.7, .7) * radius))
        angle = rng.uniform(0, math.pi)
        creases.append((origin, Vector((math.cos(angle), math.sin(angle))),
                        rng.uniform(.0007, .0020) * size * rng.choice((-1, 1)),
                        rng.uniform(.0013, .0030) * max(size, .45)))

    def height(xy: Vector, s: float) -> float:
        z = 0.0
        for origin, direction, amplitude, width in creases:
            distance = (xy - origin).cross(direction)
            z += amplitude * math.exp(-(distance / width) ** 2)
        z += .0011 * size * noise.noise(Vector((xy.x * 150, xy.y * 150, phase)))
        z += .00035 * noise.noise(Vector((xy.x * 520, xy.y * 520, phase + 9)))
        z += .00012 * noise.noise(Vector((xy.x * 1300, xy.y * 1300, phase + 17)))
        curl = .45 + 1.1 * noise.noise(Vector((xy.x * 70, xy.y * 70, phase + 4)))
        z += .0040 * size * s ** 2.5 * curl
        return z

    top, coords = [], []
    centre_top = Vector((0, 0, height(Vector((0, 0)), 0)))
    top.append(centre_top)
    coords.append((0.0, 0.0, index * .11))
    for ring in range(1, rings + 1):
        s = ring / rings
        for point in outline:
            xy = point * s
            top.append(Vector((xy.x, xy.y, height(xy, s))))
            coords.append((s, xy.length / radius, index * .11))
    # Seat the centre and ease the first rings toward their mean height so the thin triangle
    # fan there doesn't shade as a pinched star.
    first_ring = top[1:1 + sides]
    mean = sum(point.z for point in first_ring) / sides
    top[0].z = mean
    for ring, weight in ((1, 1.0), (2, .6), (3, .25)):
        if ring > rings:
            break
        row = top[1 + (ring - 1) * sides:1 + ring * sides]
        ring_mean = sum(point.z for point in row) / sides
        for point in row:
            point.z += (mean + (ring_mean - mean) * (1 - weight) - point.z) * weight
    count = len(top)
    bottom = [point - Vector((0, 0, FRIED_LAVER_THICKNESS_M)) for point in top]
    vertices = top + bottom
    faces = []
    for layer in (0, 1):
        base = layer * count
        for side in range(sides):
            following = (side + 1) % sides
            face = (base, base + 1 + side, base + 1 + following)
            faces.append(face if layer == 0 else tuple(reversed(face)))
        for ring in range(rings - 1):
            first, second = 1 + ring * sides, 1 + (ring + 1) * sides
            for side in range(sides):
                following = (side + 1) % sides
                face = (base + first + side, base + second + side,
                        base + second + following, base + first + following)
                faces.append(face if layer == 0 else tuple(reversed(face)))
    edge = 1 + (rings - 1) * sides
    for side in range(sides):
        following = (side + 1) % sides
        a, b = edge + side, edge + following
        faces.append((a, a + count, b + count, b))
    obj = _object(kit, name, vertices, faces, [material], attributes={"lavercoord": coords + coords})
    shapes.closed_normals(obj)
    return obj


def broth_lens(kit, name: str, spoon, material, level_drop: float = .0009,
               rings: int = 8, sides: int = 48):
    """Liquid filling a spoon hollow to a meniscus just below its rim."""
    surface = BVHTree.FromPolygons([v.co for v in spoon.data.vertices],
                                   [tuple(f.vertices) for f in spoon.data.polygons])
    ys = [v.co.y for v in spoon.data.vertices]
    tip = min(ys)
    centre = Vector((0, tip + .0275 * (max(ys) - tip) / .182))

    def floor(x, y):
        hit, _, _, _ = surface.ray_cast(Vector((x, y, .1)), Vector((0, 0, -1)), 1)
        return hit.z if hit is not None else None

    centre_floor = floor(centre.x, centre.y)
    rim = max(v.co.z for v in spoon.data.vertices if abs(v.co.y - centre.y) < .004 and abs(v.co.x) > .008)
    level = rim - level_drop
    boundary = []
    for side in range(sides):
        angle = side * 2 * math.pi / sides
        direction = Vector((math.cos(angle) * .5, math.sin(angle)))
        low, high = 0.0, .04
        for _ in range(30):
            mid = (low + high) / 2
            point = centre + direction * mid
            z = floor(point.x, point.y)
            if z is not None and z < level - .00005:
                low = mid
            else:
                high = mid
        point = centre + direction * low * .997
        boundary.append(Vector((point.x, point.y, max(level, floor(point.x, point.y) + .00005))))
    vertices = [Vector((centre.x, centre.y, level)), Vector((centre.x, centre.y, centre_floor + .00015))]
    top_rows, bottom_rows = [[0] * sides], [[1] * sides]
    for ring in range(1, rings):
        s = ring / rings
        top_row, bottom_row = [], []
        for point in boundary:
            xy = centre.lerp(point.xy, s)
            meniscus = .00025 * (s ** 6)
            top_row.append(len(vertices))
            vertices.append(Vector((xy.x, xy.y, level + meniscus)))
            bottom_row.append(len(vertices))
            vertices.append(Vector((xy.x, xy.y, floor(xy.x, xy.y) + .00015)))
        top_rows.append(top_row)
        bottom_rows.append(bottom_row)
    rim_row = []
    for point in boundary:
        rim_row.append(len(vertices))
        vertices.append(point)
    top_rows.append(rim_row)
    bottom_rows.append(rim_row)
    faces = []
    for rows, flip in ((top_rows, False), (bottom_rows, True)):
        for first, second in zip(rows, rows[1:]):
            for side in range(sides):
                following = (side + 1) % sides
                if first[side] == first[following]:
                    face = (first[side], second[side], second[following])
                else:
                    face = (first[side], second[side], second[following], first[following])
                faces.append(tuple(reversed(face)) if flip else face)
    obj = _object(kit, name, vertices, faces, [material])
    shapes.closed_normals(obj)
    return obj, level


# ------------------------------------------------------------------- materials

def _graph(name: str) -> Graph:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    return graph


def _attr(graph: Graph, name: str):
    return graph.node("ShaderNodeAttribute", attribute_name=name, attribute_type="GEOMETRY").outputs["Vector"]


def mussel_shell(name: str) -> bpy.types.Material:
    """Wet blue-black periostracum: irregular growth checks, violet-blue rays, olive margin, eroded beak."""
    graph = _graph(name)
    point = graph.coord()
    growth, angle, variant = graph.separate(_attr(graph, "shellcoord"))
    wobble = graph.noise(point, scale=150, detail=2).outputs["Fac"]
    ripple = graph.noise(graph.combine(graph.math("MULTIPLY", growth, 60),
                                       graph.math("MULTIPLY", angle, 3), graph.math("MULTIPLY", variant, 30)),
                         scale=1.0, detail=3, roughness=.6).outputs["Fac"]
    phase = graph.math("ADD", graph.math("MULTIPLY", growth, 46), graph.math("MULTIPLY", wobble, 1.6))
    lines = graph.math("MULTIPLY", graph.math("SINE", graph.math("MULTIPLY", phase, 6.2832)),
                       graph.remap(ripple, .35, .65, .15, 1))
    seasons = graph.noise(graph.combine(graph.math("MULTIPLY", growth, 9),
                                        graph.math("MULTIPLY", variant, 40), 0),
                          scale=1.6, detail=2).outputs["Fac"]
    checks = graph.math("FRACT", graph.math("ADD", graph.math("MULTIPLY", growth, 6.5),
                                            graph.math("MULTIPLY", seasons, 2.2)))
    colour = graph.ramp(seasons, [
        (.25, (.0075, .0085, .015)), (.50, (.014, .016, .028)),
        (.70, (.024, .026, .042)), (.85, (.034, .032, .044)),
    ], "EASE")
    rays = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 26), graph.math("MULTIPLY", variant, 50),
                                     graph.math("MULTIPLY", growth, .8)), scale=1.0, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.030, .034, .075), graph.math("MULTIPLY", graph.remap(rays, .55, .72, 0, .55),
                                                               graph.remap(growth, .35, .60)))
    olive = graph.remap(growth, .80, 1.0, 0, .60)
    olive = graph.math("MULTIPLY", olive, graph.remap(seasons, .35, .70, .4, 1))
    colour = graph.mix(colour, (.040, .032, .015), olive)
    colour = graph.mix(colour, (.005, .005, .009), graph.remap(checks, .82, 1.0, 0, .55))
    erosion_noise = graph.noise(point, scale=160, detail=3).outputs["Fac"]
    eroded = graph.remap(graph.math("ADD", growth, graph.math("MULTIPLY", erosion_noise, .25)),
                         .26, .38, 1, 0)
    chalk = graph.ramp(erosion_noise, [(.35, (.075, .062, .105)), (.65, (.15, .13, .19))])
    graph.set("Base Color", graph.mix(colour, chalk, eroded))
    gloss = graph.noise(point, scale=95, detail=2).outputs["Fac"]
    rough = graph.remap(gloss, .3, .7, .34, .52)
    rough = graph.mix(rough, (.60, .60, .60), graph.remap(checks, .80, 1.0, 0, .8))
    graph.set("Roughness", graph.mix(rough, (.62, .62, .62), eroded))
    graph.set("Coat Weight", .18)
    graph.set("Coat Roughness", .22)
    fines = graph.noise(point, scale=1900, detail=2).outputs["Fac"]
    step = graph.remap(checks, .70, 1.0)
    relief = graph.math("ADD", graph.math("MULTIPLY", lines, .30), graph.math("MULTIPLY", fines, .25))
    relief = graph.math("ADD", relief, graph.math("MULTIPLY", step, 1.2))
    graph.set("Normal", graph.bump(relief, strength=.65, distance=.00015))
    return graph.mat


def mussel_nacre(name: str) -> bpy.types.Material:
    """Chalky pearly interior: milky blue-white, a grey-blue flush, a dark violet margin band.

    Cooked nacre is satin, not mirror: roughness ~0.4-0.55 and only a whisper of coat, so
    it doesn't throw back the sky as a saturated blue."""
    graph = _graph(name)
    point = graph.coord()
    growth, angle, _ = graph.separate(_attr(graph, "shellcoord"))
    milk = graph.noise(point, scale=55, detail=2).outputs["Fac"]
    sheen = graph.noise(graph.combine(graph.math("MULTIPLY", growth, 14), graph.math("MULTIPLY", angle, 6), 0),
                        scale=1.0, detail=3).outputs["Fac"]
    colour = graph.ramp(milk, [
        (.30, (.38, .385, .39)), (.55, (.43, .43, .435)), (.80, (.47, .465, .47)),
    ], "EASE")
    colour = graph.mix(colour, (.31, .32, .36), graph.remap(sheen, .45, .70, 0, .40))
    colour = graph.mix(colour, (.41, .385, .395), graph.remap(sheen, .30, .18, 0, .40))
    chalk = graph.noise(point, scale=320, detail=3).outputs["Fac"]
    colour = graph.mix(colour, (.48, .47, .46), graph.remap(chalk, .55, .75, 0, .35))
    edge_noise = graph.noise(point, scale=120, detail=1).outputs["Fac"]
    flush = graph.remap(graph.math("ADD", growth, graph.math("MULTIPLY", edge_noise, .08)), .68, .91)
    colour = graph.mix(colour, (.25, .24, .32), graph.math("MULTIPLY", flush, .55))
    margin = graph.remap(graph.math("ADD", growth, graph.math("MULTIPLY", edge_noise, .03)), .895, .940)
    violet = graph.ramp(edge_noise, [(.35, (.040, .030, .056)), (.65, (.066, .048, .084))])
    colour = graph.mix(colour, violet, margin)
    scar_centre = graph.vmath("LENGTH", graph.combine(graph.math("SUBTRACT", growth, .62),
                                                      graph.math("MULTIPLY", graph.math("SUBTRACT", angle, .06), 2.2), 0))
    scar = graph.remap(scar_centre, .10, .07, 0, .55)
    colour = graph.mix(colour, (.50, .47, .47), scar)
    graph.set("Base Color", colour)
    rough = graph.remap(milk, .3, .7, .40, .52)
    graph.set("Roughness", graph.mix(rough, (.58, .58, .58), graph.remap(chalk, .55, .75, 0, .8)))
    graph.set("Coat Weight", .05)
    graph.set("Coat Roughness", .30)
    fines = graph.noise(point, scale=1500, detail=2).outputs["Fac"]
    graph.set("Normal", graph.bump(graph.math("ADD", fines, graph.math("MULTIPLY", sheen, .4)),
                                   strength=.10, distance=.00002))
    return graph.mat


def steamed_mussel_meat(name: str, pale: bool = False) -> bpy.types.Material:
    """Steamed meat: apricot (or cream) body, dark brown frilled mantle margin."""
    graph = _graph(name)
    point = graph.coord()
    s, across, top = graph.separate(_attr(graph, "meatcoord"))
    tissue = graph.noise(point, scale=150, detail=2).outputs["Fac"]
    if pale:
        stops = [(.25, (.40, .30, .16)), (.55, (.48, .38, .22)), (.85, (.54, .45, .28))]
    else:
        stops = [(.25, (.36, .10, .026)), (.55, (.47, .165, .043)), (.85, (.55, .235, .070))]
    colour = graph.ramp(tissue, stops, "EASE")
    blotch = graph.noise(point, scale=48, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.60, .34, .12) if not pale else (.58, .50, .33),
                       graph.remap(blotch, .52, .70, 0, .45))
    colour = graph.mix(colour, (.26, .060, .018) if not pale else (.33, .23, .12),
                       graph.remap(blotch, .40, .28, 0, .40))
    frill_noise = graph.noise(point, scale=600, detail=2).outputs["Fac"]
    mantle = graph.remap(graph.math("ADD", s, graph.math("MULTIPLY", frill_noise, .06)), .70, .78)
    mantle_colour = graph.ramp(frill_noise, [(.35, (.030, .016, .009)), (.65, (.070, .040, .020))])
    colour = graph.mix(colour, mantle_colour, mantle)
    seam = graph.remap(graph.math("ABSOLUTE", across), .0, .05, .40, 0)
    colour = graph.mix(colour, (.22, .075, .022) if not pale else (.32, .22, .11),
                       graph.math("MULTIPLY", seam, top))
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(tissue, .3, .7, .40, .52))
    graph.set("Subsurface Weight", .12)
    graph.set("Subsurface Radius", (.0016, .0007, .0003))
    graph.set("Subsurface Scale", .05)
    wrinkles = graph.noise(graph.scale(point, (1, .30, 1)), scale=520, detail=3).outputs["Fac"]
    graph.set("Normal", graph.bump(wrinkles, strength=.18, distance=.00006))
    return graph.mat


def mussel_liquor(name: str) -> bpy.types.Material:
    graph = _graph(name)
    variation = graph.noise(graph.coord(), scale=140, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(variation, [
        (.25, (.24, .205, .13)), (.75, (.31, .27, .175)),
    ]))
    graph.set("Roughness", .07)
    graph.set("IOR", 1.335)
    graph.set("Transmission Weight", .12)
    graph.set("Normal", graph.bump(variation, strength=.05, distance=.00002))
    return graph.mat


def mussel_broth(name: str, thin: bool = False) -> bpy.types.Material:
    """Cloudy potato-starched mussel broth with fine laver flecks (``thin`` for a spoonful)."""
    graph = _graph(name)
    point = graph.coord()
    cloud = graph.noise(point, scale=85, detail=3).outputs["Fac"]
    colour = graph.ramp(cloud, [
        (.25, (.20, .17, .10)), (.55, (.26, .225, .135)), (.85, (.30, .26, .165)),
    ], "EASE")
    flecks = graph.voronoi(point, scale=420).outputs["Distance"]
    fleck = graph.remap(flecks, .05, .14, .85, 0)
    fleck = graph.math("MULTIPLY", fleck, graph.remap(
        graph.noise(point, scale=60, detail=1).outputs["Fac"], .45, .62))
    graph.set("Base Color", graph.mix(colour, (.018, .024, .012), fleck))
    graph.set("Roughness", .07)
    graph.set("IOR", 1.335)
    graph.set("Transmission Weight", .75 if thin else .12)
    graph.set("Normal", graph.bump(cloud, strength=.04, distance=.00002))
    return graph.mat


def boiled_potato(name: str) -> bpy.types.Material:
    graph = _graph(name)
    point = graph.coord()
    crumb = graph.noise(point, scale=280, detail=3).outputs["Fac"]
    colour = graph.ramp(crumb, [
        (.15, (.40, .31, .12)), (.5, (.52, .41, .19)), (.85, (.60, .49, .26)),
    ], "EASE")
    stain = graph.noise(point, scale=70, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.33, .26, .11), graph.remap(stain, .55, .75, 0, .5))
    graph.set("Base Color", graph.mix(colour, (.24, .21, .12), graph.remap(stain, .32, .20, 0, .35)))
    graph.set("Roughness", graph.remap(crumb, .2, .8, .52, .34))
    graph.set("Subsurface Weight", .14)
    graph.set("Subsurface Radius", (.0028, .0018, .0008))
    graph.set("Subsurface Scale", .05)
    pores = graph.noise(point, scale=1700, detail=3).outputs["Fac"]
    graph.set("Normal", graph.bump(pores, strength=.30, distance=.00012))
    return graph.mat


def cooked_laver(name: str) -> bpy.types.Material:
    """Slick simmered laver: olive-black with purple-red undertones, thin translucent edges."""
    graph = _graph(name)
    point = graph.coord()
    edge, _, _ = graph.separate(_attr(graph, "lavercoord"))
    pigment = graph.noise(point, scale=210, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.20, (.008, .009, .005)), (.50, (.016, .018, .009)), (.80, (.028, .017, .022)),
    ], "EASE")
    graph.set("Base Color", graph.mix(colour, (.034, .036, .014), graph.remap(edge, .70, 1.0, 0, .55)))
    graph.set("Roughness", graph.remap(pigment, .2, .8, .16, .26))
    graph.set("Subsurface Weight", .10)
    graph.set("Subsurface Radius", (.0006, .0010, .0004))
    graph.set("Subsurface Scale", .05)
    membrane = graph.noise(graph.scale(point, (1, .4, 1)), scale=1400, detail=3).outputs["Fac"]
    graph.set("Normal", graph.bump(membrane, strength=.18, distance=.00003))
    return graph.mat


def fried_laver(name: str) -> bpy.types.Material:
    """Pan-crisped laver: green-black with olive-bronze toasted patches and crisped margins,
    glossy oil caught in the crinkles and lighter, see-through under-fried thin bits."""
    graph = _graph(name)
    point = graph.coord()
    edge, _, variant = graph.separate(_attr(graph, "lavercoord"))
    point = graph.vmath("ADD", point, graph.combine(graph.math("MULTIPLY", variant, 7.3),
                                                    graph.math("MULTIPLY", variant, 3.1), 0))
    pigment = graph.noise(point, scale=170, detail=4).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.20, (.0045, .0065, .0030)), (.50, (.0095, .0140, .0058)), (.80, (.017, .023, .0090)),
    ], "EASE")
    tint = graph.noise(point, scale=45, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.014, .009, .011), graph.remap(tint, .55, .72, 0, .50))
    # Broad toasted patches where the sheet lay flat on the pan: olive to bronze-brown.
    toast = graph.noise(point, scale=24, detail=3, roughness=.55).outputs["Fac"]
    toast_colour = graph.ramp(graph.noise(point, scale=70, detail=2).outputs["Fac"], [
        (.35, (.034, .034, .012)), (.65, (.052, .036, .014)),
    ], "EASE")
    colour = graph.mix(colour, toast_colour, graph.remap(toast, .50, .70, 0, .85))
    # Thin, under-fried bits: lighter green and see-through against the plate.
    thin_noise = graph.noise(point, scale=62, detail=3).outputs["Fac"]
    thin = graph.math("MULTIPLY", graph.remap(thin_noise, .56, .68),
                      graph.remap(edge, .10, .45))
    colour = graph.mix(colour, (.042, .068, .024), graph.math("MULTIPLY", thin, .9))
    # Crisped margins: olive-brown, burnt toward the very edge.
    scorch_noise = graph.noise(point, scale=95, detail=3).outputs["Fac"]
    crisp = graph.remap(graph.math("ADD", edge, graph.math("MULTIPLY", scorch_noise, .22)), .88, 1.08, 0, .8)
    patchy = graph.remap(graph.noise(point, scale=18, detail=2).outputs["Fac"], .38, .62, .2, 1)
    crisp = graph.math("MULTIPLY", crisp, patchy)
    margin = graph.ramp(scorch_noise, [(.35, (.040, .034, .013)), (.55, (.050, .035, .014)),
                                       (.75, (.024, .019, .008))], "EASE")
    colour = graph.mix(colour, margin, crisp)
    blister = graph.voronoi(point, scale=260).outputs["Distance"]
    colour = graph.mix(colour, (.020, .026, .010), graph.remap(blister, .0, .14, .30, 0))
    # Oil caught in the crinkles: darker, saturated and glossy.
    oil_cells = graph.voronoi(point, scale=55, randomness=.9).outputs["Distance"]
    oil_mask = graph.remap(graph.noise(point, scale=30, detail=1).outputs["Fac"], .48, .60)
    oil = graph.math("MULTIPLY", graph.remap(oil_cells, .26, .16), oil_mask)
    colour = graph.mix(colour, (.007, .010, .004), graph.math("MULTIPLY", oil, .5))
    graph.set("Base Color", colour)
    sheen = graph.noise(point, scale=60, detail=2).outputs["Fac"]
    rough = graph.remap(sheen, .35, .68, .50, .72)
    rough = graph.mix(rough, (.40, .40, .40), graph.math("MULTIPLY", thin, .8))
    graph.set("Roughness", graph.mix(rough, (.10, .10, .10), oil))
    graph.set("Transmission Weight", graph.math("MULTIPLY", thin, .5))
    graph.set("IOR", 1.38)
    crinkle = graph.noise(point, scale=700, detail=6, roughness=.62).outputs["Fac"]
    veins = graph.noise(graph.scale(point, (1, .5, 1)), scale=1100, detail=4, roughness=.7).outputs["Fac"]
    relief = graph.math("ADD", graph.math("ADD", veins, crinkle), graph.math("MULTIPLY", blister, .6))
    graph.set("Normal", graph.bump(relief, strength=.42, distance=.00006))
    return graph.mat


def slipware_bowl(name: str) -> bpy.types.Material:
    """Cream slip under honey lead glaze inside; dark glaze dipped outside over a bare red foot."""
    graph = _graph(name)
    point = graph.coord()
    _, _, height = graph.separate(point)
    inside, _, _ = graph.separate(_attr(graph, "vside"))
    firing = graph.noise(point, scale=48, detail=3).outputs["Fac"]
    honey = graph.ramp(firing, [
        (.25, (.33, .21, .075)), (.55, (.43, .29, .11)), (.85, (.50, .36, .15)),
    ], "EASE")
    specks = graph.voronoi(point, scale=650).outputs["Distance"]
    honey = graph.mix(honey, (.10, .045, .015), graph.remap(specks, .0, .07, .8, 0))
    pooling = graph.remap(height, .009, .016, .35, 0)
    honey = graph.mix(honey, (.25, .13, .035), pooling)
    x, y, _ = graph.separate(point)
    angle = graph.math("ARCTAN2", y, x)
    trail_noise = graph.noise(point, scale=140, detail=2).outputs["Fac"]
    wave = graph.math("ADD", .036, graph.math("MULTIPLY", graph.math("SINE", graph.math("MULTIPLY", angle, 7)), .0045))
    wave = graph.math("ADD", wave, graph.math("MULTIPLY", graph.math("SUBTRACT", trail_noise, .5), .0012))
    trail = graph.remap(graph.math("ABSOLUTE", graph.math("SUBTRACT", height, wave)), .0011, .0006)
    ring = graph.remap(graph.math("ABSOLUTE", graph.math("SUBTRACT", height, .0465)), .0008, .0004)
    slip = graph.math("MULTIPLY", graph.math("MAXIMUM", trail, ring), graph.remap(trail_noise, .2, .45, .55, .95))
    honey = graph.mix(honey, (.055, .022, .008), slip)
    treacle = graph.ramp(firing, [(.3, (.040, .018, .006)), (.7, (.085, .042, .014))])
    drip_noise = graph.noise(graph.scale(point, (1, 1, .12)), scale=90, detail=2).outputs["Fac"]
    dip_line = graph.math("ADD", .024, graph.math("MULTIPLY", graph.math("SUBTRACT", drip_noise, .5), .012))
    glazed_out = graph.remap(graph.math("SUBTRACT", height, dip_line), -.0006, .0006)
    clay = graph.ramp(firing, [(.3, (.16, .055, .025)), (.7, (.24, .095, .045))])
    outside = graph.mix(clay, treacle, glazed_out)
    inner = graph.remap(inside, .2, .8)
    graph.set("Base Color", graph.mix(outside, honey, inner))
    glazed = graph.math("MAXIMUM", inner, glazed_out)
    graph.set("Roughness", graph.mix((.82, .82, .82), (.10, .10, .10), glazed))
    graph.set("Coat Weight", graph.math("MULTIPLY", glazed, .5))
    graph.set("Coat Roughness", .05)
    crazing = graph.voronoi(point, scale=160, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    craze = graph.math("MULTIPLY", graph.remap(crazing, 0, .025, 1, 0), graph.math("MULTIPLY", inner, .5))
    grit = graph.noise(point, scale=1700, detail=2).outputs["Fac"]
    bare = graph.math("SUBTRACT", 1, glazed)
    relief = graph.math("ADD", graph.math("MULTIPLY", grit, graph.math("ADD", .08, bare)), graph.math("MULTIPLY", craze, -.3))
    graph.set("Normal", graph.bump(relief, strength=.25, distance=.00006))
    return graph.mat


def banded_creamware(name: str) -> bpy.types.Material:
    """Warm creamware with an ochre mocha band (dark dendrites) between brown slip lines."""
    graph = _graph(name)
    point = graph.coord()
    _, _, height = graph.separate(point)
    inside, _, _ = graph.separate(_attr(graph, "vside"))
    firing = graph.noise(point, scale=40, detail=3).outputs["Fac"]
    cream = graph.ramp(firing, [
        (.25, (.38, .31, .175)), (.55, (.45, .375, .225)), (.85, (.50, .425, .27)),
    ], "EASE")
    specks = graph.voronoi(point, scale=520).outputs["Distance"]
    cream = graph.mix(cream, (.16, .11, .06), graph.remap(specks, .0, .05, .6, 0))
    outer = graph.math("SUBTRACT", 1, graph.remap(inside, .3, .7))
    wobble = graph.math("MULTIPLY", graph.math("SUBTRACT", graph.noise(point, scale=30, detail=1).outputs["Fac"], .5), .0008)
    banded_height = graph.math("ADD", height, wobble)
    broad = graph.math("MULTIPLY", graph.remap(banded_height, .0330, .0338), graph.remap(banded_height, .0442, .0434))
    slip = graph.ramp(graph.noise(point, scale=160, detail=2).outputs["Fac"],
                      [(.3, (.17, .095, .038)), (.7, (.23, .135, .058))])
    colour = graph.mix(cream, slip, graph.math("MULTIPLY", broad, outer))
    for low, high in ((.0302, .0311), (.0468, .0477), (.0496, .0501)):
        line = graph.math("MULTIPLY", graph.remap(banded_height, low - .0002, low),
                          graph.remap(banded_height, high + .0002, high))
        colour = graph.mix(colour, (.035, .020, .010), graph.math("MULTIPLY", line, outer))
    foot = graph.remap(height, .0012, .0006)
    colour = graph.mix(colour, (.30, .22, .13), foot)
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.mix((.12, .12, .12), (.60, .60, .60), foot))
    graph.set("Coat Weight", .30)
    graph.set("Coat Roughness", .08)
    crazing = graph.voronoi(point, scale=110, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    glaze = graph.noise(point, scale=900, detail=2).outputs["Fac"]
    relief = graph.math("ADD", graph.math("MULTIPLY", glaze, .4), graph.math("MULTIPLY", graph.remap(crazing, 0, .02, 1, 0), -.25))
    graph.set("Normal", graph.bump(relief, strength=.16, distance=.00003))
    return graph.mat


def shell_edge_pearlware(name: str, scallops: int, rim_radius: float) -> bpy.types.Material:
    """Blue-tinted pearlware with a feathered cobalt shell-edge rim and moulded rays."""
    graph = _graph(name)
    point = graph.coord()
    x, y, height = graph.separate(point)
    radius = graph.vmath("LENGTH", graph.combine(x, y, 0))
    angle = graph.math("ARCTAN2", y, x)
    firing = graph.noise(point, scale=35, detail=3).outputs["Fac"]
    body = graph.ramp(firing, [
        (.25, (.36, .375, .39)), (.6, (.41, .425, .44)), (.9, (.45, .46, .47)),
    ], "EASE")
    rays = graph.math("SINE", graph.math("MULTIPLY", angle, scallops * 5))
    scallop = graph.math("ABSOLUTE", graph.math("SINE", graph.math("MULTIPLY", angle, scallops / 2)))
    around = graph.combine(graph.math("MULTIPLY", graph.math("COSINE", angle), 7),
                           graph.math("MULTIPLY", graph.math("SINE", angle), 7), 0)
    stroke_noise = graph.noise(around, scale=1.0, detail=2).outputs["Fac"]
    fine_around = graph.combine(graph.math("MULTIPLY", graph.math("COSINE", angle), 45),
                                graph.math("MULTIPLY", graph.math("SINE", angle), 45), 0)
    stroke_jitter = graph.noise(fine_around, scale=1.0, detail=1).outputs["Fac"]
    reach = graph.math("ADD", .0030, graph.math("MULTIPLY", stroke_noise, .0050))
    reach = graph.math("ADD", reach, graph.math("MULTIPLY", stroke_jitter, .0070))
    reach = graph.math("ADD", reach, graph.math("MULTIPLY", scallop, .0015))
    inset = graph.math("SUBTRACT", rim_radius, radius)
    feather = graph.math("ADD", graph.math("MULTIPLY", rays, .0018), inset)
    blue = graph.remap(feather, reach, graph.math("MULTIPLY", reach, .45))
    blue = graph.math("MULTIPLY", blue, graph.remap(height, .010, .015))
    pigment = graph.noise(point, scale=320, detail=2).outputs["Fac"]
    cobalt = graph.ramp(pigment, [(.3, (.008, .015, .070)), (.7, (.020, .040, .135))])
    colour = graph.mix(body, cobalt, graph.math("MULTIPLY", blue, .90))
    pool = graph.remap(height, .0055, .0075, .25, 0)
    colour = graph.mix(colour, (.40, .46, .52), pool)
    foot = graph.remap(height, .0009, .0004)
    colour = graph.mix(colour, (.42, .40, .37), foot)
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.mix((.07, .07, .07), (.55, .55, .55), foot))
    graph.set("Coat Weight", .45)
    graph.set("Coat Roughness", .04)
    rim_relief = graph.math("MULTIPLY", rays, graph.remap(radius, rim_radius - .014, rim_radius - .004))
    crazing = graph.voronoi(point, scale=70, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    relief = graph.math("ADD", graph.math("MULTIPLY", rim_relief, .5),
                        graph.math("MULTIPLY", graph.remap(crazing, 0, .015, 1, 0), -.15))
    graph.set("Normal", graph.bump(relief, strength=.22, distance=.00004))
    return graph.mat
