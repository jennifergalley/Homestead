"""Stage 1 of shrink-estate-map: carve the town pocket out of the estate and clear its footprint.

Usage: python Scripts/Terrain/shrink_map.py [--dry]    (after town_pad.py and town_layout.py)

1. Notches the EstateBoundary: a slit from its north edge (x = 160) down to a pocket round the square, so the new
   town stays public land even though the drive now reaches it from inside the estate.
2. Drops the baked scatter inside the square and along the street (EstateScenery.bin), and thins the trees, shrubs
   and rocks round them.
3. Ends the public road a short way past the estate gateway (ROAD_PAST_GATE_M), so no road runs on to the old town
   site; public_road.py then writes the shorter road and the Town stop at the junction.
4. Drops the estate forage placements that fall in the pocket or on the street (ids are never reused).

Prints what it changed; a rerun changes nothing. Mirror the notch in HomesteadEstate.cpp (cm) by hand.
"""
import json
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
LAYOUT = os.path.join(HERE, "estate_layout.json")
SCENERY = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
FORAGE = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateForagePlacements.inc")

SLIT_Y_M = (-624.0, -618.0)          # a 6 m slit: no hedge, wall or placement can fit in it
POCKET_X_M = (-134.0, -62.0)         # the pocket round the square, its street end and the store
POCKET_Y_M = (-646.0, -570.0)
BOUNDARY_EDGE_X_M = 160.0

CORE_PAD_M = 0.0                     # scatter inside the square's graded core is removed outright
STREET_BAND_M = 4.0
THIN_PAD_M = 12.0                    # beyond the core, trees, shrubs and rocks are thinned out to this far
GROUND_COVER = {3, 4, 9, 10, 11}     # bracken, yarrow, fern, tall and mid grass stay in the open
FORAGE_STREET_BAND_M = 5.0
GATEWAY_M = (-55.0, 90.0)
ROAD_PAST_GATE_M = 32.0              # the drive ends here, at a scenic terminus


def notched(boundary):
    """The boundary with the slit and pocket cut in along its x = BOUNDARY_EDGE_X_M edge."""
    pts = [list(p) for p in boundary]
    x0, x1 = POCKET_X_M
    y0, y1 = POCKET_Y_M
    cut = [[BOUNDARY_EDGE_X_M, SLIT_Y_M[0]], [x1, SLIT_Y_M[0]], [x1, y0], [x0, y0], [x0, y1], [x1, y1],
           [x1, SLIT_Y_M[1]], [BOUNDARY_EDGE_X_M, SLIT_Y_M[1]]]
    if cut[0] in pts:
        return pts, False
    i = next(k for k, p in enumerate(pts) if p == [BOUNDARY_EDGE_X_M, -1150.0])
    assert pts[i + 1] == [BOUNDARY_EDGE_X_M, -250.0], "boundary's x = 160 edge not where expected"
    return pts[:i + 1] + cut + pts[i + 1:], True


def street_distance(street, x, y):
    return np.min(np.hypot(x[:, None] - street[None, :, 0], y[:, None] - street[None, :, 1]), axis=1)


def in_rect(x, y, centre, half, pad):
    return (np.abs(x - centre[0]) < half[0] + pad) & (np.abs(y - centre[1]) < half[1] + pad)


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    town = layout["town"]
    pad = layout["townPad"]
    centre, core = pad["centre"], pad["coreHalf"]
    street = np.array(town["street"])

    boundary, changed = notched(layout["polygons"]["EstateBoundary"])
    print("boundary", "notched" if changed else "already notched", len(boundary), "points")
    if changed and not dry:
        layout["polygons"]["EstateBoundary"] = boundary
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)

    road = np.asarray(layout["road"], np.float64)
    chain = np.r_[0.0, np.cumsum(np.hypot(*np.diff(road, axis=0).T))]
    gate = int(np.argmin(np.hypot(*(road - np.array(GATEWAY_M)).T)))
    end = min(int(np.searchsorted(chain, chain[gate] + ROAD_PAST_GATE_M)) + 1, len(road))
    print("road", len(road), "points ->", end, f"(ends at {chain[end - 1]:.1f} m)")
    if end < len(road) and not dry:
        layout["road"] = layout["road"][:end]
        layout["roadProfile"] = layout["roadProfile"][:end]
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)

    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"),
                                                        ("yaw", "<f4"), ("s", "<f4")]))
    x, y = rec["x"] / 100.0, rec["y"] / 100.0
    near_street = street_distance(street, x, y) < STREET_BAND_M
    drop = in_rect(x, y, centre, core, CORE_PAD_M) | near_street
    thin = in_rect(x, y, centre, core, THIN_PAD_M) & ~np.isin(rec["k"], list(GROUND_COVER))
    drop |= thin
    print("scenery", int(drop.sum()), "of", count, "records in the town footprint")
    if drop.any() and not dry:
        keep = rec[~drop]
        with open(SCENERY, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(keep)).tobytes() + keep.tobytes() + raw[8 + count * 20:])

    pocket = (x0, x1, y0, y1) = POCKET_X_M + POCKET_Y_M
    lines = open(FORAGE, newline="").read().split("\r\n" if "\r\n" in open(FORAGE, newline="").read() else "\n")
    pattern = re.compile(r"forage\((\d+), ResourceKind::\w+, (-?[\d.]+), (-?[\d.]+)\);")
    kept, removed = [], []
    for line in lines:
        m = pattern.match(line)
        if m:
            px, py = float(m[2]) / 100.0, float(m[3]) / 100.0
            in_pocket = POCKET_X_M[0] <= px <= POCKET_X_M[1] and POCKET_Y_M[0] <= py <= POCKET_Y_M[1]
            on_street = street_distance(street, np.array([px]), np.array([py]))[0] < FORAGE_STREET_BAND_M
            if in_pocket or on_street:
                removed.append(int(m[1]))
                continue
        kept.append(line)
    print("forage removed", removed)
    if removed and not dry:
        newline = "\r\n" if "\r\n" in open(FORAGE, newline="").read() else "\n"
        with open(FORAGE, "w", newline="") as fh:
            fh.write(newline.join(kept))


if __name__ == "__main__":
    main()
