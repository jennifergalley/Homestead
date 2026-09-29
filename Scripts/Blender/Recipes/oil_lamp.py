"""Oil lamp: a small 1850s tin/brass hand hurricane lantern for Homestead.

Real-object research (written before modeling):
- Mid-19th-century British hand lanterns were commonly tinned sheet iron with a
  round oil fount/reservoir below, a simple flat-wick brass burner, a clear blown
  glass chimney/globe, a punched ventilator cap and a raised wire bail. The later
  cold-blast/dead-flame side-tube kerosene lanterns are avoided here: this is a
  simpler tin lantern suitable for an 1850s Cornish estate.
- Surviving tin hand lanterns and period pattern-book dimensions are typically
  about 11-13 in / 28-33 cm tall to the raised bail, with 4.5-5 in / 11-13 cm
  fount/base diameter; burner collars around 2.5-3 cm diameter; thin wire guards
  roughly 2-3 mm diameter. The glass chimney is hand-blown, bulbous, and open at
  top and bottom.
- Authored dimensions here: base/fount diameter 12.4 cm; opaque body to cap top
  28.6 cm; raised bail top including wooden grip 34.1 cm; globe centre at 15.4 cm;
  flame centre at 13.55 cm. Pivot is bottom-centre (z=0), Z up, -Y forward. Blender
  export mirrors Y into Unreal.
- Materials: dull tinplate sheet iron (metallic grey, rough, solder/rust freckles),
  tarnished brass burner/collar and thumbwheel, blackened wick and soot inside the
  cap, faint soot at the upper glass, and a small dark hand-worn wooden bail grip.

Meshes exported from this one recipe:
- SM_OilLamp: opaque tin/brass/wood/wick/cage/bail, baked 2K PBR + metallic map.
- SM_OilLampGlass: separate glass chimney/globe, same origin, no bake.
- SM_OilLampFlame: separate emissive teardrop flame, same origin, no bake.
Everything is original procedural geometry and materials.
"""
import math

import bpy
from mathutils import Vector, noise

NAME = "OilLamp"
DESCRIPTION = ("1850s tinned-sheet-iron hand oil lantern with brass burner, wire cage, "
               "separate glass globe and emissive flame (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 12000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BAKE_MESHES = {"SM_OilLamp"}

GLASS_BOTTOM = 0.084
GLASS_TOP = 0.224
GLASS_CENTER = (0.0, 0.0, 0.154)
FLAME_CENTER = (0.0, 0.0, 0.1355)
BAIL_GRIP_TOP = (0.0, 0.0, 0.341)
GLASS_PROFILE = [(GLASS_BOTTOM, 0.024), (0.088, 0.026), (0.093, 0.030), (0.100, 0.036),
                 (0.108, 0.041), (0.118, 0.0455), (0.130, 0.0485), (0.142, 0.0500),
                 (0.154, 0.0500), (0.166, 0.0475), (0.178, 0.0435), (0.190, 0.0385),
                 (0.202, 0.0330), (0.214, 0.0285), (GLASS_TOP, 0.026)]

REPORT = {"attach": {
    "units": "cm in exported mesh frame; pivot is bottom-centre on ground, Z up, -Y forward (Blender FBX export mirrors Y into Unreal)",
    "bail_grip_top_centre_cm": [round(c * 100, 2) for c in BAIL_GRIP_TOP],
    "flame_centre_cm": [round(c * 100, 2) for c in FLAME_CENTER],
    "glass_centre_cm": [round(c * 100, 2) for c in GLASS_CENTER],
    "overall_height_cm_to_bail_grip_top": 34.1,
    "base_fount_diameter_cm": 12.4,
}}
NOTES = {"material_slots": "SM_OilLamp bakes to one textured opaque material; SM_OilLampGlass and SM_OilLampFlame stay separate for engine translucent/emissive parents."}
BEAUTY = {"meshes": {
    "SM_OilLamp": {"pose": (0, 0, -28), "focus": (0.0, -0.01, 0.16), "ground": "origin", "detail_distance": 0.36},
    "SM_OilLampGlass": {"pose": (0, 0, -28), "focus": GLASS_CENTER, "ground": "lowest", "detail_distance": 0.28},
    "SM_OilLampFlame": {"pose": (0, 0, 0), "focus": FLAME_CENTER, "ground": "lowest", "detail_distance": 0.18, "detail_fstop": 32.0},
}}


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def _circle_tube(kit, name, radius, z, tube_radius, mat, sides=40, wire_sides=12, start=0.0, end=math.tau):
    count = max(9, int(sides * abs(end - start) / math.tau))
    pts = [Vector((math.cos(start + (end - start) * i / count) * radius,
                   math.sin(start + (end - start) * i / count) * radius, z)) for i in range(count + 1)]
    return kit.tube(name, pts, radius=tube_radius, sides=wire_sides, material=mat, cap=True)


def _lathe_mesh(kit, name, profile, mat, sides=64, cap_bottom=False, cap_top=False, coords_z_absolute=True):
    rows, coords = [], []
    for z, r in profile:
        row, pco = [], []
        for i in range(sides):
            a = math.tau * i / sides
            row.append((math.cos(a) * r, math.sin(a) * r, z))
            pco.append((math.cos(a) * r, math.sin(a) * r, z if coords_z_absolute else z - profile[0][0]))
        rows.append(row)
        coords.append(pco)
    obj = kit.loft(name, rows, material=mat, coords=coords, cap_start=cap_bottom, cap_end=cap_top)
    # Cylindrical UVs for the single-sided glass and tidy material coordinates elsewhere.
    uv = obj.data.uv_layers["UVMap"].data
    z0, z1 = profile[0][0], profile[-1][0]
    for poly in obj.data.polygons:
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            co = obj.data.vertices[vertex_index].co
            u = (math.atan2(co.y, co.x) / math.tau) % 1.0
            v = (co.z - z0) / max(1e-5, z1 - z0)
            uv[loop_index].uv = (u, v)
    return obj


def _vent_band(kit, mat, soot):
    sides = 36
    z0, z1 = 0.222, 0.249
    r = 0.033
    verts, coords, faces, mats = [], [], [], []
    for z in (z0, z1):
        for i in range(sides):
            a = math.tau * i / sides
            verts.append((math.cos(a) * r, math.sin(a) * r, z))
            coords.append((math.cos(a) * r, math.sin(a) * r, z))
    for i in range(sides):
        a0 = math.tau * i / sides
        # Twelve punched vertical slots; leave their middle segments open.
        slot = (i % 3) == 1
        if not slot:
            faces.append((i, (i + 1) % sides, sides + (i + 1) % sides, sides + i))
            mats.append(0)
    data = bpy.data.meshes.new("VentSlottedBand")
    data.from_pydata(verts, [], faces)
    data.update()
    data.uv_layers.new(name="UVMap")
    kit.tag_coords(data, [Vector(c) for c in coords])
    obj = bpy.data.objects.new("VentSlottedBand", data)
    bpy.context.scene.collection.objects.link(obj)
    data.materials.append(mat)
    data.materials.append(soot)
    kit.recalc_normals(obj)
    return obj


def _glass_material(kit):
    g = kit.mats.Graph("M_OilLampGlass")
    p = g.coord()
    x, y, z = g.separate(p)
    soot = g.remap(z, 0.184, 0.224, 0.0, 0.55)
    blotch = g.noise(p, scale=44.0, detail=4.0).outputs["Fac"]
    seed_bubbles = g.voronoi(p, scale=120.0, feature="F1").outputs["Distance"]
    bubbles = g.remap(seed_bubbles, 0.055, 0.016, 0.0, 0.38)
    color = g.mix((0.70, 0.78, 0.74), (0.28, 0.25, 0.20), soot)
    color = g.mix(color, (0.92, 0.98, 0.93), bubbles)
    color = g.mix(color, (0.52, 0.47, 0.38), g.remap(blotch, 0.68, 0.82, 0.0, 0.20))
    g.set("Base Color", color)
    g.set("Alpha", 0.34)
    g.set("Roughness", 0.08)
    for socket_name, value in (("Transmission Weight", 0.62), ("IOR", 1.46)):
        if socket_name in g.bsdf.inputs:
            g.set(socket_name, value)
    mat = g.mat
    mat.diffuse_color = (0.75, 0.9, 0.8, 0.34)
    mat.use_nodes = True
    mat.use_screen_refraction = True if hasattr(mat, "use_screen_refraction") else False
    if hasattr(mat, "blend_method"):
        mat.blend_method = "BLEND"
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    return mat


def _flame_material():
    mat = bpy.data.materials.new("M_OilLampFlame")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputMaterial")
    emis = tree.nodes.new("ShaderNodeEmission")
    attr = tree.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "pcoord"
    sep = tree.nodes.new("ShaderNodeSeparateXYZ")
    ramp = tree.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = (1.0, 0.32, 0.04, 1.0)
    ramp.color_ramp.elements[1].position = 1.0
    ramp.color_ramp.elements[1].color = (1.0, 0.18, 0.02, 1.0)
    mid = ramp.color_ramp.elements.new(0.48)
    mid.color = (1.0, 0.82, 0.42, 1.0)
    hot = ramp.color_ramp.elements.new(0.22)
    hot.color = (1.0, 0.96, 0.74, 1.0)
    map_z = tree.nodes.new("ShaderNodeMapRange")
    map_z.inputs["From Min"].default_value = FLAME_CENTER[2] - 0.018
    map_z.inputs["From Max"].default_value = FLAME_CENTER[2] + 0.018
    tree.links.new(attr.outputs["Vector"], sep.inputs["Vector"])
    tree.links.new(sep.outputs["Z"], map_z.inputs["Value"])
    tree.links.new(map_z.outputs["Result"], ramp.inputs["Fac"])
    tree.links.new(ramp.outputs["Color"], emis.inputs["Color"])
    emis.inputs["Strength"].default_value = 3.5
    tree.links.new(emis.outputs["Emission"], out.inputs["Surface"])
    mat.diffuse_color = (1.0, 0.42, 0.08, 0.72)
    return mat


def _teardrop_flame(kit, mat):
    height = 0.036
    base_z = FLAME_CENTER[2] - height * 0.50
    sides = 20
    profile = []
    for i in range(11):
        t = i / 10
        z = base_z + height * t
        if t < 0.18:
            r = 0.0025 + 0.0045 * smoothstep(0.0, 0.18, t)
        else:
            r = 0.0073 * math.sin(math.pi * min(1.0, (1.0 - t) / 0.82)) ** 0.58
        r *= (1.0 - 0.14 * t)
        profile.append((z, max(0.00025, r)))
    flame = _lathe_mesh(kit, "FlameTeardrop", profile, mat, sides=sides, cap_bottom=True, cap_top=True)
    return flame


def build(kit):
    m = kit.mats
    tin = m.tinplate("M_OilLampTinplate", base=(0.34, 0.34, 0.31), rust=0.42, seed=2.0)
    brass = m.brass("M_OilLampBrass", polished=(0.58, 0.40, 0.17), tarnish=(0.105, 0.070, 0.030), wear=0.22)
    wood = m.wood("M_OilLampGripWood", light=(0.18, 0.11, 0.055), dark=(0.075, 0.043, 0.022),
                  grain=0.6, roughness=0.68, grime=0.5, polish=0.75, polish_center=0.03, polish_length=0.04)
    wick = kit.material("M_OilLampCharredWick", (0.018, 0.014, 0.011), roughness=0.94)
    soot = kit.material("M_OilLampSoot", (0.038, 0.033, 0.028), roughness=0.98)
    for mat in (tin, brass, wood):
        mat.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value = 0.0

    parts = []
    fount_profile = [(0.000, 0.040), (0.010, 0.055), (0.022, 0.062), (0.044, 0.058),
                     (0.066, 0.046), (0.080, 0.031), (0.089, 0.025)]
    fount = _lathe_mesh(kit, "FountBulb", fount_profile, tin, sides=48, cap_bottom=True, cap_top=True)
    def fount_relief(co, pco):
        relief = 0.00045 * noise.noise(Vector((pco.x * 80, pco.y * 80, pco.z * 28)))
        # Soft dents in old sheet tin, shallow enough to stay hand-made rather than crumpled.
        for cx, cy, cz, sx, sy, sz, depth in ((0.038, -0.028, 0.043, 0.020, 0.016, 0.020, 0.0011),
                                              (-0.034, 0.021, 0.030, 0.018, 0.014, 0.015, 0.0008)):
            d = ((co.x - cx) / sx) ** 2 + ((co.y - cy) / sy) ** 2 + ((co.z - cz) / sz) ** 2
            relief -= depth * math.exp(-d)
        return relief
    kit.displace(fount, fount_relief)
    parts.append(fount)
    parts += [
        _circle_tube(kit, "FountBottomRolledSeam", 0.055, 0.012, 0.0018, tin, sides=40),
        _circle_tube(kit, "FountShoulderSolderSeam", 0.046, 0.067, 0.00125, tin, sides=40),
        _circle_tube(kit, "FountTopRim", 0.026, 0.090, 0.0016, tin, sides=36),
    ]
    # Filler cap on the shoulder, angled toward the carrier.
    cap_stem = kit.tube("FillerStem", [Vector((-0.038, -0.033, 0.068)), Vector((-0.048, -0.043, 0.080))],
                        radius=0.0045, sides=16, material=tin)
    cap = kit.cylinder("FillerCap", 0.0072, 0.0045, (-0.050, -0.045, 0.083), rotation=(52, 0, -44),
                       material=tin, sides=24, bevel=0.0007, bevel_segments=2)
    parts += [cap_stem, cap]

    # Burner, wick and thumbwheel.
    parts += [
        kit.cylinder("BurnerCollar", 0.020, 0.012, (0, 0, 0.096), material=brass, sides=32, bevel=0.001, bevel_segments=2),
        kit.cylinder("BurnerGallery", 0.014, 0.025, (0, 0, 0.109), material=brass, sides=28, bevel=0.0007, bevel_segments=2),
        kit.box("CharredFlatWick", (0.0065, 0.0026, 0.016), (0, -0.0005, 0.121), material=wick, bevel=0.0005),
        kit.tube("ThumbwheelStem", [Vector((0.018, -0.001, 0.105)), Vector((0.041, -0.001, 0.105))],
                 radius=0.0021, sides=12, material=brass),
        kit.cylinder("Thumbwheel", 0.008, 0.003, (0.045, -0.001, 0.105), rotation=(0, 90, 0),
                     material=brass, sides=24, bevel=0.00045, bevel_segments=1),
    ]
    # Knurled thumbwheel rim marks.
    for k in range(14):
        a = math.tau * k / 14
        parts.append(kit.box(f"ThumbwheelTooth{k:02d}", (0.0010, 0.0022, 0.0048),
                             (0.0466, math.cos(a) * 0.0067, 0.105 + math.sin(a) * 0.0067),
                             rotation=(0, 0, math.degrees(a)), material=brass))

    # Wire cage around the glass.
    for k, a in enumerate((math.radians(35), math.radians(145), math.radians(215), math.radians(325))):
        pts = []
        for i in range(12):
            t = i / 11
            z = 0.088 + (0.216 - 0.088) * t
            r = 0.041 + 0.011 * math.sin(math.pi * t) ** 0.75
            pts.append(Vector((math.cos(a) * r, math.sin(a) * r, z)))
        parts.append(kit.tube(f"GlassGuardWire{k}", pts, radius=0.00135, sides=12, material=tin))
    parts += [
        _circle_tube(kit, "GuardLowerRing", 0.043, 0.089, 0.00125, tin, sides=40),
        _circle_tube(kit, "GuardBellyRing", 0.053, 0.152, 0.00110, tin, sides=40),
        _circle_tube(kit, "GuardUpperRing", 0.037, 0.215, 0.00125, tin, sides=40),
    ]

    # Vented cap and chimney top.
    parts += [
        kit.cylinder("CapLowerCup", 0.038, 0.010, (0, 0, 0.217), material=tin, sides=36, radius_top=0.033,
                     bevel=0.0008, bevel_segments=1),
        _vent_band(kit, tin, soot),
        kit.cylinder("CapCone", 0.034, 0.021, (0, 0, 0.260), material=tin, sides=36, radius_top=0.014,
                     bevel=0.0004, bevel_segments=1),
        kit.cylinder("TopChimney", 0.014, 0.024, (0, 0, 0.278), material=tin, sides=32, radius_top=0.011,
                     bevel=0.0006, bevel_segments=1),
        kit.cylinder("SootInsideCap", 0.024, 0.002, (0, 0, 0.228), material=soot, sides=32),
        kit.cylinder("SootVentLiner", 0.031, 0.025, (0, 0, 0.235), material=soot, sides=32, cap=False),
    ]

    # Bail hinge lugs, raised bail and wooden grip.
    for sx in (-1, 1):
        parts += [
            kit.box(f"BailLug{sx}", (0.006, 0.012, 0.015), (sx * 0.049, 0, 0.237), material=tin, bevel=0.001),
            kit.tube(f"BailHingeLoop{sx}", [Vector((sx * 0.052, -0.007, 0.236)), Vector((sx * 0.052, 0.007, 0.236))],
                     radius=0.0021, sides=12, material=tin),
        ]
    bail_pts = []
    for i in range(25):
        t = math.pi - math.pi * i / 24
        x = 0.052 * math.cos(t)
        z = 0.237 + 0.098 * math.sin(t)
        bail_pts.append(Vector((x, 0, z)))
    parts.append(kit.tube("RaisedWireBail", bail_pts, radius=0.0020, sides=12, material=tin))
    grip = kit.tube("WoodenBailGrip", [Vector((0, -0.030, 0.335)), Vector((0, 0.030, 0.335))],
                    radius=0.006, sides=18, material=wood)
    kit.displace(grip, lambda co, pco: 0.00022 * noise.noise(Vector((pco.x * 90, pco.y * 90, pco.z * 35))))
    parts.append(grip)

    opaque = kit.join(parts, "SM_OilLamp", pivot=None, unwrap=False, reshade=True, smooth_angle=55)
    kit.finalize(opaque, pivot=None, unwrap=False, reshade=True, smooth_angle=55)

    glass_mat = _glass_material(kit)
    glass = _lathe_mesh(kit, "GlassGlobe", GLASS_PROFILE, glass_mat, sides=96, cap_bottom=False, cap_top=False)
    kit.displace(glass, lambda co, pco: 0.00018 * noise.noise(Vector((pco.x * 55, pco.y * 55, pco.z * 23))))
    glass.name = glass.data.name = "SM_OilLampGlass"
    kit.finalize(glass, pivot=None, unwrap=False, reshade=True, smooth_angle=180)

    flame = _teardrop_flame(kit, _flame_material())
    flame.name = flame.data.name = "SM_OilLampFlame"
    kit.finalize(flame, pivot=None, unwrap=False, reshade=True, smooth_angle=60)
    return [opaque, glass, flame]
