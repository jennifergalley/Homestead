"""Three loose, hand-sized granite stones the heroine gathers and carries.

Reference: the loose stones of Sierra Nevada creek beds and forest floors: a smooth,
water-rounded granite river cobble (A, ~12 cm), a sub-rounded weathered granodiorite
stone (B, ~16 cm) and a sub-angular spall fragment still showing its joint faces
(C, ~19 cm). Speckled granodiorite crystals are baked in (1K maps at ~0.4 mm per
texel over these small surfaces), with a little grey lichen on the older stones and
soil on the side each one lay on. They are read at arm's length in the gather
animation, so each is a single clean closed stone of ~2-3k triangles.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; pivots are at each stone's bounds centre (for hand attachment).
"""
import numpy as np

import homestead_rocks as rocks

NAME = "HandStones"
DESCRIPTION = "Three single hand-sized granite stones (12, 16, 19 cm) for gathering and carrying."
COLLISION = "convex"
TRIANGLE_BUDGET = 3200
BAKE = {"size": 1024, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"]}
NOTES = {"pivot": "Bounds centre of each stone (for socket attachment); the stones are authored lying on "
                  "their natural resting face, which is their local -Z (dirt side).",
         "collision": "Simple convex hull per stone."}

STONES = {
    "A": dict(radii=(0.061, 0.046, 0.034), power=2.05, lumps=0.05, lump_scale=1.2, seed=201, joints=[],
              material=dict(lichen=0.05, patina=0.45, iron=0.12, streaks=0.0, spots=0.2, roughness_seed=1.0),
              relief=[(0.03, 0.0006), (0.008, 0.00015)], tris=2200),
    "B": dict(radii=(0.081, 0.064, 0.047), power=2.35, lumps=0.07, lump_scale=1.3, seed=202,
              joints=[((0.2, -1.0, 0.3), 0.05, 0.02), ((1.0, 0.3, 0.1), 0.068, 0.02),
                      ((0.0, 0.0, -1.0), 0.04, 0.02)],
              material=dict(lichen=0.35, patina=0.7, iron=0.3, streaks=0.0, spots=1.0, roughness_seed=2.0),
              relief=[(0.04, 0.0012), (0.01, 0.0003)], tris=2600),
    "C": dict(radii=(0.098, 0.07, 0.05), power=2.9, lumps=0.06, lump_scale=1.4, seed=203,
              joints=[((0.1, 0.0, 1.0), 0.036, 0.008), ((0.0, 0.0, -1.0), 0.042, 0.012),
                      ((1.0, -0.4, 0.0), 0.08, 0.01), ((-0.8, -1.0, 0.2), 0.07, 0.012),
                      ((-1.0, 0.5, -0.1), 0.085, 0.015)],
              material=dict(lichen=0.2, patina=0.5, iron=0.35, streaks=0.0, spots=0.6, roughness_seed=3.0),
              relief=[(0.05, 0.0012), (0.012, 0.0004)], tris=2900),
}


def stone(kit, key, spec):
    params = spec["material"]
    mat = kit.mats.granite(f"M_HandStone_{key}", grain=0.0025, scale=0.15, patina=params["patina"],
                           lichen=params["lichen"], moss=0.0, iron=params["iron"], streaks=params["streaks"],
                           soil=0.35, soil_height=0.015, enclaves=0.25, spots=params["spots"],
                           seed=params["roughness_seed"])
    field = rocks.boulder(radii=spec["radii"], power=spec["power"], lumps=spec["lumps"],
                          lump_scale=spec["lump_scale"], seed=spec["seed"], joints=spec["joints"])
    obj = rocks.solid(f"SM_HandStone_{key}", field, subdivisions=6, material=mat, r_max=0.5)
    rocks.remesh(kit, obj, rocks.voxel_for(obj, 60000, 0))
    rocks.displace(obj, lambda p, n: rocks.relief(p, n, spec["seed"] + 7, spec["relief"]))
    points = rocks.coords(obj)
    points[:, 2] -= points[:, 2].min()       # lying on its resting face, ground at z = 0
    rocks.set_coords(obj, points)
    kit.tag_coords(obj.data)                  # material sees the resting side as the soil side
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = spec["tris"] / rocks.triangles(obj)
    mod.use_collapse_triangulate = True
    kit.apply_modifiers(obj)
    rocks.unwrap(obj, angle=66.0, margin=0.01)
    return kit.finalize(obj, pivot="center", unwrap=False, reshade=True, smooth_angle=180.0)


def build(kit):
    return [stone(kit, key, spec) for key, spec in STONES.items()]
