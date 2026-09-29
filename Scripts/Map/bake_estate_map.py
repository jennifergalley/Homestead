"""Bake the stylised estate map (T_EstateMap source) from the terrain heightmap and layout.

The map is an original parchment-and-ink cartographic rendering: a soft north-west hillshade on
parchment, faint 10 m contours, a muted sea with an inked, water-lined coast, the river inked in,
and the road lightened with dark edges. Woods come from the runtime scatter
(``Content/SurvivalGame/Estate/Runtime/EstateScenery.bin``): each tree and hazel is stamped as an
inked crown over a pale woodland wash. When an orthographic capture of the Estate level is
available (``--capture``), its colours are folded in gently too.

Output (repo-relative by default):
  Assets/Map/T_EstateMap.png   4096x4096 RGB, north up (u = east, v = south)
  Assets/Map/T_EstateMap.json  world rectangle the image covers, in Unreal cm

Run from the repo root:  python Scripts\\Map\\bake_estate_map.py
Then import in the editor with the console command  Homestead.ImportEstateMap
(see docs\\setup.md, "Estate map").
"""

import argparse
import datetime
import json
import os

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HALF_M = 2016.0  # The 4033-vertex landscape spans -2016..2016 m on both axes.


def load_heights(path):
    raw = np.asarray(Image.open(path), dtype=np.float64)
    heights = (raw - 32768.0) / 128.0
    # PNG rows are +Y (east) and columns +X (north); the map wants rows north->south, columns
    # west->east.
    return np.flipud(heights.T)


def world_to_px(x_m, y_m, size):
    col = (y_m + HALF_M) / (2 * HALF_M) * size
    row = (HALF_M - x_m) / (2 * HALF_M) * size
    return col, row


def line_mask(points, size, width_px, oversample=2):
    """An anti-aliased mask of a polyline of [x, y] metre points."""
    big = size * oversample
    image = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(image)
    path = [world_to_px(p[0], p[1], big) for p in points]
    if len(path) >= 2:
        draw.line(path, fill=255, width=max(1, int(round(width_px * oversample))), joint="curve")
        radius = width_px * oversample / 2.0
        for x, y in (path[0], path[-1]):
            draw.ellipse([x - radius, y - radius, x + radius, y + radius], fill=255)
    return np.asarray(image.resize((size, size), Image.LANCZOS), dtype=np.float64) / 255.0


SCENERY_TREES = {0: 3.6, 1: 2.8, 13: 8.3, 14: 7.2, 15: 7.4, 16: 2.5}  # EstateSceneryKinds index -> canopy radius in metres at scale 1
SCENERY_HAZEL = 2


def load_scenery(path):
    """The runtime scatter (HSC1: u32 count, then 20-byte records u8 kind, 3 pad, float X, Y, Yaw,
    Scale in world cm), as baked by Scripts/Terrain/scatter.py. The game draws these as HISMs at
    runtime, so an editor-world capture never sees them."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:4] != b"HSC1":
        raise ValueError(f"{path} is not an HSC1 scenery file")
    count = int(np.frombuffer(data, "<u4", 1, 4)[0])
    record = np.dtype([("kind", "u1"), ("pad", "u1", 3), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("scale", "<f4")])
    return np.frombuffer(data, record, count, 8)


def woods_layers(scenery, size, oversample=2):
    """A soft woodland wash (from local tree and hazel density) and inked tree stamps."""
    metres_per_px = 2 * HALF_M / size
    density = np.zeros((size, size))
    woody = scenery[np.isin(scenery["kind"], list(SCENERY_TREES) + [SCENERY_HAZEL])]
    col, row = world_to_px(woody["x"] / 100.0, woody["y"] / 100.0, size)
    inside = (col >= 0) & (col < size) & (row >= 0) & (row < size)
    weight = np.where(woody["kind"] == SCENERY_HAZEL, 0.6, 1.0)
    np.add.at(density, (row[inside].astype(int), col[inside].astype(int)), weight[inside])
    # Trees per 100 m^2 over a ~12 m neighbourhood; a wood reads from about one tree per 150 m^2.
    density = ndimage.gaussian_filter(density, 12.0 / metres_per_px) * 100.0 / (metres_per_px ** 2)
    wash = np.clip((density - 0.35) / 0.5, 0.0, 1.0)
    wash = ndimage.gaussian_filter(wash, 1.5)

    big = size * oversample
    canopy = Image.new("L", (big, big), 0)
    shadow = Image.new("L", (big, big), 0)
    outline = Image.new("L", (big, big), 0)
    draw_canopy, draw_shadow, draw_outline = ImageDraw.Draw(canopy), ImageDraw.Draw(shadow), ImageDraw.Draw(outline)
    order = np.argsort(-woody["x"])  # North first, so southern crowns overlap the ones behind them.
    for tree in woody[order]:
        kind = int(tree["kind"])
        radius_m = SCENERY_TREES.get(kind, 1.8) * float(np.clip(tree["scale"], 0.6, 1.6))
        x, y = world_to_px(tree["x"] / 100.0, tree["y"] / 100.0, big)
        r = max(1.2, radius_m / metres_per_px * oversample)
        s = r * 0.35  # Shadow falls to the south-east, away from the north-west light.
        draw_shadow.ellipse([x - r + s, y - r + s, x + r + s, y + r + s], fill=255)
        draw_outline.ellipse([x - r - 1, y - r - 1, x + r + 1, y + r + 1], fill=255)
        draw_canopy.ellipse([x - r, y - r, x + r, y + r], fill=255 if kind in SCENERY_TREES else 150)

    def down(image):
        return np.asarray(image.resize((size, size), Image.LANCZOS), dtype=np.float64) / 255.0

    return wash, down(canopy), down(shadow), down(outline)


def blend(base, color, alpha):
    alpha = np.clip(alpha, 0.0, 1.0)[..., None]
    return base * (1.0 - alpha) + np.asarray(color, dtype=np.float64) * alpha


def bake(heights, layout, size, capture=None, scenery=None, seed=7):
    metres_per_px = 2 * HALF_M / size
    zoom = size / heights.shape[0]
    h = ndimage.zoom(heights, zoom, order=1)[:size, :size]
    h = ndimage.gaussian_filter(h, 0.6)

    # Hillshade, lit from the north-west as maps conventionally are.
    dy, dx = np.gradient(h, metres_per_px)  # dy: down the rows (south), dx: along columns (east)
    exaggeration = 2.2
    nx, ny, nz = -dx * exaggeration, -dy * exaggeration, np.ones_like(h)
    norm = np.sqrt(nx * nx + ny * ny + nz * nz)
    light = np.array([-1.0, -1.0, 1.4])
    light /= np.linalg.norm(light)
    shade = np.clip((nx * light[0] + ny * light[1] + nz * light[2]) / norm, 0.0, 1.0)

    water = ndimage.binary_opening(h < 0.0, iterations=1)
    land = ~water

    rng = np.random.default_rng(seed)
    grain = ndimage.gaussian_filter(rng.normal(0.0, 1.0, (size, size)), 1.2)
    blotch = ndimage.gaussian_filter(rng.normal(0.0, 1.0, (size // 8, size // 8)), 3.0)
    blotch = ndimage.zoom(blotch, 8, order=1)[:size, :size]
    paper = 1.0 + grain * 0.012 + blotch * 0.05

    # Land: parchment, a touch warmer in the valleys and cooler on the high moor.
    elevation = np.clip(h / 190.0, 0.0, 1.0)[..., None]
    low = np.array([0.90, 0.81, 0.60])
    high = np.array([0.80, 0.77, 0.62])
    tint = low * (1 - elevation) + high * elevation
    relief = (0.72 + 0.40 * shade)[..., None]
    rgb = tint * relief * paper[..., None]

    if capture is not None:
        # Fold the capture's hue in gently (woods darker and greener, fields lighter), keeping
        # the parchment palette dominant so the map stays a map rather than a photograph.
        cap = np.asarray(capture.convert("RGB").resize((size, size), Image.LANCZOS), dtype=np.float64) / 255.0
        luminance = cap.mean(axis=2, keepdims=True)
        greenness = np.clip((cap[..., 1:2] - np.maximum(cap[..., 0:1], cap[..., 2:3])) * 4.0, 0.0, 1.0)
        rgb = rgb * (0.85 + 0.3 * luminance)
        rgb = blend(rgb, [0.55, 0.60, 0.40], greenness[..., 0] * 0.35 * land)

    # Contours: faint every 10 m, firmer every 50 m.
    for step, strength in ((10.0, 0.10), (50.0, 0.20)):
        bands = np.floor(h / step)
        edge = (bands != np.roll(bands, 1, axis=0)) | (bands != np.roll(bands, 1, axis=1))
        edge &= land & (h > 1.0)
        rgb = blend(rgb, [0.45, 0.33, 0.20], edge.astype(np.float64) * strength)

    if scenery is not None:
        # Woods: a pale green wash where trees stand together, then each crown stamped in ink with
        # a small south-east shadow, the way hand-drawn estate plans mark trees.
        wash, canopy, shadow, outline = woods_layers(scenery, size)
        rgb = blend(rgb, [0.66, 0.69, 0.47], wash * 0.5 * land)
        rgb = blend(rgb, [0.36, 0.31, 0.21], np.clip(shadow - canopy, 0, 1) * 0.35 * land)
        crown = np.array([0.50, 0.57, 0.35]) * (0.9 + 0.12 * shade)[..., None] * paper[..., None]
        rgb = rgb * (1 - (canopy * 0.88 * land)[..., None]) + crown * (canopy * 0.88 * land)[..., None]
        rgb = blend(rgb, [0.22, 0.21, 0.13], np.clip(outline - canopy, 0, 1) * 0.75 * land)

    # Sea: muted blue-green, deepening offshore, with water-lining parallel to the coast.
    depth = np.clip(-h / 22.0, 0.0, 1.0)
    shallow = np.array([0.66, 0.74, 0.70])
    deep = np.array([0.46, 0.57, 0.58])
    sea = (shallow * (1 - depth[..., None]) + deep * depth[..., None]) * paper[..., None]
    rgb = np.where(water[..., None], sea, rgb)
    from_coast = ndimage.distance_transform_edt(water) * metres_per_px
    for distance, strength in ((9.0, 0.35), (20.0, 0.22), (34.0, 0.12)):
        ring = np.abs(from_coast - distance) < metres_per_px * 0.7
        rgb = blend(rgb, [0.30, 0.38, 0.40], ring * strength)
    coast = water & ~ndimage.binary_erosion(water, iterations=2)
    coast = ndimage.gaussian_filter(coast.astype(np.float64), 0.7)
    rgb = blend(rgb, [0.20, 0.15, 0.11], np.clip(coast * 1.6, 0, 1) * 0.9)

    # River: inked blue over the valley floor.
    river = layout.get("river", [])
    if river:
        rgb = blend(rgb, [0.20, 0.17, 0.12], line_mask(river, size, 4.0 / metres_per_px + 1.6) * 0.8)
        rgb = blend(rgb, [0.42, 0.58, 0.62], line_mask(river, size, 4.0 / metres_per_px) * 0.95)

    # Road: lightened with dark edges, like a surveyed carriage road.
    road = layout.get("road", [])
    if road:
        rgb = blend(rgb, [0.28, 0.20, 0.13], line_mask(road, size, 7.0 / metres_per_px + 2.0) * 0.75)
        rgb = blend(rgb, [0.97, 0.93, 0.81], line_mask(road, size, 7.0 / metres_per_px) * 0.95)

    # Darken the far edges of the sheet slightly, like an old estate plan.
    yy, xx = np.mgrid[0:size, 0:size] / (size - 1.0) * 2.0 - 1.0
    edge = np.clip((np.maximum(np.abs(xx), np.abs(yy)) - 0.86) / 0.14, 0.0, 1.0)
    rgb *= (1.0 - 0.18 * edge * edge)[..., None]

    return (np.clip(rgb, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--heightmap", default=os.path.join(ROOT, "Scripts", "Terrain", "Estate_Heightmap_4033.png"))
    parser.add_argument("--layout", default=os.path.join(ROOT, "Scripts", "Terrain", "estate_layout.json"))
    parser.add_argument("--capture", help="Optional orthographic capture of the Estate level, north up.")
    parser.add_argument("--scenery", default=os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin"),
                        help="Runtime scatter (HSC1) whose trees and hazel are stamped as woods; pass '' to skip.")
    parser.add_argument("--size", type=int, default=4096)
    parser.add_argument("--out", default=os.path.join(ROOT, "Assets", "Map", "T_EstateMap.png"))
    args = parser.parse_args()

    with open(args.layout, "r", encoding="utf-8") as handle:
        layout = json.load(handle)
    heights = load_heights(args.heightmap)
    capture = Image.open(args.capture) if args.capture else None
    scenery = load_scenery(args.scenery) if args.scenery and os.path.exists(args.scenery) else None
    pixels = bake(heights, layout, args.size, capture, scenery)
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    Image.fromarray(pixels, "RGB").save(args.out, optimize=True)
    info = {
        "minX": -HALF_M * 100.0,
        "minY": -HALF_M * 100.0,
        "sizeX": HALF_M * 200.0,
        "sizeY": HALF_M * 200.0,
        "source": ("Estate capture" if capture else "Estate heightmap") + (" + scenery" if scenery is not None else ""),
        "bakedAt": datetime.datetime.now().isoformat(timespec="seconds"),
    }
    with open(os.path.splitext(args.out)[0] + ".json", "w", encoding="utf-8") as handle:
        json.dump(info, handle, indent=1)
    print(f"Wrote {args.out} ({args.size}x{args.size}) from {info['source']}.")


if __name__ == "__main__":
    main()
