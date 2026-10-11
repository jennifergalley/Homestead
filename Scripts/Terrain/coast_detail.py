"""Coast detail (map stage 3 review, 2026-10-10 night): makes the coast's cliffs and new slopes read as Cornish coast
rather than smooth graded shapes.

Usage: python Scripts/Terrain/coast_detail.py      (once, "coastDetail" in the layout; after coast_soften.py; then
       weightmaps.py, cove_heath.py, bake_ground.py, bake_estate_map.py; in the editor ApplyEstateHeightfield and
       ApplyEstateWeightmaps over the rectangle it prints)

1. Crags: the playable coast's steep faces (over CLIFF_DEG) get ridged, layered noise (CRAG_M), so the granite has
   buttresses and ledges instead of one smooth ramp. The sand at their foot is left alone.
2. A ragged waterline: the swash and shallow foreshore round the cove and Gull Sands are lifted or lowered by noise, so
   the water's edge wanders instead of running ruler-straight.
3. Scenery (EstateScenery.bin): coast_soften.py's flat granite chips (CL1) are removed; the ground the coast mix moved
   gets scrub and wildflowers (CS1: bracken, yarrow, tall grass, ferns, campion, foxglove, cow parsley, the odd
   windswept hawthorn); a few big erratics stand at the cove's west cliff foot (CS1). CS1 is replaced on every run.
The sea cave's crag and roof (Cave Agent) are kept: no scenery in CAVE_BOX, and the ground over CAVE_ROOF stays at
least CAVE_ROOF_MIN_M.
"""
import json
import os
import subprocess
import sys

import numpy as np
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
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
SIZE, H = 4033, 2016
BEFORE_MIX = "17e1880d1"          # main before map stage 3 (the coast mix's "before")

CLIFF_DEG = (38.0, 50.0)          # crags fade in over this slope range
CRAG_M = 2.2                      # crag relief on a full cliff face
CRAG_MIN_Z = 3.0                  # above the berm
WATERLINE_BAND = (-1.6, 1.2)      # ground heights (m) whose level wanders
WATERLINE_M = 0.7                 # by up to this
SEED = 1854

CAVE_BOX = (-545.0, -515.0, -692.0, -666.0)     # x0, x1, y0, y1: Cave Agent's crag and mouth (no scenery)
CAVE_ROOF = (-545.0, -515.0, -715.0, -690.0)    # the chamber's roof
CAVE_ROOF_MIN_M = 14.0

TAG, OLD_TAG = b"CS1", b"CL1"
RECORD = np.dtype([("k", "u1"), ("pad", "S3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])
# scatter.py kinds: bracken 3, yarrow 4, erratic 7, fern 9, tall grass 10, mid grass 11, hawthorn 16;
# lake_path_plants.py blooms: campion 46, foxglove 47, cow parsley 48.
COVER = [(3, 0.22, (0.8, 1.2)), (4, 0.12, (0.8, 1.2)), (10, 0.22, (0.8, 1.3)), (11, 0.18, (0.8, 1.3)),
         (9, 0.06, (0.8, 1.1)), (46, 0.08, (0.85, 1.2)), (47, 0.06, (0.85, 1.2)), (48, 0.06, (0.85, 1.2))]
COVER_EVERY_M2 = 9.0
COVER_SLOPE_DEG = 40.0
HAWTHORN_EVERY_M2 = 700.0
PATH_CLEAR_M = 2.6
CHANGED_M = 0.8                   # ground the coast mix moved at least this much
ERRATICS = [(-610.0, -682.0, 1.3), (-650.0, -691.0, 1.1), (-575.0, -671.0, 0.9), (-683.0, -697.0, 1.2)]


def git_r16(rev):
    pointer = subprocess.run(["git", "show", f"{rev}:{R16_GIT}"], cwd=REPO, capture_output=True, check=True).stdout
    data = subprocess.run(["git", "lfs", "smudge"], cwd=REPO, input=pointer, capture_output=True, check=True).stdout
    return (np.frombuffer(data, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def noise(shape, cells, seed):
    rng = np.random.default_rng(seed)
    n = ndimage.gaussian_filter(rng.standard_normal((shape[0] // cells + 3, shape[1] // cells + 3)), 0.8)
    out = ndimage.zoom(n, cells, order=3)[: shape[0], : shape[1]]
    return out / max(out.std(), 1e-9)


def crags(z, sl):
    zz = z[sl]
    gy, gx = np.gradient(ndimage.gaussian_filter(zz, 2.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    w = smoothstep(*CLIFF_DEG, slope) * smoothstep(CRAG_MIN_Z, CRAG_MIN_Z + 3.0, zz)
    w = ndimage.gaussian_filter(w, 1.5)
    # Irregular buttresses and blocks at two scales (no regular bedding: banded noise read as sculpting stripes).
    big = 1.0 - np.abs(noise(zz.shape, 13, SEED))
    mid = 1.0 - np.abs(noise(zz.shape, 5, SEED + 1))
    fine = noise(zz.shape, 2, SEED + 2)
    relief = CRAG_M * (0.55 * big + 0.3 * mid + 0.15 * fine)
    z[sl] = zz + w * (relief - CRAG_M * 0.3)
    return w


def waterline(z, sl, layout):
    zz = z[sl]
    lo, hi = WATERLINE_BAND
    band = smoothstep(lo - 0.5, lo, zz) * (1.0 - smoothstep(hi, hi + 0.4, zz))
    wob = noise(zz.shape, 11, SEED + 3) * 0.75 + noise(zz.shape, 4, SEED + 4) * 0.25
    z[sl] = zz + band * WATERLINE_M * wob


def keep_cave(z, before):
    x0, x1, y0, y1 = CAVE_ROOF
    s = (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1))
    z[s] = np.where(before[s] >= CAVE_ROOF_MIN_M, np.maximum(z[s], CAVE_ROOF_MIN_M), z[s])
    x0, x1, y0, y1 = CAVE_BOX
    s = (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1))
    z[s] = before[s]


def path_tree(layout):
    pts = []
    for p in layout.get("footpaths", []):
        a = np.asarray(p["points"], np.float64)[:, :2]
        for u, v in zip(a[:-1], a[1:]):
            pts.append(np.linspace(u, v, max(2, int(np.hypot(*(v - u)) / 0.5) + 1)))
    route = json.load(open(os.path.join(HERE, "cove_route.json")))
    pts.append(np.asarray(route["centreline"], np.float64)[:, :2])
    return cKDTree(np.concatenate(pts))


def in_box(p, box):
    return (p[:, 0] >= box[0]) & (p[:, 0] <= box[1]) & (p[:, 1] >= box[2]) & (p[:, 1] <= box[3])


def scenery(z, before, layout, rng):
    (x0, y0), (x1, y1) = np.min(layout["polygons"]["PlayableBounds"], 0), np.max(layout["polygons"]["PlayableBounds"], 0)
    sl = (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1))
    zz = z[sl]
    gy, gx = np.gradient(ndimage.gaussian_filter(zz, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    changed = np.abs(zz - before[sl]) > CHANGED_M
    yy, xx = np.mgrid[int(y0):int(y1) + 1, int(x0):int(x1) + 1]
    ok = changed & (zz > 2.6) & (slope < COVER_SLOPE_DEG)
    cand = np.c_[xx[ok], yy[ok]].astype(np.float64)
    lines = path_tree(layout)
    recs = []

    def take(every, rng):
        n = int(len(cand) / every)
        if not n:
            return cand[:0]
        p = cand[rng.choice(len(cand), size=n, replace=False)] + rng.uniform(-0.5, 0.5, (n, 2))
        p = p[lines.query(p)[0] > PATH_CLEAR_M]
        return p[~in_box(p, CAVE_BOX)]

    pts = take(COVER_EVERY_M2, rng)
    kinds, shares = [c[0] for c in COVER], np.array([c[1] for c in COVER])
    pick = rng.choice(len(COVER), size=len(pts), p=shares / shares.sum())
    for p, i in zip(pts, pick):
        k, _, (s0, s1) = COVER[i]
        # Flowers in little drifts: a few stems round each pick.
        for q in ([p] if k < 40 else p + rng.normal(0.0, 0.6, (int(rng.integers(3, 7)), 2))):
            recs.append((k, TAG, q[0] * 100.0, q[1] * 100.0, rng.uniform(0, 360), rng.uniform(s0, s1)))
    for p in take(HAWTHORN_EVERY_M2, rng):
        recs.append((16, TAG, p[0] * 100.0, p[1] * 100.0, rng.uniform(-30, 30), rng.uniform(0.7, 1.0)))
    for x, y, s in ERRATICS:
        recs.append((7, TAG, x * 100.0, y * 100.0, rng.uniform(0, 360), s))
    return recs


def write_scenery(recs):
    raw = open(SCENERY, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * RECORD.itemsize], RECORD)
    kept = rec[(rec["pad"] != TAG) & (rec["pad"] != OLD_TAG)]
    out = np.concatenate([kept, np.array(recs, RECORD)]) if recs else kept
    with open(SCENERY, "wb") as fh:
        fh.write(raw[:4] + np.uint32(len(out)).tobytes() + out.tobytes() + raw[8 + count * RECORD.itemsize:])
    print(f"coast detail: {len(recs)} scrub, flower and erratic records; removed {count - len(kept)} old CS1/CL1")


def main():
    layout = json.load(open(LAYOUT))
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0
    before_mix = git_r16(BEFORE_MIX)
    rng = np.random.default_rng(SEED)
    if not layout.get("coastDetail"):
        before = z.copy()
        (x0, y0), (x1, y1) = np.min(layout["polygons"]["PlayableBounds"], 0), np.max(layout["polygons"]["PlayableBounds"], 0)
        sl = (slice(int(y0) + H, int(y1) + H + 1), slice(int(x0) + H, int(x1) + H + 1))
        crags(z, sl)
        waterline(z, sl, layout)
        keep_cave(z, before)
        u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
        moved = np.argwhere(u16 != raw)
        u16.astype("<u2").tofile(R16)
        Image.fromarray(u16).save(PNG)
        z = (u16.astype(np.float64) - 32768.0) / 128.0
        npy = os.path.join(WORK, "game_reshaped_4033.npy")
        if os.path.exists(npy) and len(moved):
            zn = np.load(npy)
            r, c = moved[:, 0], moved[:, 1]
            zn[2 * H - c, r] = z[r, c].astype(zn.dtype)
            np.save(npy, zn)
        layout["coastDetail"] = True
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)
        (a, b), (c, e) = moved.min(0), moved.max(0)
        print(f"coast detail: {len(moved)} vertices; ApplyEstateHeightfield rows {a}-{c}, cols {b}-{e}")
    write_scenery(scenery(z, before_mix, layout, rng))


if __name__ == "__main__":
    main()
