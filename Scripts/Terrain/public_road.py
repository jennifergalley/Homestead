"""Generate the runtime public road: the manor-to-town road's centreline in world centimetres with its
chainage, the safe travel endpoints and sign anchors, and the bridge keep-out.

    python Scripts\\Terrain\\public_road.py
        -> Source/SurvivalGame/Simulation/HomesteadEstatePublicRoad.inc

Reads Scripts/Terrain/estate_layout.json ("road", "roadProfile", "river", "riverSurface", "roadGrade") and the
heightfield (for sign heights and the bridge deck). It records the deck as "roadBridge" in the layout (for
bake_ground.py) and clears the baked scatter off it (EstateScenery.bin), so no grass or fern pokes through
the planks; rerunning it changes nothing more. The road's chainage (metres along it from the manor forecourt) is the one the
travel, sign and roadside-forage lanes quote, so the points are the layout's own 4 m samples, unchanged.
"""
import json
import math
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
LAYOUT = os.path.join(HERE, "estate_layout.json")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
OUT = os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "HomesteadEstatePublicRoad.inc")
SIZE, H = 4033, 2016

SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
BRIDGE_CLEAR_MARGIN_M = 0.8      # scatter kept this far clear of the deck and its railings
SIGN_OFFSET_M = 4.2            # a sign stands this far off the centreline, on the verge
BRIDGE_HALF_ALONG_M = 20.0     # the bridge keep-out: deck and ramps (chainage 664-704 about the crossing)
BRIDGE_HALF_ACROSS_M = 6.0
# The road bridge's deck (road_grade.py holds the road level over the river): it rests this far past the
# point each way where the graded bank rises to the deck, on a stone abutment, and is this wide between
# its railings (a single cart's width).
BRIDGE_BEARING_M = 1.0
BRIDGE_SEAT_M = 0.25             # the bank counts as reached within this of the deck (the abutment makes up the rest)
BRIDGE_CLEAR_HALF_WIDTH_M = 1.8
# The signs: (name, chainage m or None for the gateway, side (+1 = right of travel toward town), facing).
# "toTown" signs face walkers coming from the manor; "toManor" faces those leaving town.
GATEWAY = (-55.0, 90.0)


def bilinear(z, x, y):
    fx, fy = x + H, y + H
    i, j = int(math.floor(fx)), int(math.floor(fy))
    tx, ty = fx - i, fy - j
    return float(z[j, i] * (1 - tx) * (1 - ty) + z[j, i + 1] * tx * (1 - ty) + z[j + 1, i] * (1 - tx) * ty
                 + z[j + 1, i + 1] * tx * ty)


def bridge_deck(layout, road, chain, z, frame):
    """The road bridge: where the road crosses the river, at road_grade.py's deck level, long enough that
    each end rests BRIDGE_BEARING_M on the graded bank. None before the road has been graded."""
    grade = layout.get("roadGrade")
    if not grade:
        return None
    level = float(grade["bridgeDeck"])
    end = layout.get("riverEnd", len(layout["river"]))
    river = np.asarray(layout["river"][:end], np.float64)
    # The crossing, to 0.1 m: the chainage nearest the river's course.
    near = float(grade["crossing"])
    best = min(np.arange(near - 6.0, near + 6.0, 0.1),
               key=lambda ch: np.min(np.hypot(*(river - frame(ch)[0]).T)))
    centre, d = frame(best)
    right = np.array([-d[1], d[0]])
    j = int(np.argmin(np.hypot(*(river - centre).T)))
    reach = 0.0
    for across in (-BRIDGE_CLEAR_HALF_WIDTH_M, 0.0, BRIDGE_CLEAR_HALF_WIDTH_M):
        for sign in (1.0, -1.0):
            s = 0.0
            while s < BRIDGE_HALF_ALONG_M:
                q = centre + right * across + d * sign * s
                if s > 1.0 and bilinear(z, *q) >= level - BRIDGE_SEAT_M:
                    break
                s += 0.1
            reach = max(reach, s)
    half = min(round((reach + BRIDGE_BEARING_M) * 2.0) / 2.0, BRIDGE_HALF_ALONG_M - 2.0)
    yaw = (math.degrees(math.atan2(d[1], d[0])) + 360.0) % 360.0
    water = float(layout["riverSurface"][j])
    bed = float(layout["riverChannel"]["bed"][j])
    return (centre[0] * 100.0, centre[1] * 100.0, yaw, level * 100.0, half * 100.0, BRIDGE_CLEAR_HALF_WIDTH_M * 100.0,
            water * 100.0, bed * 100.0)


def clear_scenery(x, y, yaw, half_along, half_across):
    """Drop the baked scatter records inside the bridge's footprint (x, y in metres)."""
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"),
                                                       ("yaw", "<f4"), ("s", "<f4")]))
    d = np.array([math.cos(math.radians(yaw)), math.sin(math.radians(yaw))])
    rel = np.c_[rec["x"] / 100.0 - x, rec["y"] / 100.0 - y]
    inside = (np.abs(rel @ d) < half_along) & (np.abs(rel @ np.array([-d[1], d[0]])) < half_across)
    if inside.any():
        keep = rec[~inside]
        with open(SCENERY, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(keep)).tobytes() + keep.tobytes() + raw[8 + count * 20:])
    return int(inside.sum())


def main():
    layout = json.load(open(LAYOUT))
    road = np.asarray(layout["road"], np.float64)
    prof = np.asarray(layout["roadProfile"], np.float64)
    seg = np.hypot(*np.diff(road, axis=0).T)
    chain = np.r_[0.0, np.cumsum(seg)]
    z = (np.fromfile(R16, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0

    def ground(x, y):
        return float(z[int(round(y)) + H, int(round(x)) + H])

    def frame(ch):
        k = int(np.clip(np.searchsorted(chain, ch), 1, len(road) - 2))
        d = road[k + 1] - road[k - 1]
        d /= np.linalg.norm(d)
        t = (ch - chain[k - 1]) / max(chain[k] - chain[k - 1], 1e-9)
        p = road[k - 1] + (road[k] - road[k - 1]) * min(max(t, 0.0), 1.0)
        return p, d

    # The river crossing: the road point nearest the river's graded course.
    river = np.asarray(layout["river"][:layout.get("riverEnd", len(layout["river"]))], np.float64)
    d2 = ((road[:, None, :] - river[None, :, :]) ** 2).sum(-1)
    k, j = np.unravel_index(np.argmin(d2), d2.shape)
    bridge_ch = float(chain[k])
    deck = bridge_deck(layout, road, chain, z, frame)
    gate_ch = float(chain[int(np.argmin(np.hypot(*(road - np.array(GATEWAY)).T)))])

    signs = []
    for name, ch, side, toward in (("ManorRoadSign", 12.0, -1, "town"),
                                   ("GatewayRoadSign", gate_ch + 14.0, -1, "town"),
                                   ("TownRoadSign", float(chain[-1]) - 14.0, 1, "manor")):
        p, d = frame(ch)
        right = np.array([-d[1], d[0]])       # x north, y east: right of the heading (Unreal is left-handed)
        q = p + right * side * SIGN_OFFSET_M
        heading = math.degrees(math.atan2(d[1], d[0]))
        yaw = heading if toward == "town" else heading + 180.0
        signs.append((name, ch, q, ground(*q), (yaw + 360.0) % 360.0))

    lines = [
        "// Generated by Scripts/Terrain/public_road.py from estate_layout.json; do not edit by hand.",
        f"// The public road from the manor forecourt to town: {len(road)} centreline points (cm), {chain[-1]:.1f} m.",
        "// road(x, y, groundZ, chainage m)",
    ]
    for (x, y), gz, ch in zip(road, prof, chain):
        lines.append(f"road({x * 100.0:.1f}, {y * 100.0:.1f}, {gz * 100.0:.1f}, {ch:.2f});")
    lines.append("// bridge(chainage m, half length m along the road, half width m across it): no placements here")
    lines.append(f"bridge({bridge_ch:.2f}, {BRIDGE_HALF_ALONG_M:.1f}, {BRIDGE_HALF_ACROSS_M:.1f});")
    if deck:
        lines.append("// deck(x cm, y cm, yaw deg, deck z cm, half length cm, half clear width cm, water z cm, bed z cm):")
        lines.append("// the road bridge over the river, centred where the road crosses it")
        lines.append("deck({:.1f}, {:.1f}, {:.2f}, {:.1f}, {:.1f}, {:.1f}, {:.1f}, {:.1f});".format(*deck))
    lines.append("// stop(name, chainage m): travel endpoints on the walkable road bed")
    lines.append(f"stop(\"Manor\", {12.0:.2f});")
    lines.append(f"stop(\"Gateway\", {gate_ch:.2f});")
    lines.append(f"stop(\"Town\", {float(chain[-1]) - 6.0:.2f});")
    lines.append("// sign(name, chainage m, x cm, y cm, ground z cm, yaw deg): the face points along yaw")
    for name, ch, q, gz, yaw in signs:
        lines.append(f"sign(\"{name}\", {ch:.2f}, {q[0] * 100.0:.1f}, {q[1] * 100.0:.1f}, {gz * 100.0:.1f}, {yaw:.1f});")
    if deck:
        x, y, yaw, level, half, width = deck[0] / 100.0, deck[1] / 100.0, deck[2], deck[3] / 100.0, deck[4] / 100.0, deck[5] / 100.0
        layout["roadBridge"] = {"centre": [round(x, 3), round(y, 3)], "yaw": round(yaw, 2), "deck": round(level, 3),
                                "halfLength": half, "halfWidth": width}
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)
        cleared = clear_scenery(x, y, yaw, half + BRIDGE_CLEAR_MARGIN_M, width + 0.6 + BRIDGE_CLEAR_MARGIN_M)
        print(f"public road: bridge {2 * half:.1f} m x {2 * width:.1f} m at ({x:.1f}, {y:.1f}); {cleared} scenery records cleared off it")
    with open(OUT, "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print(f"public road: {len(road)} points, {chain[-1]:.1f} m; bridge at {bridge_ch:.1f} m; gateway {gate_ch:.1f} m -> {OUT}")


if __name__ == "__main__":
    main()
