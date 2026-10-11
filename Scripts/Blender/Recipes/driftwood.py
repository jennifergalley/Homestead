"""Sea-bleached driftwood: three loose sticks she gathers on the beach.

Real-object research (written before modeling):
- Driftwood on an English granite coast is mostly broken branches and roundwood (oak, ash,
  beech, pine, hazel) that has been in the sea for months: the bark is long gone, the surface
  is abraded and sun/salt-bleached to silver-grey (linear albedo ~0.27-0.37, warmer tan where
  freshly scuffed), and the soft earlywood erodes faster than the latewood, so the grain
  stands in fine raised ridges along the stick. Drying opens long checks (cracks) along the
  grain, a few mm wide and deep, often filled with grey grime and sand. Ends are rounded and
  worn smooth by rolling in the surf; knots survive as harder, darker swirls or bumps, and a
  side branch leaves a worn fork or stub. Sticks a person gathers for a fire are 45-75 cm long
  and 3-6 cm thick; a long-tumbled stick is often slightly oval in section.
- SM_Driftwood_a: 62 cm x 4.2 cm, a gentle S-bend, a raised knot and a short worn stub.
- SM_Driftwood_b: 48 cm x 3.2 cm, forked near one end (a 12 cm side branch at ~30 degrees).
- SM_Driftwood_c: 72 cm x 5.4 cm, oval in section (flattened 0.82), deep checks, a flush knot.

Game notes: each stick's long axis runs along local Y (the carried-stick props assume local Y;
the FBX mirror of Y is harmless for a stick). PIVOT: the stick's centre at its underside
(bounds centre in X/Y, lowest point in Z). They lie together as a small pile (game code offsets
them) and two are lifted into her hands one at a time. LOD1 per stick shares LOD0's UVs.

Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import os

import numpy as np

import homestead_rocks as rocks
import homestead_sdf as sdf
import homestead_shore as shore

NAME = "Driftwood"
DESCRIPTION = "Three sea-bleached driftwood sticks (62, 48 and 72 cm) along local Y; pivot centre-underside; LOD1."
COLLISION = "convex"
TRIANGLE_BUDGET = 6000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_SHORE_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96, "repack": False}
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail"]}
REPORT = {"pivot": "Centre of each stick at its underside (bounds centre X/Y, lowest Z). Long axis = local Y.",
          "collision": "Convex hull per stick (only matters if one is dropped as a physics prop).",
          "sticks": {}}

STICKS = {
    "a": dict(length=0.62, radius=0.021, seed=31, bend=(0.022, 0.012), taper=0.25, flatten=1.0,
              knots=[(0.34, 40.0, 0.62, "bump"), (0.71, 200.0, 0.45, "stub")], fork=None,
              checks=4, tris=4600, tone=0.0),
    "b": dict(length=0.48, radius=0.016, seed=32, bend=(0.015, 0.008), taper=0.2, flatten=1.0,
              knots=[(0.28, 120.0, 0.5, "bump")], fork=(0.78, 35.0, 0.12, 0.62), checks=3, tris=4200,
              tone=0.5),
    "c": dict(length=0.72, radius=0.027, seed=33, bend=(0.03, 0.01), taper=0.3, flatten=0.82,
              knots=[(0.55, 300.0, 0.7, "flush")], fork=None, checks=6, tris=5000, tone=1.0),
}


def path_points(spec):
    L = spec["length"]
    rng = np.random.default_rng(spec["seed"])
    n = int(L / 0.006) + 1                 # dense skeleton: round-cone joints must not bead the stick
    y = np.linspace(-L / 2, L / 2, n)
    t = (y + L / 2) / L
    ax, az = spec["bend"]
    ph = rng.uniform(0, 6.28, 2)
    x = ax * np.sin(math.pi * 1.6 * t + ph[0]) + 0.004 * np.sin(9 * t + ph[1])
    z = az * np.sin(math.pi * t + 0.4)
    noise = rocks.Noise(spec["seed"] + 4)
    r = spec["radius"] * (1 - spec["taper"] * t) * (1 + 0.06 * noise.fbm(np.c_[t * 4, np.zeros(n), np.zeros(n)], 3))
    end = np.minimum(y + L / 2, L / 2 - y)
    r *= 0.72 + 0.28 * shore.smooth(0.0, 0.05, end)            # surf-worn ends taper and round off
    return np.c_[x, y, z], r


def frames(points):
    T = np.gradient(points, axis=0)
    T /= np.linalg.norm(T, axis=1, keepdims=True)
    up = np.array([0.0, 0.0, 1.0])
    N = up - T * (T @ up)[:, None]
    N /= np.linalg.norm(N, axis=1, keepdims=True)
    B = np.cross(T, N)
    return T, N, B


def arclength(points):
    return np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(points, axis=0), axis=1))]


def stick_field(spec, P, R):
    T, N, B = frames(P)
    S = arclength(P)
    rng = np.random.default_rng(spec["seed"] + 1)
    body = sdf.tube([tuple(p) for p in P], list(R), k=0.0)
    # Long checks along the grain, slightly spiralled, tapering at their ends.
    checks = []
    for _ in range(spec["checks"]):
        s0 = rng.uniform(0.05, 0.6) * S[-1]
        length = rng.uniform(0.12, 0.3) * S[-1]
        checks.append((s0, min(S[-1] * 0.97, s0 + length), rng.uniform(-math.pi, math.pi), rng.uniform(-3, 3),
                       rng.uniform(0.0012, 0.0026), rng.uniform(0.0008, 0.0013)))
    noise = rocks.Noise(spec["seed"] + 9)
    ys, xs, zs = P[:, 1], P[:, 0], P[:, 2]

    def surface(p):
        cx = np.interp(p[:, 1], ys, xs)
        cz = np.interp(p[:, 1], ys, zs)
        s = np.interp(p[:, 1], ys, S)
        rad = np.interp(p[:, 1], ys, R)
        theta = np.arctan2(p[:, 2] - cz, p[:, 0] - cx)
        # Long, low ridges and grooves along the grain (no round lumps).
        out = 0.0011 * noise.fbm(np.c_[np.cos(theta) * 4.0, np.sin(theta) * 4.0, s * 1.2], 3)
        out += 0.0004 * noise.fbm(np.c_[np.cos(theta) * 14.0, np.sin(theta) * 14.0, s * 2.0], 2)
        for s0, s1, th0, twist, depth, width in checks:
            ang = np.angle(np.exp(1j * (theta - th0 - twist * (s - s0))))
            along = shore.smooth(s0, s0 + 0.04, s) * shore.smooth(s1, s1 - 0.05, s)
            out -= depth * along * np.exp(-((ang * rad) / width) ** 2)
        return out

    shape = sdf.displaced(body, surface, 0.004)
    parts = [shape]
    knots = []
    for frac, ang_deg, size, kind in spec["knots"]:
        i = int(frac * (len(P) - 1))
        th = math.radians(ang_deg)
        d = math.cos(th) * B[i] + math.sin(th) * N[i]
        r = R[i]
        kr = r * size
        if kind == "stub":
            base = P[i] + d * r * 0.3
            tip = P[i] + d * (r + 0.007) + T[i] * 0.012
            parts.append(sdf.round_cone(tuple(base), tuple(tip), r * 0.55, r * 0.42))
        elif kind == "bump":
            # A sound knot stands proud of the eroded wood as a low oval swell, not a collar.
            frame_rot = sdf.rotation(axis_x=tuple(T[i]), axis_y=tuple(d))
            parts.append(sdf.ellipsoid(tuple(P[i] + d * r * 0.78), (r * 0.42, r * 0.3, r * 0.36), frame_rot))
            kr = r * 0.36
        knots.append((r * math.cos(th), r * math.sin(th), S[i], kr * (0.9 if kind == "flush" else 1.0)))
    branch = None
    if spec["fork"]:
        frac, ang_deg, length, rel = spec["fork"]
        i = int(frac * (len(P) - 1))
        th = math.radians(ang_deg)
        side = math.cos(th) * B[i] + math.sin(th) * N[i] * 0.3
        side /= np.linalg.norm(side)
        direction = T[i] * math.cos(math.radians(32)) + side * math.sin(math.radians(32))
        bp = np.array([P[i] + direction * length * f + side * 0.006 * math.sin(f * 3) for f in np.linspace(0, 1, 7)])
        br = R[i] * rel * (1 - 0.35 * np.linspace(0, 1, 7))
        parts.append(sdf.tube([tuple(p) for p in bp], list(br), k=0.003))
        branch = (bp, br)
    return sdf.smooth_union(parts, 0.007), knots, branch


def stick_pcoord(V, skeletons):
    """(x, y, s): position in each vertex's nearest skeleton frame (B, N, arclength)."""
    best = np.full(len(V), np.inf)
    out = np.zeros((len(V), 3))
    for offset, (P, R) in skeletons:
        T, N, B = frames(P)
        S = arclength(P)
        for i in range(len(P) - 1):
            a, b = P[i], P[i + 1]
            ab = b - a
            t = np.clip(((V - a) @ ab) / max(ab @ ab, 1e-12), 0, 1)
            C = a + t[:, None] * ab
            rel = V - C
            dist = np.abs(np.linalg.norm(rel, axis=1) - (R[i] + (R[i + 1] - R[i]) * t))
            better = dist < best
            if not better.any():
                continue
            best[better] = dist[better]
            Bt = B[i] + (B[i + 1] - B[i]) * t[better, None]
            Nt = N[i] + (N[i + 1] - N[i]) * t[better, None]
            out[better, 0] = np.einsum("ij,ij->i", rel[better], Bt)
            out[better, 1] = np.einsum("ij,ij->i", rel[better], Nt)
            out[better, 2] = S[i] + (S[i + 1] - S[i]) * t[better] + offset
    return out


def stick_uvs(obj, ends, segment=0.13, margin=0.012):
    """Lay the stick's own UVs (BAKE repack off): cylindrical islands of ~``segment`` m along
    each skeleton (u round the stick, v along it, both in metres so texel density is even),
    plus a planar island for each worn end (``ends``: [(s, radius)] in pcoord), all packed
    on a grid. A 60 cm haft packed by smart-project becomes a sliver that starves the bake."""
    import bmesh
    mesh = obj.data
    pco = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.attributes["pcoord"].data.foreach_get("vector", pco)
    pco = pco.reshape(-1, 3).astype(np.float64)
    theta_v = np.arctan2(pco[:, 1], pco[:, 0])
    radius_v = np.hypot(pco[:, 0], pco[:, 1])
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv_layer = bm.loops.layers.uv.active or bm.loops.layers.uv.new("UVMap")
    islands = {}
    junction = [f for f in bm.faces if np.ptp(pco[[v.index for v in f.verts], 2]) > 1.0]
    if junction:
        # Faces straddling the main stem and the fork branch: one planar island seen along
        # their mean normal (their pcoords belong to two different skeletons).
        n = sum((f.normal for f in junction), junction[0].normal * 0).normalized()
        a = n.orthogonal().normalized()
        b2 = n.cross(a)
        islands[("junction", 0)] = [(f, [(v.co.dot(a), v.co.dot(b2)) for v in f.verts]) for f in junction]
    skip = set(junction)
    for face in bm.faces:
        if face in skip:
            continue
        idx = [v.index for v in face.verts]
        s_mid = float(pco[idx, 2].mean())
        key = None
        for e, (s_end, r_end) in enumerate(ends):
            if abs(s_mid - s_end) < r_end * 0.9:
                key = ("end", e)
        if key is None:
            key = ("seg", int(math.floor(s_mid / segment)) if s_mid < 4.0 else 1000 + int(math.floor((s_mid - 5.0) / segment)))
        c = np.exp(1j * theta_v[idx]).mean()
        centre = math.atan2(c.imag, c.real)
        uv = []
        for i in idx:
            if key[0] == "end":
                uv.append((pco[i, 0], pco[i, 1]))
            else:
                d = (theta_v[i] - centre + math.pi) % (2 * math.pi) - math.pi
                uv.append(((centre + d) * max(radius_v[idx].mean(), 0.004), pco[i, 2]))
        islands.setdefault(key, []).append((face, uv))
    # Fit every island into one grid cell at a common scale (even texel density).
    keys = sorted(islands, key=str)
    boxes = {}
    for k in keys:
        pts = np.array([p for _, uv in islands[k] for p in uv])
        boxes[k] = (pts.min(axis=0), pts.max(axis=0))
    cols = math.ceil(math.sqrt(len(keys)))
    rows = math.ceil(len(keys) / cols)
    cell = np.array([1.0 / cols, 1.0 / rows])
    scale = min(min((cell[0] - 2 * margin) / max(hi[0] - lo[0], 1e-6), (cell[1] - 2 * margin) / max(hi[1] - lo[1], 1e-6))
                for lo, hi in boxes.values())
    for n, k in enumerate(keys):
        lo, hi = boxes[k]
        origin = np.array([(n % cols) * cell[0], (n // cols) * cell[1]]) + margin
        centre_off = ((cell - 2 * margin) - (hi - lo) * scale) / 2
        for face, uv in islands[k]:
            for loop, p in zip(face.loops, uv):
                loop[uv_layer].uv = tuple(origin + centre_off + (np.array(p) - lo) * scale)
    bm.to_mesh(mesh)
    bm.free()
    return obj


def stick(kit, key, spec):
    P, R = path_points(spec)
    field, knots, branch = stick_field(spec, P, R)
    mat = shore.driftwood_mat(f"M_Driftwood_{key}", knots=knots, seed=spec["seed"] * 0.37, length=spec["length"],
                              warmth=spec["tone"])
    obj = sdf.mesh(f"SM_Driftwood_{key}", field, voxel=0.0007, target_tris=spec["tris"], material=mat)
    V = rocks.coords(obj)
    skeletons = [(0.0, (P, R))]
    if branch is not None:
        skeletons.append((5.0, branch))
    kit.tag_coords(obj.data, [tuple(c) for c in stick_pcoord(V, skeletons)])
    if spec["flatten"] != 1.0:
        V[:, 2] = (V[:, 2] - np.interp(V[:, 1], P[:, 1], P[:, 2])) * spec["flatten"] + np.interp(V[:, 1], P[:, 1], P[:, 2])
        rocks.set_coords(obj, V)
    S = arclength(P)
    ends = [(0.0, R[0]), (S[-1], R[-1])]
    if branch is not None:
        ends.append((5.0 + arclength(branch[0])[-1], branch[1][-1]))
    stick_uvs(obj, ends)
    obj = kit.finalize(obj, pivot="base", unwrap=False, reshade=True, smooth_angle=75.0)
    lod = kit.finalize(rocks.lod(kit, obj, f"SM_Driftwood_{key}_LOD1", 0.35), pivot=None, unwrap=False,
                       reshade=True, smooth_angle=75.0)
    REPORT["sticks"][f"SM_Driftwood_{key}"] = {
        "length_cm": round(spec["length"] * 100, 1), "diameter_cm": round(spec["radius"] * 200, 1),
        "features": [k[3] for k in spec["knots"]] + (["fork"] if spec["fork"] else [])}
    return [obj, lod]


def build(kit):
    meshes = []
    for key, spec in STICKS.items():
        meshes += stick(kit, key, spec)
    return meshes
