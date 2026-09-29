"""Hand-gathered stones: three little piles of loose Cornish granite pebbles and cobbles.

Reference: the loose moorstone "clitter" and field-clearance stones of Bodmin Moor, Carnmenellis
and West Penwith: coarse, porphyritic biotite granite with white K-feldspar megacrysts (2-4 cm)
in a grey quartz-feldspar groundmass flecked with black biotite, weathered to rounded and
sub-rounded cobbles, the odd sub-angular spall still showing a joint face. Stones picked off a
field sit ON the turf, barely bedded, with soil on their undersides and only a little lichen:
unlike the pickaxe rocks, which are big, half-buried and lichen-crusted, these read at a glance
as something she can lift.

Each pile is 6-8 separate stones of 5-18 cm, ankle-high at most (under ~12 cm), in a 0.4-0.5 m
footprint:
  A  five stones round one 18 cm sub-angular cobble;
  B  seven flatter, slabby spalls and pebbles, one leaning on another;
  C  eight mixed rounded cobbles and pebbles.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; the ground line is at z = 0 (origin), +Y is north (moss side).
"""
import math
import random

import homestead_rocks as rocks

NAME = "GraniteHandPile"
DESCRIPTION = ("Three piles (A/B/C) of 6-8 loose, hand-sized Cornish granite stones (5-18 cm, ankle-high) "
               "sitting on the turf; Nanite; LOD1/LOD2 fallback.")
COLLISION = "none"
TRIANGLE_BUDGET = 40000
BAKE = {"size": 1024, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.06), "ground": "origin", "views": ["hero", "detail", "eye"],
          "eye_distance": 1.3}
REPORT = {"nanite": True}
NOTES = {}

# (length m, width/length, height/length, shape): shape "round" | "sub" (sub-angular) | "slab"
PILES = {
    "A": dict(seed=3101, stones=[(0.18, 0.72, 0.55, "sub"), (0.13, 0.8, 0.62, "sub"),
                                 (0.11, 0.75, 0.6, "round"), (0.09, 0.8, 0.65, "sub"),
                                 (0.07, 0.8, 0.62, "round"), (0.06, 0.75, 0.6, "sub")]),
    "B": dict(seed=3102, stones=[(0.18, 0.7, 0.36, "slab"), (0.16, 0.75, 0.34, "slab"),
                                 (0.13, 0.72, 0.4, "slab"), (0.1, 0.8, 0.45, "sub"),
                                 (0.08, 0.78, 0.5, "round"), (0.06, 0.8, 0.55, "sub"),
                                 (0.05, 0.85, 0.6, "round")], lean=True),
    "C": dict(seed=3103, stones=[(0.15, 0.8, 0.6, "sub"), (0.13, 0.78, 0.58, "round"),
                                 (0.11, 0.82, 0.62, "sub"), (0.1, 0.8, 0.6, "round"),
                                 (0.08, 0.85, 0.65, "sub"), (0.07, 0.75, 0.55, "sub"),
                                 (0.06, 0.8, 0.6, "round"), (0.05, 0.8, 0.6, "sub")]),
}


def layout(spec):
    rng = random.Random(spec["seed"])
    placed = []
    stones = []
    for index, (length, wide, high, shape) in enumerate(spec["stones"]):
        rx = length * 0.5 * rng.uniform(0.95, 1.05)
        radii = (rx, rx * wide, rx * high)
        footprint = max(radii[0], radii[1])
        if index == 0:
            xy = (rng.uniform(-0.03, 0.03), rng.uniform(-0.03, 0.03))
        else:
            for _attempt in range(200):
                angle = rng.uniform(0, 2 * math.pi)
                reach = placed[0][2] + footprint * rng.uniform(0.75, 1.05) + rng.uniform(0.0, 0.12)
                xy = (reach * math.cos(angle), reach * math.sin(angle))
                if all(math.hypot(xy[0] - x, xy[1] - y) > (r + footprint) * 0.86 for x, y, r in placed):
                    break
        placed.append((xy[0], xy[1], footprint))
        yaw = rng.uniform(0, 360)
        pitch = rng.uniform(-8, 8)
        roll = rng.uniform(-8, 8)
        # Lying on the turf: sunk only 10-25 % of its height.
        z = radii[2] * (1.0 - 2.0 * rng.uniform(0.1, 0.25))
        if spec.get("lean") and index == 2:
            # One spall propped against the first slab.
            x0, y0, r0 = placed[0]
            angle = math.atan2(xy[1] - y0, xy[0] - x0)
            xy = (x0 + math.cos(angle) * (r0 + radii[0] * 0.55), y0 + math.sin(angle) * (r0 + radii[0] * 0.55))
            placed[-1] = (xy[0], xy[1], footprint)
            yaw = math.degrees(angle)
            pitch = -24.0
            z = radii[2] + radii[0] * 0.28
        if shape == "round":
            power, lumps, joints = rng.uniform(2.0, 2.3), rng.uniform(0.06, 0.09), []
        elif shape == "sub":
            power, lumps = rng.uniform(2.3, 2.7), rng.uniform(0.07, 0.1)
            joints = [((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.2, 0.8)), radii[0] * 0.56, radii[0] * 0.05),
                      ((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.2, 0.3)), radii[0] * 0.64, radii[0] * 0.05)]
        else:
            power, lumps = rng.uniform(2.6, 3.0), rng.uniform(0.04, 0.06)
            joints = [((0, 0, 1), radii[2] * 0.8, radii[2] * 0.25), ((0, 0, -1), radii[2] * 0.8, radii[2] * 0.25),
                      ((rng.uniform(-1, 1), rng.uniform(-1, 1), 0.0), radii[0] * 0.7, radii[0] * 0.08)]
        stones.append(dict(center=(xy[0], xy[1], z), radii=radii, power=power, lumps=lumps, lump_scale=1.4,
                           seed=spec["seed"] * 10 + index, rotate=(yaw, pitch, roll), joints=joints,
                           subdivisions=6))
    return stones


def pile(kit, key, spec):
    mat = kit.mats.granite(f"M_GraniteHandPile_{key}",     grain=0.003, scale=0.25, patina=0.9, lichen=0.45,
                               moss=0.1, iron=0.35, streaks=0.0, soil=0.65, soil_height=0.025, enclaves=0.3,
                               megacrysts=0.8, film=0.65, spots=1.0, seed=31.0 + ord(key))
    pieces = rocks.scatter(f"Pile{key}", layout(spec), mat)

    def detail(points, normal):
        return rocks.relief(points, normal, spec["seed"] + 7, [(0.08, 0.0016), (0.025, 0.0006), (0.008, 0.00025)])

    meshes, sink = rocks.finish(kit, pieces, f"SM_GraniteHandPile_{key}", 30000, (7000, 1800), ground_z=0.0,
                                detail=detail, union=False)
    NOTES[key] = {"sink_depth_m": round(sink, 3)}
    return meshes


def build(kit):
    meshes = []
    for key, spec in PILES.items():
        meshes += pile(kit, key, spec)
    NOTES.update({"placement": "Origin is the ground line; place at terrain height. Walk-through: no collision.",
                  "north": "+Y (moss side)",
                  "use": "Stones resource node (hand-gathered). Keep distinct from the pickaxe rocks "
                         "(GranitePickRocks): small, loose, on top of the turf, little lichen."})
    return meshes
