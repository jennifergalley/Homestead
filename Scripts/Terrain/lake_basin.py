"""Dig the estate lake: an upland pool north-west of the farm (up-left on the north-up estate map), with a
cut bank on its uphill side, a low turfed pond bay on its downhill side, a shelving landing on the farm
side for the pail, and a footpath from the farm's north fence to the landing.

Like river_channel.py it grades the heightfield to a designed section and records the result in
estate_layout.json, so running it again changes nothing:

    python Scripts\\Terrain\\lake_basin.py

Writes (in step with each other):
    Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16   (the source of truth)
    Scripts/Terrain/Estate_Heightmap_4033.png
    <work>/game_reshaped_4033.npy                              (for scatter and the bakes)
    Scripts/Terrain/estate_layout.json                          ("lake": shore, level, landing, path)
    Content/SurvivalGame/Estate/Runtime/EstateScenery.bin       (clears the water and path, plants the margin)
Then patch the Landscape (ApplyEstateHeightfield over the rectangle it prints), run place_water.py,
bake_ground.py, build_ground.py and bake_estate_map.py. See README.md, "Estate lake".
"""
import json
import math
import os

import numpy as np
from PIL import Image

import lake_features

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016

# The site, agreed with the orchestrator 2026-09-29 (A_NW): game metres, x north, y east.
CENTRE = (-50.0, -745.0)
SEMI_AXES = (38.0, 22.0)            # along and across the contour
AXIS_DEG = 45.0                     # the long axis runs south-west to north-east, along the slope
SHORE_POINTS = 64
WOBBLE = ((3, 0.11, 1.0), (5, 0.07, 2.1), (7, 0.035, 0.4))   # (harmonic, amplitude, phase): an irregular shore
LEVEL_PERCENTILE = 30               # water level: this percentile of the natural ground round the shore

# Section (metres). In from the shore the bed shelves to MAX_DEPTH; out from it a lip rises to the bank.
SHELF_SLOPE = 0.25
MAX_DEPTH = 1.8
LIP_SLOPE, LIP_WIDTH = 0.3, 1.5     # the wet margin rising out of the water
CUT_SLOPE = 0.65                    # the uphill cut bank beyond the lip
BAY_CREST, BAY_WIDTH, BAY_BACK = 0.49, 3.0, 0.4   # the downhill pond bay: crest above water, top width, back slope
LANDING_RADIUS = 7.0                # the farm-side landing: gentler shelf and lip within this of its point
LANDING_SHELF, LANDING_LIP = 0.1, 0.12
REACH = 12.0

# The path from the farm's north fence (the derelict farm field is x -222..-162, y -705..-645).
FARM_GATE = (-160.0, -688.0)
PATH_HALF, PATH_FALLOFF = 1.4, 2.5


def shore_polygon():
    cx, cy = CENTRE
    a, b = SEMI_AXES
    t = np.linspace(0.0, 2.0 * np.pi, SHORE_POINTS, endpoint=False)
    r = 1.0 + sum(amp * np.sin(k * t + ph) for k, amp, ph in WOBBLE)
    u, v = a * r * np.cos(t), b * r * np.sin(t)
    c, s = math.cos(math.radians(AXIS_DEG)), math.sin(math.radians(AXIS_DEG))
    return np.c_[cx + u * c - v * s, cy + u * s + v * c]


def signed_distance(poly, px, py):
    """Metres from points to the closed polygon's edges, negative inside."""
    a = poly
    b = np.roll(poly, -1, axis=0)
    d = np.full(px.shape, np.inf)
    inside = np.zeros(px.shape, bool)
    for (ax, ay), (bx, by) in zip(a, b):
        ex, ey = bx - ax, by - ay
        t = np.clip(((px - ax) * ex + (py - ay) * ey) / (ex * ex + ey * ey), 0.0, 1.0)
        d = np.minimum(d, np.hypot(px - (ax + t * ex), py - (ay + t * ey)))
        crosses = (ay > py) != (by > py)
        inside ^= crosses & (px < ax + (py - ay) * ex / np.where(ey == 0, 1e-9, ey))
    return np.where(inside, -d, d)


def ground_at(z, x, y):
    fx, fy = x + H, y + H
    i, j = int(math.floor(fx)), int(math.floor(fy))
    tx, ty = fx - i, fy - j
    return (z[j, i] * (1 - tx) * (1 - ty) + z[j, i + 1] * tx * (1 - ty)
            + z[j + 1, i] * (1 - tx) * ty + z[j + 1, i + 1] * tx * ty)


def design(z):
    poly = shore_polygon()
    level = float(np.percentile([ground_at(z, x, y) for x, y in poly], LEVEL_PERCENTILE))
    gate = np.array(FARM_GATE)
    landing = poly[int(np.argmin(np.hypot(*(poly - gate).T)))]
    # The path: from the gate, bending once to meet the landing square-on to the shore.
    out = landing - np.array(CENTRE)
    out /= np.linalg.norm(out)
    approach = landing + out * 9.0
    pts = [gate, (gate + approach) / 2.0 + np.array([6.0, -4.0]), approach, landing - out * 0.5]
    path = np.array(pts)
    return {"shore": np.round(poly, 2).tolist(), "level": round(level, 3), "landing": np.round(landing, 2).tolist(),
            "path": np.round(path, 2).tolist()}


def densify(points, step):
    p = np.asarray(points, np.float64)
    out = [p[0]]
    for a, b in zip(p[:-1], p[1:]):
        n = max(1, int(np.ceil(np.linalg.norm(b - a) / step)))
        out += [a + (b - a) * k / n for k in range(1, n + 1)]
    return np.array(out)


def grade(z, lake, cells):
    """Grade the lake and its path into z in place. cells(lo, hi) -> (rows, cols, x, y) of the array."""
    poly = np.asarray(lake["shore"])
    level = lake["level"]
    lx, ly = lake["landing"]
    lo = poly.min(axis=0) - REACH
    hi = poly.max(axis=0) + REACH
    rows, cols, x, y = cells(lo, hi)
    s = signed_distance(poly, x.astype(np.float64), y.astype(np.float64))
    near = smoothstep(LANDING_RADIUS, LANDING_RADIUS * 0.4, np.hypot(x - lx, y - ly))
    shelf = SHELF_SLOPE + (LANDING_SHELF - SHELF_SLOPE) * near
    lip_slope = LIP_SLOPE + (LANDING_LIP - LIP_SLOPE) * near
    natural = z[rows, cols]
    d_in = np.maximum(-s, 0.0)
    depth = MAX_DEPTH * (1.0 - np.exp(-shelf * d_in / MAX_DEPTH))
    inside = level - np.maximum(depth, 0.05)
    lip = level + 0.04 + lip_slope * np.minimum(s, LIP_WIDTH)
    crest = level + 0.04 + lip_slope * LIP_WIDTH
    beyond = np.maximum(s - LIP_WIDTH, 0.0)
    cut = crest + CUT_SLOPE * beyond
    bay = np.where(s <= BAY_WIDTH, level + BAY_CREST, level + BAY_CREST - BAY_BACK * (s - BAY_WIDTH))
    bay = np.minimum(bay, cut)
    outside = np.where(s <= LIP_WIDTH, lip, np.maximum(np.minimum(natural, cut), bay))
    target = np.where(s < 0.0, inside, outside)
    blend = smoothstep(REACH, REACH - 2.0, s)          # fade into the untouched ground at the reach
    z[rows, cols] = natural + (target - natural) * blend

    # The footpath: level across its width, following the ground along it, blended into its verges.
    path = densify(lake["path"], 0.5)
    lo2 = path.min(axis=0) - (PATH_HALF + PATH_FALLOFF + 1)
    hi2 = path.max(axis=0) + (PATH_HALF + PATH_FALLOFF + 1)
    rows, cols, x, y = cells(lo2, hi2)
    d = np.full(x.shape, np.inf)
    idx = np.zeros(x.shape, int)
    for k, (px, py) in enumerate(path):
        dk = np.hypot(x - px, y - py)
        closer = dk < d
        d = np.where(closer, dk, d)
        idx = np.where(closer, k, idx)
    if "pathProfile" not in lake:
        along = np.array([ground_at(z, px, py) for px, py in path])
        lake["pathProfile"] = np.round(np.convolve(np.pad(along, 6, mode="edge"), np.ones(13) / 13.0, mode="valid"), 3).tolist()
    along = np.asarray(lake["pathProfile"])
    w = smoothstep(PATH_HALF + PATH_FALLOFF, PATH_HALF, d)
    z[rows, cols] = z[rows, cols] + (along[idx] - z[rows, cols]) * w


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def main():
    layout = json.load(open(LAYOUT))
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]
    lake = layout.get("lake") or design(z)
    if lake.get("graded"):
        # The verges blend by weight, so grading twice isn't a no-op: once graded, only the scenery reapplies.
        # To regrade, restore the heightfield, PNG and npy and delete "lake" from the layout.
        kept, dropped, added = lake_features.apply_to_scenery_file(
            os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), lake)
        print(f"lake: already graded (level {lake['level']} m); scenery: {dropped} cleared, {added} margin, {kept} total")
        return

    def r16_cells(lo, hi):
        c0, c1 = int(np.floor(lo[0])) + H, int(np.ceil(hi[0])) + H
        r0, r1 = int(np.floor(lo[1])) + H, int(np.ceil(hi[1])) + H
        rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
        rr, cc = rr.ravel(), cc.ravel()
        return rr, cc, (cc - H).astype(np.float64), (rr - H).astype(np.float64)

    grade(z, lake, r16_cells)          # also fixes lake["pathProfile"] on the first run
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
            return rr, cc, (H - rr).astype(np.float64), (cc - H).astype(np.float64)

        grade(zn, lake, npy_cells)
        np.save(npy, zn.astype(np.float32))

    lake["graded"] = True
    layout["lake"] = lake
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    kept, dropped, added = lake_features.apply_to_scenery_file(
        os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), lake)

    if len(changed):
        (r0, c0), (r1, c1) = changed.min(0), changed.max(0)
        print(f"lake: level {lake['level']} m, {len(changed)} vertices changed; ApplyEstateHeightfield rectangle "
              f"MinX {c0} MinY {r0} MaxX {c1} MaxY {r1}")
    else:
        print(f"lake: level {lake['level']} m; heightfield already graded")
    print(f"scenery: {dropped} records cleared from the water and path, {added} margin plants and stones, {kept} total")


if __name__ == "__main__":
    main()
