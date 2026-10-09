"""Woodland forage and windfall spots for the estate's foraging and upkeep loops.

    python Scripts\\Terrain\\woodland_forage.py            -> HomesteadEstateWoodlandForagePlacements.inc  woodlandForage(id, ResourceKind::X, x, y)
    python Scripts\\Terrain\\woodland_forage.py --windfall -> HomesteadEstateWindfallPlacements.inc        windfall(id, x, y)

Jenny, 2026-10-04: "Need more berry bushes and roots in woods on estate. Foraging (walking around and collecting
berries, roots, branches, stones) should be its own gameplay loop alongside farming and fishing." Balance 2026-10-04
(docs/design/balance.md): +40 berry bushes in thickets of 3-5 at clearings and woodland edges, +30 root spots, +30 branch
piles, +20 stone piles. They use the existing kinds (BerryBush, Roots, Branches, Stones), so yields, timers, seasons and
saves follow the existing rules.

Woodland forage ids 583000-583199 (docs/handoff/round-4.md): bushes 583000-583039, roots 583040-583069, branch piles
583070-583099, stone piles 583100-583119. Windfall spots 584000-584199: dormant FallenBranch placements under the
trees within WINDFALL_RADIUS_M of home that the wind wakes one at a time (Simulation::UpkeepRegrowth).

Ids are save identity: once an .inc exists its rows are kept, never replanned (--replan rewrites them, only before they
ship). Everything stays off the road, footpaths, the lake and its trail, the manor's grounds, the derelict farm and the
cove, and 3.2 m from every other placement (the placement skip rule is 3 m). Deterministic (fixed seeds). Needs the
terrain work folder (HOMESTEAD_TERRAIN_WORK, see scatter.py).
"""
import argparse
import json
import math
import os
import re
import sys

import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scatter import HERE, ROOT, WORK, H, sample, densify  # noqa: E402

SIM = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
FORAGE_INC = os.path.join(SIM, "HomesteadEstateWoodlandForagePlacements.inc")
WINDFALL_INC = os.path.join(SIM, "HomesteadEstateWindfallPlacements.inc")
SCENERY = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
TRUNK_KINDS = [0, 1, 13, 14, 15, 16, 19, 20]
SEED = 20261004

BUSH_FIRST_ID, ROOT_FIRST_ID, BRANCH_FIRST_ID, STONE_FIRST_ID, FORAGE_END_ID = 583000, 583040, 583070, 583100, 583200
WINDFALL_FIRST_ID, WINDFALL_END_ID = 584000, 584200
THICKET_SIZES = [5, 5, 4, 4, 4, 4, 4, 4, 3, 3]   # 40 bushes in ten thickets of 3-5 (Balance)
ROOT_SPOTS, BRANCH_SPOTS, STONE_SPOTS = 30, 30, 20
WINDFALL_SPOTS = 180
WINDFALL_RADIUS_M = 220.0     # the same reach as Upkeep::RegrowRadiusCm: she can keep the near woods tidy

MIN_FROM_HOME_M = 60.0
MAX_FROM_HOME_M = 900.0
RIVER_CLEAR_M = 12.0
MAX_SLOPE_DEG = 22.0
MIN_Z_M = 6.0                 # off the beach and the salt-marsh edge
ROAD_CLEAR_M = 6.0
PATH_CLEAR_M = 4.0
BOUNDARY_CLEAR_M = 6.0
COVE_CLEAR_M = 60.0
MANOR_CLEAR_M = 30.0
FARM = (-222.0, -162.0, -705.0, -645.0)   # Anchor::DerelictFarm in metres, x0 x1 y0 y1
FARM_CLEAR_M = 8.0
ROW_GAP_M = 3.2               # ProvisionalEstatePlacements skips a row within 3 m of an earlier one
BUSH_TRUNK_CLEAR_M = 3.0      # a bush needs light
GROUND_TRUNK_CLEAR_M = 1.6    # reachable, not inside a trunk
THICKET_GAP_M = (3.3, 5.2)    # bush to bush inside a thicket
THICKET_SPREAD_M = 7.5
THICKET_APART_M = 45.0        # between thicket centres
FOOD_APART_M = 25.0           # a thicket centre keeps this from existing berries and roots
SPOT_APART_M = {"Roots": 30.0, "Branches": 28.0, "Stones": 40.0}
CANDIDATES = 8000
WINDFALL_APART_M = 9.0
WINDFALL_TRUNK_NEAR_M = (1.5, 6.0)   # lands just beside a trunk, under the canopy
WINDFALL_SPREAD_JITTER_CELLS = 0.0


def load_ground():
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    layout = json.load(open(os.path.join(HERE, "estate_layout.json")))
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"),
                                                         ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")]))
    trunks = rec[np.isin(rec["kind"], TRUNK_KINDS)]
    return z, slope, layout, cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)


def existing_points(skip):
    """Every earlier placement in metres, and the berry/root ones separately (kind by file or row)."""
    pts, food = [], []
    for name in sorted(os.listdir(SIM)):
        if not name.endswith("Placements.inc") or name in skip:
            continue
        for m in re.finditer(r"\w+\((\d+),\s*(?:ResourceKind::(\w+),\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)",
                             open(os.path.join(SIM, name)).read()):
            p = (float(m[3]) / 100.0, float(m[4]) / 100.0)
            pts.append(p)
            if name == "HomesteadEstateBerryPlacements.inc" or m[2] in ("BerryBush", "Roots"):
                food.append(p)
    return np.array(pts), np.array(food).reshape(-1, 2)


def candidate_ground(rng, z, slope, layout, trees, count):
    """Walkable ground inside the estate, off every path, the lake, the manor's grounds, the farm and the cove."""
    home = np.array(layout["landmarks"]["StandingRoomOrigin"][:2])
    boundary = np.array(layout["polygons"]["EstateBoundary"])
    lo, hi = boundary.min(axis=0), boundary.max(axis=0)
    p = np.c_[rng.uniform(lo[0], hi[0], count), rng.uniform(lo[1], hi[1], count)]
    d_home = np.linalg.norm(p - home, axis=1)
    p = p[(d_home > MIN_FROM_HOME_M) & (d_home < MAX_FROM_HOME_M)]
    p = p[points_in_poly(p, boundary)]
    edge = cKDTree(densify(np.vstack([boundary, boundary[:1]]).tolist(), 1.0))
    p = p[edge.query(p)[0] > BOUNDARY_CLEAR_M]
    p = p[cKDTree(densify(layout["road"], 1.0)).query(p)[0] > ROAD_CLEAR_M]
    for path in layout["footpaths"]:
        p = p[cKDTree(densify(path["points"], 1.0)).query(p)[0] > PATH_CLEAR_M]
    p = p[sample(z, p[:, 0], p[:, 1]) > MIN_Z_M]
    p = p[cKDTree(densify(layout["river"])).query(p)[0] > RIVER_CLEAR_M]
    r = np.clip(np.round(H - p[:, 0]).astype(int), 0, slope.shape[0] - 1)
    c = np.clip(np.round(H + p[:, 1]).astype(int), 0, slope.shape[1] - 1)
    p = p[slope[r, c] < MAX_SLOPE_DEG]
    lake = layout.get("lake")
    if lake:
        import lake_features
        p = p[~lake_features.cleared(lake, p[:, 0], p[:, 1])]
        p = p[np.hypot(p[:, 0] - lake["landing"][0], p[:, 1] - lake["landing"][1]) > 10.0]
        p = p[cKDTree(densify(lake["path"], 1.0)).query(p)[0] > 4.0]
    cove = np.array(layout["landmarks"]["CoveBeach"][:2], np.float64)
    p = p[np.linalg.norm(p - cove, axis=1) > COVE_CLEAR_M]
    manor = np.array(layout["polygons"]["ManorFootprint"], np.float64)
    near_manor = ((p[:, 0] > manor[:, 0].min() - MANOR_CLEAR_M) & (p[:, 0] < manor[:, 0].max() + MANOR_CLEAR_M)
                  & (p[:, 1] > manor[:, 1].min() - MANOR_CLEAR_M) & (p[:, 1] < manor[:, 1].max() + MANOR_CLEAR_M))
    p = p[~near_manor]
    near_farm = ((p[:, 0] > FARM[0] - FARM_CLEAR_M) & (p[:, 0] < FARM[1] + FARM_CLEAR_M)
                 & (p[:, 1] > FARM[2] - FARM_CLEAR_M) & (p[:, 1] < FARM[3] + FARM_CLEAR_M))
    return p[~near_farm]


def farthest_pick(rng, pool, taken, count, apart):
    """Greedy spread: each pick is the pool point furthest from everything taken (random tie-break), so the
    picks fan out across the woods instead of crowding one corner."""
    taken = [tuple(t) for t in taken]
    chosen = []
    pool = pool[rng.permutation(len(pool))[:CANDIDATES]]
    for _ in range(count):
        if not len(pool):
            break
        d = cKDTree(np.array(taken)).query(pool)[0] if taken else np.full(len(pool), 1e9)
        d = d * rng.uniform(0.8, 1.0, len(pool))
        best = int(np.argmax(d))
        if d[best] < apart:
            break
        chosen.append(pool[best])
        taken.append(tuple(pool[best]))
    return chosen


def plan_forage():
    rng = np.random.default_rng(SEED)
    z, slope, layout, trees = load_ground()
    placed, food = existing_points({os.path.basename(FORAGE_INC), os.path.basename(WINDFALL_INC)})
    placed_tree = cKDTree(placed)
    ground = candidate_ground(rng, z, slope, layout, trees, 400000)
    ground = ground[placed_tree.query(ground)[0] > ROW_GAP_M + 0.8]
    trunk_d = trees.query(ground)[0]
    near14 = np.array([len(n) for n in trees.query_ball_point(ground, 14.0)])
    near25 = np.array([len(n) for n in trees.query_ball_point(ground, 25.0)])
    near10 = np.array([len(n) for n in trees.query_ball_point(ground, 10.0)])
    # Thicket centres: woodland edges (light, trees close behind) and clearings (open, ringed by wood).
    edge = (trunk_d > BUSH_TRUNK_CLEAR_M + 1.0) & (near14 >= 4)
    clearing = (trunk_d > 6.0) & (near25 >= 6)
    centres_pool = ground[edge | clearing]
    taken = [tuple(f) for f in food]
    centres = []
    for size in THICKET_SIZES:
        pick = farthest_pick(rng, centres_pool, taken, 1, FOOD_APART_M)
        if not pick:
            raise SystemExit("no room for another thicket")
        centres.append((pick[0], size))
        taken.append(tuple(pick[0]))
        keep = cKDTree(np.array([c for c, _ in centres])).query(centres_pool)[0] > THICKET_APART_M
        centres_pool = centres_pool[keep]

    rows, used = [], [tuple(p) for p in []]
    own = []

    def free(p, gap=ROW_GAP_M):
        return all(math.hypot(p[0] - q[0], p[1] - q[1]) >= gap for q in own)

    bush_ground = ground[trunk_d > BUSH_TRUNK_CLEAR_M]
    bush_tree = cKDTree(bush_ground)
    bushes = []
    for centre, size in centres:
        near = bush_ground[bush_tree.query_ball_point(centre, THICKET_SPREAD_M)]
        near = near[rng.permutation(len(near))]
        got = 0
        for p in near:
            if got >= size:
                break
            if not free(p, rng.uniform(*THICKET_GAP_M)):
                continue
            own.append(tuple(p))
            bushes.append(p)
            got += 1
        if got < size:
            raise SystemExit(f"thicket at {centre} only fits {got} of {size} bushes")
    rows += [("BerryBush", BUSH_FIRST_ID + i, p) for i, p in enumerate(bushes)]

    def spots(kind, pool, count, first_id):
        sub = np.array(own).reshape(-1, 2)
        chosen = farthest_pick(rng, pool, [tuple(t) for t in sub] + [tuple(f) for f in food],
                               count, SPOT_APART_M[kind])
        if len(chosen) < count:
            raise SystemExit(f"only {len(chosen)} of {count} {kind} spots fit")
        for p in chosen:
            own.append(tuple(p))
        return [(kind, first_id + i, p) for i, p in enumerate(chosen)]

    under = ground[(trunk_d > GROUND_TRUNK_CLEAR_M) & (near10 >= 2)]
    rows += spots("Roots", under, ROOT_SPOTS, ROOT_FIRST_ID)
    rows += spots("Branches", ground[(trunk_d > GROUND_TRUNK_CLEAR_M) & (near10 >= 3)], BRANCH_SPOTS, BRANCH_FIRST_ID)
    r = np.clip(np.round(H - ground[:, 0]).astype(int), 0, slope.shape[0] - 1)
    c = np.clip(np.round(H + ground[:, 1]).astype(int), 0, slope.shape[1] - 1)
    stony = ground[(slope[r, c] >= 6.0) & (trunk_d > GROUND_TRUNK_CLEAR_M)]   # on the slopes, where stone shows
    rows += spots("Stones", stony, STONE_SPOTS, STONE_FIRST_ID)
    return rows


def plan_windfall():
    rng = np.random.default_rng(SEED + 1)
    z, slope, layout, trees = load_ground()
    home = np.array(layout["landmarks"]["StandingRoomOrigin"][:2])
    placed, _ = existing_points({os.path.basename(WINDFALL_INC)})
    placed_tree = cKDTree(placed)
    raw = candidate_ground(rng, z, slope, layout, trees, 600000)
    raw = raw[np.linalg.norm(raw - home, axis=1) < WINDFALL_RADIUS_M]
    raw = raw[placed_tree.query(raw)[0] > ROW_GAP_M + 0.8]
    trunk_d = trees.query(raw)[0]
    under = raw[(trunk_d > WINDFALL_TRUNK_NEAR_M[0]) & (trunk_d < WINDFALL_TRUNK_NEAR_M[1])]
    under = under[rng.permutation(len(under))]
    chosen = []
    for p in under:
        if len(chosen) >= WINDFALL_SPOTS:
            break
        if chosen and np.min(np.linalg.norm(np.array(chosen) - p, axis=1)) < WINDFALL_APART_M:
            continue
        chosen.append(p)
    if len(chosen) < WINDFALL_SPOTS:
        print(f"note: only {len(chosen)} windfall spots fit")
    return [("FallenBranch", WINDFALL_FIRST_ID + i, p) for i, p in enumerate(chosen)]


def write(path, macro, header, rows, with_kind):
    lines = header
    for kind, pid, p in rows:
        if with_kind:
            lines.append(f"{macro}({pid}, ResourceKind::{kind}, {p[0] * 100.0:.1f}, {p[1] * 100.0:.1f});")
        else:
            lines.append(f"{macro}({pid}, {p[0] * 100.0:.1f}, {p[1] * 100.0:.1f});")
    with open(path, "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--windfall", action="store_true", help="plan the windfall spots instead of the forage")
    parser.add_argument("--replan", action="store_true", help="rewrite committed rows (ids are save identity: only before they ship)")
    args = parser.parse_args()
    path = WINDFALL_INC if args.windfall else FORAGE_INC
    if os.path.exists(path) and not args.replan:
        raise SystemExit(f"{path} exists; its ids are save identity. Use --replan only before it ships.")
    if args.windfall:
        rows = plan_windfall()
        write(path, "windfall", ["// Generated by Scripts/Terrain/woodland_forage.py --windfall; do not edit by hand.",
                                 "// Windfall spots (584000-584199): {id, {x, y} cm}. Dormant FallenBranch placements the wind wakes."],
              rows, False)
    else:
        rows = plan_forage()
        write(path, "woodlandForage", ["// Generated by Scripts/Terrain/woodland_forage.py; do not edit by hand.",
                                       "// Woodland forage (583000-583199): {id, kind, {x, y} cm}. Ids are save identity: append only."],
              rows, True)
    counts = {}
    for kind, _, _ in rows:
        counts[kind] = counts.get(kind, 0) + 1
    print(f"{os.path.basename(path)}: {len(rows)} rows {counts}")


if __name__ == "__main__":
    main()
