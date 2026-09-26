"""House-sized granite dome boulder: a rounded, exfoliating outcrop piece.

Reference: the small exfoliation domes and dome-shaped boulders of the Yosemite high
country and the Wawona/Glacier Point road (and, at a smaller scale, Moro Rock in
Sequoia). Granodiorite unloads in onion-skin sheets parallel to its surface: here
two nested shells (35 and 25 cm) have spalled off the flanks in broad polygonal
plates, leaving curved steps; the older top still carries its outer shell, with
two weathering pans (gnammas) and a joint crack. A detached sheet slab leans
against the south-west flank. Steep flanks carry black water streaks and rusty
iron stains, the tops lichen, the shaded north foot moss, and the base sinks into
the forest soil. Freshly spalled surfaces stay paler with less lichen.

Everything is generated here: no scanned or downloaded geometry or textures.
Crystals come from the shared tiling GraniteDetail maps (the 4K macro bake is ~6 mm
per texel over this surface), layered in object space at 1 tile per meter.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteDome"
DESCRIPTION = ("House-sized (9 x 8 x 5 m) exfoliating granite dome boulder with spalled sheet steps, "
               "weathering pans, a leaning sheet slab, streaks and lichen; Nanite-grade LOD0 plus LOD1/LOD2.")
COLLISION = "convex"
TRIANGLE_BUDGET = 1600000
BAKE = {"size": 4096, "samples": 48, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (-2.6, -2.9, 1.6), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 15.0}
NOTES = {}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0,
          "strength": 0.8}

SHELLS = dict(cell=4.2, thickness=0.4, coverage=0.42, width=0.3, wobble=0.14)
INNER = dict(cell=2.9, thickness=0.28, coverage=0.38, width=0.22, wobble=0.14)
FLAKES = dict(cell=0.7, thickness=0.025, coverage=0.18, width=0.02, wobble=0.2)


def _flank_bias(c):
    # Sheets peel most from the steep upper flanks; the crown keeps its old shell.
    height = c[:, 2]
    return 0.25 * np.clip((height + 0.5) / 1.5, -1, 1) - 0.45 * np.clip((height - 2.4) / 0.8, 0, 1)


def _shells(points, normal):
    outer, gone = rocks.plates(points, normal, seed=31, bias=_flank_bias, **SHELLS)
    inner, gone2 = rocks.plates(points, normal, seed=32, within=gone, **INNER)
    return outer + inner, np.clip(np.maximum(gone, gone2), 0, 1)


def build(kit):
    mat = kit.mats.granite("M_GraniteDome", grains=False, grain=0.004, scale=8.0, lichen=0.8, moss=0.12,
                           iron=0.3, streaks=0.6, soil=0.5, soil_height=0.35, enclaves=0.5, film=1.0, patina=0.9, seed=4.0)
    field = rocks.boulder(
        radii=(4.7, 3.9, 3.4), power=2.6, lumps=0.04, lump_scale=0.9, seed=41, center=(0, 0, 0),
        joints=[((0.0, 0.0, -1.0), 1.9, 0.9),      # buried base
                ((0.1, 0.05, 1.0), 2.75, 1.3),     # broad crown
                ((1.0, 0.25, 0.0), 4.1, 1.1),      # east joint face
                ((-0.9, 0.35, 0.15), 4.25, 1.3),   # west joint face
                ((0.2, -1.0, 0.1), 3.6, 1.2)])     # south face

    def post(points, normal):
        depth, _ = _shells(points, normal)
        depth += rocks.pits(points, normal, [(0.7, 0.5, 0.8, 0.18), (-1.6, -0.5, 0.5, 0.1)])
        depth += rocks.crack(points, (0.95, 0.3, 0.0), 1.3, 0.035, 0.12, seed=5, wobble=0.25,
                             wobble_scale=0.4, taper=(2, 0.2, 1.2))
        return points - normal * depth[:, None]

    dome = rocks.solid("Dome", field, subdivisions=8, post=post, material=mat, r_max=12.0)

    # A spalled sheet, curved like the dome it came off, resting at a low angle on the SW foot.
    slab_rot = rocks.rotation(yaw=-45.0, pitch=0.0, roll=30.0)
    slab_field = rocks.boulder(radii=(2.3, 1.8, 0.13), power=2.3, lumps=0.04, lump_scale=0.9, seed=43,
                               center=(-4.3, -3.8, -1.0), rotate=slab_rot, bend=0.08,
                               joints=[((0.0, 0.0, 1.0), 0.1, 0.04), ((0.0, 0.0, -1.0), 0.1, 0.05),
                                       ((1.0, 0.35, 0.0), 1.8, 0.28), ((-0.6, 1.0, 0.0), 1.5, 0.3),
                                       ((-0.8, -0.9, 0.0), 1.9, 0.3)])
    slab = rocks.solid("Slab", slab_field, subdivisions=7, material=mat, r_max=4.0)

    def detail(points, normal):
        # Shells were cut before the remesh (which heals folds); here only their fresh mask.
        _, fresh = _shells(points, normal)
        flake, flaked = rocks.plates(points, normal, seed=33, **FLAKES)
        grain = rocks.relief(points, normal, 44, [(2.5, 0.02), (0.4, 0.005), (0.08, 0.002)])
        return grain - flake, {"fresh": np.clip(np.maximum(fresh, flaked * 0.6), 0, 1)}

    meshes, sink = rocks.finish(kit, [dome, slab], "SM_GraniteDome", 1200000, (160000, 32000),
                                ground_z=-1.35, detail=detail)
    NOTES.update({
        "sink_depth_m": round(sink, 3),
        "placement": f"Origin is the ground line; place at terrain height (already sunk {sink:.2f} m). "
                     "On slopes sink the downhill side further or embed it in a terrain bump.",
        "north": "+Y (moss side)",
        "collision": "Convex hull is acceptable for walking around it (the leaning slab and base fill "
                     "in, which blocks nothing a player could stand in). Use complex-as-simple with "
                     "LOD2 if players should climb the stepped shells or stand in the slab gap.",
        "nanite": "LOD0 is Nanite-grade; enable Nanite and keep LOD1/LOD2 as the non-Nanite fallback.",
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
