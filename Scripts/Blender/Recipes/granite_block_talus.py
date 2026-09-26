"""Medium angular granite talus block (~0.9 m).

Reference: the rockfall talus below the Yosemite Valley walls and along the Tioga
Road cuts: blocks broken out along joints and sheeting planes, so they are bounded
by several flat fracture faces meeting at sharp-to-slightly-rounded edges, with
chipped corners. Talus is young: faces are pale, fresh and crisply speckled, with
only a little lichen and rust on the older (weathered) faces and edges chipped in
small conchoidal scars. It sits a few centimetres into the soil.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north, the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteBlockTalus"
DESCRIPTION = "Medium (0.9 m) angular granite talus block with fresh fracture faces and chipped edges; LOD1/LOD2."
COLLISION = "convex"
TRIANGLE_BUDGET = 100000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.15, -0.25, 0.4), "ground": "origin",
          "views": ["hero", "detail", "eye"]}
NOTES = {}

# Joint faces: (normal, distance, rounding, weathered?) - old faces weather grey and grow lichen,
# the recent rockfall fractures stay fresh.
FACES = [((0.0, 0.08, -1.0), 0.26, 0.03, True),
         ((0.25, 0.3, 1.0), 0.3, 0.04, True),
         ((0.45, -1.0, 0.35), 0.3, 0.02, False),
         ((1.0, 0.45, -0.25), 0.42, 0.025, False),
         ((-1.0, 0.1, 0.45), 0.4, 0.03, True),
         ((-0.35, 1.0, -0.2), 0.32, 0.03, True),
         ((0.8, 0.5, 0.9), 0.46, 0.02, False),
         ((-0.7, -0.8, 0.5), 0.45, 0.02, False),
         ((0.6, -0.4, -0.6), 0.4, 0.03, True)]


def build(kit):
    mat = kit.mats.granite("M_GraniteBlockTalus", grain=0.003, scale=0.9, patina=0.55, lichen=0.3, moss=0.05,
                           iron=0.3, streaks=0.05, soil=0.35, soil_height=0.06, enclaves=0.5, megacrysts=0.6,
                           film=0.3, seed=11.0)
    field = rocks.boulder(radii=(0.6, 0.45, 0.4), power=3.0, lumps=0.04, lump_scale=1.4, seed=111,
                          joints=[(n, d, r) for n, d, r, _ in FACES])
    block = rocks.solid("Talus", field, subdivisions=7, material=mat, r_max=2.0)

    def detail(points, normal):
        fresh = np.zeros(len(points))
        for n, d, _, old in FACES:
            if not old:
                n = rocks.unit(n)
                on_face = np.exp(-(np.maximum(d - points @ n, 0) / 0.03) ** 2) * np.clip(normal @ n, 0, 1)
                fresh = np.maximum(fresh, on_face)
        chips, chipped = rocks.plates(points, normal, seed=112, cell=0.11, thickness=0.016, coverage=0.38,
                                      width=0.008, wobble=0.3)
        edge = 1.0 - np.clip(np.abs(np.stack([normal @ rocks.unit(n) for n, *_ in FACES], 1)).max(1) * 1.15 - 0.15, 0, 1)
        grain = rocks.relief(points, normal, 113, [(0.25, 0.003), (0.05, 0.0012), (0.015, 0.0005)])
        return grain - chips * (0.3 + 0.7 * edge), {"fresh": np.clip(np.maximum(fresh * 0.85, chipped * edge), 0, 1)}

    meshes, sink = rocks.finish(kit, [block], "SM_GraniteBlockTalus", 90000, (20000, 5000), ground_z=-0.2,
                                detail=detail)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (already sunk {sink * 100:.0f} cm).",
                  "collision": "Convex hull (the block is convex).",
                  "north": "+Y"})
    return meshes
