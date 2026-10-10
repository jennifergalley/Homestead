"""Bring the cove closer: carve its valley north-west into a small bay whose sand starts about 200 m from the
manor and runs round the bay onto the long beach belt (shrink-estate-map, design decision 1).

Usage: python Scripts/Terrain/cove_bay.py [--dry]     (then cove_route.py with its new CONTROL)

Run once ("coveBay.graded" in the layout). It:
1. Takes the first cove route's cut out of the heightfield (the route moves): the difference that route made
   (its commits' heightfields, read from git) is subtracted, so later edits in its corridor are kept, and
   "coveRoute" is removed from the layout so cove_route.py designs the new route over the restored ground.
2. Carves the bay: seabed inside BAY, a dry sand fringe FRINGE_M wide round it (berm TOP at the cliff foot down
   to the swash line SWASH at the water), then steep walls rising from the berm to the old ground. The bay only
   ever lowers ground (raising the old beach to the fringe's berm left a scarp at its water's edge). It keeps clear of
   the river's channel and its run to the sea, which stay as river_channel.py graded them.

Writes EstateHeightfield.r16 (the source of truth), Estate_Heightmap_4033.png and the work npy where they
changed, and "coveBay" in estate_layout.json; prints the ApplyEstateHeightfield rectangle. --dry writes a
preview to Saved/CoveBay/ only. Afterwards: cove_route.py, river_channel.py, weightmaps.py, bake_ocean.py,
bake_ground.py, bake_estate_map.py; in the editor ApplyEstateHeightfield/ApplyEstateWeightmaps over the
rectangle, build_ocean.py, place_water.py, build_ground.py, ImportEstateMap.
"""
import json
import os
import subprocess
import sys

import numpy as np
from matplotlib.path import Path
from PIL import Image
from scipy import ndimage
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
R16_GIT = "Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16"
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SAVED = os.path.join(REPO, "Saved", "CoveBay")
SIZE, H = 4033, 2016

# The first cove route: its parent commit's heightfield and the one after its review fixes.
OLD_ROUTE_BEFORE, OLD_ROUTE_AFTER = "c1e426df2^", "6a987b7b2"

# The bay's water (x north, y east, m): its head near (-490, -612), opening south into the old inlet and the sea.
BAY = [(-700, -665), (-578, -662), (-540, -652), (-508, -636), (-492, -618), (-496, -600), (-516, -590),
       (-548, -584), (-566, -566), (-578, -536), (-700, -515)]
FRINGE_M = 28.0          # dry sand round the bay, from the water's edge to the cliff foot
TOP = 1.7                # berm at the cliff foot, as beach_belt.py
SWASH = 0.3              # the water's edge
BERM_M = 12.0            # the berm's flat top at the cliff foot
FORESHORE = 0.10         # the bay floor falls from the swash line at 1 in 10, as beach_belt.py's foreshore
BED_MIN = -4.0
WALL_SLOPE = (0.7, 1.3)  # the walls rise from the berm this steeply (rise per metre), varied along the bay
WALL_KNOT_M = 25.0
SEED = 2610
RIVER_CLEAR_M = 6.0      # off the river channel's waterline
RIVER_RAMP_M = 5.0


def load_r16(data=None):
    raw = np.frombuffer(data, "<u2") if data is not None else np.fromfile(R16, "<u2")
    raw = raw.reshape(SIZE, SIZE)
    return raw, (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]


def git_r16(rev):
    pointer = subprocess.run(["git", "show", f"{rev}:{R16_GIT}"], cwd=REPO, capture_output=True, check=True).stdout
    data = subprocess.run(["git", "lfs", "smudge"], cwd=REPO, input=pointer, capture_output=True, check=True).stdout
    return load_r16(data)[1]


def signed_distance(poly, xs, ys):
    """Metres from the polygon's edge on the (xs, ys) grid: negative inside."""
    gx, gy = np.meshgrid(xs, ys)              # rows y, cols x
    inside = Path(np.asarray(poly, float)).contains_points(np.c_[gx.ravel(), gy.ravel()]).reshape(gx.shape)
    edge = np.asarray(poly + [poly[0]], float)
    dense = []
    for a, b in zip(edge[:-1], edge[1:]):
        n = max(2, int(np.hypot(*(b - a)) / 0.5))
        dense.append(np.linspace(a, b, n))
    d, _ = cKDTree(np.concatenate(dense)).query(np.c_[gx.ravel(), gy.ravel()])
    d = d.reshape(gx.shape)
    return np.where(inside, -d, d), gx, gy


def wall_slope(gx, gy):
    """A slowly varying wall steepness so the bay's walls aren't one even ramp."""
    rng = np.random.default_rng(SEED)
    knots = rng.uniform(0.0, 1.0, (64, 64))
    field = ndimage.zoom(ndimage.gaussian_filter(knots, 1.0), WALL_KNOT_M / 6.0, order=1)
    r = ((gy - gy.min()) / 6.0).astype(int) % field.shape[0]
    c = ((gx - gx.min()) / 6.0).astype(int) % field.shape[1]
    t = np.clip((field[r, c] - field.min()) / max(np.ptp(field), 1e-9), 0, 1)
    return WALL_SLOPE[0] + t * (WALL_SLOPE[1] - WALL_SLOPE[0])


def carve(z, layout):
    pad = FRINGE_M + 120.0
    bx, by = zip(*BAY)
    x0, x1 = int(min(bx) - pad), int(max(bx) + pad)
    y0, y1 = int(min(by) - pad), int(max(by) + pad)
    xs, ys = np.arange(x0, x1 + 1), np.arange(y0, y1 + 1)
    d, gx, gy = signed_distance(list(BAY), xs, ys)
    old = z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1].copy()
    # The designed surface, by distance from the water's edge.
    bed = np.maximum(SWASH + FORESHORE * d, BED_MIN)                        # d < 0 inside
    t = np.clip(d / (FRINGE_M - BERM_M), 0.0, 1.0)
    sand = SWASH + (TOP - SWASH) * (t * t * (3 - 2 * t))
    wall = TOP + (d - FRINGE_M) * wall_slope(gx, gy)
    surface = np.where(d < 0, bed, np.where(d <= FRINGE_M, sand, wall))
    # Lower ground to the design, never raise it.
    new = np.minimum(old, surface)
    # Keep off the river: its channel and run to the sea stay as graded.
    riv = np.asarray(layout["river"], np.float64)
    half = np.r_[np.asarray(layout["riverHalfWidth"], np.float64),
                 np.full(len(riv) - len(layout["riverHalfWidth"]), layout["riverHalfWidth"][-1])]
    dr, j = cKDTree(riv).query(np.c_[gx.ravel(), gy.ravel()])
    dr = (dr - half[j]).reshape(gx.shape)
    w = np.clip((dr - RIVER_CLEAR_M) / RIVER_RAMP_M, 0.0, 1.0)
    new = old + (new - old) * w
    return (y0, x0), old, new, d


def preview(z, d, origin, path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    (y0, x0) = origin
    ny, nx = z.shape
    ys, xs = np.arange(y0, y0 + ny), np.arange(x0, x0 + nx)
    fig, ax = plt.subplots(figsize=(10, 10))
    ax.contourf(ys, xs, z.T, levels=[-10, 0, 0.3, 2.2, 200], colors=["#7aa", "#9cc", "#ec8", "#cb9"])
    cs = ax.contour(ys, xs, z.T, levels=np.arange(0, 100, 5), colors="k", linewidths=0.4)
    ax.clabel(cs, fontsize=6)
    ax.plot(-638, -257.5, "r*", ms=14)
    ax.set_aspect(1)
    ax.set_xlabel("y (east), m")
    ax.set_ylabel("x (north), m")
    plt.savefig(path, dpi=90)


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    if layout.get("coveBay", {}).get("graded"):
        print("cove bay: already graded")
        return
    raw, z = load_r16()
    # 1. Take the first route's cut out (its difference, so later edits in the corridor stay).
    if "coveRoute" in layout:
        delta = git_r16(OLD_ROUTE_AFTER) - git_r16(OLD_ROUTE_BEFORE)
        cut = np.abs(delta) > 1e-6
        z[cut] -= delta[cut]
        print(f"cove bay: took the first cove route's cut out of {int(cut.sum())} vertices")
        del layout["coveRoute"]
        layout["footpaths"] = [p for p in layout.get("footpaths", []) if p.get("name") != "Cove"]
    # 2. The bay.
    (y0, x0), old, new, d = carve(z, layout)
    z[y0 + H:y0 + H + new.shape[0], x0 + H:x0 + H + new.shape[1]] = new
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    moved = np.argwhere(u16 != raw)
    zr = (u16.astype(np.float64) - 32768.0) / 128.0
    sand = (d >= 0) & (d <= FRINGE_M) & (new >= SWASH - 0.05) & (new <= TOP + 0.05)
    print(f"cove bay: {len(moved)} vertices changed; carved down to {(new - old).min():+.1f} m; "
          f"{int(sand.sum())} m2 of dry sand round the bay")
    os.makedirs(SAVED, exist_ok=True)
    preview(zr[y0 + H:y0 + H + new.shape[0], x0 + H:x0 + H + new.shape[1]], d, (y0, x0), os.path.join(SAVED, "cove_bay.png"))
    if dry:
        print("cove bay: --dry, nothing written; preview in Saved/CoveBay/")
        return
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy):
        zn = np.load(npy)
        rows, cols = moved[:, 0], moved[:, 1]          # r16 [row = y + H, col = x + H]; npy [H - x, H + y]
        zn[2 * H - cols, rows] = zr[rows, cols].astype(zn.dtype)
        np.save(npy, zn)
    layout["coveBay"] = {"graded": True, "bay": [list(p) for p in BAY], "fringe": FRINGE_M, "top": TOP, "swash": SWASH}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
    print(f"cove bay: ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")


if __name__ == "__main__":
    main()
