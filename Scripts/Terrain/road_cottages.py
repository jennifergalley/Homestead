"""Labourers' cottages strung along the road out of the village (shrink-estate-map, Jenny 2026-10-10).

Usage: python Scripts/Terrain/road_cottages.py [--dry]

Six small rubble cottages front the public road beyond the village (road chainage SLOTS), each SETBACK_M off the
road's centreline on alternating sides, facing it, on a small level pad (blended into the ground, never within
ROAD_KEEP_M of the centreline so the road and its verge stay as graded). A slot moves along the road (up to
SLIDE_M) until no saved placement and no part of the bridge keep-out is within its footprint plus CLEAR_M.

Run once ("roadCottagesGraded" in the layout); a rerun only re-emits "roadCottages". Writes EstateHeightfield.r16,
Estate_Heightmap_4033.png and the work npy where the pads changed, "roadCottages" in estate_layout.json (the same
schema as town["buildings"]), and clears EstateScenery.bin records off the footprints. Then, in the editor,
Content/Python/homestead_agent/town_massing.py places them with the village's buildings; bake_ground.py and
bake_estate_map.py draw them.
"""
import glob
import json
import math
import os
import re
import sys
import time

import numpy as np
from PIL import Image
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIM = os.path.join(REPO, "Source", "SurvivalGame", "Simulation")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
SIZE, H = 4033, 2016
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])

SLOTS = [455.0, 485.0, 520.0, 548.0, 585.0, 618.0]   # road chainage (m), past the village's street junction (260 m)
SETBACK_M = 6.0              # the cottage's front from the road's centreline (5 m bed, then its verge)
SLIDE_M = 14.0
CLEAR_M = 2.0
ROAD_KEEP_M = 3.5
PAD_MARGIN_M = 1.0
PAD_BLEND_M = 4.0
BRIDGE_KEEP_M = 25.0         # from the bridge's centre (public_road.py keeps 20 m)
# width, depth, storey height (cm), pitch, roof, tint, chimneys (+1 right, -1 left), door at: small labourers' cottages.
STYLES = [
    (6.0, 5.5, 270, 50, "RidgeAlongStreet", (0.93, 0.92, 0.9), 1, -0.3),
    (6.5, 5.5, 265, 48, "GableToStreet", (0.97, 0.95, 0.9), -1, 0.25),
    (5.5, 5.0, 260, 52, "RidgeAlongStreet", (0.9, 0.9, 0.88), 1, 0.2),
    (6.0, 5.5, 270, 50, "RidgeAlongStreet", (1.0, 0.97, 0.92), -1, -0.25),
    (7.0, 5.5, 270, 46, "GableToStreet", (0.94, 0.94, 0.92), 1, -0.4),
    (5.5, 5.0, 260, 50, "RidgeAlongStreet", (0.96, 0.93, 0.88), 1, 0.3),
]


def retried(write, tries=20):
    """A write retried for a while: the heightfield just written by the previous script is sometimes still held
    briefly by another process (Errno 22 on Windows)."""
    for k in range(tries):
        try:
            return write()
        except OSError:
            if k == tries - 1:
                raise
            time.sleep(1.0)


def road_frame(layout):
    road = np.asarray(layout["road"], np.float64)
    chain = np.r_[0.0, np.cumsum(np.hypot(*np.diff(road, axis=0).T))]

    def at(s):
        i = int(np.clip(np.searchsorted(chain, s), 1, len(road) - 1))
        a, b = road[i - 1], road[i]
        t = (s - chain[i - 1]) / max(chain[i] - chain[i - 1], 1e-9)
        u = (b - a) / max(np.hypot(*(b - a)), 1e-9)
        return a + (b - a) * t, u
    return road, at


def corners(x, y, yaw, width, depth, grow=0.0):
    t = math.radians(yaw)
    out, along = np.array([math.cos(t), math.sin(t)]), np.array([-math.sin(t), math.cos(t)])
    front = np.array([x, y])
    return np.array([front + along * a + out * d for a, d in ((-width / 2 - grow, -grow), (width / 2 + grow, -grow),
                                                              (width / 2 + grow, depth + grow), (-width / 2 - grow, depth + grow))])


def placements():
    out = []
    for f in glob.glob(os.path.join(SIM, "HomesteadEstate*Placements.inc")):
        for m in re.finditer(r"\w+\((\d{6}),\s*(?:ResourceKind::\w+,\s*)?(-?[\d.]+),\s*(-?[\d.]+)\)", open(f).read()):
            out.append((float(m[2]) / 100.0, float(m[3]) / 100.0))
    return cKDTree(np.array(out))


def inside(poly, pts):
    from matplotlib.path import Path
    return Path(poly).contains_points(pts)


def design(layout):
    road, at = road_frame(layout)
    taken = placements()
    bridge = np.asarray(layout["roadBridge"]["centre"], np.float64)
    rows = []
    for k, s0 in enumerate(SLOTS):
        width, depth, storey, pitch, roof, tint, chimneys, door = STYLES[k]
        side = 1.0 if k % 2 else -1.0
        for slide in sorted(np.arange(-SLIDE_M, SLIDE_M + 0.1, 1.0), key=abs):
            p, u = at(s0 + slide)
            n = np.array([-u[1], u[0]]) * side
            front = p + n * SETBACK_M
            yaw = math.degrees(math.atan2(n[1], n[0]))
            box = corners(front[0], front[1], yaw, width, depth, CLEAR_M)
            near = taken.data[taken.query_ball_point(front, 20.0)]
            if (len(near) and inside(box, near).any()) or np.hypot(*(front - bridge)) < BRIDGE_KEEP_M:
                continue
            rows.append({"name": f"RoadCottage{k + 1}", "x": round(float(front[0]), 2), "y": round(float(front[1]), 2),
                         "yaw": round(yaw, 1), "width": width, "depth": depth, "storeys": 1, "storeyHeight": storey,
                         "pitch": pitch, "roof": roof, "wall": "Rubble", "tint": list(tint), "chimneys": chimneys,
                         "doorAt": door, "shopfront": False, "sign": [0.1, 0.07, 0.03], "sideWindows": True,
                         "chainage": round(float(s0 + slide), 1)})
            break
        else:
            raise SystemExit(f"road cottages: no clear spot near chainage {s0} m")
    return rows, road


def grade_pads(z, rows, road):
    tree = cKDTree(np.asarray(road, np.float64))
    changed = np.zeros_like(z, bool)
    for r in rows:
        box = corners(r["x"], r["y"], r["yaw"], r["width"], r["depth"], PAD_MARGIN_M)
        reach = PAD_BLEND_M + 1
        x0, x1 = int(box[:, 0].min() - reach), int(box[:, 0].max() + reach)
        y0, y1 = int(box[:, 1].min() - reach), int(box[:, 1].max() + reach)
        X, Y = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
        pts = np.c_[X.ravel(), Y.ravel()].astype(np.float64)
        t = math.radians(r["yaw"])
        out, along = np.array([math.cos(t), math.sin(t)]), np.array([-math.sin(t), math.cos(t)])
        rel = pts - np.array([r["x"], r["y"]])
        a, d = rel @ along, rel @ out
        da = np.clip(np.abs(a) - (r["width"] / 2 + PAD_MARGIN_M), 0, None)
        dd = np.clip(np.maximum(-PAD_MARGIN_M - d, d - r["depth"] - PAD_MARGIN_M), 0, None)
        dist = np.hypot(da, dd).reshape(X.shape)
        old = z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1]
        level = float(old[dist == 0].mean())
        tt = np.clip(dist / PAD_BLEND_M, 0, 1)
        w = 1.0 - tt * tt * (3 - 2 * tt)
        w *= (tree.query(pts)[0].reshape(X.shape) > ROAD_KEEP_M)
        z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] = old + (level - old) * w
        changed[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] |= w > 0
        r["padZ"] = round(level, 2)
    return changed


def clear_scenery(rows, dry):
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], RECORD)
    at = np.c_[rec["x"] / 100.0, rec["y"] / 100.0]
    drop = np.zeros(count, bool)
    for r in rows:
        box = corners(r["x"], r["y"], r["yaw"], r["width"], r["depth"], 1.5)
        near = (np.abs(at[:, 0] - r["x"]) < 20) & (np.abs(at[:, 1] - r["y"]) < 20)
        idx = np.flatnonzero(near)
        drop[idx[inside(box, at[idx])]] = True
    if drop.any() and not dry:
        kept = rec[~drop]
        with open(SCENERY, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(kept)).tobytes() + kept.tobytes() + raw[8 + count * 20:])
    return int(drop.sum())


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    rows, road = design(layout)
    for r in rows:
        print(f"{r['name']}: chainage {r['chainage']} m, ({r['x']}, {r['y']}), yaw {r['yaw']}")
    if layout.get("roadCottagesGraded"):
        old = {r["name"]: r.get("padZ") for r in layout.get("roadCottages", [])}
        for r in rows:
            r["padZ"] = old.get(r["name"])
        print("road cottages: pads already graded")
    else:
        raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
        z = (raw.astype(np.float64) - 32768.0) / 128.0
        grade_pads(z, rows, road)
        u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
        moved = np.argwhere(u16 != raw)
        if dry:
            print(f"road cottages: {len(moved)} vertices to level")
            return
        retried(lambda: u16.astype("<u2").tofile(R16))
        retried(lambda: Image.fromarray(u16).save(PNG))
        cleared = retried(lambda: clear_scenery(rows, dry))
        print(f"road cottages: {len(moved)} vertices levelled, {cleared} scenery records cleared")
        npy = os.path.join(WORK, "game_reshaped_4033.npy")
        if os.path.exists(npy) and len(moved):
            zr = (u16.astype(np.float64) - 32768.0) / 128.0
            zn = np.load(npy)
            rr, cc = moved[:, 0], moved[:, 1]
            zn[2 * H - cc, rr] = zr[rr, cc].astype(zn.dtype)
            np.save(npy, zn)
        if len(moved):
            (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
            print(f"road cottages: ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")
        layout["roadCottagesGraded"] = True
    if dry:
        return
    layout["roadCottages"] = rows
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)


if __name__ == "__main__":
    main()
