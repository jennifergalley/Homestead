"""Forage and wildflowers in the woods along the lake trail.

    python Scripts\\Terrain\\lake_path_plants.py
        -> Source/SurvivalGame/Simulation/HomesteadEstateLakePathPlacements.inc  lakeForage(id, ResourceKind::X, x, y)
        -> Content/SurvivalGame/Estate/Runtime/EstateScenery.bin                 wildflower records appended (kinds 42-48)

Jenny, 2026-09-30: "The path through the woods ... that leads to the lake? Chef's kiss. Now we just need to add berry
bushes and wild roots in the forest around it for me to forage, and wildflowers lining the path and the forest floor
(even if just decorative)."

Forage: live BerryBush and Roots placements (ids 582300-582399, registry in docs/handoff/round-2.md) in natural
clusters on both sides of the trail: at its edges (3.2-9 m off the centreline, reachable from the path), in
under-canopy gaps and clearings, and a few deeper in (12-30 m) for exploring. They use the existing kinds, so
gathering, regrowth, seasons and saves follow the existing rules. Ids are save identity: once the .inc exists its rows
are read back, never replanned (--replan rewrites them, only before they ship).

Wildflowers: decorative EstateScenery instances (no collision, no shadow, culled near; kinds 42-48, keep in step
with EstateSceneryKinds in HomesteadWorldEstate.cpp), in drifts along both verges and in patches on the woodland
floor where light comes through: bluebells, primroses, wild garlic (towards the lake), wood anemones, red campion,
foxgloves and cow parsley. Applying again first removes every record of those kinds, so it's idempotent.

Everything stays off the trail's walk width (lake_features.CLEAR_PATH_M), out of the water and its wet lip, clear of
tree trunks, and (forage) 3.2 m from every placement, matching ProvisionalEstatePlacements' 3 m skip.
Deterministic (fixed seed). Needs the terrain work folder (HOMESTEAD_TERRAIN_WORK, see scatter.py).
"""
import argparse
import json
import math
import os
import re
import struct
import sys

import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lake_features  # noqa: E402
from scatter import HERE, ROOT, WORK, H  # noqa: E402

FIRST_ID, END_ID = 582300, 582400        # registry: 582300-582399 (Water, lake-trail forage)
SEED = 20261001
INC = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateLakePathPlacements.inc")
SCENERY = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
# EstateSceneryKinds indices claimed for the wildflowers (docs/handoff/round-2.md registry).
BLUEBELL, PRIMROSE, WILD_GARLIC, WOOD_ANEMONE, RED_CAMPION, FOXGLOVE, COW_PARSLEY = range(42, 49)
FLOWER_KINDS = (BLUEBELL, PRIMROSE, WILD_GARLIC, WOOD_ANEMONE, RED_CAMPION, FOXGLOVE, COW_PARSLEY)
TRUNK_KINDS = [0, 1, 13, 14, 15, 16, 19, 20]

CORRIDOR_M = 30.0            # how far into the woods either side of the trail
FORAGE_PATH_CLEAR_M = 3.2    # forage stands off the trail's 2.4 m half width, beside it
EDGE_MAX_M = 9.0             # "reachable from the path": an edge cluster's centre is within this
DEEP_MIN_M = 12.0
SHORE_CLEAR_M = 3.0          # out of the water and the lake's margin grass
TRUNK_CLEAR_M = 1.3
PLACEMENT_GAP_M = 3.2        # ProvisionalEstatePlacements skips a row within 3 m of an earlier one
MAX_SLOPE_DEG = 24.0
EDGE_CLUSTERS = 6            # alternating sides along the trail
DEEP_CLUSTERS = 3
CLUSTER_RADIUS_M = 4.5
FLOWER_PATH_CLEAR_M = 2.6    # verge drifts start just off the walk width
VERGE_MAX_M = 5.5
VERGE_STEP_M = 0.55
FLOOR_PATCHES = 30
FLOOR_PATCH_CLUMPS = (8, 18)
FLOWER_TRUNK_CLEAR_M = 0.6
END_MARGIN_M = 1.0           # beside the trail only, not past its ends (the farm, the far shore)
# Matches Anchor::DerelictFarm (HomesteadEstate.cpp) in metres, x0 x1 y0 y1, as forage.py; kept 8 m clear.
FARM = (-222.0, -162.0, -705.0, -645.0)
FARM_CLEAR_M = 8.0


def existing_placements():
    sim = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
    pts = []
    for name in sorted(os.listdir(sim)):
        if not name.endswith("Placements.inc") or name == os.path.basename(INC):
            continue
        for m in re.finditer(r"\w+\((\d+),\s*(?:ResourceKind::(\w+),\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)",
                             open(os.path.join(sim, name)).read()):
            pts.append((float(m[3]) / 100.0, float(m[4]) / 100.0))
    return np.array(pts)


def side_and_distance(path, p):
    """Signed distance from the trail (left of travel towards the lake is +) and distance along it."""
    best = (math.inf, 0.0, 0.0)
    along0 = 0.0
    for (ax, ay), (bx, by) in zip(path[:-1], path[1:]):
        ex, ey = bx - ax, by - ay
        length = math.hypot(ex, ey)
        t = max(0.0, min(1.0, ((p[0] - ax) * ex + (p[1] - ay) * ey) / (length * length)))
        qx, qy = ax + t * ex, ay + t * ey
        d = math.hypot(p[0] - qx, p[1] - qy)
        if d < best[0]:
            cross = ex * (p[1] - ay) - ey * (p[0] - ax)
            best = (d, math.copysign(1.0, cross), along0 + t * length)
        along0 += length
    return best[0] * best[1], best[2]


def committed_rows():
    """The committed forage rows (kind, x m, y m), in id order."""
    text = open(INC).read()
    return [(m[2], float(m[3]) / 100.0, float(m[4]) / 100.0) for m in
            re.finditer(r"lakeForage\((\d+), ResourceKind::(\w+), (-?[\d.]+), (-?[\d.]+)\)", text)]


def bake(scenery_path=SCENERY, replan=False):
    """Plants the wildflowers into the scenery file; plans and writes the forage rows only when there are none
    committed yet (or with replan). Returns (forage rows, flower records added, scenery records)."""
    rng = np.random.default_rng(SEED)
    layout = json.load(open(os.path.join(HERE, "estate_layout.json")))
    lake = layout["lake"]
    path = np.asarray(lake["path"], np.float64)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))

    raw = open(scenery_path, "rb").read()
    records = np.frombuffer(raw[8:], lake_features.RECORD)
    records = records[~np.isin(records["k"], FLOWER_KINDS)]
    trunks = records[np.isin(records["k"], TRUNK_KINDS)]
    tree = cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)

    # Candidate ground: a 0.5 m grid over the corridor.
    lo, hi = path.min(axis=0) - CORRIDOR_M, path.max(axis=0) + CORRIDOR_M
    gxs, gys = np.meshgrid(np.arange(lo[0], hi[0], 0.5), np.arange(lo[1], hi[1], 0.5), indexing="ij")
    cand = np.c_[gxs.ravel(), gys.ravel()]
    dpath = lake_features._path_distance(path, cand[:, 0], cand[:, 1])
    dshore = lake_features._signed_distance(lake["shore"], cand[:, 0], cand[:, 1])
    r = np.clip(np.round(H - cand[:, 0]).astype(int), 0, slope.shape[0] - 1)
    c = np.clip(np.round(H + cand[:, 1]).astype(int), 0, slope.shape[1] - 1)
    farm = ((cand[:, 0] > FARM[0] - FARM_CLEAR_M) & (cand[:, 0] < FARM[1] + FARM_CLEAR_M)
            & (cand[:, 1] > FARM[2] - FARM_CLEAR_M) & (cand[:, 1] < FARM[3] + FARM_CLEAR_M))
    ok = (dpath <= CORRIDOR_M) & (dshore > SHORE_CLEAR_M) & (slope[r, c] <= MAX_SLOPE_DEG) & ~farm
    cand, dpath, dshore = cand[ok], dpath[ok], dshore[ok]
    total = float(np.hypot(*np.diff(path, axis=0).T).sum())
    sides_along = [side_and_distance(path, p) for p in cand]
    side = np.array([s for s, _ in sides_along])
    along = np.array([a for _, a in sides_along])
    beside = (along > END_MARGIN_M) & (along < total - END_MARGIN_M)
    cand, dpath, dshore, side, along = cand[beside], dpath[beside], dshore[beside], side[beside], along[beside]
    cand_tree = cKDTree(cand)
    trunk_gap = tree.query(cand)[0]
    near6 = np.array([len(i) for i in tree.query_ball_point(cand, 6.0)])
    near15 = np.array([len(i) for i in tree.query_ball_point(cand, 15.0)])
    # Light: a gap in the canopy (no trunk within 6 m) inside the wood (trees within 15 m).
    light = (near6 == 0) & (near15 >= 2)
    canopy = near6 >= 1

    # --- Forage clusters (planned once; afterwards the committed rows stand) ---
    placed = list(existing_placements())
    placed_tree = cKDTree(np.array(placed))
    forage_ok = (dpath >= FORAGE_PATH_CLEAR_M) & (trunk_gap >= TRUNK_CLEAR_M) & (placed_tree.query(cand)[0] >= PLACEMENT_GAP_M)
    rows = []
    taken = []

    def free(p):
        return all(math.hypot(p[0] - q[0], p[1] - q[1]) >= PLACEMENT_GAP_M for q in taken)

    def cluster(centre_mask, members, kinds):
        idx = np.where(centre_mask & forage_ok)[0]
        if len(idx) == 0:
            return
        centre = cand[rng.choice(idx)]
        near = np.where(forage_ok & (np.hypot(*(cand - centre).T) <= CLUSTER_RADIUS_M))[0]
        rng.shuffle(near)
        count = 0
        for i in near:
            if count >= members:
                break
            p = cand[i] + rng.uniform(-0.2, 0.2, 2)    # off the candidate grid, so the rows don't line up
            if not free(p):
                continue
            taken.append(p)
            rows.append((kinds[count % len(kinds)], float(p[0]), float(p[1])))
            count += 1

    plan = replan or not os.path.exists(INC)
    for k in range(EDGE_CLUSTERS if plan else 0):
        # Spread along the trail, alternating sides, each centre at the verge or in a gap just behind it.
        band = (along >= total * k / EDGE_CLUSTERS) & (along < total * (k + 1) / EDGE_CLUSTERS)
        on_side = np.sign(side) == (1 if k % 2 == 0 else -1)
        edge = band & on_side & (dpath <= EDGE_MAX_M)
        bramble_spot = edge & (light | (dpath <= 5.0))
        kinds = ["BerryBush", "BerryBush", "Roots"] if k % 2 == 0 else ["Roots", "BerryBush", "Roots"]
        cluster(bramble_spot if kinds[0] == "BerryBush" else edge & canopy, int(rng.integers(2, 4)), kinds)
    for k in range(DEEP_CLUSTERS if plan else 0):
        deep = (dpath >= DEEP_MIN_M) & (np.sign(side) == (1 if k % 2 else -1))
        kinds = ["Roots", "Roots", "BerryBush"] if k != 1 else ["BerryBush", "Roots"]
        cluster(deep & (canopy if kinds[0] == "Roots" else light), int(rng.integers(2, 4)), kinds)

    if not plan:
        rows = committed_rows()
    lines = ["// Generated by Scripts/Terrain/lake_path_plants.py; do not edit by hand.",
             "// Lake-trail forage (582300-582399): {id, kind, {x, y} cm}. Ids are save identity: append only."]
    lines += [f"lakeForage({FIRST_ID + i}, ResourceKind::{kind}, {x * 100.0:.1f}, {y * 100.0:.1f});"
              for i, (kind, x, y) in enumerate(rows)]
    assert FIRST_ID + len(rows) <= END_ID
    if plan:
        with open(INC, "w", newline="\n") as fh:
            fh.write("\n".join(lines) + "\n")
    # The flowers read the rows as committed (rounded to the millimetre) on their own random stream, so a
    # replan and a later rebake plant exactly the same flowers.
    rows = committed_rows()
    rng = np.random.default_rng(SEED + 1)

    # --- Wildflowers ---
    forage_pts = np.array([(x, y) for _, x, y in rows]) if rows else np.zeros((0, 2))
    flower_ok = (dpath >= FLOWER_PATH_CLEAR_M) & (trunk_gap >= FLOWER_TRUNK_CLEAR_M)
    if len(forage_pts):
        flower_ok &= cKDTree(forage_pts).query(cand)[0] >= 1.0
    flowers = []

    def plant(kind, p, scale):
        flowers.append((kind, float(p[0]), float(p[1]), float(rng.uniform(0.0, 360.0)), float(scale)))

    # Verge drifts: walk both verges; each drift is one species for a few metres, with gaps between drifts.
    verge_species = [(PRIMROSE, 0.28), (RED_CAMPION, 0.22), (COW_PARSLEY, 0.24), (BLUEBELL, 0.14), (FOXGLOVE, 0.12)]
    names, weights = zip(*verge_species)
    for sgn in (1, -1):
        s = 0.0
        while s < total:
            length = rng.uniform(3.0, 8.0)
            gap = rng.uniform(1.0, 5.0)
            kind = names[rng.choice(len(names), p=np.array(weights) / sum(weights))]
            density = 0.55 if kind == FOXGLOVE else 1.0
            t = s
            while t < min(s + length, total):
                if rng.uniform() < density:
                    # A point on the trail at distance t, pushed out to the verge on this side.
                    seg = np.cumsum(np.hypot(*np.diff(path, axis=0).T))
                    i = int(np.searchsorted(seg, t))
                    i = min(i, len(path) - 2)
                    start = seg[i - 1] if i > 0 else 0.0
                    a, b = path[i], path[i + 1]
                    u = (b - a) / np.linalg.norm(b - a)
                    normal = np.array([-u[1], u[0]]) * sgn
                    p = a + u * (t - start) + normal * rng.uniform(FLOWER_PATH_CLEAR_M + 0.1, VERGE_MAX_M)
                    j = cand_tree.query(p)[1]
                    if flower_ok[j] and abs(np.hypot(*(cand[j] - p))) < 0.5:
                        tall = kind in (FOXGLOVE, COW_PARSLEY, RED_CAMPION)
                        plant(kind, p, rng.uniform(0.85, 1.25) if tall else rng.uniform(0.9, 1.3))
                t += VERGE_STEP_M
            s += length + gap

    # Woodland floor: patches where light comes through, bluebells and anemones under the trees, wild garlic
    # towards the lake's damp ground, the odd foxglove in a clearing.
    floor = np.where(flower_ok & (dpath >= 3.0) & (light | (near6 <= 1)))[0]
    for _ in range(FLOOR_PATCHES if len(floor) else 0):
        centre = cand[rng.choice(floor)]
        damp = float(lake_features._signed_distance(lake["shore"], np.array([centre[0]]), np.array([centre[1]]))[0]) < 25.0
        roll = rng.uniform()
        kind = (WILD_GARLIC if damp and roll < 0.55 else BLUEBELL if roll < 0.5 else WOOD_ANEMONE if roll < 0.82
                else PRIMROSE if roll < 0.94 else FOXGLOVE)
        radius = rng.uniform(1.5, 4.0)
        count = int(rng.integers(*FLOOR_PATCH_CLUMPS)) // (3 if kind == FOXGLOVE else 1)
        for _ in range(count):
            p = centre + rng.normal(0.0, radius * 0.5, 2)
            j = cand_tree.query(p)[1]
            if flower_ok[j] and np.hypot(*(cand[j] - p)) < 0.5:
                plant(kind, p, rng.uniform(0.8, 1.2))

    extra = np.zeros(len(flowers), lake_features.RECORD)
    for i, (k, x, y, yaw, s) in enumerate(flowers):
        extra[i] = (k, b"\0\0\0", x * 100.0, y * 100.0, yaw, s)
    result = np.concatenate([records, extra])
    with open(scenery_path, "wb") as fh:
        fh.write(b"HSC1" + struct.pack("<I", len(result)) + result.tobytes())
    return rows, flowers, len(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--replan", action="store_true",
                        help="replan the forage rows (only before they ship: their ids are save identity)")
    args = parser.parse_args()
    rows, flowers, total = bake(SCENERY, args.replan)

    kinds = {}
    for kind, _, _ in rows:
        kinds[kind] = kinds.get(kind, 0) + 1
    species = {}
    for k, *_ in flowers:
        species[k] = species.get(k, 0) + 1
    print(f"forage: {len(rows)} rows {kinds}, ids {FIRST_ID}-{FIRST_ID + len(rows) - 1}")
    print(f"wildflowers: {len(flowers)} clumps by kind {dict(sorted(species.items()))}; scenery now {total} records")


if __name__ == "__main__":
    main()
