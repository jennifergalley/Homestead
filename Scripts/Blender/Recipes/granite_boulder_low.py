"""Medium low granite boulder (~1.5 m wide, 0.5 m tall), half sunk into the forest floor.

Reference: the low, flat-topped granite boulders that barely break the duff of the
Sierra mixed-conifer forest (Mariposa Grove, the Merced and Tuolumne groves): the
rounded top of a corestone emerging from the grus it weathered out of. Its broad top
is grey with a heavy lichen skin (grey-green crusts, chartreuse map lichen and black
spots) and a small weathering pan, one or two exfoliation shells have spalled off
the sunnier flank, and moss creeps up the shaded north side from the soil.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteBoulderLow"
DESCRIPTION = "Medium (1.5 m) low, flat-topped granite boulder half-sunk in the forest floor, lichen and moss; LOD1/LOD2."
COLLISION = "convex"
TRIANGLE_BUDGET = 100000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.2, -0.2, 0.42), "ground": "origin",
          "views": ["hero", "detail", "eye"]}
NOTES = {}


def build(kit):
    mat = kit.mats.granite("M_GraniteBoulderLow", grain=0.003, scale=1.4, patina=0.8, lichen=0.9, moss=0.3,
                           iron=0.2, streaks=0.15, soil=0.6, soil_height=0.1, enclaves=0.4, film=0.8,
                           spots=1.6, seed=13.0)
    field = rocks.boulder(radii=(0.82, 0.62, 0.5), power=2.4, lumps=0.03, lump_scale=1.1, seed=131,
                          joints=[((0.05, -0.1, 1.0), 0.26, 0.2),     # flat top
                                  ((0.0, 0.0, -1.0), 0.32, 0.1),
                                  ((1.0, -0.2, 0.2), 0.7, 0.16),
                                  ((-0.9, -0.3, 0.1), 0.72, 0.18)])

    def post(points, normal):
        depth = rocks.pits(points, normal, [(0.15, 0.05, 0.16, 0.05)])
        return points - normal * depth[:, None]

    rock = rocks.solid("Low", field, subdivisions=7, post=post, material=mat, r_max=2.5)

    def detail(points, normal):
        sunny = np.clip(normal @ rocks.unit((0.2, -1.0, 0.3)), 0, 1)
        depth, fresh = rocks.plates(points, normal, seed=132, cell=0.45, thickness=0.024, coverage=0.25,
                                    width=0.016, wobble=0.25, bias=lambda c: 0.35 * np.clip(-c[:, 1] / 0.5, -1, 1))
        grain = rocks.relief(points, normal, 133, [(0.4, 0.004), (0.06, 0.0014), (0.018, 0.0006)])
        return grain - depth * (0.4 + 0.6 * sunny), {"fresh": fresh * (0.4 + 0.6 * sunny)}

    meshes, sink = rocks.finish(kit, [rock], "SM_GraniteBoulderLow", 90000, (20000, 5000), ground_z=-0.12,
                                detail=detail)
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the ground line; place at terrain height (already sunk {sink * 100:.0f} cm). "
                               "Players can step onto its low top.",
                  "collision": "Convex hull.",
                  "north": "+Y (moss side)"})
    return meshes
