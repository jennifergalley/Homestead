"""Bake the estate road's signed-distance texture for the landscape material's cart ruts.

The DirtRoad weightmap is 1 m per pixel, far too coarse for wheel tracks. This writes the signed
distance (m) from the road centreline instead: it varies linearly across the road, so bilinear
filtering keeps it accurate to centimetres and the material can draw two wheel ruts, the grass
crown between them and the trodden shoulders from it (build_landscape_material.py).

Mapping (world cm, +X north, +Y east): u = (X + 201600) / 403200 along columns,
v = (Y + 201600) / 403200 down rows. Value 0.5 is the centreline, 0 and 1 are -REACH and +REACH.

    python Scripts\\Terrain\\road_ruts.py      -> Scripts\\Terrain\\T_EstateRoadSDF.png
"""
import json
import os

import numpy as np
from PIL import Image
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
SIZE = 2048
HALF_M = 2016.0   # half the map in metres
REACH = 4.0       # metres either side of the centreline that the texture resolves


def densify(p, step):
    p = np.asarray(p, np.float64)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]


def main():
    layout = json.load(open(os.path.join(HERE, "estate_layout.json")))
    road = densify(layout["road"], 0.25)
    tangent = np.gradient(road, axis=0)
    tangent /= np.linalg.norm(tangent, axis=1, keepdims=True)

    texel = 2 * HALF_M / SIZE
    centres = -HALF_M + (np.arange(SIZE) + 0.5) * texel
    x0, x1 = road[:, 0].min() - REACH - 2 * texel, road[:, 0].max() + REACH + 2 * texel
    y0, y1 = road[:, 1].min() - REACH - 2 * texel, road[:, 1].max() + REACH + 2 * texel
    cols = np.nonzero((centres >= x0) & (centres <= x1))[0]
    rows = np.nonzero((centres >= y0) & (centres <= y1))[0]
    rr, cc = np.meshgrid(rows, cols, indexing="ij")
    px = np.c_[centres[cc].ravel(), centres[rr].ravel()]

    # The texel's reach must cover bilinear taps just outside the road band, so query a bit further.
    dist, idx = cKDTree(road).query(px, distance_upper_bound=REACH + 2 * texel)
    near = np.isfinite(dist)
    sd = np.full(len(px), REACH, np.float64)
    q = road[idx[near]]
    t = tangent[idx[near]]
    rel = px[near] - q
    # +Y is east, so a positive cross product puts the texel on the road's right-hand side.
    side = np.sign(t[:, 0] * rel[:, 1] - t[:, 1] * rel[:, 0])
    side[side == 0] = 1
    sd[near] = side * dist[near]
    # Far texels keep +REACH; the material only reads the band where |d| < REACH.
    img = np.full((SIZE, SIZE), 255, np.uint8)
    img[rr, cc] = np.clip(np.round((sd.reshape(rr.shape) / REACH * 0.5 + 0.5) * 255), 0, 255).astype(np.uint8)
    out = os.path.join(HERE, "T_EstateRoadSDF.png")
    Image.fromarray(img, "L").save(out)
    print(f"{out}: {SIZE}^2, {texel:.2f} m/texel, road {len(road) * 0.25:.0f} m, band {rows.size}x{cols.size} texels")


if __name__ == "__main__":
    main()
