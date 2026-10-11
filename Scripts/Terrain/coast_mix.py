"""Coast mix (map stage 3, Jenny's 2026-10-10 terrain feedback): about half the coast's cliffs become grassy
slopes that run down to the sand, the cove's path follows a natural gradient instead of a stair down a cliff,
and the footpaths lie flush with the ground.

Usage: python Scripts/Terrain/coast_mix.py [--dry]     (then cove_route.py, see Scripts/Terrain/README.md)

Run once ("coastMix.graded" in the layout). Over the stage-2 ground (compact_map.py's chain) it:
1. Restores the stage-2 bay, cove route and footpath cuts to the ground before them (f9cfadaf6, less the first
   cove route's cut, as cove_bay.py took it out), keeping the river channel and the mine yard as graded.
2. Re-carves the cove bay (cove_bay.py's design) with walls whose steepness varies smoothly (cove_bay's 6 m blocks
   of steepness left terraces and cross-hatching on the walls) and a rounded mouth on its west side.
3. Turns the bay's head and east side into a concave grassy hillside rising from the sand (SLOPE), and does the
   same on the Gull Sands coast east of the river mouth, cutting a dry sand fringe at the cliff foot. The west
   wall stays a cliff (the cave's mouth is in it).
4. Lays the footpaths flush: the mine, mine-link and village-river paths' cuts and banks are taken out.
5. Outside the playable rectangle, raises the gentle grassy banks that run into the sea into cliff faces, so the
   coast seen from the map's edge reads as cliffs; sand beaches keep their slopes.

Writes EstateHeightfield.r16, Estate_Heightmap_4033.png and the work npy where they changed, and "coastMix" in
estate_layout.json (removing "coveRoute" so cove_route.py lays the new path); prints the ApplyEstateHeightfield
rectangles. --dry writes previews to Saved/CoastMix/ only.
"""
import json
import os
import subprocess
import sys

import numpy as np
from matplotlib.path import Path
from PIL import Image
from scipy import ndimage
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
R16_GIT = "Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16"
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SAVED = os.path.join(REPO, "Saved", "CoastMix")
SIZE, H = 4033, 2016

BEFORE_COMPACT = "f9cfadaf6"                       # the ground before stage 2
OLD_ROUTE_BEFORE, OLD_ROUTE_AFTER = "c1e426df2^", "6a987b7b2"   # the first cove route's cut (cove_bay.py)
STAGE2_BOX = (-730.0, -250.0, -760.0, -420.0)      # x0, x1, y0, y1 (m): the bay, its route and the footpaths
VILLAGE_PATH_BOX = (-290.0, -100.0, -340.0, -280.0)

# The bay's water (x north, y east, m), cove_bay.py's with its west mouth rounded: the straight west waterline used
# to meet the beach belt's shore in a sharp point.
BAY = [(-700, -700), (-672, -690), (-640, -676), (-608, -667), (-578, -661), (-540, -651), (-508, -636),
       (-492, -618), (-496, -600), (-516, -590), (-548, -584), (-566, -566), (-578, -536), (-700, -515)]
FRINGE_M = 28.0
TOP, SWASH, BERM_M, FORESHORE, BED_MIN = 1.7, 0.3, 12.0, 0.10, -4.0
CLIFF_SLOPE = (1.1, 1.8)      # rise per metre of the bay's walls, varying smoothly along them (rock)
CLIFF_KNOT_M = 22.0
SEED = 2610
# Along the bay's waterline from its west mouth (m): the hillside rises from the sand beyond SLOPE_FROM_M (the head
# and the east side). WEST_CLIFF (x, y) keeps the west wall a cliff (the sea cave's mouth is in it), fading into the
# hillside over WEST_FADE_M; the manor's terrace stays as it is within MANOR_KEEP_M of the standing room.
SLOPE_FROM_M = 250.0
WEST_CLIFF = [(-720, -800), (-720, -628), (-560, -628), (-482, -662), (-482, -800)]
WEST_PLATEAU = [(-482, -800), (-482, -662), (-430, -652), (-300, -652), (-300, -800)]   # the mine headland
WEST_PLATEAU_FADE_M = 75.0
WEST_FADE_M = 30.0
MANOR_KEEP_M = 45.0
MANOR_FADE_M = 45.0
RIVER_CLEAR_M, RIVER_RAMP_M = 6.0, 6.0

# A concave grassy hillside: from the sand's top it rises at SLOPE[0] per metre, steepening smoothly to SLOPE[1]
# over SLOPE_RAMP_M, and meets the old ground in a rounded brow (smooth minimum over BROW_M).
SLOPE = (0.07, 0.38)
SLOPE_RAMP_M = 80.0
BROW_M = 7.0

# Coastal slope stretches outside the bay: polygon (x, y) where the hillside replaces the cliff, its sand width.
GULL_SANDS = {"poly": [(-640, -505), (-600, -470), (-640, -360), (-860, -360), (-860, -505)],
              "sand": 24.0, "edge": 130.0, "reach": 170.0, "slope": (0.15, 0.6), "ramp": 35.0}

# Outside the playable rectangle: a grassy bank running into the sea becomes a cliff (outer_cliffs); banks whose
# ground CLIFF_SHIFT_M inland is lower than CLIFF_MIN_TOP_M, cliffs already, and sand beaches are left.
CLIFF_SHIFT_M = 35.0
CLIFF_BAND_M = 70.0
CLIFF_FACE_M = 5.0
CLIFF_MIN_TOP_M = 10.0
CLIFF_GENTLE = 0.9            # a bank already steeper than this (mean rise per metre) is a cliff already
BEACH_WIDTH_M = 8.0


def load_cur():
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    return raw, (raw.astype(np.float64) - 32768.0) / 128.0


def git_r16(rev):
    pointer = subprocess.run(["git", "show", f"{rev}:{R16_GIT}"], cwd=REPO, capture_output=True, check=True).stdout
    data = subprocess.run(["git", "lfs", "smudge"], cwd=REPO, input=pointer, capture_output=True, check=True).stdout
    return (np.frombuffer(data, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0


def grid(x0, x1, y0, y1):
    xs, ys = np.arange(int(x0), int(x1) + 1), np.arange(int(y0), int(y1) + 1)
    gx, gy = np.meshgrid(xs, ys)
    return (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1)), gx, gy


def inside(poly, gx, gy):
    return Path(np.asarray(poly, float)).contains_points(np.c_[gx.ravel(), gy.ravel()]).reshape(gx.shape)


def dense_ring(poly, step=0.5):
    edge = np.asarray(list(poly) + [poly[0]], float)
    out, arc, s = [], [], 0.0
    for a, b in zip(edge[:-1], edge[1:]):
        n = max(2, int(np.hypot(*(b - a)) / step))
        t = np.linspace(0, 1, n, endpoint=False)
        out.append(a + (b - a) * t[:, None])
        arc.append(s + np.hypot(*(b - a)) * t)
        s += np.hypot(*(b - a))
    return np.concatenate(out), np.concatenate(arc), s


def signed_distance(poly, gx, gy):
    ring, arc, _ = dense_ring(poly)
    d, j = cKDTree(ring).query(np.c_[gx.ravel(), gy.ravel()])
    d = d.reshape(gx.shape)
    return np.where(inside(poly, gx, gy), -d, d), arc[j].reshape(gx.shape)


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def smin(a, b, k):
    """Smooth minimum: min(a, b) with the crease rounded over k (k may vary per cell)."""
    k = np.maximum(k, 1e-6)
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1 - h)


def brow(old, surface):
    """min(old, surface) with the crease rounded over BROW_M where they meet up on the hill (the brow), but not
    down at the sand, where rounding would dig a trough below both."""
    lo = np.minimum(old, surface)
    return np.maximum(smin(old, surface, BROW_M * smoothstep(TOP + 1.0, TOP + 8.0, lo)), np.minimum(lo, TOP))


def hillside(u, slope=None, ramp_m=None):
    """Height above the sand's top at u metres inland of it: concave, from SLOPE[0] to SLOPE[1] per metre."""
    u = np.maximum(u, 0.0)
    t = np.clip(u / SLOPE_RAMP_M, 0.0, 1.0)
    ramp = SLOPE_RAMP_M * (t ** 3 - 0.5 * t ** 4)          # integral of smoothstep over [0, u] (u <= ramp)
    ramp = np.where(u > SLOPE_RAMP_M, 0.5 * SLOPE_RAMP_M + (u - SLOPE_RAMP_M), ramp)
    return SLOPE[0] * u + (SLOPE[1] - SLOPE[0]) * ramp


def smooth_field(gx, gy, knot_m, seed):
    """A smoothly varying 0..1 field (bicubic through random knots knot_m apart): no blocks, no seams."""
    rng = np.random.default_rng(seed)
    nx, ny = int(np.ptp(gx) / knot_m) + 4, int(np.ptp(gy) / knot_m) + 4
    knots = ndimage.gaussian_filter(rng.uniform(0, 1, (ny, nx)), 0.7)
    knots = (knots - knots.min()) / max(np.ptp(knots), 1e-9)
    r = (gy - gy.min()) / knot_m + 1.0
    c = (gx - gx.min()) / knot_m + 1.0
    return np.clip(ndimage.map_coordinates(knots, [r, c], order=3, mode="nearest"), 0, 1)


def river_weight(layout, gx, gy):
    """0 on the river channel (and RIVER_CLEAR_M beyond its waterline), 1 clear of it."""
    riv = np.asarray(layout["river"], np.float64)
    half = np.r_[np.asarray(layout["riverHalfWidth"], np.float64),
                 np.full(len(riv) - len(layout["riverHalfWidth"]), layout["riverHalfWidth"][-1])]
    dense, hw = [], []
    for i in range(len(riv) - 1):
        n = max(2, int(np.hypot(*(riv[i + 1] - riv[i])) / 0.5))
        t = np.linspace(0, 1, n, endpoint=False)
        dense.append(riv[i] + (riv[i + 1] - riv[i]) * t[:, None])
        hw.append(half[i] + (half[i + 1] - half[i]) * t)
    dense, hw = np.concatenate(dense), np.concatenate(hw)
    d, j = cKDTree(dense).query(np.c_[gx.ravel(), gy.ravel()])
    d = (d - hw[j]).reshape(gx.shape)
    return np.clip((d - RIVER_CLEAR_M) / RIVER_RAMP_M, 0.0, 1.0)


def restore_stage2(z, base, cur, layout):
    """Take stage 2's bay, route and footpath cuts out: back to the ground before them, except the river channel
    and the mine yard (estate_footpaths.YARD), which stay as graded."""
    for box in (STAGE2_BOX, VILLAGE_PATH_BOX):
        sl, gx, gy = grid(*box)
        changed = np.abs(cur[sl] - base[sl]) > 0.004
        keep = 1.0 - river_weight(layout, gx, gy)
        yard = layout.get("estateFootpaths", {}).get("yard")
        if yard:
            (cx, cy), (hx, hy), blend = yard["centre"], yard["half"], yard["blend"]
            out = np.maximum(np.abs(gx - cx) - hx, np.abs(gy - cy) - hy).clip(0, None)
            keep = np.maximum(keep, 1.0 - smoothstep(0.0, blend + 1.0, out))
        z[sl] = np.where(changed, base[sl] + (cur[sl] - base[sl]) * keep, z[sl])


def carve_bay(z, layout):
    """cove_bay.py's bay over the restored ground with smoothly varying cliff steepness, then a concave hillside
    rising from the sand at the bay's head and east side. The west wall (WEST_CLIFF) keeps its cliff."""
    pad = FRINGE_M + 230.0
    bx, by = zip(*BAY)
    sl, gx, gy = grid(min(bx) - 40, max(bx) + pad, min(by) - pad * 0.6, max(by) + pad * 1.4)
    old = z[sl].copy()
    d, arc = signed_distance(BAY, gx, gy)
    bed = np.maximum(SWASH + FORESHORE * d, BED_MIN)
    t = np.clip(d / (FRINGE_M - BERM_M), 0.0, 1.0)
    sand = SWASH + (TOP - SWASH) * (t * t * (3 - 2 * t))
    steep = CLIFF_SLOPE[0] + smooth_field(gx, gy, CLIFF_KNOT_M, SEED) * (CLIFF_SLOPE[1] - CLIFF_SLOPE[0])
    cliff = TOP + (d - FRINGE_M) * steep
    surface = np.where(d < 0, bed, np.where(d <= FRINGE_M, sand, cliff))
    carved = np.minimum(old, surface)
    # The hillside: distance inland from the top of the sand along the head and east side.
    seed = (d >= FRINGE_M - 1.0) & (d <= FRINGE_M + 0.5) & (arc >= SLOPE_FROM_M)
    u = ndimage.distance_transform_edt(~seed)
    hill = np.where(d > FRINGE_M, brow(old, TOP + hillside(u)), carved)
    keep, _ = signed_distance(WEST_CLIFF, gx, gy)
    plateau, _ = signed_distance(WEST_PLATEAU, gx, gy)
    wz = np.minimum(smoothstep(0.0, WEST_FADE_M, keep), smoothstep(0.0, WEST_PLATEAU_FADE_M, plateau))
    mx, my = layout["landmarks"]["StandingRoomOrigin"][:2]
    wz *= smoothstep(MANOR_KEEP_M, MANOR_KEEP_M + MANOR_FADE_M, np.hypot(gx - mx, gy - my))
    new = carved + wz * (np.minimum(carved, hill) - carved)
    w = river_weight(layout, gx, gy)
    z[sl] = old + (new - old) * w
    return sl, d

def sea_distance(z, sl, gx):
    """Metres from the sea (water connected to the south edge, under SWASH) over the window."""
    sea = z[sl] < SWASH
    lab, _ = ndimage.label(sea)
    south = np.unique(lab[:, gx[0] == gx.min()])
    sea = np.isin(lab, south[south > 0]) | np.isin(lab, np.unique(lab[:, 0])[1:])
    return ndimage.distance_transform_edt(~sea), ndimage.distance_transform_edt(sea)


def coastal_slope(z, layout, spec):
    """The coast inside spec["poly"] becomes a dry sand fringe under a grassy hillside graded by distance from the
    sea (spec["slope"] per metre, steepening over spec["ramp"] m), with gentle undulations. Over spec["edge"] m past
    the polygon it blends back to the old ground, so the cliffs either side dwindle into the slope as spurs no
    steeper than about 1 in 2 instead of ending in a wall."""
    poly = spec["poly"]
    px, py = zip(*poly)
    e = spec["edge"]
    sl, gx, gy = grid(min(px) - 40, max(px) + spec["reach"], min(py) - e - 20, max(py) + e + 20)
    old = z[sl].copy()
    dland, dsea = sea_distance(z, sl, gx)
    sd, _ = signed_distance(poly, gx, gy)
    W = spec["sand"]
    wobble = smooth_field(gx, gy, 35.0, SEED + 7) - 0.5
    u = np.maximum(dland - W, 0.0) * (1.0 + 0.25 * wobble)
    hill = TOP + hillside(u, spec["slope"], spec["ramp"]) + 1.2 * wobble * smoothstep(0.0, 20.0, u)
    t = np.clip(dland / (W - BERM_M), 0.0, 1.0)
    sand = SWASH + (TOP - SWASH) * (t * t * (3 - 2 * t))
    land = dland > 0
    graded = np.where(dland <= W, np.minimum(old, sand), brow(old, hill))
    sea = np.maximum(old, np.minimum(SWASH - FORESHORE * dsea, SWASH - 0.05))
    new = np.where(land, graded, sea)
    keep = (1.0 - smoothstep(0.0, e, sd)) * river_weight(layout, gx, gy)
    z[sl] = old + (new - old) * keep
    return sl

def outer_cliffs(z, playable):
    """Gentle banks into the sea outside the playable rectangle become cliffs: within CLIFF_BAND_M of the water the
    land's profile is pushed CLIFF_SHIFT_M seaward (each point takes the height CLIFF_SHIFT_M further inland), ending
    in a face CLIFF_FACE_M wide at the water. Only raises; leaves cliffs, sand beaches and low shores alone, and fades
    in over 80 m beyond the rectangle."""
    (x0, y0), (x1, y1) = np.min(playable, 0), np.max(playable, 0)
    g = np.arange(SIZE) - H
    X, Y = np.meshgrid(g, g)                     # z[row = y + H, col = x + H]
    lab, _ = ndimage.label(z < SWASH)
    sea = np.isin(lab, np.unique(lab[:, 0])[1:])             # water connected to the south (x = -H) edge
    dland = ndimage.distance_transform_edt(~sea)
    ds = ndimage.gaussian_filter(np.minimum(dland, CLIFF_BAND_M + 40.0), 4.0)
    gr, gc = np.gradient(ds)
    n = np.maximum(np.hypot(gr, gc), 1e-6)
    rows = (Y + H) + CLIFF_SHIFT_M * gr / n
    cols = (X + H) + CLIFF_SHIFT_M * gc / n
    shifted = ndimage.map_coordinates(z, [rows, cols], order=1, mode="nearest")
    face = shifted * smoothstep(0.0, CLIFF_FACE_M, dland) + SWASH * (1 - smoothstep(0.0, CLIFF_FACE_M, dland))
    ring = (dland > CLIFF_SHIFT_M - 2) & (dland < CLIFF_SHIFT_M + 2)
    top = (ndimage.gaussian_filter(np.where(ring, z, 0.0), 10.0)
           / np.maximum(ndimage.gaussian_filter(ring.astype(np.float64), 10.0), 1e-6))
    gentle = smoothstep(CLIFF_GENTLE, CLIFF_GENTLE * 0.6, top / CLIFF_SHIFT_M) * smoothstep(CLIFF_MIN_TOP_M * 0.6,
                                                                                            CLIFF_MIN_TOP_M, top)
    beach = ndimage.binary_opening((z < 2.3) & ~sea, iterations=int(BEACH_WIDTH_M / 2))
    beach_far = ndimage.distance_transform_edt(~beach)
    band = smoothstep(CLIFF_BAND_M, CLIFF_BAND_M * 0.6, dland) * (dland > 0)
    out = np.maximum.reduce([x0 - X, X - x1, y0 - Y, Y - y1]).astype(np.float64)
    w = (smoothstep(20.0, 100.0, out) * band * ndimage.gaussian_filter(gentle, 6.0)
         * smoothstep(10.0, 40.0, beach_far))
    return z + w * np.maximum(face - z, 0.0)

def coastline_mix(z, playable):
    """Share of the playable coast that's cliff (steeper than 1 in 1 within 40 m of the water) vs slope."""
    (x0, y0), (x1, y1) = np.min(playable, 0), np.max(playable, 0)
    sl, gx, gy = grid(x0, x1, y0, y1)
    zz = z[sl]
    sea = zz < SWASH
    edge = sea & ~ndimage.binary_erosion(sea)
    dland = ndimage.distance_transform_edt(~sea)
    gyy, gxx = np.gradient(ndimage.gaussian_filter(zz, 1.0))
    steep = np.hypot(gxx, gyy) > 1.0
    near = dland <= 70.0
    lab, n = ndimage.label(steep & near)
    steep_near = ndimage.binary_dilation(steep & near, iterations=70)
    pts = np.argwhere(edge)
    cliff = steep_near[pts[:, 0], pts[:, 1]]
    return float(cliff.mean()), pts, cliff, sl


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    if layout.get("coastMix", {}).get("graded"):
        print("coast mix: already graded")
        return
    raw, cur = load_cur()
    base = git_r16(BEFORE_COMPACT)
    delta = git_r16(OLD_ROUTE_AFTER) - git_r16(OLD_ROUTE_BEFORE)
    base = base - np.where(np.abs(delta) > 1e-6, delta, 0.0)
    z = cur.copy()
    restore_stage2(z, base, cur, layout)
    carve_bay(z, layout)
    coastal_slope(z, layout, GULL_SANDS)
    playable = np.asarray(layout["polygons"]["PlayableBounds"], float)
    z = outer_cliffs(z, playable)
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    zr = (u16.astype(np.float64) - 32768.0) / 128.0
    moved = np.argwhere(u16 != raw)
    share, _, _, _ = coastline_mix(zr, playable)
    share0, _, _, _ = coastline_mix(cur, playable)
    print(f"coast mix: {len(moved)} vertices changed; playable coast cliff share {share0:.0%} -> {share:.0%}")
    os.makedirs(SAVED, exist_ok=True)
    np.save(os.path.join(SAVED, "coast_mix.npy"), zr.astype(np.float32))
    if dry:
        print("coast mix: --dry, nothing written; heights in Saved/CoastMix/coast_mix.npy")
        return
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy):
        zn = np.load(npy)
        rows, cols = moved[:, 0], moved[:, 1]
        zn[2 * H - cols, rows] = zr[rows, cols].astype(zn.dtype)
        np.save(npy, zn)
    layout.pop("coveRoute", None)
    layout["footpaths"] = [p for p in layout.get("footpaths", []) if p.get("name") != "Cove"]
    layout["coastMix"] = {"graded": True, "bay": [list(p) for p in BAY], "gullSands": GULL_SANDS["poly"],
                          "westCliff": [list(p) for p in WEST_CLIFF], "slope": list(SLOPE), "cliffShare": round(share, 3)}
    layout["coveBay"]["bay"] = [list(p) for p in BAY]
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
    print(f"coast mix: ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")


if __name__ == "__main__":
    main()
