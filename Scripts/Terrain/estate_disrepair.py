"""Lay out the neglected grounds round the ruined manor (add-derelict-farm-and-estate-disrepair).

Outputs (both committed, deterministic):
  Source/SurvivalGame/Simulation/HomesteadEstateDisrepairPlacements.inc
      clearable overgrowth rows {ResourceKind, {x, y} cm}, appended as ids 550000+ after the
      derelict farm's own rows (HomesteadEstateDisrepair.cpp)
  Source/SurvivalGame/HomesteadEstateDebrisPlacements.inc
      set dressing for AHomesteadDerelictFarm: DEBRIS(folder, mesh, x, y, yaw, scale, blocks) and the
      toppled drive fence as FENCE(run, x, y) post points

Run:  python Scripts\\Terrain\\estate_disrepair.py
Reads the reshaped heightmap from HOMESTEAD_TERRAIN_WORK (E:\\TerrainSource\\work) and the layout
from estate_layout.json. Positions are game metres here (+X north, +Y east) and cm in the outputs.
"""
import json, os
import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
H = 2016

# Matches Anchor::DerelictFarm in HomesteadEstate.cpp (metres).
FARM = (-222.0, -162.0, -705.0, -645.0)
FARM_GATE = (-222.0, -657.0)
# Salvage piles, the loose starter forage and the ruin's gaps keep a clear ring (metres).
KEEP = [(-256.0, -643.5, 2.5), (-254.5, -652.5, 2.5), (-239.5, -651.5, 3.0), (-246.0, -662.0, 2.5),
        (-261.0, -661.0, 2.5), (-266.5, -634.0, 2.5), (-269.5, -645.0, 2.5), (-273.5, -629.0, 2.5),
        (-271.5, -626.0, 2.5), (-275.5, -641.0, 2.5), (-287.5, -623.0, 2.5), (-291.5, -656.0, 2.5),
        (-259.0, -654.5, 3.5), (-241.0, -651.5, 3.0), FARM_GATE + (4.0,)]

# Set dressing (metres, yaw degrees, scale, blocks movement). The lean-to stands against the ruin's
# east gable, north of the standing room; its wall side is its local -X.
DEBRIS = [
    ("EstateDebris", "SM_CollapsedLeanTo", -247.0, -633.9, 90.0, 1.0, True),
    ("EstateDebris", "SM_BrokenBarrel", -243.2, -631.4, 205.0, 1.0, True),
    ("EstateDebris", "SM_BrokenCrate", -250.6, -632.2, 130.0, 1.0, True),
    ("EstateDebris", "SM_RubbishHeap", -236.6, -637.6, 25.0, 1.0, False),
    ("EstateDebris", "SM_RubbishHeap", -247.0, -669.2, 250.0, 0.9, False),
    ("EstateDebris", "SM_BrokenBarrel", -252.8, -668.4, 70.0, 0.95, True),
    ("EstateDebris", "SM_BrokenCrate", -243.4, -668.0, 330.0, 1.0, True),
    ("EstateDebris", "SM_BrokenBarrel", -262.4, -632.6, 300.0, 1.0, True),
    ("FarmCart", "SM_FarmCart", -226.5, -649.0, 128.0, 1.0, True),
    ("EstateDebris", "SM_BrokenCrate", -228.4, -646.2, 15.0, 1.0, True),
    ("RuinFallenTimbers", "SM_RuinFallenTimbers", -236.0, -641.5, 75.0, 0.9, True),
    ("RuinSlateScatter", "SM_RuinSlateScatter", -238.4, -644.0, 180.0, 1.0, False),
]


def sample(grid, x, y):
    r = np.clip(np.round(H - np.asarray(x)).astype(int), 0, grid.shape[0] - 1)
    c = np.clip(np.round(H + np.asarray(y)).astype(int), 0, grid.shape[1] - 1)
    return grid[r, c]


def densify(p, step=1.0):
    p = np.asarray(p, float)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])], t


def main():
    rng = np.random.default_rng(18510)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    road_pts, _ = densify(L["road"], 1.0)
    road = cKDTree(road_pts)
    keep = np.array([(k[0], k[1]) for k in KEEP])
    keep_r = np.array([k[2] for k in KEEP])
    debris_xy = np.array([(d[2], d[3]) for d in DEBRIS])

    def ok(p, road_clear=4.5, debris_clear=2.2):
        p = np.atleast_2d(p)
        good = points_in_poly(p, boundary) & ~points_in_poly(p, manor)
        good &= road.query(p)[0] > road_clear
        good &= sample(z, p[:, 0], p[:, 1]) > 2.0
        good &= sample(slope, p[:, 0], p[:, 1]) < 24.0
        x0, x1, y0, y1 = FARM
        good &= ~((p[:, 0] > x0 - 2.5) & (p[:, 0] < x1 + 2.5) & (p[:, 1] > y0 - 2.5) & (p[:, 1] < y1 + 2.5))
        d = np.linalg.norm(p[:, None, :] - keep[None, :, :], axis=2)
        good &= (d > keep_r[None, :]).all(axis=1)
        good &= cKDTree(debris_xy).query(p)[0] > debris_clear
        return good

    rows = []
    chosen = []

    def take(kind, p, gap=1.6):
        if not ok(p)[0]:
            return False
        if chosen and min(np.hypot(*(np.array(chosen) - p).T)) < gap:
            return False
        chosen.append(tuple(p))
        rows.append((kind, p))
        return True

    # 1. Weeds, bramble and elder seedlings at the foot of the ruin's outside walls.
    ring = np.r_[manor, manor[:1]]
    for a, b in zip(ring[:-1], ring[1:]):
        length = np.linalg.norm(b - a)
        direction = (b - a) / length
        outward = np.array([direction[1], -direction[0]])
        if points_in_poly(np.atleast_2d((a + b) / 2 + outward * 0.5), manor)[0]:
            outward = -outward
        for s in np.arange(1.2, length - 1.0, 3.1):
            p = a + direction * (s + rng.uniform(-0.6, 0.6)) + outward * rng.uniform(1.3, 2.2)
            roll = rng.random()
            take("Weeds" if roll < 0.45 else "BrambleThin" if roll < 0.82 else "Sapling", p)
    # 2. The grounds: a ring 8-42 m round the ruin run to rank grass, bramble, docks and saplings.
    centre = manor.mean(axis=0)
    cand = centre + rng.uniform(-44, 44, (2600, 2))
    dist = np.linalg.norm(cand - centre, axis=1)
    cand = cand[(dist > 10) & (dist < 44)]
    rng.shuffle(cand)
    target = 125
    placed = 0
    for p in cand:
        if placed >= target:
            break
        roll = rng.random()
        kind = ("BrambleThin" if roll < 0.30 else "Weeds" if roll < 0.52 else "TallGrass" if roll < 0.70
                else "Sapling" if roll < 0.86 else "BrambleThicket" if roll < 0.91
                else "FallenBranch" if roll < 0.96 else "StumpSmall")
        placed += take(kind, p, 2.6)
    # 3. The drive's verges further out, 4.5-8 m off the centreline, both sides.
    pts, arc = densify(L["road"], 1.0)
    tang = np.gradient(pts, axis=0)
    tang /= np.linalg.norm(tang, axis=1, keepdims=True)
    normal = np.c_[-tang[:, 1], tang[:, 0]]
    for s in np.arange(8.0, 320.0, 6.5):
        i = int(s)
        side = 1.0 if rng.random() < 0.5 else -1.0
        p = pts[i] + normal[i] * side * rng.uniform(4.8, 8.0)
        roll = rng.random()
        take("BrambleThin" if roll < 0.45 else "Sapling" if roll < 0.62 else "Weeds" if roll < 0.82 else "TallGrass", p)

    # The old drive fence: toppled runs on the west (left-going-north-east) verge, 5.6 m off the road.
    fence = []
    run = 0
    for start, end in ((12.0, 40.0), (52.0, 71.0), (86.0, 118.0), (140.0, 158.0)):
        run += 1
        for s in np.arange(start, end, 2.75):
            i = int(s)
            p = pts[i] + normal[i] * 5.6
            if ok(p, 3.5, 1.0)[0]:
                fence.append((run, p))

    inc = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateDisrepairPlacements.inc")
    with open(inc, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/estate_disrepair.py; do not edit by hand.\n")
        for kind, (x, y) in rows:
            fh.write(f"{{ResourceKind::{kind}, {{{x * 100.0:.1f}, {y * 100.0:.1f}}}}},\n")
    dressing = os.path.join(ROOT, "Source", "SurvivalGame", "HomesteadEstateDebrisPlacements.inc")
    with open(dressing, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/estate_disrepair.py; do not edit by hand.\n")
        for folder, mesh, x, y, yaw, scale, blocks in DEBRIS:
            fh.write(f'DEBRIS(TEXT("{folder}"), TEXT("{mesh}"), {x * 100.0:.1f}, {y * 100.0:.1f}, {yaw:.1f}, {scale:.2f}, {str(blocks).lower()})\n')
        for r, (x, y) in fence:
            fh.write(f"FENCE({r}, {x * 100.0:.1f}, {y * 100.0:.1f})\n")
    counts = {}
    for kind, _ in rows:
        counts[kind] = counts.get(kind, 0) + 1
    print(len(rows), "overgrowth", counts, "|", len(fence), "fence posts", "| farm gate z",
          round(float(sample(z, FARM_GATE[0], FARM_GATE[1])) * 100.0, 1))


if __name__ == "__main__":
    main()
