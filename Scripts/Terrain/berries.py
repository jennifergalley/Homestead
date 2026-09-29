"""Bake the Estate's pickable blackberry brambles (ResourceKind::BerryBush, ids 540000+).

Blackberry grows where there is light at a woodland's edge, in hedge banks and along lanes, so the
brambles go:
  - a handful round the ruined manor, two of them within sight of the fallen front door, so she
    finds berries on her first morning;
  - along both verges of the drive, beyond the overgrowth lane's bramble and grass (4.5 m) at 7-9 m;
  - along the woodland edges within about 450 m of the manor, where WoodlandFloor fades out.

Output (committed): Source/SurvivalGame/Simulation/HomesteadEstateBerryPlacements.inc, rows
`berry(id, x, y)` in cm (x north, y east). ProvisionalEstatePlacements skips any row that lands within
3 m of an earlier placement, so ids stay stable when other lanes add placements.
Deterministic (fixed seed). Needs the terrain work folder (HOMESTEAD_TERRAIN_WORK, see scatter.py).
"""
import json, os, re
import numpy as np
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

from scatter import HERE, ROOT, WORK, H, weights, sample, densify

FIRST_ID = 540000


def main():
    rng = np.random.default_rng(1852)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    W = weights()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    lm = {k: np.array(v[:2]) for k, v in L["landmarks"].items()}
    road_line = densify(L["road"], 1.0)
    road = cKDTree(road_line)
    river = cKDTree(densify(L["river"]))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    home = lm["StandingRoomOrigin"]
    front_door = np.array([manor[:, 0].min(), manor[:, 1].min() + 10.5])
    rows = open(os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateWorldPlacements.inc")).read()
    placed = cKDTree(np.array([(float(x) / 100, float(y) / 100) for x, y in
                               re.findall(r"world\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)\)", rows)]))
    # The overgrowth lane's absolute points (drive verges, rear gap) in HomesteadEstate.cpp.
    estate = open(os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstate.cpp")).read()
    overgrowth = cKDTree(np.array([(float(x) / 100, float(y) / 100) for x, y in
                                   re.findall(r"\{(-\d{5}), (-\d{5})\}", estate)]))
    # The baked woods (scatter.py's EstateScenery.bin, kinds 0-1 are trees), for open sky and edges.
    raw = open(os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"), "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"),
                                                         ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")]))
    trunks = rec[rec["kind"] <= 1]
    trees = cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)

    def trees_within(p, r):
        return np.array([len(n) for n in trees.query_ball_point(p, r)])

    def slope_at(p):
        r = np.clip(np.round(H - p[:, 0]).astype(int), 0, slope.shape[0] - 1)
        c = np.clip(np.round(H + p[:, 1]).astype(int), 0, slope.shape[1] - 1)
        return slope[r, c]

    def ok(p, road_clear=6.0):
        p = np.atleast_2d(p)
        keep = sample(z, p[:, 0], p[:, 1]) > 1.5
        keep &= road.query(p)[0] > road_clear
        keep &= river.query(p)[0] > 6.0
        keep &= points_in_poly(p, boundary)
        # Clear of the ruin's walls by 4 m.
        grown = np.c_[np.clip(p[:, 0], manor[:, 0].min() - 4, manor[:, 0].max() + 4),
                      np.clip(p[:, 1], manor[:, 1].min() - 4, manor[:, 1].max() + 4)]
        keep &= ~((grown == p).all(axis=1))
        keep &= slope_at(p) < 22
        # Clear of the overgrowth lane's doorway bramble and forecourt meadow (HomesteadEstate.cpp).
        keep &= np.linalg.norm(p - np.array([-262.5, -654.5]), axis=1) > 7.0
        keep &= ~((p[:, 0] > -271) & (p[:, 0] < -260) & (p[:, 1] > -625) & (p[:, 1] < -617))
        # Salvage piles and the rubble and rocks shed from the ruin.
        room = np.array([-257.5, -638.0])
        others = [(-256.0, -643.5), (-254.5, -652.5), (-246.0, -662.0), (-239.5, -651.5), (-261.0, -661.0)]
        others += [tuple(room + np.array(o) / 100) for o in [(1900, -1500), (1800, -500), (-800, -2000), (-800, -1000),
                   (0, -3100), (600, -2800), (1700, -2400), (-1100, 1800), (-900, 400), (-1200, -700), (-1600, 900),
                   (-1400, 1200), (-1800, -300), (2400, -3200)]]
        keep &= cKDTree(np.array(others)).query(p)[0] > 4.0
        # The world lane's forage, trees and flowers (scatter.py).
        keep &= placed.query(p)[0] > 4.0
        keep &= overgrowth.query(p)[0] > 2.8
        # Brambles need light: no trunk within 4 m.
        keep &= trees.query(p)[0] > 4.0
        return keep

    chosen = []

    def take(points, limit, gap):
        count = 0
        for pt in points:
            if count >= limit: break
            if chosen and np.min(np.linalg.norm(np.array(chosen) - pt, axis=1)) < gap: continue
            chosen.append(pt)
            count += 1
        return count

    # Round the manor: 20-60 m out, nearest first, two of them close to the fallen front door.
    near = np.c_[rng.uniform(-60, 60, 4000), rng.uniform(-60, 60, 4000)] + home
    near = near[ok(near)]
    distance = np.linalg.norm(near - home, axis=1)
    near = near[(distance > 18) & (distance < 60)]
    by_door = near[np.argsort(np.linalg.norm(near - front_door, axis=1))]
    take(by_door, 2, 8)
    take(near[np.argsort(np.linalg.norm(near - home, axis=1))], 5, 12)

    # The drive: alternate verges every ~40 m for its first 800 m, 6-8 m out.
    tang = np.gradient(road_line, axis=0)
    tang /= np.linalg.norm(tang, axis=1, keepdims=True)
    normal = np.c_[-tang[:, 1], tang[:, 0]]
    drive = []
    for i, s in enumerate(range(20, min(800, len(road_line)), 40)):
        side = 1.0 if i % 2 == 0 else -1.0
        drive.append(road_line[s] + normal[s] * side * rng.uniform(7.0, 9.0) + tang[s] * rng.uniform(-6, 6))
    drive = np.array(drive)
    take(drive[ok(drive, 5.5)], 20, 20)

    # Woodland edges: open ground with the baked woods close by (4+ trunks within 14 m), within
    # 450 m of the manor, nearest first.
    edge = np.c_[rng.uniform(-450, 450, 60000), rng.uniform(-450, 450, 60000)] + home
    edge = edge[ok(edge)]
    edge = edge[trees_within(edge, 14.0) >= 4]
    edge = edge[np.argsort(np.linalg.norm(edge - home, axis=1) + rng.uniform(0, 60, len(edge)))]
    take(edge, 24, 16)

    out = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateBerryPlacements.inc")
    with open(out, "w", newline="\n") as f:
        f.write("// Generated by Scripts/Terrain/berries.py; do not edit by hand.\n")
        f.write("// Pickable blackberry brambles (add-overgrown-estate-clearing): {id, {x, y} cm}.\n")
        for i, (x, y) in enumerate(chosen):
            f.write(f"berry({FIRST_ID + i}, {x * 100:.1f}, {y * 100:.1f});\n")
    print(f"{len(chosen)} berry brambles -> {out}")


if __name__ == "__main__":
    main()
