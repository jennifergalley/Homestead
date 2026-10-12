"""Wildflowers round the mine house and along the last stretch of its path (decorative EstateScenery records).

    python Scripts\\Mine\\mine_flowers.py          # idempotent: replaces its own records (tagged MF1)

Balance look check (2026-10-11), endorsed for the mine house: "Flowers (cheap, reusing existing kinds):
foxgloves and campion at the wall foot and by the door, and some along the last stretch of path, so the approach
isn't a grey tower on lawn." Reuses lake_path_plants.py's bloom kinds (red campion 46, foxglove 47, cow parsley 48,
primrose 43) at its FLOWER_SCALE. Scripts/Mine/clear_mine_yard.py keeps MF1 records.
"""
import json
import struct
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
from mine import layout as L  # noqa: E402

SCENERY = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateScenery.bin"
LAYOUT = ROOT / "Scripts" / "Terrain" / "estate_layout.json"
RECORD = np.dtype([("k", "u1"), ("pad", "u1", 3), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
TAG = np.frombuffer(b"MF1", dtype="u1")
SEED = 20261011
RED_CAMPION, FOXGLOVE, COW_PARSLEY, PRIMROSE = 46, 47, 48, 43
FLOWER_SCALE = {43: 1.8, 46: 1.35, 47: 1.15, 48: 1.3}       # lake_path_plants.FLOWER_SCALE
WALL_FOOT_M = (0.2, 0.9)       # out from the wall face
WALL_SPECIES = [(FOXGLOVE, 0.45), (RED_CAMPION, 0.4), (COW_PARSLEY, 0.15)]
DOOR_CLEAR_M = 1.0             # half-width kept clear in front of the door (door 1.5 m wide)
PATH_TAIL_M = 28.0             # verges planted along this much of the path before the door
PATH_VERGE_M = (0.9, 2.2)      # either side of the path's centreline
PATH_SPECIES = [(RED_CAMPION, 0.35), (FOXGLOVE, 0.25), (PRIMROSE, 0.2), (COW_PARSLEY, 0.2)]


def pick(rng, species):
    kinds, weights = zip(*species)
    return int(rng.choice(kinds, p=np.array(weights) / sum(weights)))


def drift(rng, out, kind, cx, cy, n, spread):
    for _ in range(n):
        out.append((kind, cx + rng.normal(0, spread), cy + rng.normal(0, spread), rng.uniform(0, 360),
                    FLOWER_SCALE[kind] * rng.uniform(0.85, 1.15)))


def plant(rng):
    hx, hy, _ = L.HOUSE_WORLD
    ax, ay = L.OUT_X / 2, L.OUT_Y / 2
    out = []
    # Wall foot: drifts every metre or two round the house, none in front of the door.
    for side, (x0, y0, x1, y1, nx, ny) in {
            "E": (ax, -ay, ax, ay, 1, 0), "W": (-ax, -ay, -ax, ay, -1, 0),
            "N": (-ax, ay, ax, ay, 0, 1), "S": (-ax, -ay, ax, -ay, 0, -1)}.items():
        length = abs(x1 - x0) + abs(y1 - y0)
        t = rng.uniform(0.2, 0.8)
        while t < length - 0.2:
            px, py = x0 + (x1 - x0) * t / length, y0 + (y1 - y0) * t / length
            if not (side == "E" and abs(py) < DOOR_CLEAR_M + 0.3):
                off = rng.uniform(*WALL_FOOT_M)
                drift(rng, out, pick(rng, WALL_SPECIES), hx + px + nx * off, hy + py + ny * off,
                      int(rng.integers(2, 5)), 0.18)
            t += rng.uniform(0.9, 2.2)
    # By the door: a drift either side.
    for s in (-1, 1):
        drift(rng, out, FOXGLOVE, hx + ax + 0.5, hy + s * (DOOR_CLEAR_M + 0.5), 4, 0.2)
        drift(rng, out, RED_CAMPION, hx + ax + 1.3, hy + s * (DOOR_CLEAR_M + 0.9), 3, 0.25)
    # The path's last stretch: verge drifts on both sides.
    path = np.asarray(next(f["points"] for f in json.loads(LAYOUT.read_text())["footpaths"] if f["name"] == "Mine"))
    seg = np.r_[0, np.cumsum(np.linalg.norm(np.diff(path, axis=0), axis=1))]
    total = seg[-1]
    s = total - PATH_TAIL_M
    while s < total - 3.0:
        p = np.array([np.interp(s, seg, path[:, 0]), np.interp(s, seg, path[:, 1])])
        q = np.array([np.interp(s + 0.5, seg, path[:, 0]), np.interp(s + 0.5, seg, path[:, 1])])
        d = (q - p) / max(np.linalg.norm(q - p), 1e-6)
        normal = np.array([-d[1], d[0]])
        side = rng.choice([-1, 1])
        c = p + normal * side * rng.uniform(*PATH_VERGE_M)
        drift(rng, out, pick(rng, PATH_SPECIES), c[0], c[1], int(rng.integers(3, 7)), 0.35)
        s += rng.uniform(2.0, 4.0)
    # Never inside the walls.
    keep = [r for r in out if abs(r[1] - hx) > ax + 0.12 or abs(r[2] - hy) > ay + 0.12]
    rec = np.zeros(len(keep), RECORD)
    for i, (k, x, y, yaw, sc) in enumerate(keep):
        rec[i] = (k, TAG, x * 100.0, y * 100.0, yaw, sc)
    return rec


def main():
    raw = SCENERY.read_bytes()
    count = struct.unpack_from("<I", raw, 4)[0]
    rec = np.frombuffer(raw[8:], RECORD)
    assert raw[:4] == b"HSC1" and len(rec) == count
    rest = rec[~(rec["pad"] == TAG).all(axis=1)]
    new = plant(np.random.default_rng(SEED))
    out = np.concatenate([rest, new])
    SCENERY.write_bytes(b"HSC1" + struct.pack("<I", len(out)) + out.tobytes())
    print(f"mine flowers: {len(new)} records ({count - len(rest)} replaced), {len(out)} scenery records")


if __name__ == "__main__":
    main()
