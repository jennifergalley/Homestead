"""Seeds for planting: a small pinch of four dry tepary beans, and a loose-soil mound that
marks where one was pressed in and covered.

Real-object research (written before modeling):
- Tepary beans (Phaseolus acutifolius), the drought-hardy bean of the Southwest, are
  small: 5-8 mm long, 4-5.5 mm wide, 3-4 mm thick, rounded-oblong to slightly kidney
  shaped and a little flattened, with a small white hilum (the "eye") ringed darker on
  the inner curve. Seed coats are satiny, faintly wrinkled when dry and come in white,
  tan, brown, near-black and speckled or mottled strains; a farmer's seed saved from a
  mixed patch shows several at once. Beans are planted 2-3 cm deep, pressed in with a
  finger and covered.
- A freshly covered planting spot is a low, crumbly mound of turned topsoil 10-15 cm
  across and 2-4 cm high: dark, moist loam in the core, a paler dry crust forming on the
  crumbs, small clods, a fingertip dimple where it was patted down, and loose crumbs
  spilled around the base, feathering into the ground.

Rigid static meshes; units are meters, Z up.
SM_Seeds PIVOT: the bounds centre of the cluster (hold it at the pinch between thumb and
forefinger). The beans lie in a loose heap about 1.2 x 1.1 x 0.7 cm (2.3k triangles).
SM_SoilMound PIVOT: bottom centre; z = 0 is the ground line and the feathered rim sinks
3 mm below it so it never floats on uneven terrain. SM_SoilMound_LOD1 shares its origin
and texture set.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import random

from mathutils import Euler, Vector, noise

NAME = "Seeds"
DESCRIPTION = ("A pinch of four dry tepary beans (pivot = cluster centre) and a covered-seed loose-soil "
               "mound (pivot = bottom centre, with LOD1) (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 5000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "repack": False}
BEAUTY = {"meshes": {
    "SM_Seeds": {"pose": (0, 0, 30), "focus": (0.0, 0.0, 0.0), "detail_distance": 0.075, "detail_fstop": 128.0},
    "SM_SoilMound": {"pose": (0, 0, 0), "focus": (0.012, -0.008, 0.026), "ground": "origin"},
}}
NOTES = {
    "SM_Seeds": "Pivot = bounds centre of the 4-bean cluster, for the thumb-forefinger pinch socket.",
    "SM_SoilMound": ("Pivot = bottom centre on the ground line (z = 0); the rim sinks 3 mm below it. "
                     "~14 cm across (the feathered rim included), 3.5 cm tall, 4.8k triangles. "
                     "SM_SoilMound_LOD1 (1.4k) is a 30% decimation sharing its UVs and textures."),
}

MOUND_R = 0.058
MOUND_H = 0.034
DIMPLE = (0.01, -0.007, 0.0095, 0.0058)     # x, y, radius, depth of the fingertip press


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# ---------------------------------------------------------------------- beans

def bean(kit, name, mat, centre, rotation, length, seed, rows=18, sides=16):
    """One dry bean: rounded-oblong, slightly kidney-bent toward its back (+y) so the
    hilum side (-y) is gently concave, flattened (z). pcoord is normalized
    (x/half-length, y/half-width, z/half-thickness): the hilum sits at (0, -1, 0)."""
    width, thick = 0.62 * length, 0.43 * length
    rng = random.Random(seed)
    lump = Vector((rng.random() * 50, rng.random() * 50, rng.random() * 50))
    frame = Euler([math.radians(a) for a in rotation]).to_matrix()
    rows_pts, coords = [], []
    for i in range(rows):
        u = 0.5 - 0.5 * math.cos(math.pi * (0.02 + 0.96 * i / (rows - 1)))
        x = (u - 0.5) * length
        # Fuller, blunter ends than an egg: an oblong with rounded shoulders.
        prof = max(0.0, 1 - abs(2 * u - 1) ** 2.6) ** 0.47
        bend = 0.15 * width * (1 - (2 * u - 1) ** 2)
        ring, pco = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            cy, cz = math.cos(a), math.sin(a)
            # A little squarer across the back, fuller toward the hilum edge.
            # Flattened faces and rounded edges (a superellipse), fuller toward the hilum.
            y = 0.5 * width * prof * math.copysign(abs(cy) ** 0.85, cy)
            z = 0.5 * thick * prof * math.copysign(abs(cz) ** 0.72, cz) * (1 - 0.1 * cy)
            r = 1 + 0.025 * noise.noise(lump + Vector((u * 3.0, cy * 1.5, cz * 1.5)))
            p = Vector((x * (1 + 0.01 * cz), (y + bend) * r, z * r))
            ring.append(tuple(centre + frame @ p))
            pco.append((x / (0.5 * length), y / (0.5 * width), z / (0.5 * thick)))
        rows_pts.append(ring)
        coords.append(pco)
    obj = kit.loft(name, rows_pts, material=mat, coords=coords, cap_start=True, cap_end=True)
    return kit.recalc_normals(obj)


def seed_coat(kit, name, base, dark, pattern="plain", amount=0.5, seed=0.0):
    """Satiny dry bean seed coat on normalized pcoord: faint tone variation and fine dry
    wrinkles, an optional ``mottled`` (streaky blotches along the bean) or ``speckled``
    pattern of ``dark``, and the white hilum with its dark rim on the inner curve."""
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 3.1, seed * 1.7, seed * 2.3))
    tone = g.noise(seeded, scale=1.6, detail=3.0).outputs["Fac"]
    color = g.mix(tuple(c * 0.82 for c in base), tuple(min(1.0, c * 1.12) for c in base), g.remap(tone, 0.35, 0.65))
    if pattern == "mottled":
        streak = g.noise(g.vmath("MULTIPLY", seeded, (0.55, 1.6, 1.6)), scale=2.4, detail=4.0,
                         distortion=0.6).outputs["Fac"]
        color = g.mix(color, dark, g.remap(streak, 0.56 - 0.1 * amount, 0.6 - 0.1 * amount))
    elif pattern == "speckled":
        spots = g.voronoi(g.vmath("MULTIPLY", seeded, (0.8, 1.0, 1.0)), scale=9.0, feature="F1").outputs["Distance"]
        size = g.noise(seeded, scale=6.0, detail=2.0).outputs["Fac"]
        mask = g.remap(g.math("SUBTRACT", spots, g.math("MULTIPLY", size, 0.18)), 0.12 * amount, 0.05 * amount)
        color = g.mix(color, dark, mask)
        fine = g.noise(seeded, scale=40.0, detail=2.0).outputs["Fac"]
        color = g.mix(color, dark, g.remap(fine, 0.68, 0.72, 0.0, 0.7))
    elif pattern == "marbled":
        vein = g.wave(seeded, scale=1.6, distortion=7.0, detail=3.0, kind="BANDS", direction="X").outputs["Fac"]
        color = g.mix(color, dark, g.math("MULTIPLY", g.remap(vein, 0.45, 0.8), amount))
    # Hilum: a small white oval eye on the inner (-y) curve, ringed dark.
    ex = g.math("DIVIDE", x, 0.21)
    ez = g.math("DIVIDE", z, 0.36)
    eye = g.math("ADD", g.math("MULTIPLY", ex, ex), g.math("MULTIPLY", ez, ez))
    inner = g.remap(y, -0.7, -0.88)
    white = g.math("MULTIPLY", g.remap(eye, 0.75, 0.55), inner)
    rim = g.math("MULTIPLY", g.math("SUBTRACT", g.remap(eye, 1.25, 0.9), white), inner)
    color = g.mix(color, (0.03, 0.022, 0.016), g.math("MULTIPLY", rim, 0.85))
    color = g.mix(color, (0.3, 0.27, 0.2), white)
    g.set("Base Color", color)
    wrinkle = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 2.5, 2.5)), scale=14.0, detail=5.0,
                      roughness=0.65).outputs["Fac"]
    g.set("Roughness", g.math("ADD", g.remap(wrinkle, 0.3, 0.7, 0.46, 0.6), g.math("MULTIPLY", white, 0.25)))
    height = g.math("SUBTRACT", g.math("MULTIPLY", wrinkle, 0.5), g.math("MULTIPLY", g.math("ADD", white, rim), 0.6))
    g.set("Normal", g.bump(height, strength=0.25, distance=0.00015))
    return g.mat


BEANS = [
    # name, coat args, centre (m), rotation (deg), length (m)
    ("BeanTan", dict(base=(0.33, 0.205, 0.105), dark=(0.22, 0.13, 0.06), pattern="plain", seed=1.0),
     (0.0, 0.0026, 0.0), (0, 0, 8), 0.0074),
    ("BeanMottled", dict(base=(0.3, 0.215, 0.135), dark=(0.075, 0.033, 0.022), pattern="mottled", amount=0.6,
                         seed=2.0),
     (0.0006, -0.0024, 0.0002), (6, -4, 170), 0.0070),
    ("BeanSpeckled", dict(base=(0.25, 0.175, 0.105), dark=(0.05, 0.03, 0.02), pattern="speckled", amount=1.3,
                          seed=3.0),
     (0.0009, 0.0001, 0.0034), (-28, 9, 64), 0.0066),
    ("BeanDark", dict(base=(0.06, 0.034, 0.022), dark=(0.028, 0.016, 0.012), pattern="mottled", amount=0.4, seed=4.0),
     (-0.0045, 0.0005, 0.0014), (-8, 22, 96), 0.0078),
]


def build_seeds(kit):
    parts = []
    for name, coat, centre, rotation, length in BEANS:
        mat = seed_coat(kit, "M_" + name, **coat)
        parts.append(bean(kit, name, mat, Vector(centre), rotation, length, seed=len(parts) + 1))
    seeds = kit.join(parts, "SM_Seeds", pivot="center", unwrap=False, reshade=True, smooth_angle=80)
    kit.pack_uvs(seeds)
    return seeds


# ---------------------------------------------------------------- soil mound

def _hash(point):
    return (math.sin(point.x * 127.1 + point.y * 311.7 + point.z * 74.7) * 43758.5453) % 1.0


def mound_height(x, y):
    """Heap of loose loam scraped together over the seed: lopsided (more soil was pulled
    in from one side), patted flat on top, with a fingertip press, crumb aggregates and
    a few bigger clods; the rim feathers 3 mm below the ground line."""
    sx, sy = x + 0.005, y - 0.003
    r = math.hypot(sx, sy)
    theta = math.atan2(sy, sx)
    edge = MOUND_R * (1 + 0.1 * noise.noise(Vector((math.cos(theta) * 1.4, math.sin(theta) * 1.4, 2.0)))
                      + 0.06 * math.cos(theta - 2.4))
    t = r / edge
    dome = MOUND_H * max(0.0, 1 - t * t) ** 1.2
    dome *= 1 + 0.26 * noise.noise(Vector((x * 22, y * 22, 5.0))) + 0.1 * noise.noise(Vector((x * 55, y * 55, 8.0)))
    cap = MOUND_H * 0.84
    dome = dome - max(0.0, dome - cap) * 0.7
    dome -= 0.003 * smoothstep(0.78, 1.15, t)
    dx, dy, dr, dd = DIMPLE
    press = math.exp(-((x - dx) ** 2 + (y - dy) ** 2) / (dr * dr))
    dome -= dd * press
    crumbly = (1 - 0.8 * press) * smoothstep(1.25, 0.85, t)
    cells, points = noise.voronoi(Vector((x * 190, y * 190, 1.7)), distance_metric="DISTANCE", exponent=2.5)
    crumb = 0.0015 * max(0.0, 1 - cells[0] / 0.6) ** 1.5 * (0.6 + 0.8 * _hash(points[0]))
    big, big_points = noise.voronoi(Vector((x * 62, y * 62, 4.2)), distance_metric="DISTANCE", exponent=2.5)
    clod = 0.0035 * max(0.0, 1 - big[0] / 0.5) ** 1.6 * (1.0 if _hash(big_points[0]) > 0.62 else 0.0)
    fine = 0.0005 * noise.noise(Vector((x * 400, y * 400, 0.3)))
    return dome + crumbly * (crumb + clod + fine - 0.0012)


def build_mound(kit, mat):
    rings, segs = 30, 72
    verts, faces = [(0.0, 0.0, mound_height(0.0, 0.0))], []
    for i in range(1, rings + 1):
        r = MOUND_R * 1.2 * (i / rings) ** 0.9
        for j in range(segs):
            a = 2 * math.pi * (j + 0.5 * (i % 2)) / segs
            x, y = r * math.cos(a), r * math.sin(a)
            verts.append((x, y, mound_height(x, y)))
    for j in range(segs):
        faces.append((0, 1 + j, 1 + (j + 1) % segs))
    for i in range(rings - 1):
        a0, b0 = 1 + i * segs, 1 + (i + 1) * segs
        for j in range(segs):
            j1 = (j + 1) % segs
            faces.append((a0 + j, b0 + j, b0 + j1, a0 + j1))
    mound = kit.mesh("Mound", verts, faces, material=mat)
    parts = [kit.recalc_normals(mound)]
    rng = random.Random(4417)
    for k in range(9):
        a = rng.uniform(0, 2 * math.pi)
        spilled = k < 7
        r = MOUND_R * (rng.uniform(0.95, 1.3) if spilled else rng.uniform(0.3, 0.6))
        size = rng.uniform(0.0018, 0.0042)
        x, y = r * math.cos(a), r * math.sin(a)
        base = max(mound_height(x, y), -0.0012)
        crumb = kit.sphere(f"Clod{k}", size, segments=8, rings=5)
        squash = rng.uniform(0.5, 0.75)
        offset = Vector((rng.random() * 40, rng.random() * 40, rng.random() * 40))
        spin = rng.uniform(0, 2 * math.pi)

        def lumpy(co, x=x, y=y, base=base, squash=squash, offset=offset, size=size, spin=spin):
            # Angular clod: faceted by a coarse noise, flattened, resting half sunk.
            wobble = 1 + 0.35 * noise.noise(co * (1.2 / size) + offset)
            px = co.x * math.cos(spin) - co.y * math.sin(spin)
            py = co.x * math.sin(spin) + co.y * math.cos(spin)
            return Vector((x + px * wobble * 1.15, y + py * wobble, base + size * 0.25 + co.z * wobble * squash))

        kit.warp(crumb, lumpy)
        kit.tag_coords(crumb.data)
        crumb.data.materials.append(mat)
        parts.append(crumb)
    obj = kit.join(parts, "SM_SoilMound", pivot=None, unwrap=False, reshade=True, smooth_angle=70)
    kit.pack_uvs(obj)
    return obj


def loam(kit, name, damp=(0.028, 0.019, 0.012), mid=(0.047, 0.033, 0.02), dry=(0.094, 0.072, 0.048), seed=0.0):
    """Freshly turned forest loam on pcoord (meters): 2-5 mm crumb aggregates, each its
    own damp-to-dry tone, dark pores between them, sparse pale sand grains, dark fibrous
    organic bits and the odd straw-coloured fleck; higher crumbs dry to a paler crust.
    A fingertip press (``DIMPLE``) is compacted smooth and darker."""
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.3, seed * 0.2, seed * 0.1))
    # Domain-warp the crumb field so aggregates are irregular, not a tiling of cells.
    warp = g.noise(seeded, scale=90.0, detail=3.0).outputs["Color"]
    warped = g.vmath("ADD", seeded, g.vmath("MULTIPLY", g.vmath("SUBTRACT", warp, (0.5, 0.5, 0.5)),
                                            (0.004, 0.004, 0.004)))
    agg = g.voronoi(warped, scale=280.0)
    tone = g.channel(agg.outputs["Color"], 0)
    size = g.remap(g.channel(agg.outputs["Color"], 1), 0.0, 1.0, 0.45, 0.85)
    dome = g.remap(g.math("DIVIDE", agg.outputs["Distance"], size), 1.0, 0.2)
    small = g.voronoi(g.vmath("ADD", warped, (3.0, 1.0, 2.0)), scale=720.0).outputs["Distance"]
    grain = g.remap(small, 0.75, 0.15)
    crumbs = g.math("MAXIMUM", g.math("POWER", dome, 0.6), g.math("MULTIPLY", grain, 0.6))
    pore = g.remap(crumbs, 0.18, 0.0)
    high = g.remap(z, 0.008, MOUND_H)
    crust = g.noise(seeded, scale=120.0, detail=4.0).outputs["Fac"]
    dryness = g.math("ADD", g.math("MULTIPLY", tone, 0.28), g.math("MULTIPLY", high, 0.4))
    dryness = g.math("ADD", dryness, g.math("ADD", g.math("MULTIPLY", g.math("SUBTRACT", crust, 0.5), 0.7),
                                            g.math("MULTIPLY", crumbs, 0.18)))
    color = g.ramp(dryness, [(0.2, damp), (0.55, mid), (0.95, dry)])
    color = g.mix(color, tuple(c * 0.4 for c in damp), g.math("MULTIPLY", pore, 0.85))
    sand = g.noise(seeded, scale=4200.0, detail=1.0).outputs["Fac"]
    color = g.mix(color, (0.22, 0.2, 0.17), g.remap(sand, 0.74, 0.77, 0.0, 0.55))
    fibre = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 5.0, 1.0)), scale=260.0,
                      feature="DISTANCE_TO_EDGE").outputs["Distance"]
    organic = g.math("MULTIPLY", g.remap(fibre, 0.02, 0.0),
                     g.remap(g.noise(seeded, scale=30.0, detail=2.0).outputs["Fac"], 0.58, 0.64))
    color = g.mix(color, (0.018, 0.012, 0.008), organic)
    straw = g.remap(g.noise(g.vmath("ADD", seeded, (7.0, 3.0, 1.0)), scale=900.0, detail=1.0).outputs["Fac"],
                    0.76, 0.78, 0.0, 0.8)
    color = g.mix(color, (0.16, 0.12, 0.07), straw)
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.008, samples=16))
    color = g.mix(color, (0.012, 0.009, 0.007), g.remap(cavity, 0.08, 0.45, 0.0, 0.85))
    dx, dy, dr, _ = DIMPLE
    px, py = g.math("SUBTRACT", x, dx), g.math("SUBTRACT", y, dy)
    press = g.remap(g.math("ADD", g.math("MULTIPLY", px, px), g.math("MULTIPLY", py, py)),
                    (dr * 1.15) ** 2, (dr * 0.55) ** 2)
    color = g.mix(color, damp, g.math("MULTIPLY", press, 0.75))
    g.set("Base Color", color)
    g.set("Roughness", g.math("SUBTRACT", g.remap(dryness, 0.2, 0.9, 0.84, 0.97), g.math("MULTIPLY", press, 0.14)))
    lump = crumbs
    grit = g.noise(seeded, scale=2600.0, detail=2.0).outputs["Fac"]
    height = g.math("ADD", g.math("MULTIPLY", lump, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", press, 0.8))),
                    g.math("MULTIPLY", grit, 0.25))
    g.set("Normal", g.bump(height, strength=0.55, distance=0.0015))
    return g.mat

def build(kit):
    seeds = build_seeds(kit)
    mound = build_mound(kit, loam(kit, "M_SoilMound", seed=3.0))
    lod = mound.copy()
    lod.data = mound.data.copy()
    lod.name = lod.data.name = "SM_SoilMound_LOD1"
    kit._link(lod)
    mod = lod.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = 0.3
    mod.use_collapse_triangulate = True
    kit.apply_modifiers(lod)
    return [seeds, mound, lod]
