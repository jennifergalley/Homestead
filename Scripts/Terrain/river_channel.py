"""Seat the estate river in a proper channel: a spring pool at its head, banks on both sides all the
way down, a ford where the drive crosses, and a shallow run across the cove beach into the sea.

reshape.py's first cut only lowered the ground (a 4.4 m flat bed with low 0.45 slope banks), so on
side-hills the downhill bank was missing and the water slab floated over grass. This grades the
channel to a designed cross-section instead: it raises a bank where the ground falls away and cuts
where it rises, then records the water surface and the waterline half-width for place_water.py.

    python Scripts\\Terrain\\river_channel.py        # heightfield + PNG + npy + estate_layout.json

Inputs and outputs (all in step with each other):
    Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16   (read and rewritten: the source of truth)
    Scripts/Terrain/Estate_Heightmap_4033.png                   (rewritten from the r16)
    <work>/game_reshaped_4033.npy                              (same channel applied, for scatter and bakes)
    Scripts/Terrain/estate_layout.json                          (adds riverSurface, riverHalfWidth, riverEnd)
Then apply the heightfield to the Landscape in the editor (UHomesteadEstateAuthoringLibrary::
ApplyEstateHeightfield) and run place_water.py, bake_ground.py and build_ground.py.

The profile is computed once and stored in estate_layout.json; later runs reuse it, so running this
again (or from reshape.py, which recomputes it on fresh terrain) is idempotent.
"""
import json
import os

import numpy as np
from PIL import Image
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016

# Channel shape (metres). The stream grows from a spring rill to its full width over GROW_M.
GROW_M = 300.0
BED_HALF = (0.35, 1.3)        # flat bed half-width, spring -> full
DEPTH = (0.14, 0.35)          # water depth over the bed
FREEBOARD = (0.25, 0.5)       # bank top above the water
BANK_SLOPE = 0.6              # rise per metre from the bed edge up to the bank top
OUTER_SLOPE = 0.6             # the cut keeps rising at this slope beyond the bank top
BERM = 0.8                    # flat bank top kept where the ground falls away
FILL_SLOPE = 0.5              # a raised bank falls back to the natural ground at this slope
POOL_RADIUS, POOL_DEPTH = 1.0, 0.3   # spring pool bed half-width and depth at the head
POOL_M = 4.0                  # pool blends into the rill over this distance
FORD = (-15.0, -30.0)         # the drive crosses here: shallow water between low, gentle banks
FORD_REACH = 18.0
BEACH_CUT = 0.3               # across the beach the bed runs this far below the sand
BEACH = dict(bed=1.8, depth=0.12, freeboard=0.12, slope=0.3)
MOUTH_SURFACE = 0.08          # the stream ends at the first point whose surface is below this (sea level 0)
# The mouth (Jenny/Integration: the river stopped a couple of metres short of the sea, with dry sand and
# the ocean's shore wash between): cut the channel through the beach berm below sea level so the sea runs
# up into it, carry the stream on until its bed is MOUTH_END_BED below the sea, and hold its surface just
# under the ocean's there so the ribbon slides beneath the sea instead of fighting it.
MOUTH_GROUND = 0.5            # where the beach is lower than this, the bed is cut below the sea
MOUTH_BED = -0.3              # ... to at least this
MOUTH_END_BED = -0.6          # the stream runs on until its bed is this far under the sea
MOUTH_UNDER_SEA = -0.03       # its surface, where it has reached the sea
REACH = 9.0                   # grading reach from the centreline
DENSE_M = 0.5


def smoothstep(t):
    t = np.clip(t, 0.0, 1.0)
    return t * t * (3 - 2 * t)


def densify(pts, step):
    """Catmull-Rom through the layout points (as reshape.densify), with each sample's arc position."""
    p = np.asarray(pts, np.float64)
    ext = np.vstack([p[0] * 2 - p[1], p, p[-1] * 2 - p[-2]])
    out, src = [], []
    for i in range(1, len(ext) - 2):
        p0, p1, p2, p3 = ext[i - 1], ext[i], ext[i + 1], ext[i + 2]
        n = max(2, int(np.linalg.norm(p2 - p1) / step))
        t = np.linspace(0, 1, n, endpoint=False)[:, None]
        out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t ** 2
                          + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
        src.append(i - 1 + t[:, 0])
    out.append(p[-1:])
    src.append(np.array([len(p) - 1.0]))
    return np.vstack(out), np.concatenate(src)


def ground_at(z, x, y):
    """Bilinear height (m) at game metres; z is indexed [row = y + H, column = x + H] (the r16 layout)."""
    fx, fy = np.asarray(x) + H, np.asarray(y) + H
    i, j = np.floor(fx).astype(int), np.floor(fy).astype(int)
    tx, ty = fx - i, fy - j
    return (z[j, i] * (1 - tx) * (1 - ty) + z[j, i + 1] * tx * (1 - ty)
            + z[j + 1, i] * (1 - tx) * ty + z[j + 1, i + 1] * tx * ty)


def profile(z, layout):
    """Per layout point: bed, depth, bed half-width, freeboard, bank slope, raised-bank flag; and the end."""
    pts = np.asarray(layout["river"], np.float64)
    bed = np.asarray(layout["riverBed"], np.float64)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))]
    g = ground_at(z, pts[:, 0], pts[:, 1])
    # Across the cove beach (laid after reshape's river cut) the bed follows the sand down to the sea.
    bed = np.minimum.accumulate(np.maximum(bed, g - BEACH_CUT))
    beach = smoothstep((g - np.asarray(layout["riverBed"]) - 0.15) / 0.3)
    beach = np.maximum.accumulate(beach)
    u = smoothstep(s / GROW_M)
    half = BED_HALF[0] + (BED_HALF[1] - BED_HALF[0]) * u
    depth = DEPTH[0] + (DEPTH[1] - DEPTH[0]) * u
    free = FREEBOARD[0] + (FREEBOARD[1] - FREEBOARD[0]) * u
    slope = np.full_like(s, BANK_SLOPE)
    pool = 1.0 - smoothstep(s / POOL_M)
    half = half + (POOL_RADIUS - half) * pool
    depth = depth + (POOL_DEPTH - depth) * pool
    ford = 1.0 - smoothstep((np.linalg.norm(pts - np.array(FORD), axis=1) - 6.0) / (FORD_REACH - 6.0))
    depth = depth + (0.2 - depth) * ford
    free = free + (0.06 - free) * ford
    slope = slope + (0.22 - slope) * ford
    for key, arr in (("bed", half), ("depth", depth), ("freeboard", free), ("slope", slope)):
        arr += (BEACH[key] - arr) * beach
    surface = bed + depth
    # Include the first point at the sea, so the stream runs into the shore wash.
    end = int(np.argmax(surface < MOUTH_SURFACE)) + 1 if (surface < MOUTH_SURFACE).any() else len(pts)
    raise_ok = np.ones_like(s)  # per point, so a stretch can be left un-banked
    return dict(s=s, bed=bed, depth=depth, half=half, free=free, slope=slope, raise_ok=raise_ok, end=end)


def cut_mouth(z, layout, prof):
    """Once: lower the bed below the sea where the beach is lower than MOUTH_GROUND, and end the stream at
    the first point whose bed is MOUTH_END_BED under the sea (kept in the stored profile afterwards)."""
    pts = np.asarray(layout["river"], np.float64)
    g = ground_at(z, pts[:, 0], pts[:, 1])
    tail = np.arange(len(pts)) > len(pts) // 2          # the lower river only, never the spring
    low = tail & (g < MOUTH_GROUND)
    prof["bed"] = np.where(low, np.minimum(prof["bed"], MOUTH_BED), prof["bed"])
    prof["bed"] = np.minimum.accumulate(prof["bed"])
    deep = np.flatnonzero(tail & (prof["bed"] < MOUTH_END_BED))
    prof["end"] = int(deep[0]) + 1 if len(deep) else len(pts)


def carve(z, layout, prof, z_of_xy):
    """Grade the channel into z in place. z_of_xy(rows, cols) -> (x, y) maps array indices to game metres."""
    end = prof["end"]
    pts = np.asarray(layout["river"], np.float64)[:end]
    dense, src = densify(pts, DENSE_M)
    at = lambda key: np.interp(src, np.arange(end), prof[key][:end])
    bed, half, depth, free, slope, raise_ok = (at(k) for k in ("bed", "half", "depth", "free", "slope", "raise_ok"))
    top = depth + free                      # bank top above the bed
    wt = half + top / slope                 # bank top distance from the centreline

    xs, ys = dense[:, 0], dense[:, 1]
    lo = np.array([xs.min() - REACH, ys.min() - REACH])
    hi = np.array([xs.max() + REACH, ys.max() + REACH])
    rows, cols, gx, gy = z_of_xy(lo, hi)
    d, i = cKDTree(dense).query(np.c_[gx, gy], distance_upper_bound=REACH)
    keep = np.isfinite(d)
    rows, cols, d, i = rows[keep], cols[keep], d[keep], i[keep]
    b, w, t, top_d, sl, r = bed[i], half[i], top[i], wt[i], slope[i], raise_ok[i]
    channel = b + np.clip(d - w, 0.0, None) * sl
    cut = np.where(d <= top_d, channel, b + t + (d - top_d) * OUTER_SLOPE)
    levee = np.where(d <= top_d + BERM, b + t, b + t - (d - top_d - BERM) * FILL_SLOPE)
    levee = np.where(r > 0.5, levee, -1e9)
    old = z[rows, cols]
    z[rows, cols] = np.minimum(np.maximum(old, levee), cut)
    return np.abs(z[rows, cols] - old).max(), float(np.maximum(z[rows, cols] - old, 0).max())


def main():
    layout = json.load(open(LAYOUT))
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]

    if "riverSurface" in layout:
        n = len(layout["river"])
        prof = {k: np.asarray(layout["riverChannel"][k], np.float64) for k in ("bed", "depth", "half", "free", "slope", "raise_ok")}
        prof["end"] = layout["riverEnd"]
        assert len(prof["bed"]) == n
    else:
        prof = profile(z, layout)
    if not layout.get("riverMouth"):
        cut_mouth(z, layout, prof)
        layout["riverMouth"] = {"cut": True, "bed": MOUTH_BED, "endBed": MOUTH_END_BED, "underSea": MOUTH_UNDER_SEA}

    def r16_cells(lo, hi):
        c0, c1 = int(np.floor(lo[0])) + H, int(np.ceil(hi[0])) + H     # column = x + H
        r0, r1 = int(np.floor(lo[1])) + H, int(np.ceil(hi[1])) + H     # row = y + H
        rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
        rr, cc = rr.ravel(), cc.ravel()
        return rr, cc, cc - H, rr - H

    change, raised = carve(z, layout, prof, r16_cells)
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    changed = np.argwhere(u16 != raw)
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)

    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy):
        zn = np.load(npy).astype(np.float64)                  # [row = H - x, col = H + y]

        def npy_cells(lo, hi):
            r0, r1 = H - int(np.ceil(hi[0])), H - int(np.floor(lo[0]))
            c0, c1 = H + int(np.floor(lo[1])), H + int(np.ceil(hi[1]))
            rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
            rr, cc = rr.ravel(), cc.ravel()
            return rr, cc, H - rr, cc - H

        carve(zn, layout, prof, npy_cells)
        np.save(npy, zn.astype(np.float32))

    end = prof["end"]
    layout["riverEnd"] = int(end)
    layout["riverSurface"] = np.round(np.maximum(prof["bed"] + prof["depth"], MOUTH_UNDER_SEA)[:end], 3).tolist()
    layout["riverHalfWidth"] = np.round((prof["half"] + prof["depth"] / prof["slope"])[:end], 3).tolist()
    layout["riverChannel"] = {k: np.round(np.asarray(prof[k], np.float64), 4).tolist()
                              for k in ("bed", "depth", "half", "free", "slope", "raise_ok")}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    if len(changed):
        (r0, c0), (r1, c1) = changed.min(0), changed.max(0)
        print(f"river channel: {len(changed)} vertices changed, rows {r0}-{r1}, cols {c0}-{c1};"
              f" max change {change:.2f} m, max raise {raised:.2f} m; ends at point {end} of {len(prof['bed'])}")
    else:
        print("river channel: heightfield already graded")


if __name__ == "__main__":
    main()
