"""Bake more pickable forage: brambles in the estate's woods and fields beyond the manor's grounds
(ids 582100+), and forage stops along the public road to town (ids 581000-581099).

    python Scripts\\Terrain\\forage.py
        -> Source/SurvivalGame/Simulation/HomesteadEstateForagePlacements.inc   forage(id, ResourceKind::X, x, y)
        -> Source/SurvivalGame/Simulation/HomesteadEstateRoadsidePlacements.inc roadside(id, ResourceKind::X, x, y)

Jenny, 2026-09-29: "no berry bushes anywhere, even in the woods". berries.py puts its 42 brambles round the
manor, the drive and the woods within 450 m of home; the woodland lane's 43 are inside the MVP wood. This
adds brambles where she goes next: woodland edges and field hedges further out, and the road beyond the
gateway, where there was no pickable forage at all past chainage 800 m.

Later the same day ("abundant live berry bushes"): a second pass on its own random streams adds MORE_BRAMBLES
brambles and MORE_ROOTS root patches across the whole estate, each where the nearest food is furthest away
(woods, hedges and field scrub; roots under the trees; clear of the ruin, the farm, the lake trail and the
beach), and a second roadside stop half way between each pair of the first. Earlier rows and ids never move.
It also writes Tests/Data/HomesteadForageKeepOuts.inc (river, lake shore, lake trail) for the native tests.

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
# More food everywhere (Jenny's playtest, 2026-09-29: "abundant live berry bushes"): brambles on woodland
# edges, hedges and field scrub and roots under the trees across the whole estate, each placed where food
# is furthest away (so the gaps fill first), after every earlier pick so none of those moves.
MORE_BRAMBLES = 48
MORE_ROOTS = 24
MORE_GAP_M = 20.0                 # from any other bramble or root patch
MORE_COVER_M = 80.0               # a bramble or root patch "feeds" the ground within this
MORE_CANDIDATES = 6000            # candidate spots scored per pass (a random sample of the open ground)
MORE_JITTER_CELLS = 12.0          # random tie-break (in 10 m cells) so the fill looks natural, not a lattice
MORE_MIN_Z_M = 6.0                # off the beach and the salt-marsh edge
MORE_MANOR_CLEAR_M = 30.0         # the ruin's grounds stay as they are
MORE_FARM_CLEAR_M = 8.0           # outside the derelict farm's walls and her garden plots in it
COVE_CLEAR_M = 60.0
MORE_BOUNDARY_CLEAR_M = 6.0       # inside the estate's edge, not on its line
# Matches Anchor::DerelictFarm (HomesteadEstate.cpp) in metres, x0 x1 y0 y1.
FARM = (-222.0, -162.0, -705.0, -645.0)

# Roadside: a stop every ROAD_SPACING_M from ROAD_FROM_M to ROAD_TO_M, alternating verges.
ROAD_FROM_M, ROAD_TO_M, ROAD_SPACING_M = 830.0, 1900.0, 63.0
# ...and a second stop half way between each pair (their own slots, odd on the half-spacing grid).
ROAD_INFILL_KINDS = ["BerryBush", "BerryBush", "Roots"]
ROAD_MIN_APART_M = 20.0
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

    # --- More food across the whole estate (own random stream, after every earlier pick): each new bramble
    # or root patch goes where the nearest food is furthest away, so the empty woods and fields fill first.
    import forage_ids
    sim = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
    committed_food = []
    for name, macro, first, end in (("HomesteadEstateForagePlacements.inc", "forage", ESTATE_FIRST_ID, ESTATE_END_ID),
                                    ("HomesteadEstateRoadsidePlacements.inc", "roadside", ROADSIDE_FIRST_ID, ROADSIDE_END_ID)):
        committed_food += [(r.x / 100.0, r.y / 100.0) for r in forage_ids.read_rows(os.path.join(sim, name), macro, first, end)[1]
                           if r.kind in ("BerryBush", "Roots")]
    food = np.array([p for p, k in zip(placed, placed_kinds) if k in ("BerryBush", "Roots")] + committed_food,
                    np.float64).reshape(-1, 2)
    more_rng = np.random.default_rng(SEED + 2)
    lo, hi = boundary.min(axis=0), boundary.max(axis=0)
    mc = np.c_[more_rng.uniform(lo[0], hi[0], 200000), more_rng.uniform(lo[1], hi[1], 200000)]
    mc = mc[points_in_poly(mc, boundary)]
    mc = mc[cKDTree(densify(np.vstack([boundary, boundary[:1]]).tolist(), 1.0)).query(mc)[0] > MORE_BOUNDARY_CLEAR_M]
    mc = mc[road_tree.query(mc)[0] > 6.0]
    mc = mc[sample(z, mc[:, 0], mc[:, 1]) > MORE_MIN_Z_M]
    mc = mc[common(mc)]
    cove = np.array(L["landmarks"]["CoveBeach"][:2], np.float64)
    mc = mc[np.linalg.norm(mc - cove, axis=1) > COVE_CLEAR_M]

    def in_box(p, x0, x1, y0, y1, margin):
        return (p[:, 0] > x0 - margin) & (p[:, 0] < x1 + margin) & (p[:, 1] > y0 - margin) & (p[:, 1] < y1 + margin)

    mc = mc[~in_box(mc, manor[:, 0].min(), manor[:, 0].max(), manor[:, 1].min(), manor[:, 1].max(), MORE_MANOR_CLEAR_M)]
    mc = mc[~in_box(mc, *FARM, MORE_FARM_CLEAR_M)]
    if lake:
        mc = mc[cKDTree(densify(lake["path"], 1.0)).query(mc)[0] > 4.0]   # off the lake trail
    trunk_d = trees.query(mc)[0]
    near10 = np.array([len(n) for n in trees.query_ball_point(mc, 10.0)])
    near30 = np.array([len(n) for n in trees.query_ball_point(mc, 30.0)])
    bramble_ground = mc[(trunk_d > 4.0) & (near30 >= 1)]          # woodland edge, hedge or field scrub, in light
    root_ground = mc[(trunk_d > 1.6) & (near10 >= 2)]             # under the trees

    # Land cells (10 m) on the estate, for coverage: a cell is fed when food is within MORE_COVER_M.
    gx, gy = np.meshgrid(np.arange(lo[0], hi[0], 10.0), np.arange(lo[1], hi[1], 10.0), indexing="ij")
    cells = np.c_[gx.ravel(), gy.ravel()]
    cells = cells[points_in_poly(cells, boundary)]
    cells = cells[sample(z, cells[:, 0], cells[:, 1]) > 1.5]
    cell_tree = cKDTree(cells)

    def spread(pool, count, kind):
        """Greedy coverage: each pick is the candidate that feeds the most still-hungry land cells, so the
        empty woods and fields fill first without the picks crowding the estate's edge."""
        nonlocal food
        pool = pool[more_rng.permutation(len(pool))[:MORE_CANDIDATES]]
        hungry = cKDTree(food).query(cells)[0] > MORE_COVER_M
        reach = cell_tree.query_ball_point(pool, MORE_COVER_M)
        jitter = more_rng.uniform(0.0, MORE_JITTER_CELLS, len(pool))
        gap = cKDTree(food).query(pool)[0]
        out = []
        for _ in range(count):
            score = np.array([hungry[r].sum() for r in reach], np.float64) + jitter
            score[gap < MORE_GAP_M] = -1.0
            i = int(np.argmax(score))
            if score[i] < 0.0:
                break
            out.append((kind, pool[i].copy()))
            food = np.vstack([food, pool[i]])
            hungry[reach[i]] = False
            gap = np.minimum(gap, np.linalg.norm(pool - pool[i], axis=1))
        return out

    more_brambles = spread(bramble_ground, MORE_BRAMBLES, "BerryBush")
    more_roots = spread(root_ground, MORE_ROOTS, "Roots")
    print(f"more food: {len(more_brambles)} brambles, {len(more_roots)} root patches")

    # --- The road to town: a stop every ~63 m beyond the gateway, alternating verges, then a second stop
    # half way between each pair (own random stream). Slots count half-spacings: the first set even, infill odd.
    seg = np.hypot(*np.diff(road, axis=0).T)
    chain = np.r_[0.0, np.cumsum(seg)]
    roadside = []

    def verge(slot, ch, first_side, kind, draw):
        if abs(ch - BRIDGE_CHAINAGE_M) < BRIDGE_HALF_M:
            return
        k = int(np.clip(np.searchsorted(chain, ch), 1, len(road) - 2))
        d = road[k + 1] - road[k - 1]
        d /= np.linalg.norm(d)
        normal = np.array([-d[1], d[0]])
        base = road[k - 1] + (road[k] - road[k - 1]) * np.clip((ch - chain[k - 1]) / max(seg[k - 1], 1e-9), 0, 1)
        for attempt in range(12):
            side = first_side * (1.0 if attempt < 6 else -1.0)
            p = base + normal * side * draw.uniform(*ROAD_OFFSET_M) + d * draw.uniform(-6.0, 6.0)
            pp = p[None, :]
            if not common(pp)[0]:
                continue
            off = road_tree.query(p)[0]
            if off < ROAD_OFFSET_M[0] - 0.1 or off > 6.0:
                continue
            gap = 20.0 if slot % 2 == 0 else ROAD_MIN_APART_M
            if roadside and min(np.linalg.norm(np.array([r[2] for r in roadside]) - p, axis=1)) < gap:
                continue
            roadside.append((slot, kind, p))
            return
        print(f"roadside: no safe verge at chainage {ch:.0f} m")

    kinds = ["BerryBush", "BerryBush", "Flowers", "BerryBush", "Roots"]
    for i, ch in enumerate(np.arange(ROAD_FROM_M, ROAD_TO_M, ROAD_SPACING_M)):
        verge(2 * i, ch, 1.0 if i % 2 == 0 else -1.0, kinds[i % len(kinds)], rng)
    infill_rng = np.random.default_rng(SEED + 3)
    for i, ch in enumerate(np.arange(ROAD_FROM_M + ROAD_SPACING_M / 2, ROAD_TO_M, ROAD_SPACING_M)):
        verge(2 * i + 1, ch, -1.0 if i % 2 == 0 else 1.0, ROAD_INFILL_KINDS[i % len(ROAD_INFILL_KINDS)], infill_rng)

    # Ids are save identity: keep every committed row, add only genuinely new picks with fresh ids
    # (forage_ids.py). Each output keeps to its own reserved range.
    out_dir = os.environ.get("HOMESTEAD_FORAGE_OUT", sim)
    estate_path = os.path.join(sim, "HomesteadEstateForagePlacements.inc")
    road_path = os.path.join(sim, "HomesteadEstateRoadsidePlacements.inc")
    estate_header, estate_rows = forage_ids.read_rows(estate_path, "forage", ESTATE_FIRST_ID, ESTATE_END_ID)
    road_header, road_rows = forage_ids.read_rows(road_path, "roadside", ROADSIDE_FIRST_ID, ROADSIDE_END_ID)
    # Two pools, so the later passes' larger targets never let an earlier pass's drifted pick in.
    targets = {"BerryBush": ESTATE_WOOD_EDGE + ESTATE_FIELD_HEDGE, "Roots": ROOTS_NEAR + ROOTS_WOODS}
    picks = [(kind, x * 100.0, y * 100.0) for kind, (x, y) in estate]
    estate_rows, estate_added = forage_ids.allocate_pool(estate_rows, picks, targets, "forage", ESTATE_FIRST_ID,
                                                         ESTATE_END_ID, RETIRED_ESTATE_IDS, ESTATE_GAP_M * 100.0)
    targets = {kind: n + {"BerryBush": MORE_BRAMBLES, "Roots": MORE_ROOTS}[kind] for kind, n in targets.items()}
    picks = [(kind, x * 100.0, y * 100.0) for kind, (x, y) in more_brambles + more_roots]
    estate_rows, more_added = forage_ids.allocate_pool(estate_rows, picks, targets, "forage", ESTATE_FIRST_ID,
                                                       ESTATE_END_ID, RETIRED_ESTATE_IDS, ESTATE_GAP_M * 100.0)
    estate_added += more_added

    def slot_of(row):
        k = int(np.argmin(np.hypot(road[:, 0] - row.x / 100.0, road[:, 1] - row.y / 100.0)))
        return int(round((chain[k] - ROAD_FROM_M) / (ROAD_SPACING_M / 2)))

    road_picks = {slot: (kind, p[0] * 100.0, p[1] * 100.0) for slot, kind, p in roadside}
    road_rows, road_added = forage_ids.allocate_slots(road_rows, slot_of, road_picks, "roadside", ROADSIDE_FIRST_ID,
                                                      ROADSIDE_END_ID, RETIRED_ROADSIDE_IDS)
    os.makedirs(out_dir, exist_ok=True)
    forage_ids.write_rows(os.path.join(out_dir, "HomesteadEstateForagePlacements.inc"), estate_header, estate_rows)
    forage_ids.write_rows(os.path.join(out_dir, "HomesteadEstateRoadsidePlacements.inc"), road_header, road_rows)
    # The water and trail the forage keeps clear of, for the native tests (the Simulation has no estate water).
    keep_dir = os.path.join(ROOT, "Tests", "Data") if out_dir == sim else out_dir
    lines = ["// Generated by Scripts/Terrain/forage.py from estate_layout.json - do not edit by hand.",
             f"// HomesteadPublicRoadTests checks new forage keeps {RIVER_CLEAR_M:g} m off the river's centreline, out of the",
             "// lake and off its footpath. Points in cm."]
    lines += [f"river({x * 100.0:.1f}, {y * 100.0:.1f});" for x, y in L["river"]]
    if lake:
        lines += [f"lakeShore({x * 100.0:.1f}, {y * 100.0:.1f});" for x, y in lake["shore"]]
        lines += [f"lakePath({x * 100.0:.1f}, {y * 100.0:.1f});" for x, y in lake["path"]]
    with open(os.path.join(keep_dir, "HomesteadForageKeepOuts.inc"), "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print(f"estate: {len(estate_rows)} rows ({len(estate_added)} new: {[r.id for r in estate_added]}); "
          f"roadside: {len(road_rows)} rows ({len(road_added)} new: {[r.id for r in road_added]}) -> {out_dir}")

if __name__ == "__main__":
    main()
