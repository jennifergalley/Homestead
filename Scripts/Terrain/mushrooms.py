"""Bake the Estate's autumn field mushrooms (ResourceKind::FieldMushrooms, ids 580000-580999).

Field mushrooms (Agaricus campestris) come up in short, grazed or mown grass in late summer and
autumn, often in loose groups, and along the sunny edges of woods. So the placements go in a dozen
loose groups of two to five:
  - in the open pasture and rough lawns within about 450 m of the manor, away from trees;
  - along woodland edges, where a few trees stand within 10 m.
They're present all year but only have anything to pick in Autumn (Simulation/HomesteadSeasons).

Output (committed): Source/SurvivalGame/Simulation/HomesteadEstateMushroomPlacements.inc, rows
`mushroom(id, x, y)` in cm (x north, y east). ProvisionalEstatePlacements skips any row that lands
within 1.5 m of an earlier placement, so ids stay stable when other lanes add placements.
Deterministic (fixed seed). Needs the terrain work folder (HOMESTEAD_TERRAIN_WORK, see scatter.py).
Usage: python Scripts/Terrain/mushrooms.py
"""
import json, os, re
import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

from scatter import HERE, ROOT, WORK, H, weights, sample, densify

FIRST_ID = 580000
LAST_ID = 580999
GROUPS = 12             # loose groups of mushrooms
PER_GROUP = (2, 5)      # mushrooms in a group (inclusive)
GROUP_SPREAD_M = 4.0    # radius of a group
MIN_GAP_M = 1.6         # between mushrooms, and from any other placement
REACH_M = (45.0, 450.0) # from the standing room: past the ruin's lawns, within a short walk
SIM = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")


def placed_points():
    """Every interactable the earlier sections place, in metres."""
    pts = []
    for name, pattern in (("HomesteadEstateWorldPlacements.inc", r"world\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)\)"),
                          ("HomesteadEstateBerryPlacements.inc", r"berry\(\d+, (-?[\d.]+), (-?[\d.]+)\)"),
                          ("HomesteadEstateClearoutPlacements.inc", r"clearout\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)\)")):
        path = os.path.join(SIM, name)
        if os.path.exists(path):
            pts += [(float(x) / 100, float(y) / 100) for x, y in re.findall(pattern, open(path).read())]
    estate = open(os.path.join(SIM, "HomesteadEstate.cpp")).read()
    pts += [(float(x) / 100, float(y) / 100) for x, y in re.findall(r"\{(-\d{5}), (-\d{5})\}", estate)]
    return np.array(pts)


def main():
    rng = np.random.default_rng(1853)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    W = weights()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    lm = {k: np.array(v[:2]) for k, v in L["landmarks"].items()}
    road = cKDTree(densify(L["road"], 1.0))
    river = cKDTree(densify(L["river"]))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    home = lm["StandingRoomOrigin"]
    others = cKDTree(placed_points())
    berry_rows = open(os.path.join(SIM, "HomesteadEstateBerryPlacements.inc")).read()
    brambles = cKDTree(np.array([(float(x) / 100, float(y) / 100) for x, y in re.findall(r"berry\(\d+, (-?[\d.]+), (-?[\d.]+)\)", berry_rows)]))
    raw = open(os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"),
                                                         ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")]))
    trunks = rec[np.isin(rec["kind"], [0, 1, 13, 14, 15, 16, 19, 20])]
    trees = cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)

    def slope_at(p):
        r = np.clip(np.round(H - p[:, 0]).astype(int), 0, slope.shape[0] - 1)
        c = np.clip(np.round(H + p[:, 1]).astype(int), 0, slope.shape[1] - 1)
        return slope[r, c]

    def ok(p):
        p = np.atleast_2d(p)
        keep = sample(z, p[:, 0], p[:, 1]) > 1.5
        keep &= road.query(p)[0] > 4.0
        keep &= river.query(p)[0] > 8.0
        keep &= points_in_poly(p, boundary)
        keep &= slope_at(p) < 16
        d = np.linalg.norm(p - home, axis=1)
        keep &= (d > REACH_M[0]) & (d < REACH_M[1])
        # Clear of the ruin by 10 m and of the derelict farm (X -222..-162, Y -705..-645 m) by 3 m.
        keep &= ~((p[:, 0] > manor[:, 0].min() - 10) & (p[:, 0] < manor[:, 0].max() + 10)
                  & (p[:, 1] > manor[:, 1].min() - 10) & (p[:, 1] < manor[:, 1].max() + 10))
        keep &= ~((p[:, 0] > -225) & (p[:, 0] < -159) & (p[:, 1] > -708) & (p[:, 1] < -642))
        keep &= others.query(p)[0] > MIN_GAP_M
        keep &= brambles.query(p)[0] > 3.2  # ProvisionalEstatePlacements keeps 3 m from brambles
        # Grass, not bare woodland floor or moor.
        keep &= sample(W["Pasture"], p[:, 0], p[:, 1]) > 0.5
        return keep

    # Group centres: open grass (no tree within 8 m) or a wood's edge (a few trees within 10 m).
    cand = np.c_[rng.uniform(home[0] - 460, home[0] + 460, 20000), rng.uniform(home[1] - 460, home[1] + 460, 20000)]
    cand = cand[ok(cand)]
    near8 = np.array([len(n) for n in trees.query_ball_point(cand, 8.0)])
    near10 = np.array([len(n) for n in trees.query_ball_point(cand, 10.0)])
    open_grass = cand[near8 == 0]
    edge = cand[(near10 >= 1) & (near10 <= 4) & (trees.query(cand)[0] > 2.5)]
    centres = []
    for pool, want in ((open_grass, GROUPS * 2 // 3), (edge, GROUPS - GROUPS * 2 // 3)):
        taken = 0
        for i in rng.permutation(len(pool)):
            if taken >= want: break
            p = pool[i]
            if all(np.linalg.norm(p - c) > 35.0 for c in centres):
                centres.append(p)
                taken += 1
    chosen = []
    for centre in centres:
        n = rng.integers(PER_GROUP[0], PER_GROUP[1] + 1)
        group = 0
        for _ in range(n * 8):
            if group >= n: break
            a, r = rng.uniform(0, 2 * np.pi), GROUP_SPREAD_M * np.sqrt(rng.random())
            p = centre + np.array([np.cos(a), np.sin(a)]) * r
            if not ok(p)[0] or trees.query(p)[0] < 1.5: continue
            if chosen and np.min(np.linalg.norm(np.array(chosen) - p, axis=1)) < MIN_GAP_M: continue
            chosen.append(p)
            group += 1
    assert FIRST_ID + len(chosen) - 1 <= LAST_ID
    out = os.path.join(SIM, "HomesteadEstateMushroomPlacements.inc")
    with open(out, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/mushrooms.py; do not edit by hand.\n")
        fh.write("// Autumn field mushrooms (rework-farming-calendar-and-period-crafting, lane D): {id, {x, y} cm}.\n")
        for i, (x, y) in enumerate(chosen):
            fh.write(f"mushroom({FIRST_ID + i}, {x * 100.0:.1f}, {y * 100.0:.1f});\n")
    d = np.linalg.norm(np.array(chosen) - home, axis=1)
    print(f"{len(chosen)} field mushrooms in {len(centres)} groups; {d.min():.0f}-{d.max():.0f} m from the standing room")


if __name__ == "__main__":
    main()
