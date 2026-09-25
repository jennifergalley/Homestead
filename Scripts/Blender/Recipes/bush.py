"""Hero shrub composed from photoscanned CC0 Shrub 02 shoots.

Each Poly Haven Shrub 02 piece is a sparse willow-like shoot; a believable bush is
many of them rooted in one clump. This recipe keeps the scanned geometry, UVs,
normals and 4K atlas unchanged and only arranges copies: golden-angle spacing
around the root, upright in the middle and leaning outward at the rim, which
gives the vase-shaped dome real shrubs grow into. The same layout is built for
each publisher LOD so distant versions match exactly.
"""
import math
import random

from mathutils import Matrix, Vector

NAME = "Bush"
DESCRIPTION = "Full woodland shrub composed from 22 photoscanned Shrub 02 shoots, with LOD1/LOD2."
COLLISION = "none"
TRIANGLE_BUDGET = 150000
PROVENANCE = ("Arrangement of CC0 Poly Haven 'Shrub 02' by Rico Cilliers "
              "(https://polyhaven.com/a/shrub_02): scanned geometry, UVs, normals and 4K maps unchanged.")

VARIANTS = ("a", "b", "c", "d")
COUNT = 22
SEED = 2207


def layout():
    rng = random.Random(SEED)
    placements = []
    for index in range(COUNT):
        t = (index + 0.5) / COUNT
        azimuth = math.radians(index * 137.508 + rng.uniform(-14, 14))
        reach = 0.03 + 0.20 * math.sqrt(t)
        tilt = math.radians(3 + 22 * t + rng.uniform(-4, 4))
        yaw = rng.uniform(0, 2 * math.pi)
        scale = rng.uniform(0.82, 1.1) * (1.08 - 0.25 * t)
        outward = Vector((-math.sin(azimuth), math.cos(azimuth), 0))
        matrix = (Matrix.Translation((reach * math.cos(azimuth), reach * math.sin(azimuth), -0.03))
                  @ Matrix.Rotation(tilt, 4, outward)
                  @ Matrix.Rotation(yaw, 4, "Z")
                  @ Matrix.Scale(scale, 4))
        placements.append((rng.choice(VARIANTS), matrix))
    return placements


def build(kit):
    templates = kit.append(kit.polyhaven("shrub_02"),
                           [f"shrub_02_{v}_LOD{lod}" for v in VARIANTS for lod in range(3)])
    placements = layout()
    meshes = []
    for lod in range(3):
        parts = [kit.instance(templates[f"shrub_02_{variant}_LOD{lod}"], f"Shoot{lod}_{index}", matrix=matrix)
                 for index, (variant, matrix) in enumerate(placements)]
        name = "SM_Bush" if lod == 0 else f"SM_Bush_LOD{lod}"
        # Pivot stays at the root clump (ground level) for every LOD.
        meshes.append(kit.join(parts, name, pivot=None, unwrap=False, reshade=False))
    return meshes
