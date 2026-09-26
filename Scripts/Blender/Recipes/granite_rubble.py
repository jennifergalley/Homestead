"""Small ground cluster: mixed granite rubble and grus gravel (~1 m).

Reference: where a weathered granite outcrop crumbles into the forest floor near
Yosemite and the Sierra foothills: a few sub-angular stones (20-35 cm) with smaller
stones and a spill of coarse grus chips (1-4 cm) round them, all partly sunk in the
decomposed-granite soil. Tops weathered grey with lichen, moss on the shaded side.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import math
import random

import homestead_rocks as rocks

NAME = "GraniteRubble"
DESCRIPTION = "Small ground cluster: 3 sub-angular granite stones, 5 small stones and grus chips (1-35 cm); LOD1/LOD2."
COLLISION = "none"
TRIANGLE_BUDGET = 90000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.1, -0.1, 0.1), "ground": "origin", "views": ["hero", "detail", "eye"],
          "eye_distance": 3.0}
NOTES = {}


def _stone(rng, center, size, seed, joints, subdivisions=6):
    radii = (size, size * rng.uniform(0.7, 0.9), size * rng.uniform(0.55, 0.8))
    planes = [((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.3, 1)), size * rng.uniform(0.55, 0.8),
               size * rng.uniform(0.05, 0.15)) for _ in range(joints)]
    return dict(center=(center[0], center[1], radii[2] * rng.uniform(-0.1, 0.3) + center[2]), radii=radii,
                power=rng.uniform(2.4, 3.2), lumps=rng.uniform(0.04, 0.08), seed=seed,
                rotate=(rng.uniform(0, 360), rng.uniform(-15, 15), rng.uniform(-15, 15)), joints=planes,
                subdivisions=subdivisions)


def layout(seed=2801):
    rng = random.Random(seed)
    stones = [_stone(rng, (-0.18, 0.08, 0.0), 0.17, seed, 3, 7), _stone(rng, (0.2, 0.14, 0.0), 0.13, seed + 1, 3, 7),
              _stone(rng, (0.04, -0.22, 0.0), 0.11, seed + 2, 2, 7)]
    for index in range(5):
        angle = rng.uniform(0, 6.28)
        reach = rng.uniform(0.25, 0.5)
        stones.append(_stone(rng, (reach * math.cos(angle), reach * math.sin(angle), 0.0), rng.uniform(0.04, 0.07),
                             seed + 10 + index, 2))
    for index in range(22):
        angle = rng.uniform(0, 6.28)
        reach = rng.uniform(0.12, 0.42)
        stones.append(_stone(rng, (reach * math.cos(angle), reach * math.sin(angle), -0.004), rng.uniform(0.01, 0.026),
                             seed + 30 + index, 2, 5))
    return stones


def build(kit):
    mat = kit.mats.granite("M_GraniteRubble", grain=0.003, scale=0.35, patina=0.7, lichen=0.5, moss=0.3,
                           iron=0.35, streaks=0.0, soil=0.7, soil_height=0.03, enclaves=0.3, film=0.5,
                           spots=1.0, seed=25.0)
    pieces = rocks.scatter("Rubble", layout(), mat)

    def detail(points, normal):
        return rocks.relief(points, normal, 2802, [(0.1, 0.0018), (0.025, 0.0007), (0.008, 0.0003)])

    meshes, sink = rocks.finish(kit, pieces, "SM_GraniteRubble", 70000, (17000, 4000), ground_z=0.0,
                                detail=detail, union=False)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (stones extend {sink * 100:.0f} cm "
                               "below it). Walkable decoration: no collision.",
                  "north": "+Y (moss side)"})
    return meshes
