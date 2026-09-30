"""Lay out the town square (Jenny, 2026-09-29: "the town buildings are bunched too tightly").

The old massing was twelve blockouts packed round a 40 x 34.5 m square, 0.2-0.35 m apart, with the main
road stopping 72 m short of it. This lays out an open square of SQUARE_ALONG_Y_M x SQUARE_ALONG_X_M round
the TownSquare anchor, faced by irregular terraces and cottages with 3-6 m side lanes between the groups,
the general store in the middle of its east side, and a separate curved `townStreet` (5.5 m wide) from the
main road's end into the square. The main road, its 1.94 km chainage and its anchors don't move. The
town already stands on reshape.py's plane pad, so the heightfield doesn't change.

    python Scripts\\Terrain\\town_layout.py

Writes (in step):
    Scripts/Terrain/estate_layout.json   "town" (square, street, buildings, store) and the store anchors
    Tests/Data/HomesteadTownLayout.inc   footprints, square and street for the native tests (generated)
It checks the layout first and refuses to write one that breaks a rule (lanes, clearances, the store
room, the street's curve). Then: weightmaps.py, bake_ground.py and bake_estate_map.py; in the editor
town_massing.py (it reads "town"), ApplyEstateWeightmaps and ImportEstateMap. HomesteadEstate.cpp's
GeneralStoreDoor/Counter mirror the anchors printed here. Saves keep their shop: the game re-seats the
general store's counter on the anchor when a save loads (Simulation::RefreshShopCounters).
"""
import json
import math
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
LAYOUT = os.path.join(HERE, "estate_layout.json")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
TEST_DATA = os.path.join(REPO, "Tests", "Data", "HomesteadTownLayout.inc")
SIZE, H = 4033, 2016

SQUARE_ALONG_Y_M = 60.0          # east-west
SQUARE_ALONG_X_M = 45.0          # north-south
STREET_HALF_WIDTH_M = 2.75       # a 5.5 m street
STREET_CLEAR_M = 0.75            # between the street's edge and any wall
STREET_MIN_RADIUS_M = 12.0       # a cart can take it
LANE_M = (3.0, 6.0)              # side lanes between groups of buildings
TERRACE_JOINT_M = 0.1            # between the houses of one terrace (they share a party wall)
# AHomesteadGeneralStore (HomesteadGeneralStore.h): the counter is 6 m in from the door, the room 9 m deep
# and 8.4 m wide inside; with its walls it takes about 9.4 x 9.6 m behind the front.
STORE_DOOR_TO_COUNTER_M = 6.0
STORE_FOOTPRINT_M = (9.6, 9.4)   # depth, width
STORE_DOOR_OUT_M = 0.5           # the door anchor stands just outside the front

# Frontages of the square, its edges: (name, front line, along axis, yaw of the building's back).
# A building's actor sits at the centre of its front wall and runs back along its local +X (yaw).
# s runs along each side from its south or west end.
SIDES = {
    "north": ("x", +1, "y", 0.0),     # front at x = centre + half X, backs to the north
    "south": ("x", -1, "y", 180.0),
    "east": ("y", +1, "x", 90.0),     # front at y = centre + half Y, backs to the east
    "west": ("y", -1, "x", -90.0),
}
RIDGE, GABLE = "RidgeAlongStreet", "GableToStreet"
RUBBLE, ASHLAR, RENDER = "Rubble", "Ashlar", "Render"
# name, side, s0 (m), width (m), depth (m), setback (m), storeys, storey height (cm), pitch, roof, wall,
# wall tint, chimneys, door at, shopfront, sign colour, side windows. Groups of touching houses are
# terraces; the gaps between groups are the side lanes. Names that the old massing used keep their actors.
BUILDINGS = [
    # North side (60 m, y 1120 -> 1180)
    ("NorthCottage", "north", 3.0, 8.0, 6.5, 0.4, 1, 290, 50, RIDGE, RUBBLE, (0.95, 0.93, 0.9), 1, -0.4, False, (0.1, 0.07, 0.03), True),
    ("Baker", "north", 11.1, 9.0, 7.5, 0.0, 2, 280, 45, GABLE, RENDER, (0.98, 0.97, 0.93), -1, 0.55, True, (0.05, 0.08, 0.16), False),
    ("Draper", "north", 24.1, 10.0, 8.0, 0.2, 2, 290, 40, RIDGE, RUBBLE, (1, 1, 1), 2, 0.6, True, (0.16, 0.05, 0.08), False),
    ("CornerHouse", "north", 34.2, 9.0, 7.0, 0.6, 2, 300, 42, GABLE, RENDER, (1.0, 0.97, 0.9), -1, -0.5, False, (0.05, 0.05, 0.05), True),
    ("Chandler", "north", 48.2, 8.5, 7.0, 0.3, 2, 285, 44, RIDGE, RENDER, (0.97, 0.95, 1.0), 1, 0.4, True, (0.12, 0.09, 0.02), True),
    # East side (45 m, x -562.5 -> -517.5): the general store in the middle, spawned by the game.
    ("EastCottage", "east", 4.5, 9.0, 6.5, 0.3, 1, 285, 48, GABLE, RUBBLE, (1, 1, 1), 1, -0.5, False, (0.1, 0.07, 0.03), True),
    ("EastHouse", "east", 31.3, 10.0, 8.0, 0.0, 2, 295, 40, RIDGE, RENDER, (1.0, 0.95, 0.80), 2, 0.3, False, (0.08, 0.04, 0.02), True),
    # South side (60 m, y 1120 -> 1180)
    ("Butcher", "south", 3.0, 9.0, 7.5, 0.2, 2, 290, 45, GABLE, RUBBLE, (0.92, 0.92, 0.9), -1, -0.55, True, (0.05, 0.12, 0.06), True),
    ("SouthHouse", "south", 12.1, 9.0, 7.0, 0.5, 2, 300, 38, RIDGE, RENDER, (0.92, 0.95, 1.0), 2, 0.0, False, (0.05, 0.05, 0.05), False),
    ("Chemist", "south", 25.6, 8.8, 7.5, 0.0, 2, 310, 35, RIDGE, ASHLAR, (1.05, 1.0, 0.92), 1, 0.55, True, (0.10, 0.06, 0.02), True),
    ("Ironmonger", "south", 34.5, 11.0, 8.0, 0.4, 2, 300, 38, RIDGE, RENDER, (1.0, 0.92, 0.85), 2, -0.5, True, (0.20, 0.04, 0.04), False),
    ("SouthCottage", "south", 50.0, 7.5, 6.5, 0.3, 1, 290, 48, GABLE, RUBBLE, (1.02, 1.0, 0.95), -1, 0.4, False, (0.1, 0.07, 0.03), True),
    # West side (45 m, x -562.5 -> -517.5): the town street comes in at its north end.
    ("WestCottage", "west", 3.0, 9.0, 6.5, 0.3, 1, 290, 45, GABLE, RUBBLE, (1.05, 1.02, 0.95), -1, 0.5, False, (0.1, 0.07, 0.03), True),
    ("Inn", "west", 12.1, 12.0, 8.5, 0.0, 3, 290, 32, RIDGE, ASHLAR, (1, 1, 1), 2, 0.0, False, (0.03, 0.05, 0.12), False),
    ("LaneCottage", "west", 39.0, 6.0, 6.0, 0.3, 1, 280, 50, GABLE, RUBBLE, (1.0, 0.98, 0.94), 1, 0.3, False, (0.1, 0.07, 0.03), True),
]
STORE = ("east", None)           # centred on its side
STREET_ENTRY_S = 33.0            # where the street meets the west side (m along it)


def frame(centre, side):
    """(front-line origin, along unit, outward unit, yaw) of one side in world metres."""
    axis, sign, along, yaw = SIDES[side]
    cx, cy = centre
    half = {"x": SQUARE_ALONG_X_M / 2, "y": SQUARE_ALONG_Y_M / 2}
    if axis == "x":
        origin = np.array([cx + sign * half["x"], cy - half["y"]])
        return origin, np.array([0.0, 1.0]), np.array([float(sign), 0.0]), yaw
    origin = np.array([cx - half["x"], cy + sign * half["y"]])
    return origin, np.array([1.0, 0.0]), np.array([0.0, float(sign)]), yaw


def footprint(centre, side, s0, width, depth, setback):
    origin, along, out, yaw = frame(centre, side)
    front = origin + along * (s0 + width / 2) + out * setback
    corners = [front + along * a + out * d for a, d in ((-width / 2, 0), (width / 2, 0), (width / 2, depth), (-width / 2, depth))]
    return front, yaw, np.array(corners)


def street_points(start, heading, end):
    """A cubic Bezier from the road's end (leaving along its heading) into the square (arriving east), 1 m steps."""
    p0, p3 = np.asarray(start, float), np.asarray(end, float)
    reach = np.linalg.norm(p3 - p0) * 0.45
    p1 = p0 + heading * reach
    p2 = p3 - np.array([0.0, 1.0]) * reach
    t = np.linspace(0, 1, 400)[:, None]
    curve = (1 - t) ** 3 * p0 + 3 * (1 - t) ** 2 * t * p1 + 3 * (1 - t) * t ** 2 * p2 + t ** 3 * p3
    seg = np.r_[0, np.cumsum(np.linalg.norm(np.diff(curve, axis=0), axis=1))]
    s = np.arange(0, seg[-1], 1.0)
    points = np.c_[np.interp(s, seg, curve[:, 0]), np.interp(s, seg, curve[:, 1])]
    return np.vstack([points, p3]) if np.linalg.norm(points[-1] - p3) > 0.05 else points


def point_polygon_distance(p, poly):
    best = 1e9
    for a, b in zip(poly, np.roll(poly, -1, axis=0)):
        d = b - a
        t = np.clip(np.dot(p - a, d) / np.dot(d, d), 0, 1)
        best = min(best, np.linalg.norm(p - (a + t * d)))
    inside = False
    for a, b in zip(poly, np.roll(poly, -1, axis=0)):
        if (a[1] > p[1]) != (b[1] > p[1]) and p[0] < a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1]):
            inside = not inside
    return -best if inside else best


def main():
    layout = json.load(open(LAYOUT))
    lm = layout["landmarks"]
    centre = np.array(lm["TownSquare"][:2], float)
    problems = []

    placed = []
    for row in BUILDINGS:
        name, side, s0, width, depth, setback = row[:6]
        front, yaw, poly = footprint(centre, side, s0, width, depth, setback)
        placed.append((name, side, s0, width, depth, setback, front, yaw, poly, row[6:]))
    # The store: centred on its side, the door just outside its front.
    origin, along, out, store_yaw = frame(centre, STORE[0])
    side_len = SQUARE_ALONG_X_M if STORE[0] in ("east", "west") else SQUARE_ALONG_Y_M
    store_depth, store_width = STORE_FOOTPRINT_M
    store_s0 = side_len / 2 - store_width / 2
    store_front, _, store_poly = footprint(centre, STORE[0], store_s0, store_width, store_depth, 0.0)
    door = store_front - out * STORE_DOOR_OUT_M
    counter = door + out * STORE_DOOR_TO_COUNTER_M
    placed.append(("GeneralStore", STORE[0], store_s0, store_width, store_depth, 0.0, store_front, store_yaw, store_poly, None))

    # Rule: along each side, neighbours either form a terrace (TERRACE_JOINT_M apart) or leave a 3-6 m lane.
    lanes = []
    for side in SIDES:
        row = sorted([p for p in placed if p[1] == side], key=lambda p: p[2])
        for a, b in zip(row, row[1:]):
            gap = b[2] - (a[2] + a[3])
            if abs(gap - TERRACE_JOINT_M) < 1e-6:
                continue
            if LANE_M[0] - 1e-6 <= gap <= LANE_M[1] + 1e-6:
                lanes.append((side, a[0], b[0], round(gap, 2)))
                continue
            if side == "west" and a[2] + a[3] < STREET_ENTRY_S < b[2]:
                continue
            problems.append(f"{side}: {a[0]} to {b[0]} is {gap:.2f} m (a terrace joint or a {LANE_M[0]}-{LANE_M[1]} m lane)")
        for p in row:
            if p[2] < 0 or p[2] + p[3] > (SQUARE_ALONG_X_M if side in ("east", "west") else SQUARE_ALONG_Y_M):
                problems.append(f"{p[0]} runs past the end of the {side} side")
    # Rule: footprints never overlap (corners included).
    for i, a in enumerate(placed):
        for b in placed[i + 1:]:
            if any(point_polygon_distance(q, b[8]) < -1e-6 for q in a[8]) or any(point_polygon_distance(q, a[8]) < -1e-6 for q in b[8]):
                problems.append(f"{a[0]} overlaps {b[0]}")

    # The street: from the main road's last point, along its heading, into the west side at STREET_ENTRY_S.
    road = np.asarray(layout["road"], float)
    start = road[-1]
    heading = (road[-1] - road[-3]) / np.linalg.norm(road[-1] - road[-3])
    w_origin, w_along, w_out, _ = frame(centre, "west")
    entry = w_origin + w_along * STREET_ENTRY_S - w_out * 1.0      # a metre into the square
    street = street_points(start, heading, entry)
    for p in street:
        for b in placed:
            clear = point_polygon_distance(p, b[8]) - STREET_HALF_WIDTH_M
            if clear < STREET_CLEAR_M:
                problems.append(f"the street passes {clear:.2f} m from {b[0]}")
                break
    d1 = np.gradient(street, axis=0)
    d2 = np.gradient(d1, axis=0)
    curvature = np.abs(d1[:, 0] * d2[:, 1] - d1[:, 1] * d2[:, 0]) / np.linalg.norm(d1, axis=1) ** 3
    radius = 1.0 / max(curvature[2:-2].max(), 1e-9)
    if radius < STREET_MIN_RADIUS_M:
        problems.append(f"the street's tightest bend is {radius:.1f} m radius")
    street_len = len(street)
    # The square itself stays open: nothing stands inside it.
    half = np.array([SQUARE_ALONG_X_M / 2, SQUARE_ALONG_Y_M / 2])
    for b in placed:
        rel = np.abs(b[8] - centre)
        if np.any(np.all(rel < half - 1e-6, axis=1)):
            problems.append(f"{b[0]} stands in the square")
    if problems:
        raise SystemExit("town layout rejected:\n  " + "\n  ".join(problems))

    z = (np.fromfile(R16, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0

    def ground(p):
        return round(float(z[int(round(p[1])) + H, int(round(p[0])) + H]), 2)

    lm["GeneralStoreDoor"] = [round(float(door[0]), 2), round(float(door[1]), 2), ground(door), store_yaw]
    lm["GeneralStoreCounter"] = [round(float(counter[0]), 2), round(float(counter[1]), 2), ground(counter), store_yaw - 180.0]
    buildings = []
    for name, side, s0, width, depth, setback, front, yaw, poly, style in placed:
        if style is None:
            continue
        storeys, storey_h, pitch, roof, wall, tint, chimneys, door_at, shop, sign, side_windows = style
        buildings.append({"name": name, "x": round(float(front[0]), 2), "y": round(float(front[1]), 2), "yaw": yaw,
                          "width": width, "depth": depth, "storeys": storeys, "storeyHeight": storey_h, "pitch": pitch,
                          "roof": roof, "wall": wall, "tint": list(tint), "chimneys": chimneys, "doorAt": door_at,
                          "shopfront": shop, "sign": list(sign), "sideWindows": side_windows})
    layout["town"] = {"square": {"centre": centre.round(2).tolist(), "halfX": SQUARE_ALONG_X_M / 2, "halfY": SQUARE_ALONG_Y_M / 2},
                      "street": np.round(street, 2).tolist(), "streetHalfWidth": STREET_HALF_WIDTH_M,
                      "store": {"footprint": np.round(store_poly, 2).tolist()}, "buildings": buildings}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)

    lines = ["// Generated by Scripts/Terrain/town_layout.py from estate_layout.json - do not edit by hand.",
             "// The town square for HomesteadPublicRoadTests (metres): its centre and half extents, the town street,",
             "// and every footprint (the general store first). townSquare(cx, cy, halfX, halfY); street(x, y);",
             "// footprint(name, x0, y0, x1, y1, x2, y2, x3, y3)",
             f"townSquare({centre[0]:.2f}, {centre[1]:.2f}, {SQUARE_ALONG_X_M / 2:.2f}, {SQUARE_ALONG_Y_M / 2:.2f});",
             f"streetHalfWidth({STREET_HALF_WIDTH_M:.2f});"]
    lines += [f"street({x:.2f}, {y:.2f});" for x, y in street]
    for name, *_rest in [placed[-1]] + placed[:-1]:
        poly = next(p[8] for p in placed if p[0] == name)
        lines.append(f"footprint(\"{name}\", " + ", ".join(f"{v:.2f}" for v in poly.ravel()) + ");")
    with open(TEST_DATA, "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")

    print(f"town: square {SQUARE_ALONG_Y_M:.0f} x {SQUARE_ALONG_X_M:.0f} m, {len(buildings)} buildings + the store, "
          f"{len(lanes)} side lanes {sorted(set(l[3] for l in lanes))} m, street {street_len} m (tightest bend {radius:.0f} m)")
    print(f"town: GeneralStoreDoor {lm['GeneralStoreDoor']}, GeneralStoreCounter {lm['GeneralStoreCounter']} "
          f"(mirror in HomesteadEstate.cpp in cm)")


if __name__ == "__main__":
    main()
