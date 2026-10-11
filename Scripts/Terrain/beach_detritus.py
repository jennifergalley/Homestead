"""Beach finds, the beach campfire and the beach's decorative scatter (add-beach-gathering).

    python Scripts\\Terrain\\beach_detritus.py --dry     # plan from the current terrain and report; writes nothing
    python Scripts\\Terrain\\beach_detritus.py           # write the .inc (only if none is committed) and the scenery
    python Scripts\\Terrain\\beach_detritus.py --replan  # rewrite committed rows (ids are save identity: only before they ship)

Jenny's card jenny-mv36t3cy-p7dm5i: "Please add seashells, rocks, driftwood, and other beach detritus, both for
decoration and plenty for gathering that refresh daily, including edible gathers like mussels..." and, the same evening,
"a static campfire pile on the beach". Balance 2026-10-10: 10 shell scatters, 8 driftwood, 6 beach pebbles, 8 mussel beds
and 8 laver rocks, most of them on her own cove and the beach near the foot of the cove steps so a full pick takes under
about five minutes, flowers (thrift, sea campion) where the sand meets the grass, and one campfire. Two of the forty
(one shell scatter, one mussel bed) sit at the sea cave's mouth at the Cave Agent's coordinates (CAVE_ROWS).

Outputs:
  Source/SurvivalGame/Simulation/HomesteadEstateBeachPlacements.inc   beachFind(id, ResourceKind::X, x, y) rows, 586000+
  Content/SurvivalGame/Estate/Runtime/EstateScenery.bin              decor records of kinds 49-55 (every earlier record of
                                                                      those kinds is dropped first, so reruns are idempotent)
Kind bytes match EstateSceneryKinds in HomesteadWorldEstate.cpp (claimed 49-56 on docs/handoff/round-4.md).

Ids are save identity and frozen per kind slot (ID_SLOTS): the campfire is 586000, then each kind owns a fixed run, and
the cave's rows take the last slot of their kind. Once the .inc exists its rows are kept; --replan only before shipping.
Run after the Map Agent has settled the coast. Heights come from the committed EstateHeightfield.r16 (the shared terrain
work npy can lag a rebake); sand is ground 0.05-1.95 m above the sea, gently sloped, within 45 m of it. Needs only the
committed repo files. Deterministic (fixed seeds).
"""
import argparse
import json
import math
import os
import re
import struct
import sys

import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt, gaussian_filter, label
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scatter import HERE, ROOT, WORK, H, sample, densify  # noqa: E402

SIM = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")
INC = os.path.join(SIM, "HomesteadEstateBeachPlacements.inc")
SCENERY = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
R16 = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
COVE_ROUTE = os.path.join(HERE, "cove_route.json")
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
SEED = 20261010

# Ids (docs/handoff/round-4.md: 586000-586099). Slots are fixed per kind so a kind's count can change without
# renumbering any other kind's rows.
CAMPFIRE_ID = 586000
ID_SLOTS = {  # kind: (first id, count) - Balance 2026-10-10
    "Seashells": (586001, 10),
    "Driftwood": (586011, 8),
    "BeachPebbles": (586019, 6),
    "MusselBed": (586025, 8),
    "LaverRocks": (586033, 8),
}
# The sea cave's tideline (Cave Agent): kind -> (x m, y m), or None until it sends the settled mouth. Each takes the
# last id of its kind's slot.
CAVE_ROWS = {"Seashells": (-522.0, -665.0), "MusselBed": (-539.0, -653.0)}   # Cave Agent, 2026-10-10 23:34
CAVE_MOUTH = (-530.0, -669.0)     # Cave Agent: the mouth's floor; the rest of the scatter keeps CAVE_CLEAR_M from it
CAVE_CLEAR_M = 7.0

# Scenery kinds (EstateSceneryKinds 49-55; 56 spare).
SHELL_DRIFT, PEBBLE_DRIFT, WRACK, DRIFT_STICKS, DRIFT_LOG, THRIFT, SEA_CAMPION = range(49, 56)
DECOR_KINDS = (SHELL_DRIFT, PEBBLE_DRIFT, WRACK, DRIFT_STICKS, DRIFT_LOG, THRIFT, SEA_CAMPION)
DECOR_COUNTS = {SHELL_DRIFT: 260, PEBBLE_DRIFT: 140, WRACK: 160, DRIFT_STICKS: 90, DRIFT_LOG: 10,
                THRIFT: 600, SEA_CAMPION: 350}
DECOR_SCALE = {SHELL_DRIFT: (0.8, 1.2), PEBBLE_DRIFT: (0.8, 1.25), WRACK: (0.8, 1.2), DRIFT_STICKS: (0.85, 1.2),
               DRIFT_LOG: (0.85, 1.15), THRIFT: (0.85, 1.3), SEA_CAMPION: (0.85, 1.25)}
DECOR_APART_M = {SHELL_DRIFT: 2.5, PEBBLE_DRIFT: 3.0, WRACK: 4.0, DRIFT_STICKS: 4.0, DRIFT_LOG: 25.0,
                 THRIFT: 0.9, SEA_CAMPION: 1.1}

# The beach (game metres; +x north, +y east; the sea lies south).
WINDOW_X = (-900.0, -350.0)       # rows of the heightfield the coast lies in
SAND_TOP_M = 1.95                 # Map Agent: the berm is 1.7 m; a little over for the grade's noise
SAND_REACH_M = 45.0               # the dry beaches are ~24-38 m wide
BEACH_WEIGHT = 0.5                # the sand mask (Coast.beach) is 0/1
MAX_SAND_Z = SAND_TOP_M
MAX_SLOPE_DEG = 14.0
WATERLINE_M = (0.6, 4.0)          # from the sea: mussel and laver rocks
STRAND_M = (3.0, 12.0)            # the wet-sand tideline: shell scatters, shell drifts, wrack
UPPER_M = (9.0, 45.0)             # dry upper beach: driftwood, pebbles, logs, the campfire
FRINGE_M = (0.5, 6.0)             # thrift and sea campion: just above the sand, where it meets the grass
FRINGE_MAX_Z = 8.0
FRINGE_MAX_SLOPE_DEG = 32.0

# Where she'll gather: the foot of the cove steps (cove_route.json's last control point) and the cove.
NEAR_RADIUS_M = 140.0             # Balance: a full pick in under about five minutes from the steps
FIND_APART_M = 6.0                # any two finds
SAME_KIND_APART_M = 11.0
PLACEMENT_GAP_M = 3.2             # ProvisionalEstatePlacements skips a row within 3 m of an earlier one
ROUTE_CLEAR_M = 3.0
RIVER_CLEAR_M = 6.0
DECOR_ROUTE_CLEAR_M = 1.8
DECOR_RIVER_CLEAR_M = 3.5
DECOR_FIND_CLEAR_M = 1.6
CAMPFIRE_STEPS_M = (10.0, 28.0)   # a short walk from the foot of the steps, on the dry sand
CAMPFIRE_MAX_SLOPE_DEG = 5.0
CAMPFIRE_CLEAR_M = 5.0            # nothing else (finds, logs, flowers) right beside the fire


def load():
    # The committed heightfield is the settled coast (the shared work npy can lag a rebake): r16 rows/cols map to
    # the work frame as npy[2H - col, row] = r16[row, col] (coast_mix.py).
    raw = np.fromfile(R16, "<u2").reshape(2 * H + 1, 2 * H + 1)
    z = ((raw.astype(np.float64) - 32768.0) / 128.0)[:, ::-1].T.copy()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    layout = json.load(open(os.path.join(HERE, "estate_layout.json")))
    route = json.load(open(COVE_ROUTE))
    return z, slope, layout, route


def sea_distance(z):
    """Metres from the open sea (water connected to the map's southern edge), per heightfield cell."""
    sea = z < 0.0
    lab, _ = label(sea)
    ids = np.setdiff1d(np.unique(lab[:, 0]), [0])
    sea = np.isin(lab, ids)
    return distance_transform_edt(~sea), sea


def grid_points(lo_y, hi_y, step=0.5):
    """A fine candidate grid over the coast window."""
    xs = np.arange(WINDOW_X[0], WINDOW_X[1], step)
    ys = np.arange(lo_y, hi_y, step)
    gx, gy = np.meshgrid(xs, ys, indexing="ij")
    return np.c_[gx.ravel(), gy.ravel()]


def existing_points():
    pts = []
    for name in sorted(os.listdir(SIM)):
        if not name.endswith("Placements.inc") or name == os.path.basename(INC):
            continue
        for m in re.finditer(r"\w+\((\d+),\s*(?:ResourceKind::(\w+),\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)",
                             open(os.path.join(SIM, name)).read()):
            pts.append((float(m[3]) / 100.0, float(m[4]) / 100.0))
    return np.array(pts).reshape(-1, 2)


class Coast:
    def __init__(self):
        self.z, self.slope, self.layout, self.route = load()
        self.dsea, self.sea = sea_distance(self.z)
        # Sand: the berm and foreshore (Map Agent, 2026-10-10: dry sand 0.3-1.7 m above the sea), gently sloped and
        # near the sea, so a cliff ledge or the grass slope above never counts.
        self.beach = ((self.z > 0.05) & (self.z < SAND_TOP_M) & (self.slope < MAX_SLOPE_DEG)
                      & (self.dsea < SAND_REACH_M)).astype(np.float32)
        self.steps = np.array(self.route["control"][-1], np.float64)
        self.route_tree = cKDTree(np.asarray(self.route["centreline"], np.float64)[:, :2])
        self.river_tree = cKDTree(densify(self.layout["river"], 1.0))
        self.boundary = np.array(self.layout["polygons"]["EstateBoundary"], np.float64)

    def at(self, grid, p):
        return sample(grid, p[:, 0], p[:, 1])

    def candidates(self, rng, count=900000):
        """Shuffled sand points inside the estate, off the route, the river and the cave mouth."""
        from skimage.measure import points_in_poly
        p = grid_points(self.boundary[:, 1].min(), self.boundary[:, 1].max(), 0.5)
        p = p + rng.uniform(-0.25, 0.25, p.shape)
        z = self.at(self.z, p)
        p = p[(z > 0.05) & (z < FRINGE_MAX_Z)]
        p = p[points_in_poly(p, self.boundary)]
        p = p[self.route_tree.query(p)[0] > DECOR_ROUTE_CLEAR_M]
        p = p[self.river_tree.query(p)[0] > DECOR_RIVER_CLEAR_M]
        p = p[np.hypot(p[:, 0] - CAVE_MOUTH[0], p[:, 1] - CAVE_MOUTH[1]) > CAVE_CLEAR_M]
        p = p[rng.permutation(len(p))]
        return p[:count]

    def describe(self, p):
        return {"z": self.at(self.z, p), "sea": self.at(self.dsea, p), "beach": self.at(self.beach, p),
                "slope": self.at(self.slope, p), "steps": np.hypot(p[:, 0] - self.steps[0], p[:, 1] - self.steps[1])}


def spread(pool, taken, count, apart, same_apart=0.0):
    """Greedy: walk the (shuffled) pool, taking points at least `apart` from everything taken and `same_apart` from
    each other."""
    chosen = []
    taken = list(map(tuple, taken))
    tree = cKDTree(np.array(taken)) if taken else None
    for q in pool:
        if len(chosen) >= count:
            break
        if tree is not None and tree.query(q)[0] < apart:
            continue
        if chosen and min(math.hypot(q[0] - c[0], q[1] - c[1]) for c in chosen) < max(apart, same_apart):
            continue
        chosen.append(q)
    return chosen


def plan_finds(coast, rng):
    """Rows (kind, id, (x, y)) for the campfire and the forty finds."""
    p = coast.candidates(rng)
    d = coast.describe(p)
    sand = (d["beach"] >= BEACH_WEIGHT) & (d["z"] < MAX_SAND_Z) & (d["slope"] < MAX_SLOPE_DEG)
    near = d["steps"] < NEAR_RADIUS_M
    p_ok = coast.route_tree.query(p)[0] > ROUTE_CLEAR_M
    p_ok &= coast.river_tree.query(p)[0] > RIVER_CLEAR_M
    placed = existing_points()
    placed_tree = cKDTree(placed) if len(placed) else None
    if placed_tree is not None:
        p_ok &= placed_tree.query(p)[0] > PLACEMENT_GAP_M
    base = sand & near & p_ok
    # Nearest the steps first, with a little shuffle so the finds don't march in distance order.
    order = np.argsort(d["steps"] + rng.uniform(0.0, 35.0, len(p)))

    rows = []
    taken = [tuple(placed_pt) for placed_pt in placed] if len(placed) else []
    fire_pool = p[order][(base & (d["sea"] > UPPER_M[0] + 3.0) & (d["slope"] < CAMPFIRE_MAX_SLOPE_DEG)
                          & (d["steps"] > CAMPFIRE_STEPS_M[0]) & (d["steps"] < CAMPFIRE_STEPS_M[1]))[order]]
    if not len(fire_pool):
        raise SystemExit("no flat dry sand for the campfire near the steps")
    fire = tuple(fire_pool[0])
    rows.append(("BeachCampfire", CAMPFIRE_ID, fire))
    own = [fire]

    zones = {
        "MusselBed": base & (d["sea"] >= WATERLINE_M[0]) & (d["sea"] <= WATERLINE_M[1]),
        "LaverRocks": base & (d["sea"] >= WATERLINE_M[0]) & (d["sea"] <= WATERLINE_M[1]),
        "Seashells": base & (d["sea"] >= STRAND_M[0]) & (d["sea"] <= STRAND_M[1]),
        "Driftwood": base & (d["sea"] >= UPPER_M[0]) & (d["sea"] <= UPPER_M[1]),
        "BeachPebbles": base & (d["sea"] >= UPPER_M[0]) & (d["sea"] <= UPPER_M[1]),
    }
    for kind in ("MusselBed", "LaverRocks", "Seashells", "Driftwood", "BeachPebbles"):
        first, count = ID_SLOTS[kind]
        cave = CAVE_ROWS.get(kind)
        generated = count - (1 if kind in CAVE_ROWS else 0)
        pool = p[order][zones[kind][order]]
        pool = pool[np.hypot(pool[:, 0] - fire[0], pool[:, 1] - fire[1]) > CAMPFIRE_CLEAR_M]
        chosen = spread(pool, taken + own, generated, FIND_APART_M, SAME_KIND_APART_M)
        if len(chosen) < generated:
            raise SystemExit(f"only {len(chosen)} of {generated} {kind} spots fit near the steps")
        for i, q in enumerate(chosen):
            rows.append((kind, first + i, tuple(q)))
            own.append(tuple(q))
        if kind in CAVE_ROWS and cave is not None:
            rows.append((kind, first + count - 1, tuple(cave)))
            own.append(tuple(cave))
    rows.sort(key=lambda row: row[1])
    return rows


def committed_rows():
    if not os.path.exists(INC):
        return None
    return [(m[2], int(m[1]), (float(m[3]) / 100.0, float(m[4]) / 100.0)) for m in
            re.finditer(r"beachFind\((\d+), ResourceKind::(\w+), (-?[\d.]+), (-?[\d.]+)\)", open(INC).read())]


def write_rows(rows):
    lines = ["// Generated by Scripts/Terrain/beach_detritus.py; do not edit by hand.",
             "// Beach finds and the beach campfire (586000-586099): {id, kind, {x, y} cm}. Ids are save identity: append only."]
    lines += [f"beachFind({pid}, ResourceKind::{kind}, {q[0] * 100.0:.1f}, {q[1] * 100.0:.1f});" for kind, pid, q in rows]
    with open(INC, "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")


def shoreline_yaw(coast, q):
    """Unreal yaw (degrees) along the shore at q: perpendicular to the gradient of the sea distance."""
    r = int(np.clip(round(H - q[0]), 1, coast.dsea.shape[0] - 2))
    c = int(np.clip(round(H + q[1]), 1, coast.dsea.shape[1] - 2))
    dx = -(coast.dsea[r + 1, c] - coast.dsea[r - 1, c])   # +x is north, rows run south
    dy = coast.dsea[r, c + 1] - coast.dsea[r, c - 1]
    return math.degrees(math.atan2(dx, -dy)) if (dx or dy) else 0.0


def plan_decor(coast, rng, finds):
    p = coast.candidates(rng, 1500000)
    d = coast.describe(p)
    sand = (d["beach"] >= BEACH_WEIGHT) & (d["z"] < MAX_SAND_Z) & (d["slope"] < MAX_SLOPE_DEG)
    # The fringe: ground just above the sand (within FRINGE_M of it), not too steep, below the cliff tops.
    sand_edge = distance_transform_edt(coast.beach < 0.5)
    edge = coast.at(sand_edge, p)
    fringe = ((d["beach"] < BEACH_WEIGHT) & (edge >= FRINGE_M[0]) & (edge <= FRINGE_M[1]) & (d["z"] > 0.8)
              & (d["z"] < FRINGE_MAX_Z) & (d["slope"] < FRINGE_MAX_SLOPE_DEG))
    find_pts = np.array([q for _, _, q in finds])
    find_tree = cKDTree(find_pts)
    clear = find_tree.query(p)[0] > DECOR_FIND_CLEAR_M
    fire = next(q for kind, _, q in finds if kind == "BeachCampfire")
    clear &= np.hypot(p[:, 0] - fire[0], p[:, 1] - fire[1]) > CAMPFIRE_CLEAR_M
    zones = {
        SHELL_DRIFT: sand & clear & (d["sea"] >= STRAND_M[0]) & (d["sea"] <= STRAND_M[1] + 4.0),
        WRACK: sand & clear & (d["sea"] >= STRAND_M[0] + 1.0) & (d["sea"] <= STRAND_M[1] + 6.0),
        PEBBLE_DRIFT: sand & clear & (d["sea"] >= UPPER_M[0]),
        DRIFT_STICKS: sand & clear & (d["sea"] >= UPPER_M[0]),
        DRIFT_LOG: sand & clear & (d["sea"] >= UPPER_M[0] + 4.0) & (d["slope"] < 6.0),
        THRIFT: fringe & clear,
        SEA_CAMPION: fringe & clear,
    }
    records = []
    for kind in (DRIFT_LOG, WRACK, SHELL_DRIFT, PEBBLE_DRIFT, DRIFT_STICKS, THRIFT, SEA_CAMPION):
        pool = p[zones[kind]]
        pool = pool[rng.permutation(len(pool))]
        chosen = spread(pool, [], DECOR_COUNTS[kind], DECOR_APART_M[kind])
        lo, hi = DECOR_SCALE[kind]
        for q in chosen:
            # The tideline's wrack and drifts lie along the shore; everything else turns freely.
            yaw = shoreline_yaw(coast, q) + rng.uniform(-18.0, 18.0) if kind in (WRACK, SHELL_DRIFT, DRIFT_LOG) \
                else rng.uniform(0.0, 360.0)
            records.append((kind, q[0] * 100.0, q[1] * 100.0, yaw % 360.0, rng.uniform(lo, hi)))
    return records


def write_scenery(records, path=SCENERY):
    raw = open(path, "rb").read()
    if raw[:4] != b"HSC1":
        raise SystemExit(f"{path}: not an HSC1 scenery file")
    old = np.frombuffer(raw[8:], RECORD)
    keep = old[~np.isin(old["k"], DECOR_KINDS)]
    extra = np.zeros(len(records), RECORD)
    for i, (k, x, y, yaw, s) in enumerate(records):
        extra[i] = (k, b"\0\0\0", x, y, yaw, s)
    result = np.concatenate([keep, extra])
    with open(path, "wb") as fh:
        fh.write(b"HSC1" + struct.pack("<I", len(result)) + result.tobytes())
    return len(old) - len(keep), len(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dry", action="store_true", help="plan and report only; write nothing")
    parser.add_argument("--replan", action="store_true", help="rewrite committed rows (only before they ship)")
    args = parser.parse_args()
    coast = Coast()
    rng = np.random.default_rng(SEED)
    rows = committed_rows()
    if not rows or args.replan:
        rows = plan_finds(coast, rng)
        if not args.dry:
            write_rows(rows)
    else:
        # The cave's two rows arrive after the rest ship: append them in their reserved slots, moving nothing.
        have = {pid for _, pid, _ in rows}
        for kind, cave in CAVE_ROWS.items():
            pid = ID_SLOTS[kind][0] + ID_SLOTS[kind][1] - 1
            if cave is not None and pid not in have:
                rows.append((kind, pid, tuple(cave)))
        rows.sort(key=lambda row: row[1])
        if not args.dry:
            write_rows(rows)
    decor = plan_decor(coast, np.random.default_rng(SEED + 1), rows)
    counts = {}
    for kind, _, _ in rows:
        counts[kind] = counts.get(kind, 0) + 1
    far = max(math.hypot(q[0] - coast.steps[0], q[1] - coast.steps[1]) for _, _, q in rows)
    steps = tuple(round(float(v), 1) for v in coast.steps)
    print(f"finds: {len(rows)} rows {counts}; farthest {far:.0f} m from the steps foot {steps}")
    fire = next(q for k, _, q in rows if k == 'BeachCampfire')
    print(f"campfire: ({fire[0]:.1f}, {fire[1]:.1f}) m")
    by_kind = {}
    for k, *_ in decor:
        by_kind[k] = by_kind.get(k, 0) + 1
    print(f"decor: {len(decor)} records by kind {dict(sorted(by_kind.items()))}")
    missing = [k for k, v in CAVE_ROWS.items() if v is None]
    if missing:
        print(f"note: cave rows not set yet ({', '.join(missing)}); their ids stay free")
    if not args.dry:
        dropped, total = write_scenery(decor)
        print(f"scenery: replaced {dropped} earlier beach records; now {total} records")


if __name__ == "__main__":
    main()
