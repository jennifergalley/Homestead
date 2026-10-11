"""Clear the mine yard's trees and shrubs so the mine house is seen from the path (Jenny, blockout review:
"Clear the trees around the yard so the house is seen from the path").

    python Scripts\\Mine\\clear_mine_yard.py

Post-filters the committed decorative scenery (Content/SurvivalGame/Estate/Runtime/EstateScenery.bin,
written by Scripts/Terrain/scatter.py, which keeps 20 m clear of MineEntrance): trees and shrubs go
within ``YARD_CLEAR_M`` of the house and within a ``PATH_CLEAR_M`` corridor along the last stretch of the
Mine footpath, and anything at all goes inside the house's footprint. Decorative scenery has no
placement ids, so no saved state changes. Idempotent; rerun after any scatter.py regeneration.
Re-bake the ground afterwards (Scripts/Terrain/bake_ground.py), since its leaf litter follows the trees.
"""
import json
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
from mine import layout as L  # noqa: E402

SCENERY = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateScenery.bin"
LAYOUT = ROOT / "Scripts" / "Terrain" / "estate_layout.json"
RECORD = np.dtype([("k", "u1"), ("pad", "u1", 3), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
# Kind bytes (EstateSceneryKinds in HomesteadWorldEstate.cpp).
TREES = {0, 1, 13, 14, 15, 16, 19, 20}
SHRUBS = {2, 12, 17, 18, 21, 22, 23, 24, 25, 26, 27}
YARD_CLEAR_M = 24.0         # round the house centre
PATH_CLEAR_M = 9.0          # each side of the footpath's centreline (trees); shrubs half that
PATH_TAIL_M = 70.0          # how far back up the path the corridor runs from the yard
HOUSE_MARGIN_M = 1.0        # nothing inside the footprint plus this


def path_tail():
    layout = json.loads(LAYOUT.read_text())
    pts = next(np.array(f["points"], float) for f in layout["footpaths"] if f["name"] == "Mine")
    # Walk back from the yard end until PATH_TAIL_M is covered.
    out, walked = [pts[-1]], 0.0
    for a, b in zip(pts[::-1][:-1], pts[::-1][1:]):
        seg = np.linalg.norm(b - a)
        if walked + seg >= PATH_TAIL_M:
            out.append(a + (b - a) * (PATH_TAIL_M - walked) / seg)
            break
        out.append(b)
        walked += seg
    return np.array(out)


def segment_distance(p, a, b):
    ab = b - a
    t = np.clip(((p - a) @ ab) / max(ab @ ab, 1e-9), 0.0, 1.0)
    return np.linalg.norm(p - (a + t[:, None] * ab), axis=1)


def keep_mask(rec):
    p = np.c_[rec["x"], rec["y"]] / 100.0
    hx, hy, _ = L.HOUSE_WORLD
    d_house = np.hypot(p[:, 0] - hx, p[:, 1] - hy)
    tail = path_tail()
    d_path = np.min([segment_distance(p, a, b) for a, b in zip(tail[:-1], tail[1:])], axis=0)
    kind = rec["k"]
    tree = np.isin(kind, list(TREES))
    shrub = np.isin(kind, list(SHRUBS))
    drop = tree & ((d_house < YARD_CLEAR_M) | (d_path < PATH_CLEAR_M))
    drop |= shrub & ((d_house < YARD_CLEAR_M * 0.8) | (d_path < PATH_CLEAR_M * 0.5))
    inside = (np.abs(p[:, 0] - hx) < L.OUT_X / 2 + HOUSE_MARGIN_M) & (np.abs(p[:, 1] - hy) < L.OUT_Y / 2 + HOUSE_MARGIN_M)
    return ~(drop | inside)


def main():
    raw = SCENERY.read_bytes()
    count = int.from_bytes(raw[4:8], "little")
    rec = np.frombuffer(raw[8:], dtype=RECORD)
    assert raw[:4] == b"HSC1" and len(rec) == count
    keep = keep_mask(rec)
    kept = rec[keep]
    SCENERY.write_bytes(b"HSC1" + int(len(kept)).to_bytes(4, "little") + kept.tobytes())
    print(f"mine yard: removed {int((~keep).sum())} of {count} scenery records")


if __name__ == "__main__":
    main()
