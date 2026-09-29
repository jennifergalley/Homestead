"""The MVP survival game's woodland, rebuilt inside one region of the Estate (add-mvp-woodland-biome).

The MVP grew its woods per 24 m chunk at runtime. This ports those rules to the baked estate scenery
on a 24 m chunk grid over the region in mvp_woodland.json:

  trees        HomesteadWorldGeneration.cpp TreeCandidate / AssignTreePalette (36 slots on a 4 m
               grid, four cluster pulls, 1-in-4 small openings, 1-in-7 granite knobs;
               60% small broadleaf, 35% mature fir, 5% jacaranda)
  young trees  the Sapling forage patches (young broadleaf, fir pole, fir saplings)
  forage       Candidate(): one branch pile and small patches of stones, berry bushes, roots and
               flowers per chunk, here at a share of the MVP rate (interactable, ids 560000+)
  underbrush   HomesteadWorld.cpp GenerateUnderbrush (thicket / shrub / meadow clusters by a density
               field, plus scattered plants, 0.6 x radius spacing)
  granite      GenerateRocks (outcrops in broad bands, a dome or split boulder on each knob, lone
               erratics; the MVP's loose cobble clusters become a few granite ledges at the
               outcrops, so nothing looks like the hand stones she can pick up)
  ground cover BuildDecorations (grass clumps, every 32nd try a fern, every 64th a flower)

and the MVP's clearance rules (IsDecorationReserved: cover keeps off tree trunks and forage).
The noise is this script's own (fixed seed), so the woods follow the MVP's rules, not its exact
trees. Kind bytes must match EstateSceneryKinds in HomesteadWorld.cpp.
"""
import json
import os

import numpy as np
from scipy.ndimage import gaussian_filter, map_coordinates
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

CHUNK = 24.0
# Shared with scatter.py (the estate's own kinds, which are also MVP meshes).
BROADLEAF, FIR, HAZEL, BRACKEN, YARROW, LEDGE, LOAF, ERRATIC, DOME, FERN_A, GRASS_TALL, GRASS_MID = range(12)
# The MVP woodland's own kinds (19+; 16-18 are the trees lane's hawthorn, holly and hazel coppice).
(JACARANDA, FIR_POLE, FIR_SAPLING_A, FIR_SAPLING_C, BRAMBLE, BRAMBLE_LARGE, TOYON, DEER_BRUSH,
 THIMBLEBERRY, STRAWBERRY, FERN_B, FERN_C, FERN_D, GRASS_SMALL, GRASS_TINY, FLOWER_A, FLOWER_B,
 SPALLS, RUBBLE, TALUS, BOULDER_LOW, JOINTED, SPLIT) = range(19, 42)
TREE_KINDS = (BROADLEAF, FIR, JACARANDA, FIR_POLE, FIR_SAPLING_A, FIR_SAPLING_C)

# GenerateUnderbrush's species, in its enum order: (kind, radius m, min scale, max scale, low).
UNDERBRUSH = [
    (BRAMBLE, 1.05, 0.85, 1.2, False), (BRAMBLE_LARGE, 1.55, 0.9, 1.15, False),
    (TOYON, 1.35, 0.85, 1.1, False), (HAZEL, 0.9, 0.8, 1.15, False),
    (DEER_BRUSH, 0.72, 0.8, 1.2, False), (THIMBLEBERRY, 0.70, 0.8, 1.2, False),
    (BRACKEN, 0.75, 0.75, 1.25, True), (STRAWBERRY, 0.28, 0.8, 1.3, True),
    (YARROW, 0.30, 0.8, 1.3, True),
]
UB_BRAMBLE, UB_BRAMBLE_LARGE, UB_HEDGE, UB_HAZEL, UB_DEER, UB_THIMBLE, UB_FERN, UB_STRAWBERRY, UB_YARROW = range(9)
# GenerateRocks' granite, in its enum order: (kind, radius m, min scale, max scale, size class).
ROCKS = [
    (LEDGE, 0.45, 0.8, 1.3, 0), (SPALLS, 0.40, 0.8, 1.3, 0), (RUBBLE, 0.40, 0.8, 1.4, 0),
    (LOAF, 0.62, 0.8, 1.35, 1), (TALUS, 0.52, 0.8, 1.4, 1), (BOULDER_LOW, 0.72, 0.8, 1.3, 1),
    (ERRATIC, 1.5, 0.85, 1.2, 2), (JOINTED, 1.6, 0.85, 1.15, 2),
    (DOME, 4.7, 0.85, 1.05, 3), (SPLIT, 4.0, 0.9, 1.1, 3),
]
R_COBBLES, R_SPALLS, R_RUBBLE, R_LOAF, R_TALUS, R_LOW, R_ERRATIC, R_JOINTED, R_DOME, R_SPLIT = range(10)
KNOB_RADIUS = 5.4
# Grass variety by try (BuildDecorations: Attempt % 16): 1 mid, 2 small, 9 tall, 4 tiny.
GRASS_BY_TRY = [GRASS_MID] + [GRASS_SMALL] * 2 + [GRASS_TALL] * 9 + [GRASS_TINY] * 4
# The MVP made 1200 tries per chunk (about 2 grass clumps per m2). The estate draws the whole region
# at once, so it keeps a share of them; ferns and flowers keep the full MVP rate.
GRASS_SHARE = 0.5
# Interactables, as a share of the MVP's per-chunk rates (the estate's other woods are scenery).
TREE_INTERACTIVE_SHARE = 0.04
FORAGE_SHARE = {"Branches": 0.3, "BerryBush": 0.12, "Roots": 0.1, "Stones": 0.08, "Flowers": 0.06}
FIRST_ID = 560000


def densify(p, step):
    p = np.asarray(p, float)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def pick(rng, weights):
    """PickUnderbrush: one entry from [(value, weight), ...]."""
    total = sum(w for _, w in weights)
    roll = rng.integers(0, total)
    for value, w in weights:
        if roll < w:
            return value
        roll -= w
    return weights[0][0]


class Region:
    def __init__(self, path, seed=1852):
        spec = json.load(open(path))
        self.poly = np.array(spec["polygon"], float)
        self.edge = float(spec.get("edge_m", 25.0))
        self.glades = np.array(spec.get("glades", []), float).reshape(-1, 3)
        self.rng = np.random.default_rng(seed)
        self.ring = cKDTree(densify(np.r_[self.poly, self.poly[:1]], 1.0))
        self.lo = np.floor((self.poly.min(axis=0) - 30.0) / CHUNK) * CHUNK
        self.hi = np.ceil((self.poly.max(axis=0) + 30.0) / CHUNK) * CHUNK
        # Smooth noise fields over the region on a 2 m grid (all Perlin-like, std about 0.3).
        self.step = 2.0
        shape = tuple(((self.hi - self.lo) / self.step).astype(int) + 1)

        def field(sigma_m):
            n = gaussian_filter(self.rng.normal(0, 1, shape), sigma_m / self.step)
            return n / n.std() * 0.3
        self.edge_noise = field(8.0)
        self.broad = field(8.0)        # UnderbrushDensity's 34 m Perlin
        self.fine = field(3.0)         # and its 11 m one
        self.band = field(18.0)        # GenerateRocks' 72 m outcrop bands

    def sample(self, grid, p):
        p = np.atleast_2d(p)
        return map_coordinates(grid, [(p[:, 0] - self.lo[0]) / self.step, (p[:, 1] - self.lo[1]) / self.step],
                               order=1, mode="nearest")

    def depth(self, p):
        """Metres inside the polygon (negative outside)."""
        p = np.atleast_2d(p)
        if not len(p):
            return np.zeros(0)
        d = self.ring.query(p)[0]
        return np.where(points_in_poly(p, self.poly), d, -d)

    def reach(self, p):
        """Depth with a noisy edge, so neither the fringe nor the Cornish cut-off is a straight line."""
        return self.depth(p) + 18.0 * self.sample(self.edge_noise, p)

    def presence(self, p):
        """How fully the MVP woods grow here: 0 outside, rising to 1 across the ecotone."""
        return smoothstep(0.0, self.edge, self.reach(p)) * (self.depth(p) > 0)

    def cornish_blocked(self, p):
        """Where the estate's own (Cornish) scenery gives way to the MVP woods."""
        return self.reach(p) > 8.0 if len(p) else np.zeros(0, bool)

    def in_glade(self, p, scale=1.0):
        p = np.atleast_2d(p)
        out = np.zeros(len(p), bool)
        for gx, gy, r in self.glades:
            out |= np.hypot(p[:, 0] - gx, p[:, 1] - gy) < r * scale
        return out

    def chunks(self):
        for ox in np.arange(self.lo[0], self.hi[0], CHUNK):
            for oy in np.arange(self.lo[1], self.hi[1], CHUNK):
                yield ox, oy


def build(region, keep, ground_ok, avoid=None):
    """Returns (placements, scenery). placements: [(id, ResourceKind name, (x, y) m)];
    scenery: [(kind, x m, y m, yaw, scale)]. keep(p) -> bool mask of points clear of the estate's
    roads, river, landmarks and farm; ground_ok(p) -> bool mask of dry, walkable land; avoid: the
    estate's other interactables (m), which the woods keep clear of as they do their own."""
    rng = region.rng
    U = rng.uniform
    avoid = np.zeros((0, 2)) if avoid is None or not len(avoid) else np.asarray(avoid, float)
    avoid_tree = cKDTree(avoid) if len(avoid) else None

    def allowed(p):
        p = np.atleast_2d(p)
        if not len(p):
            return np.zeros(0, bool)
        return keep(p) & ground_ok(p) & (rng.random(len(p)) < region.presence(p))

    # ---- trees, knobs and openings, chunk by chunk -------------------------------------------
    trees, knobs, openings = [], [], []            # trees: (x, y, kind, scale)
    for ox, oy in region.chunks():
        centres = [(ox + cx * 12 + 3.5 + U(0, 5), oy + cy * 12 + 3.5 + U(0, 5)) for cx in (0, 1) for cy in (0, 1)]
        opening = (ox + 4 + U(0, 16), oy + 4 + U(0, 16)) if rng.random() < 0.25 else None
        knob = (ox + 7 + U(0, 10), oy + 7 + U(0, 10)) if rng.random() < 1 / 7 else None
        if opening: openings.append(opening)
        if knob: knobs.append(knob)
        for slot in range(36):
            col, row = slot % 6, slot // 6
            cx, cy = ox + col * 4, oy + row * 4
            centre = centres[(col // 3) * 2 + row // 3]
            x = cx + 0.5 + U(0, 3)
            y = cy + 0.5 + U(0, 3)
            x = np.clip(x + np.clip((centre[0] - x) / 3, -1, 1), cx + 0.5, cx + 3.5)
            y = np.clip(y + np.clip((centre[1] - y) / 3, -1, 1), cy + 0.5, cy + 3.5)
            roll, detail, keep_roll = rng.random(), rng.random(), rng.random()
            if keep_roll >= 0.965: continue
            if opening and np.hypot(x - opening[0], y - opening[1]) < 2.8: continue
            if knob and np.hypot(x - knob[0], y - knob[1]) < KNOB_RADIUS: continue
            if roll < 0.60: trees.append((x, y, BROADLEAF, 0.90 + detail * 0.14))
            elif roll < 0.95: trees.append((x, y, FIR, 0.90 + detail * 0.15))
            else: trees.append((x, y, JACARANDA, 0.90 + detail * 0.16))
    trees = np.array(trees)
    p = trees[:, :2]
    trees = trees[allowed(p) & ~region.in_glade(p)]
    knobs = np.array(knobs).reshape(-1, 2)
    knobs = knobs[region.depth(knobs) > 4.0]

    # A share of the trees can be felled, as every MVP tree could; they draw the MVP palette by id.
    rows = []
    next_id = [FIRST_ID]

    def add_row(kind, pt):
        rows.append((next_id[0], kind, (float(pt[0]), float(pt[1]))))
        next_id[0] += 1
    felling = np.zeros(len(trees), bool)
    chosen = []
    for i in np.flatnonzero((rng.random(len(trees)) < TREE_INTERACTIVE_SHARE) & np.isin(trees[:, 2], (BROADLEAF, FIR, JACARANDA))):
        if chosen and np.min(np.hypot(*(np.array(chosen) - trees[i, :2]).T)) < 6.0: continue
        if avoid_tree is not None and avoid_tree.query(trees[i, :2])[0] < 2.0: continue
        chosen.append(trees[i, :2])
        felling[i] = True
        add_row("ForestTree", trees[i, :2])

    # ---- young trees (the MVP's sapling patches) ----------------------------------------------
    young = []
    for ox, oy in region.chunks():
        ax, ay = ox + 4 + U(0, 16), oy + 4 + U(0, 16)
        for slot in range(4):
            x, y = (ax, ay) if slot == 0 else (ax + U(-1.2, 1.2), ay + U(-1.2, 1.2))
            member = rng.random() < 0.67
            roll, variant, detail = rng.random(), rng.integers(0, 4), rng.random()
            if slot > 0 and not member: continue
            if roll < 0.45: young.append((x, y, BROADLEAF, 0.90 + detail * 0.18))
            elif variant == 2: young.append((x, y, FIR_POLE, 0.92 + detail * 0.16))
            else: young.append((x, y, FIR_SAPLING_A if variant % 2 else FIR_SAPLING_C, 0.92 + detail * 0.16))
    young = np.array(young)
    ok = allowed(young[:, :2]) & ~region.in_glade(young[:, :2], 0.6)
    ok &= cKDTree(trees[:, :2]).query(young[:, :2])[0] > 1.2
    if len(knobs): ok &= cKDTree(knobs).query(young[:, :2])[0] > KNOB_RADIUS
    young = young[ok]

    # ---- forage (interactable) -----------------------------------------------------------------
    trunk_tree = cKDTree(trees[felling, :2]) if felling.any() else None
    forage = []
    for ox, oy in region.chunks():
        for kind, share in FORAGE_SHARE.items():
            ax, ay = ox + 4 + U(0, 16), oy + 4 + U(0, 16)
            present = rng.random() < share
            members = [(ax, ay)]
            if kind != "Branches":
                threshold = 0.40 if kind == "Flowers" else 0.67
                for _ in range(3):
                    x, y, roll = ax + U(-1.2, 1.2), ay + U(-1.2, 1.2), rng.random()
                    if roll < threshold: members.append((x, y))
            if present:
                forage += [(kind, x, y) for x, y in members]
    placed_forage = []
    for kind, x, y in forage:
        pt = np.array([x, y])
        if not allowed(pt)[0]: continue
        if len(knobs) and np.min(np.hypot(*(knobs - pt).T)) < KNOB_RADIUS + 0.6: continue
        if trunk_tree is not None and trunk_tree.query(pt)[0] < 1.5: continue
        if avoid_tree is not None and avoid_tree.query(pt)[0] < 1.5: continue
        if placed_forage and np.min(np.hypot(*(np.array(placed_forage) - pt).T)) < 0.9: continue
        placed_forage.append(pt)
        add_row(kind, pt)
    forage_pts = np.array(placed_forage).reshape(-1, 2)

    # Everything cover keeps off: trunks (all trees are "resources" in the MVP) and forage.
    stems = np.r_[trees[:, :2], young[:, :2], forage_pts, avoid]
    stem_tree = cKDTree(stems)
    # Rock scenery keeps away from the stones she can pick up.
    stone_pts = np.array([pt for _, kind, pt in rows if kind == "Stones"]).reshape(-1, 2)
    stone_tree = cKDTree(stone_pts) if len(stone_pts) else None

    def clear_of_stones(p, gap=8.0):
        return np.ones(len(p), bool) if stone_tree is None else stone_tree.query(p)[0] > gap

    def reserved(p, radius, low):
        """IsDecorationReserved against trunks and forage: FootprintRadius + 35 cm (low cover) or 130 cm."""
        return stem_tree.query(np.atleast_2d(p))[0] < radius + (0.35 if low else 1.3)

    # ---- granite (GenerateRocks) ---------------------------------------------------------------
    rocks = []                                     # (index into ROCKS, x, y, yaw, scale)
    for ox, oy in region.chunks():
        mine = []
        in_chunk_knob = [k for k in knobs if ox <= k[0] < ox + CHUNK and oy <= k[1] < oy + CHUNK]

        def try_add(r, x, y, at_outcrop=False):
            kind, radius, smin, smax, size = ROCKS[r]
            scale = U(smin, smax)
            yaw = U(-40, 40) + (U(0, 360) if size == 0 else 0)
            if not (ox <= x < ox + CHUNK and oy <= y < oy + CHUNK): return None
            rad = radius * scale
            for o in mine:
                if np.hypot(x - o[1], y - o[2]) < 0.85 * (rad + ROCKS[o[0]][1] * o[4]): return None
            pt = np.array([x, y])
            if not allowed(pt)[0]: return None
            if size < 3 and reserved(pt, rad * (0.6 if size == 0 else 0.85), True)[0]: return None
            rock = (r, x, y, yaw, scale, at_outcrop)
            mine.append(rock)
            return rock

        def around(r, anchor, lo, hi):
            base = ROCKS[anchor[0]][1] * anchor[4] + ROCKS[r][1]
            for _ in range(4):
                a, d = U(0, 2 * np.pi), base + U(lo, hi)
                if try_add(r, anchor[1] + np.cos(a) * d, anchor[2] + np.sin(a) * d, True): return

        def outcrop(anchor, house):
            if rng.random() < (0.45 if house else 0.3):
                around(R_ERRATIC if rng.random() < 0.5 else R_JOINTED, anchor, 0.2, 2.6)
            for _ in range(rng.integers(2, (5 if house else 3) + 1)):
                around(pick(rng, [(R_LOAF, 2), (R_TALUS, 3), (R_LOW, 2)]), anchor, -0.2, 3.2)
            for _ in range(rng.integers(3, (8 if house else 5) + 1)):
                around(pick(rng, [(R_COBBLES, 2), (R_SPALLS, 2), (R_RUBBLE, 3)]), anchor, -0.3, 4.2)

        band = max(0.0, float(region.sample(region.band, np.array([[ox, oy]]))[0]))
        if in_chunk_knob:
            kx, ky = in_chunk_knob[0]
            anchor = try_add(R_DOME if rng.random() < 0.55 else R_SPLIT, kx, ky)
            if anchor: outcrop(anchor, True)
            else:
                anchor = try_add(R_ERRATIC if rng.random() < 0.5 else R_JOINTED, kx, ky)
                if anchor: outcrop(anchor, False)
        elif rng.random() < 0.12 + 0.4 * band:
            r = R_ERRATIC if rng.random() < 0.5 else R_JOINTED
            margin = ROCKS[r][1]
            for _ in range(14):
                anchor = try_add(r, ox + U(margin * 0.6, CHUNK - margin * 0.6), oy + U(margin * 0.6, CHUNK - margin * 0.6))
                if anchor:
                    outcrop(anchor, False)
                    break
        elif rng.random() < 0.1 + 0.2 * band:
            for _ in range(5):
                if try_add(R_ERRATIC if rng.random() < 0.5 else R_JOINTED, ox + U(2, CHUNK - 2), oy + U(2, CHUNK - 2)): break
        if rng.random() < 0.45:
            try_add(pick(rng, [(R_LOAF, 2), (R_TALUS, 1), (R_LOW, 2)]), ox + U(0, CHUNK), oy + U(0, CHUNK))
        for _ in range(rng.integers(1, 5)):
            try_add(pick(rng, [(R_COBBLES, 3), (R_SPALLS, 2), (R_RUBBLE, 3)]), ox + U(0, CHUNK), oy + U(0, CHUNK))
        rocks += mine

    # ---- underbrush (GenerateUnderbrush) --------------------------------------------------------
    plants = []                                    # (species, x, y, yaw, scale)
    for ox, oy in region.chunks():
        mine = []

        def try_plant(s, x, y):
            _, radius, smin, smax, _ = UNDERBRUSH[s]
            scale, yaw = U(smin, smax), U(0, 360)
            if not (ox <= x < ox + CHUNK and oy <= y < oy + CHUNK): return
            rad = radius * scale
            for o in mine:
                if np.hypot(x - o[1], y - o[2]) < 0.6 * (rad + UNDERBRUSH[o[0]][1] * o[4]): return
            mine.append((s, x, y, yaw, scale))

        def density(x, y):
            p = np.array([[x, y]])
            return float(np.clip(0.45 + 0.95 * region.sample(region.broad, p)[0] + 0.3 * region.sample(region.fine, p)[0], 0, 1))
        for _ in range(11):
            cx, cy = ox + U(0, CHUNK), oy + U(0, CHUNK)
            d = density(cx, cy)
            thicket, shrubs = d >= 0.64, d >= 0.32
            count = 7 + int((d - 0.64) * 40) if thicket else (3 + int(d * 9) if shrubs else int(rng.integers(4, 10)))
            spread = 4.2 if thicket else 3.4 if shrubs else 2.4
            for _ in range(count):
                if thicket:
                    s = pick(rng, [(UB_BRAMBLE, 5), (UB_BRAMBLE_LARGE, 3), (UB_HEDGE, 2), (UB_THIMBLE, 1), (UB_FERN, 1), (UB_HAZEL, 1)])
                elif shrubs:
                    s = pick(rng, [(UB_HAZEL, 2), (UB_DEER, 3), (UB_THIMBLE, 3), (UB_FERN, 4), (UB_STRAWBERRY, 1), (UB_YARROW, 1)])
                else:
                    s = pick(rng, [(UB_STRAWBERRY, 3), (UB_YARROW, 3), (UB_FERN, 1)])
                a, r = U(0, 2 * np.pi), spread * np.sqrt(rng.random())
                try_plant(s, cx + np.cos(a) * r, cy + np.sin(a) * r)
        for _ in range(30):
            x, y, roll = ox + U(0, CHUNK), oy + U(0, CHUNK), rng.random()
            d = density(x, y)
            if roll > 0.12 + 0.6 * d: continue
            if d >= 0.35:
                s = pick(rng, [(UB_HAZEL, 2), (UB_DEER, 2), (UB_THIMBLE, 2), (UB_FERN, 3), (UB_BRAMBLE, 2 if d >= 0.6 else 0)])
            else:
                s = pick(rng, [(UB_STRAWBERRY, 2), (UB_YARROW, 3), (UB_FERN, 1)])
            try_plant(s, x, y)
        plants += mine
    plants = np.array(plants)
    species = plants[:, 0].astype(int)
    low = np.array([UNDERBRUSH[s][4] for s in species])
    rad = np.array([UNDERBRUSH[s][1] for s in species]) * plants[:, 4]
    ok = allowed(plants[:, 1:3])
    ok &= ~np.where(low, reserved(plants[:, 1:3], rad * 0.5, True), reserved(plants[:, 1:3], rad * 0.7, False))
    ok &= ~(region.in_glade(plants[:, 1:3]) & ~low)
    if rocks:
        rk = np.array(rocks)
        rr = np.array([ROCKS[int(r)][1] * (0.5 if ROCKS[int(r)][4] == 0 else 0.9) for r in rk[:, 0]]) * rk[:, 4]
        near = cKDTree(rk[:, 1:3]).query_ball_point(plants[:, 1:3], r=rr.max() + rad.max() * 0.35)
        for i, hits in enumerate(near):
            if ok[i] and any(np.hypot(*(rk[j, 1:3] - plants[i, 1:3])) < rr[j] + rad[i] * 0.35 for j in hits):
                ok[i] = False
    plants = plants[ok]

    # ---- ground cover (BuildDecorations) --------------------------------------------------------
    cover = []                                     # (kind, x, y, yaw, scale)
    tries = np.arange(1200)
    for ox, oy in region.chunks():
        x = ox + rng.uniform(0, CHUNK, len(tries))
        y = oy + rng.uniform(0, CHUNK, len(tries))
        yaw = rng.uniform(0, 360, len(tries))
        grass = rng.random(len(tries)) < GRASS_SHARE
        flower_scale = rng.uniform(0.55, 0.80, len(tries))
        for t in np.flatnonzero(grass):
            cover.append((GRASS_BY_TRY[t % 16], x[t], y[t], yaw[t], 1.0, 0.20))
        for t in tries[tries % 32 == 0]:
            cover.append(((FERN_A, FERN_B, FERN_C, FERN_D)[(t // 32) % 4], x[t], y[t], yaw[t], 1.0, 0.75))
        for t in tries[tries % 64 == 17]:
            cover.append((FLOWER_A if (t // 64) % 2 == 0 else FLOWER_B, x[t], y[t], yaw[t], flower_scale[t], 0.55))
    cover = np.array(cover)
    ok = allowed(cover[:, 1:3]) & ~reserved(cover[:, 1:3], cover[:, 5], True)
    cover = cover[ok]

    # ---- scenery records -------------------------------------------------------------------------
    # The MVP's brambles blocked her until she cut them: here they're clearable nodes (after every
    # other id, so those stay put) that the game draws with the MVP bramble meshes and collision.
    bramble_kind = {UB_BRAMBLE: "BrambleThin", UB_BRAMBLE_LARGE: "BrambleThicket"}
    recs = []
    for x, y, kind, scale in trees[~felling]:
        recs.append((int(kind), x, y, U(0, 360), scale))
    for x, y, kind, scale in young:
        recs.append((int(kind), x, y, U(0, 360), scale))
    for i, (r, x, y, yaw, scale, at_outcrop) in enumerate(rocks):
        kind, _, _, _, size = ROCKS[int(r)]
        if size == 0:
            # The MVP's loose cobble, spall and rubble clusters read as the hand stones she can pick
            # up. Round an outcrop every third becomes a ledge of the same granite breaking through
            # the ground; loose clusters on the open floor are left out.
            if at_outcrop and i % 3 == 0 and clear_of_stones(np.array([[x, y]]))[0]:
                recs.append((LEDGE, x, y, yaw % 360, scale * 1.1))
            continue
        recs.append((kind, x, y, yaw % 360, scale))
    for s, x, y, yaw, scale in plants:
        if int(s) in bramble_kind:
            add_row(bramble_kind[int(s)], (x, y))
        else:
            recs.append((UNDERBRUSH[int(s)][0], x, y, yaw, scale))
    for kind, x, y, yaw, scale, _ in cover:
        recs.append((int(kind), x, y, yaw, scale))
    return rows, recs
