"""One freshly hoed garden square: a metre of forest floor chopped open with the stone hoe
and raked into three low ridges, ready for seed.

Real-object research (written before modeling):
- Hand-hoed ground in a forest clearing is loose, dark topsoil turned up from under the
  duff: 2-6 cm of heave above the untouched floor, broken into 2-5 mm crumbs with
  walnut-sized clods the blade didn't break, and shallow drag marks from the hoe's pulls.
- A small bed hoed for sowing is drawn into low ridges (hills) a hand-span apart with
  furrows between them; the ridge tops dry to a paler crust within hours while the
  furrows stay dark and damp.
- The edge of the worked patch is ragged, not square: soil is thrown a few centimetres
  past the last chop and feathers out over the leaf litter.

Rigid static mesh; units are meters, Z up.
SM_TilledBed PIVOT: bottom centre; z = 0 is the ground line. The worked square is 1 m
(the game's garden square) and the feathered rim sinks 15 mm below the ground line, so
it sits into gently uneven terrain without floating. Ridges run along X.
SM_TilledBed_LOD1 is a 25% decimation sharing its UVs and textures.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib.util
import math
from pathlib import Path

from mathutils import Vector, noise

_spec = importlib.util.spec_from_file_location("homestead_seeds", Path(__file__).with_name("seeds.py"))
seeds = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(seeds)

NAME = "TilledBed"
DESCRIPTION = ("A freshly hoed 1 m garden square of loose loam raked into three ridges, with a ragged "
               "feathered rim (pivot = bottom centre, with LOD1) (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"meshes": {
    "SM_TilledBed": {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.02), "ground": "origin"},
}}
NOTES = {
    "SM_TilledBed": ("Pivot = bottom centre on the ground line (z = 0); the rim sinks 15 mm below it. "
                     "Worked area 1 m square (about 1.1 m with the thrown rim), ridges along X at "
                     "y = -0.3, 0, +0.3 m, up to ~5.5 cm tall."),
}

HALF = 0.5              # the worked square (garden square is 1 m)
EXTENT = 0.56           # mesh half-size including the thrown rim
GRID = 84               # vertices per side
HEAVE = 0.022           # loose soil lifted above the floor
RIDGE = 0.03            # ridge height above the heave
SPACING = 0.3           # ridge spacing (three ridges: -0.3, 0, +0.3)


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def _hash(point):
    return (math.sin(point.x * 127.1 + point.y * 311.7 + point.z * 74.7) * 43758.5453) % 1.0


def worked(x, y):
    """1 inside the hoed square, 0 on untouched ground: a rounded square with a ragged,
    chopped edge (each hoe stroke bites a little further or shorter)."""
    p = 6.0
    d = (abs(x) ** p + abs(y) ** p) ** (1 / p)
    theta = math.atan2(y, x)
    edge = HALF * (0.97 + 0.035 * noise.noise(Vector((math.cos(theta) * 2.2, math.sin(theta) * 2.2, 1.3)))
                   + 0.015 * noise.noise(Vector((math.cos(theta) * 7.0, math.sin(theta) * 7.0, 4.1))))
    return smoothstep(edge + 0.035, edge - 0.03, d), d / edge


def height(x, y):
    inside, rel = worked(x, y)
    # Ridges along X, wandering a little as each pull of the hoe drew them.
    wy = y + 0.018 * noise.noise(Vector((x * 5.0, 0.4, 2.0))) + 0.006 * noise.noise(Vector((x * 17.0, 1.1, 3.0)))
    phase = wy / SPACING
    ridge = (0.5 + 0.5 * math.cos(2 * math.pi * phase)) ** 1.4
    # The outer furrows open onto the rim; no ridge beyond the third.
    ridge *= smoothstep(1.62, 1.35, abs(phase))
    ridge *= 0.85 + 0.3 * noise.noise(Vector((x * 3.0, y * 3.0, 6.0)))
    base = HEAVE * (1 + 0.35 * noise.noise(Vector((x * 4.0, y * 4.0, 9.0))))
    h = inside * (base + RIDGE * ridge)
    # Hoe drag marks: faint grooves across the ridges, along Y.
    drag = noise.noise(Vector((x * 38.0, y * 3.0, 12.0)))
    h -= inside * 0.0025 * max(0.0, drag)
    # Clods the blade didn't break, and crumbs everywhere it did.
    big, big_points = noise.voronoi(Vector((x * 26, y * 26, 4.2)), distance_metric="DISTANCE", exponent=2.5)
    clod = 0.009 * max(0.0, 1 - big[0] / 0.45) ** 1.7 * (1.0 if _hash(big_points[0]) > 0.7 else 0.0)
    cells, points = noise.voronoi(Vector((x * 150, y * 150, 1.7)), distance_metric="DISTANCE", exponent=2.5)
    crumb = 0.002 * max(0.0, 1 - cells[0] / 0.6) ** 1.5 * (0.6 + 0.8 * _hash(points[0]))
    fine = 0.0006 * noise.noise(Vector((x * 400, y * 400, 0.3)))
    loose = smoothstep(0.0, 0.35, inside)
    # Soil thrown past the last chop: sparse crumbs over the floor just outside.
    thrown = smoothstep(1.28, 1.0, rel) * (1 - inside) * (1.0 if _hash(points[0]) > 0.55 else 0.0)
    h += loose * (clod + crumb + fine) + thrown * crumb * 1.5
    # Rim sinks below the ground line so it never floats on uneven terrain.
    h -= 0.015 * smoothstep(0.2, 0.0, inside) * (1 - thrown)
    return h


def build_bed(kit, mat):
    verts, faces = [], []
    for i in range(GRID):
        for j in range(GRID):
            x = -EXTENT + 2 * EXTENT * i / (GRID - 1)
            y = -EXTENT + 2 * EXTENT * j / (GRID - 1)
            verts.append((x, y, height(x, y)))
    for i in range(GRID - 1):
        for j in range(GRID - 1):
            a = i * GRID + j
            faces.append((a, a + GRID, a + GRID + 1, a + 1))
    bed = kit.mesh("Bed", verts, faces, material=mat)
    # An open sheet: recalc_face_normals can pick either side, so wind every face upward explicitly.
    for poly in bed.data.polygons:
        if poly.normal.z < 0:
            poly.flip()
    bed.data.update()
    obj = kit.join([bed], "SM_TilledBed", pivot=None, unwrap=False, reshade=True, smooth_angle=80)
    # A heightfield: one planar UV island over the whole square (no seams across the ridges).
    uv = obj.data.uv_layers.active.data
    for loop in obj.data.loops:
        co = obj.data.vertices[loop.vertex_index].co
        uv[loop.index].uv = ((co.x + EXTENT) / (2 * EXTENT), (co.y + EXTENT) / (2 * EXTENT))
    return obj


def build(kit):
    bed = build_bed(kit, seeds.loam(kit, "M_TilledBed", seed=5.0))
    lod = bed.copy()
    lod.data = bed.data.copy()
    lod.name = lod.data.name = "SM_TilledBed_LOD1"
    kit._link(lod)
    mod = lod.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = 0.25
    mod.use_collapse_triangulate = True
    kit.apply_modifiers(lod)
    return [bed, lod]
