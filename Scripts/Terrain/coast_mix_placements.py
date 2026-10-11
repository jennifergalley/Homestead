"""Coast-mix placements (map stage 3): retire the estate placements and drop the scenery the new coast displaced.

Usage: python Scripts/Terrain/coast_mix_placements.py [--dry]   (after coast_mix.py, cove_route.py, estate_footpaths.py)

Placement ids are save identity, so nothing is renumbered or moved: any placement (the compact map's top-up
included) that now stands in the water or on the sand, on a cliff face, on a footpath or on the village street is listed in
Simulation/HomesteadEstateCoastMixRetired.inc, which ProvisionalEstatePlacements drops with the compact map's
retired ids. Scenery records (EstateScenery.bin) on the sand, in the water or on cliff faces are dropped. A rerun
re-emits the same list. Then bake_ground.py and bake_estate_map.py.
"""
import glob
import json
import os
import re
import sys

import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
SIM = os.path.join(REPO, "Source", "SurvivalGame", "Simulation")
LAYOUT = os.path.join(HERE, "estate_layout.json")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
OUT_INC = os.path.join(SIM, "HomesteadEstateCoastMixRetired.inc")
OUT_TOPUP = os.path.join(SIM, "HomesteadEstateCoastMixPlacements.inc")
REPLACE_IDS = (587000, 587099)
REPLACE_RING = (4.0, 10.0)        # m from the displaced bramble
REPLACE_GAP_M = 3.5               # from every generated row (the table's 3 m rule, with a margin)
SIZE, H = 4033, 2016

SAND_TOP_M = 2.3            # the dry sand's berm is 1.7 m; nothing interactive grows below this
CLIFF_SLOPE = 1.0           # rise per metre (45 degrees): a cliff face
PATH_M = 2.0                # off any footpath's centreline
STREET_CLEAR_M = 2.75       # the village street's half width (town_layout.py STREET_HALF_WIDTH_M)
CLEAROUT_IDS = (570000, 579999)   # the manor clear-out: a path through it is overgrown until she clears it
TREE_SCENERY = {0, 1, 13, 14, 15, 16, 19, 20, 21, 22}
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
ROW = re.compile(r"(\w+)\((\d{6}),\s*(?:ResourceKind::)?(\w+)?,?\s*(-?[\d.]+),\s*(-?[\d.]+)")


def heights():
    return (np.fromfile(R16, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0


def bilinear(z, x, y):
    c, r = np.asarray(x, np.float64) + H, np.asarray(y, np.float64) + H
    c0, r0 = np.floor(c).astype(int), np.floor(r).astype(int)
    fc, fr = c - c0, r - r0
    return ((z[r0, c0] * (1 - fc) + z[r0, c0 + 1] * fc) * (1 - fr) + (z[r0 + 1, c0] * (1 - fc) + z[r0 + 1, c0 + 1] * fc) * fr)


def placement_rows():
    """(id, x m, y m) of every generated placement row, the compact map's top-up included."""
    rows = []
    for f in sorted(glob.glob(os.path.join(SIM, "HomesteadEstate*.inc"))):
        if "Retired" in os.path.basename(f) or "CoveRoute" in f or "PublicRoad" in f or f == OUT_TOPUP:
            continue
        for m in ROW.finditer(open(f).read()):
            if m[1] in ("retired",):
                continue
            rows.append((int(m[2]), float(m[4]) / 100.0, float(m[5]) / 100.0))
    return rows


def path_tree(layout):
    pts = []
    for p in layout.get("footpaths", []):
        a = np.asarray(p["points"], np.float64)[:, :2]
        for u, v in zip(a[:-1], a[1:]):
            pts.append(np.linspace(u, v, max(2, int(np.hypot(*(v - u)) / 0.5) + 1)))
    route = json.load(open(os.path.join(HERE, "cove_route.json")))
    pts.append(np.asarray(route["centreline"], np.float64)[:, :2])
    # The village street (berries.py and forage keep only off the main road): a 5.5 m street, so clear 2 m more.
    street = layout.get("town", {}).get("street")
    if street:
        a = np.asarray(street, np.float64)[:, :2]
        for off in np.linspace(-STREET_CLEAR_M, STREET_CLEAR_M, 7):
            d = np.gradient(a, axis=0)
            n = np.c_[-d[:, 1], d[:, 0]] / np.maximum(np.hypot(d[:, 0], d[:, 1]), 1e-9)[:, None]
            pts.append(a + n * off)
    return cKDTree(np.concatenate(pts))


def replace_brambles(rows, retired, z, slope, lines, layout):
    """A spot REPLACE_RING m from each retired bramble: dry, gentle, off paths, on the estate, clear of every row."""
    from matplotlib.path import Path
    boundary = Path(np.asarray(layout["polygons"]["EstateBoundary"], np.float64))
    manor = Path(np.asarray(layout["polygons"]["ManorFootprint"], np.float64))
    kinds = {}
    for f in sorted(glob.glob(os.path.join(SIM, "HomesteadEstate*.inc"))):
        for m in ROW.finditer(open(f).read()):
            kinds[int(m[2])] = m[3] or ("BerryBush" if m[1] == "berry" else None)
    taken = [np.array([r[1], r[2]]) for r in rows]
    out = []
    rng = np.random.default_rng(1855)
    for rid, x, y in rows:
        if rid not in retired or kinds.get(rid) != "BerryBush" or not (540000 <= rid < 550000):
            continue
        for k in range(1200):
            a, d = rng.uniform(0, 2 * np.pi), rng.uniform(REPLACE_RING[0], REPLACE_RING[1] * (1 + k // 400))
            p = np.array([x + d * np.cos(a), y + d * np.sin(a)])
            if (bilinear(z, p[0], p[1]) > SAND_TOP_M + 0.5 and bilinear(slope, p[0], p[1]) < 0.4
                    and lines.query(p)[0] > PATH_M + 1.5 and boundary.contains_point(p) and not manor.contains_point(p)
                    and min(np.hypot(*(q - p)) for q in taken) > REPLACE_GAP_M):
                out.append((float(p[0]), float(p[1])))
                taken.append(p)
                break
    return out


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    z = heights()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.hypot(gx, gy)
    lines = path_tree(layout)
    rows = placement_rows()
    xy = np.array([(r[1], r[2]) for r in rows])
    ground = bilinear(z, xy[:, 0], xy[:, 1])
    steep = bilinear(slope, xy[:, 0], xy[:, 1])
    clearout = np.array([CLEAROUT_IDS[0] <= r[0] <= CLEAROUT_IDS[1] for r in rows])
    on_path = (lines.query(xy)[0] < PATH_M) & ~clearout
    displaced = (ground < SAND_TOP_M) | (steep > CLIFF_SLOPE) | on_path
    retired = sorted({r[0] for r, d in zip(rows, displaced) if d})
    print(f"coast mix placements: {len(rows)} rows; {len(retired)} displaced "
          f"(sand/water {int((ground < SAND_TOP_M).sum())}, cliff {int((steep > CLIFF_SLOPE).sum())}, "
          f"path {int(on_path.sum())})")

    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * RECORD.itemsize], RECORD)
    at = np.c_[rec["x"] / 100.0, rec["y"] / 100.0]
    inside = (np.abs(at[:, 0]) < H - 2) & (np.abs(at[:, 1]) < H - 2)
    sg = np.where(inside, bilinear(z, np.where(inside, at[:, 0], 0), np.where(inside, at[:, 1], 0)), 99.0)
    ss = np.where(inside, bilinear(slope, np.where(inside, at[:, 0], 0), np.where(inside, at[:, 1], 0)), 0.0)
    drop = inside & ((sg < SAND_TOP_M - 0.4) | ((ss > CLIFF_SLOPE) & np.isin(rec["k"], list(TREE_SCENERY))))
    street = layout.get("town", {}).get("street")
    if street:
        dense = []
        a = np.asarray(street, np.float64)[:, :2]
        for u, v in zip(a[:-1], a[1:]):
            dense.append(np.linspace(u, v, max(2, int(np.hypot(*(v - u)) / 0.5) + 1)))
        drop |= cKDTree(np.concatenate(dense)).query(at)[0] < STREET_CLEAR_M + 1.0
    print(f"coast mix scenery: {int(drop.sum())} of {count} records on the sand, in the water or treed cliff faces")
    if dry:
        return
    replacements = replace_brambles(rows, retired, z, slope, lines, layout)
    with open(OUT_TOPUP, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/coast_mix_placements.py; do not edit by hand.\n")
        fh.write("// A bramble beside each one the coast mix's paths or street displaced (587000-587099; save identity:\n")
        fh.write("// never renumbered, never reused).\n")
        for i, (x, y) in enumerate(replacements):
            fh.write(f"coastMix({REPLACE_IDS[0] + i}, ResourceKind::BerryBush, {x * 100.0:.1f}, {y * 100.0:.1f});\n")
    print(f"coast mix placements: {len(replacements)} replacement brambles")
    with open(OUT_INC, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/coast_mix_placements.py; do not edit by hand.\n")
        fh.write("// Placements the coast mix (map stage 3) put in the water, on the sand, on a cliff face, on a path or on the moved village street.\n")
        fh.write("// ProvisionalEstatePlacements drops these ids with the compact map's; they are never reused.\n")
        for i in retired:
            fh.write(f"retired({i});\n")
    if drop.any():
        kept = rec[~drop]
        with open(SCENERY, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(kept)).tobytes() + kept.tobytes() + raw[8 + count * RECORD.itemsize:])


if __name__ == "__main__":
    main()
