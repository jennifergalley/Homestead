"""No grass in the mine house: the ground data's clearance round it (Jenny, blockout review: "fix the
landscape grass showing through the room floor (and the shaft)").

The estate meadow (UHomesteadGrassField) plants blades wherever T_EstateGround's red channel (density)
says so, on the landscape's height, so without a clearance they grow up through the house's floorboards
(18 cm above the yard) and float in the shaft over the holed landscape. ``house_ground(X, Y)`` gives the
clearance on any grid of world metres; Scripts/Terrain/bake_ground.py applies it on every bake, and
``patch_ground_texture.py`` applies it to the committed texture in place.

The data texture is 2048 texels over 4032 m (about 2 m a texel, sampled bilinearly), so the clearance
reaches about a texel past the walls: a trodden, grassless apron round the house, as a working yard has.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "Scripts" / "Blender" / "Recipes"))
from mine import layout as L  # noqa: E402

GRASS_MARGIN_M = 2.2        # no blades this far past the walls (one texel and a little)
APRON_M = 5.0               # trodden soil fading out over this far past the walls


def house_ground(X, Y):
    """(keep, wear): grass density multiplier (0 in and round the house) and trodden-soil wear 0..1."""
    hx, hy, _ = L.HOUSE_WORLD
    yaw = np.radians(L.HOUSE_YAW)
    dx, dy = X - hx, Y - hy
    lx, ly = np.cos(yaw) * dx + np.sin(yaw) * dy, -np.sin(yaw) * dx + np.cos(yaw) * dy
    ox, oy = np.abs(lx) - L.OUT_X / 2, np.abs(ly) - L.OUT_Y / 2
    outside = np.hypot(np.maximum(ox, 0.0), np.maximum(oy, 0.0)) + np.minimum(np.maximum(ox, oy), 0.0)
    keep = np.clip((outside - GRASS_MARGIN_M) / 1.0, 0.0, 1.0)
    wear = 0.7 * np.clip(1.0 - outside / APRON_M, 0.0, 1.0)
    return keep, wear


def patch_exported_texture():
    """Step 2 of patch_ground_texture.py: patch the exported T_EstateGround source in place."""
    from PIL import Image
    work = Path(__file__).resolve().parents[2] / "Saved" / "Ground" / "MinePatch"
    img = np.array(Image.open(work / "T_EstateGround_source.tga").convert("RGBA"))
    n = img.shape[0]
    centres = ((np.arange(n) + 0.5) / n * 403200.0 - 201600.0) / 100.0   # texel centres, world metres
    X, Y = np.meshgrid(centres, centres)                                  # columns = X, rows = Y
    keep, wear = house_ground(X, Y)
    before = img.copy()
    img[..., 0] = np.round(img[..., 0] * keep).astype(np.uint8)
    img[..., 3] = np.maximum(img[..., 3], np.round(wear * 255)).astype(np.uint8)
    Image.fromarray(img).save(work / "T_EstateGround.png")
    print(f"mine ground: {int((img != before).any(axis=-1).sum())} texels changed")


if __name__ == "__main__":
    patch_exported_texture()
