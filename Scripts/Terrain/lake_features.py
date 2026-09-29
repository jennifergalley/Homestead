"""The estate lake's scenery: clear the scatter out of the water and off its footpath, and plant its margin
with tussocky grass and rushes, and granite along its cut bank. Shared by lake_basin.py (applied to the
committed EstateScenery.bin) and scatter.py (applied to a fresh scatter), so both give the same margin.

Deterministic and idempotent: the margin comes from a fixed seed, and applying it again first removes
any records in the cleared areas and the exact margin records it would add.
"""
import math
import struct

import numpy as np

# EstateSceneryKinds indices (HomesteadWorld.cpp): no new kinds are claimed.
TALUS, BOULDER, GRASS_TALL, GRASS_MID = 5, 6, 10, 11
CLEAR_WATER_M = 1.0        # nothing grows closer than this outside the shore (the wet lip)
CLEAR_PATH_M = 1.6         # half width kept clear along the footpath
MARGIN_SEED = 4119
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])


def _signed_distance(poly, px, py):
    a = np.asarray(poly, np.float64)
    b = np.roll(a, -1, axis=0)
    d = np.full(px.shape, np.inf)
    inside = np.zeros(px.shape, bool)
    for (ax, ay), (bx, by) in zip(a, b):
        ex, ey = bx - ax, by - ay
        t = np.clip(((px - ax) * ex + (py - ay) * ey) / (ex * ex + ey * ey), 0.0, 1.0)
        d = np.minimum(d, np.hypot(px - (ax + t * ex), py - (ay + t * ey)))
        crosses = (ay > py) != (by > py)
        inside ^= crosses & (px < ax + (py - ay) * ex / np.where(ey == 0, 1e-9, ey))
    return np.where(inside, -d, d)


def _path_distance(path, px, py):
    p = np.asarray(path, np.float64)
    d = np.full(px.shape, np.inf)
    for (ax, ay), (bx, by) in zip(p[:-1], p[1:]):
        ex, ey = bx - ax, by - ay
        t = np.clip(((px - ax) * ex + (py - ay) * ey) / max(ex * ex + ey * ey, 1e-9), 0.0, 1.0)
        d = np.minimum(d, np.hypot(px - (ax + t * ex), py - (ay + t * ey)))
    return d


def cleared(lake, x_m, y_m):
    """True where the scatter must not stand: in the water, on its wet lip or on the footpath."""
    x_m, y_m = np.asarray(x_m, np.float64), np.asarray(y_m, np.float64)
    return (_signed_distance(lake["shore"], x_m, y_m) < CLEAR_WATER_M) | (_path_distance(lake["path"], x_m, y_m) < CLEAR_PATH_M)


def margin(lake):
    """(kind, x m, y m, yaw deg, scale) records planting the margin, in a fixed order."""
    rng = np.random.default_rng(MARGIN_SEED)
    poly = np.asarray(lake["shore"], np.float64)
    centre = poly.mean(axis=0)
    lx, ly = lake["landing"]
    seg = np.roll(poly, -1, axis=0) - poly
    lengths = np.hypot(*seg.T)
    total = lengths.sum()
    out = []
    step = 1.4
    for k in range(int(total / step)):
        along = (k + rng.uniform(-0.3, 0.3)) * step % total
        i = int(np.searchsorted(np.cumsum(lengths), along))
        t = (along - (np.cumsum(lengths)[i] - lengths[i])) / lengths[i]
        p = poly[i] + seg[i] * t
        outward = p - centre
        outward /= np.linalg.norm(outward)
        if math.hypot(p[0] - lx, p[1] - ly) < 9.0:
            continue                                   # keep the landing open
        draw = rng.uniform()
        off = rng.uniform(1.1, 2.6)
        q = p + outward * off + rng.normal(0.0, 0.25, 2)
        if _path_distance(lake["path"], np.array([q[0]]), np.array([q[1]]))[0] < CLEAR_PATH_M:
            continue
        kind = GRASS_TALL if draw < 0.62 else GRASS_MID
        # Scales as the drive's verges (scatter.py): the grass meshes are small at scale 1.
        out.append((kind, float(q[0]), float(q[1]), float(rng.uniform(0, 360)), float(rng.uniform(1.6, 2.5))))
    # Granite breaking through the cut bank on the uphill (north-west) shore, a few boulders at the water.
    uphill = np.array([1.0, -1.0]) / math.sqrt(2.0)     # x north, y east: north-west
    for k in range(9):
        i = int(rng.integers(len(poly)))
        p = poly[i]
        outward = p - centre
        outward /= np.linalg.norm(outward)
        if np.dot(outward, uphill) < 0.35 or math.hypot(p[0] - lx, p[1] - ly) < 12.0:
            continue
        q = p + outward * rng.uniform(2.5, 4.5)
        out.append((TALUS if k % 3 else BOULDER, float(q[0]), float(q[1]), float(rng.uniform(0, 360)), float(rng.uniform(0.55, 0.9))))
    return out


def apply(records, lake):
    """Scenery records (RECORD array, x/y in cm) with the lake cleared and its margin planted."""
    x, y = records["x"] / 100.0, records["y"] / 100.0
    plants = margin(lake)
    wanted = {(k, round(px * 100.0), round(py * 100.0)) for k, px, py, _, _ in plants}
    existing = {(int(k), round(float(px)), round(float(py))) for k, px, py in zip(records["k"], records["x"], records["y"])}
    duplicate = np.array([(int(k), round(float(px)), round(float(py))) in wanted
                          for k, px, py in zip(records["k"], records["x"], records["y"])], bool)
    keep = records[~(cleared(lake, x, y) | duplicate)]
    extra = np.zeros(len(plants), RECORD)
    for i, (k, px, py, yaw, s) in enumerate(plants):
        extra[i] = (k, b"\0\0\0", px * 100.0, py * 100.0, yaw, s)
    return np.concatenate([keep, extra]), len(records) - len(keep) - (len(wanted & existing)), len(plants)


def apply_to_scenery_file(path, lake):
    data = open(path, "rb").read()
    records = np.frombuffer(data[8:], RECORD)
    result, dropped, added = apply(records, lake)
    with open(path, "wb") as fh:
        fh.write(b"HSC1" + struct.pack("<I", len(result)) + result.tobytes())
    return len(result), dropped, added
