"""Big bleached driftwood trunk with a broken root plate, half-sunk in beach sand (landmark scenery).

The real object (what the geometry and textures follow):
- Storm-felled trees reach Cornish/Devon granite coves from cliff-edge woods and from the Atlantic
  (spruce, pine, oak, elm). A beached trunk 3-4 m long and 30-45 cm thick weighs 150-300 kg and lies
  where the spring tides left it, settled into the sand to about half its thickness, its heavy root
  end lower and the thin end resting on the surface.
- Bark is gone within a season afloat. Sun (UV photodegradation of lignin), salt and rain leave the
  wood silver-grey: weathered grey wood is ~0.25-0.40 linear albedo, palest on the sand-blasted ridges,
  never white. Sand and waves erode the soft earlywood faster than the latewood, so the surface is
  smooth to the touch but ribbed along the (often spiral) grain; knots are harder and stand as
  rounded bosses.
- Drying opens long radial seasoning checks, V-shaped and rounded at the lips, a few centimetres deep
  on a 40 cm trunk, following the spiral grain and closing toward the ends. Sand packs into them.
- The root plate is the giveaway of a fallen tree: the bole flares to ~50 cm and several root arms
  (8-25 cm thick where they leave it) snap off 0.3-0.8 m out and are worn to blunt, rounded stubs.
  The top end usually broke at a weak point and is worn to a ragged, rounded break with a few
  splinters still standing proud.
- Gribble (Limnoria) and shipworm (Teredo) bore millimetre pinholes and small galleries into wood
  that has floated a long time; the band touching damp sand is darker, and salt-spray orange
  Xanthoria lichen colonises the highest, driest root tops of long-beached logs.

Geometry: a signed-distance trunk + root plate (homestead_sdf), meshed at 5 mm, displaced in each
branch's own grain frame (spiral latewood ridges, wave-worn dips, seasoning checks, knots) for the
high mesh; the game mesh is decimated from the macro surface and the high mesh's shading is baked
onto it (homestead_highbake). Authoring axes: the trunk runs along X (root plate at -X, thin end at
+X), the ground line is z = 0 at the origin, pivot at the bottom centre of the visible footprint;
everything below z = -0.10 is cut flat (buried). Original procedural geometry and materials.
"""
import math
import os
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_highbake as hb  # noqa: E402
import homestead_rocks as rocks  # noqa: E402
import homestead_sdf as sdf  # noqa: E402

NAME = "DriftwoodLog"
DESCRIPTION = ("A 3.6 m bleached driftwood trunk, 34-45 cm thick, with a broken root plate, half-sunk in "
               "beach sand: silver-grey, spiral-ribbed, deeply checked and worn smooth. Landmark scenery.")
COLLISION = "capsule"
TRIANGLE_BUDGET = 15000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = None  # baked high-to-low inside build() (homestead_highbake)
BAKE_SIZE = 1024 if DRAFT else 4096
BAKE_SAMPLES = 16 if DRAFT else 64
VOXEL = 0.010 if DRAFT else 0.004
GROUND_CUT = -0.10        # m: everything below is buried and cut flat
LOD_TRIS = (12000, 4800, 1700)
BEAUTY = {"pose": (0, 0, 0), "focus": (0.15, 0.02, 0.17), "ground": "origin",
          "views": ["hero", "detail", "eye"], "eye_distance": 4.6}
REPORT = {}
NOTES = {}
SEED = 51851


# ------------------------------------------------------------------ skeleton

class Branch:
    def __init__(self, name, pts, radii, twist, seed, checks=(), knots=(), fine=1.0):
        self.name = name
        self.pts = np.asarray(pts, dtype=np.float64)
        self.radii = np.asarray(radii, dtype=np.float64)
        self.twist = twist
        self.seed = seed
        self.checks = list(checks)      # (phi, s0, s1, depth, width)
        self.knots = list(knots)        # (s, phi, radius, height)
        self.fine = fine
        seg = np.linalg.norm(np.diff(self.pts, axis=0), axis=1)
        self.acc = np.concatenate([[0.0], np.cumsum(seg)])
        tangents = np.diff(self.pts, axis=0) / seg[:, None]
        helper = np.array([0.0, 0.0, 1.0]) if abs(tangents[0][2]) < 0.9 else np.array([1.0, 0.0, 0.0])
        n = np.cross(tangents[0], helper)
        n /= np.linalg.norm(n)
        normals = [n]
        for i in range(1, len(tangents)):
            a, b = tangents[i - 1], tangents[i]
            n = n - b * n.dot(b)
            n /= np.linalg.norm(n)
            normals.append(n)
        self.tangents = tangents
        self.normals = np.array(normals)
        self.binormals = np.cross(tangents, self.normals)
        # Frames at the skeleton points (mean of the two segments' frames), interpolated along each
        # segment in coords(): a per-segment frame jumps where the axis bends (the root flare) and
        # shifts the rib phase at every segment boundary into a dashed brick pattern.
        pn = np.concatenate([self.normals[:1], self.normals[:-1] + self.normals[1:], self.normals[-1:]])
        self.point_normals = pn / np.linalg.norm(pn, axis=1, keepdims=True)

    @property
    def length(self):
        return self.acc[-1]

    def node(self, k=0.012):
        return sdf.tube([tuple(p) for p in self.pts], list(self.radii), k=k)

    def coords(self, P):
        """Per point: surface distance estimate, arclength s, angle theta, local radius, cap factor."""
        best = np.full(len(P), 1e9)
        best_sel = np.full(len(P), 1e9)
        s = np.zeros(len(P))
        th = np.zeros(len(P))
        rr = np.zeros(len(P))
        cap = np.zeros(len(P))
        seg = np.zeros(len(P), dtype=np.int64)
        sx = np.zeros(len(P))
        last = len(self.tangents) - 1
        for j, (a, t) in enumerate(zip(self.pts[:-1], self.tangents)):
            length = self.acc[j + 1] - self.acc[j]
            raw = (P - a) @ t
            u = np.clip(raw / length, 0.0, 1.0)
            q = a + np.outer(u * length, t)
            v = P - q
            dist = np.linalg.norm(v, axis=1)
            r = self.radii[j] + u * (self.radii[j + 1] - self.radii[j])
            d = dist - r
            # Pick the segment by distance to the axis, not by dist - r: on a taper (the root flare) the
            # fatter previous segment, clipped at its end, wins by dist - r for points over the next one,
            # which froze s and theta into 7.5 cm bands and broke the ribs into dashed bricks.
            sel = dist
            better = sel < best_sel
            best_sel = np.where(better, sel, best_sel)
            if not better.any():
                continue
            best = np.where(better, d, best)
            s = np.where(better, self.acc[j] + u * length, s)
            nu = (1.0 - u)[:, None] * self.point_normals[j] + u[:, None] * self.point_normals[j + 1]
            nu = nu - np.outer(nu @ t, t)
            nu /= np.maximum(np.linalg.norm(nu, axis=1, keepdims=True), 1e-9)
            bu = np.cross(t, nu)
            ang = np.arctan2(np.einsum("ij,ij->i", v, bu), np.einsum("ij,ij->i", v, nu))
            th = np.where(better, ang, th)
            rr = np.where(better, r, rr)
            seg = np.where(better, j, seg)
            beyond = np.zeros(len(P))
            if j == 0:
                beyond = np.maximum(beyond, -raw)
            if j == last:
                beyond = np.maximum(beyond, raw - length)
            axial = np.clip(beyond / np.maximum(dist, 1e-6), 0.0, 1.0)
            cap = np.where(better, axial, cap)
            ext = self.acc[j] + raw
            ext = np.where((j == 0) & (raw < 0), raw, np.where((j == last) & (raw > length), ext, self.acc[j] + u * length))
            sx = np.where(better, ext, sx)
        return best, s, th, rr, cap, seg, sx


def _wrap(a):
    return (a + np.pi) % (2 * np.pi) - np.pi


_GRADS = np.array([[1, 1, 0], [-1, 1, 0], [1, -1, 0], [-1, -1, 0], [1, 0, 1], [-1, 0, 1], [1, 0, -1], [-1, 0, -1],
                   [0, 1, 1], [0, -1, 1], [0, 1, -1], [0, -1, -1], [1, 1, 0], [-1, 1, 0], [0, -1, 1], [0, -1, -1]],
                  dtype=np.float64)


def perlin(p, seed=0):
    """Vectorised 3D gradient noise in about [-1, 1]. Value noise (homestead_sdf.noise) creases along
    its lattice when ridged, which reads as brickwork on stretched wood grain; this doesn't."""
    cell = np.floor(p).astype(np.int64)
    f = p - cell
    u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0)
    out = np.zeros(len(p))
    s = np.int64((seed * 2654435761) & 0x7FFFFFFF)
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                h = (cell[:, 0] + dx) * 73856093 ^ (cell[:, 1] + dy) * 19349663 ^ (cell[:, 2] + dz) * 83492791 ^ s
                h = (h ^ (h >> 13)) * 1274126177
                g = _GRADS[(h ^ (h >> 16)) & 15]
                d = f - np.array([dx, dy, dz], dtype=np.float64)
                w = (u[:, 0] if dx else 1 - u[:, 0]) * (u[:, 1] if dy else 1 - u[:, 1]) * (u[:, 2] if dz else 1 - u[:, 2])
                out += w * np.einsum("ij,ij->i", g, d)
    return out * 1.4


_ROT = [np.linalg.qr(np.random.default_rng(900 + i).standard_normal((3, 3)))[0] for i in range(6)]


def pfbm(p, octaves=3, seed=0):
    """Gradient-noise fBm, every octave on a rotated, offset lattice (gradient noise is zero on its
    lattice, which shows as a grid when it is ridged or stretched along the grain)."""
    out, amp, total = np.zeros(len(p)), 1.0, 0.0
    q = p @ _ROT[seed % len(_ROT)] + 0.37 * (seed % 7)
    for i in range(octaves):
        out += amp * perlin(q, seed + 31 * i)
        total += amp
        amp *= 0.5
        q = (q @ _ROT[(i + 1) % len(_ROT)]) * 2.03
    return out / total


def trunk_branch(rng):
    pts, radii = [], []
    n = 46
    for i in range(n + 1):
        t = i / n
        r = 0.205 * (1.0 + 0.30 * math.exp(-t / 0.09)) * (1.0 - 0.42 * t ** 1.25) * (1.0 + 0.035 * math.sin(t * 17 + 1.0))
        x = -1.50 + 3.45 * t
        y = 0.11 * math.sin(math.pi * t * 1.05) - 0.06 * t
        z = -0.035 + 0.24 * r + 0.018 * math.sin(t * 5.0 + 0.4) + 0.02 * t
        pts.append((x, y, z))
        radii.append(r)
    checks = []
    for k in range(10):
        phi = k * 2 * math.pi / 10 + rng.uniform(-0.25, 0.25)
        s0 = rng.uniform(0.1, 1.0)
        s1 = min(3.35, s0 + rng.uniform(1.1, 2.5))
        checks.append((phi, s0, s1, rng.uniform(0.022, 0.048), rng.uniform(0.008, 0.019)))
    for k in range(12):
        phi = rng.uniform(-math.pi, math.pi)
        s0 = rng.uniform(0.2, 3.0)
        checks.append((phi, s0, min(3.4, s0 + rng.uniform(0.2, 0.7)), rng.uniform(0.006, 0.016),
                       rng.uniform(0.003, 0.007)))
    knots = [(rng.uniform(0.5, 3.1), rng.uniform(-math.pi, math.pi), rng.uniform(0.03, 0.05), 0.006)
             for _ in range(5)]
    return Branch("trunk", pts, radii, twist=0.30, seed=11, checks=checks, knots=knots)


def root_branches(trunk, rng):
    """The broken root plate: thick roots radiating roughly in the plane across the trunk end and
    turning back away from it, snapped 0.4-1 m out and worn to blunt stubs; two sink into the sand."""
    base = trunk.pts[0]
    r0 = trunk.radii[0]
    axis = trunk.tangents[0]
    # (angle in the YZ plane from +Y toward +Z, backward lean, length m, base radius m)
    specs = [(84, 0.30, 0.92, 0.17), (122, 0.24, 0.56, 0.15), (46, 0.42, 0.72, 0.15),
             (152, 0.20, 0.42, 0.14), (8, 0.30, 0.64, 0.14), (-30, 0.40, 0.50, 0.13),
             (205, 0.40, 0.50, 0.13), (100, 1.00, 0.30, 0.12), (30, 0.95, 0.26, 0.11),
             (170, 0.90, 0.28, 0.11)]
    out = []
    for i, (deg, back, length, rb) in enumerate(specs):
        a = math.radians(deg + rng.uniform(-5, 5))
        radial = np.array([0.0, math.cos(a), math.sin(a)])
        d = radial - axis * back
        d /= np.linalg.norm(d)
        start = base + radial * r0 * 0.30 + axis * rng.uniform(0.02, 0.12)
        turn = -axis * rng.uniform(0.15, 0.35) + np.array([0.0, 0.0, rng.uniform(-0.18, 0.05)])
        pts, radii = [], []
        steps = 8
        for k in range(steps + 1):
            u = k / steps
            p = start + d * length * u + turn * length * u * u
            if k:
                p = p + np.array([rng.uniform(-0.012, 0.012), rng.uniform(-0.015, 0.015),
                                  rng.uniform(-0.015, 0.015)])
            pts.append(p)
            radii.append(rb * (1.0 - 0.70 * u ** 0.55) * rng.uniform(0.9, 1.1))
        checks = [(rng.uniform(-math.pi, math.pi), 0.08, length * rng.uniform(0.55, 0.9), rng.uniform(0.010, 0.022),
                   rng.uniform(0.005, 0.010)) for _ in range(3)]
        knots = [(rng.uniform(0.2, length * 0.8), rng.uniform(-math.pi, math.pi), 0.025, 0.003)]
        out.append(Branch(f"root{i}", pts, radii, twist=rng.uniform(-0.9, 0.9), seed=40 + i, checks=checks,
                          knots=knots))
    # Short broken rootlets on the big visible roots.
    for i, parent in enumerate((out[0], out[2], out[4])):
        u = rng.uniform(0.35, 0.65)
        j = int(u * (len(parent.pts) - 1))
        p = parent.pts[j]
        ang = rng.uniform(0, 2 * math.pi)
        side = parent.normals[j] * math.cos(ang) + parent.binormals[j] * math.sin(ang)
        d = side * 0.85 + parent.tangents[j] * 0.5
        d /= np.linalg.norm(d)
        rb = parent.radii[j] * rng.uniform(0.38, 0.5)
        length = rng.uniform(0.12, 0.24)
        pts = [p + d * length * u for u in (0.0, 0.33, 0.66, 1.0)]
        out.append(Branch(f"rootlet{i}", pts, [rb, rb * 0.85, rb * 0.7, rb * 0.55], twist=0.3, seed=70 + i))
    return out


def stub_branches(trunk, rng):
    """One branch base worn back to a low boss, the grain swirling round it."""
    out = []
    for i, (u, deg, length, rb) in enumerate(((0.56, 70, 0.03, 0.065),)):
        j = int(u * (len(trunk.pts) - 1))
        a = math.radians(deg)
        radial = trunk.normals[j] * math.cos(a) + trunk.binormals[j] * math.sin(a)
        radial = np.array([0.0, radial[1], abs(radial[2]) + 0.3])
        radial /= np.linalg.norm(radial)
        d = radial + trunk.tangents[j] * 0.5
        d /= np.linalg.norm(d)
        p = trunk.pts[j] + radial * trunk.radii[j] * 0.5
        reach = trunk.radii[j] * 0.5 + length
        pts = [p, p + d * reach * 0.5, p + d * reach]
        out.append(Branch(f"stub{i}", pts, [rb, rb * 0.95, rb * 0.85], twist=0.0, seed=90 + i, fine=0.7))
    return out


def splinter_branches(trunk, rng, cut_point, axis):
    out = []
    j = len(trunk.pts) - 2
    for i in range(4):
        a = rng.uniform(-math.pi, math.pi)
        off = (trunk.normals[j] * math.cos(a) + trunk.binormals[j] * math.sin(a)) * trunk.radii[-1] * rng.uniform(0.1, 0.75)
        d = axis + np.array([rng.uniform(-0.10, 0.10), rng.uniform(-0.10, 0.10), rng.uniform(-0.04, 0.10)])
        d /= np.linalg.norm(d)
        p = cut_point - axis * 0.08 + off
        length = rng.uniform(0.07, 0.15)
        rb = rng.uniform(0.016, 0.032)
        pts = [p + d * length * u for u in (0.0, 0.3, 0.6, 0.85, 1.0)]
        out.append(Branch(f"splinter{i}", pts, [rb, rb * 0.85, rb * 0.65, rb * 0.45, rb * 0.32], twist=0.0,
                          seed=120 + i, fine=0.35))
    return out


def body_sdf(trunk, roots, stubs, splinters, cut_point, cut_normal, axis):
    t_node = trunk.node(k=0.02)
    lo = cut_point - 0.6
    hi = cut_point + 0.6
    cutter = sdf.halfspace(tuple(cut_point), tuple(-cut_normal), tuple(lo), tuple(hi))

    def ragged(p):
        # A torn break: splintery ridges along the grain, jagged across it.
        q = p - cut_point
        along = q @ axis
        perp = q - np.outer(along, axis)
        v = perp * 26.0 + np.outer(along * 3.0, axis)
        return 0.055 * pfbm(v, 3, 777) + 0.02 * pfbm(p * 9.0, 2, 778)
    cutter = sdf.displaced(cutter, ragged, 0.08)
    t_node = sdf.subtract(t_node, cutter, k=0.012)
    body = sdf.smooth_union([t_node] + [r.node(0.012) for r in roots[:10]], k=0.12)
    body = sdf.smooth_union([body] + [r.node(0.008) for r in roots[10:]] + [s.node(0.008) for s in stubs], k=0.035)
    body = sdf.smooth_union([body] + [s.node(0.004) for s in splinters], k=0.022)
    glo = body.lo - 0.1
    ghi = body.hi + 0.1
    ground = sdf.halfspace((0.0, 0.0, GROUND_CUT), (0.0, 0.0, -1.0), tuple(glo), tuple(ghi))
    return sdf.intersect(body, ground, k=0.012)


# ------------------------------------------------------------------ displacement

def branch_fields(br, s, th, r, cap, seg):
    """Displacements and material attributes for points in this branch's grain frame."""
    seed = br.seed
    tg = th + br.twist * s
    cu, su = np.cos(tg), np.sin(tg)
    scale = np.clip(r / 0.19, 0.25, 1.4)
    keep = 1.0 - cap ** 2
    q = np.stack([cu * 0.9, su * 0.9, s * 0.55], axis=1)
    macro = 0.020 * scale * pfbm(q, 3, seed) * keep
    # Spiral latewood ribs: a whole number of growth layers round the branch (so they close on
    # themselves and crowd together where it tapers), meandering and merging, the soft earlywood
    # between them scooped out by sand; broader wave-worn hollows over them.
    stripes = max(12, round(2 * math.pi * float(np.mean(br.radii)) / 0.016))
    warp = pfbm(np.stack([cu * 1.1, su * 1.1, s / 0.45], axis=1), 3, seed + 3)
    phase = tg * stripes / (2 * math.pi) + 2.6 * warp
    fade = np.clip(0.55 + 0.9 * pfbm(np.stack([cu * 2.0, su * 2.0, s / 0.12], axis=1), 2, seed + 4), 0.0, 1.0)
    ridge = ((0.5 + 0.5 * np.cos(2 * np.pi * phase)) ** 3) * fade
    q3 = np.stack([cu * r / 0.045, su * r / 0.045, s / 0.35], axis=1)
    dips = pfbm(q3, 3, seed + 5)
    relief = 0.0055 * dips
    cav = np.clip((0.0012 - relief - 0.0036 * ridge) / 0.0065, 0.0, 1.0) * keep
    fine = (relief - 0.0012) * keep * br.fine
    ribs = 0.0036 * ridge * keep * br.fine
    check = np.zeros(len(s))
    for phi, s0, s1, depth, width in br.checks:
        e = np.clip((s - s0) / (s1 - s0), 0.0, 1.0)
        env = np.sin(np.pi * e) ** 0.55
        wob = 0.22 * pfbm(np.stack([s * 1.4, np.full(len(s), phi * 3.0), np.zeros(len(s))], axis=1), 2,
                             seed + int(phi * 100) % 997)
        arc = np.abs(_wrap(tg - phi - wob)) * r
        w = width * (0.35 + 0.65 * env) + 1e-5
        prof = np.clip(1.0 - arc / w, 0.0, 1.0)
        lip = np.clip(1.0 - arc / (w * 2.6), 0.0, 1.0)
        d = depth * env * (prof ** 1.6 + 0.2 * lip ** 2) * keep
        check = np.maximum(check, d)
    knot = np.zeros(len(s))
    for ks, kphi, kr, kh in br.knots:
        dist = np.hypot(_wrap(tg - kphi) * r, (s - ks) * 0.7)
        knot = np.maximum(knot, np.exp(-(dist / kr) ** 2))
        fine = fine + kh * np.exp(-(dist / (kr * 1.2)) ** 2) * keep
    fine = fine - check
    grain = np.stack([cu * r, su * r, s + seed * 3.7], axis=1)
    return dict(macro=macro, fine=fine, ribs=ribs, check=np.clip(check / 0.03, 0.0, 1.0), ridge=ridge * keep,
                knot=knot, cav=cav, grain=grain, cap=cap)


def surface_fields(P, branches):
    coords = [br.coords(P) for br in branches]
    dists = np.stack([c[0] for c in coords], axis=1)
    order = np.argsort(dists, axis=1)[:, :2]
    rows = np.arange(len(P))
    d0 = dists[rows, order[:, 0]]
    d1 = dists[rows, order[:, 1]]
    w1 = np.exp(-(d1 - d0) / 0.012)
    weights = (1.0 / (1.0 + w1), w1 / (1.0 + w1))
    out = {k: np.zeros(len(P)) for k in ("macro", "fine", "ribs", "check", "ridge", "knot", "cav", "cap")}
    out["grain"] = np.zeros((len(P), 3))
    for slot in range(2):
        idx = order[:, slot]
        w = weights[slot]
        for b, br in enumerate(branches):
            sel = idx == b
            if not sel.any():
                continue
            _, s, th, r, cap, seg, _ = (c[sel] for c in coords[b])
            f = branch_fields(br, s, th, r, cap, seg)
            grain = f.pop("grain")
            if slot == 0:
                out["grain"][sel] = grain
            for key, value in f.items():
                if key in ("ribs", "ridge"):
                    # Ribs come from the nearest branch only: a weighted second rib set at another
                    # angle beats against the first into a dashed brick moire across the root flare.
                    if slot == 0:
                        out[key][sel] += value
                    continue
                out[key][sel] += w[sel] * value
    # Where two branches meet the nearest-branch ribs would jump at the seam; fade them out across
    # the junction instead (the flare there is wave-worn smooth anyway).
    sep = np.clip((d1 - d0) / 0.03, 0.0, 1.0)
    calm = 0.08 + 0.92 * sep * sep * (3.0 - 2.0 * sep)
    out["fine"] += out.pop("ribs") * calm
    out["ridge"] *= calm
    out["calm"] = calm
    return out


# ------------------------------------------------------------------ material

def driftwood_material(kit, name, height_scale=1.0):
    """``height_scale`` shrinks the height bands (damp foot, sand dust, lichen tops) for small pieces
    such as DriftwoodSticks; 1.0 is tuned to the 1 m tall log."""
    g = kit.mats.Graph(name)

    def attr(n, socket="Fac"):
        return g.node("ShaderNodeAttribute", attribute_name=n, attribute_type="GEOMETRY").outputs[socket]

    check, ridge, knot, cav, cap = attr("dw_check"), attr("dw_ridge"), attr("dw_knot"), attr("dw_cav"), attr("dw_cap")
    calm = attr("dw_calm")
    grain = g.coord()
    gx, gy, gz = g.separate(grain)
    pos = g.node("ShaderNodeTexCoord").outputs["Object"]
    px, py, pz = g.separate(pos)
    if height_scale != 1.0:
        pz = g.math("DIVIDE", pz, height_scale)
    normal_z = g.separate(g.node("ShaderNodeNewGeometry").outputs["Normal"])[2]

    def along_grain(across, along):
        """Grain coordinates (pcoord: round the branch in metres, arclength along it) scaled
        ``across`` per metre across the grain and ``along`` along it."""
        return g.combine(g.math("MULTIPLY", gx, across), g.math("MULTIPLY", gy, across), g.math("MULTIPLY", gz, along))

    fibres = g.noise(along_grain(520.0, 9.0), scale=1.0, detail=6.0, roughness=0.6).outputs["Fac"]
    streak = g.noise(along_grain(45.0, 1.4), scale=1.0, detail=4.0, roughness=0.55).outputs["Fac"]
    blotch = g.noise(pos, scale=1.3, detail=4.0, roughness=0.55).outputs["Fac"]
    weather = g.noise(pos, scale=0.38, detail=3.0, roughness=0.5).outputs["Fac"]
    # Fibres fade to plain blotching on end caps and across branch junctions (grain seams there).
    plain = g.math("MAXIMUM", cap, g.math("SUBTRACT", 1.0, calm))
    fibres = g.node("ShaderNodeMix", data_type="FLOAT", Factor=plain, A=fibres, B=0.5).outputs[0]
    streak = g.node("ShaderNodeMix", data_type="FLOAT", Factor=plain, A=streak, B=0.5).outputs[0]
    fib_c = g.node("ShaderNodeMix", data_type="FLOAT", Factor=cap, A=fibres, B=blotch).outputs[0]
    tone = g.math("ADD", g.math("ADD", g.math("MULTIPLY", blotch, 0.32), g.math("MULTIPLY", weather, 0.22)),
                  g.math("ADD", g.math("MULTIPLY", fib_c, 0.30), g.math("MULTIPLY", streak, 0.16)))
    tone = g.math("ADD", tone, g.math("SUBTRACT", g.math("MULTIPLY", ridge, 0.22), g.math("MULTIPLY", cav, 0.30)))
    color = g.ramp(tone, [(0.18, (0.085, 0.078, 0.070)), (0.36, (0.160, 0.152, 0.138)),
                          (0.54, (0.235, 0.224, 0.204)), (0.72, (0.295, 0.282, 0.258)),
                          (0.90, (0.350, 0.336, 0.308))])
    stain = g.remap(weather, 0.52, 0.72, 0.0, 0.75)
    color = g.mix(color, (0.150, 0.128, 0.104), g.math("MULTIPLY", stain, g.remap(blotch, 0.3, 0.6)))
    warm = g.remap(g.noise(pos, scale=0.9, detail=2.0).outputs["Fac"], 0.5, 0.72, 0.0, 0.35)
    color = g.mix(color, (0.88, 0.80, 0.70), warm, blend="MULTIPLY")
    dark_streak = g.remap(streak, 0.64, 0.80, 0.0, 0.6)
    color = g.mix(color, (0.11, 0.10, 0.092), dark_streak)
    crack_col = g.mix((0.085, 0.076, 0.064), (0.036, 0.028, 0.020), g.remap(check, 0.5, 1.0))
    color = g.mix(color, crack_col, g.remap(check, 0.22, 0.65))
    # Hard knots: a warmer, darker core, no ring.
    color = g.mix(color, (0.17, 0.13, 0.09), g.math("MULTIPLY", g.remap(knot, 0.35, 0.95), 0.55))
    # Fine dark grain lines where the long fibre noise crosses its mean.
    lines = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", fib_c, 0.5)), 0.0, 0.035, 1.0, 0.0)
    color = g.mix(color, (0.13, 0.12, 0.11), g.math("MULTIPLY", lines, 0.35))
    # Gribble and shipworm pinholes in loose colonies, two sizes.
    holes = g.voronoi(pos, scale=70.0).outputs["Distance"]
    small = g.voronoi(pos, scale=190.0).outputs["Distance"]
    colony = g.remap(g.noise(pos, scale=2.2, detail=3.0).outputs["Fac"], 0.55, 0.66)
    hole = g.math("MULTIPLY", g.math("MAXIMUM", g.remap(holes, 0.05, 0.10, 1.0, 0.0),
                                     g.remap(small, 0.08, 0.15, 0.8, 0.0)), colony)
    color = g.mix(color, (0.028, 0.024, 0.020), hole)
    # Damp band where it lies in the sand; sand packed into checks and dusted on the low flanks.
    wobble = g.math("MULTIPLY", g.math("SUBTRACT", g.noise(pos, scale=6.0, detail=3.0).outputs["Fac"], 0.5), 0.06)
    zw = g.math("ADD", pz, wobble)
    damp = g.remap(zw, 0.07, -0.01)
    color = g.mix(color, (0.62, 0.58, 0.52), damp, blend="MULTIPLY")
    grains = g.voronoi(pos, scale=520.0).outputs["Distance"]
    grain_dots = g.remap(grains, 0.22, 0.38, 1.0, 0.0)
    sand_pockets = g.remap(g.noise(pos, scale=9.0, detail=3.0).outputs["Fac"], 0.5, 0.65)
    dust = g.math("MAXIMUM", g.math("MULTIPLY", g.math("MULTIPLY", g.remap(check, 0.75, 1.0), g.remap(pz, 0.3, 0.05)),
                                     g.math("MULTIPLY", sand_pockets, 0.5)),
                  g.math("MULTIPLY", g.remap(zw, 0.14, 0.02), 0.5))
    dust = g.math("MAXIMUM", dust, g.math("MULTIPLY", g.remap(cav, 0.55, 0.95), g.remap(zw, 0.30, 0.05)))
    sand_mask = g.math("MULTIPLY", dust, g.math("ADD", g.math("MULTIPLY", grain_dots, 0.6), 0.25), clamp=True)
    sand_col = g.mix((0.29, 0.245, 0.18), (0.40, 0.345, 0.255), g.noise(pos, scale=60.0).outputs["Fac"])
    color = g.mix(color, sand_col, sand_mask)
    # Xanthoria on the highest, upward-facing root tops only.
    up = g.math("MULTIPLY", g.remap(normal_z, 0.55, 0.9), g.remap(pz, 0.45, 0.75))
    thalli = g.voronoi(pos, scale=24.0).outputs["Distance"]
    lichen = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(thalli, 0.05, 0.17, 1.0, 0.0), up),
                    g.remap(g.noise(pos, scale=3.0).outputs["Fac"], 0.45, 0.6))
    color = g.mix(color, (0.40, 0.19, 0.035), lichen)
    g.set("Base Color", color)

    rough = g.remap(fibres, 0.3, 0.7, 0.80, 0.90)
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=g.math("MULTIPLY", ridge, 0.7), A=rough, B=0.70).outputs[0]
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=g.remap(check, 0.1, 0.6), A=rough, B=0.95).outputs[0]
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=damp, A=rough, B=0.66).outputs[0]
    g.set("Roughness", rough)
    g.set("Specular IOR Level", 0.42)
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fib_c, 0.6), g.math("MULTIPLY", streak, 0.25)),
                    g.math("ADD", g.math("MULTIPLY", hole, 1.2), g.math("MULTIPLY", grain_dots, g.math("MULTIPLY", sand_mask, -0.3))))
    g.set("Normal", g.bump(height, strength=0.55, distance=0.0010))
    g.bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return g.mat


# ------------------------------------------------------------------ build

def _set_attr(mesh, name, values, kind="FLOAT"):
    attr = mesh.attributes.get(name) or mesh.attributes.new(name, kind, "POINT")
    if kind == "FLOAT":
        attr.data.foreach_set("value", np.asarray(values, dtype=np.float32).ravel())
    else:
        attr.data.foreach_set("vector", np.asarray(values, dtype=np.float32).ravel())


def _decimate(obj, target):
    total = rocks.triangles(obj)
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.decimate_type = "COLLAPSE"
    mod.ratio = min(1.0, target / total)
    mod.use_collapse_triangulate = True
    depsgraph = bpy.context.evaluated_depsgraph_get()
    data = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
    obj.modifiers.clear()
    old = obj.data
    obj.data = data
    bpy.data.meshes.remove(old)
    return obj


def lay_uvs(obj, branches, offset, trunk_pieces=3):
    """Cylindrical UVs in metres from each branch's grain frame: u around (seam on the buried
    underside), v along; the trunk is cut into ``trunk_pieces`` lengths so nothing packs as a
    long diagonal sliver. Then packed."""
    P = rocks.coords(obj) + np.asarray(offset)
    coords = [br.coords(P) for br in branches]
    best = np.argmin(np.stack([c[0] for c in coords], axis=1), axis=1)
    up = np.array([0.0, 0.0, 1.0])
    per = []
    for b, br in enumerate(branches):
        _, s, th, r, cap, seg, sx = coords[b]
        thu = np.arctan2(br.binormals[seg] @ up, br.normals[seg] @ up)
        per.append((_wrap(th - thu), r, sx))
    mesh = obj.data
    layer = mesh.uv_layers.get("UVMap") or mesh.uv_layers.new(name="UVMap")
    trunk_len = branches[0].length
    for poly in mesh.polygons:
        verts = list(poly.vertices)
        b = int(np.bincount(best[verts], minlength=len(branches)).argmax())
        theta, r, sx = per[b]
        island = b * trunk_pieces
        if b == 0:
            island += int(np.clip(np.mean(sx[verts]) / (trunk_len / trunk_pieces), 0, trunk_pieces - 1))
        t0 = theta[verts[0]]
        for loop_index, v in zip(poly.loop_indices, verts):
            t = t0 + _wrap(theta[v] - t0)
            layer.data[loop_index].uv = (t * r[v] + island * 20.0, sx[v])
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(margin=0.003, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")


def build(kit):
    rng = np.random.default_rng(SEED)
    import random
    prng = random.Random(SEED)
    trunk = trunk_branch(prng)
    roots = root_branches(trunk, prng)
    stubs = []
    axis = trunk.tangents[-1]
    cut_point = trunk.pts[-1] - axis * 0.05
    tilt = np.array([0.0, 0.40, 0.55])
    cut_normal = axis + tilt
    cut_normal /= np.linalg.norm(cut_normal)
    splinters = splinter_branches(trunk, prng, cut_point, axis)
    branches = [trunk] + roots + stubs + splinters

    node = body_sdf(trunk, roots, stubs, splinters, cut_point, cut_normal, axis)
    base = sdf.mesh("DriftwoodHigh", node, voxel=VOXEL)
    mesh = base.data
    P = rocks.coords(base)
    N = rocks.normals(base)
    print("HOMESTEAD_DRIFT_HIGH_VERTS", len(P), flush=True)
    fields = surface_fields(P, branches)
    macro, fine = fields["macro"], fields["fine"]
    buried = np.clip((0.0 - P[:, 2]) / 0.05, 0.0, 1.0)
    macro *= 1.0 - buried
    fine *= 1.0 - 0.8 * buried
    rocks.set_coords(base, P + N * macro[:, None])

    low = base.copy()
    low.data = base.data.copy()
    low.name = low.data.name = "SM_" + NAME
    bpy.context.scene.collection.objects.link(low)
    _decimate(low, LOD_TRIS[0])

    rocks.set_coords(base, P + N * (macro + fine)[:, None])
    for key in ("check", "ridge", "knot", "cav", "cap", "calm"):
        _set_attr(mesh, "dw_" + key, fields[key])
    _set_attr(mesh, "pcoord", fields["grain"], "FLOAT_VECTOR")
    mesh.shade_smooth()
    mesh.update()
    base.data.materials.append(driftwood_material(kit, "M_DriftwoodLogProcedural"))

    # Pivot: bottom centre of the visible footprint (bounds above the cut), ground line at z = 0.
    lo, hi = kit.bounds(low)
    shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 0.0))
    for obj in (low, base):
        obj.data.transform(Matrix.Translation(-shift))
        obj.data.update()
    low["homestead_shift"] = list(shift)

    lay_uvs(low, branches, shift)
    low.data.materials.clear()
    kit.finalize(low, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
    low["homestead_shift"] = list(shift)
    folder = Path(__file__).resolve().parents[3] / "Assets" / "Props" / NAME / "Textures"
    material, info = hb.bake(kit, base, low, folder, NAME, size=BAKE_SIZE, samples=BAKE_SAMPLES,
                             extrusion=0.03, distance=0.14)
    bpy.data.objects.remove(base)

    meshes = [low]
    total = rocks.triangles(low)
    for index, target in enumerate(LOD_TRIS[1:], start=1):
        lod = rocks.lod(kit, low, f"SM_{NAME}_LOD{index}", min(1.0, target / total))
        kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
        lod.data.materials.clear()
        lod.data.materials.append(material)
        lod["homestead_shift"] = list(shift)
        meshes.append(lod)

    # Collision: one capsule along the trunk (she steps onto / over the thin end).
    a = Vector(trunk.pts[3]) - shift
    b = Vector(trunk.pts[-3]) - shift
    radius = 0.17
    a.z = b.z = 0.04
    centre = (a + b) / 2
    ax = (b - a).normalized()
    REPORT["capsule"] = {"center_cm": [round(centre.x * 100, 1), round(-centre.y * 100, 1), round(centre.z * 100, 1)],
                         "radius_cm": round(radius * 100, 1), "length_cm": round(((b - a).length) * 100, 1),
                         "axis": [round(ax.x, 4), round(-ax.y, 4), round(ax.z, 4)],
                         "note": "Unreal space (Y mirrored). Lies along the trunk axis, top ~21 cm above the "
                                 "sand, so the heroine can step onto or over it; the root plate has no blocker."}
    root_c = Vector(trunk.pts[0]) - shift
    REPORT["collision_extra"] = {"root_plate_box_cm": {
        "center": [round((root_c.x - 0.12) * 100, 1), round(-root_c.y * 100, 1), 35.0],
        "extent": [30.0, 62.0, 35.0]},
        "note": "Optional second blocker (half extents) if the importer gains multi-shape collision; "
                "not applied by import_props.py today."}
    REPORT["placement"] = ("Pivot is the ground line at the bottom centre of the visible footprint; place at sand "
                           "height. Trunk along X: root plate toward -X, thin broken end toward +X.")
    NOTES.update({"sink_depth_m": -GROUND_CUT, "bake": info,
                  "material": "Baked from a 5 mm displaced high mesh (homestead_highbake); M_PropTextured.",
                  "lods": {m.name: rocks.triangles(m) for m in meshes}})
    print("HOMESTEAD_LODS", NOTES["lods"], flush=True)
    return meshes
