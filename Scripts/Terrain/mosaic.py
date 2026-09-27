"""Mosaic the EA LIDAR Composite DTM 1 m tiles into one float32 grid and write previews."""
import glob, os, sys
import numpy as np
import rasterio
from PIL import Image

RAW = os.environ.get("HOMESTEAD_LIDAR", r"E:\TerrainSource\EA_LIDAR_DTM1m")
OUT = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
E0, E1, N0, N1 = 165000, 180000, 45000, 55000

def mosaic():
    grid = np.full((N1 - N0, E1 - E0), np.nan, np.float32)
    for path in glob.glob(os.path.join(RAW, "*", "*_DTM_1m.tif")):
        with rasterio.open(path) as src:
            data = src.read(1).astype(np.float32)
            if src.nodata is not None:
                data[data == src.nodata] = np.nan
            data[data < -100] = np.nan
            left, top = src.transform.c, src.transform.f
            c0, r0 = int(round(left - E0)), int(round(N1 - top))
            h, w = data.shape
            grid[r0:r0 + h, c0:c0 + w] = np.where(np.isnan(data), grid[r0:r0 + h, c0:c0 + w], data)
    return grid

def hillshade(z, cell=1.0, az=315, alt=45):
    zz = np.nan_to_num(z, nan=-5.0)
    gy, gx = np.gradient(zz, cell)
    slope = np.arctan(np.hypot(gx, gy))
    aspect = np.arctan2(-gx, gy)
    a, b = np.radians(360 - az + 90), np.radians(alt)
    s = np.sin(b) * np.cos(slope) + np.cos(b) * np.sin(slope) * np.cos(a - aspect)
    return np.clip(s, 0, 1)

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    grid = mosaic()
    np.save(os.path.join(OUT, "mosaic_E165000_N45000_1m.npy"), grid)
    print("valid", np.isfinite(grid).mean(), "min", np.nanmin(grid), "max", np.nanmax(grid))
    small = grid[::5, ::5]
    hs = hillshade(small, 5.0)
    rgb = np.stack([hs] * 3, -1)
    land = np.isfinite(small)
    elev = np.nan_to_num(small, nan=0) / max(1.0, float(np.nanmax(small)))
    rgb[..., 0] = np.where(land, 0.35 + 0.65 * hs * (0.6 + 0.4 * elev), 0.15)
    rgb[..., 1] = np.where(land, 0.45 + 0.55 * hs, 0.3)
    rgb[..., 2] = np.where(land, 0.3 + 0.5 * hs, 0.55)
    img = Image.fromarray((rgb * 255).astype(np.uint8))
    # 1 km grid lines, labelled by OS easting/northing km.
    px = img.load()
    for km in range(0, 16):
        x = km * 200
        for y in range(img.height):
            if x < img.width: px[x, y] = (255, 255, 0)
    for km in range(0, 11):
        y = km * 200
        for x in range(img.width):
            if y < img.height: px[x, y] = (255, 255, 0)
    img.save(os.path.join(OUT, "overview_5m.png"))

