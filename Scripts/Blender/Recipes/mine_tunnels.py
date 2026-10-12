"""The mine's shallow level: hand-cut and blasted granite galleries, drifts, the ladder chamber and its
shaft, and the sump, as one Nanite rock shell seen from inside (openspec open-estate-mine-shallow-level).

    .\\Scripts\\Blender\\New-Prop.ps1 mine_tunnels -BeautySamples 64

Real-object research: 19th-century metal-mine levels in granite country (Cornwall, Devon, the Welsh
and Shropshire orefields) were driven by hand-drilled shot holes and black powder. The walls are
angular, not smooth: blocky facets where the rock broke along its joints, 20-80 cm across, with sharp
re-entrant edges, here and there the half-barrel of a drill hole, and loose spalls lodged in the back
(roof). Haulage levels were trimmed straighter with flat floors of trodden rubble and mud, puddled
in the low spots, with the water channel along one side. The rock is grey granite, darker and glossy
where seepage runs, iron-stained orange-brown along wet joints and round old timber, with white
efflorescence crusts and, near copper lodes, green and blue staining. Only under the shaft does a
little daylight reach, and green algae film the rock there.

Frame: the tunnel frame of ``mine/layout.py`` (origin on the floor under the trapdoor, Unreal axes;
Blender Y is negated on export). Place at ``layout.tunnel_world()`` with yaw 0 (Scripts/Mine/place_mine.py).
Everything is generated here: no scanned or downloaded geometry or textures (crystals come from the
shared GraniteDetail tiling maps, as the sea cave's).
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_rocks as rocks  # noqa: E402
import homestead_sdf as sdf  # noqa: E402
from mine import layout as L  # noqa: E402
from mine import tunnels  # noqa: E402

NAME = "MineTunnels"
DESCRIPTION = ("The mine's shallow level: blasted granite galleries, side drifts, ladder chamber, shaft and sump "
               "as one Nanite shell seen from inside; LOD1/LOD2 fallback, LOD2 collision.")
COLLISION = "none"          # import_props.COLLISION_OVERRIDES: complex-as-simple on LOD2
TRIANGLE_BUDGET = 2600000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 4096, "samples": 16, "repack": False, "maps": ["basecolor", "roughness", "normal", "ao", "mask"]}
BAKE_MESHES = {"SM_MineTunnels": True, "*": False}
BEAUTY = {"pose": (0, 0, 0), "focus": (8.0, 0.0, 1.5), "ground": "origin", "views": ["hero"], "eye_distance": 90.0}
REPORT = {"nanite": True, "pivot": "Tunnel frame origin: the floor under the trapdoor's centre; place at "
          "layout.tunnel_world(), yaw 0."}
NOTES = {}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0, "strength": 0.75}

VOXEL = 0.11                 # SDF meshing (m); LOD0 is this subdivided once
FACET_CELL = 0.75            # blast-fracture facets (m)
FACET_DEPTH = 0.16           # how far the facets break back into the rock (m)
FLOOR_KEEP = 0.25            # below this height the walls stay true (a clean floor edge to walk)
LOD_TRIS = (360000, 110000)


def facets(p):
    """Blocky blast fracture: each Voronoi cell of the wall breaks back to its own plane, so the rock
    reads as joint-bounded facets with sharp re-entrant edges rather than a smooth bore."""
    warp = np.stack([rocks.Noise(70 + k).fbm(p / 0.6, 3) for k in range(3)], axis=1) * 0.18
    f1, f2, r1, r2, c1, c2 = rocks.voronoi(p + warp, FACET_CELL, 71)
    # Each cell's own plane: a random tilt of the local wall, offset by the cell's random value.
    depth = FACET_DEPTH * (0.25 + 0.75 * r1) + 0.05 * (np.sum((p - c1) * (np.c_[r1, r2, r1 * r2] - 0.5), axis=1))
    edge = rocks.smoothstep(0.0, 0.06, (f2 - f1) * 0.5)          # a narrow broken step between cells
    depth = depth * (0.55 + 0.45 * edge)
    up = rocks.smoothstep(0.02, FLOOR_KEEP, p[:, 2])
    flat, _ = tunnels.node_flat_weight(p)            # dressed flat behind the nodes (no facets there)
    return np.clip(depth, 0.0, FACET_DEPTH * 1.2) * up * (1.0 - flat)


def air():
    return sdf.displaced(tunnels.air(), facets, FACET_DEPTH * 1.2)


def attributes(p, n):
    """Material masks (0..1): ``sand`` the trodden rubble floor, ``wet`` seepage and puddles,
    ``algae`` the green film under the shaft, ``fresh`` paler new fracture on the walls,
    ``crust`` unused here (the sea cave's coralline)."""
    floor = rocks.smoothstep(0.7, 0.92, n[:, 2]) * rocks.smoothstep(0.35, 0.05, p[:, 2])
    noise = sdf.fbm(p / 0.9, octaves=3, seed=81)
    seep = rocks.smoothstep(0.15, 0.5, sdf.fbm(p * np.array([1.6, 1.6, 0.1]), octaves=3, seed=82))
    low = rocks.smoothstep(0.6, 0.0, p[:, 2])
    puddle = floor * rocks.smoothstep(0.62, 0.72, sdf.fbm(p / 1.8, octaves=3, seed=83) * 0.5 + 0.5)
    sump = np.exp(-(((p[:, 0] - L.SUMP["centre"][0]) / 2.2) ** 2 + ((p[:, 1] - L.SUMP["centre"][1]) / 2.0) ** 2))
    wet = np.clip(0.2 + 0.35 * seep + 0.3 * low + 0.6 * puddle + 0.6 * sump, 0.0, 1.0)
    shaft = np.hypot(p[:, 0] - L.SHAFT["centre"][0], p[:, 1] - L.SHAFT["centre"][1])
    algae = np.clip(rocks.smoothstep(3.5, 0.8, shaft) * rocks.smoothstep(1.5, 4.5, p[:, 2])
                    * rocks.smoothstep(0.4, 0.6, noise * 0.5 + 0.5), 0.0, 1.0)
    fresh = np.clip((1.0 - floor) * rocks.smoothstep(0.5, 0.7, sdf.fbm(p / 0.5, octaves=3, seed=84) * 0.5 + 0.5)
                    * 0.7, 0.0, 1.0)
    return {"sand": np.clip(floor * (1.0 - 0.7 * puddle), 0, 1), "wet": wet, "algae": algae, "fresh": fresh,
            "crust": np.zeros(len(p))}


def build(kit):
    mat = kit.mats.granite("M_MineTunnels", grains=False, grain=0.004, scale=4.0, patina=0.35, lichen=0.0, moss=0.0,
                           iron=0.55, streaks=0.8, soil=0.0, enclaves=0.5, film=0.6, seed=23.0, cave=True)
    obj = sdf.mesh("SM_MineTunnels", air(), voxel=VOXEL, material=mat)
    # Authored in the Unreal frame, stored in Blender's (Y negated). The mirror also reverses the winding,
    # which turns the air SDF's normals (out of the air) round to face into it: the level is seen from inside.
    p = rocks.coords(obj)
    p[:, 1] *= -1.0
    rocks.set_coords(obj, p)
    obj.data.update()
    print(f"mine tunnels: shell {rocks.triangles(obj)} tris before subdivision")
    rest = rocks.coords(obj)
    rocks.set_coords(obj, rocks._smooth_coords(obj, iterations=60, factor=0.7))
    rocks.unwrap(obj, angle=60.0, method="ANGLE_BASED")
    rocks.set_coords(obj, rest)
    kit.subdivide(obj, levels=1)

    def detail(points, normal):
        q = points * np.array([1.0, -1.0, 1.0])           # back to the Unreal frame for the masks
        m = normal * np.array([1.0, -1.0, 1.0])
        attrs = attributes(q, m)
        grain = rocks.relief(q, m, 91, [(0.35, 0.022), (0.09, 0.007), (0.03, 0.002)])
        grain *= 1.0 - 0.8 * attrs["sand"]                 # the trodden floor is smoother
        grain += 0.006 * attrs["sand"] * sdf.fbm(q / 0.05, octaves=2, seed=92)   # gravel
        return grain, attrs
    rocks.displace(obj, detail)
    kit.tag_coords(obj.data)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
    meshes = [obj]
    total = rocks.triangles(obj)
    for index, target in enumerate(LOD_TRIS, start=1):
        meshes.append(kit.finalize(rocks.lod(kit, obj, f"SM_MineTunnels_LOD{index}", min(1.0, target / total)),
                                   pivot=None, unwrap=False, reshade=True, smooth_angle=180.0))
    print(f"mine tunnels: LOD0 {total} tris")
    NOTES.update({
        "frame": "Tunnel frame (layout.tunnel_world()); Unreal axes after import.",
        "collision": "Complex-as-simple on LOD2: the floor is flat below FLOOR_KEEP (no facets), so she walks it.",
        "material": "Macro bake + GraniteDetail tiling layer (as the sea cave).",
        "detail_textures": [f"{DETAIL['folder']}/T_GraniteDetail_{role}.png"
                            for role in ("basecolor", "normal", "roughness", "height")],
    })
    return meshes


def after_bake(kit, obj):
    kit.layer_detail(obj, kit.ROOT / DETAIL["folder"], DETAIL["stem"], tile=DETAIL["tile_m"],
                     strength=DETAIL["strength"],
                     mask=kit.ROOT / "Assets" / "Props" / NAME / "Textures" / f"T_{NAME}_mask.png")
