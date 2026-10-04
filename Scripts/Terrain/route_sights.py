"""Find stretches of walking route with nothing to see, and fill them with original flower drifts.

    python Scripts\\Terrain\\route_sights.py            # report gaps only
    python Scripts\\Terrain\\route_sights.py --bake     # add 'RS1' flower drifts and re-report

A sight is a forage point, a flower clump, a fingerpost, a landmark or a bridge within SIGHT_RADIUS_M of a
route. A sprint is 4.8 m/s, so a gap over MAX_GAP_M is more than about ten seconds with nothing to look at.
RS1 padding marks only this script's scenery records, so re-baking replaces them without touching others.
Run after public_road.py / town_layout.py / shrink_map.py. Existing scatter kinds only; nothing is authored.
"""
import argparse
import json
import re
import struct
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SIM = ROOT / "Source" / "SurvivalGame" / "Simulation"
SCENERY = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateScenery.bin"
RECORD = np.dtype([("k", "u1"), ("pad", "S3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
TAG = b"RS1"
FLOWER_KINDS = (42, 43, 44, 45, 46, 47, 48)
FLOWER_SCALE = {42: 1.0, 43: 1.0, 44: 1.0, 45: 1.0, 46: 1.0, 47: 1.0, 48: 1.0}
SIGHT_RADIUS_M = 15.0        # a flower drift or forage point this close to the route is in view
MAX_GAP_M = 50.0             # Jenny: a scenery or forage point about every 50 m at a sprint
FILL_OFFSET_M = (5.5, 8.0)   # drifts sit just off the verge, either side in turn
DRIFT_CLUMPS = (6, 10)
DRIFT_RADIUS_M = 2.2
SEED = 20261004
KEEP_CLEAR_M = 3.0           # off the walked line itself
STEP_M = 1.0
STORE_CLEAR_M = 1.5       # flowers keep this far from a building footprint
STREET_CLEAR_M = 3.2      # and this far from the street centreline (half width 2.75 m)
STORE_CLUMPS = 6


def densify(points, step=STEP_M):
    pts = np.asarray(points, float)[:, :2]
    out = []
    for a, b in zip(pts[:-1], pts[1:]):
        n = max(int(round(np.hypot(*(b - a)) / step)), 1)
        out.extend(a + (b - a) * t for t in np.linspace(0, 1, n, endpoint=False))
    out.append(pts[-1])
    return np.array(out)


def read_scenery():
    raw = SCENERY.read_bytes()
    count = struct.unpack_from("<I", raw, 4)[0]
    return np.frombuffer(raw[8:8 + count * RECORD.itemsize], RECORD).copy()


def inc_points(name, pattern):
    text = (SIM / name).read_text(encoding="utf-8")
    return [(float(m[1]) / 100.0, float(m[2]) / 100.0) for m in re.finditer(pattern, text)]


def routes(layout):
    land = layout["landmarks"]
    j = layout["town"]["junctionChainage"]
    road = densify(layout["road"])
    junction = road[min(int(j / STEP_M), len(road) - 1)]
    spur = densify([junction, land["TownArrival"][:2], land["GeneralStoreDoor"][:2]])
    cove = np.array(json.loads((HERE / "cove_route.json").read_text())["centreline"])[:, :2]
    manor = land["StandingRoomSpawn"][:2]
    return {
        "Public road (manor to gateway)": road,
        "Town spur (junction to store)": spur,
        "Cove route": densify(cove[::5]),
        "Lake path": densify(layout["lake"]["path"]),
        "Manor to mine": densify([manor, land["MineEntrance"][:2]]),
        "Manor to mill": densify([manor, land["MillSite"][:2]]),
    }


def sights(layout, scenery):
    pts = []
    for name, pattern in (("HomesteadEstateForagePlacements.inc", r"forage\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)"),
                          ("HomesteadEstateBerryPlacements.inc", r"berry\(\d+, (-?[\d.]+), (-?[\d.]+)"),
                          ("HomesteadEstateLakePathPlacements.inc", r"lakeForage\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)"),
                          ("HomesteadEstateRoadsidePlacements.inc", r"roadside\(\d+, ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)")):
        pts.extend(inc_points(name, pattern))
    flowers = scenery[np.isin(scenery["k"], FLOWER_KINDS)]
    pts.extend(zip(flowers["x"] / 100.0, flowers["y"] / 100.0))
    for key, value in layout["landmarks"].items():
        if key not in ("StandingRoomOrigin", "RoadEstateEnd", "RoadTownEnd"):
            pts.append(tuple(value[:2]))
    cove = json.loads((HERE / "cove_route.json").read_text())
    pts.extend((p["x"], p["y"]) for p in cove["fingerposts"])
    bridge = layout.get("roadBridge", {})
    if "centre" in bridge:
        pts.append(tuple(bridge["centre"][:2]))
    return np.array(pts, float)


def chainages(route, points):
    """Chainage (m along the route) of each sight within SIGHT_RADIUS_M."""
    from scipy.spatial import cKDTree
    tree = cKDTree(route)
    dist, idx = tree.query(points)
    keep = dist <= SIGHT_RADIUS_M
    return np.sort(idx[keep] * STEP_M)


def gaps(route, points):
    length = (len(route) - 1) * STEP_M
    marks = np.r_[0.0, chainages(route, points), length]
    return [(a, b) for a, b in zip(marks[:-1], marks[1:]) if b - a > MAX_GAP_M], length


def drift(route, ch, side, rng, habitat):
    """One flower drift beside the route at chainage ch; tries both sides and a few offsets."""
    k = min(int(ch / STEP_M), len(route) - 2)
    tangent = route[k + 1] - route[k]
    tangent /= max(np.hypot(*tangent), 1e-9)
    normal = np.array([-tangent[1], tangent[0]])
    best = []
    for s in (side, -side):
        for offset in np.linspace(FILL_OFFSET_M[0], FILL_OFFSET_M[1] + 4.0, 5):
            centre = route[k] + normal * s * offset
            kind = int(rng.choice((43, 44, 46, 48, 45)))
            number = int(rng.integers(*DRIFT_CLUMPS))
            pts = centre + rng.normal(0.0, DRIFT_RADIUS_M * 0.5, (number, 2))
            pts = pts[habitat.allowed(pts)]
            if len(pts) > len(best):
                best = [(kind, TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(0, 360),
                         FLOWER_SCALE[kind] * rng.uniform(0.85, 1.2)) for p in pts]
            if len(best) >= DRIFT_CLUMPS[0]:
                return best, s
    return best, side


def fill(route, spans, rng, habitat):
    out, side = [], 1.0
    for a, b in spans:
        n = int((b - a) // MAX_GAP_M)
        for i in range(1, n + 1):
            records, side = drift(route, a + (b - a) * i / (n + 1), side, rng, habitat)
            out.extend(records)
            side = -side
    return out



def store_front(layout, rng):
    """Flower drifts either side of the walk up to the store door, clear of every footprint and the street."""
    town = layout["town"]
    door = np.array(layout["landmarks"]["GeneralStoreDoor"][:2])
    street = densify(town["street"], 0.5)
    store = np.array(town["store"]["footprint"])
    lo, hi = store.min(axis=0) - STORE_CLEAR_M, store.max(axis=0) + STORE_CLEAR_M
    blocks = [(np.array([b["x"], b["y"]]), 0.5 * np.hypot(b["width"], b["depth"]) + STORE_CLEAR_M)
              for b in town["buildings"]]
    out = []
    for dx in (-5.0, 5.0, -7.5, 7.5):
        for dy in (-5.0, -8.0):
            kind = int(rng.choice((43, 44, 46, 48)))
            for p in np.array([door[0] + dx, door[1] + dy]) + rng.normal(0.0, 1.0, (STORE_CLUMPS, 2)):
                if np.all(p > lo) and np.all(p < hi):
                    continue
                if any(np.hypot(*(c - p)) < r for c, r in blocks):
                    continue
                if np.hypot(*(street - p).T).min() < STREET_CLEAR_M:
                    continue
                out.append((kind, TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(0, 360),
                            FLOWER_SCALE[kind] * rng.uniform(0.85, 1.2)))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bake", action="store_true")
    args = parser.parse_args()
    layout = json.loads((HERE / "estate_layout.json").read_text(encoding="utf-8"))
    scenery = read_scenery()
    base = scenery[scenery["pad"] != TAG]
    if args.bake:
        import sys
        sys.path.insert(0, str(HERE))
        import estate_wildflowers as wf
        habitat = wf.Habitat(layout, base)
        rng = np.random.default_rng(SEED)
        records = []
        all_routes = routes(layout)
        found = sights(layout, base)
        for name, route in all_routes.items():
            spans, _ = gaps(route, found)
            arr = np.array(fill(route, spans, rng, habitat), dtype=RECORD)
            records.append(arr)
            found = np.r_[found, np.c_[arr["x"], arr["y"]] / 100.0]
        records.append(np.array(store_front(layout, rng), dtype=RECORD))
        extra = np.concatenate(records) if records else np.array([], dtype=RECORD)
        result = np.r_[base, extra]
        SCENERY.write_bytes(b"HSC1" + struct.pack("<I", len(result)) + result.tobytes())
        scenery = result
        print(f"baked {len(extra)} RS1 flower records; scenery now {len(result)}")
    found = sights(layout, scenery)
    for name, route in routes(layout).items():
        spans, length = gaps(route, found)
        worst = max([b - a for a, b in spans], default=0.0)
        print(f"{name}: {length:.0f} m, {len(spans)} gaps over {MAX_GAP_M:.0f} m, worst {worst:.0f} m")
        for a, b in spans:
            print(f"    {a:.0f}-{b:.0f} m ({b - a:.0f} m)")


if __name__ == "__main__":
    main()
