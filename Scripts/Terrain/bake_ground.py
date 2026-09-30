"""Bake the Estate map's ground data: where grass grows and how, what she walks on, and the grass
blade patch meshes the near-camera meadow is built from. No editor needed.

Outputs:
  Saved/Ground/T_EstateGround.png            2048^2 RGBA over the map, for M_EstateGrass and
                                             M_EstateLandscape: R = grass density, G = grass height,
                                             B = dryness (straw), A = wear (bare, trodden soil)
  Saved/Ground/T_EstateCanopy.png            2048^2 RGBA: R = tree canopy cover (leaf litter and moss
                                             underfoot), G = stony soil on steep banks, B = the MVP
                                             woodland zone (mvp_woodland.json)
  Content/SurvivalGame/Estate/Runtime/EstateGround.bin
                                             1024^2 cells for the game (HomesteadEstateGround):
                                             "HGD1", u16 size, then per cell u8 grass density (max over
                                             the cell, 0..255) and u8 surface (see SURFACES)
  Assets/Environment/Ground/T_GrassWind.png  256^2 tileable gust noise
  Saved/Ground/SM_GrassPatch_LOD{0,1,2}.obj  a 2 x 2 m patch of grass blades at three densities

Texture and cell mapping (world cm, +X north, +Y east), the same as T_EstateRoadSDF:
u = (X + 201600) / 403200 along columns, v = (Y + 201600) / 403200 down rows.

Blade meshes: UV0 holds each blade's root in the patch, (x + 100) / 200 and (y + 100) / 200 from its
local cm, so the material can find the root, sample the ground there and bend the blade about it.
Normals lean towards up so the blades light like a lawn rather than a set of flat cards.

Inputs: Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16 and EstateScenery.bin (its trees make
the canopy mask, at the crown radii in SCENERY_TREES of Scripts/Map/bake_estate_map.py, so re-run this
after scatter.py or a new tree kind), Scripts/Terrain/estate_layout.json
and the paint-layer weights in HOMESTEAD_TERRAIN_WORK/weights (default E:\\TerrainSource\\work).
Run from the repo root:
  python Scripts/Terrain/bake_ground.py
"""
import json
import os
import struct

import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt, gaussian_filter, map_coordinates, maximum_filter, zoom
from skimage.draw import polygon as fill_polygon

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
SIZE, H = 4033, 2016
TEX = 2048
CELLS = 1024
LAYERS = ["Pasture", "WoodlandFloor", "Moorland", "DuneSand", "Beach", "CliffRock", "DirtRoad"]
# Surface bytes in EstateGround.bin; HomesteadEstateGround.h mirrors them.
# The derelict farm's fenced field (manor lane's runtime actor), metres: x0, x1, y0, y1.
FARM = (-222.0, -162.0, -705.0, -645.0)
SURFACES = {"Soil": 0, "Grass": 1, "Road": 2, "Sand": 3, "Rock": 4, "Woodland": 5, "Moor": 6, "Water": 7}


def densify(p, step):
    p = np.asarray(p, np.float64)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def fbm(shape, octaves, base_cells, seed):
    """Smooth band-limited noise in [0, 1] over a grid, base_cells features across it."""
    rng = np.random.default_rng(seed)
    out = np.zeros(shape, np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cells = base_cells * 2 ** o
        n = gaussian_filter(rng.standard_normal((cells, cells)), 0.8)
        big = zoom(n, (shape[0] / cells, shape[1] / cells), order=3)[: shape[0], : shape[1]]
        big = (big - big.mean()) / (big.std() + 1e-9)
        out += amp * big.astype(np.float32)
        total += amp
        amp *= 0.5
    return np.clip(out / total * 0.33 + 0.5, 0, 1)


def load():
    raw = np.fromfile(os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16"), "<u2")
    h = (raw.reshape(SIZE, SIZE).astype(np.float32) - 32768.0) / 128.0            # h[yi, xi]
    w = {}
    for n in LAYERS:
        a = np.asarray(Image.open(os.path.join(WORK, "weights", f"{n}.png"))).astype(np.float32) / 255
        # scatter.py reads a.T[::-1, :] as [row = 2016 - x, col = 2016 + y]; flip to the
        # heightfield's [yi = 2016 + y, xi = 2016 + x].
        w[n] = a.T[::-1, :][::-1, :].T.copy()
    return h, w


def line_distance(points, shape):
    """Metres from each 1 m cell to a polyline (rasterised at 0.5 m)."""
    p = densify(points, 0.5)
    mask = np.ones(shape, bool)
    xi = np.clip(np.round(p[:, 0] + H).astype(int), 0, shape[1] - 1)
    yi = np.clip(np.round(p[:, 1] + H).astype(int), 0, shape[0] - 1)
    mask[yi, xi] = False
    return distance_transform_edt(mask).astype(np.float32)


# Crown radius (m, scale 1) for tree kinds that bake_estate_map.py's SCENERY_TREES doesn't list yet;
# SCENERY_TREES overrides these. 13-16 are the trees lane's oak, beech, sycamore and hawthorn; 19-22
# the woodland-biome lane's jacaranda, fir pole and two fir saplings. Shrub kinds (holly, hazel
# coppice) aren't canopy.
CROWN_FALLBACK = {0: 3.6, 1: 2.8, 13: 8.3, 14: 7.2, 15: 7.4, 16: 2.5,
                  19: 4.0, 20: 2.2, 21: 1.0, 22: 1.2}


def crown_radii():
    """EstateSceneryKinds index -> crown radius (m) at scale 1, shared with the estate map bake
    (SCENERY_TREES in Scripts/Map/bake_estate_map.py), so a tree kind added there shades the ground too."""
    import importlib.util
    path = os.path.join(REPO, "Scripts", "Map", "bake_estate_map.py")
    try:
        spec = importlib.util.spec_from_file_location("bake_estate_map", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return {**CROWN_FALLBACK, **dict(module.SCENERY_TREES)}
    except Exception as error:  # noqa: BLE001 - fall back to the known tree kinds
        print("canopy: could not read SCENERY_TREES from", path, "-", error)
        return dict(CROWN_FALLBACK)


def canopy_mask(shape, scenery=None):
    """0-1 cover of tree canopy on the 1 m grid, from the scenery's tree records (every kind in
    crown_radii(), each at its own crown radius times its scale), with a soft edge of a few metres."""
    path = scenery or os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
    raw = open(path, "rb").read()
    count = struct.unpack_from("<I", raw, 4)[0]
    rec = np.frombuffer(raw, dtype=np.dtype([("k", "u1"), ("pad", "u1", 3), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")]),
                        count=count, offset=8)
    radii = crown_radii()
    trees = rec[np.isin(rec["k"], list(radii))]
    r = np.array([radii[int(k)] for k in trees["k"]], np.float32) * np.clip(trees["s"], 0.6, 1.6)
    xi = np.clip(np.round(trees["x"] / 100.0 + H).astype(int), 0, shape[1] - 1)
    yi = np.clip(np.round(trees["y"] / 100.0 + H).astype(int), 0, shape[0] - 1)
    # Each crown is a Gaussian with sigma 0.8 r, scaled to peak at 1: about half cover at its drip
    # line. Group crowns by radius (0.5 m bins) so each group is one separable blur.
    cover = np.zeros(shape, np.float32)
    bins = np.round(r * 2.0) / 2.0
    for radius in np.unique(bins):
        m = bins == radius
        sigma = max(0.8 * float(radius), 0.8)
        grid = np.zeros(shape, np.float32)
        np.add.at(grid, (yi[m], xi[m]), 1.0)
        cover += gaussian_filter(grid, sigma) * (2 * np.pi * sigma * sigma)
    cover = gaussian_filter(np.clip(cover, 0, 1), 3.0)                 # soften to a leaf-litter edge
    kinds = {int(k): int((trees["k"] == k).sum()) for k in np.unique(trees["k"])}
    print("canopy:", len(trees), "trees", kinds, "-", round(float((cover > 0.5).mean()) * 100, 1), "% of the map under canopy")
    return np.clip(cover * 1.4, 0, 1)


def mvp_woodland_zone(shape):
    """The woodland-biome lane's "MVP woodland" region (Scripts/Terrain/mvp_woodland.json: "polygon"
    [[x, y], ...] in metres, optional "floor_edge_m" (default 15) and "glades" [[x, y, radius], ...]).
    Returns (zone, floor): zone is 1 inside the polygon (no meadow blades, woodland footsteps) and
    floor ramps the MVP forest floor in from 0 at the polygon's edge to 1 floor_edge_m inside, the
    ramp's line wandering with noise; glades get the same floor. None when absent."""
    path = os.path.join(HERE, "mvp_woodland.json")
    if not os.path.exists(path):
        return None
    spec = json.load(open(path))
    poly = np.asarray(spec["polygon"], np.float64)
    ramp = float(spec.get("floor_edge_m", 15.0))
    inside = np.zeros(shape, bool)
    rr, cc = fill_polygon(poly[:, 1] + H, poly[:, 0] + H, shape)      # rows = y, cols = x
    inside[rr, cc] = True
    depth = distance_transform_edt(inside).astype(np.float32)          # metres inside the polygon
    wander = (fbm(shape, 2, 256, 31) - 0.5) * 0.9 * ramp               # the ecotone line meanders
    floor = smoothstep(0.0, ramp, depth + wander) * inside
    zone = smoothstep(0.0, 2.0, depth).astype(np.float32)
    # Glades take the same floor as under the trees, as the prototype's openings did.
    print("MVP woodland zone:", len(poly), "points,", round(float(inside.sum()) / 1e4, 1), "ha, floor ramp", ramp, "m,",
          len(spec.get("glades", [])), "glades")
    return zone, floor.astype(np.float32)


def ground_fields(h, w, layout):
    """Everything at 1 m on the heightfield grid [yi, xi]."""
    coords = (np.arange(SIZE) - H).astype(np.float32)
    X, Y = np.meshgrid(coords, coords)                    # X[yi, xi] = x (north), Y = y (east)
    gy, gx = np.gradient(gaussian_filter(h, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    aspect_south = -gx / (np.hypot(gx, gy) + 1e-6)        # 1 where the ground faces south (-X)

    patch = fbm((SIZE, SIZE), 3, 64, 11)                  # ~60 m patchiness
    clump = fbm((SIZE, SIZE), 2, 640, 12)                 # ~6 m clumps
    dry_n = fbm((SIZE, SIZE), 3, 48, 13)

    density = (w["Pasture"] * 1.0 + w["Moorland"] * 0.62 + w["WoodlandFloor"] * 0.32 + w["DuneSand"] * 0.14
               + w["DirtRoad"] * 0.75)
    density *= 0.78 + 0.22 * patch
    density *= np.where(clump < 0.22, 0.55 + 2.0 * clump, 1.0)
    density *= smoothstep(40.0, 30.0, slope)
    density *= smoothstep(0.5, 1.3, h)

    height = (w["Pasture"] * 0.9 + w["Moorland"] * 0.45 + w["WoodlandFloor"] * 0.6 + w["DuneSand"] * 0.7
              + w["DirtRoad"] * 0.9) / np.maximum(1e-3, w["Pasture"] + w["Moorland"] + w["WoodlandFloor"] + w["DuneSand"] + w["DirtRoad"])
    height *= 0.8 + 0.35 * patch
    dry = (w["Pasture"] * 0.06 + w["Moorland"] * 0.55 + w["WoodlandFloor"] * 0.22 + w["DuneSand"] * 0.8 + w["DirtRoad"] * 0.15)
    dry += 0.22 * smoothstep(0.55, 0.85, dry_n) + 0.12 * smoothstep(12, 30, slope) * np.clip(aspect_south, 0, 1)

    # Wear: trodden soil round the manor, the road's shoulders and the estate's working sites.
    wear = np.zeros_like(h)
    manor = np.array(layout["polygons"]["ManorFootprint"], np.float64)
    inside = np.zeros_like(h, bool)
    rr, cc = fill_polygon(manor[:, 1] + H, manor[:, 0] + H, h.shape)
    inside[rr, cc] = True
    d_manor = distance_transform_edt(~inside)
    wear = np.maximum(wear, 0.75 * smoothstep(14.0, 2.0, d_manor) * (0.55 + 0.45 * clump))
    d_road = line_distance(layout["road"], h.shape)
    wear = np.maximum(wear, 0.45 * smoothstep(4.5, 2.4, d_road) * smoothstep(0.35, 0.75, clump))
    lm = layout["landmarks"]
    for name, r in (("MillSite", 16.0), ("MineEntrance", 20.0), ("EstateGateway", 9.0)):
        x, y = lm[name][:2]
        d = np.hypot(X - x, Y - y)
        wear = np.maximum(wear, 0.6 * smoothstep(r, r * 0.35, d) * (0.5 + 0.5 * clump))
    density *= 1.0 - 0.9 * wear
    height *= 1.0 - 0.6 * wear

    # No grass in the manor, on water, round the river channel or in town.
    density *= smoothstep(0.5, 1.8, d_manor)
    d_river = line_distance(layout["river"], h.shape)
    density *= smoothstep(3.1, 3.9, d_river)
    height = np.where(d_river < 8.0, np.maximum(height, 1.0 * smoothstep(8.0, 4.0, d_river)), height)  # lush banks
    # The estate lake (lake_basin.py): no blades in the water or on its wet lip, a muddy bed under the
    # water, lush grass round the margin, and a trodden path and landing from the farm.
    lake = layout.get("lake")
    s_lake = np.full(h.shape, np.inf, np.float32)
    trail = np.zeros(h.shape, np.float32)
    if lake:
        poly = np.asarray(lake["shore"], np.float64)
        wet = np.zeros(h.shape, bool)
        rr, cc = fill_polygon(poly[:, 1] + H, poly[:, 0] + H, h.shape)
        wet[rr, cc] = True
        s_lake = np.where(wet, -distance_transform_edt(wet), distance_transform_edt(~wet)).astype(np.float32)
        density *= smoothstep(0.6, 1.4, s_lake)
        height = np.where((s_lake > 0) & (s_lake < 5.0), np.maximum(height, smoothstep(5.0, 1.5, s_lake)), height)
        d_path = line_distance(lake["path"], h.shape)
        lx, ly = lake["landing"]
        # A trodden track about 2 m wide from the farm to the landing, bare in the middle.
        trail = smoothstep(2.6, 1.0, d_path).astype(np.float32)
        tread = np.maximum(0.9 * smoothstep(2.4, 0.9, d_path) * (0.75 + 0.25 * clump), 0.8 * smoothstep(5.0, 1.5, np.hypot(X - lx, Y - ly)))
        wear = np.maximum(wear, np.maximum(tread, np.where(s_lake < 0, 1.0, 0.0)))
        density *= 1.0 - 0.95 * tread
        height *= 1.0 - 0.8 * tread
    tx, ty = lm["TownSquare"][:2]
    density *= smoothstep(100.0, 140.0, np.hypot(X - tx, Y - ty))
    # No blades up through the road bridge's planks (public_road.py records the deck).
    bridge = layout.get("roadBridge")
    if bridge:
        bx, by = bridge["centre"]
        yaw = np.radians(bridge["yaw"])
        along = (X - bx) * np.cos(yaw) + (Y - by) * np.sin(yaw)
        across = -(X - bx) * np.sin(yaw) + (Y - by) * np.cos(yaw)
        density *= 1.0 - ((np.abs(along) < bridge["halfLength"] + 1.0) & (np.abs(across) < bridge["halfWidth"] + 1.4))

    # The derelict farm (manor lane): overgrown inside, but keep the fence line clear.
    fx0, fx1, fy0, fy1 = FARM
    edge = np.minimum(np.minimum(np.abs(X - fx0), np.abs(X - fx1)), np.minimum(np.abs(Y - fy0), np.abs(Y - fy1)))
    on_rect = (X > fx0 - 1.5) & (X < fx1 + 1.5) & (Y > fy0 - 1.5) & (Y < fy1 + 1.5)
    density *= np.where(on_rect, smoothstep(0.8, 1.4, edge), 1.0)

    # Under the trees: leaf litter and moss, a thin shaded grass.
    canopy = canopy_mask(h.shape)
    # The lake trail cuts through the tree belt north of the farm: its trodden soil shows through the
    # leaf litter (the landscape material lays litter over wear wherever the canopy mask is up).
    canopy = canopy * (1.0 - 0.9 * trail)
    density *= 1.0 - 0.8 * canopy
    height *= 1.0 - 0.3 * canopy
    density *= 1.0 - 0.9 * trail                          # the trail's trodden middle is bare

    zone, floor = mvp_woodland_zone(h.shape) or (np.zeros(h.shape, np.float32), np.zeros(h.shape, np.float32))
    density *= 1.0 - zone

    density = np.clip(density, 0, 1)
    # Stony soil on steep banks that aren't painted cliff (the cliff layer has its own rock).
    stony = smoothstep(20.0, 34.0, gaussian_filter(slope, 2.0)) * (1.0 - w["CliffRock"])
    fields = {"density": density, "height": np.clip(height, 0, 1), "dry": np.clip(dry, 0, 1), "wear": np.clip(wear, 0, 1),
              "canopy": canopy, "stony": np.clip(stony, 0, 1), "zone": floor}

    surface = np.full(h.shape, SURFACES["Soil"], np.uint8)
    dominant = np.argmax(np.stack([w[n] for n in LAYERS]), axis=0)
    kind = {"Pasture": "Grass", "WoodlandFloor": "Woodland", "Moorland": "Moor", "DuneSand": "Sand", "Beach": "Sand",
            "CliffRock": "Rock", "DirtRoad": "Road"}
    for i, n in enumerate(LAYERS):
        surface[dominant == i] = SURFACES[kind[n]]
    surface[(surface == SURFACES["Grass"]) & (wear > 0.45)] = SURFACES["Soil"]
    surface[(canopy > 0.5) & np.isin(surface, [SURFACES["Grass"], SURFACES["Moor"]])] = SURFACES["Woodland"]
    surface[(zone > 0.5) & (surface != SURFACES["Water"])] = SURFACES["Woodland"]
    surface[inside] = SURFACES["Soil"]
    surface[(h < 0.25) | (d_river < 3.0) | (s_lake < 0.0)] = SURFACES["Water"]
    return fields, surface


def write_ground(fields, surface):
    os.makedirs(os.path.join(REPO, "Saved", "Ground"), exist_ok=True)
    # Texture: centres of TEX texels, sampled from the 1 m grid (rows = y, cols = x).
    t = -H + (np.arange(TEX) + 0.5) * (2 * H / TEX) + H
    yy, xx = np.meshgrid(t, t, indexing="ij")
    chans = [map_coordinates(fields[k], [yy, xx], order=1, mode="nearest") for k in ("density", "height", "dry", "wear")]
    img = np.stack(chans, -1)
    Image.fromarray(np.round(np.clip(img, 0, 1) * 255).astype(np.uint8), "RGBA").save(
        os.path.join(REPO, "Saved", "Ground", "T_EstateGround.png"))
    extra = np.stack([map_coordinates(fields[k], [yy, xx], order=1, mode="nearest") for k in ("canopy", "stony", "zone")]
                     + [np.ones((TEX, TEX))], -1)
    Image.fromarray(np.round(np.clip(extra, 0, 1) * 255).astype(np.uint8), "RGBA").save(
        os.path.join(REPO, "Saved", "Ground", "T_EstateCanopy.png"))

    # Cells for the game: max density over each cell (and its neighbours, for soft edges), dominant surface.
    step = (SIZE - 1) / CELLS
    c = ((np.arange(CELLS) + 0.5) * step).astype(np.float64)
    cy, cx = np.meshgrid(c, c, indexing="ij")
    dmax = maximum_filter(fields["density"], size=int(np.ceil(step)) + 2)
    dens = map_coordinates(dmax, [cy, cx], order=0, mode="nearest")
    surf = map_coordinates(surface.astype(np.float32), [cy, cx], order=0, mode="nearest").astype(np.uint8)
    out = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateGround.bin")
    with open(out, "wb") as f:
        f.write(b"HGD1" + struct.pack("<H", CELLS))
        f.write(np.stack([np.round(np.clip(dens, 0, 1) * 255).astype(np.uint8), surf], -1).tobytes())
    counts = {k: int((surf == v).sum()) for k, v in SURFACES.items()}
    print("EstateGround.bin", CELLS, "cells;", counts, "grass cells (>0.1):", int((dens > 0.1).sum()))


def wind_noise():
    os.makedirs(os.path.join(REPO, "Assets", "Environment", "Ground"), exist_ok=True)
    n = 256
    rng = np.random.default_rng(21)
    k = np.hypot(*np.meshgrid(np.fft.fftfreq(n) * n, np.fft.fftfreq(n) * n))
    k[0, 0] = 1
    spec = k ** -1.6 * np.exp(-(k / 18.0) ** 2)
    spec[0, 0] = 0
    f = np.real(np.fft.ifft2((rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))) * spec))
    f = (f - f.min()) / (f.max() - f.min())
    Image.fromarray(np.round(f * 255).astype(np.uint8), "L").save(os.path.join(REPO, "Assets", "Environment", "Ground", "T_GrassWind.png"))


# Patch LODs: every blade has a random rank in [0, 1); LOD n keeps the blades ranked below
# LOD_KEEP[n]. M_EstateGrass thins blades by rank with distance and only lets the game use a coarser
# LOD where it would show no more than that LOD holds, so switching never pops.
BLADES = 1100
LOD_KEEP = (1.0, 0.38, 0.14)
LOD_SEGMENTS = (3, 2, 1)


def blades(seed=5):
    rng = np.random.default_rng(seed)
    side = int(np.ceil(np.sqrt(BLADES)))
    # Jittered grid: even cover with no bald spots, and no seams between neighbouring patches.
    g = (np.stack(np.meshgrid(np.arange(side), np.arange(side)), -1).reshape(-1, 2) + rng.random((side * side, 2))) / side
    roots = g[rng.permutation(len(g))[:BLADES]] * 200.0 - 100.0
    return roots, rng.random((BLADES, 6)), rng.random(BLADES)


def blade_mesh(roots, params, ranks, segments):
    """Blade strips over a 200 x 200 cm patch (local cm, z up). UV0: u = rank bucket (0-255) +
    root x in [0, 1), v = root y in [0, 1)."""
    verts, norms, uvs, faces = [], [], [], []
    for (rx, ry), (p0, p1, p2, p3, p4, p5), rank in zip(roots, params, ranks):
        height = 24.0 + 26.0 * p0 ** 1.4                  # 24-50 cm before the ground scales it
        width = 1.0 + 0.9 * p1                            # cm at the base
        face = p2 * 2 * np.pi                             # blade facing
        lean_dir = face + np.pi / 2 + (p3 - 0.5) * 1.2    # roughly across the face, so it curls naturally
        lean = 0.1 + 0.45 * p4 ** 1.5                     # tip offset as a fraction of height
        fx, fy = np.cos(face), np.sin(face)               # across the blade
        lx, ly = np.cos(lean_dir), np.sin(lean_dir)
        u = np.floor(rank * 256.0) + (rx + 100.0) / 200.0 * 0.999
        v = (ry + 100.0) / 200.0 * 0.999
        # Normal: the blade face turned most of the way to up, so the meadow shades like turf.
        n = np.array([-fy * 0.5, fx * 0.5, 1.0])
        n /= np.linalg.norm(n)
        base = len(verts)
        for i in range(segments + 1):
            s = i / segments
            z = height * s
            off = lean * height * s * s
            cx, cy = rx + lx * off, ry + ly * off
            if i < segments:
                half = 0.5 * width * (1.0 - 0.85 * s ** 1.5)
                verts += [(cx - fx * half, cy - fy * half, z), (cx + fx * half, cy + fy * half, z)]
                norms += [n, n]
                uvs += [(u, v), (u, v)]
            else:
                verts.append((cx, cy, z))
                norms.append(n)
                uvs.append((u, v))
        for i in range(segments - 1):
            a = base + 2 * i
            faces += [(a, a + 1, a + 3), (a, a + 3, a + 2)]
        a = base + 2 * (segments - 1)
        faces.append((a, a + 1, a + 2))
    return np.array(verts), np.array(norms), np.array(uvs), np.array(faces)


def write_obj(path, verts, norms, uvs, faces):
    # Interchange's OBJ reader maps OBJ (x, y, z) to Unreal (x, -y, z) (see bake_ocean.py), so write y
    # negated, and swap the winding to keep faces front-facing after the mirror. UVs are written as
    # they are; build_ground.py checks how they came in.
    with open(path, "w") as f:
        f.write("# Estate grass patch, generated by Scripts/Terrain/bake_ground.py\n")
        for x, y, z in verts:
            f.write(f"v {x:.3f} {-y:.3f} {z:.3f}\n")
        for u, v in uvs:
            f.write(f"vt {u:.6f} {v:.6f}\n")
        for x, y, z in norms:
            f.write(f"vn {x:.4f} {-y:.4f} {z:.4f}\n")
        for a, b, c in faces + 1:
            f.write(f"f {a}/{a}/{a} {c}/{c}/{c} {b}/{b}/{b}\n")


def patches():
    out = os.path.join(REPO, "Saved", "Ground")
    os.makedirs(out, exist_ok=True)
    roots, params, ranks = blades()
    stats = []
    for lod, (keep, seg) in enumerate(zip(LOD_KEEP, LOD_SEGMENTS)):
        m = ranks < keep
        v, n, uv, fc = blade_mesh(roots[m], params[m], ranks[m], seg)
        write_obj(os.path.join(out, f"SM_GrassPatch_LOD{lod}.obj"), v, n, uv, fc)
        stats.append((int(m.sum()), len(v), len(fc)))
    print("grass patch LODs (blades, verts, tris):", stats)


def main():
    layout = json.load(open(os.path.join(HERE, "estate_layout.json")))
    h, w = load()
    fields, surface = ground_fields(h, w, layout)
    write_ground(fields, surface)
    wind_noise()
    patches()


if __name__ == "__main__":
    main()
