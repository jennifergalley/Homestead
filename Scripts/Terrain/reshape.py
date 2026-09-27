"""Reshape the resampled LIDAR frame into the Trevennor estate layout and export the master heightmap.

Input:  <work>/game_raw_4033.npy from resample_game_frame.py (game frame, metres, row 0 = game north).
Output: Content-independent artefacts in Scripts/Terrain/ (committed):
          Estate_Heightmap_4033.png  16-bit, Unreal import orientation (column = +X north, row = +Y east)
          estate_layout.json         anchors, polygons, road and river centrelines (game metres, z in metres)
        and <work>/game_reshaped_4033.npy for previews.

Game frame: +X north, +Y east, metres, origin at the map centre. 1 m per landscape quad.
"""
import json, os
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter, gaussian_filter1d, distance_transform_edt, label
from scipy.spatial import cKDTree
from skimage.graph import route_through_array

HERE = os.path.dirname(os.path.abspath(__file__))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
SIZE, H = 4033, 2016
Z_SCALE = 100.0          # Landscape actor Z scale: 1 unit = Z_SCALE/128 cm, range +/-256 m
SEA_LEVEL = 0.0

def rc(x, y):
    return int(round(H - x)), int(round(H + y))

def xy_of(r, c):
    return H - r, c - H

def smoothstep(t):
    t = np.clip(t, 0.0, 1.0)
    return t * t * (3 - 2 * t)

def densify(points, step=1.0, closed=False):
    """Catmull-Rom through control points, sampled every ~step metres."""
    p = np.asarray(points, np.float64)
    ext = np.vstack([p[0] * 2 - p[1], p, p[-1] * 2 - p[-2]])
    out = []
    for i in range(1, len(ext) - 2):
        p0, p1, p2, p3 = ext[i - 1], ext[i], ext[i + 1], ext[i + 2]
        n = max(2, int(np.linalg.norm(p2 - p1) / step))
        t = np.linspace(0, 1, n, endpoint=False)[:, None]
        out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t ** 2
                          + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    out.append(p[-1:])
    return np.vstack(out)

def resample_polyline(p, step=1.0):
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    s = np.arange(0, d[-1], step)
    return np.c_[np.interp(s, d, p[:, 0]), np.interp(s, d, p[:, 1])], s

def sample(z, pts):
    r = np.clip(np.round(H - pts[:, 0]).astype(int), 0, SIZE - 1)
    c = np.clip(np.round(H + pts[:, 1]).astype(int), 0, SIZE - 1)
    return z[r, c]

def corridor(pts, reach):
    """Pixels within reach of the polyline: (rows, cols, distance, nearest index)."""
    r0, c0 = rc(pts[:, 0].max() + reach, pts[:, 1].min() - reach)
    r1, c1 = rc(pts[:, 0].min() - reach, pts[:, 1].max() + reach)
    r0, c0, r1, c1 = max(r0, 0), max(c0, 0), min(r1, SIZE - 1), min(c1, SIZE - 1)
    rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
    gx, gy = xy_of(rr.ravel(), cc.ravel())
    d, i = cKDTree(pts).query(np.c_[gx, gy], distance_upper_bound=reach)
    keep = np.isfinite(d)
    return rr.ravel()[keep], cc.ravel()[keep], d[keep], i[keep]

def least_cost_path(z, start, end, margin=160):
    """Valley-floor route between two game points on smoothed terrain."""
    zs = gaussian_filter(z, 4)
    (ra, ca), (rb, cb) = rc(*start), rc(*end)
    r0, r1 = max(min(ra, rb) - margin, 0), min(max(ra, rb) + margin, SIZE - 1)
    c0, c1 = max(min(ca, cb) - margin, 0), min(max(ca, cb) + margin, SIZE - 1)
    sub = zs[r0:r1 + 1, c0:c1 + 1]
    cost = 1.0 + ((sub - sub.min()) / 1.5) ** 2
    path, _ = route_through_array(cost, (ra - r0, ca - c0), (rb - r0, cb - c0), fully_connected=True, geometric=True)
    path = np.array(path, np.float64) + [r0, c0]
    gx, gy = xy_of(path[:, 0], path[:, 1])
    pts = np.c_[gaussian_filter1d(gx, 10, mode="nearest"), gaussian_filter1d(gy, 10, mode="nearest")]
    return resample_polyline(pts, 1.0)[0]

def carve_channel(z, pts, bed, half_width, bank_slope, reach):
    """Cut (never fill) a channel: flat bed out to half_width, then bank_slope rise."""
    rr, cc, d, i = corridor(pts, reach)
    hw = half_width[i] if np.ndim(half_width) else half_width
    target = bed[i] + np.maximum(0.0, d - hw) * bank_slope
    z[rr, cc] = np.minimum(z[rr, cc], target)

def grade_corridor(z, pts, profile, flat, falloff):
    rr, cc, d, i = corridor(pts, flat + falloff)
    w = 1.0 - smoothstep((d - flat) / falloff)
    z[rr, cc] = z[rr, cc] + (profile[i] - z[rr, cc]) * w

def pad(z, center, half, falloff, mode="flat", target=None, yaw_deg=0.0):
    """Flatten (or plane-fit) a rectangle, blending out over falloff metres."""
    cx, cy = center
    reach = max(half) + falloff
    r0, c0 = rc(cx + reach, cy - reach)
    r1, c1 = rc(cx - reach, cy + reach)
    rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
    gx, gy = xy_of(rr, cc)
    t = np.radians(yaw_deg)
    u = (gx - cx) * np.cos(t) + (gy - cy) * np.sin(t)
    v = -(gx - cx) * np.sin(t) + (gy - cy) * np.cos(t)
    out = np.hypot(np.maximum(np.abs(u) - half[0], 0), np.maximum(np.abs(v) - half[1], 0))
    w = 1.0 - smoothstep(out / falloff)
    patch = z[r0:r1 + 1, c0:c1 + 1]
    inside = out == 0
    if mode == "plane":
        A = np.c_[gx[inside], gy[inside], np.ones(inside.sum())]
        k = np.linalg.lstsq(A, patch[inside], rcond=None)[0]
        tgt = k[0] * gx + k[1] * gy + k[2]
    else:
        tgt = np.full_like(patch, np.median(patch[inside]) if target is None else target)
    z[r0:r1 + 1, c0:c1 + 1] = patch + (tgt - patch) * w
    return float(np.median(tgt[inside]))

# ---- The agreed layout (game metres) -------------------------------------------------------
MANOR_CENTER = (-250.0, -650.0)          # ruin long axis runs east-west, facing the sea
MANOR_RECT = (-259.0, -241.0, -665.0, -635.0)   # x0, x1, y0, y1 (18 x 30 m)
ROOM = (-259.0, -251.0, -643.0, -635.0)  # standing room: the carved-out south-east corner, 8 x 8 m
MINE = (-405.0, -1000.0)
MILL = (-118.0, -92.0)
FORD = (-15.0, -30.0)
TOWN_SQUARE = (-540.0, 1150.0)
COVE = (-650.0, -515.0)
RIVER_SOURCE, RIVER_MOUTH = (400.0, 255.0), (-640.0, -520.0)
ESTUARY_MOUTH, ESTUARY_HEAD = (-1345.0, 965.0), (-845.0, 1445.0)
ROAD = [(-232.0, -628.0), (-150.0, -560.0), (-40.0, -430.0), (20.0, -300.0), (5.0, -150.0), FORD,
        (-55.0, 90.0), (-150.0, 230.0), (-220.0, 450.0), (-260.0, 650.0), (-330.0, 880.0),
        (-430.0, 1030.0), (-500.0, 1090.0)]
ESTATE_GATEWAY = (-55.0, 90.0)
ESTATE_BOUNDARY = [(160.0, -1150.0), (160.0, -250.0), (60.0, -30.0), (-40.0, 110.0), (-200.0, -50.0),
                   (-350.0, -210.0), (-520.0, -320.0), (-760.0, -430.0), (-760.0, -1150.0)]
FOR_SALE = {
    "Woodland": [(160.0, -900.0), (600.0, -900.0), (600.0, -250.0), (160.0, -250.0)],
    "MoorField": [(600.0, -1500.0), (1000.0, -1500.0), (1000.0, -900.0), (600.0, -900.0)],
    "WestCove": [(160.0, -1600.0), (160.0, -1150.0), (-760.0, -1150.0), (-760.0, -1600.0)],
}

def main():
    z = np.load(os.path.join(WORK, "game_raw_4033.npy")).astype(np.float64)
    z = np.where(np.isfinite(z), z, 0.0)

    # 1. Sea: everything low and connected to the southern edge becomes seabed that deepens offshore.
    lab, _ = label(z < 2.5)
    ids = np.setdiff1d(np.unique(lab[-1, :]), [0])
    sea = np.isin(lab, ids)
    d_sea = distance_transform_edt(sea)
    seabed = SEA_LEVEL - 0.6 - 22.0 * (1.0 - np.exp(-d_sea / 160.0))
    z = np.where(sea, np.minimum(z, seabed), z)

    # 2. Estuary: sink the south-east valley below sea level (a ria), keeping its natural shape.
    est = least_cost_path(z, ESTUARY_MOUTH, ESTUARY_HEAD)
    f = np.linspace(0, 1, len(est))
    natural = np.maximum.accumulate(gaussian_filter1d(sample(gaussian_filter(z, 4), est), 15, mode="nearest"))
    drop = np.maximum(natural - (-2.6 + 1.6 * f), 0.0)
    rr, cc, d, i = corridor(est, 160.0)
    z[rr, cc] -= drop[i] * (1.0 - smoothstep((d - 40.0) / 110.0))
    del rr, cc, d, i

    # 3. Pads: the manor, the mine head, the mill and the town.
    manor_z = pad(z, MANOR_CENTER, (15.0, 21.0), 28.0)
    mine_z = pad(z, MINE, (12.0, 12.0), 16.0)
    town_z = pad(z, TOWN_SQUARE, (85.0, 70.0), 60.0, mode="plane")

    # 4. The road: a gradient-limited profile graded into a 5 m bed with soft verges.
    road = resample_polyline(densify(ROAD, 1.0), 1.0)[0]
    prof = gaussian_filter1d(sample(gaussian_filter(z, 3), road), 22, mode="nearest")
    for _ in range(3):
        for k in range(1, len(prof)):
            prof[k] = np.clip(prof[k], prof[k - 1] - 0.11, prof[k - 1] + 0.11)
        for k in range(len(prof) - 2, -1, -1):
            prof[k] = np.clip(prof[k], prof[k + 1] - 0.11, prof[k + 1] + 0.11)
    grade_corridor(z, road, prof, flat=2.8, falloff=12.0)

    # 5. The estate river down the wooded valley into the cove (cut after the road, so it fords).
    riv = least_cost_path(z, RIVER_SOURCE, RIVER_MOUTH)
    floor = np.minimum.accumulate(gaussian_filter1d(sample(gaussian_filter(z, 2), riv), 6, mode="nearest"))
    carve_channel(z, riv, bed=floor - 0.9, half_width=2.2, bank_slope=0.45, reach=10.0)
    mill_z = pad(z, MILL, (7.0, 7.0), 6.0, target=float(sample(z, np.array([MILL]))[0]) + 0.6)
    carve_channel(z, riv, bed=floor - 0.9, half_width=2.2, bank_slope=0.45, reach=10.0)

    # 6. The cove: a sand beach rising gently inland from the valley mouth.
    (r0, c0), (r1, c1) = rc(COVE[0] + 260, COVE[1] - 260), rc(COVE[0] - 260, COVE[1] + 260)
    rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
    gx, gy = xy_of(rr, cc)
    ax = np.array([0.81, 0.59]) / np.linalg.norm([0.81, 0.59])
    s = (gx - (COVE[0] - 40)) * ax[0] + (gy - (COVE[1] - 30)) * ax[1]
    lat = -(gx - (COVE[0] - 40)) * ax[1] + (gy - (COVE[1] - 30)) * ax[0]
    plane = -1.4 + 0.05 * s
    w = (1 - smoothstep((np.abs(lat) - 45) / 30)) * (1 - smoothstep((s - 110) / 40)) * (1 - smoothstep((-s - 60) / 40))
    patch = z[r0:r1 + 1, c0:c1 + 1]
    w *= np.clip(1 - np.abs(patch - plane) / 9.0, 0, 1)
    z[r0:r1 + 1, c0:c1 + 1] = patch + (plane - patch) * w

    # 7. Soften the waterline so shores read as beaches and cliffs, not steps.
    band = distance_transform_edt(~sea) < 6
    band |= (d_sea > 0) & (d_sea < 6)
    z = np.where(band, gaussian_filter(z, 2.0), z)

    np.save(os.path.join(WORK, "game_reshaped_4033.npy"), z.astype(np.float32))

    # Export: Unreal reads image column as +X and row as +Y, so column = game north, row = game east.
    img = z[::-1, :].T
    u16 = np.clip(np.round(32768 + img * 12800.0 / Z_SCALE), 0, 65535).astype(np.uint16)
    Image.fromarray(u16).save(os.path.join(HERE, "Estate_Heightmap_4033.png"))
    # The runtime heightfield the game samples for ground height (row = +Y, column = +X, little-endian).
    runtime = os.path.join(HERE, "..", "..", "Content", "SurvivalGame", "Estate", "Runtime")
    os.makedirs(runtime, exist_ok=True)
    u16.astype("<u2").tofile(os.path.join(runtime, "EstateHeightfield.r16"))

    def zat(p):
        return round(float(sample(z, np.array([p]))[0]), 2)
    room_c = ((ROOM[0] + ROOM[1]) / 2, (ROOM[2] + ROOM[3]) / 2)
    x0, x1, y0, y1 = MANOR_RECT
    rx0, rx1, ry0, ry1 = ROOM
    layout = {
        "zScale": Z_SCALE, "seaLevel": SEA_LEVEL, "size": SIZE,
        "landmarks": {
            "StandingRoomOrigin": [*room_c, manor_z, 0.0],
            "StandingRoomSpawn": [room_c[0] + 1.5, room_c[1] - 1.5, manor_z, 90.0],
            "EstateGateway": [*ESTATE_GATEWAY, zat(ESTATE_GATEWAY), 60.0],
            "CoveBeach": [*COVE, zat(COVE), 215.0],
            "MineEntrance": [*MINE, mine_z, 0.0],
            "MillSite": [*MILL, mill_z, 0.0],
            "RoadEstateEnd": [*ESTATE_GATEWAY, zat(ESTATE_GATEWAY), 60.0],
            "RoadTownEnd": [*ROAD[-1], zat(ROAD[-1]), 45.0],
            "TownSquare": [*TOWN_SQUARE, zat(TOWN_SQUARE), 0.0],
            "GeneralStoreDoor": [TOWN_SQUARE[0], TOWN_SQUARE[1] + 20.0, zat((TOWN_SQUARE[0], TOWN_SQUARE[1] + 20.0)), 90.0],
            "GeneralStoreCounter": [TOWN_SQUARE[0], TOWN_SQUARE[1] + 26.0, zat((TOWN_SQUARE[0], TOWN_SQUARE[1] + 26.0)), -90.0],
        },
        "polygons": {
            "EstateBoundary": ESTATE_BOUNDARY,
            "ManorFootprint": [(x1, y0), (x1, y1), (rx1, ry1), (rx1, ry0), (rx0, ry0), (x0, y0)],
            **{"ForSale." + k: v for k, v in FOR_SALE.items()},
        },
        "road": np.round(road[::4], 2).tolist(),
        "roadProfile": np.round(prof[::4], 2).tolist(),
        "river": np.round(riv[::4], 2).tolist(),
        "riverBed": np.round((floor - 0.9)[::4], 2).tolist(),
        "estuary": np.round(est[::4], 2).tolist(),
        "townZ": town_z,
    }
    with open(os.path.join(HERE, "estate_layout.json"), "w") as fh:
        json.dump(layout, fh, indent=1)
    print("z", round(float(z.min()), 2), round(float(z.max()), 2), "manor", manor_z, "mine", mine_z,
          "mill", mill_z, "town", town_z, "road m", len(road), "river m", len(riv), "estuary m", len(est))

if __name__ == "__main__":
    main()
