"""Granite shore boulder with its two gatherable produce variants (mussels, laver) and held props.

Real-object research (written before modeling):
- Shore boulders on an English granite coast are coarse, pale-grey granite with large
  K-feldspar megacrysts, rounded smooth by the sea (few sharp joints), lichen only on their
  dry tops (black tar lichen and grey crusts), bare and darker below the tide line.
  A low boulder she stands beside: ~90 x 70 cm, ~35 cm above the sand, bedded ~5 cm into it.
- The seaward foot stays wet: darker, glossier rock with a thin olive algal film; acorn
  barnacles (Semibalanus balanoides, 5-10 mm white-grey cones with a dark operculum slit)
  crust it in patches; common limpets (Patella vulgata, 3-4.5 cm, flatter low on the shore,
  ~0.35-0.4 L tall) clamp onto it with algae on their shells.
- Blue mussels (Mytilus edulis) grow in clumps on rock shoulders: 5-7 cm adults packed
  together, anterior (umbo) ends down, posterior ends splayed up and out, tied to the rock
  and each other by golden-brown byssus threads, glossy blue-black when wet, a few barnacled,
  with grit and scraps of sea lettuce caught among them.
- Laver (Porphyra umbilicalis): fronds 10-20 cm across, irregular and lobed from a tiny
  central holdfast, a membrane one or two cells thick. At low tide they collapse in glossy,
  crinkled, purple-brown to olive films plastered over the rock top and down its sides.
  A handful lifted in a pinch hangs as limp, folded, translucent sheets.

Meshes (all authored in one frame; Blender +Y = Unreal -Y):
- SM_ShoreRock (+ _LOD1, _LOD2): pivot at the sand line (z = 0) under the rock's bounds centre;
  sink_depth recorded in report notes. Seaward = Blender -Y (Unreal +Y): wet band, barnacles
  and four limpets on that lower flank. Collision: convex hull.
- SM_MusselClump (+ _LOD1) and SM_LaverFronds (+ _LOD1): produce in the SAME local frame and
  origin as SM_ShoreRock (joined with pivot=None, never re-pivoted), so placing them at the rock
  actor's origin and rotation puts them exactly on it: produceOffsetCm = [0, 0, 0].
- SM_MusselHeld: three mussels hanging by their gathered byssus tuft. PIVOT = the pinch point at
  the tuft; +Z up out of her pinch, the mussels hang in -Z (crop-produce convention,
  HomesteadCharacterGather.cpp MakeFromZX(-Fingers, Across)).
- SM_LaverHeld: a small handful of laver; PIVOT = the top pinch, fronds hang in -Z.

Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import os

import numpy as np
from mathutils import Matrix, Vector

import homestead_rocks as rocks
import homestead_shore as shore

NAME = "ShoreRock"
DESCRIPTION = ("Low sea-worn granite shore boulder (~90 x 70 x 35 cm) with wet band, barnacles and limpets; "
               "mussel-clump and laver produce variants in the same frame; held mussels and laver.")
COLLISION = "convex"
TRIANGLE_BUDGET = 8000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_SHORE_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 64, "repack": False}
BAKE_MESHES = {"SM_MusselHeld": {"size": 512 if DRAFT else 1024}, "SM_LaverHeld": {"size": 512 if DRAFT else 1024},
               "*": True}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "focus": (0.05, -0.33, 0.1), "views": ["hero", "detail"],
          "meshes": {
              "SM_MusselClump": {"ground": "lowest", "focus": None, "detail_distance": 0.35},
              "SM_LaverFronds": {"ground": "lowest", "focus": None, "detail_distance": 0.4},
              "SM_MusselHeld": {"pose": (90, 0, 0), "ground": "lowest", "focus": None, "detail_distance": 0.2},
              "SM_LaverHeld": {"pose": (0, 0, 0), "ground": "lowest", "focus": None, "detail_distance": 0.25},
          }}
REPORT = {"produce": {}, "held": {}}
NOTES = {}

ROCK_SEED = 4117
ABOVE_GROUND = 0.35
SEAWARD = np.array([0.0, -1.0, 0.0])


# ------------------------------------------------------------------ rock

def rock_mesh(kit):
    granite = kit.mats.granite("M_ShoreGranite", grain=0.0024, scale=0.9, patina=0.85, lichen=0.4, moss=0.0,
                               iron=0.25, streaks=0.25, soil=0.0, enclaves=0.45, megacrysts=0.8, film=0.9,
                               spots=2.2, seed=21.0)
    shore.shore_overlay(granite, seed=3.0)
    field = rocks.boulder(radii=(0.52, 0.42, 0.32), power=2.7, lumps=0.07, lump_scale=1.3, seed=ROCK_SEED,
                          joints=[((0.1, 0.18, 1.0), 0.13, 0.08),          # broad, tilted, sea-worn top
                                  ((0.0, 0.0, -1.0), 0.22, 0.05),
                                  ((0.2, -1.0, 0.45), 0.29, 0.09),          # seaward face
                                  ((1.0, 0.3, 0.15), 0.41, 0.11),
                                  ((-0.95, 0.45, 0.3), 0.38, 0.12),
                                  ((-0.5, -0.8, 0.1), 0.40, 0.10)])
    solid = rocks.solid("Shore", field, subdivisions=7, material=granite, r_max=1.2)
    top = rocks.coords(solid)[:, 2].max()
    ground_z = top - ABOVE_GROUND
    noise = rocks.Noise(ROCK_SEED + 3)

    def detail(points, normal):
        h = points[:, 2] - ground_z
        sea = np.clip(0.55 * np.clip(normal @ SEAWARD, 0, 1) + 0.6 * shore.smooth(0.05, -0.28, points[:, 1]), 0, 1)
        tide = 0.06 + 0.16 * sea + 0.03 * noise.fbm(points * 5.0, 3)
        wet = shore.smooth(tide + 0.02, tide - 0.03, h)
        barn = sea ** 1.2 * shore.smooth(tide - 0.0, tide - 0.07, h) * shore.smooth(0.0, 0.03, h)
        barn *= np.clip(0.55 + 0.9 * noise.fbm(points * 9.0 + 4.0, 3), 0, 1)
        sand = shore.smooth(0.04, 0.0, h + 0.01 * noise.fbm(points * 20.0, 2))
        grain = rocks.relief(points, normal, ROCK_SEED + 7, [(0.3, 0.008), (0.08, 0.002), (0.018, 0.0004)])
        return grain, {"wet": wet, "barnacle": np.clip(barn, 0, 1), "sand": sand}

    (rock,), sink = rocks.finish(kit, [solid], "SM_ShoreRock", 5800, (), ground_z=ground_z, detail=detail)
    return rock, sink


def snap_to(tree, obj, normal, below=0.0016, lift=0.0003):
    """Press the base (local z < ``below`` before placement) of a placed shell onto the rock."""
    P = shore.coords(obj)
    base = shore.get_attr(obj.data, "base_local")
    n = Vector(normal)
    for i in np.flatnonzero(base > 0.5):
        hit = tree.ray_cast(Vector(P[i]) + n * 0.012, -n, 0.05)
        if hit[0] is not None:
            P[i] = np.array(hit[0] + n * lift)
    shore.set_coords(obj, P)


def rock_limpets(kit, tree):
    mat = shore.limpet_mat("M_RockLimpet", seed=5.5, algae=0.55)
    rng = np.random.default_rng(ROCK_SEED + 11)
    spots = [(-0.17, 0.10, 0.052), (0.07, 0.08, 0.048), (0.2, 0.13, 0.044), (-0.05, 0.16, 0.04)]
    out = []
    for i, (x, z, size) in enumerate(spots):
        hit = tree.ray_cast(Vector((x, -1.0, z)), Vector((0, 1, 0)))
        if hit[0] is None:
            continue
        obj = shore.limpet(f"RockLimpet{i}", size, mat, seed=40 + i, nu=48, rows=4, base_only=True,
                           height=0.38 * size)
        P = shore.coords(obj)
        shore.set_attr(obj.data, "base_local", (P[:, 2] < 0.0006).astype(np.float32))
        tangent = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0.3))
        shore.transform(obj, Matrix.Translation(hit[0] - hit[1] * 0.0004) @ shore.frame(hit[1], tangent))
        snap_to(tree, obj, hit[1])
        out.append(obj)
    return out


# ------------------------------------------------------------------ mussel clump

def capsule_distance(a0, a1, b0, b1):
    best = 1e9
    for s in np.linspace(0, 1, 6):
        p = a0 + (a1 - a0) * s
        ab = b1 - b0
        t = np.clip((p - b0) @ ab / max(ab @ ab, 1e-12), 0, 1)
        best = min(best, np.linalg.norm(p - (b0 + ab * t)))
    return best


def mussel_clump(kit, rock, tree):
    mat = shore.mussel_mat("M_LiveMussel", seed=2.0, worn=0.7, barnacles=0.25, wet=1.0, brown=0.5)
    byssus_mat = shore.byssus_mat("M_Byssus")
    ulva = shore.ulva_mat("M_Ulva", seed=1.0)
    rng = np.random.default_rng(ROCK_SEED + 21)
    centre_hit = tree.ray_cast(Vector((0.05, -0.15, 2.0)), Vector((0, 0, -1)))
    S0, n0 = np.array(centre_hit[0]), np.array(centre_hit[1])
    t1 = np.cross(n0, [0, 0, 1.0]) if abs(n0[2]) < 0.99 else np.array([1.0, 0, 0])
    t1 /= np.linalg.norm(t1)
    t2 = np.cross(n0, t1)
    placed, parts, threads = [], [], []
    target = 26
    attempts = 0
    while len(placed) < target and attempts < 800:
        attempts += 1
        rho = 0.08 * math.sqrt(rng.uniform(0.0, 1.0))
        phi = rng.uniform(0, 2 * math.pi)
        guess = S0 + (t1 * math.cos(phi) + t2 * math.sin(phi)) * rho
        hit = tree.ray_cast(Vector(guess + n0 * 0.3), Vector(-n0), 0.6)
        if hit[0] is None:
            continue
        Q, n = np.array(hit[0]), np.array(hit[1])
        L = rng.uniform(0.05, 0.068)
        outward = guess - S0
        outward = outward - n * (outward @ n)
        if np.linalg.norm(outward) < 1e-4:
            outward = t1 * math.cos(phi) + t2 * math.sin(phi)
        outward /= np.linalg.norm(outward)
        # Beds lean one way (downslope, seaward) with scatter, not a rosette.
        lean = np.array([0.25, -1.0, -0.3])
        lean = lean - n * (lean @ n)
        lean /= np.linalg.norm(lean)
        swing = rng.normal(0.0, 0.55)
        tang = lean * math.cos(swing) + np.cross(n, lean) * math.sin(swing)
        ok = False
        for lift in np.arange(0.0, 0.014, 0.0035):
            elev = math.radians(rng.uniform(22, 58))
            d = tang * math.cos(elev) + n * math.sin(elev)
            tip = Q + n * (0.002 + lift)
            a0, a1 = tip + d * 0.15 * L, tip + d * 0.85 * L
            clear = all(capsule_distance(a0, a1, b0, b1) > 0.68 * (0.17 * L + r) for b0, b1, r in placed)
            body_ok = not any(shore.inside(tree, tip + d * L * f)[0] for f in (0.3, 0.55, 0.8))
            if clear and body_ok:
                ok = True
                break
        if not ok:
            continue
        index = len(placed)
        placed.append((a0, a1, 0.17 * L))
        dorsal = n - d * (n @ d)
        dorsal /= np.linalg.norm(dorsal)
        roll = rng.uniform(-0.6, 0.6)
        side = np.cross(d, dorsal)
        dorsal = dorsal * math.cos(roll) + side * math.sin(roll)
        side = np.cross(d, dorsal)
        obj = shore.mussel(f"Mussel{index}", L, mat, seed=100 + index, sides=12, rows=8)
        pco = shore.get_attr(obj.data, "pcoord", 3)
        pco[:, :2] += rng.uniform(-0.4, 0.4, 2)
        kit.tag_coords(obj.data, [tuple(c) for c in pco])
        basis = Matrix((tuple(d), tuple(dorsal), tuple(side))).transposed().to_4x4()
        shore.transform(obj, Matrix.Translation(Vector(tip)) @ basis)
        parts.append(obj)
        if rng.uniform() < 0.75:
            gape = Matrix.Translation(Vector(tip)) @ basis @ Vector(shore.mussel_point(L, rng.uniform(0.2, 0.35)))
            for k in range(rng.integers(1, 3)):
                off = (t1 * rng.uniform(-1, 1) + t2 * rng.uniform(-1, 1)) * rng.uniform(0.008, 0.02)
                end = tree.ray_cast(Vector(np.array(gape) + off + n * 0.02), Vector(-n), 0.08)
                if end[0] is None:
                    continue
                g0, g1 = np.array(gape), np.array(end[0])
                mid = (g0 + g1) / 2 - n * 0.002
                pts = [tuple(g0), tuple((g0 + mid) / 2), tuple(mid), tuple((mid + g1) / 2), tuple(g1)]
                threads.append(kit.tube(f"Byssus{index}_{k}", pts, radius=0.00025, sides=4,
                                        material=byssus_mat, cap=False))
    # Grit: sand caught where mussels touch the rock.
    for obj in parts:
        P = shore.coords(obj)
        sand = np.zeros(len(P))
        for i, p in enumerate(P):
            loc, nrm, _, dist = tree.find_nearest(Vector(p), 0.02)
            if loc is not None:
                sand[i] = 0.75 * shore.smooth(0.009, 0.0, dist)
        shore.set_attr(obj.data, "sand", sand)
    # A few scraps of sea lettuce caught in the clump.
    clump_tree, bm = shore.bvh_of([rock] + parts)
    weed = []
    for k in range(3):
        frond = shore.frond(f"Ulva{k}", rng.uniform(0.018, 0.028), ulva, seed=500 + k, sectors=10, rings=3)
        phi = rng.uniform(0, 2 * math.pi)
        centre = S0 + (t1 * math.cos(phi) + t2 * math.sin(phi)) * rng.uniform(0.02, 0.08)
        land = clump_tree.ray_cast(Vector(centre + n0 * 0.2), Vector(-n0), 0.5)
        if land[0] is None:
            continue
        centre = np.array(land[0])
        start = shore.coords(frond) @ np.array(shore.frame(n0, t1).to_3x3()).T + centre + n0 * 0.006
        x = shore.drape(frond, clump_tree, start, pinned=[0], pin_to=(centre + n0 * 0.0015)[None], steps=150,
                        offset=0.0008, friction=0.1)
        shore.set_coords(frond, x)
        shore.two_sided(frond, gap=0.0002)
        weed.append(frond)
    bm.free()
    clump = kit.join(parts + threads + weed, "SM_MusselClump", pivot=None, unwrap=False, reshade=True,
                     smooth_angle=70)
    kit.pack_uvs(clump, margin=0.003)
    lod = kit.finalize(rocks.lod(kit, clump, "SM_MusselClump_LOD1", 0.45), pivot=None, unwrap=False,
                       reshade=True, smooth_angle=70)
    REPORT["produce"]["SM_MusselClump"] = {
        "count": len(parts), "byssus_threads": len(threads), "produceOffsetCm": [0.0, 0.0, 0.0],
        "frame": "Same local frame and origin as SM_ShoreRock (not re-pivoted); place at the rock's transform."}
    return [clump, lod]


# ------------------------------------------------------------------ laver

# (x, y) of each holdfast seen from above, frond radius (m), downhill slide (0..1)
LAVER = [(0.02, 0.03, 0.12, 0.05), (-0.12, -0.10, 0.12, 0.4), (0.12, -0.12, 0.11, 0.45),
         (0.0, -0.20, 0.12, 0.5), (-0.15, 0.08, 0.10, 0.1)]


def laver_fronds(kit, rock, tree):
    """Fronds laid one over another from the top down the seaward side: each sheet is wrapped
    over the rock (and the fronds already laid) by a geodesic polar map, then buckled into the
    soft, crowded folds of a collapsed wet membrane, its margin lifting a little."""
    mat = shore.laver_mat("M_Laver", seed=2.0, wet=1.0)
    rng = np.random.default_rng(ROCK_SEED + 31)
    noise = rocks.Noise(ROCK_SEED + 33)
    laid = []
    for k, (x, y, radius, slide) in enumerate(LAVER):
        surface, bm = shore.bvh_of([rock] + laid)
        hit = surface.ray_cast(Vector((x, y, 2.0)), Vector((0, 0, -1)))
        if hit[0] is None:
            bm.free()
            continue
        frond = shore.frond(f"Laver{k}", radius, mat, seed=700 + k, sectors=38, rings=10,
                            stretch=rng.uniform(0.75, 1.25), ragged=1.0)
        flat = shore.get_attr(frond.data, "pcoord", 3)
        kit.tag_coords(frond.data, [tuple(c) for c in flat + np.array([k * 0.31, k * 0.17, 0.0])])
        P, N = shore.geodesic_lay(frond, surface, hit[0], hit[1], spin=rng.uniform(0, 2 * math.pi),
                                  downhill=slide)
        bm.free()
        r = np.linalg.norm(flat[:, :2], axis=1)
        rmax = max(float(r.max()), 1e-6)
        # Folds: the membrane is bigger than the patch it collapses onto, so it buckles in
        # crowded, branching ridges (ridged noise stretched across the frond).
        # Broad soft folds spanning several vertices (sharper creases live in the normal map),
        # relaxed over the mesh so no single vertex spikes into a pyramid.
        ridged = 1.0 - np.abs(noise.fbm(np.c_[flat[:, 0] * 10.0, flat[:, 1] * 4.5, np.full(len(P), k * 3.1)], 2)) * 2.0
        folds = np.clip(ridged, 0, 1) ** 1.2
        hold = shore.smooth(0.006, 0.035, r)
        edge = shore.smooth(0.55, 1.0, r / rmax)
        lift = hold * 0.0065 * folds * (1.0 - 0.85 * edge)       # the margin lies down on what's below
        edges = shore.mesh_edges(frond)
        count = np.bincount(edges.ravel(), minlength=len(P)).astype(float)
        for _ in range(4):
            acc = np.zeros(len(P))
            np.add.at(acc, edges[:, 0], lift[edges[:, 1]])
            np.add.at(acc, edges[:, 1], lift[edges[:, 0]])
            lift = 0.5 * lift + 0.5 * acc / np.maximum(count, 1)
        rim = shore.get_attr(frond.data, "rim")
        bearing = np.arctan2(flat[:, 1], flat[:, 0])
        ruffle = 0.5 + 0.5 * np.sin(bearing * 11.0 + k)
        lift += edge * 0.0011 * ruffle
        # Laid on the rock and the fronds already laid (in the BVH), so one small constant clearance
        # is enough: stacking it per layer stood the upper margins off the lower ones as cut steps.
        P = P + N * (0.0007 + lift)[:, None]
        shore.set_coords(frond, P)
        frond.data.update()
        normals = np.array([v.normal[:] for v in frond.data.vertices])
        if np.einsum("ij,ij->i", normals, N).mean() < 0:
            frond.data.flip_normals()
        laid.append(frond)
    for frond in laid:
        shore.two_sided(frond, gap=0.0003)
    laver = kit.join(laid, "SM_LaverFronds", pivot=None, unwrap=False, reshade=True, smooth_angle=80)
    kit.pack_uvs(laver, margin=0.003)
    lod = kit.finalize(rocks.lod(kit, laver, "SM_LaverFronds_LOD1", 0.5), pivot=None, unwrap=False,
                       reshade=True, smooth_angle=80)
    REPORT["produce"]["SM_LaverFronds"] = {
        "fronds": len(laid), "produceOffsetCm": [0.0, 0.0, 0.0],
        "frame": "Same local frame and origin as SM_ShoreRock (not re-pivoted); place at the rock's transform.",
        "two_sided": "Geometric: every sheet is doubled with a flipped back face 0.3 mm behind it."}
    return [laver, lod]


# ------------------------------------------------------------------ held props

def _hang_frame(out, splay, roll):
    """Basis for a mussel hanging from its byssus: x (umbo -> posterior) down and splayed out,
    y (dorsal) outward so the flat ventral face, where the byssus leaves, faces the bunch."""
    d = np.array([0.0, 0.0, -1.0]) * math.cos(splay) + out * math.sin(splay)
    dorsal = out - d * (out @ d)
    dorsal /= np.linalg.norm(dorsal)
    side = np.cross(d, dorsal)
    dorsal = dorsal * math.cos(roll) + side * math.sin(roll)
    side = np.cross(d, dorsal)
    return Matrix((tuple(d), tuple(dorsal), tuple(side))).transposed().to_4x4()


def _bvh_world(obj):
    from mathutils.bvhtree import BVHTree
    return BVHTree.FromPolygons([v.co.copy() for v in obj.data.vertices], [p.vertices[:] for p in obj.data.polygons])


def _push_out(point, trees, clearance):
    p = Vector(point)
    for tree in trees:
        loc, nrm, _, _ = tree.find_nearest(p, 0.02)
        if loc is None:
            continue
        depth = (p - loc).dot(nrm)
        if depth < clearance:
            p = loc + nrm * clearance
    return np.array(p)


def mussel_held(kit):
    """Three live mussels pulled off the rock as one bunch, hanging by their gathered byssus:
    umbos together just under the pinch, flat ventral faces turned in to the beard, which runs
    up the middle of the bunch and twists into a short frayed tuft at the pinch."""
    mat = shore.mussel_mat("M_HeldMussel", seed=7.0, worn=0.6, barnacles=0.22, wet=1.0, brown=0.35, bands=0.7, exact=True,
                           worn_color=((0.045, 0.034, 0.060), (0.095, 0.078, 0.115)),
                           base=((0.013, 0.014, 0.019), (0.024, 0.027, 0.040)))
    byssus_mat = shore.byssus_mat("M_HeldByssus")
    rng = np.random.default_rng(77)
    # azimuth (deg), length (m), splay (deg), roll (deg), umbo depth below the pinch (m)
    layout = [(10.0, 0.059, 13.0, 10.0, 0.008), (132.0, 0.063, 9.0, -14.0, 0.006),
              (248.0, 0.052, 18.0, 24.0, 0.011)]
    parts, frames = [], []
    for i, (az, L, splay, roll, depth) in enumerate(layout):
        out = np.array([math.cos(math.radians(az)), math.sin(math.radians(az)), 0.0])
        obj = shore.mussel_wedge(f"HeldMussel{i}", L, mat, seed=300 + i, sides=22, rows=18)
        pco = shore.get_attr(obj.data, "pcoord", 3)
        pco[:, :2] += rng.uniform(-0.4, 0.4, 2)
        kit.tag_coords(obj.data, [tuple(c) for c in pco])
        shell = shore.get_attr(obj.data, "shell", 3)
        grit = 0.4 * shore.smooth(0.3, 0.1, shell[:, 2]) if i != 1 else np.zeros(len(shell))
        shore.set_attr(obj.data, "sand", grit)
        M = Matrix.Translation(Vector(out * 0.003 + np.array([0.0, 0.0, -depth]))) @ _hang_frame(
            out, math.radians(splay), math.radians(roll))
        shore.transform(obj, M)
        parts.append(obj)
        frames.append([out, M])
    # Press the bunch together: start them overlapping, then ease each outward until they just touch.
    for _ in range(80):
        trees = [_bvh_world(o) for o in parts]
        moved = False
        for a in range(3):
            for b in range(a + 1, 3):
                if trees[a].overlap(trees[b]):
                    for k in (a, b):
                        step = Matrix.Translation(Vector(frames[k][0] * 0.0004))
                        shore.transform(parts[k], step)
                        frames[k][1] = step @ frames[k][1]
                    moved = True
        if not moved:
            break
    trees = [_bvh_world(o) for o in parts]
    threads = []
    top = np.array([0.0, 0.0, 0.0])
    for i, (obj, (out, M)) in enumerate(zip(parts, frames)):
        L = layout[i][1]
        for k in range(4):
            s = rng.uniform(0.2, 0.36)
            gape = np.array(M @ Vector(shore.mussel_wedge_point(L, s) + np.array([0.0, 0.0012, 0.0])))
            phi = rng.uniform(0, 2 * math.pi)
            twist = rng.uniform(1.6, 2.4) * math.pi
            entry_z = gape[2] * rng.uniform(0.45, 0.6)
            end = top + np.array([rng.uniform(-0.0015, 0.0015), rng.uniform(-0.0015, 0.0015),
                                  rng.uniform(0.002, 0.006)])
            pts = []
            wander = rng.uniform(0, 2 * math.pi, 3)
            for f in np.linspace(0.0, 1.0, 13):
                if f < 0.3:
                    w = shore.smooth(0.0, 0.3, f)
                    axis = np.array([0.0012 * math.cos(phi), 0.0012 * math.sin(phi), entry_z])
                    p = gape + (axis - gape) * w
                    p[2] = gape[2] + (entry_z - gape[2]) * f / 0.3
                else:
                    g = (f - 0.3) / 0.7
                    r = 0.0013 * (1 - g) + 0.0006 * g
                    ang = phi + twist * g
                    z = entry_z + (end[2] - entry_z) * g
                    p = np.array([r * math.cos(ang), r * math.sin(ang), z])
                    if g > 0.8:                          # the frayed tuft end splays a little
                        fray = shore.smooth(0.8, 1.0, g)
                        p[:2] += (end[:2] - p[:2]) * fray + 0.0012 * fray * np.array([math.cos(wander[2]), math.sin(wander[2])])
                # Smooth wander (a soft, slightly kinked thread), not per-point jitter.
                p = p + 0.0004 * math.sin(math.pi * f) * np.array([math.sin(7.0 * f + wander[0]),
                                                                   math.sin(9.0 * f + wander[1]),
                                                                   0.5 * math.sin(5.0 * f + wander[2])])
                pts.append(_push_out(p, trees, 0.0005))
            threads.append(kit.tube(f"HeldByssus{i}_{k}", [tuple(q) for q in pts],
                                    radius=lambda t: 0.00024 - 0.00008 * t, sides=4, material=byssus_mat, cap=False))
    # A few stray threads looping out of the tuft and back to a shell: the tangle.
    for k in range(2):
        i = k % 3
        out, M = frames[i]
        L = layout[i][1]
        a = np.array([0.0, 0.0, rng.uniform(-0.006, -0.003)])
        b = np.array(M @ Vector(shore.mussel_wedge_point(L, rng.uniform(0.15, 0.25)) + np.array([0, 0.0012, 0])))
        bow = np.cross(out, [0, 0, 1.0]) * rng.choice([-1, 1]) * rng.uniform(0.004, 0.007) + out * 0.003
        pts = [_push_out(a + (b - a) * f + bow * math.sin(math.pi * f), trees, 0.0005) for f in np.linspace(0, 1, 12)]
        threads.append(kit.tube(f"HeldByssusStray{k}", [tuple(q) for q in pts], radius=0.00022, sides=4,
                                material=byssus_mat, cap=False))
    held = kit.join(parts + threads, "SM_MusselHeld", pivot=None, unwrap=False, reshade=True, smooth_angle=70)
    kit.pack_uvs(held, margin=0.004)
    REPORT["held"]["SM_MusselHeld"] = ("Pivot = pinch point at the gathered byssus tuft; +Z up out of the pinch, a "
                                       "tight bunch of three mussels (5.2-6.3 cm) hangs in -Z, umbos 0.6-1.2 cm "
                                       "below the pivot; the frayed tuft end rises ~0.5 cm above it.")
    return held


# Laver handful, front (-Y) to back: (kind, x offset m, layer y m, length m, top width m,
# bottom width m, flop (rad at the bottom, - forward / + back), x of a hanging fold line (m) or None)
LAVER_HELD = [("sheet", 0.000, -0.0070, 0.104, 0.020, 0.092, -0.7, 0.016),
              ("ribbon", 0.014, -0.0035, 0.150, 0.010, 0.030, -0.25, None),
              ("sheet", 0.004, 0.0000, 0.112, 0.018, 0.084, 0.05, None),
              ("ribbon", -0.016, 0.0035, 0.142, 0.009, 0.026, 0.3, None),
              ("sheet", -0.003, 0.0070, 0.098, 0.020, 0.082, 0.6, None)]
LAVER_SIM_FRAMES = 60          # 24 fps: gather over frames 1-18, then let it hang and settle
LAVER_GATHER_FRAMES = 18
LAVER_FOLD_RADIUS = 0.003


def _laver_sheet(k, spec, rng, noise):
    """One laver frond as a quad grid hanging from the pinch: the start shape (a gently curved
    layer parallel to the others, so nothing starts interpenetrated, its lower part flopped
    forward or back and, for one, a side folded over its own face along a hanging line, as wet
    laver sticks to itself), where its top row is gathered to, and margin weights."""
    kind, x0, y0, length, top_w, bottom_w, flop, fold = spec
    nu, nv = (17, 21) if kind == "sheet" else (5, 26)
    tip = 1.0 + 0.10 * noise.fbm(np.c_[np.linspace(0, 2.5, nu), np.full(nu, k * 3.0), np.zeros(nu)], 3)
    if kind == "sheet":                                  # a torn notch in the lower margin
        tear = rng.integers(3, nu - 3)
        tip[tear] -= 0.22
        tip[tear - 1] -= 0.08
        tip[tear + 1] -= 0.1
    else:                                                # strap fronds narrow to a ragged end
        tip += np.array([0.0, 0.06, 0.1, 0.03, -0.04])[:nu]
    left = 1.0 + 0.16 * noise.fbm(np.c_[np.linspace(0, 3, nv), np.full(nv, k * 3.0 + 1.0), np.ones(nv)], 3)
    right = 1.0 + 0.16 * noise.fbm(np.c_[np.linspace(0, 3, nv), np.full(nv, k * 3.0 + 2.0), np.ones(nv)], 3)
    start, gather, rim, pco, frill = [], [], [], [], []
    r = LAVER_FOLD_RADIUS
    for j in range(nv):
        v = j / (nv - 1)
        taper = shore.smooth(0.0, 0.85, v) ** 0.8 if kind == "sheet" else math.sin(math.pi * min(v * 1.25, 1.0)) ** 0.5
        half = 0.5 * (top_w + (bottom_w - top_w) * taper)
        if kind == "ribbon":
            half = max(half, 0.5 * top_w) * (1.0 - 0.45 * shore.smooth(0.8, 1.0, v))
        for i in range(nu):
            u = i / (nu - 1) * 2 - 1
            side = left[j] if u < 0 else right[j]
            x = u * half * (1 + (side - 1) * abs(u))
            drop = length * v * (1 + (tip[i] - 1) * v ** 2)
            pco.append((x + k * 0.37, -drop + k * 0.21, 0.0))
            rim.append(max(shore.smooth(0.7, 1.0, abs(u)), shore.smooth(0.88, 1.0, v)) if j > 0 else 0.0)
            frill.append(max(shore.smooth(0.35, 1.0, abs(u)), shore.smooth(0.7, 1.0, v)) * shore.smooth(0.1, 0.35, v))
            across, lift = x, 0.0
            if fold is not None and x > fold:            # folded over its own face along a hanging line
                e = x - fold
                if e < math.pi * r:
                    across, lift = fold + r * math.sin(e / r), r * (1 - math.cos(e / r))
                else:
                    across, lift = fold - (e - math.pi * r), 2 * r
            fl = flop * shore.smooth(0.15, 1.0, v)
            down = (math.sin(fl), -math.cos(fl))
            front = (-math.cos(fl), -math.sin(fl))
            twist = 1.2 * shore.smooth(0.4, 1.0, v) if kind == "ribbon" else 0.0
            ax, ay = across * math.cos(twist), across * math.sin(twist) + 3.0 * across * across
            if kind == "ribbon":                         # strap fronds swing out clear of the sheets
                ax += math.copysign(0.3, x0) * drop * shore.smooth(0.15, 1.0, v)
            start.append((x0 + ax, y0 + ay + drop * down[0] + lift * front[0],
                          -0.002 + drop * down[1] + lift * front[1]))
            gather.append((x0 * 0.15 + x * 0.12, y0 * 0.35, -0.001))
    faces = [(j * nu + i, (j + 1) * nu + i, (j + 1) * nu + i + 1, j * nu + i + 1)
             for j in range(nv - 1) for i in range(nu - 1)]
    return start, gather, faces, rim, pco, frill, list(range(nu)), nu


def laver_held(kit):
    """A handful of laver lifted from the rock by its gathered holdfasts: three broad, limp
    membranes and two strap-like torn fronds, simulated as cloth whose top rows are drawn
    together into the pinch, so they pleat, drape, fold over one another and hang wider below;
    their margins grow longer than the blade (as laver's do) and ruffle."""
    import bpy
    mat = shore.laver_mat("M_HeldLaver", seed=5.0, wet=1.0, relief=0.5)
    noise = rocks.Noise(91)
    rng = np.random.default_rng(91)
    start, gather, faces, rim, pco, frill, pinned, selfcol = [], [], [], [], [], [], [], []
    for k, spec in enumerate(LAVER_HELD):
        s, g, f, m, c, fr, p, nu = _laver_sheet(k, spec, rng, noise)
        base = len(start)
        start += s
        gather += g
        faces += [tuple(base + q for q in face) for face in f]
        rim += m
        frill += fr
        pco += c
        pinned += [base + q for q in p]
        selfcol += [base + q for q in range(2 * nu, len(s))]
    obj = kit.mesh("HeldLaver", start, faces, material=mat)
    kit.tag_coords(obj.data, pco)
    shore.set_attr(obj.data, "rim", np.array(rim))
    obj.shape_key_add(name="Basis")
    key = obj.shape_key_add(name="Gather", from_mix=False)
    for i in pinned:
        key.data[i].co = gather[i]
    key.value = 0.0
    key.keyframe_insert("value", frame=1)
    key.value = 1.0
    key.keyframe_insert("value", frame=LAVER_GATHER_FRAMES)
    obj.vertex_groups.new(name="Pin").add(pinned, 1.0, "REPLACE")
    obj.vertex_groups.new(name="SelfCollide").add(selfcol, 1.0, "REPLACE")
    ruffle = obj.vertex_groups.new(name="Ruffle")
    for i, w in enumerate(rim):
        if w > 0.01:
            ruffle.add([i], float(w), "REPLACE")
    mod = obj.modifiers.new("Cloth", "CLOTH")
    cs = mod.settings
    cs.quality = 12
    cs.mass = 0.004
    cs.air_damping = 1.5
    cs.tension_stiffness = 15.0
    cs.compression_stiffness = 4.0        # soft: the membrane gives at the pinch rather than balloons
    cs.shear_stiffness = 2.0
    cs.bending_stiffness = 0.05
    cs.vertex_group_mass = "Pin"
    cs.vertex_group_shrink = "Ruffle"
    cs.shrink_min = 0.0
    cs.shrink_max = -0.3                  # the margin's springs 30 % longer: it ruffles
    cc = mod.collision_settings
    cc.use_collision = False
    cc.use_self_collision = True
    cc.self_distance_min = 0.0011
    cc.self_friction = 8.0
    cc.vertex_group_self_collisions = "SelfCollide"
    cc.collision_quality = 4
    scene = bpy.context.scene
    mod.point_cache.frame_start = scene.frame_start = 1
    mod.point_cache.frame_end = LAVER_SIM_FRAMES
    for frame in range(1, LAVER_SIM_FRAMES + 1):
        scene.frame_set(frame)
    kit.apply_modifiers(obj)
    if obj.data.shape_keys:
        obj.shape_key_clear()
    obj.vertex_groups.clear()
    obj.animation_data_clear()
    P = shore.coords(obj)
    P[pinned] = np.array(gather)[pinned]
    # Relax the sharpest cloth kinks (the fine crinkle lives in the normal map).
    edges = shore.mesh_edges(obj)
    count = np.bincount(edges.ravel(), minlength=len(P)).astype(float)[:, None]
    free = np.ones(len(P), dtype=bool)
    free[pinned] = False
    for _ in range(4):
        acc = np.zeros_like(P)
        np.add.at(acc, edges[:, 0], P[edges[:, 1]])
        np.add.at(acc, edges[:, 1], P[edges[:, 0]])
        P[free] = 0.6 * P[free] + 0.4 * (acc / np.maximum(count, 1))[free]
    shore.set_coords(obj, P)
    # Ruffle the margin: laver's edge grows longer than its blade, so it waves in and out.
    obj.data.update()
    normals = np.array([vert.normal[:] for vert in obj.data.vertices])
    flat = np.array(pco)
    wave = np.sin((flat[:, 0] * 1.3 + flat[:, 1]) * 2 * math.pi / 0.034) \
        + 0.45 * np.sin((flat[:, 0] - flat[:, 1] * 0.7) * 2 * math.pi / 0.021)
    P += normals * (0.0032 * np.array(frill) ** 1.5 * wave)[:, None]
    shore.set_coords(obj, P)
    scene.frame_set(1)
    shore.two_sided(obj, gap=0.0003)
    obj.name = "HeldLaverSheets"
    held = kit.join([obj], "SM_LaverHeld", pivot=None, unwrap=False, reshade=True, smooth_angle=80)
    kit.pack_uvs(held, margin=0.004)
    REPORT["held"]["SM_LaverHeld"] = ("Pivot = top pinch where the fronds are gathered; +Z up out of the pinch, three "
                                      "broad limp sheets and two strap fronds hang ~10-14 cm in -Z, wider below.")
    return held


def build(kit):
    rock, sink = rock_mesh(kit)
    tree, bm = shore.bvh_of([rock])
    limpets = rock_limpets(kit, tree)
    rock = kit.join([rock] + limpets, "SM_ShoreRock", pivot=None, unwrap=False, reshade=True, smooth_angle=80)
    rocks.unwrap(rock, angle=62.0, margin=0.003, buried=(0.01, 0.12))
    total = rocks.triangles(rock)
    lods = [kit.finalize(rocks.lod(kit, rock, f"SM_ShoreRock_LOD{i}", min(1.0, t / total)), pivot=None,
                         unwrap=False, reshade=True, smooth_angle=80) for i, t in ((1, 2600), (2, 900))]
    bm.free()
    tree, bm = shore.bvh_of([rock])
    clump = mussel_clump(kit, rock, tree)
    laver = laver_fronds(kit, rock, tree)
    bm.free()
    NOTES.update({"sink_depth_m": round(sink, 3),
                  "placement": f"Origin is the sand line under the rock's bounds centre; place at sand height "
                               f"(already bedded {sink * 100:.0f} cm). Seaward = Blender -Y = Unreal +Y.",
                  "collision": "Convex hull on SM_ShoreRock. The importer applies the set's collision to every "
                               "mesh, so give the produce and held components NoCollision in game.",
                  "produce_frame": "SM_MusselClump and SM_LaverFronds share SM_ShoreRock's origin and axes "
                                   "(produceOffsetCm [0, 0, 0]); spawn them at the rock's transform."})
    return [rock] + lods + clump + laver + [mussel_held(kit), laver_held(kit)]
