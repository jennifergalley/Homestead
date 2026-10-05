"""Level a building pad for the village in the woods north-east of the manor (shrink-estate-map, stage 1).

The village moves from the far end of the road to a hidden patch of hillside about 300 m from the manor forecourt.
The ground there falls roughly 8% to the north-west, so a plain plane would cut and fill metres at the corners.
This lays a gentler plane (PAD_GRADIENT_SHARE of the hill's own tilt) under the square and its buildings, with
the lumps smoothed out, and blends it into the hillside over FALLOFF_M. The road is kept untouched.

    python Scripts\\Terrain\\town_pad.py          # grade once; a second run only reports
    python Scripts\\Terrain\\town_pad.py --dry    # report without writing

Writes the heightfield, PNG and work npy (HOMESTEAD_TERRAIN_WORK), and records "townPad" in estate_layout.json.
Then: town_layout.py, public_road.py, weightmaps.py, bake_ground.py, bake_estate_map.py; in the editor
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

CENTRE_M = (-100.0, -330.0)       # the square's centre (x north, y east); town_layout.py reads it from the layout
CORE_HALF_M = (34.0, 42.0)         # the pad's level core: x and y half extents, the square and its buildings
FALLOFF_M = 20.0                   # blended into the hillside over this width
PAD_GRADIENT_SHARE = 0.75           # of the hill's own tilt: a gentle fall, not a terrace
FIT_HALF_M = (50.0, 56.0)          # the hillside's plane is fitted over this window
ROAD_CLEAR_M = 3.0                 # the pad leaves the road bed alone out to this far,
ROAD_RAMP_M = 12.0                 # and eases back in over this much more


def smoothstep(t):
    t = np.clip(t, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def main():
    layout = json.load(open(LAYOUT))
    if layout.get("townPad", {}).get("graded"):
        print("town pad: already graded")
        return
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]
    cx, cy = CENTRE_M
    reach = np.array(CORE_HALF_M) + FALLOFF_M
    x0, x1 = int(cx - reach[0]), int(cx + reach[0]) + 1
    y0, y1 = int(cy - reach[1]), int(cy + reach[1]) + 1
    yy, xx = np.mgrid[y0:y1 + 1, x0:x1 + 1].astype(np.float64)
    win = z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1]

    fit = (np.abs(xx - cx) <= FIT_HALF_M[0]) & (np.abs(yy - cy) <= FIT_HALF_M[1])
    a = np.c_[xx[fit], yy[fit], np.ones(fit.sum())]
    gx, gy, c0 = np.linalg.lstsq(a, win[fit], rcond=None)[0]
    centre_z = gx * cx + gy * cy + c0
    pad = centre_z + PAD_GRADIENT_SHARE * (gx * (xx - cx) + gy * (yy - cy))

    # Distance outside the core rectangle, rounded at the corners.
    ox = np.clip(np.abs(xx - cx) - CORE_HALF_M[0], 0.0, None)
    oy = np.clip(np.abs(yy - cy) - CORE_HALF_M[1], 0.0, None)
    blend = 1.0 - smoothstep(np.hypot(ox, oy) / FALLOFF_M)

    road = np.asarray(layout["road"], np.float64)
    near = np.hypot(road[:, 0, None, None] - xx[None], road[:, 1, None, None] - yy[None]).min(0)
    keep = smoothstep((near - ROAD_CLEAR_M) / ROAD_RAMP_M)  # the road's bed and its verge keep their graded height
    blend = blend * keep

    after = win + (pad - win) * blend
    delta = after - win
    print(f"town pad: hillside tilt {gx * 100:+.1f}% north, {gy * 100:+.1f}% east; level at {centre_z:.2f} m; "
          f"cut/fill {delta.min():+.2f} .. {delta.max():+.2f} m; "
          f"core residual before {np.ptp(win[blend > 0.999] - pad[blend > 0.999]):.2f} m, "
          f"slope after {np.degrees(np.arctan(np.hypot(*np.gradient(after)))).max():.1f} deg max")
    sl = lambda h: np.degrees(np.arctan(np.hypot(*np.gradient(h))))
    i = np.unravel_index(sl(after).argmax(), after.shape)
    print(f"  steepest after at x {xx[i]:.0f} y {yy[i]:.0f}: {sl(after)[i]:.1f} deg (before {sl(win)[i]:.1f}), road {near[i]:.1f} m; "
          f"max before {sl(win).max():.1f}, p99 after {np.percentile(sl(after), 99):.1f}")
    if "--dry" in sys.argv:
        print(f"town pad: dry run, nothing written; ApplyEstateHeightfield rows {y0 + H}-{y1 + H}, cols {x0 + H}-{x1 + H}")
        return

    z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1] = after
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    changed = np.argwhere(u16 != raw)
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy):
        zn = np.load(npy)                                  # [row = H - x, col = H + y]
        rows = (H - xx).astype(int)
        cols = (H + yy).astype(int)
        zn[rows, cols] = after.astype(zn.dtype)
        np.save(npy, zn)
    layout["townPad"] = {"graded": True, "centre": list(CENTRE_M), "coreHalf": list(CORE_HALF_M),
                         "falloff": FALLOFF_M, "gradientShare": PAD_GRADIENT_SHARE, "level": round(float(centre_z), 2)}
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    (a0, b0), (a1, b1) = changed.min(0), changed.max(0)
    print(f"town pad: {len(changed)} vertices changed; ApplyEstateHeightfield rows {a0}-{a1}, cols {b0}-{b1}")


if __name__ == "__main__":
    main()
