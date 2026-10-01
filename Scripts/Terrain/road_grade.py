"""Re-grade the public road to follow the ground (Jenny, 2026-09-29: "artificial raised and lowered road
segments").

reshape.py graded the road to a heavily smoothed profile clamped at 11% (1 in 9) by forward and backward
passes. Where the river valley's sides are steeper than that, the clamp drew straight lines: a causeway up
to 9.9 m high down into the valley (chainage 411-679 m) and a cutting up to 12.8 m deep out of it
(726-1264 m). This replaces the profile with one that follows the ground:
- the target is the ground the road was graded over, lightly smoothed (SMOOTH_TARGET_M);
- it is limited to MAX_GRADE (1 in 5, as steep Cornish lanes are) by the midpoint of the largest
  MAX_GRADE-Lipschitz minorant and the smallest majorant, which keeps the worst cut or fill smallest
  (the old one-sided clamp is what made the long earthworks), then eased (SMOOTH_PROFILE_M);
- across the river it holds BRIDGE_DECK_CLEARANCE_M over the water for the road bridge, climbing to it on
  1 in 12 approach ramps inside the bridge keep-out;
- the manor forecourt and the town end keep their old levels (ENDS_BLEND_M).

reshape.py's grading is linear in the profile (z = ground + (profile - ground) * w), so the change is
exact: z += (new profile - old profile) * w over the same 2.8 m bed and 12 m verges. Chainage, the
centreline, the anchors and everything off the corridor are untouched. river_channel.py then re-seats the
river where it passes under the road.

    python Scripts\\Terrain\\road_grade.py         # heightfield + PNG + npy + estate_layout.json

Writes (in step): Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16, Scripts/Terrain/
Estate_Heightmap_4033.png, <work>/game_reshaped_4033.npy, estate_layout.json ("roadProfile" and
"roadGrade" with the previous profile). Running it again changes nothing in the heightfield, but brings a
work npy that missed the regrade into step (for another checkout's work folder). Then run
river_channel.py (this runs it), public_road.py, bake_ground.py and bake_estate_map.py, and in the editor
ApplyEstateHeightfield over the rectangle it prints, build_ground.py and ImportEstateMap.
"""
import json
import os
import subprocess
import sys

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter1d

import reshape

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = reshape.SIZE, reshape.H

MAX_GRADE = 0.20                 # 1 in 5
SMOOTH_TARGET_M = 5.0            # the ground's bumps the road bed evens out
SMOOTH_PROFILE_M = 4.0           # vertical curves
ENDS_BLEND_M = 25.0              # the manor forecourt and the town end keep their graded levels
PAD_OFFSET_SIDES_M = (20.0, 24.0, 28.0)   # where the pads' change to the raw ground is read, off the corridor
BRIDGE_DECK_CLEARANCE_M = 0.9    # deck level above the river's surface at the crossing
BRIDGE_DECK_HALF_M = 6.0         # the road holds deck level this far either side of the crossing
BRIDGE_KEEP_OUT_M = 20.0         # public_road.py's BRIDGE_HALF_ALONG_M
BRIDGE_RAMP_GRADE = 0.08         # the approach ramps (1 in 12), inside the 20 m bridge keep-out
FLAT, FALLOFF = 2.8, 12.0        # reshape.py's road bed and verges


def crossing(road, layout):
    riv = np.asarray(layout["river"][:layout.get("riverEnd", len(layout["river"]))], np.float64)
    d2 = ((road[:, None, :] - riv[None, :, :]) ** 2).sum(-1)
    k, j = np.unravel_index(np.argmin(d2), d2.shape)
    return int(k), float(layout["riverSurface"][j])


def lipschitz_midpoint(target, grade):
    """The profile within `grade` per metre whose largest deviation from target is smallest."""
    lo, hi = target.copy(), target.copy()
    for passes in (range(1, len(target)), range(len(target) - 2, -1, -1)):
        step = 1 if passes.start < passes.stop else -1
        for k in passes:
            lo[k] = min(lo[k], lo[k - step] + grade)
            hi[k] = max(hi[k], hi[k - step] - grade)
    return (lo + hi) / 2.0


def design(road, old, raw, cur, layout):
    n = len(road)
    ch = np.arange(n, dtype=np.float64)
    t = np.gradient(road, axis=0)
    t /= np.linalg.norm(t, axis=1)[:, None]
    normal = np.c_[-t[:, 1], t[:, 0]]
    target = reshape.sample(raw, road)
    # The pads (manor forecourt, town) changed the ground before the road was graded: carry their offset.
    offset = np.zeros(n)
    for side in PAD_OFFSET_SIDES_M:
        for sign in (1.0, -1.0):
            p = road + normal * side * sign
            offset += (reshape.sample(cur, p) - reshape.sample(raw, p)) / (2 * len(PAD_OFFSET_SIDES_M))
    target += offset
    k, surface = crossing(road, layout)
    deck = surface + BRIDGE_DECK_CLEARANCE_M
    target = gaussian_filter1d(target, SMOOTH_TARGET_M, mode="nearest")
    prof = gaussian_filter1d(lipschitz_midpoint(target, MAX_GRADE), SMOOTH_PROFILE_M, mode="nearest")
    # Hold the deck level and climb to it on gentle ramps (a cone from the deck's ends: the maximum of two
    # profiles within the grade stays within it).
    ramp = deck - BRIDGE_RAMP_GRADE * np.maximum(np.abs(ch - k) - BRIDGE_DECK_HALF_M, 0.0)
    prof = np.maximum(prof, ramp)
    # No pit between the deck and a bank that rises past it: fill up to the lower of the deck and the
    # highest ground further out, within the keep-out on each side.
    east = np.arange(k + int(BRIDGE_DECK_HALF_M), min(k + int(BRIDGE_KEEP_OUT_M), n - 1) + 1)
    west = np.arange(k - int(BRIDGE_DECK_HALF_M), max(k - int(BRIDGE_KEEP_OUT_M), 0) - 1, -1)
    for side in (east, west):
        outward_max = np.maximum.accumulate(prof[side][::-1])[::-1]
        prof[side] = np.maximum(prof[side], np.minimum(deck, outward_max))
    ends = reshape.smoothstep(np.minimum(ch, ch[-1] - ch) / ENDS_BLEND_M)
    new = old + (prof - old) * ends
    return new, k, deck


def corridor_delta(road, delta_prof, z_is_r16):
    """Per-cell change for one heightfield layout: (rows, cols, change m)."""
    rr, cc, d, i = reshape.corridor(road, FLAT + FALLOFF)       # npy layout [row = H - x, col = H + y]
    w = 1.0 - reshape.smoothstep((d - FLAT) / FALLOFF)
    change = delta_prof[i] * w
    if z_is_r16:
        x, y = H - rr, cc - H
        return y + H, x + H, change
    return rr, cc, change


def main():
    layout = json.load(open(LAYOUT))
    road = reshape.resample_polyline(reshape.densify(reshape.ROAD, 1.0), 1.0)[0]
    if np.abs(road[::4] - np.asarray(layout["road"])).max() > 0.01:
        raise SystemExit("estate_layout.json road is not reshape.ROAD at 4 m: refusing to grade a different route")
    grid = np.arange(0, len(road), 4)[:len(layout["roadProfile"])]
    ch = np.arange(len(road), dtype=np.float64)
    npy_path = os.path.join(WORK, "game_reshaped_4033.npy")
    raw_r16 = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw_r16.astype(np.float64) - 32768.0) / 128.0          # [row = y + H, col = x + H]

    graded = layout.get("roadGrade", {}).get("graded", False)
    if graded:
        old = np.interp(ch, grid, layout["roadGrade"]["previousProfile"])
        new = np.interp(ch, grid, layout["roadProfile"])
        k = int(layout["roadGrade"]["crossing"])
    else:
        old = np.interp(ch, grid, layout["roadProfile"])
        raw = np.load(os.path.join(WORK, "game_raw_4033.npy")).astype(np.float64)
        cur = np.load(npy_path).astype(np.float64)
        new, k, deck = design(road, old, raw, cur, layout)
        del raw, cur
    delta = new - old

    changed = 0
    if not graded:
        rows, cols, change = corridor_delta(road, delta, True)
        z[rows, cols] += change
        u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
        moved = np.argwhere(u16 != raw_r16)
        changed = len(moved)
        u16.astype("<u2").tofile(R16)
        Image.fromarray(u16).save(PNG)
    if os.path.exists(npy_path):
        zn = np.load(npy_path).astype(np.float64)
        centre = reshape.sample(zn, road[::8])
        # A work npy that missed the regrade still has the old road down its middle.
        if not graded or np.median(np.abs(centre - old[::8])) < np.median(np.abs(centre - new[::8])):
            rows, cols, change = corridor_delta(road, delta, False)
            zn[rows, cols] += change
            np.save(npy_path, zn.astype(np.float32))
            print(f"road grade: work npy brought into step ({npy_path})")

    if not graded:
        layout["roadGrade"] = {"graded": True, "maxGrade": MAX_GRADE, "crossing": k,
                               "bridgeDeck": round(float(deck), 3),
                               "previousProfile": layout["roadProfile"]}
        layout["roadProfile"] = np.round(new[grid], 2).tolist()
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)
        (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
        print(f"road grade: {changed} vertices changed; max raise {delta.max():+.2f} m, max lower {delta.min():+.2f} m; "
              f"bridge deck {deck:.2f} m at chainage {k} m; ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")
        # Re-seat the river where the road crosses it (the channel cuts, never fills, and is idempotent).
        subprocess.run([sys.executable, os.path.join(HERE, "river_channel.py")], check=True)
    else:
        print("road grade: heightfield already graded")


if __name__ == "__main__":
    main()
