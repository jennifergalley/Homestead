"""Small ground cluster: platy exfoliation spalls under a granite face (~1 m).

Reference: the litter of thin, curved granite flakes that collects at the foot of
exfoliating boulders and slabs all over the Sierra (the "onion skin" pieces below
domes and sheeted boulders): plates 15-40 cm across and 3-8 cm thick, slightly
curved like the surface they peeled from, broken along angular edges, lying
overlapped and tilted on the soil with a few small chips. The upper sides are the
old weathered skin (grey, lichen); the undersides and broken edges are fresh.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north, the ground line is at z = 0.
"""
import math
import random

import numpy as np

import homestead_rocks as rocks

NAME = "GraniteSpalls"
DESCRIPTION = "Small ground cluster of 7 curved exfoliation spall plates and chips (5-40 cm), ~1.1 m footprint; LOD1/LOD2."
COLLISION = "none"
TRIANGLE_BUDGET = 80000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.05, -0.05, 0.05), "ground": "origin", "views": ["hero", "detail", "eye"],
          "eye_distance": 3.0}
NOTES = {}


def layout(seed=2701):
    rng = random.Random(seed)
    stones = []
    for index in range(7):
        angle = index * 2.2 + rng.uniform(-0.5, 0.5)
        reach = rng.uniform(0.0, 0.38) if index else 0.0
        width = rng.uniform(0.09, 0.2)
        thick = rng.uniform(0.016, 0.035)
        radii = (width, width * rng.uniform(0.6, 0.85), thick)
        start = rng.uniform(0, 6.28)
        joints = [((math.cos(a), math.sin(a) * radii[0] / radii[1], 0.0), radii[0] * rng.uniform(0.5, 0.72), 0.004)
                  for a in (start + k * 1.25 + rng.uniform(-0.3, 0.3) for k in range(5))]
        tilt = rng.uniform(4, 28)
        stones.append(dict(center=(reach * math.cos(angle), reach * math.sin(angle), thick * 0.4 + 0.012 * index / 6),
                           radii=radii, power=2.6, lumps=0.04, lump_scale=1.2, seed=seed + index,
                           rotate=(rng.uniform(0, 360), tilt, rng.uniform(-10, 10)), joints=joints,
                           bend=rng.uniform(0.25, 0.5), subdivisions=7))
    for index in range(5):
        angle = rng.uniform(0, 6.28)
        reach = rng.uniform(0.2, 0.5)
        size = rng.uniform(0.02, 0.045)
        stones.append(dict(center=(reach * math.cos(angle), reach * math.sin(angle), size * 0.1),
                           radii=(size, size * 0.8, size * 0.45), power=2.8, lumps=0.08, seed=seed + 20 + index,
                           rotate=(rng.uniform(0, 360), rng.uniform(-20, 20), 0),
                           joints=[((rng.uniform(-1, 1), rng.uniform(-1, 1), 1.0), size * 0.3, 0.003)], subdivisions=5))
    return stones


def build(kit):
    mat = kit.mats.granite("M_GraniteSpalls", grain=0.003, scale=0.35, patina=0.8, lichen=0.6, moss=0.15,
                           iron=0.3, streaks=0.0, soil=0.6, soil_height=0.02, enclaves=0.3, film=0.6,
                           spots=1.2, seed=23.0)
    pieces = rocks.scatter("Spall", layout(), mat)

    def detail(points, normal):
        # The downward faces and steep broken rims are fresh rock; the upper skin is old.
        fresh = np.clip(0.3 - normal[:, 2], 0, 1)
        return rocks.relief(points, normal, 2702, [(0.1, 0.0015), (0.025, 0.0006), (0.008, 0.00025)]), \
            {"fresh": fresh}

    meshes, sink = rocks.finish(kit, pieces, "SM_GraniteSpalls", 60000, (15000, 3500), ground_z=0.0,
                                detail=detail, union=False)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (pieces extend {sink * 100:.0f} cm "
                               "below it). Walkable decoration: no collision. Pairs with the foot of the big "
                               "exfoliating rocks.",
                  "north": "+Y"})
    return meshes
