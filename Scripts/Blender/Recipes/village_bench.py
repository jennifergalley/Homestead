"""Village bench: a two-person green-oak plank bench on carved end slabs, weathered grey.

Real-object research (written before modeling):
- Churchyard, green and well-side benches in English villages are slab-ended: two thick oak
  end slabs, each cut from one plank to a waisted ogee outline with an arched cut-out between
  two feet, carry a seat of one or two 45-55 mm oak planks. A through-tenoned stretcher locks
  the slabs together low down (its tenons show proud of the outer faces, held by small wedges),
  and thin aprons under the front and rear edges stop the planks cupping and spreading.
- Seat 450 mm high, 400-450 mm deep, about 1.6 m long for two sitters; the planks overhang
  the slabs by roughly a hand's width. The seat is pegged down through into the slab tops with
  oak pegs, whose end grain stands a few millimetres proud and weathers pale then dark.
- Oak left out for decades goes silver-grey, checks along the grain and at the ends, and wears
  into a shallow dish where people sit; the feet go dark and mossy where they meet the turf.

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Units are metres, Z up. SM_VillageBench PIVOT: centre of the seat footprint on the ground line
(z = 0). Long axis along Y; the sitter faces +X. Backless and symmetric, so it can be placed
facing either way. Feet sink 1 cm below z = 0 so it sits into uneven turf.
"""
import importlib.util
import math
import os
import random
from pathlib import Path

import bpy
from mathutils import Vector, noise
from mathutils.geometry import tessellate_polygon

_spec = importlib.util.spec_from_file_location("farm_common", Path(__file__).with_name("farm") / "common.py")
common = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(common)

NAME = "VillageBench"
DESCRIPTION = ("Two-person weathered oak plank bench, 1.6 m long, seat 0.45 m high, on carved end slabs "
               "with a through-tenoned stretcher and aprons; pivot = seat centre on the ground (original).")
COLLISION = "box"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 64,
        "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.30), "views": ["hero", "detail", "eye"], "eye_distance": 3.2}
NOTES = {"SM_VillageBench": ("Pivot = seat footprint centre on the ground line. Long axis Y (1.6 m seat), "
                             "sitter faces +X, 0.45 m seat height, 0.45 m deep. Backless and symmetric.")}
REPORT = {"pivot": "centre of the seat footprint at ground level", "seat_height_m": 0.45,
          "long_axis": "Y", "facing": "+X (symmetric, backless)"}

SEAT_TOP = 0.45
SEAT_THICK = 0.052
SEAT_HALF_LEN = 0.80
PLANK_W = 0.221
PLANK_GAP = 0.008
SLAB_Y = 0.67               # slab centre-plane distance from the middle
SLAB_T = 0.060
SLAB_TOP = SEAT_TOP - SEAT_THICK + 0.004
FOOT_SINK = 0.010


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def slab_profile():
    """Carved end slab outline in (x, z): ogee waist, two feet and an arched cut-out between them."""
    right = []
    steps = 16
    for i in range(steps + 1):
        z = -FOOT_SINK + (SLAB_TOP + FOOT_SINK) * i / steps
        if z < 0.23:
            half = 0.150 + 0.058 * ((0.23 - z) / 0.23) ** 2.2
        else:
            half = 0.150 + 0.080 * smoothstep(0.0, 1.0, (z - 0.23) / (SLAB_TOP - 0.23))
        right.append((half, z))
    left = [(-x, z) for x, z in reversed(right)]
    arch = [(-0.112, -FOOT_SINK), (-0.108, 0.040), (-0.082, 0.078), (-0.042, 0.097), (0.0, 0.103),
            (0.042, 0.097), (0.082, 0.078), (0.108, 0.040), (0.112, -FOOT_SINK)]
    return right + left + arch


def slab(kit, name, y_centre, material, seed):
    """Extrude the carved outline through the slab thickness, round the arrises, add a little
    chisel/adze irregularity to the silhouette."""
    profile = slab_profile()
    count = len(profile)
    verts = []
    for y in (y_centre - SLAB_T / 2, y_centre + SLAB_T / 2):
        verts += [(x, y, z) for x, z in profile]
    faces = []
    tris = tessellate_polygon([[Vector((x, z, 0.0)) for x, z in profile]])
    for a, b, c in tris:
        faces.append((count + a, count + b, count + c))     # +Y cap
        faces.append((c, b, a))                              # -Y cap
    for i in range(count):
        j = (i + 1) % count
        faces.append((i, count + i, count + j, j))
    obj = kit.mesh(name, verts, faces, material)
    kit.recalc_normals(obj)
    kit._bevel(obj, 0.006, 2)
    kit.apply_modifiers(obj)
    kit.roughen(obj, strength=0.0022, scale=7.0, seed=seed, subdivide=1)
    kit.recalc_normals(obj)
    return obj


def seat_wear(kit, plank, centre_x):
    """Sitters dish the top of each plank a few millimetres and round its front edge."""
    def fn(co):
        top = min(max((co.z - (SEAT_TOP - 0.026)) / 0.026, 0.0), 1.0)
        dish = 0.0035 * math.exp(-((abs(co.y) - 0.38) / 0.24) ** 2) * (1.0 - 0.6 * abs(co.x - centre_x) / 0.11)
        return Vector((co.x, co.y, co.z - top * max(dish, 0.0)))
    return kit.warp(plank, fn)


def moss_blob(kit, name, centre, radius, material, seed):
    blob = kit.sphere(name, radius, location=centre, material=material, segments=10, rings=6,
                      scale=(1.0, 1.0, 0.28))
    kit.roughen(blob, strength=radius * 0.35, scale=22.0, seed=seed)
    return blob


def build(kit):
    rng = random.Random(2161)
    oak = common.weathered_oak(kit, "M_VillageBenchOak", seed=8.0, lichen=0.28, grime=0.40)
    oak_slab = common.weathered_oak(kit, "M_VillageBenchSlabOak", seed=15.0, lichen=0.22, grime=0.55)
    pale = common.weathered_oak(kit, "M_VillageBenchPegOak", seed=23.0, lichen=0.05, grime=0.15)
    moss = common.moss_lichen(kit, "M_VillageBenchMoss", seed=6.0)
    common.zero_subsurface(oak, oak_slab, pale, moss)

    parts = []
    # Seat: two thick planks with a hand's width of overhang past the end slabs.
    for index, cx in enumerate((-(PLANK_W + PLANK_GAP) / 2, (PLANK_W + PLANK_GAP) / 2)):
        z = SEAT_TOP - SEAT_THICK / 2
        plank = common.beam(kit, f"SeatPlank{index}", [(cx, -SEAT_HALF_LEN, z), (cx, SEAT_HALF_LEN, z)],
                            PLANK_W, SEAT_THICK, oak, 31 + index * 7, spacing=0.09)
        parts.append(seat_wear(kit, plank, cx))

    # Carved end slabs, then the stretcher with proud through-tenons and wedges.
    for side, name in ((-1, "SlabW"), (1, "SlabE")):
        parts.append(slab(kit, name, side * SLAB_Y, oak_slab, 40 + side))
    parts.append(common.beam(kit, "Stretcher", [(0.0, -SLAB_Y - SLAB_T / 2 - 0.030, 0.30),
                                                (0.0, SLAB_Y + SLAB_T / 2 + 0.030, 0.30)],
                             0.072, 0.092, oak_slab, 53, spacing=0.12))
    for side in (-1, 1):
        parts.append(kit.box(f"Wedge{side}", (0.012, 0.008, 0.075),
                             (0.0, side * (SLAB_Y + SLAB_T / 2 + 0.018), 0.30), rotation=(0, 0, 0),
                             material=pale, bevel=0.0015))

    # Aprons keep the seat planks flat; they run between the slabs just under the seat.
    inner = SLAB_Y - SLAB_T / 2 + 0.004
    for side, label in ((-1, "ApronRear"), (1, "ApronFront")):
        parts.append(common.beam(kit, label, [(side * 0.178, -inner, SLAB_TOP - 0.040),
                                              (side * 0.178, inner, SLAB_TOP - 0.040)],
                                 0.030, 0.078, oak_slab, 61 + side, spacing=0.14))

    # Oak pegs through the seat into the slab tops.
    for sy in (-1, 1):
        for sx in (-1, 1):
            peg = kit.cylinder(f"Peg{sx}{sy}", 0.0115, 0.010, location=(sx * 0.108, sy * SLAB_Y, SEAT_TOP + 0.0006),
                               material=pale, sides=14, bevel=0.0018)
            parts.append(peg)

    # Moss on the shaded feet and the north (rear) slab faces, a few cushions on the seat ends.
    spots = [(-0.150, -0.70, 0.004), (0.152, -0.70, 0.004), (-0.153, 0.70, 0.004), (0.150, 0.70, 0.004),
             (-0.01, -0.64, 0.004), (0.02, 0.64, 0.004), (-0.148, 0.64, 0.05), (-0.150, -0.64, 0.07)]
    for i, (x, y, z) in enumerate(spots):
        parts.append(moss_blob(kit, f"Moss{i}", (x, y + rng.uniform(-0.03, 0.03), z),
                               rng.uniform(0.030, 0.052), moss, 80 + i))
    for i, (x, y) in enumerate(((-0.205, -0.74), (-0.215, 0.58), (0.205, 0.77), (-0.10, 0.795))):
        parts.append(moss_blob(kit, f"SeatMoss{i}", (x, y, SEAT_TOP - 0.002), rng.uniform(0.026, 0.040), moss, 95 + i))

    return kit.join(parts, "SM_VillageBench", pivot=None, unwrap=True, reshade=True, smooth_angle=48)
