"""Widen the estate's beach (Jenny, 2026-09-29): a belt of dry, walkable sand 17-38 m wide along the foot of
the cliffs on the estate's open south coast, from the west boundary to the cove, shelving into the sea. The
cliff faces above are left exactly as they are: the belt is built seaward of the old waterline and only
ever raises ground. Narrower off the headlands, wider in the bays, and varied along the coast so it isn't
ruler-straight; it tapers into the cove's own beach at the east end, and keeps clear of the river.

    python Scripts\\Terrain\\beach_belt.py       # grade once; a second run only reports

Section, seaward of the old waterline (s m): a berm TOP m above the sea at the cliff foot, falling to the
swash line (SWASH m) over the local dry width, then a foreshore at FORESHORE_GRADE down to the seabed. Low
pockets within BERM_BACK m landward of the old waterline come up to the berm.

Writes the heightfield, PNG and work npy (HOMESTEAD_TERRAIN_WORK), and records "beach" in
estate_layout.json. Then: river_channel.py (re-seats the mouth; the belt keeps RIVER_CLEAR_M off the river),
weightmaps.py (the new sand paints as Beach), bake_ocean.py, bake_ground.py and bake_estate_map.py; in the
editor ApplyEstateHeightfield and ApplyEstateWeightmaps over the rectangle it prints, build_ocean.py,
place_water.py, build_ground.py and ImportEstateMap. See README.md, "Beach belt".
"""
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt, gaussian_filter, label
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016

# The owned open coast (game metres, y east): the estate's west boundary to the cove's west headland.
Y_WEST, Y_EAST = -1150.0, -615.0
TAPER_M = 25.0                     # the belt fades in and out over this length beyond each end
X_WINDOW = (-860.0, -440.0)        # the south coast lies in here
TOP = 1.7                          # berm crest at the cliff foot, m above the sea
SWASH = 0.3                        # where the dry sand ends
FORESHORE_GRADE = 0.10             # below the swash line, down to the seabed
DRY_WIDTH = (17.0, 38.0)           # Jenny's playtest brief (orchestrator, 2026-09-30)
BAY_SIGMA_M = 40.0                 # land share within about this reach says bay (high) or headland (low)
BAY_RANGE = (0.35, 0.65)           # land share mapped onto narrow..wide
KNOT_M = 60.0                      # along-coast variation
BAY_WEIGHT = 0.5                   # of the width's variation from the coast's shape; the rest from the knots
SEED = 2026
BERM_BACK = 6.0                    # landward of the old waterline, low ground rises to the berm this far
WIDTH_SMOOTH_M = 6.0               # the dry width's smoothing once carried out to sea
RIVER_CLEAR_M = 9.0                # keep this far off the river's line and its run to the sea
RIVER_RAMP_M = 6.0


def design(z, layout):
    """The raise over the coast window: (row slice, col slice, raise m, dry width field, s field)."""
    sea = z < 0.0
    lab, _ = label(sea)
    ids = np.setdiff1d(np.unique(lab[:, 0]), [0])            # water connected to the map's southern edge
    sea = np.isin(lab, ids)

    c0, c1 = int(X_WINDOW[0]) + H, int(X_WINDOW[1]) + H
    r0, r1 = int(Y_WEST - TAPER_M - BAY_SIGMA_M) + H, int(Y_EAST + TAPER_M + BAY_SIGMA_M) + H
    win = z[r0:r1 + 1, c0:c1 + 1]
    wsea = sea[r0:r1 + 1, c0:c1 + 1]
    # s: metres seaward of the old waterline (negative inland). idx: each sea cell's nearest land cell.
    d_sea, idx = distance_transform_edt(wsea, return_indices=True)
    s = gaussian_filter(d_sea - distance_transform_edt(~wsea), 1.5)
    yy, xx = np.mgrid[r0:r1 + 1, c0:c1 + 1]
    Y, X = (yy - H).astype(np.float64), (xx - H).astype(np.float64)

    # The dry width at each waterline point, carried out to sea from the nearest one.
    land_share = gaussian_filter((~wsea).astype(np.float64), BAY_SIGMA_M)
    bay = np.clip((land_share - BAY_RANGE[0]) / (BAY_RANGE[1] - BAY_RANGE[0]), 0.0, 1.0)
    rng = np.random.default_rng(SEED)
    knots = np.arange(Y_WEST - TAPER_M - KNOT_M, Y_EAST + TAPER_M + 2 * KNOT_M, KNOT_M)
    along = np.interp(Y, knots, rng.uniform(0.0, 1.0, len(knots)))
    mix = np.clip(BAY_WEIGHT * bay + (1.0 - BAY_WEIGHT) * along, 0.0, 1.0)
    width_at_shore = DRY_WIDTH[0] + (DRY_WIDTH[1] - DRY_WIDTH[0]) * mix
    width = width_at_shore[idx[0], idx[1]]
    width = np.where(wsea, width, width_at_shore)
    # Where the nearest shore cell switches, the carried width jumps (up to 6 m): smoothed, so the berm has
    # no scarp across the swash zone (review, 2026-09-30).
    width = gaussian_filter(width, WIDTH_SMOOTH_M)

    berm = TOP - (TOP - SWASH) * np.clip(s, 0.0, None) / width
    beach = np.where(s <= width, berm, SWASH - FORESHORE_GRADE * (s - width))
    beach = np.where(s < -BERM_BACK, -1e9, np.where(s < 0.0, TOP, beach))

    t = np.clip(np.minimum(Y - (Y_WEST - TAPER_M), (Y_EAST + TAPER_M) - Y) / TAPER_M, 0.0, 1.0)
    ends = t * t * (3 - 2 * t)
    river = np.asarray(layout["river"], np.float64)
    d_river = cKDTree(river).query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape)
    keep_river = np.clip((d_river - RIVER_CLEAR_M) / RIVER_RAMP_M, 0.0, 1.0)
    raise_by = np.maximum(beach - win, 0.0) * ends * keep_river
    return slice(r0, r1 + 1), slice(c0, c1 + 1), raise_by, width, s, Y, X


def dry_widths(z_after, rows, cols, s, Y):
    """Measured dry sand: from each point of the old waterline, march seaward along the belt's normal until the
    new ground drops under the swash line. Returns (y of the point, metres) for the points inside the belt."""
    after = z_after[rows, cols]
    gy, gx = np.gradient(gaussian_filter(s, 4.0))
    norm = np.hypot(gx, gy) + 1e-9
    ny, nx = gy / norm, gx / norm                                  # seaward (s grows)
    pts = np.argwhere((np.abs(s) < 0.5) & (Y >= Y_WEST) & (Y <= Y_EAST))
    out = []
    for r, c in pts[::3]:
        if after[r, c] < SWASH:
            continue
        d = 0.0
        while d < 80.0:
            d += 0.5
            rr, cc = int(round(r + ny[r, c] * d)), int(round(c + nx[r, c] * d))
            if not (0 <= rr < after.shape[0] and 0 <= cc < after.shape[1]) or after[rr, cc] < SWASH:
                break
        out.append((float(Y[r, c]), d))
    return out


SECTION_M = 50.0


def main():
    layout = json.load(open(LAYOUT))
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]
    if layout.get("beach", {}).get("graded"):
        # The design reads the old waterline, which the belt has moved: nothing to redesign or re-measure.
        print("beach: already graded (Tests/EstateBeachTests.py checks the graded belt)")
        return
    rows, cols, raise_by, width, s, Y, X = design(z, layout)
    z[rows, cols] += raise_by
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    changed = np.argwhere(u16 != raw)
    if "--dry" not in sys.argv:
        u16.astype("<u2").tofile(R16)
        Image.fromarray(u16).save(PNG)
        npy = os.path.join(WORK, "game_reshaped_4033.npy")
        if os.path.exists(npy):
            # The work npy ([row = H - x, col = H + y]) takes the graded heights where the belt raised the
            # ground (a copy, not an addition, so a work folder graded before stays right).
            zn = np.load(npy)
            raised = raise_by > 0
            zn[(H - X[raised]).astype(int), (H + Y[raised]).astype(int)] = z[rows, cols][raised].astype(zn.dtype)
            np.save(npy, zn)
        layout["beach"] = {"graded": True, "yWest": Y_WEST, "yEast": Y_EAST, "taper": TAPER_M, "top": TOP,
                           "swash": SWASH, "foreshoreGrade": FORESHORE_GRADE, "dryWidth": list(DRY_WIDTH),
                           "seed": SEED}
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)
    (a0, b0), (a1, b1) = changed.min(0), changed.max(0)
    print(f"beach: {len(changed)} vertices raised, up to {raise_by.max():.2f} m; ApplyEstateHeightfield "
          f"rows {a0}-{a1}, cols {b0}-{b1}{' (dry run: nothing written)' if '--dry' in sys.argv else ''}")
    shore = width[(s > -0.5) & (s < 0.5)]
    print(f"beach: designed dry width {shore.min():.0f}-{shore.max():.0f} m (median {np.median(shore):.0f}) from the "
          f"old waterline, over {Y_EAST - Y_WEST:.0f} m of coast plus {TAPER_M:.0f} m tapers")
    measured = dry_widths(z, rows, cols, s, Y)
    w = np.array([m for _, m in measured])
    if not len(w):
        print("beach: no waterline points measured")
        return
    print(f"beach: measured dry sand from the cliff foot to the swash line: {np.percentile(w, 5):.0f}-"
          f"{np.percentile(w, 95):.0f} m (5th-95th percentile), median {np.median(w):.0f} m, over {len(w)} points")
    for y0 in np.arange(Y_WEST, Y_EAST, SECTION_M):
        sec = [m for y, m in measured if y0 <= y < y0 + SECTION_M]
        if sec:
            print(f"  y {y0:6.0f}..{y0 + SECTION_M:.0f}: median {np.median(sec):4.1f} m")


if __name__ == "__main__":
    main()
