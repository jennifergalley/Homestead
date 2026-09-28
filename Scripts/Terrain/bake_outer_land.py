"""Bake the land beyond the Estate map's edge so the fixed map doesn't read as a raised plateau.

The 4 km landscape ends in 90-140 m walls on its north, east and west edges, where real Cornwall
carries on inland. This writes a low-detail ring mesh outside the map: it starts on the smoothed
edge heights (slightly inside and below the landscape, to hide the seam), drifts towards a rolling
~110 m upland with broad noise as it goes out, and reaches 70 km so it meets the horizon haze.
Where the map edge is sea, the ring sits below sea level under the ocean (bake_ocean.py), and off
the south coast headlands fall away into the Atlantic.

Outputs Saved/Terrain/SM_EstateOuterLand.obj (world cm, y negated for Interchange's OBJ reader);
build_outer_land.py imports it. Run from the repo root:
  python Scripts/Terrain/bake_outer_land.py
"""
import os

import numpy as np
from scipy.ndimage import binary_dilation, gaussian_filter, label, map_coordinates

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
SIZE, H = 4033, 2016
OUT = os.path.join(REPO, "Saved", "Terrain")

EDGE_STEP = 8.0      # metres between vertices along the map edge
OVERLAP = 6.0        # the ring starts this far inside the map, under the landscape
SINK = 0.6           # and this far below it
FAR = 70000.0        # metres beyond the edge (the ocean mesh reaches 60 km)
UPLAND = 110.0       # the height the hinterland settles to


def load_heights():
    raw = np.fromfile(os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16"), "<u2")
    return (raw.reshape(SIZE, SIZE).astype(np.float32) - 32768.0) / 128.0   # h[yi, xi]


def axis():
    inner = np.arange(-H + OVERLAP, H - OVERLAP + 0.01, EDGE_STEP)
    def grow(start, sign):
        out, step, p = [], EDGE_STEP, start
        while abs(p - start) < FAR + OVERLAP:
            # Fine enough out to 7 km for the coast to read, then coarser to the horizon.
            step = min(step * 1.12, 80.0) if abs(p - start) < 7000.0 else step * 1.15
            p += sign * step
            out.append(p)
        return out
    return np.concatenate([grow(inner[0], -1)[::-1], inner, grow(inner[-1], +1)])


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def ring():
    """The ring's grid: axes xs (north) and ys (east) in metres, and heights z[i, j] in metres.

    Other bakes (for example the ocean's shore texture) can sample the land past the map from this.
    """
    h = load_heights()
    hs = gaussian_filter(h, 24.0)
    xs, ys = axis(), axis()
    X, Y = np.meshgrid(xs, ys, indexing="ij")
    cx, cy = np.clip(X, -H, H), np.clip(Y, -H, H)
    d = np.hypot(X - cx, Y - cy)
    edge = map_coordinates(h, [cy + H, cx + H], order=1, mode="nearest")
    edge_s = map_coordinates(hs, [cy + H, cx + H], order=1, mode="nearest")
    # Near the seam follow the true edge; a few hundred metres out, only its smoothed shape.
    base = np.where(d < 1.0, edge, edge_s + (edge - edge_s) * np.exp(-d / 60.0))

    rng = np.random.default_rng(31)
    cells = 160
    noise = gaussian_filter(rng.standard_normal((cells, cells)), 2.5)
    noise /= noise.std()
    span = 2 * (FAR + H)
    nu = (X + FAR + H) / span * (cells - 1)
    nv = (Y + FAR + H) / span * (cells - 1)
    broad = map_coordinates(noise, [nu, nv], order=3, mode="reflect")
    fine = map_coordinates(gaussian_filter(rng.standard_normal((cells * 4, cells * 4)), 1.5), [nu * 4, nv * 4], order=1, mode="reflect")

    land_edge = edge_s > 1.5
    grow = smoothstep(0.0, 2500.0, d)
    fine_n = fine / max(fine.std(), 1e-6)
    hills = UPLAND + 38.0 * broad + 6.0 * fine_n
    land = base + (hills - base) * grow
    sea = np.minimum(base, -4.0) - 0.004 * d
    # Land or sea past the map starts from the edge's own profile, then broad noise bends the coast
    # as it goes out, so no coastline runs ruler-straight to the horizon. South of the map is open
    # Atlantic, so headlands that run off the south edge fall away within a few hundred metres.
    # Warp where each point reads the edge in proportion to its distance, so a coastline that meets
    # the map edge turns and wanders as it leaves instead of extruding straight outwards.
    warp = 1.0 * d
    shore = map_coordinates(noise, [nv, nu], order=3, mode="reflect")   # an independent field
    wx = np.clip(X + warp * (shore + 0.4 * fine_n), -H, H)
    wy = np.clip(Y + warp * (broad - 0.4 * fine_n), -H, H)
    edge_w = map_coordinates(hs, [wy + H, wx + H], order=1, mode="nearest")
    score = edge_w - 1.5 + 60.0 * smoothstep(200.0, 3000.0, d) * (broad + 0.6 * fine_n)
    score -= 0.6 * np.maximum(0.0, -X - H + 500.0 * shore)
    # The land comes down to the water over ~150 m, a steep coast rather than a sheer wall.
    # Only sea joined to the open water in the south stays sea; inland hollows would show the ocean
    # sheet as flat lakes, so they stay land.
    wet, _ = label(score <= 0.0)
    open_sea = np.setdiff1d(np.unique(wet[0, :]), [0])
    near_sea = binary_dilation(np.isin(wet, open_sea), iterations=4)
    score = np.where(near_sea, score, np.maximum(score, 22.0))
    z = sea + (land - sea) * smoothstep(0.0, 22.0, score)
    # Right at the seam keep the edge's own land/sea split, so the ring meets the landscape cleanly.
    z = np.where(land_edge, land, sea) * np.exp(-d / 100.0) + z * (1.0 - np.exp(-d / 100.0))
    # Hug the landscape at the seam and fade the far rim down behind the haze.
    z = np.where(d < 1.0, edge - SINK, z)
    z -= 40.0 * smoothstep(0.6 * FAR, FAR, d)
    return xs, ys, z


def main():
    os.makedirs(OUT, exist_ok=True)
    xs, ys, z = ring()
    X, Y = np.meshgrid(xs, ys, indexing="ij")
    inside = (np.abs(X) < H - OVERLAP - 0.01) & (np.abs(Y) < H - OVERLAP - 0.01)
    nx, ny = X.shape
    cell = np.ones((nx - 1, ny - 1), bool)
    cell &= ~(inside[:-1, :-1] & inside[1:, :-1] & inside[:-1, 1:] & inside[1:, 1:])
    # Drop cells that are entirely under the sea surface: the ocean mesh covers them.
    under = z < -2.0
    cell &= ~(under[:-1, :-1] & under[1:, :-1] & under[:-1, 1:] & under[1:, 1:])
    used = np.zeros((nx, ny), bool)
    for a, b in ((0, 0), (1, 0), (0, 1), (1, 1)):
        used[a:nx - 1 + a, b:ny - 1 + b] |= cell
    index = -np.ones((nx, ny), np.int64)
    index[used] = np.arange(used.sum())
    path = os.path.join(OUT, "SM_EstateOuterLand.obj")
    with open(path, "w") as f:
        f.write("# Estate outer land ring, generated by Scripts/Terrain/bake_outer_land.py\n")
        for i, j in zip(*np.nonzero(used)):
            f.write(f"v {X[i, j] * 100:.1f} {-Y[i, j] * 100:.1f} {z[i, j] * 100:.1f}\n")
        for i, j in zip(*np.nonzero(used)):
            f.write(f"vt {Y[i, j] / 50.0:.4f} {X[i, j] / 50.0:.4f}\n")
        for i, j in zip(*np.nonzero(cell)):
            a, b, c, e = index[i, j] + 1, index[i + 1, j] + 1, index[i + 1, j + 1] + 1, index[i, j + 1] + 1
            # The y flip mirrors the winding (see bake_ocean.py).
            f.write(f"f {a}/{a} {c}/{c} {b}/{b}\nf {a}/{a} {e}/{e} {c}/{c}\n")
    print(f"{path}: {used.sum()} vertices, {2 * cell.sum()} triangles, z {z[used].min():.0f}..{z[used].max():.0f} m")


if __name__ == "__main__":
    main()
