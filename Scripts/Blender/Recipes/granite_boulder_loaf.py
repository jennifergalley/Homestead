"""Medium granite boulder: a rounded, loaf-shaped Sierra Nevada corestone.

Reference: the knee- to hip-high boulders strewn through the pine forest between
Wawona and Yosemite Valley and around Tuolumne Meadows. They are corestones of
granodiorite, rounded where three joint sets once blocked the rock out, then
weathered free of the surrounding grus. Surfaces are pale speckled grey (plagioclase,
quartz, K-feldspar, black biotite and hornblende) under a warmer weathering rind, with
one or two onion-skin exfoliation shells spalling off the upper flanks, rusty
blotches, grey-green and chartreuse crustose lichen on top, moss in the crevices on
the shaded side and a soil line where the base sinks into the forest floor.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is treated as north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteBoulderLoaf"
DESCRIPTION = "Medium (1.3 m) rounded granite corestone boulder with exfoliation shells, lichen and moss; LOD1/LOD2."
COLLISION = "convex"
TRIANGLE_BUDGET = 100000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.15, -0.3, 0.35), "ground": "origin",
          "views": ["hero", "detail", "eye"]}
NOTES = {}


def build(kit):
    mat = kit.mats.granite("M_GraniteBoulderLoaf", grain=0.003, scale=1.2, lichen=0.55, moss=0.14,
                           iron=0.25, streaks=0.2, enclaves=0.5, soil=0.45, seed=1.0)
    field = rocks.boulder(
        radii=(0.72, 0.52, 0.46), power=2.3, lumps=0.035, lump_scale=1.1, seed=11,
        joints=[((0.2, -1.0, 0.05), 0.40, 0.14),    # front joint face, weathered round
                ((0.0, 0.0, -1.0), 0.24, 0.16),     # broad base
                ((1.0, 0.15, 0.35), 0.62, 0.16),    # sloping east shoulder
                ((-0.3, 0.2, 1.0), 0.40, 0.18),     # flattened top
                ((-1.0, 0.3, 0.2), 0.62, 0.12)])    # west end joint

    def post(points, normal):
        depth = rocks.crack(points, (0.9, 0.35, 0.0), 0.28, 0.006, 0.012, seed=4, wobble=0.02,
                            wobble_scale=3.0)
        return points - normal * depth[:, None]

    piece = rocks.solid("Loaf", field, subdivisions=7, post=post, material=mat)

    def detail(points, normal):
        # Exfoliation: the outer shell (~3 cm) has spalled off in polygonal plates, more on the
        # exposed top than near the ground. Done on the final mesh so the steps stay crisp.
        depth, fresh = rocks.plates(points, normal, seed=3, cell=0.42, thickness=0.028,
                                    coverage=0.35, width=0.02, wobble=0.3,
                                    bias=lambda c: 0.35 * np.clip(c[:, 2] / 0.35, -1, 1))
        grain = rocks.relief(points, normal, 21, [(0.35, 0.004), (0.06, 0.0014), (0.018, 0.0006)])
        return grain - depth, {"fresh": fresh}

    meshes, sink = rocks.finish(kit, [piece], "SM_GraniteBoulderLoaf", 90000, (20000, 5000),
                                ground_z=-0.14, detail=detail)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": "Origin is the ground line; place at terrain height (already sunk "
                               f"{sink * 100:.0f} cm). Add ~0.5 x slope x radius on slopes.",
                  "north": "+Y (moss and denser lichen side)"})
    return meshes
