"""Bake the Estate ocean's inputs from the runtime heightfield (no editor needed).

Outputs:
  Assets/Environment/Ocean/T_OceanRipples_N.png   512^2 tileable capillary/short-wave normal map (FFT)
  Assets/Environment/Ocean/T_OceanFoam.png        512^2 tileable foam pattern (cellular + fBm)
  Saved/Ocean/T_EstateOceanShore.png              shore data over the sea's bounding box:
                                                    R = sqrt(depth / 32 m), G = sqrt(shore distance / 512 m),
                                                    B = exposure to the open sea (0 sheltered .. 1 open)
  Saved/Ocean/SM_EstateOcean.obj                  the ocean surface mesh (tensor grid: 8 m inside the map
                                                    where the sea can reach, growing to ~60 km outside)
  Saved/Ocean/ocean_bake.json                     the shore texture's world frame, for the material

The heightfield is Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16 (row = +Y east,
column = +X north, 1 m per sample, value = 32768 + metres * 128). Run from the repo root:
  python Scripts/Terrain/bake_ocean.py
"""
import json
import os

import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt, gaussian_filter, label, minimum_filter

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
SIZE, H = 4033, 2016
ASSETS = os.path.join(REPO, "Assets", "Environment", "Ocean")
SAVED = os.path.join(REPO, "Saved", "Ocean")

GRID_STEP = 8.0        # metres between ocean vertices inside the map
REACH_HEIGHT = 1.2     # cells whose lowest ground is below this can hold (lapping) water
FAR_EDGE = 60000.0     # metres: the skirt reaches the horizon haze


def load_heights():
    raw = np.fromfile(os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16"), "<u2")
    return (raw.reshape(SIZE, SIZE).astype(np.float32) - 32768.0) / 128.0   # h[yi, xi]


def connected_to_south(mask):
    lab, _ = label(mask)
    keep = np.unique(lab[:, 0])            # column 0 is x = -2016 m, the southern (Atlantic) edge
    keep = keep[keep > 0]
    return np.isin(lab, keep)


def shore_texture(h):
    sea = connected_to_south(h < 0.0)
    depth = np.where(sea, -h, 0.0)
    dist = distance_transform_edt(sea)                     # metres to the nearest dry sample
    dist = gaussian_filter(dist, 3.0) * sea                 # smooth crest lines for the shore waves
    exposure = gaussian_filter(sea.astype(np.float32), 140.0)
    exposure = np.clip((exposure - 0.35) / 0.45, 0.0, 1.0)

    ys, xs = np.nonzero(sea)                                 # rows = y index, cols = x index
    margin = 64
    x0, x1 = 0, min(SIZE - 1, xs.max() + margin)             # south edge .. northernmost water
    y0, y1 = max(0, ys.min() - margin), min(SIZE - 1, ys.max() + margin)
    # The texture: u runs east (+Y), v runs south from its northern edge (image top = north).
    width, height = 2048, 1024
    sx = x1 - x0
    sy = y1 - y0
    u = np.linspace(y0, y1, width)
    v = np.linspace(x1, x0, height)
    uu, vv = np.meshgrid(u, v)
    from scipy.ndimage import map_coordinates

    def resample(a):
        return map_coordinates(a, [uu, vv], order=1, mode="nearest")

    r = np.sqrt(np.clip(resample(depth) / 32.0, 0, 1))
    g = np.sqrt(np.clip(resample(dist) / 512.0, 0, 1))
    b = np.clip(resample(exposure), 0, 1)
    img = np.stack([r, g, b, np.ones_like(r)], -1)
    Image.fromarray(np.round(img * 255).astype(np.uint8), "RGBA").save(os.path.join(SAVED, "T_EstateOceanShore.png"))
    frame = {
        # World frame in metres: u = (Y - ShoreY0) / ShoreSizeY, v = (ShoreX1 - X) / ShoreSizeX.
        "ShoreY0": float(y0 - H), "ShoreSizeY": float(sy),
        "ShoreX1": float(x1 - H), "ShoreSizeX": float(sx),
        "maxDepth": float(depth.max()),
    }
    return sea, frame


def axis(lo_inner, hi_inner, extend_lo, extend_hi):
    inner = np.arange(lo_inner, hi_inner + 0.01, GRID_STEP)
    def grow(start, sign):
        out, step, p = [], GRID_STEP, start
        while abs(p - start) < FAR_EDGE:
            step *= 1.28
            p += sign * step
            out.append(p)
        return out
    lo = grow(lo_inner, -1)[::-1] if extend_lo else []
    hi = grow(hi_inner, +1) if extend_hi else []
    return np.concatenate([lo, inner, hi])


def ocean_mesh(h, sea):
    low = connected_to_south(h < REACH_HEIGHT)
    # Lowest ground in every 8 m block, dilated by a block so the surface overlaps its shoreline.
    block_low = minimum_filter(np.where(low, h, 99.0), size=int(GRID_STEP * 2 + 1))
    reach = block_low < REACH_HEIGHT
    xs = axis(-H, H, True, True)       # sea on every side, so land at the map edge never shows void behind it
    ys = axis(-H, H, True, True)
    nx, ny = len(xs), len(ys)
    cell = np.zeros((nx - 1, ny - 1), bool)
    for i in range(nx - 1):
        xc = 0.5 * (xs[i] + xs[i + 1])
        xi = int(round(np.clip(xc, -H, H) + H))
        for j in range(ny - 1):
            yc = 0.5 * (ys[j] + ys[j + 1])
            yi = int(round(np.clip(yc, -H, H) + H))
            inside = -H <= xc <= H and -H <= yc <= H
            # South of the map it is open Atlantic everywhere, including off headlands that run to
            # the map edge; east and west of the map only where the edge itself is sea.
            cell[i, j] = reach[yi, xi] if inside else True
    used = np.zeros((nx, ny), bool)
    used[:-1, :-1] |= cell
    used[1:, :-1] |= cell
    used[:-1, 1:] |= cell
    used[1:, 1:] |= cell
    index = -np.ones((nx, ny), np.int64)
    index[used] = np.arange(used.sum())
    verts = [(xs[i], ys[j]) for i, j in zip(*np.nonzero(used))]
    faces = []
    for i, j in zip(*np.nonzero(cell)):
        a, b, c, d = index[i, j], index[i + 1, j], index[i + 1, j + 1], index[i, j + 1]
        faces.append((a, b, c))
        faces.append((a, c, d))
    return np.array(verts), np.array(faces)


def write_obj(path, verts, faces):
    # Interchange's OBJ reader maps OBJ (x, y, z) to Unreal (x, -y, z) (checked by the imported
    # bounds), so the mesh is written in world centimetres with y negated.
    with open(path, "w") as f:
        f.write("# Estate ocean surface, generated by Scripts/Terrain/bake_ocean.py\n")
        for x, y in verts:
            f.write(f"v {x * 100:.1f} {-y * 100:.1f} 0\n")
        for x, y in verts:
            f.write(f"vt {y / 64.0:.5f} {x / 64.0:.5f}\n")
        f.write("vn 0 0 1\n")
        for a, b, c in faces + 1:
            f.write(f"f {a}/{a}/1 {c}/{c}/1 {b}/{b}/1\n")   # the y flip mirrors the winding


def tileable_ripples(n=512, seed=7):
    """Short gravity-capillary waves on a torus: FFT of a band-limited, mildly directional spectrum."""
    rng = np.random.default_rng(seed)
    k = np.fft.fftfreq(n) * n
    kx, ky = np.meshgrid(k, k)
    kk = np.hypot(kx, ky)
    kk[0, 0] = 1
    spectrum = kk ** -4.0 * np.exp(-(kk / 64.0) ** 2) * (1 - np.exp(-(kk / 4.0) ** 2))
    spectrum *= 0.25 + 0.75 * (kx / kk) ** 4                 # crests mostly across +U
    spectrum[0, 0] = 0
    phase = rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))
    hk = phase * np.sqrt(spectrum)
    gx = np.real(np.fft.ifft2(1j * kx * hk))
    gy = np.real(np.fft.ifft2(1j * ky * hk))
    scale = 0.55 / np.sqrt(np.mean(gx ** 2 + gy ** 2))       # RMS slope ~0.55 at full strength
    nrm = np.stack([-gx * scale, -gy * scale, np.ones_like(gx)], -1)
    nrm /= np.linalg.norm(nrm, axis=-1, keepdims=True)
    img = np.round((nrm * 0.5 + 0.5) * 255).astype(np.uint8)
    Image.fromarray(img, "RGB").save(os.path.join(ASSETS, "T_OceanRipples_N.png"))


def tileable_foam(n=512, seed=11):
    """Lacy sea foam: the ridges between wrapped Worley cells, broken up by fBm."""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:n, 0:n] / n
    def worley(count):
        pts = rng.random((count, 2))
        d = np.full((n, n), 9.0)
        d2 = np.full((n, n), 9.0)
        for px, py in pts:
            dx = np.abs(xx - px); dx = np.minimum(dx, 1 - dx)
            dy = np.abs(yy - py); dy = np.minimum(dy, 1 - dy)
            dd = np.hypot(dx, dy)
            d2 = np.where(dd < d, d, np.minimum(d2, dd))
            d = np.minimum(d, dd)
        return (d2 - d) * np.sqrt(count)
    def fbm(octaves=5):
        out = np.zeros((n, n))
        for o in range(octaves):
            f = 4 * 2 ** o
            noise = rng.standard_normal((f, f))
            big = np.fft.ifft2(np.fft.fft2(np.kron(noise, np.ones((n // f, n // f)))) *
                               np.exp(-(np.hypot(*np.meshgrid(np.fft.fftfreq(n), np.fft.fftfreq(n))) * n / f) ** 2 * 2))
            out += np.real(big) / 2 ** o
        return (out - out.min()) / (out.max() - out.min())
    lace = 1 - np.clip(worley(90) * 1.6, 0, 1)
    fine = 1 - np.clip(worley(420) * 1.4, 0, 1)
    foam = np.clip((0.65 * lace + 0.35 * fine) * (0.35 + 0.9 * fbm()), 0, 1) ** 1.4
    Image.fromarray(np.round(foam * 255).astype(np.uint8), "L").save(os.path.join(ASSETS, "T_OceanFoam.png"))


def wave_volume(n=128, frames=64, tile=48.0, period=16.0, wind=5.5, seed=23):
    """A time-looping patch of wind sea: slopes of a Phillips-spectrum FFT ocean, one tile per frame.

    Every angular frequency is rounded to a multiple of 2*pi/period, so frame `frames` equals frame 0
    and the patch loops seamlessly in time as well as space. Written as an 8x8 atlas of 128^2 frames
    (row-major, frame 0 top-left) for Unreal's volume texture:
      R, G = slope along +U (downwind) and +V, scaled so that (2*c - 1) has an RMS of 0.25
      B    = whitecap potential from the folding (Jacobian) of the choppy surface, 0..1
      A    = height, scaled like the slopes
    """
    g = 9.81
    rng = np.random.default_rng(seed)
    k1 = 2 * np.pi * np.fft.fftfreq(n, d=tile / n)
    kx, ky = np.meshgrid(k1, k1)                      # kx along U (columns), ky along V (rows)
    k = np.hypot(kx, ky)
    k[0, 0] = 1e-6
    lw = wind * wind / g
    cosw = kx / k
    phillips = np.exp(-1.0 / (k * lw) ** 2) / k ** 4 * np.abs(cosw) ** 2
    phillips *= np.where(cosw < 0, 0.07, 1.0)             # little energy running against the wind
    phillips *= np.exp(-(k * 0.08) ** 2)                  # nothing shorter than ~0.5 m
    phillips[0, 0] = 0
    h0 = (rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))) * np.sqrt(phillips / 2)
    h0m = np.conj(h0[(-np.arange(n)) % n][:, (-np.arange(n)) % n])
    hk0 = h0 + h0m                                        # a physical slope RMS for the Jacobian
    s0 = np.sqrt(np.mean(np.real(np.fft.ifft2(1j * kx * hk0)) ** 2 + np.real(np.fft.ifft2(1j * ky * hk0)) ** 2))
    h0 *= 0.13 / s0
    h0m *= 0.13 / s0
    w0 = 2 * np.pi / period
    omega = np.round(np.sqrt(g * k) / w0) * w0
    ik = 1j / k
    slopes, caps, heights = [], [], []
    for f in range(frames):
        t = period * f / frames
        hk = h0 * np.exp(1j * omega * t) + h0m * np.exp(-1j * omega * t)
        sx = np.real(np.fft.ifft2(1j * kx * hk))
        sy = np.real(np.fft.ifft2(1j * ky * hk))
        # Choppy displacement D = -i k/|k| h; its Jacobian goes below zero where crests fold.
        dxx = np.real(np.fft.ifft2(kx * kx / k * hk))
        dyy = np.real(np.fft.ifft2(ky * ky / k * hk))
        dxy = np.real(np.fft.ifft2(kx * ky / k * hk))
        slopes.append((sx, sy))
        caps.append((dxx, dyy, dxy))
        heights.append(np.real(np.fft.ifft2(hk)))
    rms = np.sqrt(np.mean([np.mean(sx ** 2 + sy ** 2) for sx, sy in slopes]))
    hrms = np.sqrt(np.mean([np.mean(h ** 2) for h in heights]))
    dscale = 0.25 / rms                                  # decoded slope RMS 0.25 (clipped at 4 sigma)
    jac = [(1 - dxx * 0.9) * (1 - dyy * 0.9) - (dxy * 0.9) ** 2 for dxx, dyy, dxy in caps]
    allj = np.concatenate([j.ravel() for j in jac])
    jthr, jlo = np.percentile(allj, 5.0), np.percentile(allj, 0.3)   # the most folded 5% can whitecap
    side = int(np.ceil(np.sqrt(frames)))
    atlas = np.zeros((side * n, side * n, 4), np.float32)
    for f in range(frames):
        sx, sy = slopes[f]
        cap = np.clip((jthr - jac[f]) / (jthr - jlo), 0, 1) ** 1.5
        tile_img = np.stack([sx * dscale * 0.5 + 0.5, sy * dscale * 0.5 + 0.5, cap,
                             heights[f] / hrms * 0.125 + 0.5], -1)
        r, c = divmod(f, side)
        atlas[r * n:(r + 1) * n, c * n:(c + 1) * n] = tile_img
    img = np.round(np.clip(atlas, 0, 1) * 255).astype(np.uint8)
    Image.fromarray(img, "RGBA").save(os.path.join(ASSETS, "T_OceanWaves.png"))
    meta = {"tile_m": tile, "period_s": period, "frames": frames, "size": n, "wind_ms": wind,
            "slope_rms": float(rms), "height_rms_m": float(hrms)}
    json.dump(meta, open(os.path.join(SAVED, "ocean_waves.json"), "w"), indent=2)
    print("wave volume", meta)


def main():
    os.makedirs(ASSETS, exist_ok=True)
    os.makedirs(SAVED, exist_ok=True)
    import sys
    if sys.argv[1:] == ["waves"]:
        wave_volume()
        return
    h = load_heights()
    sea, frame = shore_texture(h)
    verts, faces = ocean_mesh(h, sea)
    write_obj(os.path.join(SAVED, "SM_EstateOcean.obj"), verts, faces)
    frame.update(vertices=int(len(verts)), triangles=int(len(faces)))
    json.dump(frame, open(os.path.join(SAVED, "ocean_bake.json"), "w"), indent=2)
    tileable_ripples()
    tileable_foam()
    wave_volume()
    print(json.dumps(frame, indent=2))


if __name__ == "__main__":
    main()
