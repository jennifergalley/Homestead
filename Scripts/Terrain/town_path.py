"""Smooth the village street - the path down to the village from the main road (village-path-and-dressing).

The street (estate_layout.json -> town.street) was laid on the raw hillside, so its long profile followed every
bump (rms about 9 cm, up to 40 cm, against a 9 m moving average) and its cross-section wobbled by half a metre.
This gives it a corridor re-grade like the main road's: an even long profile between the two ends that other
scripts already fixed (the road's height at the junction, the pad's height at the square), a gentle crown across the
bed, and soft verges that blend back into the hillside.

    python Scripts\\Terrain\\town_path.py          # grade once; a second run only reports
    python Scripts\\Terrain\\town_path.py --dry    # report without writing

Run it after town_pad.py and town_layout.py. Everything is relative to the street polyline in the layout, so the
stage 2 rebake just re-runs it: delete "townPath" from estate_layout.json first. It writes the heightfield, PNG and
work npy (HOMESTEAD_TERRAIN_WORK) and records "townPath" in estate_layout.json.
Then: town_layout.py (street z), public_road.py, weightmaps.py, bake_ground.py, bake_estate_map.py; in the editor
ApplyEstateHeightfield over the rows and cols it prints, ApplyEstateWeightmaps, build_ground.py and ImportEstateMap.
"""
import json
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016

FLAT_M = 3.0                      # the bed and its shoulders are graded flat across out to this far from the centre line
FALLOFF_M = 9.0                   # then blended back into the hillside over this width
CROWN_FALL = 1.0 / 30.0           # the bed sheds water toward both edges at 1:30 (Balance: 1:30 cross-fall)
KEEP_RESIDUAL = 0.35              # share of the hillside's own long (>= RESIDUAL_WINDOW_M) swell kept, so it is not a ramp
RESIDUAL_WINDOW_M = 40.0
END_EASE_M = 8.0                  # the hill's swell fades out toward the junction and the square over this length
END_BLEND_M = 4.0                 # the profile starts from the existing ground at each end and eases to the even fall over this
ROAD_CLEAR_M = 1.5                # the road bed beside the junction keeps its height out to this far,
ROAD_RAMP_M = 3.0                 # and the corridor eases back in over this much more
MAX_GRADE_REPORT = 0.12           # Balance: 10% typical, 12% on a short pitch; reported, not enforced (see layout)


def smoothstep(t):
    t = np.clip(t, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def bilinear(z, x, y):
    """Height at metre coordinates (x north, y east) from z[row = y + H, col = x + H]."""
    fx, fy = np.asarray(x) + H, np.asarray(y) + H
    x0, y0 = np.floor(fx).astype(int), np.floor(fy).astype(int)
    tx, ty = fx - x0, fy - y0
    return ((z[y0, x0] * (1 - tx) + z[y0, x0 + 1] * tx) * (1 - ty)
            + (z[y0 + 1, x0] * (1 - tx) + z[y0 + 1, x0 + 1] * tx) * ty)


def moving_average(v, window):
    k = int(window) | 1
    pad = np.pad(v, k // 2, mode="edge")
    return np.convolve(pad, np.ones(k) / k, mode="valid")


def target_profile(raw_line):
    """An even fall between the two fixed ends, easing in and out, with a little of the hill's own swell kept."""
    n = len(raw_line)
    s = np.arange(n, dtype=np.float64)
    line = raw_line[0] + (raw_line[-1] - raw_line[0]) * s / (n - 1)
    swell = moving_average(raw_line - line, RESIDUAL_WINDOW_M)
    swell -= swell[0] + (swell[-1] - swell[0]) * s / (n - 1)     # keep both ends exactly where they were
    ease_in = smoothstep(s / END_EASE_M)
    ease_out = smoothstep((n - 1 - s) / END_EASE_M)
    prof = line + KEEP_RESIDUAL * swell * ease_in * ease_out
    # At each end the profile starts from the existing ground (the road bed beside the junction, the pad at the
    # square) and eases to the even fall over a few metres, so the grade does not jump where the corridor meets them.
    own = moving_average(raw_line, 9)
    own[0], own[-1] = raw_line[0], raw_line[-1]
    blend_in = 1.0 - smoothstep(s / END_BLEND_M)
    blend_out = 1.0 - smoothstep((n - 1 - s) / END_BLEND_M)
    return prof + (own - prof) * np.maximum(blend_in, blend_out)


def main():
    layout = json.load(open(LAYOUT))
    if layout.get("townPath", {}).get("graded"):
        print("town path: already graded")
        return
    street = np.asarray(layout["town"]["street"], np.float64)       # x north, y east, 1 m steps
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0

    line_before = bilinear(z, street[:, 0], street[:, 1])
    prof = target_profile(line_before)
    seg = np.hypot(*np.diff(street, axis=0).T)
    chain = np.r_[0.0, np.cumsum(seg)]

    reach = FLAT_M + FALLOFF_M
    x0, x1 = int(street[:, 0].min() - reach) - 1, int(street[:, 0].max() + reach) + 2
    y0, y1 = int(street[:, 1].min() - reach) - 1, int(street[:, 1].max() + reach) + 2
    yy, xx = np.mgrid[y0:y1 + 1, x0:x1 + 1].astype(np.float64)
    win = z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1]

    # Nearest point on the street polyline: its distance, and its position along it for the profile.
    best_d = np.full(xx.shape, 1e9)
    best_s = np.zeros(xx.shape)
    for i in range(len(street) - 1):
        a, b = street[i], street[i + 1]
        d = b - a
        t = np.clip(((xx - a[0]) * d[0] + (yy - a[1]) * d[1]) / np.dot(d, d), 0.0, 1.0)
        dist = np.hypot(xx - (a[0] + t * d[0]), yy - (a[1] + t * d[1]))
        better = dist < best_d
        best_d = np.where(better, dist, best_d)
        best_s = np.where(better, chain[i] + t * seg[i], best_s)
    bed_z = np.interp(best_s, chain, prof)
    crowned = bed_z - CROWN_FALL * np.minimum(best_d, FLAT_M)
    weight = 1.0 - smoothstep((best_d - FLAT_M) / FALLOFF_M)

    road = np.asarray(layout["road"], np.float64)
    near = np.hypot(road[:, 0, None, None] - xx[None], road[:, 1, None, None] - yy[None]).min(0)
    weight = weight * smoothstep((near - ROAD_CLEAR_M) / ROAD_RAMP_M)    # the road bed keeps its graded height

    after = win + (crowned - win) * weight

    def along(h):
        return bilinear(h_full(h), street[:, 0], street[:, 1])

    def h_full(h):
        out = z.copy()
        out[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] = h
        return out

    line_after = along(after)

    def roughness(line):
        return float(np.sqrt(np.mean((line - moving_average(line, 9)) ** 2)))

    grade = np.abs(np.gradient(line_after))
    g_avg = (line_after[0] - line_after[-1]) / chain[-1]
    across = lambda h: float(np.ptp(h))
    sl = lambda h: np.degrees(np.arctan(np.hypot(*np.gradient(h))))
    print(f"town path: {chain[-1]:.0f} m street, falls {line_before[0] - line_before[-1]:.1f} m ({g_avg * 100:.1f}% average); "
          f"roughness {roughness(line_before) * 100:.1f} cm -> {roughness(line_after) * 100:.1f} cm; "
          f"grade now {np.percentile(grade, 95) * 100:.1f}% p95, {grade.max() * 100:.1f}% max at the junction kink (Balance asks {MAX_GRADE_REPORT * 100:.0f}%); "
          f"cut/fill {(after - win).min():+.2f} .. {(after - win).max():+.2f} m; "
          f"steepest ground in the window {sl(after).max():.1f} deg (before {sl(win).max():.1f})")
    if g_avg > MAX_GRADE_REPORT:
        print(f"  note: the ends are fixed (junction on the road, mouth on the pad), so the street cannot fall less than "
              f"{g_avg * 100:.1f}% on this length; a longer street (junction further back up the road) is the way to flatten it")
    if "--dry" in sys.argv:
        print(f"town path: dry run, nothing written; ApplyEstateHeightfield rows {y0 + H}-{y1 + H}, cols {x0 + H}-{x1 + H}")
        return

    z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] = after
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    changed = np.argwhere(u16 != raw)
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy):
        zn = np.load(npy)                                  # [row = H - x, col = H + y]
        zn[(H - xx).astype(int), (H + yy).astype(int)] = after.astype(zn.dtype)
        np.save(npy, zn)
    layout["townPath"] = {"graded": True, "flat": FLAT_M, "falloff": FALLOFF_M, "crownFall": round(CROWN_FALL, 4),
                          "averageGrade": round(float(g_avg), 4), "maxGrade": round(float(grade.max()), 4)}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    (a0, b0), (a1, b1) = changed.min(0), changed.max(0)
    print(f"town path: {len(changed)} vertices changed; ApplyEstateHeightfield rows {a0}-{a1}, cols {b0}-{b1}")


if __name__ == "__main__":
    main()
