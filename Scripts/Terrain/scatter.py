"""Scatter the Estate map's decorative scenery and bake the world lane's interactive placements.

Outputs (both committed):
  Content/SurvivalGame/Estate/Runtime/EstateScenery.bin   decorative instances read by AHomesteadWorld
  Source/SurvivalGame/Simulation/HomesteadEstateWorldPlacements.inc   world-lane EstatePlacement rows (500100+)

Deterministic (fixed seed). Kind bytes must match EstateSceneryKinds in HomesteadWorld.cpp.
"""
import json, os, struct
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
H = 2016
BROADLEAF, FIR, HAZEL, BRACKEN, YARROW, COBBLES, BOULDER, ERRATIC, DOME, FERN, GRASS_TALL, GRASS_MID, SHRUB = range(13)

def weights():
    out = {}
    for n in ["Pasture", "WoodlandFloor", "Moorland", "DuneSand", "Beach", "CliffRock", "DirtRoad"]:
        a = np.asarray(Image.open(os.path.join(WORK, "weights", f"{n}.png"))).astype(np.float32) / 255
        out[n] = a.T[::-1, :]            # back to [row = north..south, col = west..east]
    return out

def sample(grid, x, y):
    r = np.clip(np.round(H - x).astype(int), 0, grid.shape[0] - 1)
    c = np.clip(np.round(H + y).astype(int), 0, grid.shape[1] - 1)
    return grid[r, c]

def densify(p, step=2.0):
    p = np.asarray(p, float)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]

def main():
    rng = np.random.default_rng(1851)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    W = weights()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    lm = {k: np.array(v[:2]) for k, v in L["landmarks"].items()}
    road = cKDTree(densify(L["road"]))
    river = cKDTree(densify(L["river"]))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    town = lm["TownSquare"]

    def candidates(per_m2, area=(-2000, 2000, -2000, 2000)):
        x0, x1, y0, y1 = area
        n = rng.poisson(per_m2 * (x1 - x0) * (y1 - y0))
        return np.c_[rng.uniform(x0, x1, n), rng.uniform(y0, y1, n)]

    def keep_common(p, road_clear=6.0, river_clear=5.0):
        ok = sample(z, p[:, 0], p[:, 1]) > 1.5
        ok &= road.query(p)[0] > road_clear
        ok &= river.query(p)[0] > river_clear
        ok &= np.linalg.norm(p - town, axis=1) > 180
        ok &= np.linalg.norm(p - lm["StandingRoomOrigin"], axis=1) > 28
        ok &= np.linalg.norm(p - lm["MineEntrance"], axis=1) > 20
        ok &= np.linalg.norm(p - lm["MillSite"], axis=1) > 14
        return p[ok]

    def by_weight(p, layer, lo=0.35, jitter=True):
        w = sample(W[layer], p[:, 0], p[:, 1])
        if jitter:
            return p[rng.random(len(p)) < np.clip((w - lo) / (1 - lo), 0, 1)]
        return p[w > lo]

    # ---- interactive placements (world lane, 500100+) -------------------------------------
    rows = []
    next_id = [500100]
    inside = lambda p: points_in_poly(p, boundary) & ~points_in_poly(p, manor)

    def place(kind, pts, limit, min_gap, taken):
        chosen = []
        for pt in pts:
            if len(chosen) >= limit: break
            if taken and min(np.linalg.norm(np.array(taken) - pt, axis=1)) < min_gap: continue
            chosen.append(pt); taken.append(pt)
        for pt in chosen:
            rows.append((next_id[0], kind, pt))
            next_id[0] += 1
        return chosen

    taken = [lm["StandingRoomOrigin"].tolist()]
    wood = keep_common(candidates(1 / 60.0, (-800, 200, -1200, 0)), 7, 7)
    wood = wood[inside(wood)]
    wood_trees = by_weight(wood, "WoodlandFloor", 0.4, False)
    interactive_trees = place("ForestTree", wood_trees, 180, 7.0, taken)
    near = keep_common(candidates(1 / 25.0, (-450, 0, -950, -300)), 3, 4)
    near = near[inside(near)]
    near = near[np.argsort(np.linalg.norm(near - lm["StandingRoomOrigin"], axis=1))]
    in_wood = sample(W["WoodlandFloor"], near[:, 0], near[:, 1]) > 0.3
    place("Branches", np.r_[near[in_wood][:400], near[~in_wood][:200]], 45, 6, taken)
    place("Stones", near[slope[np.clip(np.round(H - near[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + near[:, 1]).astype(int), 0, 4032)] > 8], 30, 8, taken)
    place("Flowers", near[~in_wood], 35, 7, taken)
    place("BerryBush", near[in_wood][40:], 22, 9, taken)
    place("Roots", near[in_wood][80:], 22, 8, taken)
    reeds = densify(L["river"], 12.0)
    reeds = reeds[inside(reeds)] + rng.normal(0, 1.0, (inside(reeds).sum(), 2)) + np.array([0, 4.2])
    place("Reeds", reeds, 18, 10, taken)
    tree_tree = cKDTree(np.array(interactive_trees)) if interactive_trees else None

    # ---- decorative scenery -------------------------------------------------------------
    recs = []
    def emit(kind, p, smin, smax):
        for x, y in p:
            recs.append((kind, x, y, rng.uniform(0, 360), rng.uniform(smin, smax)))
    def clear_of_interactive(p, gap):
        pts = np.array(taken)
        return p[cKDTree(pts).query(p)[0] > gap] if len(p) else p

    all_wood = keep_common(candidates(1 / 45.0), 7, 6)
    wt = by_weight(all_wood, "WoodlandFloor", 0.3)
    wt = clear_of_interactive(wt, 6.5)
    firs = rng.random(len(wt)) < 0.12
    emit(BROADLEAF, wt[~firs], 0.85, 1.25)
    emit(FIR, wt[firs], 0.8, 1.15)
    under = keep_common(candidates(1 / 22.0, (-1000, 700, -1500, 700)), 3, 3)
    under = clear_of_interactive(by_weight(under, "WoodlandFloor", 0.3), 2.0)
    pick = rng.random(len(under))
    emit(HAZEL, under[pick < 0.18], 0.8, 1.15)
    emit(BRACKEN, under[(pick >= 0.18) & (pick < 0.55)], 0.75, 1.25)
    emit(FERN, under[(pick >= 0.55) & (pick < 0.85)], 0.8, 1.3)
    emit(SHRUB, under[pick >= 0.85], 0.8, 1.2)
    # Lone field trees and hedgerow shrubs on the pasture.
    field = clear_of_interactive(by_weight(keep_common(candidates(1 / 2500.0)), "Pasture", 0.6), 10)
    emit(BROADLEAF, field, 1.0, 1.35)
    # Grass tufts and yarrow close to where she walks: round the manor and along the road.
    close = candidates(1 / 5.0, (-420, -80, -900, -400))
    close = np.r_[close, densify(L["road"], 1.0)[::1] + rng.normal(0, 9, (len(densify(L["road"], 1.0)), 2))]
    close = keep_common(close, 3.2, 3.5)
    close = by_weight(close, "Pasture", 0.4)
    close = clear_of_interactive(close, 1.2)
    pick = rng.random(len(close))
    emit(GRASS_TALL, close[pick < 0.45], 0.8, 1.25)
    emit(GRASS_MID, close[(pick >= 0.45) & (pick < 0.9)], 0.8, 1.3)
    emit(YARROW, close[pick >= 0.9], 0.8, 1.3)
    # Moor: bracken, boulders, erratics and a few tors on the high ground.
    moor = keep_common(candidates(1 / 70.0))
    moor = by_weight(moor, "Moorland", 0.4)
    pick = rng.random(len(moor))
    emit(BRACKEN, moor[pick < 0.7], 0.8, 1.3)
    emit(BOULDER, moor[(pick >= 0.7) & (pick < 0.95)], 0.8, 1.4)
    emit(ERRATIC, moor[pick >= 0.985], 0.85, 1.2)
    high = np.argwhere(z > 170)
    if len(high):
        for r, c in high[rng.choice(len(high), size=min(5, len(high)), replace=False)]:
            recs.append((DOME, float(H - r), float(c - H), rng.uniform(0, 360), rng.uniform(0.9, 1.05)))
    # Cliffs and steep valley sides: rubble.
    steep = keep_common(candidates(1 / 90.0))
    sv = slope[np.clip(np.round(H - steep[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + steep[:, 1]).astype(int), 0, 4032)]
    steep = steep[(sv > 22) & (sv < 45)]
    pick = rng.random(len(steep))
    emit(COBBLES, steep[pick < 0.6], 0.8, 1.3)
    emit(BOULDER, steep[pick >= 0.6], 0.8, 1.3)

    # ---- write -------------------------------------------------------------------------
    runtime = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime")
    with open(os.path.join(runtime, "EstateScenery.bin"), "wb") as fh:
        fh.write(b"HSC1" + struct.pack("<I", len(recs)))
        for k, x, y, yaw, s in recs:
            fh.write(struct.pack("<B3x4f", k, x * 100.0, y * 100.0, yaw, s))
    inc = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateWorldPlacements.inc")
    with open(inc, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/scatter.py; do not edit by hand.\n")
        fh.write("// World-lane interactive placements: {id, kind, {x, y} cm}.\n")
        for pid, kind, (x, y) in rows:
            fh.write(f"world({pid}, ResourceKind::{kind}, {x * 100.0:.1f}, {y * 100.0:.1f});\n")
    counts = {}
    for k, *_ in recs: counts[k] = counts.get(k, 0) + 1
    kinds = {}
    for _, k, _p in rows: kinds[k] = kinds.get(k, 0) + 1
    print("scenery", len(recs), counts)
    print("placements", len(rows), kinds)

if __name__ == "__main__":
    main()
