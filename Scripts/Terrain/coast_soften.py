"""Soften where the cove's west cliff ends against the grassy hillside (map stage 3 review, 2026-10-10): the cliff's
end wall stood square to the slope, leaving a flat-floored notch at the foot. Blends that corner over a few metres.

Usage: python Scripts/Terrain/coast_soften.py      (once, "coastSoften" in the layout; after coast_mix.py and
       cove_route.py, then weightmaps.py, cove_heath.py, bake_ground.py, bake_estate_map.py)

It also strews granite ledges and boulders over the playable coast's steep faces (scenery tagged CL1, replaced on
every run, so a rerun after the grade only re-emits them).
"""
import json
import os

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016

CORNER = (-462.0, -640.0)     # x, y (m): where the west cliff's end wall meets the hillside
RADIUS_M = 26.0               # blended inside this, fading over its outer half
SIGMA_M = 5.0
SAND_TOP = 1.7                # the berm stays a berm

# Granite breaking through the playable coast's steep faces (Balance, cohesion.md section 4: the cove cliff read as
# bare earth): scatter.py's LEDGE (a talus block sunk into the slope) and BOULDER kinds, tagged CL1 in EstateScenery.bin
# and replaced on every run.
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
RECORD = np.dtype([("k", "u1"), ("pad", "S3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
TAG = b"CL1"
LEDGE, BOULDER = 5, 6
CLIFF_SLOPE_DEG = 42.0
LEDGE_EVERY_M2 = 160.0
MIN_Z_M = 2.5
LEDGE_SCALE = (2.6, 4.2)
SEED = 1853


def cliff_ledges(z, layout):
    (x0, y0), (x1, y1) = np.min(layout["polygons"]["PlayableBounds"], 0), np.max(layout["polygons"]["PlayableBounds"], 0)
    sl = (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1))
    zz = z[sl]
    gy, gx = np.gradient(gaussian_filter(zz, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    yy, xx = np.mgrid[int(y0):int(y1) + 1, int(x0):int(x1) + 1]
    ok = (slope > CLIFF_SLOPE_DEG) & (zz > MIN_Z_M) & (zz < 70.0)
    rng = np.random.default_rng(SEED)
    cand = np.c_[xx[ok], yy[ok]].astype(np.float64)
    n = int(len(cand) / LEDGE_EVERY_M2)
    pts = cand[rng.choice(len(cand), size=n, replace=False)] + rng.uniform(-0.5, 0.5, (n, 2)) if n else cand[:0]
    kinds = np.where(rng.random(n) < 0.75, LEDGE, BOULDER)
    scales = rng.uniform(*LEDGE_SCALE, n)
    return [(int(k), TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(0, 360), s) for k, p, s in zip(kinds, pts, scales)]


def write_ledges(z, layout):
    recs = cliff_ledges(z, layout)
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * RECORD.itemsize], RECORD)
    kept = rec[rec["pad"] != TAG]
    out = np.concatenate([kept, np.array(recs, RECORD)]) if recs else kept
    with open(SCENERY, "wb") as fh:
        fh.write(raw[:4] + np.uint32(len(out)).tobytes() + out.tobytes() + raw[8 + count * RECORD.itemsize:])
    print(f"coast soften: {len(recs)} granite ledges on the coast's cliff faces (replaced {count - len(kept)})")


def main():
    layout = json.load(open(LAYOUT))
    if layout.get("coastSoften"):
        print("coast soften: already done; ledges re-emitted")
        write_ledges((np.fromfile(R16, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0, layout)
        return
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0
    cx, cy = CORNER
    pad = int(RADIUS_M + 4 * SIGMA_M)
    r0, r1, c0, c1 = int(cy) + H - pad, int(cy) + H + pad, int(cx) + H - pad, int(cx) + H + pad
    win = z[r0:r1 + 1, c0:c1 + 1]
    yy, xx = np.mgrid[r0 - H:r1 - H + 1, c0 - H:c1 - H + 1]
    d = np.hypot(xx - cx, yy - cy)
    t = np.clip((RADIUS_M - d) / (RADIUS_M * 0.5), 0.0, 1.0)
    w = t * t * (3 - 2 * t)
    smooth = gaussian_filter(win, SIGMA_M)
    new = win + (smooth - win) * w
    new = np.where(win <= SAND_TOP + 0.05, np.minimum(new, win), np.maximum(new, SAND_TOP))
    z[r0:r1 + 1, c0:c1 + 1] = new
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    moved = np.argwhere(u16 != raw)
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy) and len(moved):
        zr = (u16.astype(np.float64) - 32768.0) / 128.0
        zn = np.load(npy)
        rr, cc = moved[:, 0], moved[:, 1]
        zn[2 * H - cc, rr] = zr[rr, cc].astype(zn.dtype)
        np.save(npy, zn)
    layout["coastSoften"] = True
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    (a, b), (c, e) = moved.min(0), moved.max(0)
    print(f"coast soften: {len(moved)} vertices; ApplyEstateHeightfield rows {a}-{c}, cols {b}-{e}")
    write_ledges(z, layout)


if __name__ == "__main__":
    main()
