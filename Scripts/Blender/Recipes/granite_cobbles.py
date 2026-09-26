"""Small ground cluster: rounded granite cobbles half-buried in the forest floor (~1 m).

Reference: the loose stones that litter the duff and grus between the pines in the
Sierra foothills and around Yosemite: granodiorite cores weathered out of decomposed
granite, rounded to sub-angular, 10-40 cm, one bigger stone with smaller ones settled
round it. Each sits a third to half sunk into soil, stained brown at the soil line,
with a little lichen on the tops and moss on the shaded north sides.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import math
import random

import homestead_rocks as rocks

NAME = "GraniteCobbles"
DESCRIPTION = "Small ground cluster of 8 half-buried rounded granite stones (10-38 cm) in a ~1.1 m footprint; LOD1/LOD2."
COLLISION = "none"
TRIANGLE_BUDGET = 80000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.05, 0.08), "ground": "origin", "views": ["hero", "detail", "eye"],
          "eye_distance": 3.0}
NOTES = {}


def layout(seed=2601):
    rng = random.Random(seed)
    stones = [dict(center=(0.02, 0.04, 0.0), radii=(0.19, 0.155, 0.125), power=2.5, lumps=0.06, seed=seed,
                   rotate=(20, 4, -3), joints=[((0, 0, 1), 0.1, 0.06), ((1, 0.2, 0), 0.17, 0.05)],
                   subdivisions=7)]
    for index in range(7):
        angle = index * 2.4 + rng.uniform(-0.5, 0.5)
        size = rng.choice([0.05, 0.06, 0.08, 0.1, 0.12, 0.14]) * rng.uniform(0.9, 1.1)
        reach = 0.16 + size * 0.7 + rng.uniform(0.0, 0.18)
        flat = rng.uniform(0.5, 0.85)
        radii = (size, size * rng.uniform(0.65, 0.9), size * flat)
        angular = rng.random() < 0.5
        joints = [((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.3, 1)), radii[0] * 0.65, radii[0] * 0.12),
                  ((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.3, 0.5)), radii[0] * 0.75, radii[0] * 0.1)] \
            if angular else []
        lift = radii[2] * rng.uniform(-0.3, 0.35)     # a third to most of each stone is buried
        stones.append(dict(center=(reach * math.cos(angle), reach * math.sin(angle), lift),
                           radii=radii, power=rng.uniform(2.0, 2.5), lumps=rng.uniform(0.05, 0.09),
                           seed=seed + index + 1, rotate=(rng.uniform(0, 360), rng.uniform(-12, 12),
                                                          rng.uniform(-12, 12)), joints=joints))
    return stones


def build(kit):
    mat = kit.mats.granite("M_GraniteCobbles", grain=0.003, scale=0.35, patina=0.75, lichen=0.55, moss=0.3,
                           iron=0.25, streaks=0.0, soil=0.7, soil_height=0.035, enclaves=0.3, film=0.6,
                           spots=1.2, seed=21.0)
    pieces = rocks.scatter("Cobble", layout(), mat)

    def detail(points, normal):
        return rocks.relief(points, normal, 2602, [(0.12, 0.002), (0.03, 0.0008), (0.01, 0.0003)])

    meshes, sink = rocks.finish(kit, pieces, "SM_GraniteCobbles", 60000, (15000, 3500), ground_z=0.0,
                                detail=detail, union=False)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (stones extend {sink * 100:.0f} cm "
                               "below it). Walkable decoration: no collision.",
                  "north": "+Y (moss side)"})
    return meshes
