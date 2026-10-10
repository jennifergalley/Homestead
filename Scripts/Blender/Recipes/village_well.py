"""Village well: a dressed-granite wishing-well ring under a small oak-shingled roof, with a windlass,
rope and a hooped oak bucket.

Real-object research (written before modeling):
- English village wells of granite country are a round or octagonal "kerb" of squared and dressed
  stones laid in courses, with a flat coping ring on top (often a dozen pieces cut to the curve)
  that overhangs both faces a few centimetres so rain sheds clear and nobody trips on the joint.
  The courses are laid with broken joints, set in lime mortar, and the lowest courses are buried
  so the well keeps its footing on a slope. The inside is wet, dark and green below the lip and
  black at the waterline; the outside is lichened, with damp-darkened stone at the foot.
- The classic roofed well has two oak posts either side of the kerb carrying a ridge beam, with
  diagonal knee braces to resist racking, a pair of rafters at each bay, a plank deck and oak shakes
  or shingles laid in overlapping courses. Shakes are 30-40 cm long with 12-15 cm showing, rounded
  at the butt, and each course rises over the heads of the one below, so the roof looks scaled.
  Barge boards finish the gable ends and a ridge cap of two boards seals the apex.
- The windlass is a round oak roller (a barrel) between the posts, turned in iron-collared
  journals that pass through them, with a cranked handle (arm plus a grip) on one end. The rope is
  laid in turns onto the roller in the order it is wound and leaves from the underside, so the bucket
  hangs plumb over the water, away from the crank side.
- The bucket is a coopered oak bucket (about 28 cm high, 28 cm across the lip): tapered staves held by
  two iron hoops, a flat floor, and an iron bail on ears at the rim, dark and wet inside.
- Sizes here: ring 1.5 m across, 0.5 m inside radius, rim 0.9 m above ground, water 0.5 m below the rim,
  eave 2.0 m, ridge 2.7 m.

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Units are metres, Z up. SM_VillageWell PIVOT: centre of the ring at ground level (z = 0), with the ring's
buried skirt below it (to z = -0.30) so it keeps its footing on slopes. The ridge, posts and windlass run
along X, the crank is at +X. SM_VillageWell_LOD1 / _LOD2 are decimated copies sharing the UVs and textures.
"""
import importlib.util
import math
import os
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector, noise

RECIPES = Path(__file__).resolve().parent
for path in (str(RECIPES), str(RECIPES.parent)):
    if path not in sys.path:
        sys.path.insert(0, path)
from stone_building import masonry  # noqa: E402
from stone_building.wall import stone_material  # noqa: E402

_spec = importlib.util.spec_from_file_location("farm_common", Path(__file__).with_name("farm") / "common.py")
common = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(common)

NAME = "VillageWell"
DESCRIPTION = ("Roofed granite well: dressed-stone kerb with coping, dark water, two oak posts with braces, "
               "ridge beam and shingle roof, windlass with crank, rope and hooped oak bucket; pivot = ring "
               "centre at ground (original).")
COLLISION = "box"
TRIANGLE_BUDGET = 30000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 1.25), "views": ["hero", "detail", "eye"], "eye_distance": 5.2}
NOTES = {"SM_VillageWell": ("Pivot = ring centre at ground level (z = 0); skirt buried to z = -0.30. Rim top 0.90 m, "
                            "water 0.40 m, eave 2.0 m, ridge 2.7 m. Ridge/posts/windlass along X, crank at +X. "
                            "Import with complex-as-simple collision (see import_props.COLLISION_OVERRIDES).")}
REPORT = {"pivot": "ring centre at ground level, skirt below", "rim_height_m": 0.90, "water_depth_below_rim_m": 0.5,
          "eave_height_m": 2.0, "ridge_height_m": 2.7, "collision": "box in recipe; complex-as-simple via import_props"}

# --- ring (metres) ---
R_IN = 0.50
R_OUT = 0.75
R_MID = 0.5 * (R_IN + R_OUT)
SKIRT = -0.30
RIM = 0.90
COURSES = [-0.30, -0.085, 0.125, 0.335, 0.540, 0.745]
COURSE_OFFSET_DEG = [0.0, 15.0, 6.0, 21.0, 9.0]
PER_COURSE = 12
GAP = 0.012
COPING_BOTTOM = 0.745
COPING_IN, COPING_OUT = 0.475, 0.780
COPING_COUNT = 9
WATER_Z = 0.40
MORTAR_OUT = R_OUT - 0.012
MORTAR_IN = R_IN + 0.012
BLOCK_STEP = 0.12

# --- frame and roof ---
POST_X = 0.62
POST_BOTTOM, POST_TOP = RIM - 0.05, 2.56
RIDGE_Z = 2.70
SLOPE = 0.74
ALPHA = math.atan(SLOPE)
COS_A, SIN_A = math.cos(ALPHA), math.sin(ALPHA)
HALF_RUN = 0.90
SLOPE_LEN = HALF_RUN / COS_A
ROOF_HALF = 1.0
AXLE_Z = 1.70
ROLLER_R = 0.075
ROPE_R = 0.014
ROPE_WRAP = ROLLER_R + ROPE_R
BUCKET_Y = ROPE_WRAP


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


# ---------------------------------------------------------------------------------------------- stone ring
def bend(points, r_mid, theta_c):
    """Wrap a block built flat (x tangential, y radial about r_mid, z up) onto the ring."""
    x, y, z = points[:, 0], points[:, 1], points[:, 2]
    r = r_mid + y
    angle = theta_c - x / r_mid
    return np.stack([r * np.cos(angle), r * np.sin(angle), z], axis=1)


def add_curved_block(m, rng, r_mid, theta_c, width, z0, z1, thick_in, thick_out, skip, radius, bulge):
    lo = (-width / 2, -thick_in, z0)
    hi = (width / 2, thick_out, z1)
    m.block(lo, hi, mat=0, radius=radius, relief=0.0035, step=BLOCK_STEP, skip=skip, bulge=bulge)
    index = len(m.verts) - 1
    bent = bend(m.verts[index], r_mid, theta_c)
    m.verts[index] = bent
    m.pcoords[index] = bent.copy()
    radial = np.hypot(bent[:, 0], bent[:, 1])
    z = bent[:, 2]
    tint = m.tints[index]
    inner = radial < R_IN + 0.035
    wet = 0.42 + 0.58 * np.clip((z - 0.36) / 0.30, 0.0, 1.0)
    tint[inner] *= wet[inner, None]
    outer = radial > R_OUT - 0.04
    damp = 0.78 + 0.22 * np.clip(z / 0.30, 0.0, 1.0)
    tint[outer] *= damp[outer, None]


def build_ring(m, rng):
    for course in range(5):
        z0, z1 = COURSES[course], COURSES[course + 1] - GAP
        cuts = []
        for k in range(PER_COURSE):
            base = math.radians(COURSE_OFFSET_DEG[course] + k * 360.0 / PER_COURSE)
            cuts.append(base + rng.uniform(-0.12, 0.12) * math.tau / PER_COURSE)
        for k in range(PER_COURSE):
            a0, a1 = cuts[k], cuts[(k + 1) % PER_COURSE] + (math.tau if k == PER_COURSE - 1 else 0.0)
            width = (a1 - a0) * R_MID - GAP
            skip = (("x", -1), ("x", 1), ("z", 1)) + ((("z", -1),) if course == 0 else ())
            tin = R_MID - R_IN + rng.uniform(-0.004, 0.004)
            tout = R_OUT - R_MID + rng.uniform(-0.004, 0.004)
            add_curved_block(m, rng, R_MID, 0.5 * (a0 + a1), width, z0, z1, tin, tout, skip,
                             radius=0.017, bulge=0.004)
    # Coping: nine slightly overhanging pieces, each cut to the curve, with a worn, domed top.
    cop_mid = 0.5 * (COPING_IN + COPING_OUT)
    start = rng.uniform(0.0, 0.3)
    for k in range(COPING_COUNT):
        a0 = start + k * math.tau / COPING_COUNT + rng.uniform(-0.03, 0.03)
        a1 = start + (k + 1) * math.tau / COPING_COUNT + (rng.uniform(-0.03, 0.03) if k < COPING_COUNT - 1 else 0.0)
        width = (a1 - a0) * cop_mid - GAP
        add_curved_block(m, rng, cop_mid, 0.5 * (a0 + a1), width, COPING_BOTTOM, RIM,
                         cop_mid - COPING_IN, COPING_OUT - cop_mid, (("x", -1), ("x", 1)),
                         radius=0.026, bulge=0.006)


def add_mortar(m):
    """Lime-mortar core behind the stones, so joints read as recessed mortar rather than see-through."""
    sides = 96
    for radius, z_rows, inward in ((MORTAR_OUT, (-0.28, 0.20, 0.745), False), (MORTAR_IN, (-0.28, 0.20, 0.745), True),
                                   (COPING_OUT - 0.012, (0.74, RIM - 0.012), False),
                                   (COPING_IN + 0.012, (0.74, RIM - 0.012), True)):
        verts = [(radius * math.cos(math.tau * i / sides), radius * math.sin(math.tau * i / sides), z)
                 for z in z_rows for i in range(sides)]
        faces = []
        for row in range(len(z_rows) - 1):
            for i in range(sides):
                j = (i + 1) % sides
                quad = (row * sides + i, row * sides + j, (row + 1) * sides + j, (row + 1) * sides + i)
                faces.append(tuple(reversed(quad)) if inward else quad)
        m.add(verts, faces, 1)
    # Flat mortar bed just under the coping tops, so the joints between coping pieces never show through.
    ring_sides = 96
    inner_r, outer_r, bed_z = COPING_IN + 0.01, COPING_OUT - 0.01, RIM - 0.014
    verts = [(r * math.cos(math.tau * i / ring_sides), r * math.sin(math.tau * i / ring_sides), bed_z)
             for r in (inner_r, outer_r) for i in range(ring_sides)]
    faces = [(i, (i + 1) % ring_sides, ring_sides + (i + 1) % ring_sides, ring_sides + i) for i in range(ring_sides)]
    faces = [tuple(reversed(face)) for face in faces]
    m.add(verts, faces, 1)


def build_water(kit, material):
    sides = 48
    radius = MORTAR_IN + 0.004
    verts = [(0.0, 0.0, WATER_Z)]
    for ring, k in enumerate((0.5, 1.0)):
        for i in range(sides):
            a = math.tau * i / sides
            verts.append((radius * k * math.cos(a), radius * k * math.sin(a),
                          WATER_Z + 0.0008 * noise.noise(Vector((math.cos(a) * 3 * k, math.sin(a) * 3 * k, 1.0)))))
    faces = [(0, 1 + i, 1 + (i + 1) % sides) for i in range(sides)]
    for i in range(sides):
        a, b = 1 + i, 1 + (i + 1) % sides
        faces.append((a, a + sides, b + sides, b))
    obj = kit.mesh("Water", verts, faces, material=material)
    return kit.recalc_normals(obj)


# ---------------------------------------------------------------------------------------------- helpers
def roof_point(x, side, v, w=0.0):
    """Point on the roof plane: v down the slope from the ridge, w along the roof normal."""
    return Vector((x, side * (v * COS_A + w * SIN_A), RIDGE_Z - v * SIN_A + w * COS_A))


def local_box(kit, name, origin, axes, ranges, material, seed, side_flip=False):
    """Closed box in a local (u, v, w) frame; pcoord = (u, w, v) so oak grain runs along v."""
    ux, vx, wx = (Vector(a) for a in axes)
    (u0, u1), (v0, v1), (w0, w1) = ranges
    corners = [(u, v, w) for u in (u0, u1) for v in (v0, v1) for w in (w0, w1)]
    verts = [Vector(origin) + ux * u + vx * v + wx * w for u, v, w in corners]
    quads = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    obj = kit.mesh(name, verts, quads, material=material)
    kit.tag_coords(obj.data, [(u + seed * 3.1, w, v + seed * 1.7) for u, v, w in corners])
    return kit.recalc_normals(obj)


def shingle(kit, side, x0, x1, v_butt, length, rng, material, tag):
    """One rounded oak shingle lying on the roof plane, butt raised on the courses beneath it."""
    hw = 0.5 * (x1 - x0)
    cx = 0.5 * (x0 + x1)
    v_head = v_butt - length
    shoulder = min(0.03, 0.4 * length)
    outline = [(-hw, v_head), (hw, v_head), (hw + rng.uniform(-0.002, 0.002), v_head + 0.55 * length),
               (hw, v_butt - shoulder), (hw - 0.014, v_butt), (-hw + 0.014, v_butt), (-hw, v_butt - shoulder),
               (-hw + rng.uniform(-0.002, 0.002), v_head + 0.55 * length)]
    lift = rng.uniform(0.012, 0.022)
    tilt_u = rng.uniform(-0.01, 0.01)

    def height(u, v):
        t = (v - v_head) / length
        bottom = lift * t + tilt_u * u
        return bottom, bottom + 0.006 + 0.011 * t + rng.uniform(-0.0006, 0.0006)

    u_dir = Vector((side, 0.0, 0.0))
    d_dir = Vector((0.0, side * COS_A, -SIN_A))
    n_dir = Vector((0.0, side * SIN_A, COS_A))
    origin = Vector((cx, 0.0, RIDGE_Z))
    tops, bottoms, coords = [], [], []
    for u, v in outline:
        lo, hi = height(u, v)
        bottoms.append(origin + u_dir * u + d_dir * v + n_dir * lo)
        tops.append(origin + u_dir * u + d_dir * v + n_dir * hi)
        coords.append((u, hi, v))
    verts = tops + bottoms
    n = len(outline)
    centre = sum(tops, Vector()) / n
    verts.append(centre + n_dir * 0.0015)
    faces = [(i, (i + 1) % n, len(verts) - 1) for i in range(n)]
    faces += [(n + i, n + (i + 1) % n, (i + 1) % n, i) for i in range(n)]
    obj = kit.mesh(f"Shingle{tag}", verts, faces, material=material)
    off = (rng.uniform(-9, 9), rng.uniform(-9, 9), rng.uniform(-9, 9))
    pco = [(u + off[0], w + off[1], v + off[2]) for u, w, v in coords]
    pco = pco + [(0.0 + off[0], 0.0 + off[1], 0.0 + off[2])] * n + [(off[0], off[1], off[2])]
    kit.tag_coords(obj.data, pco)
    return obj


def moss_blob(kit, name, centre, radius, material, seed, rotation=(0, 0, 0)):
    blob = kit.sphere(name, radius, location=centre, rotation=rotation, material=material, segments=10, rings=6,
                      scale=(1.0, 1.0, 0.28))
    kit.roughen(blob, strength=radius * 0.35, scale=22.0, seed=seed)
    return blob


# ---------------------------------------------------------------------------------------------- bucket
def bucket(kit, centre, outside, inside, iron):
    height, base_r, top_r, floor, sides = 0.30, 0.118, 0.140, 0.022, 56

    def r_out(z):
        return base_r + (top_r - base_r) * (z / height) + 0.004 * math.sin(math.pi * z / height)

    profile = [(-0.006, 0.0), (0.0, 0.008)]
    profile += [(0.0, height * t) for t in (0.15, 0.3, 0.45, 0.6, 0.75, 0.9, 1.0)]
    rim_start = len(profile)
    profile += [("rim", 0.002), ("rim", 0.003), ("rim", 0.002)]
    profile += [(-0.012, height * t) for t in (0.92, 0.72, 0.52, 0.32, 0.14)]
    profile += [(-0.016, floor + 0.004)]
    rows, coords = [], []
    for dr, z in profile:
        ring, cring = [], []
        for j in range(sides):
            a = math.tau * j / sides
            if dr == "rim":
                k = [0.0, 0.5, 1.0][len(rows) - rim_start]
                r = r_out(height) - 0.002 - 0.010 * k
                zz = height + z
            else:
                r, zz = r_out(z) + dr, z
                if dr >= 0.0:
                    stave = max(0.0, 1.0 - abs(math.sin(7.0 * a)) * 7.0)
                    r += -0.0026 * stave + 0.0006 * noise.noise(Vector((math.cos(a) * 5, math.sin(a) * 5, z * 9)))
            local = (r * math.cos(a), r * math.sin(a), zz)
            cring.append(local)
            ring.append((local[0] + centre[0], local[1] + centre[1], local[2] + centre[2]))
        rows.append(ring)
        coords.append(cring)
    obj = kit.loft("Bucket", rows, material=[outside, inside], coords=coords,
                   cap_start=(centre[0], centre[1], centre[2]),
                   cap_end=(centre[0], centre[1], centre[2] + floor + 0.002))
    kit.recalc_normals(obj)
    row_faces = (len(rows) - 1) * sides
    for index, poly in enumerate(obj.data.polygons):
        if index < row_faces:
            poly.material_index = 1 if index // sides >= rim_start + 1 else 0
        elif index >= row_faces + sides:
            poly.material_index = 1
    parts = [obj]
    # Two iron hoops: flat bands rolled round the staves.
    section = [(0.0, -0.011), (0.0055, -0.011), (0.0075, -0.004), (0.0075, 0.004), (0.0055, 0.011), (0.0, 0.011),
               (-0.0015, 0.004), (-0.0015, -0.004)]
    for label, z in (("Lower", 0.075), ("Upper", 0.215)):
        hoop_rows = []
        for i in range(48):
            a = math.tau * i / 48
            radius = r_out(z) - 0.0035
            hoop_rows.append([(centre[0] + (radius + dr) * math.cos(a), centre[1] + (radius + dr) * math.sin(a),
                               centre[2] + z + dz) for dr, dz in section])
        parts.append(kit.recalc_normals(kit.loft(f"Hoop{label}", hoop_rows, material=iron, cyclic=True)))
    # Rim ears and an iron bail arching up to the rope.
    ear_z = centre[2] + height - 0.028
    for sx in (-1, 1):
        parts.append(kit.box(f"Ear{sx}", (0.016, 0.030, 0.034),
                             (centre[0] + sx * (top_r + 0.003), centre[1], ear_z), material=outside, bevel=0.003))
    arch = []
    for i in range(33):
        t = math.pi * i / 32
        arch.append((centre[0] + (top_r + 0.014) * math.cos(t), centre[1],
                     ear_z + 0.004 + (1.20 - ear_z - 0.004) * math.sin(t) ** 0.85))
    parts.append(kit.tube("Bail", arch, radius=0.0065, sides=8, material=iron))
    return parts


# ---------------------------------------------------------------------------------------------- build
def build(kit):
    rng = random.Random(9131)
    m = masonry.Masonry(seed=311)
    stone = stone_material(kit, "M_VillageWellStone", 33, lichen=0.55, moss=0.28, soil=0.45, soil_height=0.30,
                           patina=0.95, iron=0.5, streaks=0.5)
    mortar = kit.mats.lime_mortar("M_VillageWellMortar", seed=4.0)
    build_ring(m, rng)
    add_mortar(m)
    ring = m.build("RingMasonry", [stone, mortar])

    oak = common.weathered_oak(kit, "M_VillageWellOak", seed=41.0, lichen=0.30, grime=0.45)
    oak_post = common.weathered_oak(kit, "M_VillageWellPostOak", seed=47.0, lichen=0.36, grime=0.50)
    shingle_oak = common.weathered_oak(kit, "M_VillageWellShingle", seed=53.0, lichen=0.55, grime=0.55)
    roller_oak = common.weathered_oak(kit, "M_VillageWellRoller", seed=59.0, lichen=0.12, grime=0.60)
    moss = common.moss_lichen(kit, "M_VillageWellMoss", seed=9.0)
    iron = common.rusted_iron(kit, "M_VillageWellIron", seed=5.0)
    bucket_out = common.weathered_oak(kit, "M_VillageWellBucket", seed=61.0, lichen=0.10, grime=0.55)
    bucket_in = kit.mats.wood("M_VillageWellBucketInside", light=(0.12, 0.08, 0.047), dark=(0.05, 0.032, 0.018),
                              grain=0.7, roughness=0.4, weathering=0.0, grime=0.5, seed=14.0, relief=0.6)
    rope_mat = kit.mats.rawhide("M_VillageWellRope", color=(0.30, 0.235, 0.14), strands=3, twist=110.0)
    water_mat = _water_material(kit, "M_VillageWellWater")
    common.zero_subsurface(oak, oak_post, shingle_oak, roller_oak, moss, bucket_out)

    parts = [ring, build_water(kit, water_mat)]

    # Oak posts standing on the coping, with knee braces up to the ridge beam.
    for side, label in ((-1, "W"), (1, "E")):
        parts.append(common.beam(kit, f"Post{label}", [(side * POST_X, 0.0, POST_BOTTOM), (side * POST_X, 0.0, POST_TOP)],
                                 0.125, 0.145, oak_post, 71 + side, spacing=0.22))
        parts.append(common.beam(kit, f"Brace{label}", [(side * (POST_X - 0.05), 0.0, 1.98),
                                                        (side * 0.30, 0.0, POST_TOP - 0.03)],
                                 0.075, 0.075, oak_post, 77 + side, spacing=0.3))
    parts.append(common.beam(kit, "Ridge", [(-0.99, 0.0, 2.615), (0.99, 0.0, 2.615)], 0.125, 0.115, oak, 83,
                             spacing=0.3))

    # Roof: plank deck, rafters, rounded oak shingles in overlapping courses, ridge cap, barge boards.
    courses = 8
    exposure = (SLOPE_LEN + 0.04) / courses
    for side in (-1, 1):
        u_dir = (side, 0.0, 0.0)
        d_dir = (0.0, side * COS_A, -SIN_A)
        n_dir = (0.0, side * SIN_A, COS_A)
        axes = (u_dir, d_dir, n_dir)
        boards = 7
        for b in range(boards):
            span = 2 * 0.99 / boards
            u0 = -0.99 + b * span + 0.004
            parts.append(local_box(kit, f"Deck{side}{b}", (0.0, 0.0, RIDGE_Z), axes,
                                   ((u0, u0 + span - 0.008), (0.0, SLOPE_LEN + 0.05), (-0.030, -0.004)),
                                   oak, b + 4 * (side + 2)))
        for index, x in enumerate((-0.86, -0.43, 0.0, 0.43, 0.86)):
            a, b = roof_point(x, side, 0.03, -0.075), roof_point(x, side, SLOPE_LEN + 0.04, -0.075)
            parts.append(common.beam(kit, f"Rafter{side}{index}", [tuple(a), tuple(b)], 0.07, 0.085, oak, 90 + index,
                                     spacing=0.6))
        for c in range(courses):
            v_butt = SLOPE_LEN + 0.035 - c * exposure * 1.0 + rng.uniform(-0.006, 0.006)
            length = min(0.36 + rng.uniform(-0.015, 0.015), v_butt + 0.03)
            x = -ROOF_HALF + 0.025 - rng.uniform(0.0, 0.09)
            number = 0
            while x < ROOF_HALF - 0.04:
                w = rng.uniform(0.105, 0.130)
                x0, x1 = max(x, -ROOF_HALF + 0.02), min(x + w, ROOF_HALF - 0.02)
                if x1 - x0 > 0.045:
                    parts.append(shingle(kit, side, x0, x1, v_butt, length, rng, shingle_oak, f"{side}_{c}_{number}"))
                x += w + 0.006
                number += 1
        # Ridge cap: a board either side, lapped.
        cap_v = 0.09 if side > 0 else 0.10
        centre = roof_point(0.0, side, cap_v, 0.045 if side > 0 else 0.052)
        parts.append(common.beam(kit, f"RidgeCap{side}", [(-1.02, centre.y, centre.z), (1.02, centre.y, centre.z)],
                                 0.19, 0.024, oak, 100 + side, spacing=0.6, roll=-side * ALPHA))
        # Barge boards on the gable ends.
        for end in (-1, 1):
            a, b = roof_point(end * 1.0, side, 0.0, -0.035), roof_point(end * 1.0, side, SLOPE_LEN + 0.04, -0.035)
            parts.append(common.beam(kit, f"Barge{side}{end}", [tuple(a), tuple(b)], 0.032, 0.165, oak_post,
                                     105 + end + side, spacing=0.4))
    # Moss pillows on the shaded (north, -Y) slope and the ridge.
    for index, (x, v) in enumerate(((-0.62, 0.78), (0.42, 0.88), (0.74, 0.40), (-0.1, 0.98))):
        centre = roof_point(x, -1, v, 0.028)
        parts.append(moss_blob(kit, f"RoofMoss{index}", tuple(centre), rng.uniform(0.045, 0.075), moss, 120 + index,
                               rotation=(math.degrees(ALPHA), 0, 0)))

    # Windlass: roller turned in iron-collared journals, crank at +X, rope wound on and bucket hanging below.
    roller = kit.cylinder("Roller", ROLLER_R, 1.16, location=(0.0, 0.0, AXLE_Z), rotation=(0, 90, 0),
                          material=roller_oak, sides=28, bevel=0.006)
    kit.roughen(roller, strength=0.0025, scale=14.0, seed=7, subdivide=0)
    parts.append(roller)
    parts.append(kit.cylinder("Journal", 0.032, 1.62, location=(0.05, 0.0, AXLE_Z), rotation=(0, 90, 0),
                              material=roller_oak, sides=16, bevel=0.003))
    for sx, label in ((-1, "W"), (1, "E")):
        parts.append(kit.cylinder(f"Collar{label}", 0.052, 0.034, location=(sx * (POST_X + 0.080), 0.0, AXLE_Z),
                                  rotation=(0, 90, 0), material=iron, sides=20, bevel=0.004))
        parts.append(kit.cylinder(f"Plate{label}", 0.068, 0.012, location=(sx * (POST_X + 0.068 - 0.004), 0.0, AXLE_Z),
                                  rotation=(0, 90, 0), material=iron, sides=20, bevel=0.002))
    parts.append(kit.cylinder("Boss", 0.046, 0.050, location=(0.90, 0.0, AXLE_Z), rotation=(0, 90, 0),
                              material=roller_oak, sides=18, bevel=0.005))
    arm_tip = (0.90, -0.246, AXLE_Z - 0.172)
    parts.append(common.beam(kit, "CrankArm", [(0.90, 0.0, AXLE_Z), arm_tip], 0.042, 0.052, roller_oak, 131,
                             spacing=0.15))
    parts.append(kit.cylinder("CrankGrip", 0.021, 0.165, location=(arm_tip[0] + 0.045, arm_tip[1], arm_tip[2]),
                              rotation=(0, 90, 0), material=roller_oak, sides=14, bevel=0.004))
    parts.append(kit.cylinder("CrankPin", 0.012, 0.070, location=(arm_tip[0], arm_tip[1], arm_tip[2]),
                              rotation=(0, 90, 0), material=iron, sides=10, bevel=0.002))

    rope = []
    turns, per_turn = 6, 26
    for i in range(turns * per_turn + 1):
        t = i / (turns * per_turn)
        phi = math.tau * turns * (1.0 - t)
        x = -0.24 + 0.24 * t
        rope.append((x, ROPE_WRAP * math.cos(phi) + 0.002 * math.sin(t * 40), AXLE_Z + ROPE_WRAP * math.sin(phi)))
    drop_top = AXLE_Z - 0.04
    for k in range(1, 11):
        rope.append((0.002 * math.sin(k), BUCKET_Y + 0.003 * math.sin(k * 1.9), AXLE_Z - 0.05 - (AXLE_Z - 0.05 - 1.215) * k / 10))
    parts.append(kit.tube("Rope", rope, radius=ROPE_R, sides=8, material=rope_mat))
    parts.append(kit.sphere("Knot", 0.026, location=(0.0, BUCKET_Y, 1.215), material=rope_mat, segments=10, rings=6,
                            scale=(1.0, 1.0, 1.25)))
    parts += bucket(kit, (0.0, BUCKET_Y, 0.66), bucket_out, bucket_in, iron)

    # Moss at the foot of the kerb and on the coping.
    for index, (angle, radius, z, size) in enumerate(((210, 0.70, RIM + 0.002, 0.055), (300, 0.56, RIM + 0.004, 0.04),
                                                      (35, 0.72, RIM + 0.002, 0.045))):
        a = math.radians(angle)
        parts.append(moss_blob(kit, f"KerbMoss{index}", (radius * math.cos(a), radius * math.sin(a), z), size, moss,
                               140 + index))

    obj = kit.join(parts, "SM_VillageWell", pivot=None, unwrap=False, reshade=False)
    names = [mat.name if mat else "" for mat in obj.data.materials]
    shrink = {names.index("M_VillageWellMortar"): 0.4}
    masonry.unwrap(obj, shrink=shrink)
    return masonry.finish(kit, obj, lod_ratios=(0.45, 0.16))


def _water_material(kit, name):
    g = kit.mats.Graph(name)
    p = g.coord()
    ripple = g.noise(g.vmath("MULTIPLY", p, (1.0, 1.0, 0.0)), scale=45.0, detail=3.0).outputs["Fac"]
    silt = g.noise(p, scale=12.0, detail=2.0).outputs["Fac"]
    g.set("Base Color", g.mix((0.018, 0.017, 0.012), (0.034, 0.03, 0.02), g.remap(silt, 0.35, 0.65)))
    g.set("Roughness", g.remap(ripple, 0.4, 0.6, 0.03, 0.07))
    g.set("Normal", g.bump(ripple, strength=0.08, distance=0.002))
    return g.mat
