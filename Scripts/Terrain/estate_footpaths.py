"""Footpaths and pads for the compact map (shrink-estate-map): the mine ruin's yard, the woodland path from
the manor to the mine, the clifftop link from the mine to the head of the cove steps, and the path from the
village down to the river. Each path is a graded earth bed like the cove route's (cove_route.py), no steeper
than its own limit, following the ground as closely as that allows; the yard is a level pad.

Usage: python Scripts/Terrain/estate_footpaths.py [--dry]       (after cove_bay.py and cove_route.py)

Run once ("estateFootpaths.graded" in the layout); later runs only re-emit the map's dashed footpaths and
clear the scatter off them. Writes EstateHeightfield.r16, Estate_Heightmap_4033.png and the work npy where
they changed, "footpaths" (the map) and the mine's height in estate_layout.json, and clears EstateScenery.bin
records off the paths and the yard. Prints the ApplyEstateHeightfield rectangle. Then bake_ground.py (it wears
every footpath) and bake_estate_map.py. Mirror MineEntrance's z in HomesteadEstate.cpp.
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter, gaussian_filter1d
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cove_route as route_kit  # noqa: E402  (its path design and heightfield helpers)

LAYOUT = os.path.join(HERE, "estate_layout.json")
SCENERY = route_kit.SCENERY
H = route_kit.H

STATION_M = 0.25
CLEAR_M = 2.4                # scatter cleared either side of a path, as the cove route's
SMOOTH_GROUND_M = 1.5
COVE_KEEP_M = (1.8, 3.8)      # the cove route's bed stays as cove_route.py graded it: no change within the first,
                              # blending in by the second (a path joining it meets its level)

# (name, control points (x, y) m, max grade, pin the start to this height or None for the ground)
PATHS = [
    ("Mine", [(-252, -668), (-270, -688), (-300, -700), (-340, -712), (-375, -718), (-401, -721)], 1.0 / 5.0),
    ("MineLink", [(-404, -716), (-398, -690), (-392, -662), (-394, -645), (-397, -638)], 1.0 / 5.0),
    ("VillageRiver", [(-118, -322), (-138, -322), (-162, -345), (-190, -350), (-202, -328), (-212, -306),
                      (-236, -298), (-258, -292), (-278, -287)], 1.0 / 5.0),
]
YARD = {"centre": (-410.0, -722.0), "half": (9.0, 7.0), "blend": 8.0}   # the mine ruin's level yard


def ground_fn_for(z, pts):
    x0, y0 = int(math.floor(pts[:, 0].min())) - 16, int(math.floor(pts[:, 1].min())) - 16
    x1, y1 = int(math.ceil(pts[:, 0].max())) + 16, int(math.ceil(pts[:, 1].max())) + 16
    smooth = gaussian_filter(z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1], SMOOTH_GROUND_M)

    def fn(x, y):
        c = np.asarray(x, np.float64) - x0
        r = np.asarray(y, np.float64) - y0
        c0 = np.clip(np.floor(c).astype(int), 0, smooth.shape[1] - 2)
        r0 = np.clip(np.floor(r).astype(int), 0, smooth.shape[0] - 2)
        fc, fr = c - c0, r - r0
        return ((smooth[r0, c0] * (1 - fc) + smooth[r0, c0 + 1] * fc) * (1 - fr)
                + (smooth[r0 + 1, c0] * (1 - fc) + smooth[r0 + 1, c0 + 1] * fc) * fr)
    return fn


def design_path(z, control, grade):
    pts = route_kit.chaikin([tuple(p) for p in control], route_kit.CHAIKIN_PASSES)
    xy, s = route_kit.resample(pts, STATION_M)
    ground = ground_fn_for(z, xy)
    tgt = gaussian_filter1d(ground(xy[:, 0], xy[:, 1]), 3.0 / STATION_M, mode="nearest")
    prof = route_kit.pinned_lipschitz(tgt, s[1] - s[0], grade * route_kit.DESIGN_MARGIN, tgt[0], tgt[-1])
    stations = [(x, y, zz, zz, "path", 0) for (x, y), zz in zip(xy, prof)]
    worst = float(np.max(np.abs(np.diff(prof)) / np.diff(s)))
    return stations, float(s[-1]), worst, float(np.abs(prof - tgt).max())


def level_yard(z):
    (cx, cy), (hx, hy), blend = YARD["centre"], YARD["half"], YARD["blend"]
    x0, x1 = int(cx - hx - blend), int(cx + hx + blend)
    y0, y1 = int(cy - hy - blend), int(cy + hy + blend)
    X, Y = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
    old = z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1]
    inside = (np.abs(X - cx) <= hx) & (np.abs(Y - cy) <= hy)
    level = float(old[inside].mean())
    out = np.maximum(np.abs(X - cx) - hx, np.abs(Y - cy) - hy).clip(0, None)
    t = np.clip(out / blend, 0, 1)
    w = 1.0 - t * t * (3 - 2 * t)
    z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] = old + (level - old) * w
    return level


def clear_scenery(paths, dry):
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], route_kit.RECORD)
    at = np.c_[rec["x"] / 100.0, rec["y"] / 100.0]
    keep = np.ones(count, bool)
    for pts in paths:
        dense, _ = route_kit.resample(np.asarray(pts, np.float64), 0.5)
        keep &= cKDTree(dense).query(at)[0] >= CLEAR_M
    (cx, cy), (hx, hy) = YARD["centre"], YARD["half"]
    keep &= ~((np.abs(at[:, 0] - cx) <= hx + 2) & (np.abs(at[:, 1] - cy) <= hy + 2))
    if (~keep).any() and not dry:
        kept = rec[keep]
        with open(SCENERY, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(kept)).tobytes() + kept.tobytes() + raw[8 + count * 20:])
    return int((~keep).sum())


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    raw, z = route_kit.load_r16()
    graded = layout.get("estateFootpaths", {}).get("graded")
    route = json.load(open(os.path.join(HERE, "cove_route.json")))
    cove = cKDTree(np.asarray(route["centreline"], np.float64)[:, :2])
    lines = {}
    for name, control, grade in PATHS:
        stations, length, worst, dev = design_path(z, control, grade)
        lines[name] = [(st[0], st[1]) for st in stations]
        print(f"footpath {name}: {length:.0f} m, steepest 1 in {1 / max(worst, 1e-6):.1f}, "
              f"bed up to {dev:.1f} m off the ground")
        if not graded:
            rows, cols, change = route_kit.corridor_change(stations, z, layout)
            d = cove.query(np.c_[cols - H, rows - H].astype(np.float64))[0]
            z[rows, cols] += change * np.clip((d - COVE_KEEP_M[0]) / (COVE_KEEP_M[1] - COVE_KEEP_M[0]), 0.0, 1.0)
    if not graded:
        level = level_yard(z)
        mine = layout["landmarks"]["MineEntrance"]
        layout["landmarks"]["MineEntrance"] = [mine[0], mine[1], round(level, 2), mine[3]]
        print(f"mine yard levelled at {level:.2f} m")
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    moved = np.argwhere(u16 != raw)
    cleared = clear_scenery(list(lines.values()), dry)
    print(f"footpaths: {len(moved)} vertices changed, {cleared} scenery records cleared")
    if dry:
        return
    if len(moved):
        u16.astype("<u2").tofile(route_kit.R16)
        Image.fromarray(u16).save(route_kit.PNG)
        npy = os.path.join(route_kit.WORK, "game_reshaped_4033.npy")
        if os.path.exists(npy):
            zr = (u16.astype(np.float64) - 32768.0) / 128.0
            zn = np.load(npy)
            r, c = moved[:, 0], moved[:, 1]
            zn[2 * H - c, r] = zr[r, c].astype(zn.dtype)
            np.save(npy, zn)
        (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
        print(f"footpaths: ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")
    names = {n for n, _, _ in PATHS}
    paths = [p for p in layout.get("footpaths", []) if p.get("name") not in names]
    for name, pts in lines.items():
        dense, _ = route_kit.resample(np.asarray(pts), route_kit.FOOTPATH_STEP_M)
        paths.append({"name": name, "points": np.round(dense, 2).tolist()})
    layout["footpaths"] = paths
    layout["estateFootpaths"] = {"graded": True, "yard": YARD}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)


if __name__ == "__main__":
    main()
