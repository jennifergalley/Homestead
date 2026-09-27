"""The dried, collapsed hide of the winter-killed deer.

A deer that dies in winter and lies under snow is cleaned out by scavengers and insects
within months; what remains is the hide, dried stiff as rawhide over the bones: it tents
over the ribs and hips, sags flat onto the ground where the belly was, is torn open
along the belly and over the chest (ribs showing), torn back from the neck (vertebrae and
skull bare), with the lower legs still in their furred skin down to the hooves.

The body hide is solved as a membrane on a grid in the ground plane: pinned to the ground
along its outer outline, free at torn edges, resting on the raster of bone tops (plus a
little dried-rawhide stiffness), sagging under gravity between supports. Coordinates
along the body come from the spine curve: s = arclength from the atlas, t = distance
from the spine toward the belly.
"""
import math

import bpy
import numpy as np

import homestead_sdf as sdf
from deer import common as C
from deer import skeleton as S

CELL = 0.0065
FUR = 0.0065            # fur + skin above the supporting surface
THICK = 0.0032          # solidified rawhide thickness (rim and flesh side)


def body_coords(xy):
    """(s, t) for XY points: arclength along the spine and ventral distance from it."""
    pts = S.SPINE.p[:, :2]
    a, b = pts[:-1], pts[1:]
    ab = b - a
    l2 = np.maximum(np.einsum("ij,ij->i", ab, ab), 1e-12)
    s_out = np.empty(len(xy))
    t_out = np.empty(len(xy))
    for c0 in range(0, len(xy), 4000):
        q = xy[c0:c0 + 4000]
        rel = q[:, None, :] - a[None]
        f = np.clip(np.einsum("nij,ij->ni", rel, ab) / l2, 0.0, 1.0)
        c = a[None] + ab[None] * f[..., None]
        d = np.linalg.norm(q[:, None, :] - c, axis=2)
        i = np.argmin(d, axis=1)
        n = np.arange(len(q))
        s_out[c0:c0 + 4000] = S.SPINE.s[i] + f[n, i] * np.sqrt(l2[i])
        tangent = ab[i] / np.sqrt(l2[i])[:, None]
        dorsal = np.stack([-tangent[:, 1], tangent[:, 0]], axis=1)          # Z x t in XY
        t_out[c0:c0 + 4000] = -np.einsum("ij,ij->i", q - c[n, i], dorsal)
    return s_out, t_out


def _noise2(s, t, scale, seed):
    s, t = np.asarray(s, dtype=np.float64), np.asarray(t, dtype=np.float64)
    p = np.column_stack([s.ravel() * scale, t.ravel() * scale, np.full(s.size, float(seed))])
    return sdf.fbm(p, 3, seed).reshape(s.shape)


S0 = S.S_OF["C4"] - 0.01            # neck, torn edge
S_T6 = S.S_OF["T6"]
S_BELLY = (S.S_OF["T13"] - 0.02, S.S_OF["L5"])
S1 = S.S_OF["Cd1"] + 0.01            # tail root


def dorsal_extent(s):
    """How far the hide reaches past the spine on the back (over the spinous processes)."""
    withers = np.interp(s, [S0, S.S_OF["T1"], S.S_OF["T4"], S.S_OF["T10"], S.S_OF["L3"], S.S_OF["Sacrum"], S1],
                        [0.035, 0.075, 0.105, 0.06, 0.055, 0.085, 0.04])
    return withers


def ventral_extent(s):
    v = np.interp(s, [S0, S.S_OF["C7"], S.S_OF["T3"], S.S_OF["T9"], S.S_OF["L2"], S.S_OF["L6"], S1],
                  [0.075, 0.12, 0.25, 0.3, 0.3, 0.25, 0.1])
    lo, hi = S_BELLY
    torn = np.clip(np.minimum((s - lo) / 0.05, (hi - s) / 0.05), 0.0, 1.0)
    return v - 0.11 * torn


def mask_terms(s, t):
    """Signed margins (m) of the hide outline; the hide is where all are positive.
    Returns (outer, torn, hole): outer edges tuck to the ground, torn edges are free."""
    ragged = 0.018 * _noise2(s, t, 14.0, 3) + 0.005 * _noise2(s, t, 70.0, 5)
    back = t + dorsal_extent(s) + ragged
    belly = ventral_extent(s) - t + ragged
    rump = S1 - s + 0.6 * ragged
    outer = np.minimum(np.minimum(back, belly), rump)
    neck = s - S0 + 1.3 * ragged + 0.02 * np.clip(t / 0.08, -1, 1)
    lo, hi = S_BELLY
    open_belly = np.where((s > lo + 0.02) & (s < hi - 0.02), belly, 1.0)
    torn = np.minimum(neck, open_belly)
    # Opening over the chest where scavengers tore in: ribs show through.
    ds, dt = (s - S_T6 - 0.012) / 0.092, (t - 0.125) / 0.066
    hole = (np.hypot(ds, dt) - 1.0) * 0.07 + 1.2 * ragged
    return outer, torn, hole


def mask(s, t):
    outer, torn, hole = mask_terms(s, t)
    return np.minimum(np.minimum(outer, torn), hole)


def body_prior(s, t):
    """Dried-rawhide stiffness: a low collapsed body the hide keeps even where nothing
    holds it up (m above ground)."""
    height = np.interp(s, [S0, S.S_OF["T1"], S.S_OF["T8"], S.S_OF["L1"], S.S_OF["L4"], S.S_OF["Sacrum"], S1],
                       [0.018, 0.035, 0.05, 0.028, 0.022, 0.05, 0.03])
    centre = 0.35 * ventral_extent(s)
    width = 0.5 * (ventral_extent(s) + dorsal_extent(s))
    return height * np.sqrt(np.clip(1 - ((t - centre) / width) ** 2, 0, 1))


def blur(a, passes=1):
    """Separable 1-2-1 blur, ``passes`` times."""
    for _ in range(passes):
        p = np.pad(a, 1, mode="edge")
        a = (p[:-2, 1:-1] + 2 * p[1:-1, 1:-1] + p[2:, 1:-1]) / 4
        p = np.pad(a, 1, mode="edge")
        a = (p[1:-1, :-2] + 2 * p[1:-1, 1:-1] + p[1:-1, 2:]) / 4
    return a


def solve(support, inside, pinned, prior, iterations=2400, gravity=0.0014):
    """Membrane over ``support`` (NaN = none): relax toward the neighbour average minus
    gravity, never below the support, the prior or the ground; ``pinned`` neighbours
    (outer outline) hold it to the ground, torn neighbours are free."""
    floor = np.maximum(np.nan_to_num(support, nan=0.0), np.maximum(prior, 0.0)) + 0.002
    z = np.where(inside, floor, 0.0)
    ground = 0.0015
    in_pad = np.pad(inside, 1, mode="constant")
    pin_pad = np.pad(pinned, 1, mode="edge")
    views = [(slice(0, -2), slice(1, -1)), (slice(2, None), slice(1, -1)),
             (slice(1, -1), slice(0, -2)), (slice(1, -1), slice(2, None))]
    ins = [in_pad[v] for v in views]
    pins = [pin_pad[v] & ~i for v, i in zip(views, ins)]
    free = [~i & ~p for i, p in zip(ins, pins)]
    n_pinned = sum(p.astype(np.float64) for p in pins)
    n_free = sum(f.astype(np.float64) for f in free)
    for _ in range(iterations):
        pad = np.pad(z, 1, mode="edge")
        acc = sum(np.where(i, pad[v], 0.0) for v, i in zip(views, ins))
        acc = acc + n_pinned * ground + n_free * z
        z = np.where(inside, np.maximum(floor, 0.35 * z + 0.65 * (acc / 4.0 - gravity)), 0.0)
    return z

def body_hide(kit, skeleton_objects, fur_mat, leather_mat):
    """Build the body hide mesh. Returns (object, raster dict for coverage tests)."""
    lo = np.array([-0.42, -0.2])
    hi = np.array([0.62, 0.34])
    shape = tuple(np.ceil((hi - lo) / CELL).astype(int) + 1)
    gx = lo[0] + CELL * np.arange(shape[0])
    gy = lo[1] + CELL * np.arange(shape[1])
    X, Y = np.meshgrid(gx, gy, indexing="ij")
    xy = np.column_stack([X.ravel(), Y.ravel()])
    s, t = body_coords(xy)
    s, t = s.reshape(shape), t.reshape(shape)
    outer, torn, hole = mask_terms(s, t)
    m = np.minimum(np.minimum(outer, torn), hole)
    inside = m > 0
    pinned = (outer <= np.minimum(torn, hole)) & ~inside           # outside across the outer outline
    raster = C.heights(skeleton_objects, lo - CELL / 2, CELL, shape)
    raster = C.dilate_max(raster, 1)
    has = np.isfinite(raster)
    support = np.where(has, raster, 0.0)
    # Rawhide bridges between bones instead of stepping down each cell: blur the support.
    support = np.maximum(blur(support, 2), support - 0.004)
    support = np.where(has | (blur(has.astype(float), 2) > 0.2), support + FUR * 0.35, np.nan)
    prior = body_prior(s, t)
    z = solve(support, inside, pinned, prior)
    z = np.where(inside, np.maximum(blur(np.where(inside, z, 0.0), 2), np.nan_to_num(support) - 0.003), 0.0)

    # Vertices at grid points inside, then boundary vertices slid out onto the outline.
    index = -np.ones(shape, dtype=np.int64)
    ids = np.argwhere(inside)
    index[inside] = np.arange(len(ids))
    verts = np.column_stack([X[inside], Y[inside], z[inside]])
    quads = []
    for i, j in ids:
        if i + 1 < shape[0] and j + 1 < shape[1]:
            a, b, c, d = index[i, j], index[i + 1, j], index[i + 1, j + 1], index[i, j + 1]
            if min(a, b, c, d) >= 0:
                quads.append((a, b, c, d))
    used = np.zeros(len(verts), dtype=bool)
    used[np.array(quads).ravel()] = True
    neighbours = np.zeros(len(verts), dtype=int)
    for q in quads:
        neighbours[list(q)] += 1
    boundary = used & (neighbours < 4)
    bxy = verts[boundary, :2].copy()
    for _ in range(8):
        bs, bt = body_coords(bxy)
        mv = mask(bs, bt)
        e = CELL * 0.25
        gxv = (mask(*body_coords(bxy + [e, 0])) - mv) / e
        gyv = (mask(*body_coords(bxy + [0, e])) - mv) / e
        g2 = np.maximum(gxv ** 2 + gyv ** 2, 1e-6)
        step = np.clip(mv / g2, -CELL * 0.5, CELL * 0.9)[:, None]
        bxy = bxy - step * np.column_stack([gxv, gyv]) / 1.0
    verts[boundary, :2] = bxy

    vs, vt = body_coords(verts[:, :2])
    o_m, t_m, h_m = mask_terms(vs, vt)
    free = np.minimum(t_m, h_m) < o_m                                   # nearest edge is torn
    edge_d = np.clip(np.minimum(np.minimum(o_m, t_m), h_m), 0.0, None)
    # Outer edges tuck down to the ground; torn edges curl up a little and thin out.
    tuck = np.clip(1.0 - edge_d / 0.02, 0.0, 1.0) * (~free)
    curl = np.clip(1.0 - edge_d / 0.014, 0.0, 1.0) * free
    verts[:, 2] = verts[:, 2] * (1 - tuck ** 2) + 0.0015 * tuck ** 2 + 0.004 * curl ** 2
    # Fur clumps and matting in the silhouette.
    clump = sdf.fbm(np.column_stack([vs * 40, vt * 90, np.zeros_like(vs)]), 3, 11)
    verts[:, 2] += FUR + 0.0018 * clump * (1 - curl) - 0.003 * curl

    data = bpy.data.meshes.new("HideBody")
    data.from_pydata([tuple(v) for v in verts], [], quads)
    data.update()
    uv = data.uv_layers.new(name="UVMap")
    for poly in data.polygons:
        for loop in poly.loop_indices:
            co = verts[data.loops[loop].vertex_index]
            uv.data[loop].uv = (co[0], co[1])
    kit.tag_coords(data, [(float(b), 0.0, float(a)) for a, b in zip(vs, vt)])
    obj = bpy.data.objects.new("HideBody", data)
    bpy.context.scene.collection.objects.link(obj)
    data.materials.append(fur_mat)
    data.materials.append(leather_mat)
    skirt(obj, THICK)
    kit.recalc_normals(obj)
    surface = np.where(inside, z + FUR, np.nan)
    return obj, {"lo": lo, "cell": CELL, "shape": shape, "surface": surface}


def skirt(obj, depth):
    """Rawhide edge: extrude the hide's boundary down by ``depth`` (flesh-side material),
    instead of a full second shell underneath that would never be seen."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    edges = [e for e in bm.edges if e.is_boundary]
    result = bmesh.ops.extrude_edge_only(bm, edges=edges)
    new_verts = [g for g in result["geom"] if isinstance(g, bmesh.types.BMVert)]
    for vert in new_verts:
        vert.co.z -= depth
    for face in (g for g in result["geom"] if isinstance(g, bmesh.types.BMFace)):
        face.material_index = 1
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return obj


def covered(obj, raster, margin=0.0015):
    """Fraction of ``obj``'s vertices hidden under the hide surface."""
    v = C.vertices(obj)
    ij = np.floor((v[:, :2] - raster["lo"] + raster["cell"] / 2) / raster["cell"]).astype(int)
    shape = raster["shape"]
    ok = (ij[:, 0] >= 0) & (ij[:, 1] >= 0) & (ij[:, 0] < shape[0]) & (ij[:, 1] < shape[1])
    z = np.full(len(v), np.nan)
    z[ok] = raster["surface"][ij[ok, 0], ij[ok, 1]]
    hidden = np.isfinite(z) & (v[:, 2] < z - margin)
    return float(hidden.mean())


# ------------------------------------------------------------ stockings

def stocking(kit, name, points, radii, material, sides=16):
    """Furred skin tube dried onto a lower leg (hair flowing down toward the hoof)."""
    pts = C.catmull_rom(points, 26)
    rr = np.interp(np.linspace(0, 1, len(pts)), np.linspace(0, 1, len(radii)), radii)
    tube = kit.tube(name, pts, radii=list(rr), sides=sides, material=material)

    def tufts(co, pco):
        a = math.atan2(pco.y, pco.x)
        clump = math.sin(a * 9 + pco.z * 70) * math.sin(pco.z * 150 + a * 3)
        # Dried skin lies flat on the ground side of the leg.
        return 0.0012 * clump - 0.0035 * max(0.0, -co.z + 0.012) / 0.012
    kit.displace(tube, tufts)
    return tube


def leg_stockings(kit, material):
    out = []
    for leg, spec in S.LEGS.items():
        j = [np.array(p, dtype=np.float64) for p in spec["joints"]]
        fore = spec["kind"] == "fore"
        start = j[1] + (j[1] - j[0]) * -0.25
        pts = [start, j[1], j[1] + (j[2] - j[1]) * 0.5, j[2], j[2] + (j[3] - j[2]) * 0.5, j[3],
               j[3] + (j[4] - j[3]) * 0.6, j[4]]
        radii = ([0.024, 0.021, 0.016, 0.0165, 0.0125, 0.0145, 0.0125, 0.0115] if fore else
                 [0.026, 0.022, 0.0155, 0.0175, 0.0128, 0.0148, 0.0125, 0.0115])
        for p, r in zip(pts, radii):
            p[2] = max(p[2], r * 0.72)
        out.append(stocking(kit, f"Stocking{leg}", pts, radii, material))
    return out


def tail(kit, material):
    """The tail's skin and hair, drooped over the caudal vertebrae and past them."""
    s0 = S.S_OF["Cd1"] - 0.01
    pts = [S.SPINE.at(s0 + 0.03 * k) for k in range(6)]
    tip = pts[-1] + S.SPINE.tangent(S.SPINE.length) * 0.05 + np.array([0.0, -0.03, 0.0])
    pts.append(tip)
    for p in pts:
        p[2] = max(0.013, p[2] * 0.7)
    radii = [0.024, 0.022, 0.02, 0.019, 0.017, 0.013, 0.004]
    tube = stocking(kit, "Tail", pts, radii, material, sides=14)
    kit.warp(tube, lambda co: (co.x, co.y, 0.0015 + (co.z - 0.0015) * 0.55))
    return tube
