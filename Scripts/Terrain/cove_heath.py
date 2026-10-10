"""Coastal heath on the cove's carved ground (shrink-estate-map, Balance): the bay's new walls and the sunken cut
down to the cove steps keep grass, heath and flowers, with bare rock or earth only on the steepest slopes.

Usage: python Scripts/Terrain/cove_heath.py      (after weightmaps.py; then bake_ground.py and bake_estate_map.py)

weightmaps.py and bake_ground.py import heath_weight() to loosen their slope limits inside the zone. Run as a
script, it scatters flower clumps (EstateScenery.bin records tagged 'CH1', replaced on every run) across the
zone's grassed slopes, clear of the route itself.
"""
import json
import os

import numpy as np
from matplotlib.path import Path
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
LAYOUT = os.path.join(HERE, "estate_layout.json")
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
SIZE, H = 4033, 2016

BAY_REACH_M = 30.0        # the bay polygon grown by this: its carved walls up to the clifftop
ROUTE_REACH_M = 14.0      # either side of the cove route: the sunken cut and its banks
FADE_M = 10.0             # back to the ordinary rules over this
# Inside the zone: rock only above these slopes (degrees; elsewhere 30-42), grass thins out over the next.
CLIFF_SLOPE_DEG = (46.0, 58.0)
GRASS_SLOPE_DEG = (54.0, 44.0)
STONY_KEEP = 0.35         # of bake_ground's stony-bank soil left on the zone's slopes

TAG = b"CH1"
FLOWER_KINDS = (42, 43, 46, 47, 46, 47)   # route_sights.py's bloom kinds, campion and foxglove weighted
CLUMP_EVERY_M2 = 55.0
CLUMP_SLOPE_DEG = (12.0, 44.0)
CLUMP_MIN_Z_M = 4.0                   # above the sand
CLUMP_OFF_ROUTE_M = 2.6
CLUMP_SIZE = (4, 8)
CLUMP_RADIUS_M = 1.4
SEED = 1852
RECORD = np.dtype([("k", "u1"), ("pad", "S3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])


def _route(layout):
    return next((p["points"] for p in layout.get("footpaths", []) if p.get("name") == "Cove"), None)


def _dense(points, step=0.5):
    p = np.asarray(points, np.float64)[:, :2]
    out = [p[0]]
    for a, b in zip(p[:-1], p[1:]):
        n = max(int(np.hypot(*(b - a)) / step), 1)
        out.extend(a + (b - a) * t for t in np.linspace(0, 1, n + 1)[1:])
    return np.array(out)


def heath_weight(x, y, layout):
    """0..1 heath zone weight at metric points x (north), y (east), any matching shapes."""
    x = np.asarray(x, np.float64)
    y = np.asarray(y, np.float64)
    out = np.zeros(np.broadcast(x, y).shape, np.float32)
    bay = layout.get("coveBay", {}).get("bay")
    route = _route(layout)
    if not bay or not route:
        return out
    bay = np.asarray(bay, np.float64)
    line = _dense(route)
    reach = max(BAY_REACH_M, ROUTE_REACH_M) + FADE_M
    lo = np.minimum(bay.min(0), line.min(0)) - reach
    hi = np.maximum(bay.max(0), line.max(0)) + reach
    xb, yb = np.broadcast_to(x, out.shape), np.broadcast_to(y, out.shape)
    win = (xb >= lo[0]) & (xb <= hi[0]) & (yb >= lo[1]) & (yb <= hi[1])
    if not win.any():
        return out
    pts = np.c_[xb[win], yb[win]]
    d_bay = cKDTree(_dense(np.vstack([bay, bay[:1]]))).query(pts)[0]
    d_bay[Path(bay).contains_points(pts)] = 0.0
    d_route = cKDTree(line).query(pts)[0]
    excess = np.minimum(d_bay - BAY_REACH_M, d_route - ROUTE_REACH_M)
    t = np.clip(1.0 - excess / FADE_M, 0.0, 1.0)
    out[win] = t * t * (3 - 2 * t)
    return out


def main():
    layout = json.load(open(LAYOUT))
    z = (np.fromfile(R16, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0   # [y + H, x + H]
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    bay = np.asarray(layout["coveBay"]["bay"], np.float64)
    line = _dense(_route(layout))
    reach = BAY_REACH_M + FADE_M
    x0, x1 = int(min(bay[:, 0].min(), line[:, 0].min()) - reach), int(max(bay[:, 0].max(), line[:, 0].max()) + reach)
    y0, y1 = int(min(bay[:, 1].min(), line[:, 1].min()) - reach), int(max(bay[:, 1].max(), line[:, 1].max()) + reach)
    X, Y = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1), indexing="ij")
    zone = heath_weight(X, Y, layout)
    zz, ss = z[Y + H, X + H], slope[Y + H, X + H]
    ok = (zone > 0.6) & (zz > CLUMP_MIN_Z_M) & (ss > CLUMP_SLOPE_DEG[0]) & (ss < CLUMP_SLOPE_DEG[1])
    ok &= cKDTree(line).query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape) > CLUMP_OFF_ROUTE_M
    rng = np.random.default_rng(SEED)
    cand = np.c_[X[ok], Y[ok]].astype(np.float64)
    n = int(len(cand) / CLUMP_EVERY_M2)
    centres = cand[rng.choice(len(cand), size=n, replace=False)] + rng.uniform(-0.5, 0.5, (n, 2)) if n else cand[:0]
    allowed = set(map(tuple, cand.astype(int)))
    records = []
    for c in centres:
        kind = int(rng.choice(FLOWER_KINDS))
        for p in c + rng.normal(0.0, CLUMP_RADIUS_M * 0.5, (int(rng.integers(*CLUMP_SIZE)), 2)):
            if (int(round(p[0])), int(round(p[1]))) in allowed:
                records.append((kind, TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(0, 360), rng.uniform(0.85, 1.2)))
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * RECORD.itemsize], RECORD)
    kept = rec[rec["pad"] != TAG]
    out = np.concatenate([kept, np.array(records, RECORD)]) if records else kept
    with open(SCENERY, "wb") as fh:
        fh.write(raw[:4] + np.uint32(len(out)).tobytes() + out.tobytes() + raw[8 + count * RECORD.itemsize:])
    print(f"cove heath: {len(centres)} flower clumps, {len(records)} records (replaced {count - len(kept)}); "
          f"scenery now {len(out)}")


if __name__ == "__main__":
    main()
