"""Field mushrooms (Agaricus campestris), a small Cornish pasture group.

Research summary (written before modeling):
- Field mushrooms fruit in autumn in short unimproved pasture and grassy woodland
  edges, including Cornwall in the 1850s. They appear scattered or in loose
  groups rather than in dense tufts.
- Caps are 3-10 cm across: white to off-white or pale greyish cream, slightly
  silky-fibrillose, often with faint tan-brown scales or bruising near the
  centre. Young caps are domed/buttons with a slightly inrolled margin; mature
  caps open to convex or nearly flat.
- Gills are free and crowded, starting pink on young open caps and darkening
  through cocoa to chocolate brown with age. Stems are short, stout and white,
  about 3-6 cm tall and 1-2 cm thick, with a thin fragile ring and a slightly
  tapered base.
- Pasture specimens often carry a little dark soil and short grass caught around
  the stem bases; no ground disc is present.

Original procedural geometry and PBR materials: six age-varied mushrooms, free
radial gills, fragile rings, dirt clods, moss flecks and short grass. Units are
meters, Z up, -Y forward. Pivot is bottom-centre at the lowest contact point.
"""

import math
import random

import bpy
from mathutils import Matrix, Vector, noise

NAME = "FieldMushrooms"
DESCRIPTION = ("A loose 30 x 25 cm group of field mushrooms: young buttons, open caps with "
               "pink and chocolate gills, fragile rings, soil and short pasture grass.")
COLLISION = "none"
TRIANGLE_BUDGET = 30000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
BEAUTY = {"pose": (0, 0, -18), "focus": (-0.040, 0.052, 0.044), "detail_distance": 0.46,
          "ground": "origin",
          "detail_fstop": 22}
NOTES = {
    "reference": "Agaricus campestris in Cornish pasture/woodland edge, autumn, 1850s.",
    "scale": "Patch footprint roughly 30 x 25 cm; individual caps 2.5-8.8 cm across.",
    "collision": "None; small forage decoration/static prop.",
}

SEED = 20260929


def _smoothstep(a, b, x):
    if a == b:
        return 0.0
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def _graph_materials(kit):
    mats = {}

    def cap_material(name, dark, light, buff, seed, scale_strength=0.78):
        g = kit.mats.Graph(name)
        p = g.coord()
        x, y, z = g.separate(p)
        seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.19, seed * 0.47))
        radial = g.vmath("LENGTH", g.combine(x, y, 0.0))
        angle = g.math("ARCTAN2", y, x)
        broad = g.noise(seeded, scale=14.0, detail=7.0, roughness=0.60).outputs["Fac"]
        fine = g.noise(seeded, scale=145.0, detail=8.0, roughness=0.64).outputs["Fac"]
        radial_fibre = g.wave(g.combine(g.math("MULTIPLY", angle, 0.22),
                                        g.math("MULTIPLY", radial, 32.0),
                                        g.math("MULTIPLY", z, 0.10)),
                              scale=18.0, distortion=7.0, detail=5.0,
                              kind="BANDS", direction="Y").outputs["Color"]
        fibre_luma = g.channel(radial_fibre, 0)
        centre_scales = g.math("MULTIPLY", g.remap(radial, 0.060, 0.004, 1.0, 0.0),
                               g.remap(fine, 0.50, 0.78, 0.0, scale_strength))
        small_bruises = g.math("MULTIPLY", g.remap(fine, 0.77, 0.88, 0.0, 0.34),
                               g.remap(radial, 0.010, 0.090, 0.20, 1.0))
        col = g.mix(dark, light, g.remap(broad, 0.18, 0.82))
        col = g.mix(col, buff, g.remap(fibre_luma, 0.50, 0.96, 0.0, 0.32))
        col = g.mix(col, (0.31, 0.220, 0.135), centre_scales)
        col = g.mix(col, (0.23, 0.155, 0.090), small_bruises)
        g.set("Base Color", col)
        rough = g.math("ADD", g.remap(broad, 0.0, 1.0, 0.52, 0.63),
                       g.math("MULTIPLY", fibre_luma, 0.045))
        g.set("Roughness", rough)
        height = g.math("ADD", g.math("MULTIPLY", fibre_luma, 0.55),
                        g.math("MULTIPLY", centre_scales, 0.65))
        height = g.math("ADD", height, g.math("MULTIPLY", small_bruises, -0.22))
        g.set("Normal", g.bump(height, strength=0.12, distance=0.00034))
        g.set("Subsurface Weight", 0.0)
        return g.mat

    mats["cap_cream"] = cap_material("M_FieldMushroomCapCream",
                                     (0.455, 0.440, 0.395), (0.590, 0.570, 0.505),
                                     (0.525, 0.492, 0.405), 1.0, 0.72)
    mats["cap_offwhite"] = cap_material("M_FieldMushroomCapOffWhite",
                                        (0.485, 0.472, 0.430), (0.610, 0.592, 0.535),
                                        (0.545, 0.515, 0.445), 2.0, 0.62)
    mats["cap_buff"] = cap_material("M_FieldMushroomCapBuff",
                                    (0.435, 0.410, 0.355), (0.565, 0.538, 0.462),
                                    (0.520, 0.455, 0.340), 3.0, 0.86)
    mats["cap_grey"] = cap_material("M_FieldMushroomCapGreyCream",
                                    (0.425, 0.420, 0.390), (0.555, 0.548, 0.505),
                                    (0.485, 0.455, 0.380), 4.0, 0.56)

    g = kit.mats.Graph("M_FieldMushroomCap")
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (0.31, 0.19, 0.47))
    broad = g.noise(seeded, scale=18.0, detail=6.0, roughness=0.58).outputs["Fac"]
    fine = g.noise(seeded, scale=120.0, detail=7.0, roughness=0.62).outputs["Fac"]
    fibres = g.wave(g.combine(g.math("MULTIPLY", x, 1.4), g.math("MULTIPLY", y, 1.4),
                              g.math("MULTIPLY", z, 0.25)),
                    scale=22.0, distortion=5.5, detail=4.0, kind="RINGS", direction="Z").outputs["Color"]
    fibre_luma = g.channel(fibres, 0)
    radial = g.vmath("LENGTH", g.combine(x, y, 0.0))
    centre_scales = g.math("MULTIPLY", g.remap(radial, 0.055, 0.0, 1.0, 0.0),
                           g.remap(fine, 0.52, 0.79, 0.0, 1.0))
    freckles = g.math("MULTIPLY", g.remap(fine, 0.72, 0.82, 0.0, 0.65),
                      g.remap(radial, 0.018, 0.090, 0.25, 1.0))
    cream = g.mix((0.49, 0.470, 0.420), (0.64, 0.615, 0.545), g.remap(broad, 0.25, 0.78))
    cream = g.mix(cream, (0.56, 0.535, 0.475), g.remap(fibre_luma, 0.45, 0.95, 0.0, 0.38))
    cream = g.mix(cream, (0.33, 0.235, 0.145), centre_scales)
    cream = g.mix(cream, (0.28, 0.19, 0.105), freckles)
    g.set("Base Color", cream)
    g.set("Roughness", g.remap(broad, 0.0, 1.0, 0.54, 0.70))
    height = g.math("ADD", g.math("MULTIPLY", fibre_luma, 0.35), g.math("MULTIPLY", freckles, 0.55))
    g.set("Normal", g.bump(height, strength=0.22, distance=0.00075))
    g.set("Subsurface Weight", 0.0)
    mats["cap"] = g.mat

    g = kit.mats.Graph("M_FieldMushroomCapBruised")
    p = g.coord()
    seeded = g.vmath("ADD", p, (1.7, 0.4, 0.2))
    stain = g.noise(seeded, scale=44.0, detail=6.0, roughness=0.68).outputs["Fac"]
    g.set("Base Color", g.ramp(stain, [(0.18, (0.44, 0.38, 0.30)),
                                       (0.55, (0.30, 0.205, 0.130)),
                                       (0.92, (0.19, 0.115, 0.065))]))
    g.set("Roughness", g.remap(stain, 0.0, 1.0, 0.62, 0.82))
    g.set("Normal", g.bump(stain, strength=0.26, distance=0.00065))
    g.set("Subsurface Weight", 0.0)
    mats["bruise"] = g.mat

    def gill_material(name, pale, dark, rough=(0.70, 0.86), seed=0.0):
        g = kit.mats.Graph(name)
        p = g.coord()
        x, y, z = g.separate(p)
        line = g.wave(g.combine(g.math("MULTIPLY", x, 1.2 + seed), g.math("MULTIPLY", y, 1.2),
                                g.math("MULTIPLY", z, 0.1)),
                      scale=34.0, distortion=2.2, detail=3.0, kind="RINGS", direction="Z").outputs["Color"]
        pores = g.noise(g.vmath("ADD", p, (seed, seed * 0.3, 0.0)), scale=170.0,
                        detail=4.0, roughness=0.56).outputs["Fac"]
        stripes = g.channel(line, 0)
        col = g.mix(pale, dark, g.remap(stripes, 0.20, 0.85, 0.15, 0.86))
        col = g.mix(col, tuple(c * 0.72 for c in dark), g.remap(pores, 0.66, 0.82, 0.0, 0.42))
        g.set("Base Color", col)
        g.set("Roughness", g.remap(pores, 0.0, 1.0, rough[0], rough[1]))
        g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", stripes, 0.4),
                                      g.math("MULTIPLY", pores, 0.28)),
                               strength=0.32, distance=0.00036))
        g.set("Subsurface Weight", 0.0)
        return g.mat

    mats["gill_pink"] = gill_material("M_FieldMushroomGillsPink",
                                      (0.60, 0.405, 0.385), (0.535, 0.345, 0.325),
                                      rough=(0.66, 0.82), seed=0.2)
    mats["gill_brown"] = gill_material("M_FieldMushroomGillsBrown",
                                       (0.285, 0.175, 0.135), (0.245, 0.145, 0.110),
                                       rough=(0.78, 0.90), seed=1.1)

    g = kit.mats.Graph("M_FieldMushroomStem")
    p = g.coord()
    x, y, z = g.separate(p)
    angle = g.math("ARCTAN2", y, x)
    stripe = g.wave(g.combine(g.math("MULTIPLY", angle, 0.9), g.math("MULTIPLY", z, 1.8), 0.0),
                    scale=15.0, distortion=1.2, detail=4.0, kind="BANDS", direction="Z").outputs["Color"]
    stripe = g.channel(stripe, 0)
    fuzz = g.noise(g.vmath("ADD", p, (0.4, 1.2, 0.2)), scale=95.0,
                   detail=6.0, roughness=0.62).outputs["Fac"]
    base_dirt = g.remap(z, 0.026, 0.000, 0.0, 0.92)
    stem = g.mix((0.48, 0.455, 0.390), (0.67, 0.640, 0.555), g.remap(fuzz, 0.24, 0.78))
    stem = g.mix(stem, (0.78, 0.74, 0.64), g.remap(stripe, 0.62, 0.96, 0.0, 0.25))
    stem = g.mix(stem, (0.155, 0.105, 0.065), base_dirt)
    g.set("Base Color", stem)
    g.set("Roughness", g.remap(fuzz, 0.0, 1.0, 0.52, 0.70))
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", stripe, 0.48),
                                  g.math("MULTIPLY", fuzz, 0.22)),
                           strength=0.20, distance=0.00055))
    g.set("Subsurface Weight", 0.0)
    mats["stem"] = g.mat

    g = kit.mats.Graph("M_FieldMushroomRing")
    p = g.coord()
    u, v, z = g.separate(p)
    tissue = g.noise(g.vmath("ADD", p, (0.9, 0.1, 1.4)), scale=72.0, detail=6.0).outputs["Fac"]
    rag = g.remap(tissue, 0.30, 0.86)
    col = g.mix((0.46, 0.43, 0.36), (0.62, 0.59, 0.51), rag)
    col = g.mix(col, (0.22, 0.155, 0.095), g.remap(v, -0.015, -0.030, 0.0, 0.45))
    g.set("Base Color", col)
    g.set("Roughness", 0.78)
    g.set("Normal", g.bump(tissue, strength=0.28, distance=0.00035))
    g.set("Subsurface Weight", 0.0)
    mats["ring"] = g.mat

    g = kit.mats.Graph("M_FieldMushroomGrass")
    uv = g.uv()
    u, v, _ = g.separate(uv)
    across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
    mid = g.remap(across, 0.0, 0.04, 1.0, 0.0)
    mottle = g.noise(uv, scale=18.0, detail=4.0, roughness=0.6, dims="2D").outputs["Fac"]
    blade = g.mix((0.055, 0.115, 0.030), (0.145, 0.225, 0.060), g.remap(v, 0.0, 1.0))
    blade = g.mix(blade, (0.20, 0.17, 0.055), g.math("MULTIPLY", g.remap(mottle, 0.64, 0.86),
                                                      g.remap(v, 0.58, 1.0)))
    blade = g.mix(blade, (0.19, 0.27, 0.095), mid)
    g.set("Base Color", blade)
    g.set("Roughness", g.remap(mottle, 0.0, 1.0, 0.55, 0.78))
    g.set("Normal", g.bump(g.math("MAXIMUM", mid, mottle), strength=0.22, distance=0.00035))
    g.set("Subsurface Weight", 0.0)
    mats["grass"] = g.mat

    mats["soil"] = kit.mats.soil("M_FieldMushroomSoil", damp=(0.060, 0.041, 0.025),
                                  dry=(0.19, 0.145, 0.090), seed=8.0)
    mats["moss"] = kit.material("M_FieldMushroomMoss", (0.060, 0.105, 0.040), roughness=0.92)
    mats["dead"] = kit.material("M_FieldMushroomDeadGrass", (0.19, 0.145, 0.075), roughness=0.88)
    return mats


def _cap_matrix(x, y, z, yaw, tilt_x=0.0, tilt_y=0.0):
    return (Matrix.Translation((x, y, z))
            @ Matrix.Rotation(yaw, 4, "Z")
            @ Matrix.Rotation(tilt_x, 4, "X")
            @ Matrix.Rotation(tilt_y, 4, "Y"))


def _cap_edge_factor(theta, rng, broken=False):
    wobble = 1.0 + 0.025 * math.sin(theta * 3.0 + 0.37) + 0.014 * math.sin(theta * 7.0 - 0.9)
    if broken:
        # Two small bites / nibbles in the rim, not a clean cut.
        for centre, width, depth in ((0.55, 0.22, 0.26), (0.95, 0.12, 0.15)):
            d = math.atan2(math.sin(theta - centre), math.cos(theta - centre))
            wobble -= depth * math.exp(-0.5 * (d / width) ** 2)
    return max(0.64, wobble)


def _make_cap(kit, name, mats, loc, radius, dome, thickness, yaw, age, rng,
              tilt=(0.0, 0.0), broken=False, open_cap=True, cap_key="cap_cream"):
    rings = 10 if open_cap else 9
    sides = 52 if radius >= 0.040 else 42
    matrix = _cap_matrix(loc[0], loc[1], loc[2], yaw, tilt[0], tilt[1])
    top_rows = []
    under_rows = []
    verts = []
    for i in range(rings + 1):
        rr = i / rings
        top_row = []
        under_row = []
        for j in range(sides):
            theta = math.tau * j / sides
            edge = _cap_edge_factor(theta, rng, broken)
            radial = radius * rr * edge
            x = radial * math.cos(theta)
            y = radial * math.sin(theta)
            wave = (0.0013 * math.sin(theta * 5.0 + age * 1.7)
                    + 0.0009 * math.sin(theta * 9.0 - age * 0.8)) * (rr ** 1.6)
            dent = -0.0017 * max(0.0, noise.noise(Vector((x * 55.0 + age * 9.0,
                                                           y * 55.0 - age * 3.0, 0.4)))) * (0.25 + rr)
            if open_cap:
                rim = _smoothstep(0.72, 1.0, rr)
                top_z = thickness * 0.24 + dome * ((1.0 - rr ** 2.0) ** 1.25) - 0.0034 * rr ** 4 + wave + dent
                under_z = -thickness * (0.26 + 0.27 * rr) - 0.0009 * rr ** 2.5 + 0.0018 * rim
            else:
                rim = _smoothstep(0.78, 1.0, rr)
                top_z = thickness * 0.28 + dome * ((1.0 - rr ** 2.15) ** 0.58) - 0.0022 * rr ** 3 + wave + dent
                under_z = -thickness * (0.40 + 0.55 * rr ** 1.9) - 0.0032 * rr ** 7 + 0.0022 * rim
            top_row.append(len(verts))
            verts.append(matrix @ Vector((x, y, top_z)))
            under_row.append(len(verts))
            # Slightly smaller underside near the rim gives the inrolled cap edge.
            shrink = 1.0 - (0.075 if open_cap else 0.120) * (rr ** 2)
            verts.append(matrix @ Vector((x * shrink, y * shrink, under_z)))
        top_rows.append(top_row)
        under_rows.append(under_row)

    faces = []
    face_mats = []
    for i in range(rings):
        for j in range(sides):
            jn = (j + 1) % sides
            faces.append((top_rows[i][j], top_rows[i + 1][j], top_rows[i + 1][jn], top_rows[i][jn]))
            face_mats.append(0)
            # Underside is gill-coloured, but separate rib geometry supplies the crowded edges.
            faces.append((under_rows[i][jn], under_rows[i + 1][jn], under_rows[i + 1][j], under_rows[i][j]))
            face_mats.append(1)
    for j in range(sides):
        jn = (j + 1) % sides
        faces.append((top_rows[-1][j], top_rows[-1][jn], under_rows[-1][jn], under_rows[-1][j]))
        face_mats.append(2 if broken and 0.33 < (j / sides) < 0.56 else 0)

    gill_key = "gill_pink" if age < 0.48 else "gill_brown"
    obj = kit.mesh(name, verts, faces, material=mats.get(cap_key, mats["cap"]))
    obj.data.materials.append(mats[gill_key])
    obj.data.materials.append(mats["bruise"])
    for poly, mat_index in zip(obj.data.polygons, face_mats):
        poly.material_index = mat_index
    obj.data.update()
    kit.tag_coords(obj.data)
    kit.displace(obj, lambda co, pco: 0.00055 * noise.noise(Vector((pco.x * 65.0 + age * 7.0,
                                                                     pco.y * 65.0,
                                                                     pco.z * 40.0))))
    return obj, matrix


def _make_gills(kit, name, mat, matrix, radius, inner_radius, thickness, age, rng, broken=False):
    ribs = 88 if radius > 0.040 else 58
    segments = 3
    verts, faces = [], []
    for k in range(ribs):
        theta = math.tau * (k + rng.uniform(-0.18, 0.18)) / ribs
        if broken and 0.35 < (theta % math.tau) < 0.65:
            continue
        base = len(verts)
        depth = 0.00008 + 0.00012 * rng.random()
        phase = rng.random() * math.tau
        length_kind = (1.0, 0.58, 0.34, 0.82, 0.48, 0.72)[k % 6] * rng.uniform(0.92, 1.05)
        outer = radius * rng.uniform(0.88, 0.96)
        start = outer - (outer - inner_radius) * min(1.0, length_kind)
        for s in range(segments + 1):
            t = s / segments
            r = start + (outer - start) * t
            edge = _cap_edge_factor(theta, rng, broken)
            r = min(r, radius * edge * 0.91)
            wiggle = 0.010 * math.sin(t * math.pi * 1.4 + phase) + 0.004 * math.sin(t * math.pi * 5.0 + k)
            x = r * math.cos(theta + wiggle)
            y = r * math.sin(theta + wiggle)
            top_z = -thickness * (0.36 + 0.23 * t) - 0.0008 * t
            low_z = top_z - depth * (0.55 + 0.65 * t)
            verts.append(matrix @ Vector((x, y, top_z)))
            verts.append(matrix @ Vector((x, y, low_z)))
        for s in range(segments):
            a = base + s * 2
            b = base + (s + 1) * 2
            faces.append((a, b, b + 1, a + 1))
            faces.append((a + 1, b + 1, b, a))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data)
    return obj


def _make_ring(kit, name, mat, loc, stem_radius, yaw, rng):
    sides = 56
    rows = 3
    inner = stem_radius * rng.uniform(0.96, 1.04)
    outer = stem_radius * rng.uniform(1.45, 1.82)
    droop = rng.uniform(0.0040, 0.0075)
    thickness = rng.uniform(0.00045, 0.00080)
    verts, faces = [], []
    matrix = _cap_matrix(loc[0], loc[1], loc[2], yaw, rng.uniform(-0.035, 0.035), rng.uniform(-0.035, 0.035))
    edge_cache = []
    for j in range(sides):
        t = math.tau * j / sides
        tear = 1.0 + 0.10 * math.sin(t * 4.0 + 0.7) + 0.08 * math.sin(t * 9.0 + rng.random())
        if j % 17 in (0, 1):
            tear *= rng.uniform(0.72, 0.88)
        edge_cache.append(max(0.68, tear))
    for side_z in (thickness * 0.5, -thickness * 0.5):
        for i in range(rows):
            rr = i / (rows - 1)
            for j in range(sides):
                t = math.tau * j / sides
                rag = edge_cache[j]
                r = inner + (outer * rag - inner) * (rr ** 0.78)
                scallop = 0.0011 * math.sin(t * 7.0 + rr * 2.1) * rr
                zoff = side_z - droop * (rr ** 1.35) * (0.72 + 0.28 * math.sin(t * 3.0 + 0.3)) + scallop
                verts.append(matrix @ Vector((r * math.cos(t), r * math.sin(t), zoff)))
    layer = rows * sides
    for i in range(rows - 1):
        for j in range(sides):
            jn = (j + 1) % sides
            a = i * sides + j
            b = (i + 1) * sides + j
            c = (i + 1) * sides + jn
            d = i * sides + jn
            faces.append((a, b, c, d))
            faces.append((layer + d, layer + c, layer + b, layer + a))
    # Thin torn outer edge and inner attachment lip.
    for j in range(sides):
        jn = (j + 1) % sides
        faces.append(((rows - 1) * sides + j, (rows - 1) * sides + jn,
                      layer + (rows - 1) * sides + jn, layer + (rows - 1) * sides + j))
        faces.append((j, layer + j, layer + jn, jn))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data)
    return obj


def _make_stem(kit, name, mat, base, height, radius, yaw, rng, lean=(0.0, 0.0)):
    rings = 13
    points = []
    phase = rng.random() * math.tau
    lean_vec = Vector((lean[0], lean[1], 0.0))
    sink = rng.uniform(0.0035, 0.0050)
    for i in range(rings):
        t = i / (rings - 1)
        wobble = Vector((math.sin(t * math.pi * 1.4 + phase), math.cos(t * math.pi * 1.1 + phase), 0.0))
        z = -sink + (height + sink) * t
        points.append(Vector(base) + Vector((0, 0, z)) + lean_vec * (t ** 1.35)
                      + wobble * (0.0015 * math.sin(math.pi * t)))

    def rad(t):
        basal_bulge = 0.30 * math.exp(-((t - 0.06) / 0.13) ** 2)
        middle_swell = 0.09 * math.sin(math.pi * t)
        upward_taper = -0.18 * t
        return radius * (1.02 + basal_bulge + middle_swell + upward_taper)

    stem = kit.tube(name, points, radius=rad, sides=18, material=mat, cap=False, roll=yaw)
    kit.displace(stem, lambda co, pco: (
        0.00026 * math.sin(math.atan2(pco.y, pco.x) * 16.0 + pco.z * 72.0)
        + 0.00032 * noise.noise(Vector((pco.x * 42.0, pco.y * 42.0, pco.z * 11.0 + 3.0)))
    ))
    return stem, points[-1], rad(0.8)


def _make_grass_blade(kit, name, mat, base, yaw, length, width, bend, rng):
    rows = 6
    verts = []
    faces = []
    uvs = []
    direction = Vector((math.cos(yaw), math.sin(yaw), 0.0))
    right = Vector((-math.sin(yaw), math.cos(yaw), 0.0))
    side = rng.uniform(-0.018, 0.018)
    for i in range(rows + 1):
        t = i / rows
        centre = (Vector(base) + direction * (bend * (t ** 1.45))
                  + right * (side * math.sin(math.pi * t))
                  + Vector((0, 0, length * (t - 0.18 * t * t))))
        w = width * (1.0 - t) ** 0.78
        cup = Vector((0, 0, -0.0012 * math.sin(math.pi * t)))
        verts.extend([centre - right * w * 0.5 + cup, centre + right * w * 0.5 + cup])
        uvs.extend([(0.0, t), (1.0, t)])
    for i in range(rows):
        a = i * 2
        faces.append((a, a + 2, a + 3, a + 1))
        faces.append((a + 1, a + 3, a + 2, a))
    obj = kit.mesh(name, verts, faces, material=mat)
    layer = obj.data.uv_layers["UVMap"].data
    for poly in obj.data.polygons:
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            layer[loop_index].uv = uvs[vertex_index]
    return obj


def _make_litter_blade(kit, name, mat, base, yaw, length, width, curl, rng):
    rows = 7
    verts, faces, uvs = [], [], []
    direction = Vector((math.cos(yaw), math.sin(yaw), 0.0))
    right = Vector((-math.sin(yaw), math.cos(yaw), 0.0))
    for i in range(rows + 1):
        t = i / rows
        centre = (Vector(base) + direction * (length * (t - 0.5))
                  + right * (curl * math.sin(math.pi * t))
                  + Vector((0, 0, 0.0012 + 0.0016 * math.sin(math.pi * t) ** 1.4)))
        w = width * (0.75 + 0.25 * math.sin(math.pi * t))
        verts.extend([centre - right * w * 0.5, centre + right * w * 0.5])
        uvs.extend([(0.0, t), (1.0, t)])
    for i in range(rows):
        a = i * 2
        faces.append((a, a + 2, a + 3, a + 1))
        faces.append((a + 1, a + 3, a + 2, a))
    obj = kit.mesh(name, verts, faces, material=mat)
    layer = obj.data.uv_layers["UVMap"].data
    for poly in obj.data.polygons:
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            layer[loop_index].uv = uvs[vertex_index]
    return obj


def _make_soil_clod(kit, name, mat, loc, scale, rng):
    obj = kit.sphere(name, 1.0, location=loc, material=mat, segments=8, rings=4,
                     scale=(scale[0], scale[1], scale[2]))
    kit.roughen(obj, strength=min(scale) * 0.28, scale=18.0, seed=int(rng.random() * 10000), subdivide=0)
    return obj


MUSHROOMS = [
    # x, y, stem_h, stem_r, cap_r, dome, thickness, yaw, age, tilt_x, tilt_y, open, broken
    (-0.116, -0.046, 0.018, 0.0072, 0.027, 0.020, 0.0085, 0.4, 0.10, 0.02, -0.03, False, False),
    (-0.044, -0.082, 0.034, 0.0080, 0.034, 0.026, 0.0090, 2.2, 0.18, -0.04, 0.03, False, False),
    (0.090, -0.065, 0.024, 0.0064, 0.024, 0.018, 0.0075, 4.4, 0.22, 0.00, 0.04, False, False),
    (0.018, 0.035, 0.055, 0.0092, 0.044, 0.010, 0.0052, 0.9, 0.42, -0.80, 1.20, True, False),
    (-0.082, 0.064, 0.053, 0.0087, 0.043, 0.008, 0.0050, -0.2, 0.78, -0.82, -0.24, True, False),
    (0.112, 0.052, 0.030, 0.0065, 0.030, 0.009, 0.0046, 3.4, 0.86, -0.10, 0.07, True, True),
]

CAP_KEYS = ["cap_offwhite", "cap_cream", "cap_grey", "cap_offwhite", "cap_buff", "cap_cream"]


def build(kit):
    rng = random.Random(SEED)
    mats = _graph_materials(kit)
    parts = []
    bases = []

    for index, spec in enumerate(MUSHROOMS):
        x, y, stem_h, stem_r, cap_r, dome, thick, yaw, age, tx, ty, open_cap, broken = spec
        lean = (0.004 * math.cos(yaw + 1.3) * (1.0 if open_cap else 0.45),
                0.004 * math.sin(yaw + 1.3) * (1.0 if open_cap else 0.45))
        stem, top, upper_radius = _make_stem(kit, f"Stem{index}", mats["stem"], (x, y, 0.0),
                                             stem_h, stem_r, yaw, rng, lean=lean)
        parts.append(stem)
        bases.append((x, y, stem_r))
        if not (index == 0):  # The half-emerged button has no visible ring.
            parts.append(_make_ring(kit, f"Ring{index}", mats["ring"],
                                    (x + lean[0] * 0.70, y + lean[1] * 0.70,
                                     stem_h * rng.uniform(0.55, 0.70)),
                                    upper_radius, yaw, rng))
        cap_loc = (top.x, top.y, top.z + thick * (0.20 if open_cap else 0.32))
        cap, matrix = _make_cap(kit, f"Cap{index}", mats, cap_loc, cap_r, dome, thick, yaw, age, rng,
                                tilt=(tx, ty), broken=broken, open_cap=open_cap,
                                cap_key=CAP_KEYS[index % len(CAP_KEYS)])
        parts.append(cap)
        if open_cap:
            gill_mat = mats["gill_pink"] if age < 0.55 else mats["gill_brown"]
            parts.append(_make_gills(kit, f"Gills{index}", gill_mat, matrix, cap_r,
                                     max(stem_r * 1.35, 0.010), thick, age, rng, broken=broken))

    # Dirt caught at bases: individual clods and moss pads only, no ground plane/disc.
    for i, (x, y, stem_r) in enumerate(bases):
        for c in range(rng.randint(2, 4)):
            ang = rng.random() * math.tau
            dist = rng.uniform(stem_r * 0.8, stem_r * 2.2)
            sx = rng.uniform(0.0035, 0.0085)
            sy = rng.uniform(0.003, 0.007)
            sz = rng.uniform(0.0016, 0.004)
            parts.append(_make_soil_clod(kit, f"Soil{i}_{c}", mats["soil"],
                                         (x + math.cos(ang) * dist, y + math.sin(ang) * dist, sz * 0.55),
                                         (sx, sy, sz), rng))
        if i in (1, 3, 4):
            parts.append(_make_soil_clod(kit, f"Moss{i}", mats["moss"],
                                         (x + rng.uniform(-0.012, 0.012), y + rng.uniform(-0.010, 0.010),
                                          0.0017),
                                         (rng.uniform(0.006, 0.011), rng.uniform(0.003, 0.006), 0.0016), rng))
        if i in (0, 1, 2):
            # Half-emerged buttons push through a broken collar of soil and moss.
            for m in range(3):
                ang = rng.random() * math.tau
                parts.append(_make_soil_clod(kit, f"ButtonMound{i}_{m}", mats["soil"],
                                             (x + math.cos(ang) * stem_r * rng.uniform(1.7, 2.6),
                                              y + math.sin(ang) * stem_r * rng.uniform(1.7, 2.6), 0.0014),
                                             (rng.uniform(0.006, 0.011), rng.uniform(0.0035, 0.0075),
                                              rng.uniform(0.0015, 0.0030)), rng))

    # Short pasture grass blades woven between stems.
    grass_roots = [(-0.140, -0.070), (-0.128, 0.004), (-0.103, 0.090), (-0.065, -0.108),
                   (-0.023, -0.112), (-0.016, 0.102), (0.028, -0.094), (0.046, 0.088),
                   (0.072, -0.024), (0.098, -0.104), (0.142, -0.028), (0.136, 0.080),
                   (-0.060, 0.020), (0.005, -0.025), (0.078, 0.038), (-0.118, 0.052),
                   (-0.136, 0.030), (-0.086, -0.006), (-0.038, 0.058), (0.025, 0.012),
                   (0.060, -0.112), (0.116, 0.018), (0.138, 0.044), (-0.010, -0.060)]
    for i, root in enumerate(grass_roots):
        for j in range(1 if i % 3 else 2):
            yaw = rng.uniform(-math.pi, math.pi)
            parts.append(_make_grass_blade(kit, f"Grass{i}_{j}", mats["grass"],
                                           (root[0] + rng.uniform(-0.008, 0.008),
                                            root[1] + rng.uniform(-0.008, 0.008), 0.0),
                                           yaw, rng.uniform(0.035, 0.090), rng.uniform(0.0020, 0.0048),
                                           rng.uniform(0.010, 0.035), rng))
    for i, (x, y, yaw, length) in enumerate(((-0.123, 0.028, 0.7, 0.060),
                                             (-0.030, -0.020, -0.45, 0.075),
                                             (0.088, 0.010, 2.6, 0.052))):
        parts.append(_make_litter_blade(kit, f"DeadLitter{i}", mats["dead"], (x, y, 0.0),
                                        yaw, length, rng.uniform(0.0035, 0.0060),
                                        rng.uniform(-0.014, 0.014), rng))

    lod0 = kit.join(parts, "SM_FieldMushrooms", pivot="base", unwrap=True, reshade=True, smooth_angle=64)
    lod0.data.transform(Matrix.Translation((0, 0, -0.004)))
    lod0.data.update()
    lods = [lod0]
    for name, ratio in (("SM_FieldMushrooms_LOD1", 0.46), ("SM_FieldMushrooms_LOD2", 0.18)):
        lod = lod0.copy()
        lod.data = lod0.data.copy()
        lod.name = lod.data.name = name
        kit._link(lod)
        mod = lod.modifiers.new("BudgetDecimate", "DECIMATE")
        mod.ratio = ratio
        mod.use_collapse_triangulate = True
        kit.apply_modifiers(lod)
        lod.data.shade_smooth()
        lod.data.set_sharp_from_angle(angle=math.radians(70))
        lods.append(lod)
    return lods
