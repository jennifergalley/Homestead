"""Carved wooden water pail: a hollowed log section with a pouring lip, two carved ears
and a peeled-withy bail bound with twisted cord where the hand grips it, holding water.

Real-object research (written before modeling):
- Before coopering, and in many foraging cultures, water vessels were burned-and-scraped
  out of a single log section of soft, straight-grained wood (alder, cottonwood, basswood,
  pine), leaving a solid base 2-3 cm thick and walls 1-1.6 cm, with the grain vertical,
  adze/knife facets outside, and fine checks at the rim. Birch-bark pails are the other
  classic; the Sierra has little paper birch, so carved wood fits here.
- A handle (bail) of a green withy (willow, hazel, dogwood) is bent into an arch and its
  ends pushed through holes in two ears carved up from the rim, then hooked over inside.
  Where the hand closes, a twisted plant-fibre cord binding stops the thin rod cutting in.
- A pouring lip is carved into the rim on one side. Wood that holds water goes dark and
  slick inside and at the waterline; the base and the side below the lip stay damp.
- Size here: 24 cm tall (rim) x 20 cm across, holding water ~3.5 cm below the rim.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the middle of the bail's grip (the rod centre where the hand closes),
with the pail hanging BELOW it (-Z; base at about z -0.38). The bail's grip segment runs
along Y (ears at +-Y), and the pouring lip/spout points +X.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math

from mathutils import Matrix, Vector, noise

NAME = "WaterPail"
DESCRIPTION = ("Carved wooden water pail with pouring lip, withy bail and cord-bound grip, holding "
               "water (original). Pivot = bail grip centre; pail hangs -Z, spout +X, grip along Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}

H = 0.24                  # rim height above the base
R_TOP = 0.100             # outer radius at the rim
R_BASE = 0.091
WALL = 0.0115
FLOOR = 0.024
WATER = H - 0.036
EAR_H = 0.034             # ears rise this far above the rim
EAR_HALF = math.radians(24)
HOLE_Z = H + 0.018
HOLE_R = 0.0056
ROD_R = 0.0043
BAIL_RISE = 0.13          # apex of the bail above the hole
SIDES = 72


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def r_out(z):
    t = z / H
    return R_BASE + (R_TOP - R_BASE) * t + 0.004 * math.sin(math.pi * t)


def ear_u(angle):
    """0 at the centre of the nearer carved ear (+-Y), 1 at its shoulder."""
    d = min(abs(math.atan2(math.sin(angle - math.pi / 2), math.cos(angle - math.pi / 2))),
            abs(math.atan2(math.sin(angle + math.pi / 2), math.cos(angle + math.pi / 2))))
    return d / EAR_HALF


def ear(angle):
    """Rounded tab profile of the carved ears, with filleted shoulders into the rim."""
    u = ear_u(angle)
    dome = math.sqrt(max(1.0 - u * u, 0.0))
    return dome * smoothstep(1.0, 0.72, u) + 0.12 * smoothstep(1.45, 0.9, u) * (1 - dome)


def spout(angle):
    d = abs(math.atan2(math.sin(angle), math.cos(angle)))
    return smoothstep(math.radians(40), 0.0, d)


def rim_lift(angle):
    """Height of the rim above H: rounded ear tabs and a gentle rise into the lip."""
    e = ear(angle)
    return EAR_H * e - 0.004 * spout(angle) ** 1.5


def profile():
    """(radius_fn(z, a) kind, z) rows from the outer base edge up over the rim and down
    inside to the floor edge. Each row: (side, z_or_param)."""
    rows = [("out", 0.0), ("chamfer", 0.0)]
    rows += [("out", H * t) for t in (0.02, 0.08, 0.18, 0.3, 0.42, 0.54, 0.66, 0.76, 0.85, 0.92, 0.97)]
    rows += [("rim", k) for k in (0.0, 0.3, 0.6, 1.0)]
    rows += [("in", H * t) for t in (0.97, 0.9, 0.8, 0.66, 0.5, 0.36, 0.24, 0.16)]
    rows += [("in", FLOOR + 0.012), ("floor", 0.0)]
    return rows


def ring_point(kind, value, angle):
    lift = rim_lift(angle)
    sp = spout(angle)
    if kind == "chamfer":
        r, z = R_BASE - 0.004, 0.0
        return r, z
    if kind == "out":
        z = value
        top = smoothstep(H - 0.05, H, z)
        r = r_out(z) + 0.021 * sp ** 1.4 * top ** 2
        if value == 0.0:
            r, z = R_BASE - 0.0015, 0.003
        return r, z + lift * top ** 1.5
    if kind == "rim":
        # Rounded rim top from the outer to the inner face.
        k = value
        ro = r_out(H) + 0.021 * sp ** 1.4
        wall = WALL * (1 - 0.35 * sp)
        r = ro - wall * k
        z = H + lift + 0.0035 * math.sin(math.pi * k) - 0.007 * sp * (1 - abs(2 * k - 1)) ** 0.7
        return r, z
    if kind == "in":
        z = value
        top = smoothstep(H - 0.05, H, z)
        wall = WALL + 0.006 * (1 - z / H)
        r = r_out(z) + 0.021 * sp ** 1.4 * top ** 2 - wall * (1 - 0.35 * sp * top)
        return r, z + lift * top ** 1.5
    # floor edge: rounded into the carved floor
    return r_out(FLOOR) - WALL - 0.006 - 0.008, FLOOR


def build_body(kit, outside, inside):
    rows, coords, kinds = [], [], []
    for kind, value in profile():
        ring = []
        for j in range(SIDES):
            a = 2 * math.pi * j / SIDES
            r, z = ring_point(kind, value, a)
            # Adze facets outside, gouge scallops inside: shallow irregular radial offsets.
            if kind in ("out", "chamfer"):
                facet = abs(math.cos(0.5 * (11 * a + 1.5 * noise.noise(Vector((0, 0, z * 9))))))
                r += -0.0016 * facet + 0.0007 * noise.noise(Vector((math.cos(a) * 4, math.sin(a) * 4, z * 14)))
            elif kind == "in":
                r += 0.0006 * noise.noise(Vector((math.cos(a) * 9, math.sin(a) * 9, z * 30)))
            ring.append((r * math.cos(a), r * math.sin(a), z))
        rows.append(ring)
        coords.append([tuple(p) for p in ring])
        kinds.append(kind)
    body = kit.loft("Body", rows, material=[outside, inside], coords=coords,
                    cap_start=(0.0, 0.0, 0.0), cap_end=(0.0, 0.0, FLOOR - 0.002))
    kit.recalc_normals(body)
    mesh = body.data
    inner_from = next(i for i, k in enumerate(kinds) if k == "rim" ) + 2
    # Faces between ring i and i+1 are ordered by row; the end cap (floor) follows them.
    row_faces = (len(rows) - 1) * SIDES
    for index, poly in enumerate(mesh.polygons):
        if index < row_faces:
            poly.material_index = 1 if index // SIDES >= inner_from else 0
        elif index >= row_faces + SIDES:
            poly.material_index = 1
    # Drill the bail holes through both ears (along Y).
    import bpy
    cutter = kit.cylinder("HoleCutter", HOLE_R, 0.3, (0.0, 0.0, HOLE_Z), rotation=(90, 0, 0), sides=12)
    mod = body.modifiers.new("Holes", "BOOLEAN")
    mod.operation, mod.solver, mod.object = "DIFFERENCE", "EXACT", cutter
    kit.apply_modifiers(body)
    bpy.data.objects.remove(cutter)
    return body


def build_water(kit, water):
    rings = []
    for k in (0.0, 0.5, 1.0):
        ring = []
        for j in range(SIDES):
            a = 2 * math.pi * j / SIDES
            r = (r_out(WATER) - WALL - 0.0035 + 0.003) * k
            z = WATER + 0.0006 * noise.noise(Vector((math.cos(a) * 3 * k, math.sin(a) * 3 * k, 2.0)))
            ring.append((r * math.cos(a), r * math.sin(a), z))
        rings.append(ring)
    rings[0] = [(0.0, 0.0, WATER)] * SIDES
    verts = [(0.0, 0.0, WATER)] + [p for ring in rings[1:] for p in ring]
    faces = []
    for j in range(SIDES):
        faces.append((0, 1 + j, 1 + (j + 1) % SIDES))
    for j in range(SIDES):
        a, b = 1 + j, 1 + (j + 1) % SIDES
        faces.append((a, a + SIDES, b + SIDES, b))
    obj = kit.mesh("Water", verts, faces, material=water)
    return kit.recalc_normals(obj)


def bail_arc(theta, side_y):
    c, s = math.cos(theta), math.sin(theta)
    y = side_y * c
    z = HOLE_Z + 0.022 + (BAIL_RISE - 0.022) * max(s, 0.0) ** 0.5
    # Natural unevenness of a hand-bent withy.
    x = 0.004 * math.sin(theta * 1.7) * s
    return Vector((x, y, z))


# Pivot: the bail rod's centre at the top of its arch (the middle of the hand's grip).
APEX = bail_arc(math.pi / 2, 1.0)
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -R_TOP - APEX.y, HOLE_Z - APEX.z)}


def build_bail(kit, rod_mat):
    side = r_out(HOLE_Z - EAR_H) + 0.013
    r_in = r_out(H) - WALL
    hook = lambda sgn: [Vector((0.003, sgn * (r_in - 0.008), HOLE_Z - 0.030)),
                        Vector((0.002, sgn * (r_in - 0.009), HOLE_Z - 0.017)),
                        Vector((0.001, sgn * (r_in - 0.007), HOLE_Z - 0.005)),
                        Vector((0.0, sgn * (r_in - 0.001), HOLE_Z)),
                        Vector((0.0, sgn * (r_out(H) + 0.004), HOLE_Z)),
                        Vector((0.0, sgn * (r_out(H) + 0.011), HOLE_Z + 0.008))]
    arc = [bail_arc(math.pi * i / 40, side) for i in range(41)]
    points = hook(1) + arc[1:-1] + list(reversed(hook(-1)))
    radii = [ROD_R * (0.72 if i == 0 or i == len(points) - 1 else 1.0) for i in range(len(points))]
    return kit.tube("Bail", points, radii=radii, sides=12, material=rod_mat)


def build_grip_cord(kit, cord, turns=9.0, span=0.34, radius=0.0012):
    side = r_out(HOLE_Z - EAR_H) + 0.013
    steps = int(turns * 16)
    points, radii = [], []
    for i in range(steps + 1):
        t = i / steps
        theta = math.pi / 2 - span / 2 + span * t
        p = bail_arc(theta, side)
        q = bail_arc(theta + 1e-3, side)
        tangent = (q - p).normalized()
        binormal = Vector((1.0, 0.0, 0.0))
        normal = tangent.cross(binormal).normalized()
        binormal = normal.cross(tangent).normalized()
        a = 2 * math.pi * turns * t
        rr = ROD_R + radius * (0.85 + 0.15 * noise.noise(Vector((t * 13, 1, 0))))
        points.append(p + (normal * math.cos(a) + binormal * math.sin(a)) * rr)
        radii.append(radius * (0.9 + 0.2 * noise.noise(Vector((t * 29, 2, 0)))))
    return kit.tube("GripCord", points, radii=radii, sides=6, material=cord)


def node_helpers(mat):
    nodes, links = mat.node_tree.nodes, mat.node_tree.links

    def node(kind, **settings):
        n = nodes.new(kind)
        for key, value in settings.items():
            if key in n.inputs:
                if hasattr(value, "node"):
                    links.new(value, n.inputs[key])
                else:
                    n.inputs[key].default_value = value
            else:
                setattr(n, key, value)
        return n

    return nodes, links, node


def wet_overlay(mat, damp_below=0.035, drip=True):
    """Darken and slicken a wood material where water soaks it: a damp band at the base
    and a drip trail down the outside below the pouring lip (pcoord = rest position)."""
    nodes, links, node = node_helpers(mat)
    bsdf = nodes["Principled BSDF"]
    attr = node("ShaderNodeAttribute", attribute_name="pcoord", attribute_type="GEOMETRY")
    sep = node("ShaderNodeSeparateXYZ", Vector=attr.outputs["Vector"])
    ragged = node("ShaderNodeTexNoise", Vector=attr.outputs["Vector"], Scale=40.0, Detail=5.0).outputs["Fac"]
    jitter = node("ShaderNodeMath", operation="MULTIPLY_ADD", Value=ragged)
    jitter.inputs[1].default_value = 0.03
    links.new(sep.outputs["Z"], jitter.inputs[2])
    base_band = node("ShaderNodeMapRange", Value=jitter.outputs[0], **{"From Min": damp_below + 0.02,
                                                                      "From Max": damp_below}).outputs[0]
    mask = base_band
    if drip:
        angle = node("ShaderNodeMath", operation="ARCTAN2")
        links.new(sep.outputs["Y"], angle.inputs[0])
        links.new(sep.outputs["X"], angle.inputs[1])
        wobble = node("ShaderNodeMath", operation="MULTIPLY_ADD", Value=ragged)
        wobble.inputs[1].default_value = 0.12
        links.new(angle.outputs[0], wobble.inputs[2])
        off = node("ShaderNodeMath", operation="ADD", Value=wobble.outputs[0])
        off.inputs[1].default_value = -0.06
        dist = node("ShaderNodeMath", operation="ABSOLUTE", Value=off.outputs[0])
        trail = node("ShaderNodeMapRange", Value=dist.outputs[0], **{"From Min": 0.09, "From Max": 0.03}).outputs[0]
        fade = node("ShaderNodeMapRange", Value=sep.outputs["Z"], **{"From Min": H * 0.35, "From Max": H * 0.95,
                                                                   "To Min": 1.0, "To Max": 0.35}).outputs[0]
        streak = node("ShaderNodeMath", operation="MULTIPLY", Value=trail)
        links.new(fade, streak.inputs[1])
        mask = node("ShaderNodeMath", operation="MAXIMUM", Value=base_band)
        links.new(streak.outputs[0], mask.inputs[1])
        mask = mask.outputs[0]
    darken(mat, mask, 0.55, 0.22)
    return mat


def darken(mat, mask, factor, gloss):
    nodes, links, node = node_helpers(mat)
    bsdf = nodes["Principled BSDF"]
    col_in = bsdf.inputs["Base Color"].links[0].from_socket
    mul = node("ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY")
    rgba = [s for s in mul.inputs if s.type == "RGBA"]
    links.new(mask, mul.inputs["Factor"])
    links.new(col_in, rgba[0])
    rgba[1].default_value = (factor, factor * 0.97, factor * 0.93, 1.0)
    links.new(next(s for s in mul.outputs if s.type == "RGBA"), bsdf.inputs["Base Color"])
    rough_links = bsdf.inputs["Roughness"].links
    sub = node("ShaderNodeMath", operation="MULTIPLY_ADD", Value=mask)
    sub.inputs[1].default_value = -gloss
    if rough_links:
        links.new(rough_links[0].from_socket, sub.inputs[2])
    else:
        sub.inputs[2].default_value = bsdf.inputs["Roughness"].default_value
    links.new(sub.outputs[0], bsdf.inputs["Roughness"])


def adze_marks(mat, scale=38.0, strength=0.35, distance=0.0025):
    """Shallow dished knife/adze facets over the outside: Voronoi cells of pcoord,
    squashed vertically so each cut is taller than wide, with soft ridges between."""
    nodes, links, node = node_helpers(mat)
    bsdf = nodes["Principled BSDF"]
    attr = node("ShaderNodeAttribute", attribute_name="pcoord", attribute_type="GEOMETRY")
    mapping = node("ShaderNodeMapping", Vector=attr.outputs["Vector"])
    mapping.inputs["Scale"].default_value = (1.0, 1.0, 0.45)
    vor = node("ShaderNodeTexVoronoi", Vector=mapping.outputs["Vector"], Scale=scale, feature="DISTANCE_TO_EDGE")
    clamp = node("ShaderNodeMath", operation="MINIMUM", Value=vor.outputs["Distance"])
    clamp.inputs[1].default_value = 0.3
    dish = node("ShaderNodeMath", operation="POWER", Value=clamp.outputs[0])
    dish.inputs[1].default_value = 0.6
    bump = node("ShaderNodeBump", Strength=strength, Distance=distance, Height=dish.outputs[0])
    if bsdf.inputs["Normal"].links:
        links.new(bsdf.inputs["Normal"].links[0].from_socket, bump.inputs["Normal"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def water_material(kit, name):
    g = kit.mats.Graph(name)
    p = g.coord()
    ripple = g.noise(g.vmath("MULTIPLY", p, (1.0, 1.0, 0.0)), scale=45.0, detail=3.0).outputs["Fac"]
    silt = g.noise(p, scale=12.0, detail=2.0).outputs["Fac"]
    g.set("Base Color", g.mix((0.018, 0.017, 0.012), (0.034, 0.03, 0.02), g.remap(silt, 0.35, 0.65)))
    g.set("Roughness", g.remap(ripple, 0.4, 0.6, 0.03, 0.07))
    g.set("Normal", g.bump(ripple, strength=0.08, distance=0.002))
    return g.mat


def build(kit):
    m = kit.mats
    outside = m.wood("M_PailOutside", light=(0.235, 0.158, 0.09), dark=(0.105, 0.066, 0.036), grain=0.8,
                     roughness=0.68, weathering=0.05, grime=0.3, seed=13.0)
    wet_overlay(outside)
    adze_marks(outside)
    inside = m.wood("M_PailInside", light=(0.12, 0.08, 0.047), dark=(0.05, 0.032, 0.018), grain=0.7,
                    roughness=0.4, weathering=0.0, grime=0.5, seed=14.0, relief=0.6)
    rod = m.wood("M_PailBail", light=(0.33, 0.25, 0.15), dark=(0.17, 0.12, 0.07), grain=0.35,
                 roughness=0.55, weathering=0.1, grime=0.35, seed=15.0,
                 polish=0.7, polish_center=0.21, polish_length=0.05)
    cord = m.rawhide("M_PailCord", color=(0.24, 0.19, 0.12), strands=2, twist=140.0)
    water = water_material(kit, "M_PailWater")
    parts = [build_body(kit, outside, inside), build_water(kit, water), build_bail(kit, rod),
             build_grip_cord(kit, cord)]
    for part in parts:
        part.data.transform(Matrix.Translation(-APEX))
        part.data.update()
    return kit.join(parts, "SM_WaterPail", pivot=None, unwrap=False, reshade=True, smooth_angle=50)
