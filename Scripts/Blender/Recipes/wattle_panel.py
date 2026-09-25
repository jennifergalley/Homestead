"""Woven hazel wattle wall panel with a partial daub face.

Original procedural geometry and shader networks only: seven bark-on stakes,
tightly packed alternating withies, visible mid-panel splices, pale cut ends,
and a lower in-progress clay-and-straw daub layer on one face.
"""
import math
import random

import bpy
from mathutils import Vector, noise


NAME = "WattlePanel"
DESCRIPTION = "Primitive homestead wall section: woven hazel wattle with partial clay-straw daub."
COLLISION = "box"
TRIANGLE_BUDGET = 250000
BAKE = {"size": 4096, "samples": 96}
BEAUTY = {"pose": (0, 0, 180), "focus": (0.25, 0.03, 0.82)}

WIDTH = 2.0
HEIGHT = 1.80
FRONT_Y = 1.0
STAKE_COUNT = 7
STAKE_SPACING = 0.30
STAKE_XS = [(-STAKE_SPACING * (STAKE_COUNT - 1) / 2) + i * STAKE_SPACING for i in range(STAKE_COUNT)]
ROW_START = 0.045
ROW_STEP = 0.0146
ROW_COUNT = 112


def hazel_bark(name, low=(0.055, 0.046, 0.037), mid=(0.145, 0.120, 0.088),
               high=(0.255, 0.230, 0.175), green=0.0, red=0.0):
    """Smooth grey-brown hazel bark with fine striations and pale lenticel flecks."""
    g = bpy.data.materials.get(name)
    if g:
        return g
    graph = __import__("homestead_materials").Graph(name)
    p = graph.coord()
    x, y, z = graph.separate(p)
    angle = graph.math("ARCTAN2", y, x)
    vertical = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 8.0),
                                         graph.math("MULTIPLY", z, 2.7), 0.0),
                           scale=15.0, detail=7.0, roughness=0.64).outputs["Fac"]
    fine = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 34.0),
                                     graph.math("MULTIPLY", z, 10.0), 0.0),
                       scale=34.0, detail=5.0, roughness=0.62).outputs["Fac"]
    bands = graph.wave(graph.combine(angle, 0.0, graph.math("MULTIPLY", z, 0.55)),
                       scale=8.0, distortion=3.0, detail=2.0, kind="BANDS", direction="Z").outputs["Fac"]
    tone = graph.math("ADD", graph.math("MULTIPLY", vertical, 0.52),
                      graph.math("MULTIPLY", fine, 0.30))
    tone = graph.math("ADD", tone, graph.math("MULTIPLY", bands, 0.18))
    def shifted(color):
        return (min(0.8, color[0] + red * 0.014),
                min(0.8, color[1] + green * 0.016),
                max(0.015, color[2] - red * 0.006 + green * 0.004))

    bark = graph.ramp(tone, [(0.18, shifted(low)), (0.50, shifted(mid)), (0.88, shifted(high))])
    dash_phase = graph.math("ADD", graph.math("MULTIPLY", z, 80.0),
                            graph.math("MULTIPLY", angle, 0.55))
    dash_rows = graph.remap(graph.math("ABSOLUTE", graph.math("SINE", dash_phase)), 0.93, 1.0)
    dash_breakup = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 14.0),
                                             graph.math("MULTIPLY", z, 37.0), 0.0),
                               scale=20.0, detail=2.0, roughness=0.48).outputs["Fac"]
    flecks = graph.math("MULTIPLY", dash_rows, graph.remap(dash_breakup, 0.58, 0.76, 0.0, 0.55))
    color = graph.mix(bark, (0.50, 0.47, 0.36), flecks)
    dark_striae = graph.remap(fine, 0.14, 0.34, 0.26, 0.0)
    color = graph.mix(color, (0.042, 0.035, 0.028), dark_striae)
    grime = graph.noise(p, scale=42.0, detail=4.0, roughness=0.7).outputs["Fac"]
    color = graph.mix(color, (0.050, 0.040, 0.032), graph.remap(grime, 0.35, 0.82, 0.0, 0.30))
    stains = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 5.0),
                                       graph.math("MULTIPLY", z, 5.0), 1.7),
                         scale=9.0, detail=5.0, roughness=0.7).outputs["Fac"]
    color = graph.mix(color, (0.080, 0.065, 0.050), graph.remap(stains, 0.42, 0.74, 0.0, 0.36))
    micro = graph.noise(graph.combine(graph.math("MULTIPLY", angle, 52.0),
                                      graph.math("MULTIPLY", z, 28.0), 2.1),
                        scale=58.0, detail=4.0, roughness=0.72).outputs["Fac"]
    graph.set("Base Color", color)
    rough = graph.math("ADD", graph.remap(fine, 0.25, 0.75, 0.81, 0.93),
                       graph.math("MULTIPLY", graph.remap(micro, 0.25, 0.78, -0.035, 0.045), 1.0))
    graph.set("Roughness", rough)
    height = graph.math("ADD", graph.math("MULTIPLY", fine, 0.40),
                        graph.math("MULTIPLY", vertical, 0.45))
    height = graph.math("ADD", height, graph.math("MULTIPLY", micro, 0.26))
    graph.set("Normal", graph.bump(height, strength=0.78, distance=0.0022))
    return graph.mat


def daub_material(name):
    """Grey-brown clay with dried crust, damp patches, fine straw and sparse hairline cracks."""
    mat = bpy.data.materials.get(name)
    if mat:
        return mat
    graph = __import__("homestead_materials").Graph(name)
    p = graph.coord()
    x, y, z = graph.separate(p)
    lumps = graph.noise(graph.combine(graph.math("MULTIPLY", x, 0.8),
                                      graph.math("MULTIPLY", y, 2.0),
                                      graph.math("MULTIPLY", z, 0.9)),
                        scale=10.0, detail=6.0, roughness=0.58).outputs["Fac"]
    smear = graph.wave(graph.combine(graph.math("MULTIPLY", x, 0.5), y,
                                     graph.math("MULTIPLY", z, 1.2)),
                       scale=7.0, distortion=9.0, detail=3.0, kind="BANDS",
                       direction="X").outputs["Fac"]
    damp_bottom = graph.remap(z, 0.02, 0.52, 0.50, 0.0)
    damp_patches = graph.noise(graph.combine(graph.math("MULTIPLY", x, 1.1),
                                             graph.math("MULTIPLY", y, 0.7),
                                             graph.math("MULTIPLY", z, 1.6)),
                               scale=5.5, detail=5.0, roughness=0.62).outputs["Fac"]
    base = graph.ramp(lumps, [
        (0.16, (0.205, 0.160, 0.110)),
        (0.56, (0.300, 0.240, 0.170)),
        (0.90, (0.440, 0.380, 0.285)),
    ])
    base = graph.mix(base, (0.155, 0.120, 0.082), damp_bottom)
    base = graph.mix(base, (0.180, 0.135, 0.092), graph.remap(damp_patches, 0.58, 0.78, 0.0, 0.32))
    base = graph.mix(base, (0.365, 0.300, 0.215), graph.remap(smear, 0.56, 0.86, 0.0, 0.18))
    straw_noise = graph.noise(graph.combine(graph.math("MULTIPLY", x, 44.0),
                                            graph.math("MULTIPLY", z, 8.0),
                                            graph.math("MULTIPLY", y, 7.0)),
                              scale=7.0, detail=2.0, roughness=0.55).outputs["Fac"]
    straw_mask = graph.remap(straw_noise, 0.815, 0.845, 0.0, 0.22)
    base = graph.mix(base, (0.390, 0.320, 0.210), straw_mask)
    hair = graph.voronoi(graph.combine(graph.math("MULTIPLY", x, 0.8), y,
                                       graph.math("MULTIPLY", z, 1.3)),
                         scale=5.4, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack_gate = graph.noise(graph.combine(graph.math("MULTIPLY", x, 2.0), y,
                                           graph.math("MULTIPLY", z, 2.0)),
                             scale=4.0, detail=2.0).outputs["Fac"]
    crack_mask = graph.math("MULTIPLY", graph.remap(hair, 0.0, 0.0022, 0.16, 0.0),
                            graph.remap(crack_gate, 0.66, 0.78, 0.0, 1.0))
    base = graph.mix(base, (0.090, 0.070, 0.050), crack_mask)
    graph.set("Base Color", base)
    graph.set("Roughness", graph.remap(damp_bottom, 0.0, 0.55, 0.94, 0.84))
    height = graph.math("ADD", graph.math("MULTIPLY", lumps, 0.75),
                        graph.math("MULTIPLY", smear, 0.25))
    height = graph.math("SUBTRACT", height, graph.math("MULTIPLY", crack_mask, 0.25))
    graph.set("Normal", graph.bump(height, strength=0.55, distance=0.004))
    return graph.mat


def stake_axis(x, height, bow_x, bow_y, lean_x, lean_y, seed):
    def axis(t):
        z = -0.075 + (height + 0.075) * t
        wind = noise.noise(Vector((seed, t * 2.1, 0.0)))
        return Vector((
            x + lean_x * t + bow_x * math.sin(math.pi * t) + 0.004 * wind,
            lean_y * t + bow_y * math.sin(math.pi * (t + 0.13)),
            z,
        ))
    return axis


def make_stake(kit, index, x, bark, endwood, rng):
    height = HEIGHT + rng.uniform(-0.045, 0.055)
    base_radius = rng.uniform(0.018, 0.024)
    top_radius = base_radius * rng.uniform(0.72, 0.90)
    knot_t = rng.uniform(0.22, 0.78)
    bow_x = rng.uniform(-0.025, 0.025)
    bow_y = rng.uniform(-0.015, 0.015)
    lean_x = rng.uniform(-0.015, 0.015)
    lean_y = rng.uniform(-0.012, 0.012)
    axis = stake_axis(x, height, bow_x, bow_y, lean_x, lean_y, index + 11)
    points = [axis(i / 90) for i in range(91)]

    def radius(t):
        knot = 0.003 * math.exp(-((t - knot_t) / 0.045) ** 2)
        return base_radius + (top_radius - base_radius) * t + knot + 0.0012 * math.sin(5.0 * math.pi * t + index)

    stake = kit.tube(f"Stake_{index}", points, radius=radius, sides=18, material=bark, cap=False)

    def relief(co, pco):
        z = pco.z
        angle = math.atan2(pco.y, pco.x)
        long = noise.noise(Vector((math.cos(angle) * 4.0 + index, math.sin(angle) * 4.0, z * 13.0)))
        bumps = noise.noise(Vector((pco.x * 95.0 + index, pco.y * 95.0, z * 9.0)))
        return long * 0.0010 + bumps * 0.00065

    kit.displace(stake, relief)
    top = points[-1]
    top_parts = [stake]
    if index in (1, 5):
        point = kit.cylinder(f"Stake_{index}_AxePoint", radius(1.0) * 0.98, 0.060,
                             location=top + Vector((0, 0, 0.030)), material=bark,
                             sides=18, radius_top=radius(1.0) * 0.18)
        kit.roughen(point, strength=0.0025, scale=40.0, seed=90 + index)
        scar = kit.cylinder(f"Stake_{index}_FreshAxeFacet", radius(1.0) * 0.42, 0.003,
                            location=top + Vector((0.004 * (-1) ** index, 0.006, 0.052)),
                            material=endwood, sides=10)
        scar.rotation_euler = (math.radians(66), math.radians(12 * (-1) ** index), rng.uniform(-0.4, 0.4))
        top_parts.extend([point, scar])
    else:
        cap = kit.cylinder(f"Stake_{index}_CutTop", radius(1.0) * 0.96, 0.0045,
                           location=top + Vector((0, 0, 0.0016)), material=endwood, sides=16)
        cap.rotation_euler = (math.radians(rng.uniform(-7, 7)), math.radians(rng.uniform(-10, 10)),
                              rng.uniform(-0.2, 0.2))
        top_parts.append(cap)
    return top_parts, base_radius


def rod_center_y(row_index, x, amplitude, rng_offset=0.0):
    signs = [FRONT_Y * (1 if (i + row_index) % 2 == 0 else -1) for i in range(STAKE_COUNT)]
    if x <= STAKE_XS[0]:
        y = signs[0] * amplitude
    elif x >= STAKE_XS[-1]:
        y = signs[-1] * amplitude
    else:
        right = next(i for i, sx in enumerate(STAKE_XS) if sx >= x)
        left = right - 1
        a, b = STAKE_XS[left], STAKE_XS[right]
        t = (x - a) / (b - a)
        smooth = 0.5 - 0.5 * math.cos(math.pi * t)
        y = amplitude * ((1.0 - smooth) * signs[left] + smooth * signs[right])
    return y + 0.0025 * math.sin(10.5 * x + row_index * 0.47 + rng_offset)


def withy_path(row_index, x0, x1, z, radius, stake_radii, seed, sag=1.0):
    points = []
    samples = max(28, int(abs(x1 - x0) / 0.030))
    amplitude = max(stake_radii) + radius + 0.006
    for i in range(samples + 1):
        t = i / samples
        x = x0 + (x1 - x0) * t
        span_t = (x - STAKE_XS[0]) / (STAKE_XS[-1] - STAKE_XS[0])
        y = rod_center_y(row_index, x, amplitude, seed)
        z_wobble = (0.0048 * noise.noise(Vector((x * 3.2, row_index * 0.21, seed))) -
                    0.0052 * sag * math.sin(math.pi * max(0.0, min(1.0, span_t))))
        x_wobble = 0.0045 * noise.noise(Vector((row_index * 0.19, t * 3.0, seed)))
        points.append(Vector((x + x_wobble, y, z + z_wobble)))
    return points


def tube_end_cap(kit, name, point, tangent, radius, mat, sides=10):
    cap = kit.cylinder(name, radius * 0.97, 0.0024, location=point, material=mat, sides=sides)
    cap.rotation_euler = Vector(tangent).normalized().to_track_quat("Z", "Y").to_euler()
    return cap


def nearest_path_point(points, x):
    index = min(range(1, len(points) - 1), key=lambda i: abs(points[i].x - x))
    tangent = (points[index + 1] - points[index - 1]).normalized()
    return points[index], tangent


def make_bend_scuffs(kit, row, seg_i, points, radii, mat, rng):
    parts = []
    for stake_i, stake_x in enumerate(STAKE_XS):
        front = (stake_i + row) % 2 == 0
        if not front or rng.random() > 0.07:
            continue
        point, tangent = nearest_path_point(points, stake_x)
        length = rng.uniform(0.006, 0.014)
        surface = point + Vector((0.0, FRONT_Y * (radii[len(radii) // 2] + 0.00025), rng.uniform(-0.0008, 0.0008)))
        pts = [surface - tangent * length * 0.5, surface, surface + tangent * length * 0.5]
        parts.append(kit.tube(f"Withy_{row:02d}_{seg_i}_Scuff_{stake_i}", pts,
                              radius=rng.uniform(0.00030, 0.00052), sides=5, material=mat, cap=True))
    return parts


def make_withies(kit, barks, endwood, scuff_mat, stake_radii, rng):
    parts = []
    for row in range(ROW_COUNT):
        z = ROW_START + row * ROW_STEP + rng.uniform(-0.0019, 0.0019)
        radius = rng.uniform(0.0060, 0.0125) * (1.0 - 0.035 * row / ROW_COUNT)
        over_l = rng.uniform(0.035, 0.115)
        over_r = rng.uniform(0.035, 0.120)
        left, right = -WIDTH / 2 - over_l, WIDTH / 2 + over_r
        butt_left = row % 2 == 0
        segments = [(left, right)]
        if row in (9, 17, 31, 44, 58, 73, 84):
            joint = rng.choice(STAKE_XS[1:-1]) + rng.uniform(-0.055, 0.055)
            overlap = rng.uniform(0.045, 0.080)
            segments = [(left, joint + overlap), (joint - overlap, right)]
        for seg_i, (a, b) in enumerate(segments):
            path_a, path_b = (a, b) if butt_left else (b, a)
            pts = withy_path(row, path_a, path_b, z + seg_i * radius * 0.42, radius, stake_radii,
                             seed=20.0 + row * 1.37 + seg_i * 6.1, sag=rng.uniform(0.5, 1.3))
            r0 = radius * rng.uniform(1.02, 1.20)
            r1 = radius * rng.uniform(0.58, 0.84)
            radii = []
            for i in range(len(pts)):
                t = i / (len(pts) - 1)
                radii.append((1 - t) * r0 + t * r1 + 0.0007 * math.sin(2 * math.pi * t + row))
            mat = barks[(row * 3 + seg_i + rng.randrange(len(barks))) % len(barks)]
            rod = kit.tube(f"Withy_{row:02d}_{seg_i}", pts, radii=radii, sides=12, material=mat, cap=False)

            def nick(co, pco, row=row):
                fine = noise.noise(Vector((pco.x * 120.0 + row, pco.y * 120.0, pco.z * 8.0)))
                long = noise.noise(Vector((pco.x * 36.0, pco.y * 36.0 + row, pco.z * 1.7)))
                return fine * 0.00075 + long * 0.00105

            kit.displace(rod, nick)
            parts.append(rod)
            parts.append(tube_end_cap(kit, f"Withy_{row:02d}_{seg_i}_CutA", pts[0], pts[0] - pts[1],
                                      radii[0], endwood, sides=12))
            parts.append(tube_end_cap(kit, f"Withy_{row:02d}_{seg_i}_CutB", pts[-1], pts[-1] - pts[-2],
                                      radii[-1], endwood, sides=12))
            parts.extend(make_bend_scuffs(kit, row, seg_i, pts, radii, scuff_mat, rng))
    return parts


def make_daub(kit, mat, rng):
    cols, rows = 58, 36
    z_min, z_nominal_top = 0.015, 0.68
    front_y = FRONT_Y * 0.074
    back_y = FRONT_Y * 0.014
    top_profile = []
    vertices = []
    for layer_y in (front_y, back_y):
        for j in range(rows + 1):
            v = j / rows
            for i in range(cols + 1):
                u = i / cols
                left = (-WIDTH / 2 + 0.070 +
                        0.050 * noise.noise(Vector((v * 2.7, 4.1, 0.0))) +
                        0.020 * noise.noise(Vector((v * 21.0, 6.2, 0.0))))
                right = WIDTH / 2 - 0.060 + 0.030 * noise.noise(Vector((v * 3.1, 8.2, 0.0)))
                x = left + u * (right - left)
                side_lump = 0.030 * math.sin(v * 15.0 + 1.8) + 0.018 * noise.noise(Vector((v * 35.0, 2.0, 0.0)))
                if u < 0.12:
                    x += (0.12 - u) / 0.12 * side_lump
                elif u > 0.92:
                    x -= (u - 0.92) / 0.08 * 0.010 * math.sin(v * 13.0 + 0.4)
                edge = (0.070 * math.sin(8.6 * u + 0.9) +
                        0.045 * noise.noise(Vector((u * 4.0, 1.7, 0.0))) +
                        0.020 * noise.noise(Vector((u * 23.0, 9.7, 0.0))))
                local_top = z_nominal_top + edge
                z = z_min + v * (local_top - z_min)
                ragged_side = (abs(u - 0.5) * 2) ** 5
                ridge_phase = 2 * math.pi * ((z - ROW_START) / ROW_STEP)
                ridge = max(0.0, 0.5 + 0.5 * math.cos(ridge_phase)) ** 2.8
                smear = 0.006 * math.sin(38.0 * x + 8.0 * z + 0.5)
                lobe = 0.017 * noise.noise(Vector((u * 5.5, v * 6.5, 1.0)))
                finger = 0.006 * noise.noise(Vector((u * 18.0, v * 2.3, 6.0)))
                edge_fade = min(1.0, max(0.0, u / 0.14), max(0.0, (1.0 - u) / 0.10),
                                max(0.0, (1.0 - v) / 0.12))
                y_lump = ((0.014 * ridge + lobe + smear + finger) * edge_fade) if layer_y == front_y else -0.004 * ridge
                torn = 0.0
                if v > 0.90:
                    torn += (v - 0.90) / 0.10 * 0.010 * noise.noise(Vector((u * 47.0, 3.0, 0.0)))
                y = layer_y + FRONT_Y * (y_lump - ragged_side * 0.008 + torn)
                if layer_y == front_y:
                    y = back_y + (y - back_y) * max(0.18, edge_fade)
                vertices.append((x, y, z))
                if layer_y == front_y and j == rows:
                    top_profile.append((x, y, z))
    faces = []
    stride = cols + 1
    layer = stride * (rows + 1)
    for j in range(rows):
        for i in range(cols):
            a = j * stride + i
            faces.append((a, a + 1, a + 1 + stride, a + stride))
            b = layer + a
            faces.append((b + stride, b + 1 + stride, b + 1, b))
    for i in range(cols):
        faces.append((i, layer + i, layer + i + 1, i + 1))
        t0 = rows * stride + i
        faces.append((t0, t0 + 1, layer + t0 + 1, layer + t0))
    for j in range(rows):
        l0 = j * stride
        r0 = j * stride + cols
        faces.append((l0, l0 + stride, layer + l0 + stride, layer + l0))
        faces.append((r0 + stride, r0, layer + r0, layer + r0 + stride))
    daub = kit.mesh("LowerDaub", vertices, faces, material=mat)

    def lumpy(co, pco):
        n1 = noise.noise(Vector((pco.x * 6.0, pco.z * 8.0, 2.2)))
        n2 = noise.noise(Vector((pco.x * 23.0, pco.z * 19.0, 8.1)))
        return 0.0040 * n1 + 0.0016 * n2

    kit.displace(daub, lumpy)
    parts = [daub]
    for i in range(14):
        x, y, z = top_profile[int((i + 0.5) * len(top_profile) / 14)]
        z += rng.uniform(0.010, 0.055)
        length = rng.uniform(0.025, 0.070)
        pts = []
        for s in range(4):
            t = s / 3
            pts.append(Vector((x + (t - 0.5) * length,
                               y + rng.uniform(-0.002, 0.003),
                               z + 0.004 * math.sin(math.pi * t))))
        parts.append(kit.tube(f"DaubPressedRemnant_{i}", pts, radius=rng.uniform(0.0035, 0.0070),
                              sides=7, material=mat, cap=True))
    return parts


def make_loose_fibres(kit, mat, rng):
    parts = []
    for i in range(52):
        z = rng.uniform(0.05, 0.68)
        x = rng.uniform(-0.88, 0.88)
        length = rng.uniform(0.018, 0.060)
        angle = rng.uniform(-0.7, 0.7)
        y = FRONT_Y * rng.uniform(0.074, 0.092)
        pts = []
        for s in range(5):
            t = s / 4
            dx = (t - 0.5) * length * math.cos(angle)
            dz = (t - 0.5) * length * math.sin(angle)
            pts.append(Vector((x + dx, y + 0.001 * math.sin(t * math.pi), z + dz)))
        parts.append(kit.tube(f"DaubStraw_{i}", pts, radius=rng.uniform(0.00030, 0.00062),
                              sides=5, material=mat, cap=True))
    return parts


def build(kit):
    rng = random.Random(87231)
    bark_variants = [
        hazel_bark("M_WattleHazelBarkWarm", low=(0.058, 0.046, 0.034), mid=(0.145, 0.112, 0.076),
                   high=(0.250, 0.205, 0.145), red=0.10),
        hazel_bark("M_WattleHazelBarkWarm2", low=(0.052, 0.045, 0.035), mid=(0.132, 0.112, 0.083),
                   high=(0.230, 0.205, 0.160)),
        hazel_bark("M_WattleHazelBarkGrey", low=(0.050, 0.046, 0.040), mid=(0.132, 0.125, 0.104),
                   high=(0.235, 0.226, 0.190)),
        hazel_bark("M_WattleHazelBarkOlive", low=(0.048, 0.050, 0.038), mid=(0.118, 0.124, 0.088),
                   high=(0.210, 0.222, 0.165), green=0.12),
        hazel_bark("M_WattleHazelBarkGrey2", low=(0.046, 0.044, 0.038), mid=(0.120, 0.116, 0.098),
                   high=(0.220, 0.215, 0.184)),
        hazel_bark("M_WattleHazelBarkRed", low=(0.058, 0.044, 0.038), mid=(0.142, 0.108, 0.086),
                   high=(0.238, 0.190, 0.150), red=0.16),
        hazel_bark("M_WattleHazelBarkGrey3", low=(0.050, 0.047, 0.041), mid=(0.128, 0.123, 0.102),
                   high=(0.226, 0.220, 0.188)),
        hazel_bark("M_WattleHazelBarkWarm3", low=(0.055, 0.047, 0.036), mid=(0.136, 0.116, 0.082),
                   high=(0.240, 0.210, 0.155)),
    ]
    endwood = kit.mats.wood("M_WattleCutEnd", light=(0.63, 0.50, 0.31), dark=(0.35, 0.22, 0.10),
                            grain=0.42, roughness=0.72, weathering=0.1, grime=0.05, seed=4.0)
    scuff = kit.material("M_WattleRubbedScuffs", (0.58, 0.55, 0.36), roughness=0.82)
    daub = daub_material("M_WattlePressedDaub")
    straw = kit.material("M_WattleLooseStraw", (0.34, 0.275, 0.180), roughness=0.88)

    parts = []
    stake_radii = []
    for i, x in enumerate(STAKE_XS):
        stake_parts, radius = make_stake(kit, i, x, bark_variants[i % len(bark_variants)], endwood, rng)
        parts.extend(stake_parts)
        stake_radii.append(radius)
    parts.extend(make_withies(kit, bark_variants, endwood, scuff, stake_radii, rng))
    parts.extend(make_daub(kit, daub, rng))
    parts.extend(make_loose_fibres(kit, straw, rng))
    return kit.join(parts, "SM_WattlePanel", pivot=None, unwrap=False, reshade=True, smooth_angle=62)
