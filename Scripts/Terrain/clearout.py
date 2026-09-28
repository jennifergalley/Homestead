"""Bake the manor clear-out (add-coral-island-clearout, ids 570000+).

Like the first days on a Coral Island farm, the ground right round the ruined manor starts littered
with things to clear before she can dig or build: weeds and nettles, bramble and saplings, stumps of
three sizes, rocks and rubble shed from the ruin, and the rubbish of years of neglect (smashed
crates, burst barrels, middens, rotten planks). Each kind's tool, tier and yield live in
HomesteadOvergrowth.cpp. The field is a band 2-38 m out from the ruin's walls in three moods:
  - the yard, between the ruin's rear wall and the derelict farm's fence: middens, crates, barrels,
    planks and the nettles that thrive on them, with rubble shed from the walls;
  - the old kitchen garden, east of the standing room: rank weeds, nettles and grass over self-sown
    roots, a few rotten cold-frame planks and stones;
  - the grounds, south and west toward the valley: stumps, saplings, bramble and rocks, with the
    odd large stump or boulder that needs a better tool.
Clear lanes stay open from the front door, round the east side to the road, from the rear-wall gap
to the farm gate and round every salvage pile, and nothing lands in the ruin or the standing room.

Output (committed): Source/SurvivalGame/Simulation/HomesteadEstateClearoutPlacements.inc, rows
`clearout(id, ResourceKind::Kind, x, y);` in cm (x north, y east). ProvisionalEstatePlacements skips a
row within 1.5 m of an earlier placement (3 m of a blackberry bramble), so ids stay stable when
other lanes add placements. Deterministic (fixed seed). Needs the terrain work folder
(HOMESTEAD_TERRAIN_WORK, see scatter.py). Reads the manor lane's disrepair rows (550000+) from
HOMESTEAD_DISREPAIR_INC or this checkout when present.
"""
import json, os, re
import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

from scatter import HERE, ROOT, WORK, sample, densify

FIRST_ID = 570000
TARGET = 380
SIM = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation")

# Matches Anchor::DerelictFarm (manor lane) in metres, x0 x1 y0 y1.
FARM = (-222.0, -162.0, -705.0, -645.0)
FARM_GATE = (-222.0, -657.0)
STANDING_ROOM = (-259.0, -253.0, -641.0, -635.0)
# The manor lane's set dressing that stays (lean-to, cart, fallen timbers, slate scatter), metres.
DRESSING = [(-247.0, -633.9, 2.8), (-226.5, -649.0, 3.0), (-236.0, -641.5, 2.6), (-238.4, -644.0, 1.6)]
# Its rubbish that becomes clearable here, at the same spots: kind, x, y (metres).
RUBBISH_SPOTS = [("BrokenBarrel", -243.2, -631.4), ("BrokenCrate", -250.6, -632.2), ("RubbishHeap", -236.6, -637.6),
                 ("RubbishHeap", -247.0, -669.2), ("BrokenBarrel", -252.8, -668.4), ("BrokenCrate", -243.4, -668.0),
                 ("BrokenBarrel", -262.4, -632.6), ("BrokenCrate", -228.4, -646.2)]

WEIGHTS = {
    "yard": {"Nettles": 22, "Weeds": 12, "TallGrass": 5, "RubbishHeap": 6, "BrokenCrate": 7, "BrokenBarrel": 5,
             "RottenPlanks": 9, "Rubble": 9, "SmallRock": 7, "StumpSmall": 5, "StumpMedium": 4, "Sapling": 4,
             "FallenBranch": 3, "BrambleThin": 2, "StumpLarge": 1},
    "garden": {"Weeds": 30, "Nettles": 15, "TallGrass": 15, "RottenPlanks": 6, "BrokenCrate": 3, "BrokenBarrel": 2,
               "SmallRock": 8, "Rubble": 3, "StumpSmall": 5, "StumpMedium": 2, "Sapling": 5, "BrambleThin": 6},
    "grounds": {"Weeds": 15, "Nettles": 6, "TallGrass": 10, "BrambleThin": 12, "Sapling": 10, "StumpSmall": 10,
                "StumpMedium": 7, "StumpLarge": 4, "StumpAncient": 0.5, "SmallRock": 10, "Rubble": 5, "Boulder": 2.5,
                "BrambleThicket": 2, "FallenBranch": 5, "RottenPlanks": 2},
}
# Minimum spacing to other clear-out rows by size (metres): big things want more room.
SIZE = {"StumpLarge": 3.2, "StumpAncient": 4.0, "Boulder": 3.4, "BrambleThicket": 3.4, "RubbishHeap": 3.0,
        "BrokenBarrel": 2.8, "BrokenCrate": 2.6, "RottenPlanks": 2.6, "StumpMedium": 2.6}


def segments(points, step=0.5):
    out = []
    for a, b in zip(points[:-1], points[1:]):
        a, b = np.array(a, float), np.array(b, float)
        n = max(2, int(np.linalg.norm(b - a) / step))
        out.append(a + (b - a) * np.linspace(0, 1, n)[:, None])
    return np.vstack(out)


def read_points(path, pattern):
    if not path or not os.path.exists(path):
        return []
    return [(float(x) / 100, float(y) / 100) for x, y in re.findall(pattern, open(path).read())]


def main():
    rng = np.random.default_rng(1850)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    road = cKDTree(densify(L["road"], 1.0))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    mx0, mx1 = manor[:, 0].min(), manor[:, 0].max()
    my0, my1 = manor[:, 1].min(), manor[:, 1].max()
    front_door = np.array([mx0, my0 + 10.5])
    rear_gap = np.array([-241.0, -651.5])
    road_start = np.array(L["road"][0])

    # Everything already placed nearby: the world lane's forage and trees, the overgrowth lane's
    # rows in HomesteadEstate.cpp, the berry brambles and the manor lane's disrepair.
    others = read_points(os.path.join(SIM, "HomesteadEstateWorldPlacements.inc"),
                         r"world\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)\)")
    estate = open(os.path.join(SIM, "HomesteadEstate.cpp")).read()
    others += [(float(x) / 100, float(y) / 100) for x, y in re.findall(r"\{(-\d{5}), (-\d{5})\}", estate)]
    berries = read_points(os.path.join(SIM, "HomesteadEstateBerryPlacements.inc"),
                          r"berry\(\d+, (-?[\d.]+), (-?[\d.]+)\)")
    disrepair_inc = os.environ.get("HOMESTEAD_DISREPAIR_INC", os.path.join(SIM, "HomesteadEstateDisrepairPlacements.inc"))
    disrepair = read_points(disrepair_inc, r"\{ResourceKind::\w+, \{(-?[\d.]+), (-?[\d.]+)\}\}")
    spawn = np.array([-257.5, -638.0])
    # The overgrowth lane's spawn-relative rows (forecourt meadow, shed rubble and rocks, loose forage).
    for dx, dy in [(-900, 400), (-1200, -700), (-1600, 900), (-1400, 1200), (-1800, -300), (-3000, 1500),
                   (-3400, -1800), (1900, -1500), (1800, -500), (-800, -2000), (-800, -1000), (0, -3100),
                   (600, -2800), (1700, -2400), (-1100, 1800), (2400, -3200), (-1300, 2600), (900, 2500),
                   (-700, 1400), (-200, 1700), (300, 1550), (600, 1900), (-900, 2000), (100, 2100)]:
        others.append(tuple(spawn + np.array([dx, dy]) / 100))
    for row in range(4):
        for column in range(6):
            others.append(tuple(spawn + np.array([-1100 + column * 110 + (row % 2) * 45, 1500 + row * 120]) / 100))
    # Salvage piles 520001-520005 (HomesteadEstate.cpp's u/v frame).
    salvage = [(mx0 + v / 100, my0 + u / 100) for u, v in [(2150, 300), (1250, 450), (300, 1300), (1350, 1950), (400, -200)]]
    placed = cKDTree(np.array(others + disrepair + [(x, y) for x, y, _ in DRESSING] + salvage))
    berry_tree = cKDTree(np.array(berries)) if berries else None

    # The decorative woods and boulders (scatter.py's EstateScenery.bin): trunks, rocks and shrubs.
    raw = open(os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"),
                                                         ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")]))
    solid = rec[np.isin(rec["kind"], [0, 1, 2, 6, 7, 8, 12])]
    scenery = cKDTree(np.c_[solid["x"], solid["y"]] / 100.0)

    # Lanes she walks: out of the front door and round the east side to the road; out of the rear
    # gap to the farm gate and the road; and to the salvage piles outside.
    lanes = [segments([front_door + (0.5, 0), (mx0 - 5.5, my0 + 10.5), (mx0 - 5.5, my1 + 5.0),
                       (mx1 + 3.0, my1 + 5.5), tuple(road_start)]),
             segments([front_door, (mx0 - 14.0, my0 + 10.5)]),
             segments([rear_gap, (rear_gap[0] + 6, rear_gap[1]), FARM_GATE]),
             segments([(rear_gap[0] + 6, rear_gap[1]), tuple(road_start)]),
             segments([(mx0 - 5.5, my0 + 10.5), salvage[4]]),
             # The manor lane's keep-clear strips: west of the farm gate, and the rear gap to the drive.
             segments([FARM_GATE, (-228.0, -657.0)]),
             segments([(-243.0, -650.5), (-239.5, -651.0), (-234.0, -643.0), (-231.0, -632.0), (-228.0, -626.0),
                       (-216.0, -612.0)])]
    lane = cKDTree(np.vstack(lanes))
    # Round the farm gate (and its leaf swung open toward the manor), and before the lean-to's open side.
    keep = [(FARM_GATE[0], FARM_GATE[1], 4.0), (-223.5, -658.6, 2.0), (-247.0, -631.5, 2.2)] + DRESSING

    def ok(p):
        p = np.atleast_2d(p)
        good = points_in_poly(p, boundary) & ~points_in_poly(p, manor)
        # 2-38 m out from the ruin's outer walls.
        dx = np.maximum(0, np.maximum(mx0 - p[:, 0], p[:, 0] - mx1))
        dy = np.maximum(0, np.maximum(my0 - p[:, 1], p[:, 1] - my1))
        wall = np.hypot(dx, dy)
        good &= (wall > 2.0) & (wall < 38.0)
        rx0, rx1, ry0, ry1 = STANDING_ROOM
        good &= ~((p[:, 0] > rx0 - 3) & (p[:, 0] < rx1 + 3) & (p[:, 1] > ry0 - 3) & (p[:, 1] < ry1 + 3))
        x0, x1, y0, y1 = FARM
        good &= ~((p[:, 0] > x0 - 3) & (p[:, 0] < x1 + 3) & (p[:, 1] > y0 - 3) & (p[:, 1] < y1 + 3))
        good &= road.query(p)[0] > 3.5
        good &= lane.query(p)[0] > 1.7
        for kx, ky, kr in keep:
            good &= np.hypot(p[:, 0] - kx, p[:, 1] - ky) > kr
        good &= sample(z, p[:, 0], p[:, 1]) > 2.0
        good &= sample(slope, p[:, 0], p[:, 1]) < 24.0
        good &= placed.query(p)[0] > 2.0
        good &= scenery.query(p)[0] > 2.5
        if berry_tree is not None:
            good &= berry_tree.query(p)[0] > 3.2
        return good

    def zone(p):
        x, y = p
        if x > mx1 and y < my1 + 6: return "yard"
        if y > my1 and x > mx0 - 6: return "garden"
        return "grounds"

    rows, chosen, gaps = [], [], []

    def take(kind, p, forced=False):
        if not forced and not ok(p)[0]:
            return False
        gap = SIZE.get(kind, 2.2)
        for q, g in zip(chosen, gaps):
            if np.hypot(*(q - p)) < max(gap, g) * (0.8 if forced else 1.0):
                return False
        chosen.append(np.array(p))
        gaps.append(gap)
        rows.append((kind, p))
        return True

    # The manor lane's rubbish, where it was dressed (kept clear of the lanes and the walls).
    for kind, x, y in RUBBISH_SPOTS:
        p = np.array([x, y])
        if lane.query(p)[0] > 1.2 and not points_in_poly(np.atleast_2d(p), manor)[0]:
            take(kind, p, forced=True)
    # The field, drawn nearest the ruin first so the ground by the house is the thickest. A kind that
    # doesn't fit where it was drawn waits for the next spot in its zone, so the big things keep
    # their share instead of losing every tight spot to weeds.
    cand = np.c_[rng.uniform(mx0 - 40, mx1 + 40, 30000), rng.uniform(my0 - 40, my1 + 40, 30000)]
    cand = cand[ok(cand)]
    dx = np.maximum(0, np.maximum(mx0 - cand[:, 0], cand[:, 0] - mx1))
    dy = np.maximum(0, np.maximum(my0 - cand[:, 1], cand[:, 1] - my1))
    cand = cand[np.argsort(np.hypot(dx, dy) + rng.uniform(0, 18, len(cand)))]
    pending = {}
    for p in cand:
        if len(rows) >= TARGET: break
        zn = zone(p)
        w = WEIGHTS[zn]
        kinds = list(w)
        prob = np.array([w[k] for k in kinds], float)
        wall = np.hypot(max(0, mx0 - p[0], p[0] - mx1), max(0, my0 - p[1], p[1] - my1))
        if wall < 5.0 and "Rubble" in w: prob[kinds.index("Rubble")] *= 3.0  # Masonry shed from the walls.
        kind = pending.pop(zn, None) or kinds[rng.choice(len(kinds), p=prob / prob.sum())]
        if not take(kind, p): pending[zn] = kind

    out = os.path.join(SIM, "HomesteadEstateClearoutPlacements.inc")
    with open(out, "w", newline="\n") as f:
        f.write("// Generated by Scripts/Terrain/clearout.py; do not edit by hand.\n")
        f.write("// The manor clear-out (add-coral-island-clearout): {id, kind, {x, y} cm}.\n")
        for i, (kind, (x, y)) in enumerate(rows):
            f.write(f"clearout({FIRST_ID + i}, ResourceKind::{kind}, {x * 100:.1f}, {y * 100:.1f});\n")
    tally = {}
    for kind, _ in rows: tally[kind] = tally.get(kind, 0) + 1
    print(f"{len(rows)} clear-out rows -> {out}")
    print(dict(sorted(tally.items(), key=lambda kv: -kv[1])))
    print("zones:", {zn: sum(1 for _, p in rows if zone(p) == zn) for zn in WEIGHTS})
    print("disrepair rows read:", len(disrepair))
    plot = os.environ.get("CLEAROUT_PLOT")
    if plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(12, 12))
        # Plotted as a map: east to the right, north up.
        ax.fill(np.r_[manor[:, 1], manor[:1, 1]], np.r_[manor[:, 0], manor[:1, 0]], color="0.6")
        x0, x1, y0, y1 = FARM
        ax.plot([y0, y1, y1, y0, y0], [x0, x0, x1, x1, x0], "k--")
        for seg in lanes: ax.plot(seg[:, 1], seg[:, 0], color="gold", lw=6, alpha=.5)
        o = np.array(others + disrepair)
        ax.scatter(o[:, 1], o[:, 0], s=10, c="0.3", marker="x")
        if berries: ax.scatter(np.array(berries)[:, 1], np.array(berries)[:, 0], s=40, c="purple")
        colours = {k: plt.cm.tab20(i % 20) for i, k in enumerate(sorted({k for k, _ in rows}))}
        for k, c in colours.items():
            pts = np.array([p for kk, p in rows if kk == k])
            ax.scatter(pts[:, 1], pts[:, 0], s=40 * SIZE.get(k, 2.2), color=c, label=k, edgecolors="k", linewidths=.3)
        rd = densify(L["road"], 1.0)
        ax.plot(rd[:, 1], rd[:, 0], "brown", lw=3)
        ax.set_xlim(my0 - 45, my1 + 45); ax.set_ylim(mx0 - 45, mx1 + 45); ax.set_aspect("equal"); ax.legend(fontsize=8)
        fig.savefig(plot, dpi=90)


if __name__ == "__main__":
    main()
