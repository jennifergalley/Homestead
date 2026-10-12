"""Regrade the mine site so the mine house stands on open, level ground with its surroundings, not down
in a hollow (Jenny, 2026-10-11: "Put the house on ground level with its surroundings, not set down in a
hollow. Level/regrade the yard so it sits flush").

    python Scripts\\Mine\\regrade_mine_site.py [--dry] [--preview <png>]          # once: raise and level the site
    python Scripts\\Mine\\regrade_mine_site.py --smooth [--dry]                     # then: smooth the surround (no trough)
    python Scripts\\Mine\\regrade_mine_site.py --plain [--dry]                      # then: a wider level plain

estate_footpaths.py cut the mine yard as an 18 x 14 m pad into the hillside at 52.2 m with an 8 m blend,
leaving banks 5-15 m high on three sides. This raises the yard to ``YARD_LEVEL`` and eases the hillside
round it into a broad shelf: inside the pad the ground is level; out to ``SHELF_M`` it is the hillside
flattened toward the yard level (``SHELF_FLATTEN``); from there to ``OUTER_M`` it blends back into the
untouched ground. Fill toward the sea is capped so the cliff edge doesn't move. Run it once on the
pre-regrade heightfield (it refuses to run twice: estate_layout.json's ``mineSite`` records it).

Writes EstateHeightfield.r16, Estate_Heightmap_4033.png and the work npy where they changed, the mine's
height in estate_layout.json and Scripts/Blender/Recipes/mine/layout.py's HOUSE_WORLD, and prints the
ApplyEstateHeightfield rectangle. Then: mirror MineEntrance's z in HomesteadEstate.cpp, apply the
heightfield in the editor (apply_estate_heightfield over the rectangle, save only the touched proxies),
re-run Scripts/Mine/mine_layout_inc.py, place_mine.py and paint_mine_holes.py, re-bake the ground
(bake_ground.py + build_ground.py, or Scripts/Mine/patch_ground_texture.py) and the map (bake_estate_map.py).
"""
import json
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Terrain"))
import cove_route as route_kit  # noqa: E402  (heightfield paths and helpers)

LAYOUT = ROOT / "Scripts" / "Terrain" / "estate_layout.json"
MINE_LAYOUT = ROOT / "Scripts" / "Blender" / "Recipes" / "mine" / "layout.py"
H = route_kit.H
CENTRE = (-410.0, -722.0)
PAD_HALF = (9.0, 7.0)        # the yard (estate_footpaths.YARD)
YARD_LEVEL = 56.0            # m: about the mean of the ground 10-25 m round the old pad
SHELF_M = 26.0               # the flattened shelf reaches this far past the pad
SHELF_FLATTEN = 0.30         # on the shelf the hillside keeps this fraction of its rise or fall
OUTER_M = 72.0               # blended back into the untouched ground by here (past the pad)
SMOOTH_SIGMA_M = 10.0        # the hillside is read smoothed, so the shelf has no lumps
FILL_CAP = ((14.0, 6.0), (40.0, 0.0))  # (distance past the pad m, max fill m): no new ground over the cliff


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def regrade(z):
    r = int(OUTER_M + max(PAD_HALF) + 20)
    cx, cy = int(CENTRE[0]), int(CENTRE[1])
    rows, cols = slice(cy - r + H, cy + r + H + 1), slice(cx - r + H, cx + r + H + 1)
    old = z[rows, cols].copy()
    X, Y = np.meshgrid(np.arange(cx - r, cx + r + 1), np.arange(cy - r, cy + r + 1))
    past = np.hypot(np.maximum(np.abs(X - CENTRE[0]) - PAD_HALF[0], 0), np.maximum(np.abs(Y - CENTRE[1]) - PAD_HALF[1], 0))
    smooth = gaussian_filter(old, SMOOTH_SIGMA_M)
    shelf = YARD_LEVEL + SHELF_FLATTEN * (smooth - YARD_LEVEL)
    near = 1.0 - smoothstep(0.0, SHELF_M, past)              # 1 on the pad -> 0 at the shelf's edge
    target = shelf + (YARD_LEVEL - shelf) * near
    outer = 1.0 - smoothstep(SHELF_M * 0.6, OUTER_M, past)   # 1 near -> 0 at OUTER_M
    new = old + (target - old) * outer
    (d0, c0), (d1, c1) = FILL_CAP
    cap = np.interp(past, [d0, d1], [c0, c1])
    new = np.minimum(new, old + cap)
    # Ease the capped edge: smooth the change itself, then keep the pad exactly level.
    new = old + gaussian_filter(new - old, 3.0) * (1.0 - smoothstep(OUTER_M * 0.85, OUTER_M, past))
    pad = past <= 1.0
    new[pad] = YARD_LEVEL
    z[rows, cols] = new
    return old, new, past


SMOOTH_FLAT_M, SMOOTH_REACH_M = 16.0, 34.0   # --smooth: replaced by the smooth surface out to the first, blended by the second
# --plain (Jenny: "make it green, level, and covered with grass like the plains"): level about 20 m round the pad,
# rising or falling gently into the hillside by 46 m, blended back by 48 m.
PLAIN = {"sigma": 14.0, "ramp": (12.0, 46.0), "blend": (30.0, 48.0)}


def smooth_surround(z, sigma=SMOOTH_SIGMA_M, ramp=(2.0, SMOOTH_REACH_M), blend=(SMOOTH_FLAT_M, SMOOTH_REACH_M)):
    """Jenny (2026-10-11): "It's still a little sunken ... now it's just got a moat around it. Please smooth that
    out." The first regrade's capped, smoothed change left a 0.5-0.9 m trough a few metres past the pad. This
    replaces the ground round the pad with one smooth surface (the yard level rising or falling into the
    heavily smoothed hillside, monotone outward, no trough) and blends it back into the ground by
    SMOOTH_REACH_M. Works on the current heightfield."""
    r = int(max(ramp[1], blend[1]) + max(PAD_HALF) + 30)
    cx, cy = int(CENTRE[0]), int(CENTRE[1])
    rows, cols = slice(cy - r + H, cy + r + H + 1), slice(cx - r + H, cx + r + H + 1)
    old = z[rows, cols].copy()
    X, Y = np.meshgrid(np.arange(cx - r, cx + r + 1), np.arange(cy - r, cy + r + 1))
    past = np.hypot(np.maximum(np.abs(X - CENTRE[0]) - PAD_HALF[0], 0), np.maximum(np.abs(Y - CENTRE[1]) - PAD_HALF[1], 0))
    target = YARD_LEVEL + (gaussian_filter(old, sigma) - YARD_LEVEL) * smoothstep(*ramp, past)
    new = old + (target - old) * (1.0 - smoothstep(*blend, past))
    new[past <= 1.0] = YARD_LEVEL
    z[rows, cols] = new
    return old, new, past


def slope_deg(a):
    gy, gx = np.gradient(a)
    return np.degrees(np.arctan(np.hypot(gx, gy)))


def hillshade(a):
    gy, gx = np.gradient(a)
    n = np.dstack([-gx, -gy, np.ones_like(a)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    light = np.array([-0.5, -0.5, 0.7])
    return np.clip(n @ (light / np.linalg.norm(light)), 0, 1)


def main():
    dry = "--dry" in sys.argv
    layout = json.loads(LAYOUT.read_text())
    plain = "--plain" in sys.argv
    smooth = "--smooth" in sys.argv or plain
    if layout.get("mineSite", {}).get("regraded") and not dry and not smooth:
        print("mine site: already regraded (estate_layout.json mineSite); restore the heightfield to redo it")
        return
    raw, z = route_kit.load_r16()
    old, new, past = (smooth_surround(z, PLAIN["sigma"], PLAIN["ramp"], PLAIN["blend"]) if plain
                      else smooth_surround(z) if smooth else regrade(z))
    s_old, s_new = slope_deg(old), slope_deg(new)
    ring = (past > 0) & (past < 30)
    print(f"mine site: yard {YARD_LEVEL:.2f} m (was 52.23); within 30 m of the pad: mean slope "
          f"{s_old[ring].mean():.1f} -> {s_new[ring].mean():.1f} deg, p95 {np.percentile(s_old[ring], 95):.1f} -> "
          f"{np.percentile(s_new[ring], 95):.1f}; cut {np.clip(old - new, 0, None).sum():.0f} m3, "
          f"fill {np.clip(new - old, 0, None).sum():.0f} m3")
    if "--preview" in sys.argv:
        out = Path(sys.argv[sys.argv.index("--preview") + 1])
        img = np.concatenate([hillshade(old), hillshade(new)], axis=1)
        Image.fromarray((img * 255).astype(np.uint8)).resize((img.shape[1] * 3, img.shape[0] * 3)).save(out)
        print("preview", out)
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    moved = np.argwhere(u16 != raw)
    print(f"mine site: {len(moved)} vertices changed")
    if dry or not len(moved):
        return
    u16.astype("<u2").tofile(route_kit.R16)
    Image.fromarray(u16).save(route_kit.PNG)
    npy = Path(route_kit.WORK) / "game_reshaped_4033.npy"
    if npy.exists():
        zr = (u16.astype(np.float64) - 32768.0) / 128.0
        zn = np.load(npy)
        r, c = moved[:, 0], moved[:, 1]
        zn[2 * H - c, r] = zr[r, c].astype(zn.dtype)
        np.save(npy, zn)
    (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
    print(f"mine site: ApplyEstateHeightfield MinX {c0} MinY {r0} MaxX {c1} MaxY {r1}")
    mine = layout["landmarks"]["MineEntrance"]
    layout["landmarks"]["MineEntrance"] = [mine[0], mine[1], YARD_LEVEL, mine[3]]
    layout["mineSite"] = {"regraded": True, "smoothed": smooth or layout.get("mineSite", {}).get("smoothed", False),
                          "yardLevel": YARD_LEVEL, "rect": [int(c0), int(r0), int(c1), int(r1)]}
    LAYOUT.write_text(json.dumps(layout, indent=1))
    text = MINE_LAYOUT.read_text()
    text = re.sub(r"HOUSE_WORLD = \(-410\.0, -722\.0, [\d.]+\)", f"HOUSE_WORLD = (-410.0, -722.0, {YARD_LEVEL})", text)
    MINE_LAYOUT.write_text(text)
    print("mine site: now set MineEntrance's z to", round(YARD_LEVEL * 100, 1), "cm in HomesteadEstate.cpp")


if __name__ == "__main__":
    main()
