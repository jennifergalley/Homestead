"""Sea cave in the cove's west cliff (Jenny's card: "add a cave into the cliffs along the beach, that has a
cool little water reservoir inside ... it should be fairly sizable").

    .\\Scripts\\Blender\\New-Prop.ps1 sea_cave          # needs Scripts\\Terrain\\sea_cave_site.json

Reference: wave-cut sea caves in jointed granite cliffs (Kynance, Porthcurno, the Pembrokeshire and Scilly
caves). The sea works a vertical joint into a tall narrow cleft at the cliff foot that widens at its base;
behind it a passage follows the joint, bending, and opens into a chamber where the roof has fallen along
crossing joints, leaving a domed, blocky ceiling. Granite cave walls are clean and wave-scoured low down,
darker and wet, with a pink-mauve band of encrusting coralline algae and barnacles in the splash zone near
the mouth, a green algal film where light reaches, and pale sand and cobbles on the floor. A seep from the
roof collects in a rock basin: a still, clear freshwater pool. A narrow joint in the roof above it lets in a
thread of light (the runtime puts an emissive sky card and a spot light at its top; the slot is closed).

Sizes: mouth cleft ~5 m wide at the floor and ~8 m tall; passage ~15 m, 5 m wide, 6-7 m high; chamber ~20 x
15 m, roof 9-10 m, with a ~8 x 5 m pool (0.5 m deep in the middle, shelving to nothing at the edge) and a
0.8 m rock ledge at the back with walkable ramps (<= 21 degrees). A granite crag (stacked, rounded joint
blocks, as the estate's tors) hoods the mouth so the passage has rock over it where the cliff is shallow.

The mesh is authored in the cave's own frame (metres): origin on the cliff foot at the mouth, floor level
z = 0, +X inland along the cave, Z up. It is fitted to the real ground: the recipe reads
EstateHeightfield.r16 at the site in sea_cave_site.json, keeps the rock under the landscape only where
it can show (within a few metres of the landscape hole), and writes:

- Scripts/Terrain/sea_cave_holes.json: the landscape vertices to paint as holes (Visibility layer) where
  the ground passes through the cave (paint with Scripts/Terrain/build_landscape_holes.py).
- Source/SurvivalGame/Simulation/HomesteadSeaCave.inc: the placement, footprint, pool shore and light
  points the runtime needs (generated; do not edit by hand).

Everything is generated here: no scanned or downloaded geometry or textures (crystals come from the shared
GraniteDetail tiling maps, as the big granite rocks).
"""
import json
import math
from pathlib import Path

import numpy as np

import homestead_rocks as rocks
import homestead_sdf as sdf

NAME = "SeaCave"
DESCRIPTION = ("Granite sea cave fitted into the cove's west cliff: mouth cleft under a crag, a bending "
               "passage and a domed chamber with a rock-basin pool, a ledge and a roof fissure; Nanite LOD0.")
COLLISION = "none"          # import_props.COLLISION_OVERRIDES gives it complex-as-simple (LOD2)
TRIANGLE_BUDGET = 1800000
BAKE = {"size": 4096, "samples": 32, "repack": False, "maps": ["basecolor", "roughness", "normal", "ao", "mask"]}
BEAUTY = {"pose": (0, 0, 0), "focus": (24.0, 2.0, 2.0), "ground": "origin", "views": ["hero"],
          "eye_distance": 60.0}
NOTES = {}
REPORT = {"nanite": True}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0,
          "strength": 0.7}

ROOT = Path(__file__).resolve().parents[3]
SITE = ROOT / "Scripts" / "Terrain" / "sea_cave_site.json"
R16 = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateHeightfield.r16"
HOLES_OUT = ROOT / "Scripts" / "Terrain" / "sea_cave_holes.json"
INC_OUT = ROOT / "Source" / "SurvivalGame" / "Simulation" / "HomesteadSeaCave.inc"
SIZE, HALF = 4033, 2016

VOXEL = 0.14                 # SDF meshing (m); LOD0 is this subdivided once
SUNK = 0.15                  # the rock under the landscape sits this far below it (never z-fights)
KEEP_NEAR_HOLE_M = 3.5       # hidden rock is kept this near a hole (it can be seen through the hole's edge)
HOLE_INSIDE = -0.3          # a landscape point within this of the cave's air (or inside it) makes its quad a hole
MOUTH_FLOOR_LIFT = 0.06      # outside, the cave floor runs this far above the beach (the beach stays)
WATER_Z = 0.22               # the pool's surface (local m)
LOD_TRIS = (220000, 60000)
LOD0_TRIS = 1300000

PROPS = "/Game/SurvivalGame/Environment/Props"
# Reused set dressing (cave-local Blender m, yaw deg, scale): fallen joint blocks and spalls on the chamber
# floor, cobbles washed into the passage, ferns in the damp just inside the mouth (Balance: hart's-tongue and
# maidenhair; the estate's fern stands in), and campion at the foot of the crag outside. Seated on the floor
# (inside) or the beach (outside) by the recipe; none of them stands on the walking line or in the pool.
DRESSING = [
    (f"{PROPS}/GraniteBoulderLoaf/SM_GraniteBoulderLoaf", 20.5, -3.4, 30.0, 0.9),
    (f"{PROPS}/GraniteBoulderLow/SM_GraniteBoulderLow", 21.6, 7.4, -50.0, 1.1),
    (f"{PROPS}/GraniteSpalls/SM_GraniteSpalls", 17.6, 6.2, 10.0, 1.0),
    (f"{PROPS}/GraniteBlockTalus/SM_GraniteBlockTalus", 32.6, 1.0, 75.0, 1.0),
    (f"{PROPS}/GraniteCobbles/SM_GraniteCobbles", 2.4, -1.7, 0.0, 1.0),
    (f"{PROPS}/GraniteCobbles/SM_GraniteCobbles", 8.0, 4.0, 120.0, 0.9),
    (f"{PROPS}/GraniteRubble/SM_GraniteRubble", 12.5, 0.6, 40.0, 0.8),
    ("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a", 1.8, 2.5, 20.0, 0.8),
    ("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a", 4.4, -1.8, 140.0, 0.7),
    ("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a", 0.9, -2.4, 260.0, 0.75),
    ("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a", 6.2, 4.3, 80.0, 0.65),
    (f"{PROPS}/RedCampion/SM_RedCampion", -4.6, 4.6, 0.0, 1.0),
    (f"{PROPS}/RedCampion/SM_RedCampion", -4.1, -5.6, 120.0, 0.9),
    (f"{PROPS}/RedCampion/SM_RedCampion", -5.3, 2.7, 230.0, 1.1),
]


# ------------------------------------------------------------------ site and ground

class Site:
    """The cave frame on the estate: anchor (world m, Unreal +X north, +Y east), yaw (deg, the cave's +X in
    the world, from +X toward +Y) and floor height (m above the sea; the ground at the anchor by default).
    Blender's +Y is the cave's left; the FBX export mirrors Y, so in Unreal local Y = -Blender Y."""

    def __init__(self):
        spec = json.loads(SITE.read_text())
        self.ax, self.ay = spec["anchor_m"]
        self.yaw = spec["yaw_deg"]
        self.c, self.s = math.cos(math.radians(self.yaw)), math.sin(math.radians(self.yaw))
        raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
        self.z = (raw.astype(np.float64) - 32768.0) / 128.0          # [row = y + HALF, col = x + HALF]
        self.z0 = spec.get("floor_m")
        if self.z0 is None:
            self.z0 = float(self.world_height(np.array([self.ax]), np.array([self.ay]))[0])

    def to_world(self, xb, yb):
        return self.ax + self.c * xb + self.s * yb, self.ay + self.s * xb - self.c * yb

    def to_local(self, wx, wy):
        dx, dy = wx - self.ax, wy - self.ay
        return self.c * dx + self.s * dy, self.s * dx - self.c * dy

    def world_height(self, wx, wy):
        col, row = wx + HALF, wy + HALF
        c0, r0 = np.floor(col).astype(int), np.floor(row).astype(int)
        fc, fr = col - c0, row - r0
        z = self.z
        # The landscape's own triangulation (as HomesteadEstateTerrain: split along the quad's diagonal).
        a, b = z[r0, c0], z[r0, c0 + 1]
        c, d = z[r0 + 1, c0], z[r0 + 1, c0 + 1]
        lower = fc + fr <= 1.0
        return np.where(lower, a + fc * (b - a) + fr * (c - a), d + (1 - fc) * (c - d) + (1 - fr) * (b - d))

    def ground(self, xb, yb):
        """Ground height in the cave frame (m) at local x, y."""
        wx, wy = self.to_world(xb, yb)
        return self.world_height(wx, wy) - self.z0


# ------------------------------------------------------------------ the cave's shape

def basin_q(x, y):
    """The pool basin's shape: 0 at its middle, 1 at its rim, irregular (warped ellipse) under the fissure."""
    px, py = x - 26.0, y - 4.8
    ca, sa = math.cos(math.radians(25)), math.sin(math.radians(25))
    u, v = (ca * px + sa * py) / 4.3, (-sa * px + ca * py) / 2.7
    pts = np.c_[x / 2.2, y / 2.2, np.zeros(len(np.atleast_1d(x)))]
    wobble = 1.0 + 0.22 * sdf.fbm(pts, octaves=3, seed=41)
    return (u * u + v * v) * wobble


def floor_height(x, y, site):
    """The cave floor (local m): a sand floor rising gently inland, a low ledge at the back, the pool basin;
    outside the mouth it rides just above the beach (never above the cave's own floor: no sill to step over)."""
    x, y = np.atleast_1d(x), np.atleast_1d(y)
    base = np.clip(x / 14.0, 0.0, 1.0) * 0.45 + 0.05 * np.sin(x * 0.7 + y * 0.45) * np.clip(x / 4.0, 0, 1)
    # Chamber: a slight dish toward the pool.
    base -= 0.12 * np.exp(-(((x - 25.0) / 7.0) ** 2 + ((y - 4.0) / 6.0) ** 2))
    # The ledge: a rock shelf at the chamber's back right, 0.8 m up, ramps 2.4 m long (18-20 degrees).
    lx, ly = (x - 32.5) / 4.6, (y + 6.0) / 3.2
    r = np.sqrt(lx * lx + ly * ly)
    ledge = 0.8 * rocks.smoothstep(1.0 + 2.4 / 4.0, 1.0, r)
    # The pool basin under the fissure: deepest 0.75 m below the chamber floor, shelving gently from its rim.
    basin = -0.75 * np.clip(1.0 - basin_q(x, y), 0.0, 1.0) ** 0.8
    f = base + ledge + basin
    beach = site.ground(x, y) + MOUTH_FLOOR_LIFT
    outside = rocks.smoothstep(1.0, -2.5, x)
    return np.where(x < 1.0, (1 - outside) * f + outside * np.minimum(beach, f), f), ledge


def ledge_mask(x, y):
    lx, ly = (x - 32.5) / 4.6, (y + 6.0) / 3.2
    return rocks.smoothstep(1.3, 1.0, np.sqrt(lx * lx + ly * ly))


def _ell(c, r, yaw=0.0, pitch=0.0, roll=0.0):
    return sdf.ellipsoid(c, r, rotation=rocks.rotation(yaw, pitch, roll))


def _box(c, h, rounding, yaw=0.0, pitch=0.0, roll=0.0):
    return sdf.box(c, h, rotation=rocks.rotation(yaw, pitch, roll), rounding=rounding)


def void_node(site):
    """The cave's air: negative inside."""
    big = sdf.smooth_union([
        _ell((-2.6, 0.4, 2.5), (3.3, 3.0, 3.4)),                         # the mouth: wide low at the sand
        _ell((-1.9, 1.7, 4.4), (2.2, 1.3, 1.8), yaw=22),                 # an undercut to one side
        _ell((3.5, 0.8, 3.1), (3.7, 2.9, 3.7), yaw=8),                   # passage, bending left
        _ell((9.0, 2.2, 3.2), (3.8, 2.5, 3.7), yaw=16),
        _ell((14.5, 3.2, 3.6), (3.6, 3.2, 4.2), yaw=6),
        _ell((24.0, 2.5, 3.4), (9.5, 7.2, 5.8)),                         # chamber
        _ell((29.5, -2.8, 2.6), (6.5, 5.4, 4.6), yaw=-20),               # its right-hand lobe (the ledge)
        _ell((25.0, 3.0, 6.0), (5.6, 4.6, 4.0), yaw=20),                 # the dome
    ], k=1.4)
    # The joint the sea worked: a tall leaning slot through the mouth and along the passage roof.
    cleft = _box((1.0, -0.5, 5.2), (6.5, 1.0, 3.0), 0.45, yaw=4, roll=9)
    # A crossing joint above the pool: the fissure (closed at its top; the runtime lights it).
    fissure = _box((26.4, 4.7, 10.0), (3.0, 0.3, 1.9), 0.15, yaw=28, roll=-6)
    shape = sdf.smooth_union([big, cleft], k=0.5)
    shape = sdf.smooth_union([shape, fissure], k=0.45)

    def relief(p):
        broad = sdf.fbm(p / np.array([2.8, 2.8, 2.0]), octaves=3, seed=11)
        fine = sdf.fbm(p / 0.9, octaves=2, seed=12)
        # Joint-bounded blocks spalled from the walls and roof: stepped facets, not a smooth bore.
        big_blocks, _ = rocks.plates(p, None, seed=15, cell=3.0, thickness=0.24, coverage=0.4, width=0.3,
                                     wobble=0.3)
        small_blocks, _ = rocks.plates(p, None, seed=16, cell=1.2, thickness=0.06, coverage=0.4, width=0.12,
                                       wobble=0.3)
        above_floor = rocks.smoothstep(0.35, 1.1, p[:, 2])
        overhead = np.clip((p[:, 2] - 4.0) / 3.0, 0.0, 1.0)
        groove = joints(p, sheets=[2.6, 6.4], verticals=[(14, [12.0, 27.0]), (101, [-3.5])],
                        depth=0.14, width=0.4, seed=14)
        return (0.28 * broad + 0.08 * fine + (big_blocks * (0.6 + 0.4 * overhead) + small_blocks) * above_floor
                + groove * (p[:, 2] > 0.6))
    shape = sdf.displaced(shape, relief, 1.2)

    def floor_fn(p, band):
        f, _ = floor_height(p[:, 0], p[:, 1], site)
        return np.clip(f - p[:, 2], -band, band).astype(np.float32)
    floor = sdf.Node(floor_fn, shape.lo - 1.0, shape.hi + 1.0)
    return sdf.intersect(shape, floor, k=0.45)


def joints(p, sheets, verticals, depth, width, seed):
    """V grooves where joint planes meet the surface (m, >= 0): horizontal sheeting at ``sheets`` heights
    (wavy) and two vertical sets ``verticals`` = [(yaw_deg, [offsets])]."""
    out = np.zeros(len(p))
    wave = 0.35 * sdf.fbm(np.c_[p[:, 0] / 3.0, p[:, 1] / 3.0, np.zeros(len(p))], octaves=2, seed=seed)
    for z in sheets:
        d = np.abs(p[:, 2] - (z + wave))
        out = np.maximum(out, depth * np.clip(1.0 - d / width, 0.0, 1.0) ** 1.5)
    for yaw, offsets in verticals:
        nrm = np.array([math.cos(math.radians(yaw)), math.sin(math.radians(yaw))])
        along = p[:, :2] @ nrm + 0.25 * sdf.fbm(p / np.array([2.0, 2.0, 4.0]), octaves=2, seed=seed + 1)
        for o in offsets:
            d = np.abs(along - o)
            out = np.maximum(out, 0.8 * depth * np.clip(1.0 - d / width, 0.0, 1.0) ** 1.5)
    return out


def crag_node():
    """The granite blocks hooding the mouth (positive outside rock): a small tor of jointed, rounded blocks,
    split by horizontal sheeting and two vertical joint sets, as the estate's granite outcrops."""
    blocks = sdf.smooth_union([
        _box((0.6, 0.4, 4.6), (4.2, 6.8, 6.0), 1.6, yaw=6, pitch=-5, roll=3),   # the main block over the mouth
        _box((6.6, -0.6, 6.6), (4.4, 6.2, 5.6), 1.8, yaw=-9, pitch=4),          # behind it, up the slope
        _box((1.2, 7.6, 2.6), (3.4, 2.6, 3.2), 1.3, yaw=27, roll=-5),           # left buttress
        _box((0.6, -7.8, 2.2), (2.8, 2.6, 2.8), 1.2, yaw=-19, pitch=6),         # right buttress
        _box((-0.6, -3.0, 10.7), (2.6, 3.1, 1.35), 0.9, yaw=33, roll=7),        # perched blocks on top
        _box((3.2, 3.6, 10.9), (2.0, 2.6, 1.2), 0.8, yaw=-14, roll=-5),
        _box((-3.4, 6.8, 0.8), (1.6, 1.4, 1.2), 0.7, yaw=40, roll=9),           # fallen blocks at the foot
        _box((-3.0, -7.4, 0.6), (1.3, 1.5, 1.0), 0.6, yaw=-25, pitch=8),
    ], k=0.8)

    def relief(p):
        smooth = 0.2 * sdf.fbm(p / 3.2, octaves=3, seed=21) + 0.05 * sdf.fbm(p / 0.55, octaves=2, seed=22)
        groove = joints(p, sheets=[2.3, 5.9, 9.2], verticals=[(9, [-3.1]), (101, [4.6])],
                        depth=0.3, width=0.55, seed=23)
        spalls, _ = rocks.plates(p, None, seed=24, cell=2.2, thickness=0.22, coverage=0.4, width=0.1, wobble=0.3)
        return smooth - groove - spalls
    return sdf.displaced(blocks, relief, 0.6)


def terrain_node(site, lo, hi):
    """Solid ground below the landscape (sunk SUNK m): positive above."""
    def fn(p, band):
        h = site.ground(p[:, 0], p[:, 1])
        e = 0.5
        gx = (site.ground(p[:, 0] + e, p[:, 1]) - site.ground(p[:, 0] - e, p[:, 1])) / (2 * e)
        gy = (site.ground(p[:, 0], p[:, 1] + e) - site.ground(p[:, 0], p[:, 1] - e)) / (2 * e)
        d = (p[:, 2] - (h - SUNK)) / np.sqrt(1.0 + gx * gx + gy * gy)
        return np.clip(d, -band, band).astype(np.float32)
    return sdf.Node(fn, lo, hi)


def shell_node():
    return _box((16.0, 0.0, 6.0), (24.0, 15.5, 12.5), 2.0)


def rock_node(site):
    void = void_node(site)
    crag = crag_node()
    shell = shell_node()
    ground = terrain_node(site, shell.lo, shell.hi)
    mass = sdf.union([crag, ground])
    rock = sdf.intersect(shell, mass)
    return sdf.subtract(rock, void), void, crag, ground, shell


def _eval(node, points, band=2.0):
    out = np.empty(len(points), dtype=np.float32)
    for i in range(0, len(points), 400000):
        out[i:i + 400000] = node(points[i:i + 400000], band)
    return out


# ------------------------------------------------------------------ fitting outputs

def hole_vertices(site, void):
    """Landscape quads the cave passes through, as the vertices to paint: each quad's four corners."""
    lo, hi = void.lo, void.hi
    corners = np.array([[x, y] for x in (lo[0], hi[0]) for y in (lo[1], hi[1])])
    wx, wy = site.to_world(corners[:, 0], corners[:, 1])
    cols = np.arange(int(np.floor(wx.min())) - 1, int(np.ceil(wx.max())) + 2)
    rows = np.arange(int(np.floor(wy.min())) - 1, int(np.ceil(wy.max())) + 2)
    sub = np.linspace(0.0, 1.0, 6)
    holes = set()
    for qx in cols:
        gx = (qx + sub[:, None] + 0 * sub[None, :]).ravel()
        for qy in rows:
            gy = (qy + 0 * sub[:, None] + sub[None, :]).ravel()
            h = site.world_height(gx, gy) - site.z0
            lx, ly = site.to_local(gx, gy)
            pts = np.c_[lx, ly, h]
            inside = (pts[:, 0] > lo[0]) & (pts[:, 0] < hi[0]) & (pts[:, 1] > lo[1]) & (pts[:, 1] < hi[1]) \
                & (pts[:, 2] > lo[2]) & (pts[:, 2] < hi[2])
            if not inside.any():
                continue
            # Within the margin of the cave's air (so no landscape pokes through its walls), but not where the
            # ground lies under the cave's floor (the beach before the mouth stays).
            sub_pts = pts[inside]
            floor, _ = floor_height(sub_pts[:, 0], sub_pts[:, 1], site)
            near = _eval(void, sub_pts) < -HOLE_INSIDE
            if (near & (sub_pts[:, 2] > floor + 0.02)).any():
                holes.add((int(qx), int(qy)))
    verts = set()
    for qx, qy in holes:
        for dx in (0, 1):
            for dy in (0, 1):
                verts.add((qx + dx + HALF, qy + dy + HALF))    # (col, row) in the r16 / landscape grid
    return sorted(holes), sorted(verts)


def pool_shore(site):
    """The pool's waterline (local m), marched out from the basin's centre."""
    centre = np.array([26.0, 4.8])
    shore = []
    for a in np.radians(np.arange(0.0, 360.0, 6.0)):
        d = np.array([math.cos(a), math.sin(a)])
        r = 0.0
        while r < 8.0:
            p = centre + d * (r + 0.02)
            f, _ = floor_height(np.array([p[0]]), np.array([p[1]]), site)
            if f[0] > WATER_Z:
                break
            r += 0.02
        shore.append(centre + d * r)
    return np.array(shore)


def footprint(void):
    """The cave's floor outline (local m): the void's cross-section 1 m above the floor, as a hull."""
    xs = np.arange(void.lo[0], void.hi[0], 0.5)
    ys = np.arange(void.lo[1], void.hi[1], 0.5)
    gx, gy = np.meshgrid(xs, ys, indexing="ij")
    pts = np.c_[gx.ravel(), gy.ravel(), np.full(gx.size, 1.0)]
    inside = pts[_eval(void, pts) < 0.0][:, :2]
    # Convex hull (monotone chain).
    pts = sorted(map(tuple, np.round(inside, 3)))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return np.array(lower[:-1] + upper[:-1])


def fissure_top(void):
    """The fissure's top over the pool (local m): marched up through the slot until it closes."""
    x, y = 26.4, 4.7
    best = None
    for dx, dy in ((0, 0), (0.8, 0.4), (-0.8, -0.4), (1.6, 0.85), (-1.6, -0.85)):
        zs = np.arange(8.0, 14.0, 0.02)
        pts = np.c_[np.full(len(zs), x + dx), np.full(len(zs), y + dy), zs]
        d = _eval(void, pts)
        inside = np.flatnonzero(d < 0)
        if len(inside):
            top = zs[inside.max()]
            if best is None or top > best[2]:
                best = (x + dx, y + dy, float(top))
    return best or (x, y, 10.0)


def write_fit(site=None):
    """The fitting outputs only (holes, the runtime table), without meshing: python-callable from Blender."""
    site = site or Site()
    _, void, _, _, _ = rock_node(site)
    holes, verts = hole_vertices(site, void)
    write_outputs(site, void, holes, verts, pool_shore(site), footprint(void))
    return site, void, holes


def write_outputs(site, void, holes, verts, shore, hull):
    # Paint over the holes' box (padded), and over the last fit's box too, so a refit clears the old hole.
    cols, rows = [c for c, _ in verts], [r for _, r in verts]
    rect = [min(cols) - 8, min(rows) - 8, max(cols) + 8, max(rows) + 8]
    if HOLES_OUT.exists():
        old = json.loads(HOLES_OUT.read_text()).get("paint_rect")
        if old:
            rect = [min(rect[0], old[0]), min(rect[1], old[1]), max(rect[2], old[2]), max(rect[3], old[3])]
    HOLES_OUT.write_text(json.dumps({
        "generated_by": "Scripts/Blender/Recipes/sea_cave.py (do not edit by hand)",
        "site": json.loads(SITE.read_text()), "floor_m": site.z0, "paint_rect": rect,
        "quads": holes, "vertices": verts}, indent=1) + "\n")

    def unreal_cm(points):          # cave-local Unreal axes (Y mirrored), centimetres
        return [(round(float(p[0]) * 100.0, 1), round(-float(p[1]) * 100.0, 1)) for p in points]
    lines = ["// Generated by Scripts/Blender/Recipes/sea_cave.py from Scripts/Terrain/sea_cave_site.json and the",
             "// estate heightfield; do not edit by hand. Cave-local centimetres: +X inland, +Y the cave's right,",
             "// Z from the floor at the mouth.",
             f"SEA_CAVE_ANCHOR({site.ax * 100.0:.1f}, {site.ay * 100.0:.1f}, {site.z0 * 100.0:.1f}, {site.yaw:.2f})",
             f"SEA_CAVE_WATER_Z({WATER_Z * 100.0:.1f})",
             f"SEA_CAVE_FLOOR_MIN({-75.0:.1f})",
             f"SEA_CAVE_ROOF_MAX({void.hi[2] * 100.0:.1f})"]
    fx, fy, fz = fissure_top(void)
    lines.append(f"SEA_CAVE_LIGHT({fx * 100.0:.1f}, {-fy * 100.0:.1f}, {fz * 100.0:.1f})")
    lines += [f"SEA_CAVE_FOOTPRINT({x:.1f}, {y:.1f})" for x, y in unreal_cm(hull)]
    lines += [f"SEA_CAVE_SHORE({x:.1f}, {y:.1f})" for x, y in unreal_cm(shore)]
    for path, xb, yb, yaw, scale in DRESSING:
        f, _ = floor_height(np.array([xb]), np.array([yb]), site)
        z = float(f[0]) if xb > 0.5 else float(site.ground(np.array([xb]), np.array([yb]))[0])
        name = path.rsplit("/", 1)[1]
        # Unreal local: Y mirrored, so yaw mirrors too.
        lines.append(f'SEA_CAVE_PROP("{path}.{name}", {xb * 100.0:.1f}, {-yb * 100.0:.1f}, {z * 100.0:.1f}, '
                     f'{-yaw:.1f}, {scale:.2f})')
    INC_OUT.write_text("\n".join(lines) + "\n")


# ------------------------------------------------------------------ meshing and surface

def _cull(obj, site, void, holes):
    """Delete the rock surface no one can see: under the landscape (beyond the hole margin) and the shell."""
    import bmesh
    import bpy
    data = obj.data
    p = rocks.coords(obj)
    faces = np.array([poly.vertices[:] if len(poly.vertices) == 4 else (*poly.vertices[:], poly.vertices[0])
                      for poly in data.polygons])
    cent = p[faces].mean(axis=1)
    d_void = _eval(void, cent)
    ground = site.ground(cent[:, 0], cent[:, 1])
    above = cent[:, 2] - ground
    wx, wy = site.to_world(cent[:, 0], cent[:, 1])
    if holes:
        centres = np.array(holes, dtype=np.float64) + 0.5
        near = np.full(len(cent), 1e9)
        for i in range(0, len(cent), 100000):
            d = np.hypot(wx[i:i + 100000, None] - centres[None, :, 0], wy[i:i + 100000, None] - centres[None, :, 1])
            near[i:i + 100000] = d.min(axis=1)
    else:
        near = np.full(len(cent), 1e9)
    shell = _eval(shell_node(), cent)
    keep = (np.abs(d_void) < 0.3) | (above > -0.08) | (near < KEEP_NEAR_HOLE_M)
    keep &= shell < -0.2
    bm = bmesh.new()
    bm.from_mesh(data)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[f for f, k in zip(bm.faces, keep) if not k], context="FACES")
    # Drop tiny islands left behind (cull fringes).
    bm.faces.ensure_lookup_table()
    seen, small = set(), []
    for f in bm.faces:
        if f.index in seen:
            continue
        island, stack = [], [f]
        seen.add(f.index)
        while stack:
            g = stack.pop()
            island.append(g)
            for e in g.edges:
                for o in e.link_faces:
                    if o.index not in seen:
                        seen.add(o.index)
                        stack.append(o)
        if len(island) < 200:
            small.extend(island)
    bmesh.ops.delete(bm, geom=small, context="FACES")
    bm.to_mesh(data)
    bm.free()
    data.update()
    print(f"sea cave: kept {int(keep.sum())} of {len(keep)} faces; dropped {len(small)} in small islands")


def attributes(p, n, site, void):
    """Material masks per vertex (0..1): sand floor, wetness, coralline crust, green algae, and 'fresh'
    (scoured, unweathered rock) deeper in."""
    x, y, z = p[:, 0], p[:, 1], p[:, 2]
    f, _ = floor_height(x, y, site)
    up = rocks.smoothstep(0.55, 0.9, n[:, 2])
    above = z - f
    interior = rocks.smoothstep(-2.0, 3.0, x) * (np.abs(_eval(void, p)) < 0.6)
    ledge = ledge_mask(x, y)
    pool = rocks.smoothstep(WATER_Z + 0.08, WATER_Z - 0.05, z) * rocks.smoothstep(14.0, 18.0, x)
    noise = sdf.fbm(p / 0.8, octaves=3, seed=31)
    # A lip of bare wet rock round the pool, broken where sand has drifted over it.
    q = basin_q(x, y)
    rim = rocks.smoothstep(1.55, 1.05, q) * (1.0 - pool) * rocks.smoothstep(0.42, 0.58, noise * 0.5 + 0.5)
    sand = up * rocks.smoothstep(0.3, 0.06, above) * (1.0 - ledge) * (1.0 - pool) * (1.0 - rim) \
        * rocks.smoothstep(-6.0, -3.5, x)
    seep = rocks.smoothstep(0.1, 0.45, sdf.fbm(p * np.array([1.4, 1.4, 0.12]), octaves=3, seed=32))
    wet = interior * (0.35 + 0.45 * rocks.smoothstep(1.5, 0.2, above) + 0.4 * seep)
    wet = np.maximum(wet, np.maximum(pool, 0.85 * rim))
    wet = np.maximum(wet, 0.25 * rocks.smoothstep(2.0, 0.0, above) * (1 - interior))
    # Coralline crust and barnacles: the splash zone just inside the mouth, in patches, never a band.
    mouthward = rocks.smoothstep(10.0, 2.0, x) * rocks.smoothstep(-3.5, -1.0, x)
    crust = mouthward * rocks.smoothstep(0.08, 0.25, above) * rocks.smoothstep(1.6, 0.8, above + 0.4 * noise)
    crust *= (1.0 - up) * rocks.smoothstep(0.4, 0.58, sdf.fbm(p / 0.45, octaves=3, seed=33) * 0.5 + 0.5)
    lit_mouth = rocks.smoothstep(14.0, 4.0, x) * rocks.smoothstep(1.0, 2.0, above) * rocks.smoothstep(6.0, 3.0, above)
    under_fissure = np.exp(-(((x - 26.2) / 3.5) ** 2 + ((y - 4.7) / 3.0) ** 2)) * rocks.smoothstep(0.1, 0.6, above)
    algae = np.clip(np.maximum(lit_mouth * 0.8, under_fissure * 0.9) * rocks.smoothstep(0.35, 0.6, noise * 0.5 + 0.5), 0, 1)
    algae *= interior
    fresh = np.clip(interior * (0.55 + 0.3 * rocks.smoothstep(1.5, 0.0, above)), 0, 0.9)
    return {"sand": sand, "wet": np.clip(wet, 0, 1), "crust": np.clip(crust, 0, 1), "algae": algae,
            "fresh": fresh}


def build(kit):
    import bpy
    site = Site()
    node, void, crag, ground, shell = rock_node(site)
    print(f"sea cave: site ({site.ax:.1f}, {site.ay:.1f}) m, yaw {site.yaw:.1f}, floor {site.z0:.2f} m")
    _, _, holes = write_fit(site)
    print(f"sea cave: {len(holes)} landscape quads become the hole")

    mat = kit.mats.granite("M_SeaCave", grains=False, grain=0.004, scale=6.0, lichen=0.9, moss=0.15,
                           iron=0.25, streaks=0.7, soil=0.0, enclaves=0.5, film=0.9, patina=0.8, seed=7.0,
                           cave=True)
    obj = sdf.mesh("SM_SeaCave", node, voxel=VOXEL, material=mat)
    _cull(obj, site, void, holes)
    # The SDF surface faces out of the rock; the cave is seen from inside the void, which is the rock's
    # outside, so the normals already face the viewer everywhere.
    # Unwrap a heavily smoothed copy (the spalled relief would shatter the islands), then put the relief back.
    rest = rocks.coords(obj)
    rocks.set_coords(obj, rocks._smooth_coords(obj, iterations=80, factor=0.7))
    rocks.unwrap(obj, angle=60.0, method="ANGLE_BASED")
    rocks.set_coords(obj, rest)
    kit.subdivide(obj, levels=1)

    def detail(points, normal):
        grain = rocks.relief(points, normal, 44, [(1.4, 0.05), (0.35, 0.014), (0.07, 0.003)])
        attrs = attributes(points, normal, site, void)
        # Sand lies smooth; scoured rock low down is smoother than the roof.
        grain *= 1.0 - 0.85 * attrs["sand"]
        return grain, attrs
    rocks.displace(obj, detail)
    kit.tag_coords(obj.data)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
    meshes = [obj]
    total = rocks.triangles(obj)
    for index, target in enumerate(LOD_TRIS, start=1):
        meshes.append(kit.finalize(rocks.lod(kit, obj, f"SM_SeaCave_LOD{index}", min(1.0, target / total)),
                                   pivot=None, unwrap=False, reshade=True, smooth_angle=180.0))
    NOTES.update({
        "frame": "Origin on the cliff foot at the mouth, floor level; +X inland. Place with "
                 "HomesteadSeaCave.inc's anchor and yaw (the runtime does).",
        "site": json.loads(SITE.read_text()), "floor_m": round(site.z0, 3),
        "holes": f"{len(holes)} quads; paint Scripts/Terrain/sea_cave_holes.json with build_landscape_holes.py",
        "collision": "Complex-as-simple on LOD2: she walks the floor, the ramps and the pool's edge.",
        "material": "Macro bake + GraniteDetail tiling layer (as GraniteDome); cave masks in the bake.",
        "detail_textures": [f"{DETAIL['folder']}/T_GraniteDetail_{role}.png"
                            for role in ("basecolor", "normal", "roughness", "height")],
    })
    return meshes


def after_bake(kit, obj):
    kit.layer_detail(obj, kit.ROOT / DETAIL["folder"], DETAIL["stem"], tile=DETAIL["tile_m"],
                     strength=DETAIL["strength"],
                     mask=kit.ROOT / "Assets" / "Props" / NAME / "Textures" / f"T_{NAME}_mask.png")
