"""The mine house's ground: no grass in or round the house, and open heath where the woods were cleared
(Jenny, blockout reviews: "fix the landscape grass showing through the room floor (and the shaft)";
"open grass and heath around it, not woods").

The estate meadow (UHomesteadGrassField) plants blades wherever T_EstateGround's red channel (density)
says so, on the landscape's height, so without a clearance they grow up through the house's floorboards
and float in the shaft over the holed landscape. ``house_ground(X, Y)`` gives the clearance on any grid of
world metres; Scripts/Terrain/bake_ground.py applies it on every bake.

``patch_exported()`` (step 2 of patch_ground_texture.py) applies the same result to the committed ground
data in place, without a full re-bake: the clearance round the house, and, within ``PATCH_M`` of it, the
change Scripts/Mine/clear_mine_yard.py made to the trees' canopy (bake_ground.canopy_mask before and after:
grass comes back where the shade went, the leaf litter goes, and woodland cells turn to grass).

The data texture is 2048 texels over 4032 m (about 2 m a texel, sampled bilinearly), so the clearance
reaches about a texel past the walls: a trodden, grassless apron round the house, as a working yard has.
"""
import struct
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
from mine import layout as L  # noqa: E402

GRASS_MARGIN_M = -1.5       # full meadow density right up to the walls (Jenny/Orchestrator, review 7: "drop the bare
                            # apron"); UHomesteadGrassField clips the blades at the house footprint itself (Mine
                            # Gameplay's bare squares), so the 2 m density texels can straddle the walls
APRON_M = 0.0               # Jenny: "just give it grass around it" - no trodden apron
YARD_GRASS_M = (44.0, 58.0) # meadow grass right up to the house within the first, blended out by the second
MEADOW_RING_M = (60.0, 80.0)  # the surrounding meadow's density and height are read from this ring
# Jenny: "fix the brown dirt lines around the mine building ... make it green": the canopy texture's stony-bank (G)
# and steep-granite (A) masks were baked from the old cut banks; they're cleared over the same plain.
SURF_KEEP = {7}            # EstateGround.bin surfaces left alone on the plain (water); the rest become grass
PATCH_M = 160.0             # the canopy change is applied this far round the house
BEFORE_COMMIT = "27a0d0c2e" # the scenery the committed ground data was baked from (main before the mine lane).
                            # The canopy part multiplies: WORK/EstateScenery_before.bin, when present, is the scenery the
                            # last patch accounted for (copy EstateScenery.bin there before clearing more trees).
WORK = ROOT / "Saved" / "Ground" / "MinePatch"
GROUND_BIN = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateGround.bin"
SURF_GRASS, SURF_WOODLAND = 1, 5     # bake_ground.SURFACES
H, SIZE = 2016, 4033


def house_ground(X, Y):
    """(keep, wear): grass density multiplier (0 in and round the house) and trodden-soil wear 0..1."""
    hx, hy, _ = L.HOUSE_WORLD
    yaw = np.radians(L.HOUSE_YAW)
    dx, dy = X - hx, Y - hy
    lx, ly = np.cos(yaw) * dx + np.sin(yaw) * dy, -np.sin(yaw) * dx + np.cos(yaw) * dy
    ox, oy = np.abs(lx) - L.OUT_X / 2, np.abs(ly) - L.OUT_Y / 2
    outside = np.hypot(np.maximum(ox, 0.0), np.maximum(oy, 0.0)) + np.minimum(np.maximum(ox, oy), 0.0)
    keep = np.clip((outside - GRASS_MARGIN_M) / 1.0, 0.0, 1.0)
    wear = np.zeros_like(outside) if APRON_M <= 0 else 0.7 * np.clip(1.0 - outside / APRON_M, 0.0, 1.0)
    return keep, wear


def canopies():
    """Canopy cover on the 1 m grid before (the scatter's own trees) and after the mine's clearing."""
    sys.path.insert(0, str(ROOT / "Scripts" / "Terrain"))
    import bake_ground
    before = WORK / "EstateScenery_before.bin"
    if not before.exists():
        blob = subprocess.run(["git", "show", f"{BEFORE_COMMIT}:Content/SurvivalGame/Estate/Runtime/EstateScenery.bin"],
                              cwd=ROOT, capture_output=True, check=True).stdout
        if blob.startswith(b"version https://git-lfs"):
            blob = subprocess.run(["git", "lfs", "smudge"], input=blob, cwd=ROOT, capture_output=True, check=True).stdout
        before.write_bytes(blob)
    return bake_ground.canopy_mask((SIZE, SIZE), str(before)), bake_ground.canopy_mask((SIZE, SIZE))


def patch_exported():
    from PIL import Image
    from scipy.ndimage import map_coordinates
    old_c, new_c = canopies()
    hx, hy, _ = L.HOUSE_WORLD
    # Texture texels (2048 over the map; columns = X, rows = Y).
    ground = np.array(Image.open(WORK / "T_EstateGround_source.tga").convert("RGBA")).astype(np.float64) / 255
    canopy = np.array(Image.open(WORK / "T_EstateCanopy_source.tga").convert("RGBA")).astype(np.float64) / 255
    n = ground.shape[0]
    centres = (np.arange(n) + 0.5) / n * (2 * H)            # grid index (metres + H)
    yy, xx = np.meshgrid(centres, centres, indexing="ij")
    X, Y = xx - H, yy - H
    near = np.hypot(X - hx, Y - hy) < PATCH_M
    oc = map_coordinates(old_c, [yy, xx], order=1, mode="nearest")
    nc = map_coordinates(new_c, [yy, xx], order=1, mode="nearest")
    gain_d = np.where(near, (1 - 0.8 * nc) / np.maximum(1 - 0.8 * oc, 0.2), 1.0)
    gain_h = np.where(near, (1 - 0.3 * nc) / np.maximum(1 - 0.3 * oc, 0.7), 1.0)
    keep, wear = house_ground(X, Y)
    g0, c0 = ground.copy(), canopy.copy()
    ground[..., 0] = np.clip(ground[..., 0] * gain_d, 0, 1)
    ground[..., 1] = np.clip(ground[..., 1] * gain_h, 0, 1)
    # The yard: the meadow right up to the house, no worn working-site ring (bake_ground used to wear 20 m
    # round MineEntrance, and the first patch added an apron).
    dist = np.hypot(X - hx, Y - hy)
    ring = (dist > MEADOW_RING_M[0]) & (dist < MEADOW_RING_M[1])
    yard = 1.0 - np.clip((dist - YARD_GRASS_M[0]) / (YARD_GRASS_M[1] - YARD_GRASS_M[0]), 0.0, 1.0)
    for ch in (0, 1):
        meadow = float(np.median(ground[..., ch][ring]))
        ground[..., ch] = ground[..., ch] + (np.maximum(ground[..., ch], meadow) - ground[..., ch]) * yard
    ground[..., 3] = ground[..., 3] * (1.0 - yard)
    ground[..., 0] *= keep
    ground[..., 3] = np.maximum(ground[..., 3], wear)
    canopy[..., 0] = np.where(near, nc, canopy[..., 0])
    for ch in (1, 3):
        canopy[..., ch] = canopy[..., ch] * (1.0 - yard)
    for name, img, was in (("T_EstateGround", ground, g0), ("T_EstateCanopy", canopy, c0)):
        Image.fromarray(np.round(img * 255).astype(np.uint8), "RGBA").save(WORK / f"{name}.png")
        print(f"mine ground: {name} {int((np.abs(img - was) > 0.5 / 255).any(axis=-1).sum())} texels changed")
    # EstateGround.bin cells (1024; cell centres on the grid, rows = Y).
    raw = GROUND_BIN.read_bytes()
    cells = struct.unpack_from("<H", raw, 4)[0]
    data = np.frombuffer(raw, np.uint8, offset=6).reshape(cells, cells, 2).copy()
    step = (SIZE - 1) / cells
    c = (np.arange(cells) + 0.5) * step
    cy, cx = np.meshgrid(c, c, indexing="ij")
    CX, CY = cx - H, cy - H
    cnear = np.hypot(CX - hx, CY - hy) < PATCH_M
    oc = map_coordinates(old_c, [cy, cx], order=0, mode="nearest")
    nc = map_coordinates(new_c, [cy, cx], order=0, mode="nearest")
    keep, _ = house_ground(CX, CY)
    dens = data[..., 0].astype(np.float64) / 255
    dens = np.where(cnear, np.clip(dens * (1 - 0.8 * nc) / np.maximum(1 - 0.8 * oc, 0.2), 0, 1), dens)
    cdist = np.hypot(CX - hx, CY - hy)
    cyard = 1.0 - np.clip((cdist - YARD_GRASS_M[0]) / (YARD_GRASS_M[1] - YARD_GRASS_M[0]), 0.0, 1.0)
    cring = (cdist > MEADOW_RING_M[0]) & (cdist < MEADOW_RING_M[1])
    dens = dens + (np.maximum(dens, float(np.median(dens[cring]))) - dens) * cyard
    dens = dens * np.where(cdist < 12, keep, 1.0)
    before = data.copy()
    data[..., 0] = np.round(dens * 255).astype(np.uint8)
    opened = cnear & (oc > 0.5) & (nc <= 0.5) & (data[..., 1] == SURF_WOODLAND)
    opened |= (cyard > 0.5) & ~np.isin(data[..., 1], list(SURF_KEEP)) & (data[..., 1] != SURF_GRASS)
    data[..., 1][opened] = SURF_GRASS
    GROUND_BIN.write_bytes(raw[:6] + data.tobytes())
    print(f"mine ground: EstateGround.bin {int((data != before).any(axis=-1).sum())} cells changed, "
          f"{int(opened.sum())} cells now grass")


if __name__ == "__main__":
    patch_exported()
