"""Pickaxe rubble: a heap of dressed and broken granite blocks shed from a ruined wall (~0.9 m).

Reference: the tumbled walls of abandoned 18th- and 19th-century farmsteads, count houses and
engine houses on the Cornish moors (Carn Brea, Bodmin Moor, Penwith). Their granite was
rough-squared from moorstone and dressed with the punch and pick: flat faces covered in
small pecked pits, arrises (edges) square but knocked and chipped where they fell. When the
lime mortar washes out, the blocks slump into a heap at the wall foot. Broken blocks show
paler, rough, conchoidal fracture faces. After decades on the ground the dressed faces grey
over and crust with lichen, moss creeps along the shaded foot and the turf closes round them.

The heap: a long rough-squared block lying flat, a second block that slid down onto it and
leans with one end in the turf, a broken block fragment, a small quoin fragment, a broken
chunk and a few spalls. About 0.9 m across and 0.45 m high: plainly too big to lift, worked
with a pick (the Rubble node).

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import math
import random

import numpy as np

import homestead_rocks as rocks

NAME = "GranitePickRubble"
DESCRIPTION = ("Pickaxe rubble: a heap of dressed and broken Cornish granite blocks shed from a ruined wall "
               "(~0.9 m across, ~0.45 m high); Nanite; LOD1/LOD2.")
COLLISION = "none"
TRIANGLE_BUDGET = 180000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.05, -0.15, 0.22), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 3.2}
REPORT = {"nanite": True}
NOTES = {}

GROUND_Z = 0.03

# Blocks: center (m), half extents (m), rotate (yaw, pitch, roll degrees), blockiness, and broken
# faces (local normal, local distance) that cut the dressed block with a rough fracture.
BLOCKS = [
    dict(center=(-0.1, 0.06, 0.12), radii=(0.26, 0.16, 0.14), rotate=(18, 0, 3), power=6.0,
         broken=[((-1.0, -0.2, 0.3), 0.215)]),                             # long block, west end broken off
    dict(center=(0.24, -0.03, 0.217), radii=(0.21, 0.14, 0.12), rotate=(-10, 40, 0), power=6.0,
         broken=[]),                                                        # slid down, leaning on it
    dict(center=(0.06, -0.27, 0.075), radii=(0.15, 0.12, 0.09), rotate=(35, 8, -6), power=5.0,
         broken=[((0.7, 0.6, 0.4), 0.07), ((-0.5, 0.8, 0.25), 0.08)]),       # broken block fragment
    dict(center=(-0.35, -0.18, 0.055), radii=(0.11, 0.09, 0.07), rotate=(60, 0, 12), power=5.0,
         broken=[((1.0, -0.3, 0.4), 0.05)]),                                # quoin fragment
    dict(center=(0.1, 0.27, 0.05), radii=(0.1, 0.08, 0.065), rotate=(-25, 6, 0), power=3.2,
         broken=[((0.6, -0.5, 0.6), 0.03), ((-0.7, 0.2, 0.5), 0.04)]),       # broken chunk
]


def _frame(spec):
    rotation = rocks.rotation(*spec["rotate"])
    center = np.asarray(spec["center"], dtype=np.float64)
    planes = []
    for n, d in spec["broken"]:
        world = rotation @ rocks.unit(n)
        planes.append((world, float(d + center @ world)))
    return rotation, center, planes


def _block(index, spec, material, seed):
    rotation, center, planes = _frame(spec)
    r = spec["radii"]
    dressed = [((sx * (axis == 0), sx * (axis == 1), sx * (axis == 2)), r[axis] * 0.97, 0.012)
               for axis in range(3) for sx in (1.0, -1.0)]
    field = rocks.boulder(radii=r, power=spec["power"], lumps=0.006, lump_scale=1.5, seed=seed,
                          center=center, rotate=rotation, joints=dressed)
    for k, (n, d) in enumerate(planes):
        field = rocks.cut(field, n, d, rounding=0.006, wobble=0.012, wobble_scale=7.0, seed=seed + k)
    return rocks.solid(f"Rubble{index}", field, subdivisions=7, material=material, r_max=max(r) * 3.0)


def _spalls(rng, seed):
    """Loose spalls knocked off the arrises, lying round the heap foot."""
    spalls = []
    for index, (angle, reach) in enumerate([(4.3, 0.3), (5.3, 0.36), (2.4, 0.36), (0.4, 0.4)]):
        size = rng.uniform(0.04, 0.065)
        planes = [((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 1)), size * rng.uniform(0.4, 0.6),
                   size * 0.04) for _ in range(3)]
        spalls.append(dict(center=(reach * math.cos(angle), reach * math.sin(angle), size * 0.25),
                           radii=(size, size * rng.uniform(0.6, 0.85), size * rng.uniform(0.35, 0.5)),
                           power=3.0, lumps=0.05, seed=seed + index, rotate=(rng.uniform(0, 360), 0, 0),
                           joints=planes, subdivisions=6))
    return spalls


def build(kit):
    mat = kit.mats.granite("M_GranitePickRubble", grain=0.0035, scale=0.5, patina=0.72, lichen=0.8, moss=0.22,
                           iron=0.3, streaks=0.15, soil=0.6, soil_height=0.05, enclaves=0.35, megacrysts=0.7,
                           film=0.6, spots=1.0, seed=47.0)
    pieces = [_block(i, spec, mat, 4700 + 17 * i) for i, spec in enumerate(BLOCKS)]
    pieces += rocks.scatter("Spall", _spalls(random.Random(4790), 4790), mat)
    frames = [_frame(spec) for spec in BLOCKS]

    def detail(points, normal):
        # Which block owns each point (spalls fall back to the nearest block; they're tiny).
        g = []
        for (rotation, center, _), spec in zip(frames, BLOCKS):
            local = (points - center) @ rotation
            g.append(np.max(np.abs(local) / np.asarray(spec["radii"]), axis=1))
        owner = np.argmin(np.stack(g, 1), axis=1)
        dressed = np.zeros(len(points))
        broken = np.zeros(len(points))
        for k, (rotation, center, planes) in enumerate(frames):
            mine = owner == k
            local_n = normal[mine] @ rotation
            dressed[mine] = rocks.smoothstep(0.9, 0.985, np.abs(local_n).max(axis=1))
            for n, d in planes:
                on = np.exp(-(np.maximum(d - points[mine] @ n, 0) / 0.02) ** 2) * np.clip(normal[mine] @ n, 0, 1)
                broken[mine] = np.maximum(broken[mine], on)
        dressed = dressed * (1.0 - broken)
        edge = np.clip(1.0 - dressed - broken, 0, 1)
        # Punch-dressed faces: dense pecked pits over a slight hand-worked waviness.
        pecked = rocks.relief(points, normal, 4801, [(0.22, 0.0018), (0.018, 0.0011), (0.006, 0.0005)])
        pits = -np.abs(rocks.relief(points, normal, 4802, [(0.011, 0.0016)]))
        fracture = rocks.relief(points, normal, 4803, [(0.12, 0.006), (0.035, 0.0025), (0.01, 0.0008)])
        grain = rocks.relief(points, normal, 4804, [(0.004, 0.0003)])
        chips, chipped = rocks.plates(points, normal, seed=4805, cell=0.07, thickness=0.011, coverage=0.4,
                                      width=0.006, wobble=0.3)
        chips = chips * (0.15 + 0.85 * edge)
        offset = dressed * (pecked + pits) + broken * fracture + edge * pecked * 0.5 + grain - chips
        fresh = np.clip(np.maximum(broken * 0.85, chipped * edge * 0.7), 0, 1)
        return offset, {"fresh": fresh}

    meshes, sink = rocks.finish(kit, pieces, "SM_GranitePickRubble", 150000, (30000, 7000), ground_z=GROUND_Z,
                                detail=detail, union=False)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (blocks extend {sink * 100:.0f} cm "
                               "below it). Walk-through resource node (Rubble, worn pick): no collision.",
                  "north": "+Y (moss side)"})
    return meshes
