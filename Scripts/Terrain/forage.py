"""Bake more pickable forage: brambles in the estate's woods and fields beyond the manor's grounds
(ids 582100+), and forage stops along the public road to town (ids 581000-581099).

    python Scripts\\Terrain\\forage.py
        -> Source/SurvivalGame/Simulation/HomesteadEstateForagePlacements.inc   forage(id, ResourceKind::X, x, y)
        -> Source/SurvivalGame/Simulation/HomesteadEstateRoadsidePlacements.inc roadside(id, ResourceKind::X, x, y)

Jenny, 2026-09-29: "no berry bushes anywhere, even in the woods". berries.py puts its 42 brambles round the
manor, the drive and the woods within 450 m of home; the woodland lane's 43 are inside the MVP wood. This
adds brambles where she goes next: woodland edges and field hedges further out, and the road beyond the
gateway, where there was no pickable forage at all past chainage 800 m.

Rows land in ProvisionalEstatePlacements after every other section, and a row within 3 m of an earlier
placement is skipped (its id stays unused), so nothing existing moves or renumbers. The roadside rows sit
3.2-6 m off the road's centreline, outside the bridge keep-out: Homestead::IsPublicRoadsidePlacement
lets exactly those (ids 581000-581099, forage kinds) stand off the estate.
Deterministic (fixed seed). Needs the terrain work folder (HOMESTEAD_TERRAIN_WORK, see scatter.py).

Ids are save identity (a picked bush is remembered by its id): every committed row is kept verbatim and
new picks get fresh ids in their own range (forage_ids.py). HOMESTEAD_FORAGE_OUT=<dir> writes the result
there instead of over the committed files, to compare a rebake before committing it.
"""
import json
import os
import re

import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

from scatter import HERE, ROOT, WORK, H, sample, densify

ESTATE_FIRST_ID, ESTATE_END_ID = 582100, 582300       # registry: 582100-582299 (Water)
ROADSIDE_FIRST_ID, ROADSIDE_END_ID = 581000, 581100   # registry: 581000-581099 (Water, public roadside)
# Ids taken out of service (terrain moved under them): their rows are dropped and the ids never reused.
RETIRED_ESTATE_IDS = frozenset()
RETIRED_ROADSIDE_IDS = frozenset()
SEED = 20260929

# Estate brambles: open ground (no trunk within 4 m), gentle slope, well out from the manor.
ESTATE_MIN_FROM_HOME_M = 150.0
ESTATE_MAX_FROM_HOME_M = 900.0
ESTATE_GAP_M = 14.0               # between brambles (new and old)
ESTATE_WOOD_EDGE = 18             # woodland edges: 4+ trunks within 14 m
ESTATE_FIELD_HEDGE = 10           # field hedges: open pasture with a tree or two within 20 m
# Wild roots (a cooked meal) in the woods round the manor: the MVP wood's are 220 m and more away.
ROOTS_NEAR = 3                    # 40-110 m from the standing room, for her first day
ROOTS_WOODS = 13                  # 110-350 m, in the woods
ROOTS_GAP_M = 14.0

# Roadside: a stop every ROAD_SPACING_M from ROAD_FROM_M to ROAD_TO_M, alternating verges.
ROAD_FROM_M, ROAD_TO_M, ROAD_SPACING_M = 830.0, 1900.0, 63.0
ROAD_OFFSET_M = (3.4, 5.6)        # off the centreline (the bed is 2.8 m half wide)
BRIDGE_CHAINAGE_M, BRIDGE_HALF_M = 684.0, 25.0
RIVER_CLEAR_M = 12.0
MAX_SLOPE_DEG = 22.0


def existing_points():
    sim = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
    pts, berries, kinds = [], [], []
    for name in os.listdir(sim):
        if not name.endswith("Placements.inc") or name in ("HomesteadEstateForagePlacements.inc",
                                                            "HomesteadEstateRoadsidePlacements.inc"):
            continue
        text = open(os.path.join(sim, name)).read()
        for m in re.finditer(r"\w+\((\d+),\s*(?:ResourceKind::(\w+),\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)", text):
            p = (float(m[3]) / 100.0, float(m[4]) / 100.0)
            pts.append(p)
            kinds.append(m[2] or "BerryBush")
            if name == "HomesteadEstateBerryPlacements.inc" or m[2] == "BerryBush":
                berries.append(p)
    return np.array(pts), np.array(berries), kinds


def main():
    rng = np.random.default_rng(SEED)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    home = np.array(L["landmarks"]["StandingRoomOrigin"][:2])
    boundary = np.array(L["polygons"]["EstateBoundary"])
    road = np.asarray(L["road"], np.float64)
    road_line = densify(L["road"], 1.0)
    road_tree = cKDTree(road_line)
    river = cKDTree(densify(L["river"]))
    lake = L.get("lake")
    placed, old_berries, placed_kinds = existing_points()
    placed_tree = cKDTree(placed)
    raw = open(os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"),
                                                         ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")]))
    trunks = rec[np.isin(rec["kind"], [0, 1, 13, 14, 15, 16, 19, 20])]
    trees = cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)
    mvp = json.load(open(os.path.join(HERE, "mvp_woodland.json")))["polygon"] if os.path.exists(os.path.join(HERE, "mvp_woodland.json")) else None

    def slope_at(p):
        r = np.clip(np.round(H - p[:, 0]).astype(int), 0, slope.shape[0] - 1)
        c = np.clip(np.round(H + p[:, 1]).astype(int), 0, slope.shape[1] - 1)
        return slope[r, c]

    def common(p):
        keep = sample(z, p[:, 0], p[:, 1]) > 1.5
        keep &= river.query(p)[0] > RIVER_CLEAR_M
        keep &= slope_at(p) < MAX_SLOPE_DEG
        keep &= placed_tree.query(p)[0] > 4.0
        if lake:
            import lake_features
            keep &= ~lake_features.cleared(lake, p[:, 0], p[:, 1])
            keep &= np.hypot(p[:, 0] - lake["landing"][0], p[:, 1] - lake["landing"][1]) > 10.0
        return keep

    chosen = []
    berry_tree = cKDTree(old_berries) if len(old_berries) else None

    def take(points, limit, gap):
        n = 0
        for pt in points:
            if n >= limit:
                break
            if berry_tree is not None and berry_tree.query(pt)[0] < gap:
                continue
            if chosen and np.min(np.linalg.norm(np.array([c[1] for c in chosen]) - pt, axis=1)) < gap:
                continue
            chosen.append(("BerryBush", pt))
            n += 1
        return n

    # --- The estate: woodland edges and field hedges, 150-900 m out, spread by a random walk outward.
    cand = np.c_[rng.uniform(-ESTATE_MAX_FROM_HOME_M, ESTATE_MAX_FROM_HOME_M, 120000),
                 rng.uniform(-ESTATE_MAX_FROM_HOME_M, ESTATE_MAX_FROM_HOME_M, 120000)] + home
    d_home = np.linalg.norm(cand - home, axis=1)
    cand = cand[(d_home > ESTATE_MIN_FROM_HOME_M) & (d_home < ESTATE_MAX_FROM_HOME_M)]
    cand = cand[points_in_poly(cand, boundary)]
    cand = cand[road_tree.query(cand)[0] > 6.0]
    cand = cand[common(cand)]
    cand = cand[trees.query(cand)[0] > 4.0]                       # brambles need light
    near14 = np.array([len(n) for n in trees.query_ball_point(cand, 14.0)])
    near20 = np.array([len(n) for n in trees.query_ball_point(cand, 20.0)])
    wood_edge = cand[near14 >= 4]
    hedge = cand[(near14 <= 1) & (near20 >= 1)]
    order = lambda p: p[rng.permutation(len(p))]
    got_wood = take(order(wood_edge), ESTATE_WOOD_EDGE, ESTATE_GAP_M)
    got_hedge = take(order(hedge), ESTATE_FIELD_HEDGE, ESTATE_GAP_M)
    estate = list(chosen)

    # --- Wild roots under the trees round the manor (after the brambles, so their ids and draws stay put).
    roots = []
    old_roots = np.array([p for p, k in zip(placed, placed_kinds) if k == "Roots"]) if len(placed) else np.zeros((0, 2))
    root_tree = cKDTree(old_roots) if len(old_roots) else None
    manor = np.array(L["polygons"]["ManorFootprint"], np.float64)
    # Their own random stream, so adding them moved none of the brambles or roadside stops.
    root_rng = np.random.default_rng(SEED + 1)
    rc = np.c_[root_rng.uniform(-360, 360, 60000), root_rng.uniform(-360, 360, 60000)] + home
    d_rc = np.linalg.norm(rc - home, axis=1)
    rc = rc[(d_rc > 40.0) & (d_rc < 350.0)]
    rc = rc[points_in_poly(rc, boundary)]
    rc = rc[road_tree.query(rc)[0] > 5.0]
    rc = rc[common(rc)]
    rc = rc[trees.query(rc)[0] > 1.6]                            # reachable, not inside a trunk
    under = np.array([len(n) for n in trees.query_ball_point(rc, 10.0)])
    rc = rc[under >= 2]                                            # under the trees
    grown = np.c_[np.clip(rc[:, 0], manor[:, 0].min() - 30, manor[:, 0].max() + 30),
                  np.clip(rc[:, 1], manor[:, 1].min() - 30, manor[:, 1].max() + 30)]
    rc = rc[~(grown == rc).all(axis=1)]                           # clear of the ruin's grounds by 30 m
    if mvp is not None:
        rc = rc[~points_in_poly(rc, np.asarray(mvp, np.float64))]
    rc = rc[root_rng.permutation(len(rc))]
    d_rc = np.linalg.norm(rc - home, axis=1)
    for pool, limit in ((rc[d_rc < 110.0], ROOTS_NEAR), (rc[d_rc >= 110.0], ROOTS_WOODS)):
        n = 0
        for pt in pool:
            if n >= limit:
                break
            if root_tree is not None and root_tree.query(pt)[0] < ROOTS_GAP_M:
                continue
            others = [q for _, q in estate + roots]
            if others and np.min(np.linalg.norm(np.array(others) - pt, axis=1)) < ROOTS_GAP_M:
                continue
            roots.append(("Roots", pt))
            n += 1
    estate += roots

    # --- The road to town: a stop every ~63 m beyond the gateway, alternating verges.
    seg = np.hypot(*np.diff(road, axis=0).T)
    chain = np.r_[0.0, np.cumsum(seg)]
    roadside = []
    kinds = ["BerryBush", "BerryBush", "Flowers", "BerryBush", "Roots"]
    for i, ch in enumerate(np.arange(ROAD_FROM_M, ROAD_TO_M, ROAD_SPACING_M)):
        if abs(ch - BRIDGE_CHAINAGE_M) < BRIDGE_HALF_M:
            continue
        k = int(np.clip(np.searchsorted(chain, ch), 1, len(road) - 2))
        d = road[k + 1] - road[k - 1]
        d /= np.linalg.norm(d)
        normal = np.array([-d[1], d[0]])
        base = road[k - 1] + (road[k] - road[k - 1]) * np.clip((ch - chain[k - 1]) / max(seg[k - 1], 1e-9), 0, 1)
        placed_here = False
        for attempt in range(12):
            side = (1.0 if i % 2 == 0 else -1.0) * (1.0 if attempt < 6 else -1.0)
            p = base + normal * side * rng.uniform(*ROAD_OFFSET_M) + d * rng.uniform(-6.0, 6.0)
            pp = p[None, :]
            if not common(pp)[0]:
                continue
            off = road_tree.query(p)[0]
            if off < ROAD_OFFSET_M[0] - 0.1 or off > 6.0:
                continue
            if roadside and min(np.linalg.norm(np.array([r[2] for r in roadside]) - p, axis=1)) < 20.0:
                continue
            roadside.append((i, kinds[i % len(kinds)], p))
            placed_here = True
            break
        if not placed_here:
            print(f"roadside: no safe verge at chainage {ch:.0f} m")

    # Ids are save identity: keep every committed row, add only genuinely new picks with fresh ids
    # (forage_ids.py). Each output keeps to its own reserved range.
    import forage_ids
    sim = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
    out_dir = os.environ.get("HOMESTEAD_FORAGE_OUT", sim)
    estate_path = os.path.join(sim, "HomesteadEstateForagePlacements.inc")
    road_path = os.path.join(sim, "HomesteadEstateRoadsidePlacements.inc")
    estate_header, estate_rows = forage_ids.read_rows(estate_path, "forage", ESTATE_FIRST_ID, ESTATE_END_ID)
    road_header, road_rows = forage_ids.read_rows(road_path, "roadside", ROADSIDE_FIRST_ID, ROADSIDE_END_ID)
    targets = {"BerryBush": ESTATE_WOOD_EDGE + ESTATE_FIELD_HEDGE, "Roots": ROOTS_NEAR + ROOTS_WOODS}
    picks = [(kind, x * 100.0, y * 100.0) for kind, (x, y) in estate]
    estate_rows, estate_added = forage_ids.allocate_pool(estate_rows, picks, targets, "forage", ESTATE_FIRST_ID,
                                                         ESTATE_END_ID, RETIRED_ESTATE_IDS, ESTATE_GAP_M * 100.0)

    def slot_of(row):
        k = int(np.argmin(np.hypot(road[:, 0] - row.x / 100.0, road[:, 1] - row.y / 100.0)))
        return int(round((chain[k] - ROAD_FROM_M) / ROAD_SPACING_M))

    road_picks = {slot: (kind, p[0] * 100.0, p[1] * 100.0) for slot, kind, p in roadside}
    road_rows, road_added = forage_ids.allocate_slots(road_rows, slot_of, road_picks, "roadside", ROADSIDE_FIRST_ID,
                                                      ROADSIDE_END_ID, RETIRED_ROADSIDE_IDS)
    os.makedirs(out_dir, exist_ok=True)
    forage_ids.write_rows(os.path.join(out_dir, "HomesteadEstateForagePlacements.inc"), estate_header, estate_rows)
    forage_ids.write_rows(os.path.join(out_dir, "HomesteadEstateRoadsidePlacements.inc"), road_header, road_rows)
    print(f"estate: {len(estate_rows)} rows ({len(estate_added)} new: {[r.id for r in estate_added]}); "
          f"roadside: {len(road_rows)} rows ({len(road_added)} new: {[r.id for r in road_added]}) -> {out_dir}")

if __name__ == "__main__":
    main()
