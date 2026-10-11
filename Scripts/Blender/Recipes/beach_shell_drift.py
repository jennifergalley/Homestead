"""Beach shell drift: a flat tideline drift of whole and broken shells and shell grit (scenery).

The real thing (what the geometry and textures follow):
- On a sheltered south-west English cove the last high tide leaves a thin, wavy drift of empty
  shells along its mark, sorted by the swash: a band ~1 m long and 30-60 cm wide, mostly single
  valves lying convex side up (the stable position), some cupped up and holding sand, broken bits
  and a gritty litter of crushed shell between them.
- Common cockle (Cerastoderma edule): valves 2.5-5 cm, swollen, 22-28 radiating ribs crossed by
  growth lines, dirty white to buff and pale brown, white inside with a faint brown-purple stain.
- Blue mussel (Mytilus edulis): 3-7 cm on a beach, a pointed teardrop, smooth with fine growth
  lines; worn blue-black to violet outside, pearly blue-white inside with a dark violet margin.
- Common limpet (Patella vulgata): a ridged cone 3-5 cm, grey-buff with darker rays, apex worn pale;
  inside glossy with a horseshoe-shaped muscle scar.
- Common periwinkle (Littorina littorea): 2-3 cm, sturdy, dark grey-brown with fine spiral lines,
  apex worn pale. Thin pink and yellow tellins (Macoma balthica, ~2 cm) add the odd bright accent.

Geometry: every shell is a parametric surface (valves round their umbo, cones, a spiralled tube for
the winkles, sectors for fragments) built twice from the same placement - a dense, displaced high
mesh with ribs, growth lines and per-vertex colour, and a light game mesh - and the high one is
baked onto the low one (homestead_highbake, 2K). Shells lying convex-up show only their outer sheet,
its rim sunk 1.5-3 mm below z = 0; cupped ones carry inner and outer sheets and a rim.
Pivot bottom centre; ground line z = 0. Original procedural geometry and materials.
"""
import math
import os
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_highbake as hb  # noqa: E402
import homestead_rocks as rocks  # noqa: E402

NAME = "BeachShellDrift"
DESCRIPTION = ("A 1.2 x 0.6 m tideline drift of whole and broken cockle, mussel, limpet, periwinkle and tellin "
               "shells with shell grit. Ground-hugging scenery, no collision.")
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = None  # baked high-to-low inside build() (homestead_highbake)
BAKE_SIZE = 1024 if DRAFT else 2048
BEAUTY = {"pose": (0, 0, 0), "focus": (0.05, 0.0, 0.01), "ground": "origin", "detail_distance": 0.42,
          "detail_fstop": 11.0, "views": ["hero", "detail", "eye"], "eye_distance": 1.8}
REPORT = {"placement": "Ground line at z = 0 (pivot, bottom centre); shell rims sink 1.5-3 mm below it. "
                       "Long axis along X. Random yaw, 0.8-1.2 scale."}
NOTES = {"sink_depth_m": 0.003}
SEED = 18511
LOD_RATIOS = (0.5, 0.22)


# ------------------------------------------------------------------ shell surfaces

def outline_radius(theta, umbo, a, b, egg, wobble):
    """Distance from the umbo to an egg-shaped outline along direction theta (vectorised bisection)."""
    d = np.stack([np.cos(theta), np.sin(theta)], axis=1)
    lo = np.zeros(len(theta))
    hi = np.full(len(theta), 3.0 * max(a, b))
    for _ in range(36):
        mid = (lo + hi) * 0.5
        x = umbo[0] + d[:, 0] * mid
        y = umbo[1] + d[:, 1] * mid
        bb = b * (1.0 + egg * x / a)
        inside = (x / a) ** 2 + (y / np.maximum(bb, 1e-4)) ** 2 < 1.0
        lo = np.where(inside, mid, lo)
        hi = np.where(inside, hi, mid)
    r = lo
    if wobble:
        r = r * (1.0 + wobble[0] * np.sin(theta * wobble[1] + wobble[2]))
    return r


class Shell:
    """One valve-like surface over (theta, rho) around its umbo. ``kind`` picks the colour scheme."""

    def __init__(self, kind, rng, size):
        self.kind = kind
        self.rng = rng
        self.seed = rng.randint(0, 10 ** 6)
        if kind == "cockle":
            self.a, self.b = size * 0.5, size * 0.5 * rng.uniform(0.88, 0.98)
            self.umbo = (rng.uniform(-0.1, 0.1) * self.a, self.b * 0.55)
            self.egg = 0.05
            self.H = size * rng.uniform(0.30, 0.38)
            self.ribs, self.rib_amp = rng.randint(22, 27), size * 0.028
            self.profile = (2.0, 0.62)
        elif kind == "mussel":
            self.a, self.b = size * 0.5, size * 0.5 * rng.uniform(0.48, 0.56)
            self.umbo = (-self.a * 0.94, 0.0)
            self.egg = 0.45
            self.H = size * rng.uniform(0.16, 0.2)
            self.ribs, self.rib_amp = 0, 0.0
            self.profile = (2.2, 0.55)
        elif kind == "tellin":
            self.a, self.b = size * 0.5, size * 0.5 * rng.uniform(0.78, 0.86)
            self.umbo = (rng.uniform(-0.05, 0.05) * self.a, self.b * 0.6)
            self.egg = 0.0
            self.H = size * 0.14
            self.ribs, self.rib_amp = 0, 0.0
            self.profile = (2.0, 0.6)
        elif kind == "limpet":
            self.a, self.b = size * 0.5, size * 0.5 * rng.uniform(0.80, 0.88)
            self.umbo = (self.a * rng.uniform(0.12, 0.22), 0.0)
            self.egg = 0.08
            self.H = size * rng.uniform(0.28, 0.36)
            self.ribs, self.rib_amp = rng.randint(28, 36), size * 0.014
            self.profile = (1.0, 0.72)
        self.thick = max(0.0006, size * 0.025)

    def surface(self, theta, rho, inner=False):
        """Rest positions (n, 3) for outer (or inner) surface at (theta, rho); z up, rim at z = 0."""
        r = outline_radius(theta, self.umbo, self.a, self.b, self.egg, None)
        if inner:
            r = r * (1.0 - self.thick / max(self.a, 1e-4) * 1.2)
        x = self.umbo[0] + np.cos(theta) * r * rho
        y = self.umbo[1] + np.sin(theta) * r * rho
        p, q = self.profile
        if self.kind == "limpet":
            z = self.H * (1.0 - rho) ** q
        else:
            z = self.H * np.clip(1.0 - rho ** p, 0.0, 1.0) ** q
        if inner:
            z = z - self.thick
        return np.stack([x, y, z], axis=1)

    def relief(self, theta, rho):
        """Outer-surface displacement (m): radial ribs, growth checks, crenulated margin."""
        rng = np.random.default_rng(self.seed)
        out = np.zeros(len(theta))
        if self.ribs:
            if self.kind == "limpet":
                jitter = np.interp(theta % (2 * np.pi), np.linspace(0, 2 * np.pi, 9),
                                   rng.uniform(-0.5, 0.5, 9))
                rib = (0.5 + 0.5 * np.cos(theta * self.ribs + 1.5 * jitter)) ** 1.5
            else:
                # Rounded ribs with narrow flat-bottomed interspaces, a little uneven round the valve.
                wob = 0.25 * np.sin(theta * 3.0 + self.seed % 11)
                rib = np.clip(1.25 * (0.5 + 0.5 * np.cos(theta * self.ribs + wob)) ** 0.45, 0, 1)
            out += self.rib_amp * rib * np.clip(rho * 1.6, 0, 1)
        # Growth checks: irregular narrow ledges, crowded toward the margin. Each one recovers (a
        # groove with a sharp shoulder) so the sheet never sinks through a cupped valve's inner face.
        bands = np.sort(1.0 - rng.uniform(0.0, 0.85, 11) ** 1.6)
        for k, bnd in enumerate(bands):
            w = rng.uniform(0.006, 0.012)
            depth = self.H * rng.uniform(0.012, 0.025) * (1.0 + 0.6 * bnd)
            local = bnd + 0.03 * np.sin(theta * rng.integers(2, 5) + k)
            out -= depth * (1.0 / (1.0 + np.exp(-(rho - local) / w))) * np.exp(-np.clip(rho - local, 0, None) / 0.035)
        return out

    def perimeter_theta(self, u):
        """Map uniform 0..2pi to angles round the umbo that are evenly spaced along the outline, so
        a mussel (umbo at its pointed end) doesn't spend half its columns on the hinge."""
        t = np.linspace(0.0, 2 * np.pi, 721)
        r = outline_radius(t, self.umbo, self.a, self.b, self.egg, None)
        x = self.umbo[0] + np.cos(t) * r
        y = self.umbo[1] + np.sin(t) * r
        acc = np.concatenate([[0.0], np.cumsum(np.hypot(np.diff(x), np.diff(y)))])
        even = 0.5 * acc / acc[-1] * 2 * np.pi + 0.5 * t  # half arc-length, half angle: keeps the umbo end sampled
        return np.interp(np.mod(u, 2 * np.pi), even, t) + (u - np.mod(u, 2 * np.pi))

    def inner_relief(self, theta, rho):
        return np.zeros(len(theta))

    def colour(self, theta, rho, inner):
        """Linear albedo and roughness per point."""
        rng = np.random.default_rng(self.seed + 1)
        n = len(theta)
        # Concentric growth colour: irregular bands of different widths, not a regular sine.
        bands = np.zeros(n)
        for _ in range(9):
            c0, w0 = rng.uniform(0.2, 1.0), rng.uniform(0.015, 0.07)
            bands += rng.uniform(0.3, 1.0) * np.exp(-((rho - c0 - 0.02 * np.sin(theta * 3 + c0 * 9)) / w0) ** 2)
        bands = np.clip(bands, 0, 1)
        tint = rng.uniform(0.85, 1.12)
        hue = np.array(rng.choice(((1.0, 1.0, 1.0), (1.0, 0.92, 0.80), (0.92, 0.80, 0.66), (0.90, 0.90, 0.92),
                                   (1.04, 0.95, 0.85))))
        if self.kind == "cockle":
            if inner:
                col = np.array([0.44, 0.42, 0.38]) * np.ones((n, 1))
                stain = np.clip(1.0 - rho / 0.45, 0, 1) * rng.uniform(0.2, 0.6)
                col = col * (1 - stain[:, None]) + np.array([0.24, 0.17, 0.15]) * stain[:, None]
                groove = (0.5 + 0.5 * np.cos(theta * self.ribs)) * np.clip((rho - 0.6) / 0.3, 0, 1)
                col = col * (1.0 - 0.18 * groove[:, None])
                rough = np.full(n, 0.42)
            else:
                base = np.array([0.36, 0.33, 0.26]) * tint * hue
                dark = np.array([0.19, 0.15, 0.10]) * tint * hue
                rib = (0.5 + 0.5 * np.cos(theta * self.ribs)) ** 0.6
                f = np.clip(0.55 * bands + 0.25 * (1 - rib) * rho, 0, 1) * np.clip(rho * 1.3, 0, 1)
                col = base * (1 - f[:, None]) + dark * f[:, None]
                worn = np.clip(1.0 - rho / 0.22, 0, 1)
                col = col * (1 - worn[:, None]) + np.array([0.44, 0.42, 0.38]) * worn[:, None]
                rough = 0.72 + 0.1 * bands
        elif self.kind == "mussel":
            if inner:
                nac = 0.5 + 0.5 * np.sin(theta * 3.0 + rho * 11.0)
                col = np.array([0.36, 0.38, 0.42]) * (0.85 + 0.15 * nac)[:, None]
                rim = np.clip((rho - 0.82) / 0.1, 0, 1)
                col = col * (1 - rim[:, None]) + np.array([0.05, 0.045, 0.08]) * rim[:, None]
                rough = 0.22 + 0.15 * rim
            else:
                col = np.array([0.020, 0.018, 0.024]) * (0.8 + 0.5 * bands)[:, None] * tint
                worn = np.clip(1.0 - rho / 0.4, 0, 1) ** 1.5
                patch = np.clip(np.sin(theta * 2.0 + self.seed % 5) * 1.5 - 0.6, 0, 1) * np.clip(rho * 2 - 0.4, 0, 1)
                col = col * (1 - patch[:, None] * 0.5) + np.array([0.07, 0.07, 0.09]) * patch[:, None] * 0.5
                col = col * (1 - worn[:, None]) + np.array([0.12, 0.10, 0.13]) * worn[:, None]
                rough = 0.55 + 0.12 * bands
        elif self.kind == "tellin":
            pink = rng.random() < 0.6
            c0 = np.array([0.52, 0.24, 0.22]) if pink else np.array([0.55, 0.40, 0.14])
            col = c0 * (0.85 + 0.2 * bands)[:, None] * tint
            if inner:
                col = col * 1.05
            rough = np.full(n, 0.35 if inner else 0.45)
        else:  # limpet
            if inner:
                col = np.array([0.40, 0.40, 0.38]) * np.ones((n, 1))
                scar = np.exp(-((rho - 0.55) / 0.08) ** 2) * (np.cos(theta) < 0.75)
                col = col * (1 - 0.6 * scar[:, None]) + np.array([0.20, 0.14, 0.10]) * 0.6 * scar[:, None]
                rim = np.clip((rho - 0.9) / 0.08, 0, 1)
                col = col * (1 - rim[:, None]) + np.array([0.10, 0.09, 0.08]) * rim[:, None]
                rough = 0.25 + 0.2 * rim
            else:
                rays = 0.5 + 0.5 * np.sin(theta * rng.integers(5, 9) + rng.uniform(0, 6))
                rib = (0.5 + 0.5 * np.cos(theta * self.ribs)) ** 1.5
                col = np.array([0.30, 0.28, 0.24]) * (0.75 + 0.35 * rib)[:, None] * tint
                col = col * (1 - 0.35 * rays[:, None] * rho[:, None])
                apex = np.clip(1.0 - rho / 0.25, 0, 1)
                col = col * (1 - apex[:, None]) + np.array([0.42, 0.33, 0.22]) * apex[:, None]
                rough = 0.78 + 0.08 * rib
        return np.clip(col, 0, 1), rough


def grid_faces(nt, nr, periodic, apex):
    """Quads/tris for a (nr rings x nt columns) grid; with ``apex`` ring 0 is one point."""
    faces = []
    cols = nt if periodic else nt + 1
    start = 1 if apex else 0

    def vid(i, j):
        if apex and i == 0:
            return 0
        return start + (i - start) * cols + (j % cols if periodic else j)
    for i in range(nr):
        for j in range(nt):
            if apex and i == 0:
                faces.append((0, vid(1, j), vid(1, j + 1)))
            else:
                faces.append((vid(i, j), vid(i + 1, j), vid(i + 1, j + 1), vid(i, j + 1)))
    return faces


def sample_grid(nt, nr, periodic, apex, t0=0.0, t1=2 * math.pi, r0=0.0, power=0.85):
    thetas, rhos = [], []
    cols = nt if periodic else nt + 1
    if apex:
        thetas.append(0.0)
        rhos.append(0.0)
    for i in range(1 if apex else 0, nr + 1):
        rho = r0 + (1.0 - r0) * (i / nr) ** power
        for j in range(cols):
            thetas.append(t0 + (t1 - t0) * j / nt)
            rhos.append(rho)
    return np.array(thetas), np.array(rhos)


class Part:
    """Accumulates high and low geometry; ``attr`` arrays only for the high mesh."""

    def __init__(self):
        self.verts, self.faces, self.col, self.rough = [], [], [], []
        self.n = 0

    def add(self, P, faces, col=None, rough=None, flip=False):
        base = self.n
        self.verts.append(P)
        self.faces.extend(tuple(base + i for i in (f[::-1] if flip else f)) for f in faces)
        if col is not None:
            self.col.append(col)
            self.rough.append(rough)
        self.n += len(P)

    def build(self, name, with_attr):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(np.concatenate(self.verts).tolist(), [], self.faces)
        mesh.validate()
        if with_attr:
            col = np.concatenate(self.col)
            attr = mesh.attributes.new("sh_col", "FLOAT_COLOR", "POINT")
            attr.data.foreach_set("color", np.concatenate([col, np.ones((len(col), 1))], 1).astype(np.float32).ravel())
            ra = mesh.attributes.new("sh_rough", "FLOAT", "POINT")
            ra.data.foreach_set("value", np.concatenate(self.rough).astype(np.float32))
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def vertex_normals(P, faces):
    N = np.zeros_like(P)
    for f in faces:
        idx = list(f)
        a, b, c = P[idx[0]], P[idx[1]], P[idx[2]]
        n = np.cross(b - a, c - a)
        for k in idx:
            N[k] += n
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


def emit_chip(high, low, shell, x, y, yaw, r1, rng, low_res=(9, 2)):
    """A worn fragment: the umbo end of a valve broken off along a jagged line at rho ~ ``r1``,
    lying convex side up and settled until its broken edge is just under the sand."""
    jag = [(rng.uniform(0.08, 0.2), rng.randint(2, 5), rng.uniform(0, 6.3)) for _ in range(2)]

    def rmax(th):
        out = np.full(len(th), r1)
        for amp, k, ph in jag:
            out = out * (1.0 + amp * np.sin(th * k + ph))
        return np.clip(out, 0.2, 0.95)
    edge_t = np.linspace(0, 2 * math.pi, 64, endpoint=False)
    z_edge = shell.surface(edge_t, rmax(edge_t))[:, 2]
    tilt_x, tilt_y = rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12)
    xf = np.array(xform(x, y, yaw, (tilt_x, tilt_y), False, 0.0))
    for target, (nt, nr), hi in ((high, (64, 18), True), (low, low_res, False)):
        th, rho = sample_grid(nt, nr, True, True, power=0.85 if hi else 0.68)
        th = shell.perimeter_theta(th)
        rho = rho * rmax(th)
        faces = grid_faces(nt, nr, True, True)
        P = shell.surface(th, rho)
        if hi:
            P = P + vertex_normals(P, faces) * shell.relief(th, rho)[:, None]
        Pw = (np.c_[P, np.ones(len(P))] @ xf.T)[:, :3]
        if target is high:
            Pe = shell.surface(edge_t, rmax(edge_t))
            Pe = (np.c_[Pe, np.ones(len(Pe))] @ xf.T)[:, :3]
            drop = Pe[:, 2].max() + rng.uniform(0.0005, 0.0015)
            chip_drop = drop
        Pw[:, 2] -= chip_drop if target is high else low_drop
        if target is high:
            low_drop = chip_drop
        col, rough = shell.colour(th, rho, False) if hi else (None, None)
        target.add(Pw, faces, col, rough)


def emit_valve(high, low, shell, xf, cupped, sector=None, low_res=(10, 3)):
    """A whole valve (or a ``sector`` fragment (t0, t1, r0)) placed by 4x4 ``xf``.
    Convex-up shows the outer sheet only; ``cupped`` adds the inner sheet and the rim band."""
    periodic = sector is None
    apex = sector is None or sector[2] <= 0.0
    t0, t1, r0 = sector if sector else (0.0, 2 * math.pi, 0.0)
    for target, (nt, nr), hi in ((high, (96 if periodic else 40, 26), True), (low, low_res, False)):
        th, rho = sample_grid(nt, nr, periodic, apex, t0, t1, r0, power=0.85 if hi else 0.68)
        if periodic:
            th = shell.perimeter_theta(th)
        faces = grid_faces(nt, nr, periodic, apex)
        outer = shell.surface(th, rho)
        if hi:
            N = vertex_normals(outer, faces)
            outer = outer + N * shell.relief(th, rho)[:, None]
        sheets = [(outer, False)]
        if cupped:
            inner_p = shell.surface(th, rho, inner=True)
            if hi:
                inner_p = inner_p + vertex_normals(inner_p, faces) * shell.inner_relief(th, rho)[:, None]
            sheets.append((inner_p, True))
        for P, inner in sheets:
            Pw = (np.c_[P, np.ones(len(P))] @ np.asarray(xf).T)[:, :3]
            col, rough = shell.colour(th, rho, inner) if hi else (None, None)
            target.add(Pw, faces, col, rough, flip=inner)
        if cupped:
            cols = nt if periodic else nt + 1
            ring = np.arange(cols) + (len(th) - cols)
            Po = outer[ring]
            Pi = shell.surface(th[ring], rho[ring], inner=True)
            P = np.concatenate([Po, Pi])
            Pw = (np.c_[P, np.ones(len(P))] @ np.asarray(xf).T)[:, :3]
            m = len(ring)
            rf = [(j, m + j, m + ((j + 1) % m if periodic else j + 1), (j + 1) % m if periodic else j + 1)
                  for j in range(m if periodic else m - 1)]
            if hi:
                c, rgh = shell.colour(th[ring], rho[ring], False)
                c = np.concatenate([c, c]) * 1.15
                rgh = np.concatenate([rgh, rgh])
            target.add(Pw, rf, c if hi else None, rgh if hi else None)


def winkle(high, low, xf, rng, size):
    """A periwinkle lying on its side: a surface of revolution (blunt cone of ~4 whorls) with
    sunken spiral sutures and fine spiral lines on the high mesh, a dark oval aperture near the base."""
    whorls = 4.0
    turn = rng.uniform(0, 6.3)
    for target, (nz, nt), hi in ((high, (60, 48), True), (low, (4, 8), False)):
        v = np.linspace(0.0, 1.0, nz + 1)            # 0 at the base (aperture end), 1 at the apex
        prof = size * 0.5 * np.clip(1.0 - v, 0, 1) ** 0.85 * (1.0 + 0.18 * np.sin(np.pi * np.clip(v * 1.2, 0, 1)))
        prof[0] = size * 0.30
        zz = size * (0.92 * v - 0.08)
        verts, cols, rough = [], [], []
        for i in range(nz + 1):
            for j in range(nt):
                a = 2 * math.pi * j / nt
                r = prof[i]
                if hi:
                    ph = (v[i] * whorls + a / (2 * math.pi) + turn) % 1.0
                    suture = math.exp(-((ph - 0.5) / 0.05) ** 2)
                    r = r * (1.0 - 0.07 * suture * (v[i] > 0.12)) + size * 0.004 * math.sin(v[i] * 140 + a * 2)
                verts.append((r * math.cos(a), r * math.sin(a), zz[i]))
                if hi:
                    lines = 0.5 + 0.5 * math.sin(v[i] * 160.0 + a * 0.3)
                    c = np.array([0.050, 0.044, 0.038]) * (0.7 + 0.6 * lines)
                    c = c * (1 - 0.4 * suture)
                    apex = max(0.0, (v[i] - 0.72) / 0.28)
                    c = c * (1 - apex) + np.array([0.24, 0.20, 0.16]) * apex
                    aperture = math.exp(-(((a - math.pi) / 0.9) ** 2)) * max(0.0, 1.0 - v[i] / 0.32)
                    c = c * (1 - aperture) + np.array([0.012, 0.01, 0.01]) * aperture
                    cols.append(c)
                    rough.append(0.5 + 0.1 * lines)
        faces = []
        for i in range(nz):
            for j in range(nt):
                k = (j + 1) % nt
                faces.append((i * nt + j, i * nt + k, (i + 1) * nt + k, (i + 1) * nt + j))
        verts.append((0.0, 0.0, zz[0] - size * 0.02))
        if hi:
            cols.append(np.array([0.012, 0.01, 0.01]))
            rough.append(0.6)
        cap = len(verts) - 1
        for j in range(nt):
            faces.append((cap, (j + 1) % nt, j))
        P = np.array(verts)
        Pw = (np.c_[P, np.ones(len(P))] @ np.asarray(xf).T)[:, :3]
        target.add(Pw, faces, np.array(cols) if hi else None, np.array(rough) if hi else None)

def grit(high, low, rng, xy, count):
    """Crushed shell litter: small angular flakes lying on the sand."""
    palette = [((0.40, 0.37, 0.32), 0.3), ((0.31, 0.27, 0.21), 0.3), ((0.04, 0.04, 0.055), 0.22),
               ((0.36, 0.22, 0.21), 0.06), ((0.18, 0.14, 0.10), 0.12)]
    cols = [c for c, _ in palette]
    weights = [w for _, w in palette]
    for i in range(count):
        x, y = xy()
        s = rng.uniform(0.0012, 0.0035)
        k = rng.randint(3, 4)
        ang0 = rng.uniform(0, 2 * math.pi)
        pts = [(x + math.cos(ang0 + 2 * math.pi * m / k + rng.uniform(-0.3, 0.3)) * s * rng.uniform(0.6, 1.1),
                y + math.sin(ang0 + 2 * math.pi * m / k + rng.uniform(-0.3, 0.3)) * s * rng.uniform(0.6, 1.1),
                rng.uniform(-0.0006, 0.0003)) for m in range(k)]
        P = np.array(pts)
        faces = [(0, m, m + 1) for m in range(1, k - 1)]
        c = np.array(rng.choices(cols, weights)[0]) * rng.uniform(0.8, 1.1)
        high.add(P, faces, np.tile(c, (k, 1)), np.full(k, 0.6))
        low.add(P.copy(), faces)


# ------------------------------------------------------------------ layout

def centre_y(x):
    return 0.07 * math.sin(2.6 * x + 0.4) + 0.025 * math.sin(7.1 * x)


def place(rng, placed, radius, spread=1.0, tries=60):
    for _ in range(tries):
        x = rng.uniform(-0.58, 0.58)
        dens = math.exp(-(x / 0.75) ** 4)
        if rng.random() > dens:
            continue
        y = centre_y(x) + rng.gauss(0, 0.06 * spread)
        if all(math.hypot(x - px, y - py) > (radius + pr) * 0.8 for px, py, pr in placed):
            placed.append((x, y, radius))
            return x, y
    return None


def xform(x, y, yaw, tilt, flip, z):
    m = Matrix.Translation((x, y, z)) @ Matrix.Rotation(yaw, 4, "Z") @ Matrix.Rotation(tilt[0], 4, "X") \
        @ Matrix.Rotation(tilt[1], 4, "Y")
    if flip:
        m = m @ Matrix.Rotation(math.pi, 4, "X")
    return m


def shell_material(kit, name):
    g = kit.mats.Graph(name)
    col = g.node("ShaderNodeAttribute", attribute_name="sh_col", attribute_type="GEOMETRY").outputs["Color"]
    rough = g.node("ShaderNodeAttribute", attribute_name="sh_rough", attribute_type="GEOMETRY").outputs["Fac"]
    pos = g.node("ShaderNodeTexCoord").outputs["Object"]
    pz = g.separate(pos)[2]
    fine = g.noise(pos, scale=900.0, detail=4.0, roughness=0.6).outputs["Fac"]
    mott = g.noise(pos, scale=60.0, detail=3.0).outputs["Fac"]
    color = g.mix(col, (0.80, 0.78, 0.74), g.remap(mott, 0.35, 0.7, 0.0, 0.25), blend="MULTIPLY")
    # Sand clinging round the base of each shell.
    sand = g.math("MULTIPLY", g.remap(pz, 0.0008, -0.0015), g.math("MULTIPLY", g.remap(fine, 0.42, 0.62), 0.7))
    color = g.mix(color, (0.36, 0.30, 0.215), sand)
    g.set("Base Color", color)
    g.set("Roughness", g.node("ShaderNodeMix", data_type="FLOAT", Factor=sand, A=rough, B=0.92).outputs[0])
    g.set("Normal", g.bump(g.math("ADD", fine, g.math("MULTIPLY", mott, 0.3)), strength=0.25, distance=0.0002))
    g.bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return g.mat


def build(kit):
    rng = random.Random(SEED)
    high, low = Part(), Part()
    placed = []
    kinds = (["cockle"] * 24 + ["mussel"] * 15 + ["limpet"] * 5 + ["tellin"] * 6)
    rng.shuffle(kinds)
    sizes = {"cockle": (0.024, 0.042), "mussel": (0.032, 0.062), "limpet": (0.028, 0.046), "tellin": (0.016, 0.022)}
    stats = {"valves": 0, "cupped": 0, "fragments": 0, "winkles": 0, "grit": 0}
    for kind in kinds:
        size = rng.uniform(*sizes[kind])
        pos = place(rng, placed, size * 0.5)
        if pos is None:
            continue
        sh = Shell(kind, rng, size)
        cupped = kind != "limpet" and rng.random() < 0.26 or kind == "limpet" and rng.random() < 0.25
        yaw = rng.uniform(0, 2 * math.pi)
        if cupped:
            tilt = (rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12))
            xf = xform(pos[0], pos[1], yaw, tilt, True, sh.H * rng.uniform(0.72, 0.88))
            stats["cupped"] += 1
        else:
            tilt = (rng.uniform(-0.05, 0.05), rng.uniform(-0.05, 0.05))
            xf = xform(pos[0], pos[1], yaw, tilt, False, -rng.uniform(0.0015, 0.003))
        big = size > 0.034
        res = (14, 2) if cupped else (20, 2) if big else (14, 2)
        emit_valve(high, low, sh, xf, cupped, low_res=res)
        stats["valves"] += 1
    for i in range(26):
        kind = rng.choice(("cockle", "cockle", "mussel", "mussel", "mussel", "limpet"))
        size = rng.uniform(*sizes[kind])
        pos = place(rng, placed, size * 0.35, spread=1.25)
        if pos is None:
            continue
        sh = Shell(kind, rng, size)
        emit_chip(high, low, sh, pos[0], pos[1], rng.uniform(0, 2 * math.pi), rng.uniform(0.45, 0.8), rng,
                  low_res=(10, 1) if size > 0.03 else (7, 1))
        stats["fragments"] += 1
    for i in range(5):
        size = rng.uniform(0.016, 0.024)
        pos = place(rng, placed, size * 0.5)
        if pos is None:
            continue
        lean = Matrix.Rotation(math.pi / 2 - 0.22, 4, "Y")
        xf = (Matrix.Translation((pos[0], pos[1], size * 0.30)) @ Matrix.Rotation(rng.uniform(0, 6.3), 4, "Z") @ lean
              @ Matrix.Translation((0, 0, -size * 0.35)))
        winkle(high, low, xf, rng, size)
        stats["winkles"] += 1

    def grit_xy():
        while True:
            x = rng.uniform(-0.6, 0.6)
            if rng.random() < math.exp(-(x / 0.7) ** 4):
                return x, centre_y(x) + rng.gauss(0, 0.075)
    grit(high, low, rng, grit_xy, 44)
    stats["grit"] = 44

    hi_obj = high.build("ShellDriftHigh", True)
    lo_obj = low.build("SM_" + NAME, False)
    hi_obj.data.materials.append(shell_material(kit, "M_BeachShellDriftProcedural"))
    hi_obj.data.shade_smooth()
    lo, hi = kit.bounds(lo_obj)
    shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 0.0))
    for obj in (lo_obj, hi_obj):
        obj.data.transform(Matrix.Translation(-shift))
        obj.data.update()
    kit.pack_uvs(lo_obj, margin=0.003)
    kit.finalize(lo_obj, pivot=None, unwrap=False, reshade=True, smooth_angle=60.0)
    lo_obj["homestead_shift"] = list(shift)
    folder = Path(__file__).resolve().parents[3] / "Assets" / "Props" / NAME / "Textures"
    material, info = hb.bake(kit, hi_obj, lo_obj, folder, NAME, size=BAKE_SIZE, samples=16 if DRAFT else 64,
                             extrusion=0.0025, distance=0.006)
    bpy.data.objects.remove(hi_obj)
    meshes = [lo_obj]
    for index, ratio in enumerate(LOD_RATIOS, start=1):
        lod = rocks.lod(kit, lo_obj, f"SM_{NAME}_LOD{index}", ratio)
        kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=60.0)
        lod.data.materials.clear()
        lod.data.materials.append(material)
        lod["homestead_shift"] = list(shift)
        meshes.append(lod)
    NOTES.update({"bake": info, "counts": stats, "lods": {m.name: rocks.triangles(m) for m in meshes},
                  "material": "Baked from a dense displaced, vertex-coloured high mesh (homestead_highbake); "
                              "M_PropTextured."})
    print("HOMESTEAD_SHELLS", stats, NOTES["lods"], flush=True)
    return meshes
