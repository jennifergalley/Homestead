"""Large granite erratic: a rounded, faceted glacial boulder (~3.3 m).

Reference: the glacial erratics at Olmsted Point and around Tenaya Lake and Tuolumne
Meadows, dropped by the Tioga-age glaciers on polished granite and in the lodgepole
forest. Granodiorite, rounded by transport and weathering but still showing the broad
facets of the joint blocks it came from; a pale cream-grey weathering rind; black
crustose lichen and rock-tripe spots, grey-green and chartreuse map-lichen crusts;
a few thin exfoliation shells spalled off the crown in plates; rusty blotches; moss
and soil where it has settled into the ground.

Everything is generated here: no scanned or downloaded geometry or textures.
Crystals come from the shared tiling GraniteDetail maps (macro bake without
crystals), layered in object space at 1 tile per meter.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteErratic"
DESCRIPTION = "Large (3.3 m) rounded, faceted granite glacial erratic with spalled shells and lichen; LOD1/LOD2."
COLLISION = "convex"
TRIANGLE_BUDGET = 400000
BAKE = {"size": 4096, "samples": 64, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.6, -1.0, 1.6), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 7.5}
NOTES = {}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0,
          "strength": 0.8}

SHELLS = dict(cell=1.5, thickness=0.055, coverage=0.4, width=0.04, wobble=0.16)
FLAKES = dict(cell=0.35, thickness=0.012, coverage=0.2, width=0.012, wobble=0.22)


def build(kit):
    mat = kit.mats.granite("M_GraniteErratic", grains=False, grain=0.0035, scale=3.0, patina=0.75, lichen=0.75,
                           moss=0.15, iron=0.3, streaks=0.3, soil=0.5, soil_height=0.18, enclaves=0.6,
                           spots=2.2, seed=9.0)
    field = rocks.boulder(
        radii=(1.75, 1.4, 1.25), power=2.35, lumps=0.035, lump_scale=1.0, seed=91,
        joints=[((0.0, 0.0, -1.0), 0.95, 0.45),     # settled base
                ((0.15, -1.0, 0.25), 1.25, 0.55),   # south facet
                ((1.0, 0.1, 0.35), 1.5, 0.6),       # east shoulder
                ((-0.35, 0.3, 1.0), 1.05, 0.7),     # crown facet
                ((-1.0, -0.2, 0.1), 1.6, 0.5),      # west facet
                ((0.2, 1.0, -0.2), 1.3, 0.5)])      # north facet

    def post(points, normal):
        depth = rocks.crack(points, (0.9, -0.35, 0.1), 0.35, 0.012, 0.03, seed=92, wobble=0.06,
                            wobble_scale=1.2, taper=(2, -0.4, 0.3))
        return points - normal * depth[:, None]

    rock = rocks.solid("Erratic", field, subdivisions=8, post=post, material=mat, r_max=5.0)

    def detail(points, normal):
        up = np.clip(points[:, 2] / 0.8, -1, 1)
        depth, fresh = rocks.plates(points, normal, seed=93, bias=lambda c: 0.25 * np.clip(c[:, 2] / 0.8, -1, 1),
                                    **SHELLS)
        flake, flaked = rocks.plates(points, normal, seed=94, **FLAKES)
        grain = rocks.relief(points, normal, 95, [(0.9, 0.01), (0.2, 0.003), (0.05, 0.0012)])
        return grain - depth - flake * (0.5 + 0.5 * up), {"fresh": np.clip(np.maximum(fresh, flaked * 0.5), 0, 1)}

    meshes, sink = rocks.finish(kit, [rock], "SM_GraniteErratic", 300000, (60000, 12000), ground_z=-0.7,
                                detail=detail)
    NOTES.update({
        "sink_depth_m": round(sink, 3),
        "placement": f"Origin is the ground line; place at terrain height (already sunk {sink:.2f} m).",
        "north": "+Y (moss side)",
        "collision": "Convex is fine: the boulder is convex apart from shallow shell steps.",
        "material": ("Macro bake (no crystals) + shared GraniteDetail tiling maps: world-aligned 1 m "
                     "tiles, BaseColor = Macro * lerp(1, 2 * Detail, 0.8); normal = blend of macro and "
                     "detail normals; roughness = lerp(Macro, DetailRoughness, 0.4)."),
        "detail_textures": [f"{DETAIL['folder']}/T_GraniteDetail_{role}.png"
                            for role in ("basecolor", "normal", "roughness", "height")],
    })
    return meshes


def after_bake(kit, obj):
    kit.layer_detail(obj, kit.ROOT / DETAIL["folder"], DETAIL["stem"], tile=DETAIL["tile_m"],
                     strength=DETAIL["strength"])
