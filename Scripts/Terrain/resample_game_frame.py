"""Resample the LIDAR mosaic into the game frame: a 4033 m square at 1 m, turned so the sea lies south.

Game frame (Unreal): +X north, +Y east, centred on the origin. Real bearing = game bearing + ROTATION.
"""
import os
import numpy as np
from PIL import Image
from scipy.ndimage import map_coordinates, distance_transform_edt

WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
MOSAIC = os.path.join(WORK, "mosaic_E165000_N45000_1m.npy")
E0, N1 = 165000.0, 55000.0          # mosaic top-left (OSGB metres)
CENTER = (170300.0, 49000.0)         # OSGB point at the game origin
ROTATION = 90.0                      # degrees: game north is real east, so the sea (real west) lies south
SIZE = 4033

def game_to_osgb(gx, gy):
    """gx: metres north in game, gy: metres east in game -> OSGB E, N."""
    t = np.radians(ROTATION)
    # Game north unit vector in real (E, N): bearing t -> (sin t, cos t); game east: bearing t+90.
    ne, nn = np.sin(t), np.cos(t)
    ee, en = np.sin(t + np.pi / 2), np.cos(t + np.pi / 2)
    return CENTER[0] + gx * ne + gy * ee, CENTER[1] + gx * nn + gy * en

def sample():
    mosaic = np.load(MOSAIC)
    half = (SIZE - 1) / 2.0
    # Row r runs north->south in game (image top = game north), column c runs west->east.
    idx = np.arange(SIZE, dtype=np.float64) - half
    gy, gx = np.meshgrid(idx, -idx)
    e, n = game_to_osgb(gx, gy)
    rows, cols = N1 - n, e - E0
    valid = np.isfinite(mosaic)
    filled = np.where(valid, mosaic, -9999.0)
    z = map_coordinates(filled, [rows, cols], order=1, mode="constant", cval=-9999.0)
    ok = map_coordinates(valid.astype(np.float32), [rows, cols], order=1, mode="constant", cval=0.0) > 0.999
    z = np.where(ok, z, np.nan).astype(np.float32)
    return z

if __name__ == "__main__":
    z = sample()
    np.save(os.path.join(WORK, "game_raw_4033.npy"), z)
    print("nan", np.isnan(z).mean(), "min", np.nanmin(z), "max", np.nanmax(z))

