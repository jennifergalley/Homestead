"""Find stretches of walking route with nothing to see, and fill them with original flower drifts.

    python Scripts\\Terrain\\route_sights.py            # report gaps only
    python Scripts\\Terrain\\route_sights.py --bake     # add 'RS1' flower drifts and re-report
    python Scripts\\Terrain\\route_sights.py --top-up   # keep the RS1 drifts there are; add drifts only in the gaps left

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
# Bloom-heavy kinds for the planted beds and drifts (Balance, cohesion.md section 4): bluebell, primrose,
# red campion and foxglove show colour from 10 m; wild garlic, cow parsley and wood anemone read as green leaves.
BLOOM_KINDS = (42, 43, 46, 47, 47, 46)
FLOWER_SCALE = {42: 1.0, 43: 1.0, 44: 1.0, 45: 1.0, 46: 1.0, 47: 1.0, 48: 1.0}
SIGHT_RADIUS_M = 15.0        # a flower drift or forage point this close to the route is in view
MAX_GAP_M = 50.0             # Jenny: a scenery or forage point about every 50 m at a sprint
FILL_OFFSET_M = (5.5, 8.0)   # drifts sit just off the verge, either side in turn
DRIFT_CLUMPS = (6, 10)
DRIFT_RADIUS_M = 2.2
SEED = 20261004
KEEP_CLEAR_M = 3.0           # off the walked line itself
STEP_M = 1.0
BED_SCALE = 2.6           # village beds are planted thick, so the clumps read from across the square
STREET_CLEAR_M = 3.2      # and this far from the street centreline (half width 2.75 m)


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
        "Manor to mine": densify(next((p["points"] for p in layout.get("footpaths", []) if p.get("name") == "Mine"),
                                      [manor, land["MineEntrance"][:2]])),
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
            kind = int(rng.choice(BLOOM_KINDS))
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



def bed_record(rng, p, kinds=BLOOM_KINDS, grow=BED_SCALE):
    kind = int(rng.choice(kinds))
    return (kind, TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(0, 360), FLOWER_SCALE[kind] * grow * rng.uniform(0.85, 1.2))


def facades(layout):
    """(front centre, outward unit vector, side unit vector, width, depth, door offset m) for every building."""
    town = layout["town"]
    rows = []
    for b in town["buildings"]:
        yaw = np.radians(b["yaw"])
        out = np.array([-np.cos(yaw), -np.sin(yaw)])
        side = np.array([-out[1], out[0]])
        door = abs(b["doorAt"]) * max(0.0, b["width"] / 2 - 1.1 - 0.6)
        rows.append((np.array([b["x"], b["y"]]), out, side, b["width"], b["depth"], door))
    store = np.array(town["store"]["footprint"])
    door = np.array(layout["landmarks"]["GeneralStoreDoor"][:2])
    lo, hi = store.min(axis=0), store.max(axis=0)
    rows.append((door + np.array([0.0, 0.5]), np.array([0.0, 1.0]), np.array([1.0, 0.0]), hi[0] - lo[0], hi[1] - lo[1], 0.0))
    return rows


def inside_any(p, rows, margin):
    for front, out, side, width, depth, _ in rows:
        rel = p - front
        u, v = float(rel @ out), float(rel @ side)
        if -depth - margin < u < margin and abs(v) < width / 2 + margin:
            return True
    return False


def village_flowers(layout, rng, habitat):
    """Beds along every facade either side of the door steps, and drifts where the spur leaves the road.

    The door's side is not recorded here, so beds keep clear of the door's offset on both sides of the centre.
    """
    town = layout["town"]
    street = densify(town["street"], 0.5)
    road = densify(layout["road"], 0.5)
    rows = facades(layout)
    out = []

    def clear_of_street(p):
        return np.hypot(*(street - p).T).min() >= STREET_CLEAR_M

    for front, normal, side, width, depth, door in rows:
        half = width / 2 - 0.5
        for v in np.arange(-half, half + 0.01, 0.55):
            if abs(abs(v) - door) < 1.9 or (door == 0.0 and abs(v) < 2.0):
                continue
            for _ in range(4):
                p = front + normal * rng.uniform(0.9, 1.5) + side * (v + rng.uniform(-0.3, 0.3))
                if inside_any(p, rows, 0.4) or not clear_of_street(p):
                    continue
                out.append(bed_record(rng, p))
    # Junction: drifts on both sides of the road and of the spur just past it.
    j = min(int(town["junctionChainage"] / 0.5), len(road) - 2)
    for centre_line, base, n in ((road, j, 80), (road, j + 40, 80), (street, 12, 40), (street, 32, 40)):
        for _ in range(n):
            k = min(base + int(rng.integers(-16, 16)), len(centre_line) - 2)
            tangent = centre_line[k + 1] - centre_line[k]
            tangent /= max(np.hypot(*tangent), 1e-9)
            normal = np.array([-tangent[1], tangent[0]])
            p = centre_line[k] + normal * rng.choice((-1.0, 1.0)) * rng.uniform(4.2, 7.5) + rng.normal(0.0, 0.6, 2)
            if not habitat.allowed(p[None, :])[0] or inside_any(p, rows, 1.0) or not clear_of_street(p):
                continue
            if np.hypot(*(road - p).T).min() < STREET_CLEAR_M:
                continue
            out.append(bed_record(rng, p))
    return out

def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bake", action="store_true")
    parser.add_argument("--top-up", action="store_true", help="keep existing RS1 drifts; fill only the gaps left "
                        "(the compact map's estate is smaller than the drifts the full bake planted outside it)")
    args = parser.parse_args()
    layout = json.loads((HERE / "estate_layout.json").read_text(encoding="utf-8"))
    scenery = read_scenery()
    base = scenery if args.top_up else scenery[scenery["pad"] != TAG]
    if args.bake or args.top_up:
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
        if not args.top_up:
            records.append(np.array(village_flowers(layout, rng, habitat), dtype=RECORD))
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
