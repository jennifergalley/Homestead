"""Shore finds for Homestead: seashells, blue mussels, laver weed, barnacled granite and driftwood.

    import homestead_shore as shore     # Recipes\\beach_shells.py, shore_rock.py, driftwood.py

Geometry
- ``valve`` builds one bivalve (or limpet) shell as a closed solid from its growth-line grid:
  margin outline, umbo, an outer dome, radial ribs and a wall thickness. Growth lines are
  scaled copies of the margin about the umbo and ribs radiate from it, as real shells grow.
  Every vertex carries ``shell`` = (cos a, sin a, v) (a: direction from the umbo, v: 0 at
  the umbo .. 1 at the margin) and ``sh_in`` (1 on the inner surface), read by the materials.
- ``cockle``, ``mussel_valve``, ``razor``, ``limpet`` and ``winkle`` are the species.
- ``mussel`` is a whole live blue mussel (both valves shut), lofted from cross-sections.
- ``drape`` lays thin membranes (laver) over a mesh with a small position-based cloth solve.
Materials (``kit.mats``-style shader graphs, baked to textures by the recipes)
- ``cockle_mat``, ``limpet_mat``, ``mussel_mat``, ``razor_mat``, ``winkle_mat``, ``laver_mat``,
  ``byssus_mat``, ``ulva_mat``, ``driftwood_mat`` and ``shore_overlay`` (wet band, barnacle
  crusts and sand dust over ``kit.mats.granite``).

Everything is generated here: no scanned or downloaded geometry or textures. Seeded throughout.
"""
import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import homestead_kit as kit
import homestead_rocks as rocks
from homestead_materials import Graph

SAND = (0.34, 0.285, 0.20)          # dry quartz/shell sand, linear albedo
SAND_WET = (0.17, 0.14, 0.10)


# ------------------------------------------------------------------ utilities

def smooth(e0, e1, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def set_attr(mesh, name, values):
    values = np.asarray(values, dtype=np.float32)
    if values.ndim == 2:
        attr = mesh.attributes.get(name) or mesh.attributes.new(name, "FLOAT_VECTOR", "POINT")
        attr.data.foreach_set("vector", values.ravel())
    else:
        attr = mesh.attributes.get(name) or mesh.attributes.new(name, "FLOAT", "POINT")
        attr.data.foreach_set("value", values)


def get_attr(mesh, name, width=1):
    attr = mesh.attributes.get(name)
    if attr is None:
        return np.zeros((len(mesh.vertices), width) if width > 1 else len(mesh.vertices))
    out = np.empty(len(mesh.vertices) * width, dtype=np.float32)
    attr.data.foreach_get("vector" if width > 1 else "value", out)
    return out.reshape(-1, width) if width > 1 else out


def coords(obj):
    return rocks.coords(obj)


def set_coords(obj, points):
    rocks.set_coords(obj, points)


def transform(obj, matrix):
    obj.data.transform(matrix)
    obj.data.update()
    return obj


def frame(z_axis, x_hint):
    """Rotation matrix whose local Z is ``z_axis`` and local X is ``x_hint`` made orthogonal."""
    z = Vector(z_axis).normalized()
    x = Vector(x_hint)
    x = (x - z * x.dot(z))
    if x.length < 1e-6:
        x = Vector((1, 0, 0)) if abs(z.x) < 0.9 else Vector((0, 1, 0))
        x = (x - z * x.dot(z))
    x.normalize()
    y = z.cross(x)
    return Matrix((x, y, z)).transposed().to_4x4()


def closed_spline(points, count, close_scale=1.0):
    """Arc-length uniform samples of a closed Catmull-Rom curve through ``points`` (k, 2),
    returned counter-clockwise starting at the first control point."""
    p = np.asarray(points, dtype=np.float64)
    k = len(p)
    dense = []
    for i in range(k):
        p0, p1, p2, p3 = p[(i - 1) % k], p[i], p[(i + 1) % k], p[(i + 2) % k]
        for t in np.linspace(0, 1, 40, endpoint=False):
            t2, t3 = t * t, t * t * t
            dense.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2
                                + (-p0 + 3 * p1 - 3 * p2 + p3) * t3))
    dense = np.array(dense)
    seg = np.linalg.norm(np.roll(dense, -1, axis=0) - dense, axis=1)
    s = np.r_[0.0, np.cumsum(seg)]
    loop = np.vstack([dense, dense[:1]])
    targets = np.linspace(0, s[-1], count, endpoint=False)
    out = np.c_[np.interp(targets, s, loop[:, 0]), np.interp(targets, s, loop[:, 1])]
    area = 0.5 * np.sum(out[:, 0] * np.roll(out[:, 1], -1) - np.roll(out[:, 0], -1) * out[:, 1])
    if area < 0:
        out = np.vstack([out[:1], out[1:][::-1]])
    return out


def _grid_normals(grid):
    dv = np.gradient(grid, axis=0)
    du = (np.roll(grid, -1, axis=1) - np.roll(grid, 1, axis=1)) * 0.5
    n = np.cross(dv, du)
    n /= np.maximum(np.linalg.norm(n, axis=2, keepdims=True), 1e-12)
    if n[: max(2, len(grid) // 2), :, 2].mean() < 0:
        n = -n
    return n


# ------------------------------------------------------------------ bivalve / limpet shells

def valve(name, outline, umbo, dome, thickness, material, rows_out=8, rows_in=4, rib=None,
          inner_rib_from=0.8, v0=0.04, base_only=False, attrs=None):
    """One shell valve as a closed solid, outer surface facing +Z, margin on z ~ 0.

    outline: (nu, 2) margin points, counter-clockwise seen from +Z.
    umbo: (x, y) of the beak projected on the commissure plane (inside the outline).
    dome(P (N,2), v (N,)) -> outer height (m) above the commissure plane.
    thickness(v) -> wall thickness (m). rib(a, v) -> outward relief (m) along the normal.
    base_only: no inner surface; the margin ring is closed by a flat floor (a limpet glued to rock).
    """
    M = np.asarray(outline, dtype=np.float64)
    U = np.asarray(umbo, dtype=np.float64)
    nu = len(M)
    D = M - U
    ang = np.arctan2(D[:, 0], -D[:, 1])
    vs = v0 + (1.0 - v0) * np.linspace(0.0, 1.0, rows_out) ** 0.85
    base = np.array([np.c_[U + D * v, dome(U + D * v, np.full(nu, v))] for v in vs])
    normals = _grid_normals(base)
    relief = np.zeros(base.shape[:2])
    if rib is not None:
        relief = np.array([rib(ang, np.full(nu, v)) for v in vs])
    outer = base + normals * relief[..., None]
    rows = [r for r in outer]
    vrow = list(vs)
    inner_flag = [0.0] * rows_out
    if not base_only:
        idx = np.unique(np.round(np.linspace(0, rows_out - 1, rows_in + 1)).astype(int))[::-1]
        up = np.array([0.0, 0.0, 1.0])
        for i in idx:
            v = vs[i]
            keep = smooth(inner_rib_from, 1.0, v)
            # Near the umbo the grid normals are ill-defined; lean them to +Z so the inner wall
            # never pokes through the outer one.
            lean = smooth(0.35, 0.0, v) * 2.0
            nn = normals[i] + up * lean
            nn /= np.linalg.norm(nn, axis=-1, keepdims=True)
            row = base[i] + normals[i] * (relief[i] * keep)[..., None] - nn * thickness(v)
            row[:, 2] = np.minimum(row[:, 2], outer[i][:, 2] - 0.6 * thickness(v))
            rows.append(row)
            vrow.append(v)
            inner_flag.append(1.0)
    apex = np.r_[U, dome(U[None], np.zeros(1))[0]]
    if base_only:
        floor = np.r_[M.mean(axis=0), -0.0015]
        obj = kit.loft(name, [[tuple(p) for p in r] for r in rows], material=material,
                       cap_start=tuple(apex), cap_end=tuple(floor))
        caps = [(apex, 0.0, 0.0), (floor, 1.0, 0.0)]
    else:
        inner_apex = apex - np.array([0, 0, thickness(0.0)])
        obj = kit.loft(name, [[tuple(p) for p in r] for r in rows], material=material,
                       cap_start=tuple(apex), cap_end=tuple(inner_apex))
        caps = [(apex, 0.0, 0.0), (inner_apex, 0.0, 1.0)]
    shell = [np.c_[np.cos(ang), np.sin(ang), np.full(nu, v)] for v in vrow]
    shell = np.vstack(shell + [np.array([[0.0, -1.0, v]]) for _, v, _ in caps])
    inner = np.concatenate([np.full(nu, f) for f in inner_flag] + [np.array([f]) for _, _, f in caps])
    set_attr(obj.data, "shell", shell)
    set_attr(obj.data, "sh_in", inner)
    for key, value in (attrs or {}).items():
        set_attr(obj.data, key, np.full(len(obj.data.vertices), value, dtype=np.float32))
    return obj


def unwrap(obj, angle=75.0, margin=0.004):
    """Few, low-stretch islands: smart-project to find islands, turn their borders into seams,
    relax each with an angle-based unwrap, equalise texel density and pack."""
    rocks._select(obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=margin, scale_to_bounds=False)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.seams_from_islands(mark_seams=True)
    bpy.ops.uv.unwrap(method="ANGLE_BASED", margin=margin)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.average_islands_scale()
    bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def _rng_noise(seed):
    return rocks.Noise(seed)


def radial(P, centre, outline):
    """Normalised radius of planar points P about ``centre``: 0 at the centre, 1 on the
    (star-shaped) margin ``outline`` in the same direction."""
    C = np.asarray(centre, dtype=np.float64)
    M = np.asarray(outline, dtype=np.float64) - C
    ang = np.arctan2(M[:, 1], M[:, 0])
    order = np.argsort(ang)
    ang, rad = ang[order], np.linalg.norm(M, axis=1)[order]
    ang = np.r_[ang[-1] - 2 * math.pi, ang, ang[0] + 2 * math.pi]
    rad = np.r_[rad[-1], rad, rad[0]]
    D = np.asarray(P, dtype=np.float64) - C
    a = np.arctan2(D[:, 1], D[:, 0])
    return np.linalg.norm(D, axis=1) / np.maximum(np.interp(a, ang, rad), 1e-9)


def cockle(name, length, material, seed=0, detail="low"):
    """Common cockle (Cerastoderma edule) single valve: rounded, slightly oblique outline,
    height ~0.92 L, strongly inflated (one valve ~0.36 L deep), 24 radial ribs with
    crenulated margin. ``detail="high"`` models the ribs and margin crenulation in the mesh."""
    L, H = length, length * 0.92
    noise = _rng_noise(seed + 11)
    high = detail == "high"
    nu = 112 if high else 28
    th = np.linspace(0, 2 * math.pi, nu, endpoint=False) - math.pi / 2
    c, s = np.cos(th), np.sin(th)
    n = 2.3
    x = 0.5 * L * np.sign(c) * np.abs(c) ** (2 / n) * (1 + 0.05 * np.clip(c, 0, 1))
    y = 0.5 * H * np.sign(s) * np.abs(s) ** (2 / n)
    y = np.where(y > 0, y * 0.93, y)
    x = x + 0.07 * L * (y / H)
    wob = 1 + 0.012 * noise.fbm(np.c_[c * 2, s * 2, np.zeros(nu)], 3)
    outline = np.c_[x * wob, y * wob]
    umbo = (-0.04 * L + 0.07 * L * 0.3, 0.33 * H)
    centre = (umbo[0] * 0.6, 0.08 * H)
    depth = 0.36 * L

    def dome(P, v):
        r = np.clip(radial(P, centre, outline), 0, 1)
        beak = 0.10 * np.exp(-np.sum((P - np.array(umbo)) ** 2, axis=1) / (0.14 * L) ** 2)
        return depth * ((1 - r ** 2) ** 0.72 + beak * (1 - np.clip(v, 0, 1)))

    rib = None
    if high:
        def rib(a, v):
            fan = smooth(2.5, 1.9, np.abs(a))                 # ribs fade out toward the hinge
            profile = smooth(0.12, 0.6, 0.5 + 0.5 * np.cos(33 * a))
            return 0.0007 * L / 0.035 * profile * smooth(0.06, 0.55, v) * fan
    return valve(name, outline, umbo, dome, lambda v: (0.0016 - 0.0007 * v) * L / 0.035, material,
                 rows_out=14 if high else 7, rows_in=7 if high else 3, rib=rib, inner_rib_from=0.82)


MUSSEL_OUTLINE = [(0.0, 0.0), (0.10, -0.10), (0.30, -0.165), (0.55, -0.19), (0.78, -0.17),
                  (0.94, -0.08), (1.0, 0.05), (0.96, 0.18), (0.84, 0.28), (0.66, 0.33),
                  (0.48, 0.30), (0.28, 0.19), (0.09, 0.06)]


def mussel_valve(name, length, material, seed=0, nu=28):
    """Empty blue mussel (Mytilus edulis) half-shell: pointed terminal umbo, nearly straight
    ventral margin, dorsal angle at ~60 % length, rounded posterior; one valve ~0.17 L deep."""
    L = length
    outline = closed_spline(np.array(MUSSEL_OUTLINE) * L, nu)
    umbo = (0.035 * L, 0.006 * L)
    centre = (0.47 * L, 0.06 * L)
    depth = 0.17 * L

    def dome(P, v):
        r = np.clip(radial(P, centre, outline), 0, 1)
        return depth * (1 - r ** 2.2) ** 0.8
    return valve(name, outline, umbo, dome, lambda v: (0.0009 - 0.0004 * v) * L / 0.055, material,
                 rows_out=7, rows_in=3, inner_rib_from=1.1)


def razor(name, length, material, seed=0, nu=36):
    """Pod razor (Ensis siliqua) single valve: long, straight, parallel-sided with squared,
    rounded ends; height ~0.15 L; a shallow arched valve ~0.035 L deep; umbo near the
    anterior-dorsal corner."""
    L, H = length, length * 0.15
    pts = [(0.0, 0.0), (0.012, -0.38), (0.07, -0.5), (0.5, -0.5), (0.93, -0.48), (0.99, -0.34),
           (1.0, 0.02), (0.99, 0.36), (0.93, 0.5), (0.5, 0.52), (0.07, 0.5), (0.012, 0.38)]
    pts = [(px * L, py * H + 0.01 * L * math.sin(math.pi * px)) for px, py in pts]
    outline = closed_spline(pts, nu)
    umbo = (0.05 * L, 0.30 * H)
    depth = 0.035 * L

    def dome(P, v):
        yc = 0.01 * L * np.sin(math.pi * np.clip(P[:, 0] / L, 0, 1))
        across = 1 - np.clip(np.abs(P[:, 1] - yc) / (0.5 * H), 0, 1) ** 2
        return depth * across ** 0.85 * (1 - np.clip(v, 0, 1) ** 6) ** 0.5 + 0.0003
    return valve(name, outline, umbo, dome, lambda v: 0.0006, material, rows_out=6, rows_in=3,
                 inner_rib_from=1.1)


def limpet(name, length, material, seed=0, nu=32, rows=6, base_only=False, height=None):
    """Common limpet (Patella vulgata): oval base ~0.83 L wide, conical, apex a little
    anterior of centre, ~32 coarse irregular radial ridges that crenulate the margin."""
    L = length
    W = 0.83 * L
    Hh = height if height is not None else 0.45 * L
    noise = _rng_noise(seed + 5)
    th = np.linspace(0, 2 * math.pi, nu, endpoint=False)
    rng = np.random.default_rng(seed)
    phase = rng.uniform(0, 6.28)
    crenel = 0.025 * np.cos(32 * th + 0.7 * np.sin(3 * th + phase)) + 0.02 * noise.fbm(
        np.c_[np.cos(th) * 3, np.sin(th) * 3, np.zeros(nu)], 3)
    outline = np.c_[0.5 * L * np.cos(th), 0.5 * W * np.sin(th)] * (1 + crenel)[:, None]
    umbo = (-0.08 * L, 0.0)

    def dome(P, v):
        v = np.clip(v, 0, 1)
        return Hh * (1 - v ** 1.5) * (1 - 0.05 * smooth(0.18, 0.0, v))

    def rib(a, v):
        ridge = 0.5 + 0.5 * np.cos(32 * a + 0.7 * np.sin(3 * a + phase))
        return 0.0011 * L / 0.045 * ridge ** 1.4 * smooth(0.1, 0.9, v)
    return valve(name, outline, umbo, dome, lambda v: (0.0016 - 0.0006 * v) * L / 0.045, material,
                 rows_out=rows, rows_in=3, rib=rib if nu >= 48 else None, base_only=base_only,
                 inner_rib_from=0.75)


def winkle(name, height, material, seed=0, sides=12):
    """Edible periwinkle (Littorina littorea): a closed, pointed spire of ~5 whorls on a
    logarithmic helicospiral (whorl expansion ~2.2 per turn), body whorl ~60 % of the
    height, round-ovate aperture closed here by a recessed dark interior. pcoord from the
    tube: (r cos phi, r sin phi, arclength); ``aperture`` attribute 1 at the aperture centre."""
    b = math.log(1.85) / (2 * math.pi)
    turns = 5.6
    T = turns * 2 * math.pi
    per_turn = [2, 2, 3, 5, 7, 12]
    ts = []
    for k, count in enumerate(per_turn):
        start = T - (len(per_turn) - k) * 2 * math.pi
        for i in range(count):
            t = start + 2 * math.pi * i / count
            if t >= 0:
                ts.append(t)
    ts = [0.0] + [t for t in ts if t > 0.15] + [T]
    ts = np.array(sorted(set(round(t, 5) for t in ts)))
    g = np.exp(b * (ts - T))
    rho = 0.5 * g                       # tube radius (relative)
    r = 0.30 * g                         # coil radius: whorls overlap the axis (no umbilicus)
    zc = 1.3 * g                         # drop per growth: successive whorls overlap
    pts = np.c_[r * np.cos(ts), r * np.sin(ts), -zc]
    scale = height / (pts[:, 2].max() - (pts[-1, 2] - rho[-1]))
    pts *= scale
    radii = list(np.maximum(rho * scale, 0.0002))
    radii[0] = 0.0003
    obj = kit.tube(name, [tuple(p) for p in pts], radii=radii, sides=sides, material=material)
    mesh = obj.data
    count = len(pts) * sides
    verts = coords(obj)
    tangent = pts[-1] - pts[-2]
    tangent /= np.linalg.norm(tangent)
    # Recess the aperture cap into a dark interior.
    verts[count + 1] -= tangent * radii[-1] * 0.7
    set_coords(obj, verts)
    aperture = np.zeros(len(verts))
    aperture[count + 1] = 1.0
    set_attr(mesh, "aperture", aperture)
    whorl = np.zeros(len(verts))
    whorl[:count] = np.repeat(ts / T, sides)
    whorl[count:] = (0.0, 1.0)
    set_attr(mesh, "whorl", whorl)
    return obj


# ------------------------------------------------------------------ whole blue mussel

_S = np.linspace(0, 1, 401)
_ENV_PEAK = float((_S ** 0.6 * np.clip(1 - _S ** 3, 0, 1) ** 0.5).max())
_WID_PEAK = float((_S ** 0.5 * np.clip(1 - _S ** 2.5, 0, 1) ** 0.55).max())


def _mussel_section(s):
    """Blue mussel cross-section at s = x / L: centre height, half height, half width (x L).
    Narrow pointed umbo, broadest at ~0.65 L, rounded posterior; height 0.5 L, width 0.34 L."""
    env = s ** 0.6 * np.clip(1 - s ** 3, 0, 1) ** 0.5 / _ENV_PEAK
    half_h = 0.215 * env
    yc = 0.065 * s ** 0.9
    width = 0.15 * s ** 0.5 * np.clip(1 - s ** 2.5, 0, 1) ** 0.55 / _WID_PEAK
    return yc, half_h, width


def mussel(name, length, material, seed=0, sides=10, rows=9, gape=0.0):
    """A live blue mussel (Mytilus edulis), both valves shut: x from the anterior umbo (0)
    to the rounded posterior (L), y dorsal, z across the valves (commissure plane z = 0).
    Height ~0.5 L, width ~0.34 L. ``shell`` = (cos a, sin a, v) about the umbo as on the
    valves; ``sh_in`` = 0. Returns the object; the byssal gape is at x ~0.3 L, ventral."""
    L = length
    rng = np.random.default_rng(seed)
    ss = 0.5 - 0.5 * np.cos(np.linspace(0.04, 0.985, rows) * math.pi)
    ss = np.clip(ss, 0.012, 0.992)
    phi = np.linspace(0, 2 * math.pi, sides, endpoint=False)
    squash = 1.0 + rng.uniform(-0.06, 0.06)
    rows_pts = []
    for s in ss:
        yc, hh, w = _mussel_section(s)
        y = yc + hh * np.cos(phi)
        edge = np.where(np.cos(phi) > 0, 0.72, 0.42)      # keeled dorsal, flattened ventral
        z = w * squash * np.sign(np.sin(phi)) * (1 - np.abs(np.cos(phi))) ** edge
        z = z + gape * np.sign(np.sin(phi)) * 0.01
        rows_pts.append(np.c_[np.full(sides, s), y, z] * L)
    tip = (0.0, 0.0, 0.0)
    tail_s = 1.0
    yc, hh, w = _mussel_section(0.999)
    tail = (tail_s * L, (yc + 0.15 * hh) * L, 0.0)
    obj = kit.loft(name, [[tuple(p) for p in r] for r in rows_pts], material=material,
                   cap_start=tip, cap_end=tail)
    P = coords(obj)
    # Growth parameter v: the margin (outline) scaled about the umbo.
    prof = np.linspace(0.0, 1.0, 200)
    sec = [_mussel_section(s) for s in prof]
    outline = np.r_[np.c_[prof, [c + h for c, h, _ in sec]], np.c_[prof[::-1], [c - h for c, h, _ in sec][::-1]]] * L
    oa = np.arctan2(outline[:, 1], outline[:, 0])
    order = np.argsort(oa)
    orad = np.linalg.norm(outline, axis=1)
    a = np.arctan2(P[:, 1], P[:, 0])
    rad = np.interp(a, oa[order], orad[order])
    v = np.clip(np.linalg.norm(P[:, :2], axis=1) / np.maximum(rad, 1e-6), 0, 1)
    set_attr(obj.data, "shell", np.c_[np.cos(a), np.sin(a), v])
    set_attr(obj.data, "sh_in", np.zeros(len(P)))
    return obj


def mussel_point(length, s, side="ventral"):
    yc, hh, _ = _mussel_section(s)
    return np.array([s * length, (yc - hh if side == "ventral" else yc + hh) * length, 0.0])


# Side view of a closed Mytilus edulis (x / L from the umbo, y / L dorsal): pointed terminal umbo,
# nearly straight ventral margin, dorsal angle at ~0.6 L, broadly rounded posterior; height ~0.49 L.
WEDGE_VENTRAL = [(0.0, 0.0), (0.04, -0.045), (0.12, -0.09), (0.25, -0.125), (0.42, -0.15), (0.6, -0.158),
                 (0.76, -0.15), (0.88, -0.118), (0.95, -0.075), (0.99, -0.02), (1.0, 0.03)]
WEDGE_DORSAL = [(0.0, 0.0), (0.03, 0.025), (0.1, 0.075), (0.22, 0.155), (0.38, 0.255), (0.52, 0.318),
                (0.62, 0.33), (0.74, 0.305), (0.85, 0.245), (0.93, 0.17), (0.98, 0.095), (1.0, 0.03)]


def _wedge_margins(s):
    vx, vy = np.array(WEDGE_VENTRAL).T
    dx, dy = np.array(WEDGE_DORSAL).T
    return np.interp(s, vx, vy), np.interp(s, dx, dy)


def _wedge_width(s):
    """Half the valve-to-valve breadth (x L): fat behind the beak, greatest at ~0.4 L."""
    s = np.clip(s, 0.0, 1.0)
    beak = np.minimum(s / 0.4, 1.0) ** 0.5
    tail = np.clip(1 - (np.maximum(s - 0.4, 0.0) / 0.61) ** 2, 0, 1) ** 0.6
    return 0.215 * beak * tail


def mussel_wedge(name, length, material, seed=0, sides=18, rows=16):
    """A live blue mussel (Mytilus edulis), shut, modelled as the real wedge: pointed terminal
    umbo, straight ventral margin, dorsal angle, rounded posterior and fat, convex valves with
    a flattened ventral (byssal) face; breadth ~0.43 L. Same frame and attributes as ``mussel``:
    x from the umbo (0) to the posterior (L), y dorsal, z across the valves (seam at z = 0);
    ``shell`` = (cos a, sin a, v) about the umbo, ``sh_in`` = 0. See ``mussel_wedge_point``."""
    L = length
    rng = np.random.default_rng(seed)
    ss = 0.5 - 0.5 * np.cos(np.linspace(0.06, 0.985, rows) * math.pi)
    ss = np.clip(ss, 0.006, 0.994)
    t = np.linspace(0, 2 * math.pi, sides, endpoint=False)
    u = np.cos(t)                                      # +1 dorsal margin, -1 ventral margin
    across = np.sign(np.sin(t))
    # Breadth sits low in the section: keeled dorsal edge, broad flat ventral face.
    expo = np.where(u > 0, 0.85, 0.42)
    bulge = np.abs(np.sin(t)) ** expo * (1.0 - 0.28 * u)
    bulge /= bulge.max()
    squash = 1.0 + rng.uniform(-0.05, 0.05)
    tilt = rng.uniform(-0.04, 0.04)                    # valves a touch unequal
    rows_pts = []
    for s in ss:
        yv, yd = _wedge_margins(s)
        h = yd - yv
        level = np.where(u > 0, u ** 1.0, -np.abs(u) ** 0.75)
        y = yv + h * (0.5 + 0.5 * level)
        z = _wedge_width(s) * squash * across * bulge * (1.0 + tilt * across)
        rows_pts.append(np.c_[np.full(sides, s), y, z] * L)
    obj = kit.loft(name, [[tuple(p) for p in r] for r in rows_pts], material=material,
                   cap_start=(0.0, 0.0, 0.0), cap_end=(L, 0.03 * L, 0.0))
    P = coords(obj)
    prof = np.linspace(0.0, 1.0, 300)
    yv, yd = _wedge_margins(prof)
    outline = np.r_[np.c_[prof, yv], np.c_[prof[::-1], yd[::-1]]] * L
    oa = np.arctan2(outline[1:, 1], outline[1:, 0])
    orad = np.linalg.norm(outline[1:], axis=1)
    order = np.argsort(oa)
    a = np.arctan2(P[:, 1], P[:, 0])
    rad = np.interp(a, oa[order], orad[order])
    v = np.clip(np.linalg.norm(P[:, :2], axis=1) / np.maximum(rad, 1e-6), 0, 1)
    set_attr(obj.data, "shell", np.c_[np.cos(a), np.sin(a), v])
    set_attr(obj.data, "shell_p", np.c_[P[:, :2] / L, np.zeros(len(P))])
    set_attr(obj.data, "sh_in", np.zeros(len(P)))
    return obj


def _wedge_reach_table(count=24, lo=-1.2, hi=1.0):
    """Colour-ramp stops of 1 / (umbo-to-margin distance) of ``mussel_wedge``'s side outline by
    bearing from the umbo, normalised by its maximum: (lo, hi, kmax, stops)."""
    prof = np.linspace(0.0, 1.0, 600)
    yv, yd = _wedge_margins(prof)
    outline = np.r_[np.c_[prof, yv], np.c_[prof[::-1], yd[::-1]]][1:-1]
    oa = np.arctan2(outline[:, 1], outline[:, 0])
    orad = np.linalg.norm(outline, axis=1)
    order = np.argsort(oa)
    angles = np.linspace(lo, hi, count)
    k = 1.0 / np.maximum(np.interp(angles, oa[order], orad[order]), 0.04)
    kmax = float(k.max())
    return lo, hi, kmax, [(i / (count - 1), (float(x / kmax),) * 3) for i, x in enumerate(k)]


def mussel_wedge_point(length, s, side="ventral"):
    """Point on the shut seam of ``mussel_wedge`` at s = x / L (the byssal gape is ventral, s ~0.3)."""
    yv, yd = _wedge_margins(s)
    return np.array([s * length, (yv if side == "ventral" else yd) * length, 0.0])


# ------------------------------------------------------------------ settling and BVH

def bvh_of(objects, offset=0.0):
    bm = bmesh.new()
    for obj in objects:
        tmp = bmesh.new()
        tmp.from_mesh(obj.data)
        if offset:
            for v in tmp.verts:
                v.co += v.normal * offset
        tmp.transform(obj.matrix_world)
        data = bpy.data.meshes.new("_bvh_tmp")
        tmp.to_mesh(data)
        tmp.free()
        bm.from_mesh(data)
        bpy.data.meshes.remove(data)
    bm.faces.ensure_lookup_table()
    tree = BVHTree.FromBMesh(bm)
    return tree, bm


def surface_down(tree, x, y, z0=2.0):
    hit = tree.ray_cast(Vector((x, y, z0)), Vector((0, 0, -1)))
    return hit if hit[0] is not None else None


def inside(tree, point):
    loc, normal, _, _ = tree.find_nearest(Vector(point))
    if loc is None:
        return False, 0.0
    d = (Vector(point) - loc).dot(normal)
    return d < 0, d


# ------------------------------------------------------------------ laver: membranes

def frond(name, radius, material, seed, sectors=24, rings=7, stretch=1.0, ragged=0.0):
    """A laver (Porphyra umbilicalis) frond as a flat polar membrane around its small
    central holdfast: irregular, ruffled lobes, 0.6-1.2 of ``radius``. pcoord = flat coords.
    ``ragged`` (0..1) frays the outer two rings into a torn, uneven margin with a few nicks."""
    rng = np.random.default_rng(seed)
    noise = _rng_noise(seed + 3)
    phi = np.linspace(0, 2 * math.pi, sectors, endpoint=False)
    lobes = 1 + 0.24 * noise.fbm(np.c_[np.cos(phi) * 1.4, np.sin(phi) * 1.4, np.zeros(sectors)], 3) \
        + 0.10 * np.cos(rng.integers(2, 4) * phi + rng.uniform(0, 6)) \
        + 0.09 * noise.fbm(np.c_[np.cos(phi) * 6.0, np.sin(phi) * 6.0, np.ones(sectors)], 2)
    R = radius * np.clip(lobes, 0.45, 1.5)
    tear = np.zeros(sectors)
    if ragged:
        fray = _rng_noise(seed + 29)
        tear = 0.10 * np.abs(fray.fbm(np.c_[np.cos(phi) * 11.0, np.sin(phi) * 11.0, np.full(sectors, 3.0)], 3)) * 2.0
        for j in np.random.default_rng(seed + 31).choice(sectors, 3, replace=False):
            tear[j] += 0.2
            tear[(j + 1) % sectors] += 0.08
    verts = [(0.0, 0.0, 0.0)]
    frill = 1 + 0.035 * np.sin(phi * rng.integers(9, 13) + rng.uniform(0, 6)) \
        + 0.03 * noise.fbm(np.c_[np.cos(phi) * 9.0, np.sin(phi) * 9.0, np.full(sectors, 2.0)], 2)
    for k in range(1, rings + 1):
        f = (k / rings) ** 0.9
        jitter = 1 + 0.03 * rng.standard_normal(sectors) * (k / rings)
        if k == rings:
            jitter = jitter * frill
        if ragged and k >= rings - 1:
            jitter = jitter * (1.0 - ragged * (0.5 if k < rings else 1.0) * tear)
        for j in range(sectors):
            ang = phi[j] + (0.5 * (k % 2)) * (2 * math.pi / sectors) * 0.5
            rr = R[j] * f * jitter[j]
            verts.append((rr * math.cos(ang) * stretch, rr * math.sin(ang), 0.0))
    faces = []
    for j in range(sectors):
        faces.append((0, 1 + j, 1 + (j + 1) % sectors))
    for k in range(1, rings):
        a0, b0 = 1 + (k - 1) * sectors, 1 + k * sectors
        for j in range(sectors):
            j2 = (j + 1) % sectors
            faces.append((a0 + j, b0 + j, b0 + j2, a0 + j2))
    obj = kit.mesh(name, verts, faces, material=material)
    rim = np.zeros(len(verts))
    rim[1 + (rings - 1) * sectors:] = 1.0
    set_attr(obj.data, "rim", rim)
    return obj


def mesh_edges(obj):
    e = np.empty(len(obj.data.edges) * 2, dtype=np.int64)
    obj.data.edges.foreach_get("vertices", e)
    return e.reshape(-1, 2)


def geodesic_lay(frond, tree, anchor, normal, spin=0.0, offset=0.0012, step=0.004, downhill=0.0):
    """Wrap a flat polar ``frond`` (centre vertex 0, rings x sectors from ``frond``) over the
    surface in ``tree`` from ``anchor``: each sector is marched out along the surface
    (a geodesic polar map), so the membrane keeps its size as it rolls over the rock's
    shoulders. ``downhill`` (0..1) bends the march toward -Z, as a wet sheet slides down a
    steep face. Returns world positions (N, 3) and per-vertex surface normals."""
    P0 = coords(frond)
    n0 = Vector(normal).normalized()
    ref = Vector((1, 0, 0)) if abs(n0.x) < 0.9 else Vector((0, 1, 0))
    t1 = (ref - n0 * ref.dot(n0)).normalized()
    t1 = Matrix.Rotation(spin, 3, n0) @ t1
    t2 = n0.cross(t1)
    out = np.zeros_like(P0)
    nrm = np.zeros_like(P0)
    out[0] = np.array(anchor)
    nrm[0] = np.array(n0)
    radii = np.linalg.norm(P0[:, :2], axis=1)
    angles = np.arctan2(P0[:, 1], P0[:, 0])
    order = np.argsort(radii)
    # Group vertices by direction: march each one from the anchor along its own bearing.
    for i in order[1:]:
        d = (t1 * math.cos(angles[i]) + t2 * math.sin(angles[i]))
        p, n = Vector(anchor), n0.copy()
        remaining = radii[i]
        while remaining > 1e-6:
            ds = min(step, remaining)
            remaining -= ds
            if downhill:
                g = Vector((0, 0, -1))
                g = (g - n * g.dot(n))
                if g.length > 1e-6:
                    d = (d + g.normalized() * downhill * ds / 0.02).normalized()
            p = p + d * ds
            loc, nn, _, _ = tree.find_nearest(p)
            if loc is None:
                break
            p, n = loc, nn
            d = (d - n * d.dot(n))
            if d.length < 1e-9:
                break
            d.normalize()
        out[i] = np.array(p)
        nrm[i] = np.array(n)
    return out, nrm


def drape(obj, collide, start, pinned, steps=220, dt=1 / 60, offset=0.0012, friction=0.15,
          gravity=(0.0, 0.0, -9.8), iterations=6, pin_to=None, rest=None, compress=0.0):
    """Position-based cloth: lay ``obj``'s vertices (``start`` (N, 3) world) under gravity
    on the ``collide`` BVH tree. Edges never stretch; ``compress`` (0..1) is how strongly they
    resist shortening (0: a membrane that crumples freely in its plane; ~0.5: the excess
    buckles into folds). ``rest``: edge rest lengths (default: from ``start``).
    ``pinned``: vertex indices held at ``pin_to`` (default: their start position)."""
    x = np.array(start, dtype=np.float64)
    prev = x.copy()
    edges = mesh_edges(obj)
    if rest is None:
        rest = np.linalg.norm(x[edges[:, 1]] - x[edges[:, 0]], axis=1)
    pin_pos = x[pinned].copy() if pin_to is None else np.asarray(pin_to, dtype=np.float64)
    g = np.array(gravity)
    counts = np.bincount(edges.ravel(), minlength=len(x)).astype(np.float64)[:, None]
    contact = np.zeros(len(x), dtype=bool)
    for _ in range(steps):
        vel = (x - prev) * 0.985
        vel[contact] *= friction
        prev = x.copy()
        x = x + vel + g * dt * dt
        for _ in range(iterations):
            d = x[edges[:, 1]] - x[edges[:, 0]]
            length = np.linalg.norm(d, axis=1)
            over = (length - rest) / np.maximum(length, 1e-9)
            over = np.where(over > 0, over, over * compress)
            corr = d * (0.5 * over)[:, None]
            acc = np.zeros_like(x)
            np.add.at(acc, edges[:, 0], corr)
            np.add.at(acc, edges[:, 1], -corr)
            x += acc / np.maximum(counts, 1) * 1.6
            x[pinned] = pin_pos
        contact[:] = False
        if collide is not None:
            for i in range(len(x)):
                loc, normal, _, _ = collide.find_nearest(Vector(x[i]), 0.05)
                if loc is None:
                    continue
                depth = (Vector(x[i]) - loc).dot(normal)
                if depth < offset:
                    x[i] = np.array(loc + normal * offset)
                    contact[i] = True
        x[pinned] = pin_pos
    return x


def two_sided(obj, gap=0.00025):
    """Give a thin open sheet a back face: duplicate it, flipped, ``gap`` behind."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    back = bm.verts.layers.float.get("back") or bm.verts.layers.float.new("back")
    bm.normal_update()
    geom = bmesh.ops.duplicate(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:])
    new_verts = [e for e in geom["geom"] if isinstance(e, bmesh.types.BMVert)]
    new_faces = [e for e in geom["geom"] if isinstance(e, bmesh.types.BMFace)]
    for v in new_verts:
        v.co -= v.normal * gap
    bmesh.ops.reverse_faces(bm, faces=new_faces)
    for v in new_verts:
        v[back] = 1.0
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return obj


# ------------------------------------------------------------------ materials: helpers

def _fac(g, name):
    return g.node("ShaderNodeAttribute", attribute_name=name, attribute_type="GEOMETRY").outputs["Fac"]


def _vec(g, name):
    return g.node("ShaderNodeAttribute", attribute_name=name, attribute_type="GEOMETRY").outputs["Vector"]


def _fmix(g, a, b, fac):
    return g.node("ShaderNodeMix", data_type="FLOAT", Factor=fac, A=a, B=b).outputs[0]


def _shell_inputs(g, exact=False):
    p = g.coord()
    sh = _vec(g, "shell")
    cx, sx, v = g.separate(sh)
    a = g.math("ARCTAN2", sx, cx)
    if exact:
        # ``mussel_wedge``: rebuild a and v per pixel from the shell-frame position ``shell_p``
        # (x / L, y / L), which interpolates exactly, and a smooth table of the margin's distance
        # from the umbo by bearing. v interpolated from the vertices kinks at every edge, and the
        # bumped growth lines then show the triangulation as facets.
        sp = _vec(g, "shell_p")
        spx, spy, _ = g.separate(sp)
        a = g.math("ARCTAN2", spy, spx)
        lo, hi, kmax, table = _wedge_reach_table()
        k = g.channel(g.ramp(g.remap(a, lo, hi), table, interpolation="CARDINAL"), 0)
        v = g.math("MINIMUM", g.math("MULTIPLY", g.vmath("LENGTH", sp), g.math("MULTIPLY", k, kmax)), 1.0)
    inner = _fac(g, "sh_in")
    return p, a, v, inner


def _sand(g, p, color, rough, height, amount=1.0):
    """Sand grains clinging where the ``sand`` attribute says the shell touched the beach."""
    mask = _fac(g, "sand")
    grains = g.voronoi(p, scale=1.0 / 0.0005).outputs["Distance"]
    speck = g.noise(p, scale=1.0 / 0.0012, detail=2.0).outputs["Fac"]
    cover = g.math("MULTIPLY", g.remap(g.math("ADD", mask, g.math("MULTIPLY", g.math("SUBTRACT", speck, 0.5), 0.9)),
                                       0.35, 0.75), amount)
    tint = g.mix(SAND, (0.22, 0.19, 0.15), g.remap(speck, 0.3, 0.7))
    color = g.mix(color, tint, cover)
    rough = _fmix(g, rough, 0.92, cover)
    height = g.math("ADD", height, g.math("MULTIPLY", cover, g.remap(grains, 0.0, 0.4, 0.6, 0.0)))
    return color, rough, height


def _finish(g, color, rough, height, strength=0.6, distance=0.0006, normal=None):
    g.set("Base Color", color)
    g.set("Roughness", rough)
    g.set("Normal", g.bump(height, strength=strength, distance=distance, normal=normal))
    g.bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return g.mat


def _growth(g, v, count=70.0, seed=0.0):
    """Concentric growth lines along v: irregularly spaced fine ridges whose strength comes
    and goes, and a few dark growth checks (winter rings)."""
    jitter = g.noise(g.combine(g.math("MULTIPLY", v, 13.0), seed + 1.7, 0.0), scale=1.0, detail=3.0).outputs["Fac"]
    vv = g.combine(g.math("ADD", g.math("MULTIPLY", v, count), g.math("MULTIPLY", jitter, 6.0)), seed, 0.0)
    wave = g.wave(vv, scale=1.0, distortion=3.0, detail=4.0, kind="BANDS", direction="X", profile="SAW").outputs["Fac"]
    gain = g.noise(g.combine(g.math("MULTIPLY", v, 24.0), seed + 5.3, 0.0), scale=1.0, detail=2.0).outputs["Fac"]
    fine = g.math("MULTIPLY", g.math("POWER", wave, 3.0), g.remap(gain, 0.3, 0.7, 0.25, 1.0))
    major = g.noise(g.combine(g.math("MULTIPLY", v, 9.0), seed + 3.1, 0.0), scale=1.0, detail=2.0).outputs["Fac"]
    return fine, major


# ------------------------------------------------------------------ shell materials

def cockle_mat(name, seed=0.0, stain=0.4, ribs=True):
    """Cerastoderma edule: dirty cream to buff exterior, brown growth bands, scaly ribs,
    grey sulphide staining from the sand; porcellaneous cream interior with a brown
    posterior blotch and margin crenulations."""
    g = Graph(name)
    p, a, v, inner = _shell_inputs(g)
    ps = g.vmath("ADD", p, (seed, seed * 0.7, seed * 1.3))
    rib_wave = g.math("ADD", 0.5, g.math("MULTIPLY", g.math("COSINE", g.math("MULTIPLY", a, 33.0)), 0.5))
    rib = g.remap(rib_wave, 0.12, 0.6)                    # flat-topped ribs, narrow grooves
    fan = g.math("MULTIPLY", g.remap(g.math("ABSOLUTE", a), 2.5, 1.9), g.remap(v, 0.05, 0.3))
    rib = g.math("MULTIPLY", rib, fan)
    fine, major = _growth(g, v, 55.0, seed)
    scale_bumps = g.math("MULTIPLY", g.math("MULTIPLY", rib, g.remap(fine, 0.4, 0.9)), g.remap(v, 0.4, 0.85))
    tone = g.noise(ps, scale=1.0 / 0.012, detail=4.0).outputs["Fac"]
    out = g.mix((0.34, 0.30, 0.22), (0.42, 0.38, 0.30), g.remap(tone, 0.35, 0.7))
    out = g.mix(out, (0.22, 0.15, 0.085), g.math("MULTIPLY", g.remap(major, 0.5, 0.66), 0.8))
    out = g.mix(out, (0.27, 0.21, 0.13), g.math("MULTIPLY", g.remap(v, 0.55, 0.95), 0.45))
    out = g.mix(out, (0.80, 0.77, 0.72), g.math("MULTIPLY", fine, 0.35), blend="MULTIPLY")
    grey = g.noise(ps, scale=1.0 / 0.02, detail=5.0, roughness=0.6).outputs["Fac"]
    out = g.mix(out, (0.15, 0.15, 0.14), g.math("MULTIPLY", g.remap(grey, 0.55, 0.72), stain))
    out = g.mix(out, (0.74, 0.71, 0.66), g.math("SUBTRACT", 1.0, rib), blend="MULTIPLY")
    out = g.mix(out, (0.42, 0.40, 0.35), g.remap(v, 0.25, 0.08))          # worn chalky umbo
    ins = g.mix((0.40, 0.37, 0.31), (0.45, 0.43, 0.37), g.remap(tone, 0.3, 0.7))
    blot_n = g.noise(ps, scale=1.0 / 0.008, detail=4.0).outputs["Fac"]
    blotch = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(a, 0.5, 1.7), g.remap(v, 0.3, 0.75)),
                    g.remap(blot_n, 0.35, 0.65, 0.4, 1.0))
    ins = g.mix(ins, (0.26, 0.17, 0.15), g.math("MULTIPLY", blotch, 0.5))
    ins = g.mix(ins, (0.34, 0.31, 0.25), g.math("MULTIPLY", g.remap(v, 0.86, 0.98), g.math("SUBTRACT", 1.0, rib)))
    color = g.mix(out, ins, inner)
    rough_out = g.remap(tone, 0.3, 0.7, 0.62, 0.78)
    rough = _fmix(g, rough_out, g.remap(tone, 0.3, 0.7, 0.30, 0.42), inner)
    height_out = g.math("ADD", g.math("MULTIPLY", rib, 0.9 if ribs else 0.4),
                        g.math("ADD", g.math("MULTIPLY", fine, 0.35), g.math("MULTIPLY", scale_bumps, 0.7)))
    height = _fmix(g, height_out, g.math("MULTIPLY", g.math("MULTIPLY", rib, g.remap(v, 0.8, 0.97)), 0.6), inner)
    color, rough, height = _sand(g, p, color, rough, height)
    return _finish(g, color, rough, height, strength=0.7, distance=0.0009)


def limpet_mat(name, seed=0.0, algae=0.0):
    """Patella vulgata: chalky grey ridged exterior with dark grooves and an eroded,
    rusty apex; glossy grey-green interior, cream head scar and horseshoe muscle scar.
    ``algae`` adds a green-brown film (limpets on the shore rock)."""
    g = Graph(name)
    p, a, v, inner = _shell_inputs(g)
    ps = g.vmath("ADD", p, (seed, seed * 0.4, seed * 0.9))
    jag = g.noise(g.combine(g.math("MULTIPLY", g.math("COSINE", a), 2.0), g.math("MULTIPLY", g.math("SINE", a), 2.0), seed), scale=1.0, detail=3.0).outputs["Fac"]
    wob = g.math("ADD", g.math("MULTIPLY", g.math("SINE", g.math("ADD", g.math("MULTIPLY", a, 3.0), seed)), 0.7),
                 g.math("MULTIPLY", jag, 4.0))
    ridge_c = g.math("ADD", 0.5, g.math("MULTIPLY", g.math("COSINE", g.math("ADD", g.math("MULTIPLY", a, 32.0), wob)), 0.5))
    ridge_n = g.noise(g.combine(g.math("MULTIPLY", g.math("COSINE", a), 7.0), g.math("MULTIPLY", g.math("SINE", a), 7.0),
                                g.math("ADD", g.math("MULTIPLY", v, 1.2), seed)), scale=1.0, detail=4.0, roughness=0.6).outputs["Fac"]
    ridge = g.math("ADD", g.math("MULTIPLY", ridge_c, 0.5), g.math("MULTIPLY", g.remap(ridge_n, 0.3, 0.7), 0.5))
    ridge = g.math("MULTIPLY", ridge, g.remap(v, 0.12, 0.4))
    sub = g.math("ADD", 0.5, g.math("MULTIPLY", g.math("COSINE", g.math("ADD", g.math("MULTIPLY", a, 96.0), wob)), 0.5))
    fine, major = _growth(g, v, 45.0, seed)
    tone = g.noise(ps, scale=1.0 / 0.01, detail=4.0).outputs["Fac"]
    out = g.mix((0.20, 0.19, 0.16), (0.29, 0.27, 0.22), g.remap(tone, 0.3, 0.7))
    out = g.mix(out, (0.09, 0.085, 0.075), g.math("MULTIPLY", g.remap(ridge, 0.45, 0.1), 0.5))
    crust = g.noise(ps, scale=1.0 / 0.004, detail=5.0, roughness=0.65).outputs["Fac"]
    out = g.mix(out, (0.36, 0.34, 0.30), g.math("MULTIPLY", g.remap(crust, 0.62, 0.75), 0.6))
    out = g.mix(out, (0.30, 0.20, 0.12), g.remap(v, 0.22, 0.05))
    out = g.mix(out, (0.24, 0.22, 0.17), g.math("MULTIPLY", g.remap(major, 0.55, 0.75), 0.2))
    if algae:
        film = g.noise(ps, scale=1.0 / 0.015, detail=4.0).outputs["Fac"]
        out = g.mix(out, (0.055, 0.065, 0.032), g.math("MULTIPLY", g.remap(film, 0.35, 0.6), algae))
        out = g.mix(out, (0.75, 0.75, 0.72), algae * 0.6, blend="MULTIPLY")
    ins = g.mix((0.24, 0.27, 0.26), (0.33, 0.36, 0.35), g.remap(tone, 0.3, 0.7))
    head = g.remap(v, 0.42, 0.3)
    ins = g.mix(ins, (0.42, 0.33, 0.20), g.math("MULTIPLY", head, 0.8))
    scar = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(v, 0.45, 0.5), g.remap(v, 0.62, 0.56)),
                  g.remap(g.math("ABSOLUTE", a), 0.35, 0.6))
    ins = g.mix(ins, (0.44, 0.42, 0.37), scar)
    ins = g.mix(ins, (0.06, 0.06, 0.07), g.math("MULTIPLY", g.remap(v, 0.82, 0.95),
                                                   g.remap(ridge, 0.5, 0.15)))
    color = g.mix(out, ins, inner)
    rough = _fmix(g, g.remap(tone, 0.3, 0.7, 0.78, 0.9), g.remap(tone, 0.3, 0.7, 0.18, 0.28), inner)
    height_out = g.math("ADD", g.math("MULTIPLY", ridge, 1.0), g.math("ADD", g.math("MULTIPLY", sub, 0.15),
                                                                       g.math("MULTIPLY", fine, 0.3)))
    height_out = g.math("ADD", height_out, g.math("MULTIPLY", crust, 0.35))
    height = _fmix(g, height_out, g.math("MULTIPLY", scar, 0.3), inner)
    color, rough, height = _sand(g, p, color, rough, height)
    return _finish(g, color, rough, height, strength=0.75, distance=0.0012)


def mussel_mat(name, seed=0.0, worn=0.5, barnacles=0.0, wet=0.0, brown=0.25, bands=0.0, base=None, exact=False,
               worn_color=None):
    """Mytilus edulis: glossy blue-black periostracum with fine growth lines and brownish
    young growth at the margin, eroded violet-grey umbo; nacreous blue-white interior with
    a dark violet margin band. ``wet`` lowers roughness (live mussels on the rock).
    ``bands`` (0..1) adds faint brown growth bands across the valves; ``base`` overrides the
    (dark, light) periostracum colours and ``worn_color`` the (dark, light) eroded umbo;
    ``exact`` reads ``mussel_wedge``'s per-pixel growth frame."""
    g = Graph(name)
    p, a, v, inner = _shell_inputs(g, exact)
    ps = g.vmath("ADD", p, (seed, seed * 0.6, seed * 1.7))
    fine, major = _growth(g, v, 110.0, seed)
    tone = g.noise(ps, scale=1.0 / 0.012, detail=3.0).outputs["Fac"]
    dark, light = base or ((0.018, 0.020, 0.032), (0.032, 0.036, 0.058))
    out = g.mix(dark, light, g.remap(tone, 0.35, 0.7))
    out = g.mix(out, (0.050, 0.036, 0.022), g.math("MULTIPLY", g.remap(v, 0.7, 0.98), brown))
    if bands:
        ring = g.noise(g.combine(g.math("MULTIPLY", v, 16.0), seed + 21.0, 0.0), scale=1.0, detail=3.0).outputs["Fac"]
        out = g.mix(out, (0.046, 0.034, 0.022), g.math("MULTIPLY", g.remap(ring, 0.52, 0.66),
                                                       g.math("MULTIPLY", g.remap(v, 0.25, 0.5), bands)))
    zone = g.noise(g.combine(g.math("MULTIPLY", v, 6.0), seed + 9.0, 0.0), scale=1.0, detail=2.0).outputs["Fac"]
    out = g.mix(out, (0.040, 0.030, 0.045), g.math("MULTIPLY", g.remap(zone, 0.55, 0.7), 0.6))
    out = g.mix(out, (0.012, 0.012, 0.016), g.math("MULTIPLY", g.remap(major, 0.5, 0.65), 0.6))
    bands = g.math("MULTIPLY", fine, 0.45)
    out = g.mix(out, (0.055, 0.06, 0.085), bands)
    erode_n = g.noise(ps, scale=1.0 / 0.006, detail=5.0, roughness=0.6).outputs["Fac"]
    erosion = g.math("MULTIPLY", g.remap(g.math("ADD", g.math("SUBTRACT", 1.0, v),
                                                g.math("MULTIPLY", g.math("SUBTRACT", erode_n, 0.5), 0.5)),
                                         0.80, 0.9), worn)
    worn_lo, worn_hi = worn_color or ((0.10, 0.085, 0.13), (0.20, 0.19, 0.22))
    worn_col = g.mix(worn_lo, worn_hi, g.remap(erode_n, 0.4, 0.7))
    out = g.mix(out, worn_col, erosion)
    # The valve margins: a thin brown-olive periostracum lip either side of the shut seam.
    seam_d = g.math("ABSOLUTE", g.separate(p)[2])
    lip = g.math("MULTIPLY", g.remap(seam_d, 0.0018, 0.0006), g.math("SUBTRACT", 1.0, inner))
    out = g.mix(out, (0.075, 0.055, 0.03), g.math("MULTIPLY", lip, 0.8))
    seam = g.remap(seam_d, 0.0004, 0.0)
    out = g.mix(out, (0.01, 0.008, 0.006), g.math("MULTIPLY", seam, g.math("SUBTRACT", 1.0, inner)))
    big = g.noise(g.scale(ps, 0.6), scale=2.0, detail=1.0).outputs["Fac"]
    out = g.mix(out, (1.6, 1.45, 1.3), g.math("MULTIPLY", g.remap(big, 0.5, 0.7), 0.45), blend="MULTIPLY")
    swirl = g.noise(ps, scale=1.0 / 0.01, detail=3.0, distortion=1.5).outputs["Color"]
    layers = g.noise(g.combine(g.math("MULTIPLY", v, 30.0), g.math("MULTIPLY", a, 2.0), seed), scale=1.0,
                     detail=4.0, distortion=2.0).outputs["Fac"]
    nacre = g.mix((0.22, 0.23, 0.28), (0.32, 0.33, 0.36), g.remap(layers, 0.3, 0.7))
    nacre = g.mix(nacre, swirl, 0.35, blend="OVERLAY")
    nacre = g.mix(nacre, (0.26, 0.24, 0.32), g.math("MULTIPLY", g.remap(tone, 0.55, 0.75), 0.6))
    nacre = g.mix(nacre, (0.10, 0.09, 0.17), g.remap(v, 0.80, 0.93))
    nacre = g.mix(nacre, (0.02, 0.02, 0.03), g.remap(v, 0.965, 1.0))
    color = g.mix(out, nacre, inner)
    sheen = g.noise(ps, scale=1.0 / 0.02, detail=3.0).outputs["Fac"]
    rough_out = g.remap(sheen, 0.3, 0.7, 0.26 - 0.06 * wet, 0.50 - 0.08 * wet)
    rough_out = g.math("ADD", rough_out, g.math("MULTIPLY", fine, 0.12))
    rough_out = _fmix(g, rough_out, 0.72, erosion)
    rough = _fmix(g, rough_out, g.remap(layers, 0.3, 0.7, 0.12, 0.22), inner)
    height = g.math("ADD", g.math("MULTIPLY", fine, 0.6), g.math("MULTIPLY", g.remap(major, 0.5, 0.65), 0.5))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", erosion, g.math("MULTIPLY", erode_n, 0.5)))
    height = _fmix(g, height, g.math("MULTIPLY", layers, 0.08), inner)
    if barnacles:
        cells = g.node("ShaderNodeTexVoronoi", Vector=ps, Scale=1.0 / 0.005, Randomness=0.9, feature="F1")
        pick = g.channel(cells.outputs["Color"], 0)
        size = g.remap(g.channel(cells.outputs["Color"], 1), 0.0, 1.0, 0.28, 0.45)
        on = g.math("MULTIPLY", g.remap(pick, 1.0 - 0.06 * barnacles, 1.0 - 0.06 * barnacles + 0.005),
                    g.math("SUBTRACT", 1.0, inner))
        on = g.math("MULTIPLY", on, g.remap(v, 0.3, 0.6))
        dist = cells.outputs["Distance"]
        cone = g.math("MAXIMUM", g.math("SUBTRACT", 1.0, g.math("DIVIDE", dist, size)), 0.0)
        hole = g.remap(dist, 0.09, 0.05)
        body = g.math("MULTIPLY", g.remap(cone, 0.0, 0.1), on)
        bcol = g.mix((0.11, 0.105, 0.095), (0.21, 0.20, 0.18), g.remap(cone, 0.1, 0.6))
        bcol = g.mix(bcol, (0.04, 0.035, 0.03), hole)
        color = g.mix(color, bcol, body)
        rough = _fmix(g, rough, 0.8, body)
        height = g.math("ADD", height, g.math("MULTIPLY", g.math("SUBTRACT", g.math("MINIMUM", g.math("MULTIPLY", cone, 1.5), 0.7),
                                                               g.math("MULTIPLY", hole, 0.5)), on))
    color, rough, height = _sand(g, p, color, rough, height)
    return _finish(g, color, rough, height, strength=0.45, distance=0.0008)


def razor_mat(name, seed=0.0):
    """Ensis siliqua: varnish-glossy olive-brown periostracum over a cream-violet shell,
    the dorsal-anterior triangle marked off by a diagonal line, growth lines, the
    periostracum flaking at the margin; white-violet interior."""
    g = Graph(name)
    p, a, v, inner = _shell_inputs(g)
    px, py, pz = g.separate(p)
    ps = g.vmath("ADD", p, (seed, seed, seed))
    fine, major = _growth(g, v, 60.0, seed)
    tone = g.noise(ps, scale=1.0 / 0.02, detail=4.0).outputs["Fac"]
    shell = g.mix((0.36, 0.31, 0.31), (0.30, 0.22, 0.25), g.remap(tone, 0.3, 0.7))
    varnish = g.mix((0.11, 0.085, 0.04), (0.19, 0.15, 0.075), g.remap(major, 0.35, 0.7))
    tri = g.remap(a, 1.38, 1.46)
    varnish = g.mix(varnish, (0.14, 0.08, 0.05), g.math("MULTIPLY", tri, 0.7))
    varnish = g.mix(varnish, (0.05, 0.035, 0.018), g.math("MULTIPLY", fine, 0.6))
    flake = g.noise(ps, scale=1.0 / 0.006, detail=5.0, roughness=0.6).outputs["Fac"]
    wear = g.noise(ps, scale=1.0 / 0.03, detail=3.0).outputs["Fac"]
    peel = g.remap(g.math("ADD", g.math("ADD", g.math("MULTIPLY", g.remap(v, 0.85, 1.0), 0.5),
                                         g.math("MULTIPLY", g.remap(v, 0.3, 0.05), 0.4)),
                          g.math("ADD", g.math("MULTIPLY", flake, 0.35), g.math("MULTIPLY", wear, 0.35))), 0.72, 0.8)
    out = g.mix(varnish, shell, peel)
    streak = g.noise(g.combine(g.math("MULTIPLY", px, 15.0), g.math("MULTIPLY", py, 300.0), pz), scale=1.0, detail=3.0).outputs["Fac"]
    ins = g.mix((0.44, 0.40, 0.42), (0.36, 0.28, 0.34), g.remap(streak, 0.45, 0.7))
    color = g.mix(out, ins, inner)
    rough = _fmix(g, _fmix(g, g.remap(tone, 0.3, 0.7, 0.12, 0.2), 0.55, peel), 0.3, inner)
    height = g.math("ADD", g.math("MULTIPLY", fine, 0.4), g.math("MULTIPLY", peel, -0.3))
    color, rough, height = _sand(g, p, color, rough, height)
    return _finish(g, color, rough, height, strength=0.4, distance=0.0006)


def winkle_mat(name, seed=0.0, worn=0.5):
    """Littorina littorea: dark grey-brown with fine dark spiral lines along the whorls,
    axial growth lines, a pale worn apex; chocolate interior with a white lip."""
    g = Graph(name)
    p = g.coord()
    px, py, pz = g.separate(p)
    ps = g.vmath("ADD", p, (seed, seed, seed))
    phi = g.math("ARCTAN2", py, px)
    whorl = _fac(g, "whorl")
    ap = _fac(g, "aperture")
    spiral = g.wave(g.combine(g.math("MULTIPLY", phi, 4.1), seed, 0.0), scale=1.0, distortion=2.5, detail=3.0,
                    kind="BANDS", direction="X").outputs["Fac"]
    axial = g.wave(g.combine(g.math("MULTIPLY", pz, 2600.0), seed, 0.0), scale=1.0, distortion=4.0, detail=3.0,
                   kind="BANDS", direction="X").outputs["Fac"]
    tone = g.noise(ps, scale=1.0 / 0.004, detail=4.0).outputs["Fac"]
    color = g.mix((0.026, 0.022, 0.019), (0.060, 0.050, 0.040), g.remap(tone, 0.3, 0.7))
    color = g.mix(color, (0.010, 0.009, 0.008), g.math("MULTIPLY", g.remap(spiral, 0.55, 0.85), 0.75))
    color = g.mix(color, (0.10, 0.085, 0.065), g.math("MULTIPLY", g.remap(axial, 0.8, 0.97), 0.12))
    wear = g.math("MULTIPLY", g.remap(whorl, 0.45, 0.2), worn)
    color = g.mix(color, (0.24, 0.21, 0.17), wear)
    lip = g.math("MULTIPLY", g.remap(ap, 0.0, 0.25), g.remap(ap, 0.5, 0.3))
    color = g.mix(color, (0.07, 0.04, 0.025), g.remap(ap, 0.15, 0.5))
    color = g.mix(color, (0.40, 0.37, 0.32), g.math("MULTIPLY", lip, 0.7))
    rough = _fmix(g, g.remap(tone, 0.3, 0.7, 0.42, 0.58), 0.8, wear)
    height = g.math("ADD", g.math("MULTIPLY", spiral, 0.35), g.math("MULTIPLY", axial, 0.12))
    color, rough, height = _sand(g, p, color, rough, height)
    return _finish(g, color, rough, height, strength=0.35, distance=0.0004)


def laver_mat(name, seed=0.0, wet=1.0, relief=1.0):
    """Porphyra umbilicalis: a thin, glossy, crinkled membrane, purple-brown to olive, darker
    and glossier wet, with pale bleached spots and thinner, lighter torn margins. ``relief``
    scales the crinkle bump (a lifted, hanging sheet is smoother than one collapsed on rock)."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed, seed * 1.3, seed * 0.7))
    rim = _fac(g, "rim")
    patch = g.noise(ps, scale=1.0 / 0.05, detail=4.0, roughness=0.55).outputs["Fac"]
    fine = g.noise(ps, scale=1.0 / 0.006, detail=5.0).outputs["Fac"]
    color = g.ramp(patch, [(0.15, (0.046, 0.012, 0.030)), (0.38, (0.070, 0.030, 0.042)),
                           (0.6, (0.072, 0.052, 0.028)), (0.82, (0.050, 0.064, 0.022))])
    edge_tint = g.math("MULTIPLY", rim, g.remap(fine, 0.3, 0.7, 0.35, 0.85))
    color = g.mix(color, (0.115, 0.090, 0.035), edge_tint)
    color = g.mix(color, (0.82, 0.78, 0.80), g.remap(fine, 0.3, 0.7), blend="MULTIPLY")
    bleach = g.noise(ps, scale=1.0 / 0.02, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.17, 0.12, 0.10), g.math("MULTIPLY", g.remap(bleach, 0.7, 0.8), 0.6))
    color = g.mix(color, (0.13, 0.08, 0.075), g.math("MULTIPLY", rim, 0.5))
    crinkle = g.noise(ps, scale=1.0 / 0.004, detail=6.0, roughness=0.6, distortion=0.8).outputs["Fac"]
    folds = g.noise(ps, scale=1.0 / 0.012, detail=3.0, distortion=1.2).outputs["Fac"]
    rough = g.remap(fine, 0.3, 0.7, 0.16 + 0.25 * (1 - wet), 0.30 + 0.25 * (1 - wet))
    height = g.math("ADD", g.math("MULTIPLY", crinkle, 0.6), g.math("MULTIPLY", folds, 0.8))
    return _finish(g, color, rough, height, strength=0.6 * relief, distance=0.0015 * relief)


def byssus_mat(name):
    g = Graph(name)
    p = g.coord()
    tone = g.noise(p, scale=400.0, detail=3.0).outputs["Fac"]
    color = g.mix((0.10, 0.07, 0.035), (0.20, 0.15, 0.07), g.remap(tone, 0.3, 0.7))
    return _finish(g, color, 0.45, tone, strength=0.2, distance=0.0003)


def ulva_mat(name, seed=0.0):
    """Sea lettuce scraps: thin bright-to-dark green sheet, glossy wet."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed, seed, seed))
    tone = g.noise(ps, scale=1.0 / 0.01, detail=4.0).outputs["Fac"]
    color = g.mix((0.018, 0.042, 0.010), (0.040, 0.075, 0.016), g.remap(tone, 0.3, 0.7))
    crinkle = g.noise(ps, scale=1.0 / 0.003, detail=4.0).outputs["Fac"]
    return _finish(g, color, g.remap(tone, 0.3, 0.7, 0.28, 0.4), crinkle, strength=0.3, distance=0.0008)


# ------------------------------------------------------------------ granite shore overlay

def shore_overlay(mat, seed=0.0):
    """Layer a granite material for the intertidal: ``wet`` (0..1 point attribute) darkens
    and glosses the seaward foot of the rock with a thin olive algal film; ``barnacle``
    (0..1) grows Semibalanus crusts (5-10 mm white-grey cones with dark opercula); ``sand``
    dusts the base with sand grains."""
    tree = mat.node_tree
    bsdf = next(n for n in tree.nodes if n.type == "BSDF_PRINCIPLED")
    g = Graph.__new__(Graph)
    g.mat, g.tree, g.bsdf = mat, tree, bsdf
    base_color = bsdf.inputs["Base Color"].links[0].from_socket
    base_rough = bsdf.inputs["Roughness"].links[0].from_socket if bsdf.inputs["Roughness"].is_linked \
        else bsdf.inputs["Roughness"].default_value
    base_normal = bsdf.inputs["Normal"].links[0].from_socket
    p = g.coord()
    ps = g.vmath("ADD", p, (seed, seed * 1.7, seed * 0.3))
    wet = _fac(g, "wet")
    barn = _fac(g, "barnacle")
    sand = _fac(g, "sand")
    # Intertidal rock carries a thin film of microalgae and silt: greyer and darker overall.
    color = g.mix(base_color, (0.70, 0.70, 0.66), 1.0, blend="MULTIPLY")
    film = g.noise(ps, scale=9.0, detail=5.0, roughness=0.6).outputs["Fac"]
    color = g.mix(color, (0.46, 0.46, 0.44), wet, blend="MULTIPLY")
    color = g.mix(color, (0.06, 0.075, 0.035), g.math("MULTIPLY", wet, g.remap(film, 0.45, 0.7, 0.0, 0.55)))
    rough = _fmix(g, base_rough, g.remap(film, 0.3, 0.7, 0.30, 0.45), g.math("MULTIPLY", wet, 0.9))
    # Barnacles: one per Voronoi cell where the crust density allows.
    warp = g.noise(ps, scale=40.0, detail=2.0).outputs["Color"]
    bp = g.vmath("ADD", ps, g.scale(g.vmath("SUBTRACT", warp, (0.5, 0.5, 0.5)), 0.004))
    cells = g.node("ShaderNodeTexVoronoi", Vector=bp, Scale=1.0 / 0.009, Randomness=0.9, feature="F1")
    pick = g.channel(cells.outputs["Color"], 0)
    size = g.remap(g.channel(cells.outputs["Color"], 1), 0.0, 1.0, 0.38, 0.62)
    cluster = g.noise(ps, scale=1.0 / 0.06, detail=3.0).outputs["Fac"]
    dens = g.math("MULTIPLY", barn, g.remap(cluster, 0.45, 0.6, 0.0, 1.4))
    on = g.remap(g.math("SUBTRACT", dens, pick), -0.05, 0.02)
    dist = cells.outputs["Distance"]
    cone = g.math("MAXIMUM", g.math("SUBTRACT", 1.0, g.math("DIVIDE", dist, size)), 0.0)
    plates = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", g.math("ARCTAN2",
                     g.separate(g.vmath("SUBTRACT", bp, cells.outputs["Position"]))[1],
                     g.separate(g.vmath("SUBTRACT", bp, cells.outputs["Position"]))[0]), 3.0)))
    orifice = g.remap(dist, 0.13, 0.07)
    body = g.math("MULTIPLY", g.remap(cone, 0.0, 0.08), on)
    bcol = g.mix((0.10, 0.096, 0.088), (0.165, 0.16, 0.148), g.remap(plates, 0.2, 0.9))
    bcol = g.mix(bcol, (0.05, 0.048, 0.044), g.remap(dist, 0.2, 0.12))
    bcol = g.mix(bcol, (0.05, 0.045, 0.04), orifice)
    old = g.noise(bp, scale=300.0, detail=2.0).outputs["Fac"]
    bcol = g.mix(bcol, (0.18, 0.17, 0.15), g.math("MULTIPLY", g.remap(old, 0.55, 0.75), 0.6))
    color = g.mix(color, bcol, body)
    rough = _fmix(g, rough, 0.82, body)
    bh = g.math("MULTIPLY", g.math("SUBTRACT", g.math("MINIMUM", g.math("MULTIPLY", cone, 1.6), 0.75),
                                   g.math("MULTIPLY", orifice, 0.6)), on)
    bh = g.math("ADD", bh, g.math("MULTIPLY", g.math("MULTIPLY", plates, 0.08), body))
    # Sand dust at the foot.
    grain = g.noise(ps, scale=1.0 / 0.0015, detail=2.0).outputs["Fac"]
    dust = g.remap(g.math("ADD", sand, g.math("MULTIPLY", g.math("SUBTRACT", grain, 0.5), 0.8)), 0.3, 0.7)
    color = g.mix(color, g.mix(SAND, SAND_WET, wet), dust)
    rough = _fmix(g, rough, 0.9, dust)
    tree.links.new(color, bsdf.inputs["Base Color"])
    if isinstance(rough, float):
        bsdf.inputs["Roughness"].default_value = rough
    else:
        tree.links.new(rough, bsdf.inputs["Roughness"])
    bump = g.bump(g.math("ADD", g.math("MULTIPLY", bh, 1.0), g.math("MULTIPLY", dust, g.math("MULTIPLY", grain, 0.3))),
                  strength=0.9, distance=0.0035, normal=base_normal)
    tree.links.new(bump, bsdf.inputs["Normal"])
    return mat


# ------------------------------------------------------------------ driftwood

def driftwood_mat(name, knots=(), seed=0.0, length=0.6, warmth=0.0):
    """Sea-bleached driftwood (no bark): silver-grey, salt-and-sun bleached surface over
    warmer tan wood where it was scuffed; the soft earlywood eroded so the latewood stands in
    fine raised lines along the grain; long checks along the grain filled with grey grime
    and sand; dark weathered knots. pcoord = (x, y across, s along the stick) in meters.
    ``knots``: [(x, y, s, radius)] in pcoord space."""
    g = Graph(name)
    p = g.coord()
    x, y, s = g.separate(p)
    ps = g.vmath("ADD", p, (seed * 0.31, seed * 0.17, seed * 0.53))
    swirl = None
    knot_mask = None
    for kx, ky, ks, kr in knots:
        d = g.vmath("DISTANCE", p, (kx, ky, ks))
        m = g.remap(d, kr * 3.0, kr * 0.4)
        knot_mask = m if knot_mask is None else g.math("MAXIMUM", knot_mask, m)
        core = g.remap(d, kr * 1.0, kr * 0.65)
        swirl = core if swirl is None else g.math("MAXIMUM", swirl, core)
    # Grain coordinate: angle round the stick (lines run along s), wandering slowly and
    # swept aside round the knots.
    ang = g.math("ARCTAN2", y, x)
    wander = g.noise(g.combine(g.math("MULTIPLY", s, 4.0), seed, 0.0), scale=1.0, detail=3.0).outputs["Fac"]
    grain_u = g.math("ADD", ang, g.math("MULTIPLY", wander, 1.2))
    if knot_mask is not None:
        grain_u = g.math("ADD", grain_u, g.math("MULTIPLY", knot_mask, 1.6))
    lines = g.wave(g.combine(g.math("MULTIPLY", grain_u, 5.0), g.math("MULTIPLY", s, 0.6), seed), scale=1.0,
                   distortion=0.25, detail=1.0, kind="BANDS", direction="X").outputs["Fac"]
    fade = g.noise(g.combine(g.math("MULTIPLY", grain_u, 30.0), g.math("MULTIPLY", s, 8.0), seed + 4.0),
                   scale=1.0, detail=2.0).outputs["Fac"]
    lines = g.math("MULTIPLY", lines, g.remap(fade, 0.3, 0.6, 0.35, 1.0))
    fib = g.noise(g.combine(g.math("MULTIPLY", grain_u, 40.0), g.math("MULTIPLY", s, 6.0), seed + 1.0),
                  scale=1.0, detail=3.0, roughness=0.5).outputs["Fac"]
    ridges = g.math("ADD", g.math("MULTIPLY", g.math("POWER", lines, 2.0), 0.6), g.math("MULTIPLY", fib, 0.4))
    # Checks: long dark splits along the grain that open and close along their length.
    split = g.noise(g.combine(g.math("MULTIPLY", grain_u, 2.2), g.math("MULTIPLY", s, 1.4), seed + 2.0),
                    scale=1.0, detail=2.0).outputs["Fac"]
    run = g.noise(g.combine(g.math("MULTIPLY", grain_u, 1.5), g.math("MULTIPLY", s, 5.0), seed + 7.0),
                  scale=1.0, detail=2.0).outputs["Fac"]
    crack = g.math("MULTIPLY", g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", split, 0.5)), 0.012, 0.0),
                   g.remap(run, 0.5, 0.62))
    bleach = g.noise(ps, scale=6.0, detail=4.0).outputs["Fac"]
    silver = (0.25 - 0.02 * warmth, 0.245 - 0.03 * warmth, 0.23 - 0.045 * warmth)
    color = g.mix(tuple(c * 0.68 for c in silver), silver, g.remap(bleach, 0.35, 0.7))
    warm = g.noise(ps, scale=3.5, detail=3.0, roughness=0.6).outputs["Fac"]
    color = g.mix(color, (0.23, 0.18, 0.125), g.math("MULTIPLY", g.remap(warm, 0.5 - 0.06 * warmth, 0.66), 0.85))
    color = g.mix(color, (0.06, 0.056, 0.05), g.math("MULTIPLY", g.math("SUBTRACT", 1.0, ridges), 0.6))
    color = g.mix(color, (1.15, 1.14, 1.12), g.math("MULTIPLY", g.remap(lines, 0.75, 0.95), 0.6), blend="MULTIPLY")
    # Long tonal streaks along the grain, a sun-bleached top and a damp, sand-stained underside
    # (pcoord y is the stick's up side), and grimy worn ends.
    streak = g.noise(g.combine(g.math("MULTIPLY", grain_u, 1.6), g.math("MULTIPLY", s, 0.9), seed + 11.0),
                     scale=1.0, detail=3.0).outputs["Fac"]
    color = g.mix(color, (1.2, 1.18, 1.14), g.math("MULTIPLY", g.remap(streak, 0.55, 0.75), 0.5), blend="MULTIPLY")
    color = g.mix(color, (0.62, 0.58, 0.52), g.math("MULTIPLY", g.remap(streak, 0.42, 0.25), 0.6), blend="MULTIPLY")
    up = g.math("DIVIDE", y, g.math("MAXIMUM", g.math("SQRT", g.math("ADD", g.math("MULTIPLY", x, x),
                                                                       g.math("MULTIPLY", y, y))), 0.002))
    color = g.mix(color, (1.12, 1.11, 1.09), g.remap(up, 0.2, 0.9, 0.0, 0.7), blend="MULTIPLY")
    color = g.mix(color, (0.55, 0.50, 0.42), g.remap(up, -0.3, -0.9, 0.0, 0.8), blend="MULTIPLY")
    end = g.math("MINIMUM", s, g.math("ABSOLUTE", g.math("SUBTRACT", length, s)))
    color = g.mix(color, (0.6, 0.56, 0.5), g.remap(end, 0.05, 0.0, 0.0, 0.7), blend="MULTIPLY")
    mildew = g.noise(ps, scale=45.0, detail=5.0, roughness=0.7).outputs["Fac"]
    color = g.mix(color, (0.05, 0.048, 0.045), g.math("MULTIPLY", g.remap(mildew, 0.64, 0.8), 0.6))
    grime = g.mix((0.03, 0.028, 0.025), SAND, g.remap(g.noise(ps, scale=300.0).outputs["Fac"], 0.55, 0.7))
    color = g.mix(color, grime, crack)
    rough = g.remap(ridges, 0.2, 0.9, 0.92, 0.78)
    if knot_mask is not None:
        knot_col = g.mix((0.06, 0.05, 0.04), (0.12, 0.10, 0.08), g.remap(lines, 0.3, 0.8))
        color = g.mix(color, knot_col, swirl)
        color = g.mix(color, (0.8, 0.77, 0.72), g.math("MULTIPLY", knot_mask, 0.35), blend="MULTIPLY")
        rough = _fmix(g, rough, 0.62, swirl)
    height = g.math("SUBTRACT", g.math("MULTIPLY", ridges, 0.7), g.math("MULTIPLY", crack, 2.5))
    if swirl is not None:
        height = g.math("ADD", height, g.math("MULTIPLY", swirl, 0.5))
    return _finish(g, color, rough, height, strength=0.8, distance=0.0012)
