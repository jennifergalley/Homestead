"""Rock the cove's west cliff (Jenny's card "[Balance] Rock face for the cove cliff"): drape the estate's granite
outcrop meshes over the steep face round the sea cave, sunk into the slope, with thrift (sea pink) cushions on its
ledges and at its foot, so the wall reads as a rock cliff from the beach instead of a smooth earth bank.

    python Scripts/Terrain/cove_cliff_rocks.py [--preview]

Reads EstateHeightfield.r16 (Map's settled coast) and the sea cave's table (HomesteadSeaCave.inc, for the cave's
footprint and mouth), and writes Source/SurvivalGame/Simulation/HomesteadCoveCliff.inc (generated; do not edit by
hand), which AHomesteadSeaCave places at runtime. Scenery only: no placement ids, nothing saved, the landscape and
the walkable beach untouched. Rerun whenever the coast or the cave's site changes.

Rules: rocks only where the ground is steep (a cliff, not the beach or the grassy head of the bay); every rock's
footprint stays above the dry berm (so nothing juts onto the sand) and is lowered until no edge floats; nothing
over the cave (its roof is the hillside) or within the mouth's box; nothing near the cove path.
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
LAYOUT = os.path.join(HERE, "estate_layout.json")
CAVE_INC = os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "HomesteadSeaCave.inc")
OUT = os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "HomesteadCoveCliff.inc")
SIZE, H = 4033, 2016
SEED = 4417
PROPS = "/Game/SurvivalGame/Environment/Props"

# The wall (world m): the cove bay's west side from the path's foot round the corner toward the open coast.
REGION = dict(x=(-612.0, -462.0), y=(-760.0, -600.0))
STEEP = 0.75                  # rise per metre: steeper than this is cliff
MIN_Z, MAX_Z = 3.0, 62.0      # rock centres between these heights (m above the sea)
BERM_Z = 2.4                  # every rock's footprint stays above this (the dry sand is at ~1.7 m)
MOUTH_BOX = dict(x=(-537.0, -523.0), y=(-697.0, -655.0))   # the walk into the mouth (rock may flank the crag)
MOUTH_TOP_Z = 15.0            # ... up to its top (world m); above, the face over the crag is rocked too
CAVE_MARGIN_M = 5.0           # no rock within this of the cave's footprint (the hillside is its roof)
PATH_CLEAR_M = 10.0           # nor this near the cove path
TILT = 0.85                   # how far a rock leans from upright toward the slope's normal
# (mesh, footprint radius m at scale 1, height m, plan scales, height scale (of the plan scale), unused). Angular
# blocks only (the rounded dome read as glued-on pebbles): the split boulder leads each outcrop as a broad slab,
# jointed blocks and erratics crowd round it.
ROCKS = [
    ("GraniteSplitBoulder/SM_GraniteSplitBoulder", 3.6, 4.2, (1.2, 2.6), (0.45, 0.8), 0),
    ("GraniteBoulderJointed/SM_GraniteBoulderJointed", 1.5, 1.9, (0.8, 3.2), (0.7, 1.1), 0),
    ("GraniteSplitBoulder/SM_GraniteSplitBoulder", 3.6, 4.2, (0.6, 1.4), (0.6, 1.0), 0),
    ("GraniteErratic/SM_GraniteErratic", 1.4, 2.0, (0.9, 2.6), (0.7, 1.0), 0),
]
LEAD_SCALE = (4.0, 6.0)       # each cluster's lead slab
CLUSTERS = 18
CLUSTER_SPACING_M = 15.0
COMPANIONS = (5, 11)
EMBED = (0.0, 0.18)           # extra share of a block's height sunk below its rim (its mesh already sits sunk)
ROOF_CLEAR_M = 2.5            # over the cave, rock only where the ground stands this far above its highest roof
SPACING = 0.35                # of the sum of two footprint radii: blocks in a cluster crowd and overlap
# Blocks banked against the cave crag's sides and shoulders (world m, plan scale): the crag spans about x -538..-522
# at the cliff foot (y -676) up to y -688.
CRAG_FLANKS = [(-541.0, -681.0, 2.4), (-543.5, -686.5, 2.8), (-519.0, -681.0, 2.4), (-516.5, -686.0, 2.8),
               (-536.0, -690.5, 2.6), (-524.0, -691.0, 2.6), (-530.0, -693.0, 3.0)]
CORE = 0.55                   # of a block's footprint radius: the part that must clear the sand, mouth and paths
CRACK_THRIFT = 0.5            # chance of a thrift tuft in the crack at a block's downhill foot
THRIFT = "Thrift/SM_ThriftClump"
THRIFT_DRIFTS = 26            # drifts of 3-7 cushions on ledges and the cliff foot


def load_heights():
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    return (raw.astype(np.float64) - 32768.0) / 128.0


def height(z, x, y):
    col, row = np.asarray(x, float) + H, np.asarray(y, float) + H
    c0, r0 = np.floor(col).astype(int), np.floor(row).astype(int)
    fc, fr = col - c0, row - r0
    a, b, c, d = z[r0, c0], z[r0, c0 + 1], z[r0 + 1, c0], z[r0 + 1, c0 + 1]
    return np.where(fc + fr <= 1.0, a + fc * (b - a) + fr * (c - a), d + (1 - fc) * (c - d) + (1 - fr) * (b - d))


def normal(z, x, y, e=1.5):
    gx = (height(z, x + e, y) - height(z, x - e, y)) / (2 * e)
    gy = (height(z, x, y + e) - height(z, x, y - e)) / (2 * e)
    n = np.stack([-gx, -gy, np.ones_like(gx)], axis=-1)
    return n / np.linalg.norm(n, axis=-1, keepdims=True)


def cave_footprint():
    """The cave's outline in world m, from HomesteadSeaCave.inc (cave-local cm, +Y the cave's right); sets
    CAVE_ROOF_Z, its highest roof (world m)."""
    global CAVE_ROOF_Z
    text = open(CAVE_INC).read()
    floor = float(re.search(r"SEA_CAVE_ANCHOR\(([^)]*)\)", text).group(1).split(",")[2])
    CAVE_ROOF_Z = (floor + float(re.search(r"SEA_CAVE_ROOF_MAX\(([^)]*)\)", text).group(1))) / 100.0
    ax, ay, _, yaw = map(float, re.search(r"SEA_CAVE_ANCHOR\(([^)]*)\)", text).group(1).split(","))
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    ring = []
    for m in re.finditer(r"SEA_CAVE_FOOTPRINT\(([^,]+),([^)]+)\)", text):
        lx, ly = float(m.group(1)), float(m.group(2))
        ring.append(((ax + c * lx - s * ly) / 100.0, (ay + s * lx + c * ly) / 100.0))
    return np.array(ring)


def ring_distance(ring, pts):
    """Distance (m) from points to a closed ring's outline, negative inside."""
    from matplotlib.path import Path
    inside = Path(ring).contains_points(pts)
    a, b = ring, np.roll(ring, -1, axis=0)
    ab = b - a
    t = np.clip(((pts[:, None, :] - a[None]) * ab[None]).sum(-1) / (ab * ab).sum(-1)[None], 0, 1)
    d = np.linalg.norm(pts[:, None, :] - (a[None] + t[..., None] * ab[None]), axis=-1).min(1)
    return np.where(inside, -d, d)


def polyline_distance(line, pts):
    a, b = line[:-1], line[1:]
    ab = b - a
    t = np.clip(((pts[:, None, :] - a[None]) * ab[None]).sum(-1) / np.maximum((ab * ab).sum(-1)[None], 1e-9), 0, 1)
    return np.linalg.norm(pts[:, None, :] - (a[None] + t[..., None] * ab[None]), axis=-1).min(1)


def in_box(box, x, y, pad=0.0):
    return (box["x"][0] - pad <= x) & (x <= box["x"][1] + pad) & (box["y"][0] - pad <= y) & (y <= box["y"][1] + pad)


def footprint_points(cx, cy, r, count=12):
    a = np.linspace(0, math.tau, count, endpoint=False)
    return np.c_[cx + r * np.cos(a), cy + r * np.sin(a)]


def main():
    rng = np.random.default_rng(SEED)
    z = load_heights()
    layout = json.load(open(LAYOUT))
    paths = [np.array(f["points"], float) for f in layout.get("footpaths", []) if f.get("name") in ("Cove", "Mine")]
    cave = cave_footprint()

    xs = np.arange(REGION["x"][0], REGION["x"][1], 1.0)
    ys = np.arange(REGION["y"][0], REGION["y"][1], 1.0)
    gx, gy = np.meshgrid(xs, ys)
    px, py = gx.ravel(), gy.ravel()
    pz = height(z, px, py)
    n = normal(z, px, py)
    slope = np.hypot(n[:, 0], n[:, 1]) / n[:, 2]
    ok = (slope > STEEP) & (pz > MIN_Z) & (pz < MAX_Z) & ~(in_box(MOUTH_BOX, px, py) & (pz < MOUTH_TOP_Z))
    pts = np.c_[px, py]
    roof = CAVE_ROOF_Z + ROOF_CLEAR_M
    ok &= (ring_distance(cave, pts) > CAVE_MARGIN_M) | (pz > roof)
    for line in paths:
        ok &= polyline_distance(line, pts) > PATH_CLEAR_M
    cand = np.flatnonzero(ok)
    rng.shuffle(cand)
    print(f"cove cliff: {len(cand)} m2 of steep face to dress")

    # Outcrops in clusters (orchestrator/Balance review: rows of same-size boulders read as props): a few cluster
    # sites spread over the face, each a big angular lead block with companions strung along the contour (the
    # sheet joints run along the face, so ledges do), of mixed size, tilt and embed depth.
    sites = []
    for i in cand:
        if len(sites) >= CLUSTERS:
            break
        if all(math.hypot(px[i] - sx, py[i] - sy) > CLUSTER_SPACING_M for sx, sy, _ in sites):
            sites.append((px[i], py[i], i))
    trials = []
    for sx, sy, si in sites:
        ncl = n[si]
        along = np.array([-ncl[1], ncl[0]])
        along /= max(np.linalg.norm(along), 1e-6)
        downhill = np.array([ncl[0], ncl[1]]) / max(np.hypot(ncl[0], ncl[1]), 1e-6)
        trials.append((0, sx, sy, LEAD_SCALE))
        for _ in range(rng.integers(*COMPANIONS)):
            t = rng.normal(0, 7.0)
            d = rng.normal(0, 2.5)
            trials.append((rng.integers(1, len(ROCKS)), sx + along[0] * t + downhill[0] * d,
                           sy + along[1] * t + downhill[1] * d, None))
    # Bury the mouth's crag in the wall: blocks banked against its sides and over its shoulders, so the slope runs
    # on round it instead of meeting it at a seam.
    for x, y, sc in CRAG_FLANKS:
        trials.insert(0, (1, x, y, (sc, sc * 1.15)))
    placed = []           # (x, y, radius)
    rocks = []
    for k, x, y, lead in trials:
        mesh, r1, h1, scales, flat, _ = ROCKS[k]
        s = rng.uniform(*(lead or scales))
        sz = s * rng.uniform(*flat)
        r = r1 * s
        x, y = float(x), float(y)
        if any(math.hypot(x - qx, y - qy) < SPACING * (r + qr) for qx, qy, qr in placed):
            continue
        h0 = float(height(z, x, y))
        nn = normal(z, np.array([x]), np.array([y]))[0]
        if np.hypot(nn[0], nn[1]) / nn[2] < STEEP * 0.6 or not (MIN_Z < h0 < MAX_Z):
            continue
        if in_box(MOUTH_BOX, np.array([x]), np.array([y]))[0] and h0 < MOUTH_TOP_Z:
            continue
        # The block's solid core (its rounded rim is mostly sunk) must stay off the sand and out of the mouth.
        fp = footprint_points(x, y, r * CORE)
        fz = height(z, fp[:, 0], fp[:, 1])
        if fz.min() < BERM_Z or (in_box(MOUTH_BOX, fp[:, 0], fp[:, 1], pad=1.0) & (fz < MOUTH_TOP_Z)).any():
            continue
        over = ring_distance(cave, fp) < CAVE_MARGIN_M
        if over.any() and (fz[over] - 0.3 * h1 * sz).min() < roof:
            continue
        if any((polyline_distance(line, fp) < PATH_CLEAR_M).any() for line in paths):
            continue
        # Lean with the slope, then off it a little, each block its own way.
        up = np.array([0.0, 0.0, 1.0]) * (1 - TILT) + nn * TILT + rng.normal(0, 0.18, 3)
        up[2] = max(up[2], 0.35)
        up /= np.linalg.norm(up)
        # Lower the rock along its up axis until its base plane is at or under the ground all round (no edge
        # floats), then sink it a varied share of its height (some barely bedded, some mostly buried).
        base = np.array([x, y, h0])
        offs = np.c_[fp - [x, y], np.zeros(len(fp))]
        offs[:, 2] = -(offs[:, 0] * up[0] + offs[:, 1] * up[1]) / up[2]      # stay in the plane normal to up
        rim = base + offs
        drop = max(0.0, float((rim[:, 2] - height(z, rim[:, 0], rim[:, 1])).max())) + 0.15 \
            + rng.uniform(*EMBED) * h1 * sz
        pivot = base - up * drop / up[2]
        yaw = rng.uniform(0, 360)
        rocks.append((mesh, pivot, yaw, up, s, sz))
        placed.append((x, y, r))
    print(f"cove cliff: {len(rocks)} rocks")
    cover = np.zeros(len(cand), bool)
    for qx, qy, qr in placed:
        cover |= np.hypot(px[cand] - qx, py[cand] - qy) < qr * 0.8
    print(f"cove cliff: {cover.mean():.0%} of the steep face under rock")

    # Thrift: drifts on the face's gentler ledges and along the cliff foot, never on the walking line.
    flowers = []
    # ... and tufts in the cracks at the outcrops' downhill feet.
    for (mesh, p, _, up, s, sz), (qx, qy, qr) in zip(rocks, placed):
        if rng.random() > CRACK_THRIFT:
            continue
        nn = normal(z, np.array([qx]), np.array([qy]))[0]
        down = np.array([nn[0], nn[1]]) / max(np.hypot(nn[0], nn[1]), 1e-6)
        for _ in range(rng.integers(1, 4)):
            a = rng.normal(0, 0.7)
            d = np.array([down[0] * math.cos(a) - down[1] * math.sin(a), down[0] * math.sin(a) + down[1] * math.cos(a)])
            x, y = qx + d[0] * qr * 0.92, qy + d[1] * qr * 0.92
            fz = float(height(z, x, y))
            if fz < 2.0 or ring_distance(cave, np.array([[x, y]]))[0] < 1.0 or in_box(MOUTH_BOX, x, y) and fz < 3.0:
                continue
            ts = rng.uniform(0.8, 1.2)
            flowers.append((THRIFT, np.array([x, y, fz - 0.03]), rng.uniform(0, 360), np.array([0.0, 0.0, 1.0]), ts, ts))
    ledge = np.flatnonzero((slope > 0.35) & (slope < 1.2) & (pz > 2.0) & (pz < 45.0) & ~in_box(MOUTH_BOX, px, py, -4.0))
    seeds = rng.choice(ledge, size=min(THRIFT_DRIFTS, len(ledge)), replace=False)
    for i in seeds:
        for _ in range(rng.integers(3, 8)):
            x, y = px[i] + rng.normal(0, 0.8), py[i] + rng.normal(0, 0.8)
            fz = float(height(z, x, y))
            if fz < 2.0 or ring_distance(cave, np.array([[x, y]]))[0] < 1.0:
                continue
            nn = normal(z, np.array([x]), np.array([y]))[0]
            up = np.array([0.0, 0.0, 1.0]) * 0.7 + nn * 0.3
            s = rng.uniform(0.8, 1.3)
            flowers.append((THRIFT, np.array([x, y, fz - 0.03]), rng.uniform(0, 360), up / np.linalg.norm(up), s, s))
    # And on the shelves round the cave's own crag outside the mouth (Balance: sea pinks on the ledges).
    for x, y in ((-543.5, -673.5), (-542.0, -671.0), (-517.5, -673.0), (-519.0, -670.5), (-545.0, -677.0),
                 (-515.0, -676.5)):
        fz = float(height(z, x, y))
        s = rng.uniform(0.9, 1.2)
        flowers.append((THRIFT, np.array([x, y, fz - 0.03]), rng.uniform(0, 360), np.array([0.0, 0.0, 1.0]), s, s))
    print(f"cove cliff: {len(flowers)} thrift cushions")

    lines = ["// Generated by Scripts/Terrain/cove_cliff_rocks.py from the estate heightfield and HomesteadSeaCave.inc;",
             "// do not edit by hand. COVE_CLIFF_PROP(mesh, x, y, z, yaw, up x, up y, up z, scale, height scale): world cm, the",
             "// mesh's pivot (ground line) and its up axis; rocks block, thrift is walk-through."]
    for mesh, p, yaw, up, s, sz in rocks + flowers:
        name = mesh.rsplit("/", 1)[1]
        lines.append(f'COVE_CLIFF_PROP("{PROPS}/{mesh}.{name}", {p[0] * 100:.1f}, {p[1] * 100:.1f}, {p[2] * 100:.1f}, '
                     f"{yaw:.1f}, {up[0]:.4f}, {up[1]:.4f}, {up[2]:.4f}, {s:.3f}, {sz:.3f})")
    with open(OUT, "w", newline="\r\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("cove cliff: wrote", os.path.relpath(OUT, REPO))

    if "--preview" in sys.argv:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(9, 9))
        sub = z[int(REGION["y"][0]) + H:int(REGION["y"][1]) + H, int(REGION["x"][0]) + H:int(REGION["x"][1]) + H]
        ax.contourf(ys[:sub.shape[0]], xs[:sub.shape[1]], sub.T, levels=[-5, 0, 2.2, 5, 10, 20, 30, 45, 70],
                    cmap="terrain")
        for mesh, p, _, _, s, _ in rocks:
            r = next(r for m, r, *_ in ROCKS if m == mesh) * s
            ax.add_patch(plt.Circle((p[1], p[0]), r, color="0.3", alpha=0.6))
        f = np.array([fl[1] for fl in flowers]) if flowers else np.zeros((0, 3))
        ax.plot(f[:, 1], f[:, 0], ".", color="m", ms=3)
        ax.plot(np.r_[cave[:, 1], cave[0, 1]], np.r_[cave[:, 0], cave[0, 0]], "k-")
        for line in paths:
            ax.plot(line[:, 1], line[:, 0], "r-")
        ax.set_aspect(1)
        out = os.path.join(os.environ.get("HOMESTEAD_SCRATCH", os.path.join(REPO, "Saved")), "cove_cliff.png")
        plt.savefig(out, dpi=90)
        print("cove cliff: preview", out)


if __name__ == "__main__":
    main()
