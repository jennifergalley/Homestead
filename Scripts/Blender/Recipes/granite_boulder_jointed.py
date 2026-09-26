"""Large jointed granite boulder (~3.6 m): a blocky corestone with rounded edges.

Reference: the loaf- and block-shaped boulders of the Yosemite Valley floor and the
big corestones along the Wawona Road: granodiorite blocked out by three joint sets,
their edges and corners rounded by spheroidal weathering while the joint faces stay
broad and flat-ish. A vertical joint crack cuts partway down through the block, a
sheet has spalled from the crown in plates, black water streaks run down the steep
north face from the rim, rust stains bleed from the crack, lichen crusts the top and
moss fills the crack and the shaded foot.

Everything is generated here: no scanned or downloaded geometry or textures.
Crystals come from the shared tiling GraniteDetail maps (macro bake without
crystals), layered in object space at 1 tile per meter.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteBoulderJointed"
DESCRIPTION = "Large (3.6 m) blocky jointed granite corestone with a joint crack, spalled plates and streaks; LOD1/LOD2."
COLLISION = "convex"
TRIANGLE_BUDGET = 400000
BAKE = {"size": 4096, "samples": 64, "repack": False, "maps": ["basecolor", "roughness", "normal", "ao", "mask"]}
BEAUTY = {"pose": (0, 0, 180), "focus": (0.55, 0.2, 1.7), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 8.0}
NOTES = {}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0,
          "strength": 0.8}
CRACK = (rocks.unit((1.0, -0.15, 0.05)), 0.35)


def build(kit):
    mat = kit.mats.granite("M_GraniteBoulderJointed", grains=False, grain=0.0035, scale=3.2, patina=0.8,
                           lichen=1.15, moss=0.2, iron=0.4, streaks=0.8, soil=0.5, soil_height=0.2,
                           enclaves=0.5, film=0.8, seed=15.0)
    field = rocks.boulder(
        radii=(1.95, 1.3, 1.2), power=3.1, lumps=0.025, lump_scale=1.0, seed=151,
        joints=[((0.0, 0.0, -1.0), 0.9, 0.35),       # buried base
                ((0.04, -0.06, 1.0), 1.05, 0.55),    # crown joint
                ((0.1, 1.0, 0.0), 1.15, 0.45),       # steep north face
                ((0.15, -1.0, 0.1), 1.2, 0.5),       # south face
                ((1.0, 0.1, 0.05), 1.75, 0.5),       # east end
                ((-1.0, 0.2, 0.0), 1.8, 0.55)])      # west end

    def post(points, normal):
        n, d = CRACK
        depth = rocks.crack(points, n, d, 0.07, 0.2, seed=152, wobble=0.05, wobble_scale=1.2,
                            taper=(2, -0.3, 0.6))
        return points - normal * depth[:, None]

    rock = rocks.solid("Jointed", field, subdivisions=8, post=post, material=mat, r_max=6.0)

    def detail(points, normal):
        n, d = CRACK
        joint = np.exp(-((points @ n - d) / 0.07) ** 2) * np.clip((points[:, 2] + 0.2) / 0.6, 0, 1)
        depth, fresh = rocks.plates(points, normal, seed=153, cell=1.4, thickness=0.06, coverage=0.35,
                                    width=0.04, wobble=0.15, bias=lambda c: 0.3 * np.clip(c[:, 2] / 0.8, -1, 1))
        flake, flaked = rocks.plates(points, normal, seed=154, cell=0.35, thickness=0.007, coverage=0.18,
                                     width=0.012, wobble=0.22)
        grain = rocks.relief(points, normal, 155, [(1.0, 0.01), (0.2, 0.003), (0.05, 0.0012)])
        return grain - depth - flake, {"fresh": np.clip(np.maximum(fresh * 0.4, flaked * 0.6), 0, 1), "joint": joint}

    meshes, sink = rocks.finish(kit, [rock], "SM_GraniteBoulderJointed", 300000, (60000, 12000), ground_z=-0.65,
                                detail=detail)
    NOTES.update({
        "sink_depth_m": round(sink, 3),
        "placement": f"Origin is the ground line; place at terrain height (already sunk {sink:.2f} m).",
        "north": "+Y (steep streaked face and moss)",
        "collision": "Convex is acceptable: the joint crack is too narrow to walk into.",
        "material": ("Macro bake (no crystals) + shared GraniteDetail tiling maps (world-aligned 1 m tiles), "
                     "gated by T_<Name>_mask (UV0; 1 = bare rock, 0 under lichen/moss/soil): k = 0.8 * Mask; "
                     "BaseColor = Macro * lerp(1, 2 * Detail, k); normal = macro blended with detail normal by k; "
                     "roughness = lerp(Macro, DetailRoughness, 0.5 * k)."),
        "detail_textures": [f"{DETAIL['folder']}/T_GraniteDetail_{role}.png"
                            for role in ("basecolor", "normal", "roughness", "height")],
    })
    return meshes


def after_bake(kit, obj):
    kit.layer_detail(obj, kit.ROOT / DETAIL["folder"], DETAIL["stem"], tile=DETAIL["tile_m"],
                     strength=DETAIL["strength"],
                     mask=kit.ROOT / "Assets" / "Props" / NAME / "Textures" / f"T_{NAME}_mask.png")
