"""Derive the landscape paint layers from the reshaped heightmap and the layout.

Writes 8-bit weightmaps, one per layer, to <work>/weights/<Layer>.png in Unreal import orientation
(column = +X north, row = +Y east). Weights sum to 255 at every vertex.
"""
import json, os
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter, distance_transform_edt
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
LAYERS = ["Pasture", "WoodlandFloor", "Moorland", "DuneSand", "Beach", "CliffRock", "DirtRoad"]
H = 2016

def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)

def near_polyline(pts, shape, reach):
    """Distance (m) from every vertex to a polyline, capped at reach (computed in a bounding window)."""
    d = np.full(shape, reach, np.float32)
    pts = np.asarray(pts)
    r0 = max(int(H - pts[:, 0].max() - reach), 0); r1 = min(int(H - pts[:, 0].min() + reach), shape[0] - 1)
    c0 = max(int(H + pts[:, 1].min() - reach), 0); c1 = min(int(H + pts[:, 1].max() + reach), shape[1] - 1)
    rr, cc = np.mgrid[r0:r1 + 1, c0:c1 + 1]
    dist, _ = cKDTree(pts).query(np.c_[(H - rr).ravel(), (cc - H).ravel()], distance_upper_bound=reach)
    d[r0:r1 + 1, c0:c1 + 1] = np.minimum(dist, reach).reshape(rr.shape)
    return d

def densify(p, step=1.0):
    p = np.asarray(p, np.float64)
    seg = np.linalg.norm(np.diff(p, axis=0), axis=1)
    s = np.r_[0, np.cumsum(seg)]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]

def main():
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy")).astype(np.float32)
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    sea = z < 0.0
    d_water = distance_transform_edt(~sea).astype(np.float32)
    road = near_polyline(densify(L["road"]), z.shape, 20.0)
    river = near_polyline(densify(L["river"]), z.shape, 160.0)
    rows, cols = np.mgrid[:z.shape[0], :z.shape[1]]
    gxm = (H - rows).astype(np.float32)

    w = {}
    w["CliffRock"] = smoothstep(30, 42, slope)
    w["Beach"] = (1 - smoothstep(2.5, 5.0, z)) * (1 - smoothstep(30, 60, d_water)) * (1 - w["CliffRock"])
    # Dunes on the far (south-eastern) coast, just back from the beaches.
    rr = gaussian_filter(np.random.default_rng(7).random(z.shape).astype(np.float32), 25) * 10
    east = smoothstep(300, 600, (cols - H).astype(np.float32))
    w["DuneSand"] = east * (1 - smoothstep(12, 22, z)) * (1 - smoothstep(60, 160, d_water)) * smoothstep(0.35, 0.65, rr - rr.mean() + 0.5)
    w["Moorland"] = smoothstep(125, 150, z + 20 * (rr - rr.mean())) + smoothstep(900, 1400, gxm) * 0.6
    w["WoodlandFloor"] = (1 - smoothstep(60, 140, river)) * (1 - smoothstep(0.0, 1.0, np.maximum(z - 95, 0) / 20))
    w["DirtRoad"] = 1 - smoothstep(2.0, 3.6, road)
    # The town's street and its open square are packed earth too (town_layout.py).
    town = L.get("town")
    if town:
        street = near_polyline(np.asarray(town["street"]), z.shape, 20.0)
        w["DirtRoad"] = np.maximum(w["DirtRoad"], 1 - smoothstep(town["streetHalfWidth"] - 0.6, town["streetHalfWidth"] + 0.8, street))
        (cx, cy), hx, hy = town["square"]["centre"], town["square"]["halfX"], town["square"]["halfY"]
        out = np.maximum(np.abs(gxm - cx) - hx, np.abs((cols - H).astype(np.float32) - cy) - hy)
        w["DirtRoad"] = np.maximum(w["DirtRoad"], 1 - smoothstep(-0.5, 1.5, out))
    w["Pasture"] = np.full(z.shape, 0.35, np.float32)

    # Priority stack: road over cliff over beach/dune over woodland/moor over pasture.
    order = ["DirtRoad", "CliffRock", "Beach", "DuneSand", "WoodlandFloor", "Moorland", "Pasture"]
    remaining = np.ones(z.shape, np.float32)
    out = {}
    for name in order:
        v = np.clip(w[name], 0, 1) * remaining if name != "Pasture" else remaining
        out[name] = v
        remaining = remaining - v
    total = sum(out.values())
    os.makedirs(os.path.join(WORK, "weights"), exist_ok=True)
    acc = np.zeros(z.shape, np.int32)
    quant = {}
    for name in order[:-1]:
        q = np.minimum(np.round(out[name] / total * 255).astype(np.int32), 255 - acc)
        acc += q
        quant[name] = q
    quant["Pasture"] = 255 - acc
    for name in LAYERS:
        img = quant[name].astype(np.uint8)[::-1, :].T
        Image.fromarray(img).save(os.path.join(WORK, "weights", f"{name}.png"))
        print(name, round(float(quant[name].mean() / 255), 3))

if __name__ == "__main__":
    main()
