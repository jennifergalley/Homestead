"""House-sized split granite boulder with a sheeting slab.

Reference: the giant split and jointed boulders of Yosemite Valley's floor and the
Tioga Road (Camp Four's Columbia Boulder, the split erratics around Olmsted Point
and Tenaya Lake). A blocky corestone of granodiorite, rounded at its edges, has
split along a near-vertical joint: the crack is ~0.6 m wide at the top and closes
where the halves still touch low down, and the east half has settled and tilted a
few degrees. The split faces are old joint surfaces with a rusty iron-oxide film
and fewer lichens. On the east face an exfoliation sheet (~25 cm) has peeled away
and leans against its parent, touching at the top. The weathered skin carries
spalled shell steps, black water streaks down the steep sides, lichen on top, moss
on the shaded north foot and soil where it sinks into the ground.

Everything is generated here: no scanned or downloaded geometry or textures.
Crystals come from the shared tiling GraniteDetail maps (macro bake without
crystals), layered in object space at 1 tile per meter.
Units are meters; +Y is north (moss side), the ground line is at z = 0.
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GraniteSplitBoulder"
DESCRIPTION = ("House-sized (8 x 6.5 x 4 m) jointed granite boulder split in two, with a leaning exfoliation "
               "slab, rusty joint faces, streaks and lichen; Nanite-grade LOD0 plus LOD1/LOD2.")
COLLISION = "convex"
TRIANGLE_BUDGET = 1600000
BAKE = {"size": 4096, "samples": 48, "repack": False, "maps": ["basecolor", "roughness", "normal", "ao", "mask"]}
BEAUTY = {"pose": (0, 0, -90), "focus": (0.0, -2.6, 3.0), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 13.0}
NOTES = {}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0,
          "strength": 0.8}

# Split planes: half A keeps nA.P <= dA, half B keeps nB.P >= dB. They lean apart upward
# (0.6 m at the crown) and cross ~1.4 m below the crown, so the halves stay joined low down.
N_A = rocks.unit((1.0, 0.18, 0.12))
N_B = rocks.unit((1.0, 0.18, -0.12))
D_A, D_B = -0.1, 0.12
SHELLS = dict(cell=3.6, thickness=0.3, coverage=0.4, width=0.25, wobble=0.14)
FLAKES = dict(cell=0.6, thickness=0.007, coverage=0.18, width=0.02, wobble=0.2)


def _joint_mask(points, normal):
    """1 on the two split faces (close to their planes and facing across the crack)."""
    near_a = np.exp(-(np.maximum(points @ N_A - D_A, 0) / 0.18) ** 2) * np.clip(normal @ N_A, 0, 1)
    near_b = np.exp(-(np.maximum(D_B - points @ N_B, 0) / 0.18) ** 2) * np.clip(-(normal @ N_B), 0, 1)
    return np.clip(np.maximum(near_a, near_b) * 1.4, 0, 1)


def build(kit):
    mat = kit.mats.granite("M_GraniteSplitBoulder", grains=False, grain=0.004, scale=7.0, lichen=1.15,
                           moss=0.14, iron=0.35, streaks=0.75, soil=0.5, soil_height=0.3, enclaves=0.5,
                           seed=7.0)
    block = rocks.boulder(
        radii=(4.2, 3.3, 2.7), power=2.9, lumps=0.025, lump_scale=0.9, seed=71,
        joints=[((0.0, 0.0, -1.0), 1.75, 0.7),      # buried base
                ((0.05, -0.1, 1.0), 2.35, 1.2),     # crown
                ((1.0, 0.1, 0.0), 3.7, 0.9),        # east joint face (sheet peels here)
                ((-1.0, 0.2, 0.1), 3.9, 1.1),       # west face
                ((0.1, -1.0, 0.05), 2.9, 1.0),      # south face
                ((-0.1, 1.0, 0.0), 3.0, 1.2)])      # north face
    half_a = rocks.cut(block, N_A, D_A, rounding=0.12, wobble=0.05, wobble_scale=0.7, seed=1)
    half_b = rocks.cut(block, -N_B, -D_B, rounding=0.12, wobble=0.05, wobble_scale=0.7, seed=2)

    def post(points, normal):
        joint = _joint_mask(points, normal)
        depth, _ = rocks.plates(points, normal, seed=74, **SHELLS)
        depth = depth * (1.0 - joint)
        depth += rocks.pits(points, normal, [(-1.9, 0.4, 0.55, 0.2)])
        return points - normal * depth[:, None]

    west = rocks.solid("West", half_a, center=(-2.0, 0.0, 0.0), subdivisions=8, post=post, material=mat,
                       r_max=10.0)
    east = rocks.solid("East", half_b, center=(2.0, 0.0, 0.0), subdivisions=8, post=post, material=mat,
                       r_max=10.0)
    # The east half settled outward: tilt it 2.5 degrees about the crack's foot.
    tilt = rocks.rotation(pitch=2.5)
    pivot = np.array([0.1, 0.0, -1.2])
    rocks.transform(east, tilt, pivot - tilt @ pivot)

    # Exfoliation sheet leaning on the east face, curved like the face it came off.
    slab_rot = rocks.rotation(yaw=96.0, roll=74.0)
    slab = rocks.boulder(radii=(1.8, 1.5, 0.14), power=3.0, lumps=0.05, lump_scale=1.1, seed=73,
                         center=(4.25, 0.35, 0.15), rotate=slab_rot, bend=0.07,
                         joints=[((0.0, 0.0, 1.0), 0.1, 0.03), ((0.0, 0.0, -1.0), 0.11, 0.04),
                                 ((1.0, 0.4, 0.0), 1.35, 0.06), ((-0.5, -1.0, 0.0), 1.3, 0.07),
                                 ((-1.0, 0.3, 0.0), 1.55, 0.06), ((0.6, -0.9, 0.0), 1.45, 0.06),
                                 ((-0.3, 1.0, 0.0), 1.3, 0.07)])
    slab = rocks.solid("Slab", slab, center=(4.25, 0.35, 0.15), subdivisions=7, material=mat, r_max=4.0)
    rocks.transform(slab, np.eye(3), (0.25, 0.0, 0.0))

    def detail(points, normal):
        joint = _joint_mask(points, normal)
        _, fresh = rocks.plates(points, normal, seed=74, **SHELLS)
        flake, flaked = rocks.plates(points, normal, seed=75, **FLAKES)
        grain = rocks.relief(points, normal, 76, [(2.2, 0.02), (0.4, 0.005), (0.08, 0.002)])
        fresh = np.clip(np.maximum(fresh * 0.4 * (1 - joint), flaked * 0.6), 0, 1)
        return grain - flake, {"fresh": fresh, "joint": joint}

    meshes, sink = rocks.finish(kit, [west, east, slab], "SM_GraniteSplitBoulder", 1200000, (160000, 32000),
                                ground_z=-1.2, detail=detail)
    NOTES.update({
        "sink_depth_m": round(sink, 3),
        "placement": f"Origin is the ground line; place at terrain height (already sunk {sink:.2f} m).",
        "north": "+Y (moss side)",
        "collision": ("Needs complex collision: a convex hull would fill the walk-in split and the gap "
                      "behind the slab and put invisible walls around the overhangs. Use "
                      "complex-as-simple with SM_GraniteSplitBoulder_LOD2 (32k tris), or author 3 convex "
                      "hulls (west half, east half, slab). The recipe exports 'convex' only because the "
                      "importer supports none/box/convex."),
        "nanite": "LOD0 is Nanite-grade; enable Nanite and keep LOD1/LOD2 as the non-Nanite fallback.",
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
