"""Compact-map placements (shrink-estate-map, design decision 4): which saved placements the new bay and paths
displace, the scatter the bay leaves on sand or water, scenery trees in place of the fellable trees that now
stand on neighbours' land, and a top-up of fellable trees and forage on her smaller estate.

Usage: python Scripts/Terrain/compact_placements.py [--dry]   (after cove_bay.py, cove_route.py, estate_footpaths.py)

Placement ids are save identity, so nothing here renumbers or moves a placement:
- HomesteadEstateRetiredPlacements.inc lists the ids the bay, its sand, the new paths and the mine yard displaced
  (water or swash under them, the ground moved more than RETIRE_DZ_M, or within RETIRE_PATH_M of a path or step
  centreline). ProvisionalEstatePlacements drops them, and everything off the estate, after the whole table is
  built, so no other row's proximity skip changes: every surviving id keeps its kind and position.
- HomesteadEstateCompactPlacements.inc appends new ids (585000-585199 fellable trees, 585200-585299 forage).
  Each tree takes the spot of a scatter tree of the same mesh, which leaves EstateScenery.bin.
- EstateScenery.bin, once ("compactPlacements.scenery" in the layout): records on the bay's sand and water and
  on ground the bay moved are dropped (low ground cover stays on the walls), and each fellable tree retired off
  the estate is drawn as a scatter tree of the same mesh family where it stood, so neighbours' woods stay woods.
The .inc files are regenerated identically on a rerun. Then bake_ground.py and bake_estate_map.py.
"""
import glob
import json
import os
import re
import subprocess
import sys

import numpy as np
from matplotlib.path import Path
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
SIM = os.path.join(REPO, "Source", "SurvivalGame", "Simulation")
LAYOUT = os.path.join(HERE, "estate_layout.json")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
R16_GIT = "Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16"
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
RETIRED_INC = os.path.join(SIM, "HomesteadEstateRetiredPlacements.inc")
TOPUP_INC = os.path.join(SIM, "HomesteadEstateCompactPlacements.inc")
SIZE, H = 4033, 2016
BEFORE = "f9cfadaf6"            # the heightfield before the compact map (the approved plan's commit)

RETIRE_DZ_M = 0.5
RETIRE_WATER_M = 0.8            # ground at or under this: swash, sand's edge or water
RETIRE_PATH_M = 2.0
GROUND_COVER = {3, 4, 9, 10, 11, 32, 33}   # bracken, yarrow, fern, grasses: stay on the bay's walls
SAND_TOP_M = 2.5                # nothing grows on ground this low inside the bay
TREE_KINDS = (0, 1)             # scatter meshes the fellable trees use: broadleaf (SM_TreeSmall02_Woodland), fir
TREE_IDS = (585000, 585199)
FORAGE_IDS = (585200, 585299)
TREE_COUNT = 100                # Jenny, 2026-10-10: ~100 more fellable trees on the smaller estate
FORAGE_COUNT = 45               # Balance: woodland forage back to about 65 spots (22 of 120 stay on the estate)
DOOR_BERRY_M = (8.0, 18.0)      # one bramble within sight of the front door (the cove path displaced one)
FORAGE_GAP_M = 10.0
MORE_FOOD_GAP_M = 20.5          # from forage.py's abundance rows, which keep 20 m from all other food
MORE_FOOD_IDS = ((582144, 582299), (581015, 581099))
TREE_MANOR_M = 45.0             # off the manor (its clear-out and forecourt)
TREE_FARM_M = 15.0
TREE_GAP_M = 8.0                # between new fellable trees
PLACEMENT_GAP_M = 4.0           # from any other placement
PATH_GAP_M = 3.5
FORAGE_OFF_PATH_M = (3.5, 7.0)  # forage beside her paths
FORAGE_MANOR_M = 35.0
SEED = 1851
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
KIND_BY_FUNCTION = {"berry": "BerryBush", "windfall": "FallenBranch"}
# The whole built table (rows HomesteadEstate.cpp places in code too), for the top-up's spacing. Build it first:
#   cmake --build Build\Native --config Release --target HomesteadPlacementDump
DUMP = os.path.join(REPO, "Build", "Native", "Release", "HomesteadPlacementDump.exe")


def r16_heights(data=None):
    raw = np.frombuffer(data, "<u2") if data is not None else np.fromfile(R16, "<u2")
    return (raw.reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0    # [row = y + H, col = x + H]


def git_r16(rev):
    pointer = subprocess.run(["git", "show", f"{rev}:{R16_GIT}"], cwd=REPO, capture_output=True, check=True).stdout
    return r16_heights(subprocess.run(["git", "lfs", "smudge"], cwd=REPO, input=pointer, capture_output=True,
                                      check=True).stdout)


def bilinear(z, x, y):
    c, r = np.asarray(x, np.float64) + H, np.asarray(y, np.float64) + H
    c0, r0 = np.floor(c).astype(int), np.floor(r).astype(int)
    fc, fr = c - c0, r - r0
    return ((z[r0, c0] * (1 - fc) + z[r0, c0 + 1] * fc) * (1 - fr) + (z[r0 + 1, c0] * (1 - fc) + z[r0 + 1, c0 + 1] * fc) * fr)


def placements():
    """Every generated placement row: (id, kind, x m, y m), skipping this script's own output."""
    rows = []
    pat = re.compile(r"(\w+)\((\d{6}),\s*(?:ResourceKind::(\w+),\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)")
    for f in sorted(glob.glob(os.path.join(SIM, "HomesteadEstate*Placements.inc"))):
        if f in (RETIRED_INC, TOPUP_INC):
            continue
        for m in pat.finditer(open(f).read()):
            kind = m[3] or KIND_BY_FUNCTION.get(m[1])
            if kind:
                rows.append((int(m[2]), kind, float(m[4]) / 100.0, float(m[5]) / 100.0))
    return rows


def dense_lines(layout):
    pts = []
    for p in layout.get("footpaths", []):
        a = np.asarray(p["points"], np.float64)
        for u, v in zip(a[:-1], a[1:]):
            n = max(2, int(np.hypot(*(v - u)) / 0.5) + 1)
            pts.append(np.linspace(u, v, n))
    route = json.load(open(os.path.join(HERE, "cove_route.json")))
    pts.append(np.asarray(route["centreline"], np.float64)[:, :2])
    return cKDTree(np.concatenate(pts))


def read_scenery():
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    return raw, np.frombuffer(raw[8:8 + count * 20], RECORD).copy()


def write_scenery(raw, rec):
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    with open(SCENERY, "wb") as fh:
        fh.write(raw[:4] + np.uint32(len(rec)).tobytes() + rec.tobytes() + raw[8 + count * 20:])


def write_retired(retired):
    with open(RETIRED_INC, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/compact_placements.py; do not edit by hand.\n")
        fh.write("// Placements the compact map's bay, its sand, the new paths and the mine yard displaced (shrink-estate-map).\n")
        fh.write("// ProvisionalEstatePlacements drops these ids after building the table; they are never reused.\n")
        for i in retired:
            fh.write(f"retired({i});\n")


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    z_now, z_was = r16_heights(), git_r16(BEFORE)
    boundary = Path(np.asarray(layout["polygons"]["EstateBoundary"], np.float64))
    bay = layout["coveBay"]["bay"]
    bay_box = (min(p[0] for p in bay) - 60, max(p[0] for p in bay) + 60, min(p[1] for p in bay) - 60, max(p[1] for p in bay) + 60)
    yard = layout["estateFootpaths"]["yard"]
    lines = dense_lines(layout)
    rng = np.random.default_rng(SEED)

    rows = placements()
    xy = np.array([(r[2], r[3]) for r in rows])
    now, was = bilinear(z_now, xy[:, 0], xy[:, 1]), bilinear(z_was, xy[:, 0], xy[:, 1])
    on_path = lines.query(xy)[0] < RETIRE_PATH_M
    (cx, cy), (hx, hy) = yard["centre"], yard["half"]
    in_yard = (np.abs(xy[:, 0] - cx) <= hx + 2) & (np.abs(xy[:, 1] - cy) <= hy + 2)
    displaced = (now < RETIRE_WATER_M) | (np.abs(now - was) > RETIRE_DZ_M) | on_path | in_yard
    retired = sorted(r[0] for r, d in zip(rows, displaced) if d)
    inside = boundary.contains_points(xy)
    off_estate = sum(1 for r, i, d in zip(rows, inside, displaced) if not i and not d)
    print(f"placements: {len(rows)} generated rows; {len(retired)} displaced by the bay and paths; "
          f"{off_estate} more off the estate (dropped in code)")

    raw, rec = read_scenery()
    at = np.c_[rec["x"] / 100.0, rec["y"] / 100.0]
    done = layout.get("compactPlacements", {}).get("scenery")
    if not done:
        # 1. The bay: nothing on its sand or water; only low ground cover on the walls it carved.
        sx, sy = at[:, 0], at[:, 1]
        near_bay = (sx >= bay_box[0]) & (sx <= bay_box[1]) & (sy >= bay_box[2]) & (sy <= bay_box[3])
        zn = np.where(near_bay, bilinear(z_now, np.where(near_bay, sx, 0), np.where(near_bay, sy, 0)), 99.0)
        zw = np.where(near_bay, bilinear(z_was, np.where(near_bay, sx, 0), np.where(near_bay, sy, 0)), 99.0)
        moved = np.abs(zn - zw) > RETIRE_DZ_M
        drop = near_bay & ((zn < SAND_TOP_M) | (moved & ~np.isin(rec["k"], list(GROUND_COVER))))
        print(f"scenery: {int(drop.sum())} records off the bay's sand, water and carved walls")
        rec, at = rec[~drop], at[~drop]
        # 2. Fellable trees retired off the estate stand on as scatter trees of the same mesh family.
        trees = [(r, x, y) for r, d, i in zip(rows, displaced, inside) if r[1] == "ForestTree" and not d and not i
                 for x, y in [(r[2], r[3])]]
        add = np.zeros(len(trees), RECORD)
        for k, (r, x, y) in enumerate(trees):
            h = np.random.default_rng(r[0])
            add[k] = (0 if h.random() >= 0.2 else 1, b"\0\0\0", x * 100.0, y * 100.0, h.uniform(0, 360), h.uniform(0.9, 1.16))
        rec = np.concatenate([rec, add])
        at = np.c_[rec["x"] / 100.0, rec["y"] / 100.0]
        print(f"scenery: {len(add)} fellable trees off the estate drawn as scatter trees")

    if done:
        # A rerun: the top-up already took its scatter trees; keep the committed .inc as it is.
        if not dry:
            write_retired(retired)
        print("compact placements: scenery and top-up already done; retired list re-emitted")
        return

    # 3. New fellable trees in her woods, each in place of a scatter tree of the same mesh.
    if not os.path.exists(DUMP):
        raise SystemExit(f"compact placements: build {DUMP} first (see DUMP)")
    out = subprocess.run([DUMP], capture_output=True, text=True, check=True).stdout.split()
    table = np.array(out, np.float64).reshape(-1, 3)
    # Every earlier row, retired or not: the table's 3 m skip runs before the retire pass.
    table = table[[not (TREE_IDS[0] <= int(i) <= FORAGE_IDS[1]) for i in table[:, 0]]]
    keep_xy = table[:, 1:] / 100.0
    more_food = table[[any(a <= int(i) <= b for a, b in MORE_FOOD_IDS) for i in table[:, 0]], 1:] / 100.0
    more_food = cKDTree(more_food if len(more_food) else np.zeros((1, 2)) + 1e9)
    taken = cKDTree(keep_xy)
    manor = np.mean(np.asarray(layout["polygons"]["ManorFootprint"], np.float64), axis=0)
    farm = np.array([(-222, -705), (-222, -645), (-162, -645), (-162, -705)], np.float64)
    fx0, fx1, fy0, fy1 = farm[:, 0].min(), farm[:, 0].max(), farm[:, 1].min(), farm[:, 1].max()
    farm_d = np.hypot(np.clip(np.maximum(fx0 - at[:, 0], at[:, 0] - fx1), 0, None),
                      np.clip(np.maximum(fy0 - at[:, 1], at[:, 1] - fy1), 0, None))
    cand = (np.isin(rec["k"], TREE_KINDS) & boundary.contains_points(at)
            & (np.hypot(*(at - manor).T) >= TREE_MANOR_M) & (farm_d >= TREE_FARM_M)
            & (taken.query(at)[0] >= PLACEMENT_GAP_M) & (lines.query(at)[0] >= PATH_GAP_M))
    cand &= bilinear(z_now, at[:, 0], at[:, 1]) > 3.0
    idx = np.flatnonzero(cand)
    rng.shuffle(idx)
    chosen = []
    for k in idx:
        if all(np.hypot(*(at[k] - at[j])) >= TREE_GAP_M for j in chosen):
            chosen.append(k)
        if len(chosen) == TREE_COUNT:
            break
    chosen.sort(key=lambda k: (round(float(at[k, 0]), 1), round(float(at[k, 1]), 1)))
    mask = np.ones(len(rec), bool)
    mask[chosen] = False
    rec_out = rec[mask]
    chosen_xy = [(float(at[k, 0]), float(at[k, 1])) for k in chosen]
    print(f"top-up: {len(chosen_xy)} fellable trees in her woods")

    # 4. Forage beside her paths: roots and berry bushes, off the manor's clear-out.
    tree_tree = cKDTree(np.array(chosen_xy)) if chosen_xy else None
    names = {"Cove", "Mine", "MineLink"}
    forage = []
    for p in layout.get("footpaths", []):
        if p["name"] not in names:
            continue
        pts = np.asarray(p["points"], np.float64)
        for a, b in zip(pts[:-1], pts[1:]):
            d = b - a
            n = np.array([-d[1], d[0]]) / max(np.hypot(*d), 1e-9)
            for side in (-1, 1):
                off = rng.uniform(*FORAGE_OFF_PATH_M)
                c = (a + b) / 2 + n * side * off
                forage.append(c)
    forage = np.array(forage)
    rng.shuffle(forage)
    # ... and in her woods away from the paths.
    woods = rng.uniform([-560.0, -880.0], [60.0, -470.0], (4000, 2))
    forage = np.concatenate([forage, woods])
    door = np.array(layout["coveRoute"]["control"][0], np.float64)
    ring = [door + r * np.array([np.cos(a), np.sin(a)]) for r in np.linspace(*DOOR_BERRY_M, 6)
            for a in np.linspace(0.0, 2 * np.pi, 24, endpoint=False)]
    ruin = Path(np.asarray(layout["polygons"]["ManorFootprint"], np.float64))
    picked = []
    for k, c in enumerate(list(ring) + list(forage)):
        if ruin.contains_point(c, radius=-2.0) or ruin.contains_point(c, radius=2.0):
            continue
        by_door = k < len(ring)
        if by_door and picked:
            continue
        if not boundary.contains_points([c])[0] or (not by_door and np.hypot(*(c - manor)) < FORAGE_MANOR_M):
            continue
        zc = float(bilinear(z_now, c[0], c[1]))
        if zc < 3.0 or abs(zc - float(bilinear(z_was, c[0], c[1]))) > RETIRE_DZ_M:
            continue
        if taken.query(c)[0] < 3.2 or lines.query(c)[0] < FORAGE_OFF_PATH_M[0] - 0.5:
            continue
        if tree_tree is not None and tree_tree.query(c)[0] < 3.2:
            continue
        if more_food.query(c)[0] < MORE_FOOD_GAP_M:
            continue
        if any(np.hypot(*(c - q)) < FORAGE_GAP_M for q in picked):
            continue
        picked.append(c)
        if len(picked) == FORAGE_COUNT:
            break
    print(f"top-up: {len(picked)} forage spots by the door, beside her paths and in her woods")

    if dry:
        return
    write_retired(retired)
    with open(TOPUP_INC, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/compact_placements.py; do not edit by hand.\n")
        fh.write("// Compact-map top-up (shrink-estate-map): fellable trees 585000-585199 in place of scatter trees of the\n")
        fh.write("// same mesh, forage 585200-585299 beside her paths. {id, kind, {x, y} cm}. Append only.\n")
        for k, (x, y) in enumerate(chosen_xy):
            fh.write(f"topUp({TREE_IDS[0] + k}, ResourceKind::ForestTree, {x * 100:.1f}, {y * 100:.1f});\n")
        for k, c in enumerate(picked):
            kind = "BerryBush" if k % 2 == 0 else "Roots"
            fh.write(f"topUp({FORAGE_IDS[0] + k}, ResourceKind::{kind}, {c[0] * 100:.1f}, {c[1] * 100:.1f});\n")
    write_scenery(raw, rec_out)
    layout["compactPlacements"] = {"scenery": True, "treeIds": list(TREE_IDS), "forageIds": list(FORAGE_IDS)}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    print("compact placements written")


if __name__ == "__main__":
    main()
