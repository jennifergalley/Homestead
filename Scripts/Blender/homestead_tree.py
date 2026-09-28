"""Broadleaf tree grower for Homestead (mature woodland trees, windswept thorns, big shrubs).

Builds on ``homestead_foliage`` (F: numpy atlas painting, ``Batch`` geometry, Wind colours) and
``homestead_shrub`` (S: crown envelope and branch growth). A recipe supplies a ``SPEC`` dict plus
painters for its bark and leaf clusters; ``build`` returns ``SM_<Name>`` and ``_LOD1``..``_LOD3``.

Geometry
- The skeleton is ``S.grow`` with a single level-0 stem (the trunk) unless the spec lists several
  ``crowns`` (multi-stemmed thorns and coppice). The trunk gets its own sweep with a root flare,
  buttress lobes and a short skirt below the ground line so it sits into slopes.
- Foliage is *cluster cards*: painted twigs carrying 10-40 leaves (``paint_cluster``), anchored on
  the outer part of the finest branches and facing out of the crown. Far LODs merge the anchors
  on a coarse grid into big *mass* cards (``paint_mass``: a dense leaf clump with a dark core).
- Leaf-card normals are bent toward the crown ellipsoid (``normal_blend``), so the canopy shades
  as one soft volume at distance instead of a pile of flat cards.

LOD plan (per spec ``lods``): LOD0 all branches and every cluster card (hero, near the player);
LOD1 drops the finest branch level and merges clusters on a ~1.2 m grid; LOD2 keeps limbs and
~2.5 m mass cards; LOD3 is trunk + main limbs + a few dozen 5-6 m mass cards (a few hundred
triangles, the "impostor" that tens of thousands of instances draw at range).

Materials (two slots, in this order, so slot indices are stable for Unreal):
  0 ``M_<Name>Bark``   tiling bark, its own 2K set ``T_<Name>Bark_*`` (u around, v along).
  1 ``M_<Name>Leaves`` leaf-cluster atlas ``T_<Name>Leaves_*`` (alpha-masked, two-sided).
Both carry the packed maps of ``homestead_foliage`` (R roughness, G translucency, B AO) and both use
``M_PropFoliage`` in Unreal, so wood and leaves sway together from the ``Wind`` vertex colour.

Collision: one capsule round the lower trunk, written to the report as ``capsule`` (cm) and built
by ``import_props.py`` as the mesh's only simple shape (``SphylElems[0]``).
"""
import math
import random

import bpy
import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

UP = Vector((0, 0, 1))


# ------------------------------------------------------------------ leaf shapes

def placed(shape, origin=(0.0, 0.0), theta=0.0, scale=1.0, squash=1.0):
    """Evaluate ``shape`` (blade units: base at y=0, tip at y=1) placed at ``origin`` with its
    axis leaning ``theta`` from +Y toward +X and ``scale`` long; ``inside`` stays in tile units."""
    def fn(X, Y):
        Xl, Yl = S.rotated(X, Y, origin, theta, scale, squash)
        inside, s, hw = shape(Xl, Yl)
        return inside * scale, s, hw
    return fn


def union(*shapes):
    """Silhouette union (lobed and palmate leaves from simple ovate parts)."""
    def fn(X, Y):
        inside = None
        for shape in shapes:
            value, _, hw = shape(X, Y)
            inside = value if inside is None else np.maximum(inside, value)
        return inside, np.clip(Y, 0.0, 1.0), hw
    return fn


def ellipse(rx, ry, center=(0.0, 0.5)):
    def fn(X, Y):
        d = np.sqrt(((X - center[0]) / rx) ** 2 + ((Y - center[1]) / ry) ** 2)
        return (1.0 - d) * min(rx, ry), np.clip(Y, 0, 1), np.full(X.shape, rx)
    return fn


# ------------------------------------------------------------------ bark painting

def _voronoi_edges(shape, nrng, count, aniso=(1.0, 1.0)):
    """Periodic Voronoi: (edge distance F2-F1, cell id, F1) on a (h, w) grid; cells can be
    stretched with ``aniso`` (x, y) to make tall or wide plates."""
    h, w = shape
    pts = nrng.random((count, 2))
    ys, xs = np.meshgrid((np.arange(h) + 0.5) / h, (np.arange(w) + 0.5) / w, indexing="ij")
    f1 = np.full(shape, 9.0, np.float32)
    f2 = np.full(shape, 9.0, np.float32)
    cell = np.zeros(shape, np.int32)
    for i, (px, py) in enumerate(pts):
        dx = np.abs(xs - px)
        dx = np.minimum(dx, 1 - dx) / aniso[0]
        dy = np.abs(ys - py)
        dy = np.minimum(dy, 1 - dy) / aniso[1]
        d = np.sqrt(dx * dx + dy * dy).astype(np.float32)
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        cell = np.where(closer, i, cell)
        f1 = np.where(closer, d, f1)
    return f2 - f1, cell, f1


def paint_bark(atlas, key, nrng, *, size_m=1.0, style="fissured", base=(0.10, 0.09, 0.08),
               ridge=(0.16, 0.15, 0.13), fissure=(0.03, 0.025, 0.02), depth=0.02, ridges=9.0,
               breaks=0.5, plates=0, plate_color=(0.20, 0.13, 0.08), lichen=0.3,
               lichen_color=(0.24, 0.26, 0.19), moss=0.2, moss_color=(0.05, 0.08, 0.02),
               algae=0.0, lenticels=0.0, rough=0.82, stripes=0.0, crack=(0.004, 0.018), fresh=0.28,
               broken=0.0):
    """Tileable bark column covering ``size_m`` x ``size_m``.

    ``style``: ``fissured`` (oak, hawthorn: anastomosing vertical ridges broken into blocks),
    ``smooth`` (beech, holly: thin grey skin, faint horizontal lenticel dashes, algae),
    ``plated`` (sycamore: smooth grey flaking in ``plates`` irregular scales; ``crack`` is the
    (sharp, soft) crack half-width in tile units, ``fresh`` the fraction of freshly bared plates,
    ``broken`` 0-1 how much of the plate net fades to faint seams so it doesn't read as cobbles)."""
    U, V = atlas.column_grid(key)
    shp = U.shape
    fine = F.noise(shp, nrng, freq=160.0, beta=1.3)
    mid = F.noise(shp, nrng, freq=40.0, beta=1.8)
    blot = F.noise(shp, nrng, freq=5.0, beta=2.6)
    height = np.zeros(shp)
    color = F.lerp(base, np.asarray(base) * 1.25, blot)
    r = np.full(shp, rough)
    ao = np.ones(shp)
    if style == "fissured":
        # Meandering vertical cracks: zero crossings of noise stretched along v.
        n1 = F.noise(shp, nrng, freq=ridges, beta=2.2, aniso=(1.0, 5.0))
        n2 = F.noise(shp, nrng, freq=ridges * 2.1, beta=2.0, aniso=(1.0, 4.0))
        crack = np.maximum(F.ridged(n1), 0.7 * F.ridged(n2))
        crack = F.smoothstep(0.62, 0.97, crack)
        # Cross breaks cut the ridges into blocks.
        cross = F.smoothstep(0.80, 0.97, F.ridged(F.noise(shp, nrng, freq=ridges * 0.9, beta=2.2, aniso=(3.0, 1.0))))
        cross = cross * F.smoothstep(0.35, 0.7, F.noise(shp, nrng, freq=ridges, beta=2.0)) * breaks
        fis = np.clip(crack + cross * 0.8, 0, 1)
        plate = 1.0 - fis
        # Rounded ridge tops with a scaly surface.
        top = F.smoothstep(0.0, 0.55, plate)
        height = depth * (top - 1.0) + depth * 0.12 * (mid - 0.5) + depth * 0.05 * (fine - 0.5)
        color = F.lerp(fissure, F.lerp(base, ridge, F.smoothstep(0.55, 1.0, plate) * (0.6 + 0.4 * mid)), top)
        ao = 0.45 + 0.55 * top
        r = rough + 0.08 * (1 - top)
        # Mosses and algae live in the damp fissures, lichen on the dry ridge tops.
        if moss:
            m = F.smoothstep(0.55, 0.85, F.noise(shp, nrng, freq=8.0, beta=2.4)) * moss
            color = F.lerp(color, moss_color, np.clip(m * (1.2 - top * 0.7), 0, 1))
            height = height + depth * 0.25 * m * (1 - top)
    elif style == "smooth":
        wrinkle = F.noise(shp, nrng, freq=30.0, beta=1.8, aniso=(4.0, 1.0))
        height = depth * (0.4 * (mid - 0.5) + 0.3 * (wrinkle - 0.5) + 0.1 * (fine - 0.5))
        color = color * (0.92 + 0.12 * wrinkle)[..., None]
        if lenticels:
            dash = F.smoothstep(0.86, 0.95, F.noise(shp, nrng, freq=70.0, beta=1.3, aniso=(5.0, 1.0)))
            color = F.lerp(color, np.asarray(base) * 0.6, dash * lenticels)
            height = height - depth * 0.3 * dash
    elif style == "plated":
        edge, cell, f1 = _voronoi_edges(shp, nrng, plates, aniso=(1.0, 1.6))
        crack = 1.0 - F.smoothstep(crack[0], crack[1], edge)
        if broken:
            # Only some plate borders have actually split; the rest stay as faint seams.
            open_ = F.smoothstep(0.35, 0.65, F.noise(shp, nrng, freq=max(plates, 1) ** 0.5 * 1.5, beta=2.0))
            crack = crack * (1.0 - broken + broken * open_)
        cell_rand = (np.sin(cell * 12.9898 + 4.1) * 43758.5453) % 1.0
        fresh = (cell_rand > 1.0 - fresh) * F.smoothstep(0.02, 0.05, edge)
        lift = F.smoothstep(0.0, 0.08, edge) * (0.5 + 0.5 * cell_rand)
        height = depth * (lift - 1.0) * (1 - crack) - depth * crack + depth * 0.1 * (mid - 0.5)
        # Each plate weathers a little differently.
        color = color * (0.88 + 0.24 * cell_rand)[..., None]
        color = F.lerp(color, plate_color, fresh * 0.85)
        color = F.lerp(color, fissure, crack)
        ao = 0.7 + 0.3 * (1 - crack)
    if algae:
        a = F.smoothstep(0.5, 0.85, F.noise(shp, nrng, freq=4.0, beta=2.8, aniso=(1.0, 2.0))) * algae
        color = F.lerp(color, (0.07, 0.10, 0.04), a)
    if lichen:
        li = F.smoothstep(0.80, 0.90, F.noise(shp, nrng, freq=14.0, beta=2.3)) * lichen
        crust = F.smoothstep(0.4, 0.9, fine) * li
        color = F.lerp(color, lichen_color, np.clip(li * 0.8 + crust * 0.2, 0, 1))
        height = height + depth * 0.06 * li
    if stripes:
        s = F.smoothstep(0.7, 0.95, F.noise(shp, nrng, freq=20.0, beta=1.6, aniso=(1.0, 10.0)))
        color = F.lerp(color, np.asarray(color) * 0.75, s * stripes)
    color = color * (0.94 + 0.12 * fine)[..., None]
    layer = F.Layer(shp)
    layer.color, layer.height = np.clip(color, 0, 1), height
    layer.rough = np.clip(r + 0.06 * (fine - 0.5), 0.35, 0.97)
    layer.trans[...] = 0.0
    layer.ao = np.clip(ao, 0, 1)
    atlas.put(key, layer, meters_per_px=size_m / shp[0], wrap=True, opaque=True)
    return layer


# ------------------------------------------------------------------ leaf clusters

def _fit(twigs, leaves, a, margin=0.03, base_y=0.015):
    """Scale and shift a cluster layout so it fits the tile (x in [-a, a], y in [0, 1]) with
    the main twig's base at the bottom centre."""
    xs, ys = [], []
    for pts, w in twigs:
        for x, y in pts:
            xs.append(x)
            ys.append(y)
    for (ox, oy), th, ln, sq, *_ in leaves:
        for t in (0.0, 1.0):
            xs.append(ox + math.sin(th) * ln * t)
            ys.append(oy + math.cos(th) * ln * t)
        # The blade's width, roughly.
        xs.extend([ox - ln * 0.35, ox + ln * 0.35])
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    k = min((2 * a - 2 * margin) / max(x1 - x0, 1e-6), (1 - base_y - margin) / max(y1 - y0, 1e-6))
    base = twigs[0][0][0]
    cx = (x0 + x1) * 0.5
    # Keep the attachment point near the bottom centre (the card's pivot on the branch).
    shift_x = -cx
    ft = [([((x + shift_x) * k, (y - y0) * k + base_y) for x, y in pts], w * k) for pts, w in twigs]
    fl = [(((ox + shift_x) * k, (oy - y0) * k + base_y), th, ln * k, sq, *rest)
          for (ox, oy), th, ln, sq, *rest in leaves]
    return ft, fl, ((base[0] + shift_x) * k, (base[1] - y0) * k + base_y)


def cluster_layout(rng, *, pattern="alternate", side_twigs=5, main_len=1.0, side_len=(0.35, 0.55),
                   side_angle=(35, 60), leaf_len=0.16, per_twig=(3, 6), leaf_angle=(35, 75),
                   squash=(0.55, 1.0), tip_fan=(4, 6), twig_width=0.010, curve=0.12, main_leaves=True,
                   keys=None):
    """Twig skeleton and leaf placements for one painted cluster (unfitted units).

    ``pattern``: ``tips`` (oak: rosettes crowded at the shoot ends), ``alternate`` (beech,
    hawthorn, hazel), ``opposite`` (sycamore, ash), ``spiral`` (holly)."""
    twigs, leaves = [], []
    bend = rng.uniform(-curve, curve)
    main = [(bend * math.sin(math.pi * t * 0.8), main_len * t) for t in np.linspace(0, 1, 12)]
    twigs.append((main, twig_width))
    shoots = [(main, 1.0)]
    side = rng.choice((-1, 1))
    for i in range(side_twigs):
        t = 0.18 + 0.7 * (i + rng.uniform(0.2, 0.8)) / side_twigs
        px, py = bend * math.sin(math.pi * t * 0.8), main_len * t
        ang = math.radians(rng.uniform(*side_angle)) * side
        ln = main_len * rng.uniform(*side_len) * (1.15 - 0.5 * t)
        c = rng.uniform(-0.25, 0.25)
        pts = [(px + math.sin(ang + c * u) * ln * u, py + math.cos(ang + c * u) * ln * u)
               for u in np.linspace(0, 1, 8)]
        twigs.append((pts, twig_width * rng.uniform(0.5, 0.7)))
        shoots.append((pts, 0.8))
        side = -side
    for pts, scale in shoots:
        if pts is main and not main_leaves:
            continue
        acc = [0.0]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            acc.append(acc[-1] + math.hypot(x1 - x0, y1 - y0))
        L = acc[-1]

        def at(s):
            for i in range(1, len(pts)):
                if acc[i] >= s:
                    f = (s - acc[i - 1]) / max(acc[i] - acc[i - 1], 1e-9)
                    x = pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * f
                    y = pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * f
                    return (x, y), math.atan2(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1])
            return pts[-1], math.atan2(pts[-1][0] - pts[-2][0], pts[-1][1] - pts[-2][1])

        def leaf(origin, th, size):
            k = rng.choice(keys) if keys else None
            leaves.append((origin, th, size, rng.uniform(*squash), k, rng.uniform(0.72, 1.12)))

        if pattern == "tips":
            tip, heading = at(L)
            n = rng.randint(*tip_fan)
            for j in range(n):
                th = heading + math.radians(-75 + 150 * (j + rng.uniform(0.2, 0.8)) / n)
                leaf(tip, th, leaf_len * scale * rng.uniform(0.8, 1.15))
            for _ in range(rng.randint(0, 2)):
                o, h = at(L * rng.uniform(0.55, 0.85))
                leaf(o, h + rng.choice((-1, 1)) * math.radians(rng.uniform(*leaf_angle)),
                     leaf_len * scale * rng.uniform(0.6, 0.9))
            continue
        n = rng.randint(*per_twig)
        sd = rng.choice((-1, 1))
        for j in range(n):
            s = L * (0.25 + 0.72 * (j + rng.uniform(0.3, 0.7)) / n)
            o, h = at(s)
            size = leaf_len * scale * rng.uniform(0.85, 1.1) * (1.0 - 0.25 * s / L)
            if pattern == "opposite":
                for side_ in (-1, 1):
                    leaf(o, h + side_ * math.radians(rng.uniform(*leaf_angle)), size)
            elif pattern == "spiral":
                a = math.radians(rng.uniform(*leaf_angle)) * sd
                leaf(o, h + a, size * rng.uniform(0.85, 1.05))
                sd = -sd if rng.random() < 0.7 else sd
            else:
                leaf(o, h + sd * math.radians(rng.uniform(*leaf_angle)), size)
                sd = -sd
        tip, heading = at(L)
        leaf(tip, heading + rng.uniform(-0.2, 0.2), leaf_len * scale * rng.uniform(0.75, 0.95))
    return twigs, leaves


def _paint_twigs(layer, X, Y, px, nrng, twigs, color):
    for pts, width in twigs:
        d, along = F.polyline_distance(X, Y, pts, max_dist=width * 3 + px * 3)
        w = width * (1 - 0.55 * along)
        a = np.clip(-(d - w) / px + 0.5, 0, 1)
        if not a.any():
            continue
        t = F.Layer(X.shape)
        t.color = F.lerp(np.asarray(color) * 0.75, np.asarray(color) * 1.25,
                         np.clip(1 - d / np.maximum(w, px), 0, 1))
        t.alpha = a
        t.height = 0.0006 * np.sqrt(np.clip(1 - (d / np.maximum(w, px)) ** 2, 0, 1))
        t.rough[...] = 0.7
        t.trans[...] = 0.02
        layer.over(t)


def paint_cluster(atlas, key, nrng, rng, blade, layout, twig_color=(0.07, 0.05, 0.03), meters=0.9):
    """Paint one leaf cluster card into tile ``key``. ``blade(Xl, Yl, pxl, key) -> Layer``
    paints a single leaf in blade units; ``layout`` is ``cluster_layout(...)`` output. Leaves are
    composited in random order, earlier (lower) leaves darker, as a real spray self-shadows."""
    X, Y, px = atlas.grid(key)
    a = X.max()
    twigs, leaves, _ = _fit(*layout, a)
    layer = F.Layer(X.shape)
    layer.color[...] = twig_color
    _paint_twigs(layer, X, Y, px, nrng, twigs, twig_color)
    order = list(range(len(leaves)))
    rng.shuffle(order)
    for rank, i in enumerate(order):
        (ox, oy), th, ln, sq, k, bright = leaves[i]
        depth = rank / max(len(order) - 1, 1)
        shade = bright * (0.72 + 0.34 * depth)
        ctr = (ox + 0.5 * ln * math.sin(th), oy + 0.5 * ln * math.cos(th))

        def paint(Xs, Ys, o=(ox, oy), th=th, ln=ln, sq=sq, k=k, shade=shade, depth=depth):
            Xl, Yl = S.rotated(Xs, Ys, o, th, ln, sq)
            lf = blade(Xl, Yl, px / ln, k)
            lf.color = np.clip(lf.color * shade, 0, 1)
            lf.ao = lf.ao * (0.7 + 0.3 * depth)
            return lf
        S.over_window(layer, X, Y, ctr, ln * 0.75, paint)
    atlas.put(key, layer, meters_per_px=meters / X.shape[0])
    return layer


def paint_mass(atlas, key, nrng, rng, blade, *, count=160, leaf_len=0.075, keys=None,
               core=(0.018, 0.028, 0.010), holes=0.5, meters=4.0, squash=(0.5, 1.0), lobes=6):
    """A dense clump of foliage for far-LOD cards: a dark leafy core with lobed, gappy edges and
    many small leaves over it, lit from above (upper leaves brighter)."""
    X, Y, px = atlas.grid(key)
    a = X.max()
    cx, cy = 0.0, 0.5
    th = np.arctan2(Y - cy, X - cx)
    rad = np.sqrt(((X - cx) / (a * 0.84)) ** 2 + ((Y - cy) / 0.44) ** 2)
    edge = 1.0
    for k in range(2, 2 + lobes):
        edge = edge + 0.09 / math.sqrt(k) * np.sin(k * th + rng.uniform(0, math.tau))
    wob = F.noise(X.shape, nrng, freq=10.0, beta=2.0)
    inside = edge * (0.86 + 0.14 * wob) - rad
    layer = F.Layer(X.shape)
    speck = F.noise(X.shape, nrng, freq=90.0, beta=1.2)
    layer.color = F.lerp(np.asarray(core) * 0.7, np.asarray(core) * 1.5,
                         0.6 * F.noise(X.shape, nrng, freq=14.0, beta=2.0) + 0.4 * speck)
    layer.color = layer.color * (0.75 + 0.5 * F.smoothstep(0.1, 0.95, Y))[..., None]
    layer.alpha = F.smoothstep(0.10, 0.20, inside) * F.smoothstep(0.25, 0.45, speck + 0.3 * F.smoothstep(0.1, 0.4, inside))
    if holes:
        gap = F.smoothstep(0.80, 0.86, F.noise(X.shape, nrng, freq=16.0, beta=2.2)) * holes
        layer.alpha = layer.alpha * (1 - gap * F.smoothstep(0.05, 0.25, inside))
    layer.ao = 0.55 + 0.25 * F.smoothstep(0.0, 0.4, Y)
    layer.rough[...] = 0.6
    layer.trans[...] = 0.2
    placed_leaves = []
    tries = 0
    while len(placed_leaves) < count and tries < count * 40:
        tries += 1
        x, y = rng.uniform(-a, a), rng.uniform(0.02, 0.98)
        rr = math.hypot((x - cx) / (a * 0.84), (y - cy) / 0.44)
        if rr > 1.05 * rng.uniform(0.85, 1.1):
            continue
        placed_leaves.append((x, y))
    placed_leaves.sort(key=lambda p: p[1])
    for i, (x, y) in enumerate(placed_leaves):
        thl = rng.uniform(-math.pi, math.pi)
        ln = leaf_len * rng.uniform(0.75, 1.2)
        sq = rng.uniform(*squash)
        k = rng.choice(keys) if keys else None
        light = 0.52 + 0.62 * y + rng.uniform(-0.12, 0.12)
        o = (x - 0.5 * ln * math.sin(thl), y - 0.5 * ln * math.cos(thl))

        def paint(Xs, Ys, o=o, thl=thl, ln=ln, sq=sq, k=k, light=light, y=y):
            Xl, Yl = S.rotated(Xs, Ys, o, thl, ln, sq)
            lf = blade(Xl, Yl, px / ln, k)
            lf.color = np.clip(lf.color * light, 0, 1)
            lf.ao = lf.ao * (0.7 + 0.3 * y)
            return lf
        S.over_window(layer, X, Y, (x, y), ln * 0.75, paint)
    atlas.put(key, layer, meters_per_px=meters / X.shape[0])
    return layer


# ------------------------------------------------------------------ growth

def grow(spec, rng):
    """Skeleton plus foliage anchors. Returns dict(branches, env, height, anchors, trunk)."""
    sspec = dict(spec)
    sspec.setdefault("crowns", [(0.0, 0.0)])
    sspec["leaf"] = dict(level=99, zone=0.1, spacing=(1, 1), angle=(0, 0), size=0.1)
    desc = S.grow(sspec, rng)
    env = desc["env"]
    fol = spec["foliage"]
    leaf_level = fol.get("level", len(spec["levels"]) - 1)
    has_child = {id(b["parent"]) for b in desc["branches"] if b.get("parent") is not None}
    center = Vector((env.c.x, env.c.y, spec.get("crown_center", 0.6) * spec["height"]))
    anchors = []
    for br in desc["branches"]:
        lvl = br["level"]
        if lvl < leaf_level - 1:
            continue
        terminal = id(br) not in has_child
        if lvl == leaf_level - 1 and not fol.get("on_parents", 0.0):
            continue
        L = br["length"]
        zone = min(L, fol["zone"] * (1.0 if lvl >= leaf_level else fol.get("on_parents", 0.0)))
        if zone <= 0:
            continue
        s = L - zone + rng.uniform(0, fol["spacing"][0])
        while s <= L + 1e-6:
            node, tan, i = S.at(br["pts"], br["acc"], min(s, L))
            ratio = env.ratio(node)
            keep = 1.0 if ratio >= fol.get("inner", 0.6) else fol.get("inner_density", 0.35)
            if node.z < spec["height"] * fol.get("min_z", 0.2):
                keep = 0.0
            if rng.random() < keep:
                surf = env.normal(node)
                jitter = S._rand_unit(rng)
                d = (tan * fol.get("along", 0.6) + surf * fol.get("face_out", 0.6) + jitter * 0.35 -
                     UP * fol.get("droop", 0.15)).normalized()
                up = (surf * 0.8 + UP * fol.get("face_up", 0.5) + jitter * 0.45).normalized()
                if abs(up.dot(d)) > 0.95:
                    up = (up + d.orthogonal()).normalized()
                size = fol["size"] * rng.uniform(0.8, 1.2) * (0.85 if ratio < fol.get("inner", 0.6) else 1.0)
                anchors.append(dict(pos=node.copy(), dir=d, up=up, size=size, phase=br["phase"],
                                    outer=ratio, key=rng.random(), spin=rng.uniform(-0.5, 0.5)))
            s += rng.uniform(*fol["spacing"])
        if terminal and fol.get("tip", True):
            tip = br["pts"][-1]
            tan = (br["pts"][-1] - br["pts"][-2]).normalized()
            surf = env.normal(tip)
            d = (tan + surf * 0.5 - UP * fol.get("droop", 0.15)).normalized()
            up = (surf * 0.8 + UP * fol.get("face_up", 0.5)).normalized()
            if abs(up.dot(d)) > 0.95:
                up = (up + d.orthogonal()).normalized()
            anchors.append(dict(pos=tip.copy(), dir=d, up=up, size=fol["size"] * rng.uniform(0.9, 1.2),
                                phase=br["phase"], outer=env.ratio(tip), key=rng.random(),
                                spin=rng.uniform(-0.5, 0.5)))
    desc["anchors"] = anchors
    desc["center"] = center
    desc["trunk"] = [b for b in desc["branches"] if b["level"] == 0]
    print(f"HOMESTEAD_TREE branches={len(desc['branches'])} anchors={len(anchors)} "
          f"by_level={[sum(1 for b in desc['branches'] if b['level'] == k) for k in range(len(spec['levels']))]}")
    return desc


def merge_anchors(anchors, cell, rng, min_count=1, offset=None):
    """Voxel-merge foliage anchors into far-LOD cards: one per occupied cell."""
    off = offset or Vector((rng.uniform(0, cell), rng.uniform(0, cell), rng.uniform(0, cell)))
    cells = {}
    for an in anchors:
        p = an["pos"] + off
        k = (math.floor(p.x / cell), math.floor(p.y / cell), math.floor(p.z / cell))
        cells.setdefault(k, []).append(an)
    merged = []
    for group in cells.values():
        if len(group) < min_count:
            continue
        pos = sum((g["pos"] for g in group), Vector()) / len(group)
        d = sum((g["dir"] for g in group), Vector())
        up = sum((g["up"] for g in group), Vector())
        d = d.normalized() if d.length > 1e-4 else Vector((1, 0, 0))
        up = up.normalized() if up.length > 1e-4 else UP.copy()
        if abs(up.dot(d)) > 0.95:
            up = (up + d.orthogonal()).normalized()
        merged.append(dict(pos=pos, dir=d, up=up, count=len(group), phase=group[0]["phase"],
                           outer=max(g["outer"] for g in group), key=rng.random(),
                           spin=rng.uniform(-0.6, 0.6)))
    return merged


# ------------------------------------------------------------------ geometry

def trunk_tube(b, br, spec, rect, sides, seg, v_len, u_repeat):
    """Trunk sweep with a root flare, buttress lobes and a skirt below the ground line."""
    fl = spec.get("flare", {})
    amount, fh = fl.get("amount", 0.6), fl.get("height", 0.8)
    lobes = fl.get("lobes", [(5, 0.18, 0.4), (3, 0.08, 1.3), (8, 0.05, 2.1)])
    lobe_h = fl.get("lobe_height", 1.2)
    skirt = spec.get("skirt", 0.25)
    rough = fl.get("rough", 0.03)
    pts, acc, radii = br["pts"], br["acc"], br["radii"]
    L = br["length"]
    samples = [0.0]
    s = 0.0
    while s < L:
        # Dense rings near the ground (flare), coarser up the bole.
        s += seg * (0.35 if s < fh * 1.5 else 1.0)
        samples.append(min(s, L))
    ring_pts, ring_r, tangents = [], [], []
    for s in samples:
        p, t, i = S.at(pts, acc, s)
        f = (s - acc[i - 1]) / max(acc[i] - acc[i - 1], 1e-9) if i > 0 else 0.0
        r = radii[max(i - 1, 0)] + (radii[i] - radii[max(i - 1, 0)]) * f
        ring_pts.append(p)
        ring_r.append(r)
        tangents.append(t)
    # Skirt: continue the base straight down so the flare never floats on a slope.
    ring_pts.insert(0, ring_pts[0] - UP * skirt)
    ring_r.insert(0, ring_r[0])
    tangents.insert(0, tangents[0])
    samples.insert(0, -skirt)
    normal = tangents[0].cross(Vector((1, 0, 0))).normalized()
    rng = random.Random(spec.get("seed", 1) + 17)
    phases = [(k, amp, ph + rng.uniform(-0.3, 0.3)) for k, amp, ph in lobes]
    bumps = [(rng.uniform(0, math.tau), rng.uniform(0.5, 1.5)) for _ in range(4)]
    rings = []
    u0, u1 = rect[0], rect[1]
    for n, (p, r, t, s) in enumerate(zip(ring_pts, ring_r, tangents, samples)):
        if n:
            normal = (tangents[n - 1].rotation_difference(t) @ normal).normalized()
        binormal = t.cross(normal).normalized()
        z = max(s, 0.0)
        flare = 1.0 + amount * math.exp(-z / fh)
        lobe_w = math.exp(-z / lobe_h)
        ring = []
        for j in range(sides):
            ang = math.tau * j / sides
            lobe = sum(amp * math.sin(k * ang + ph) for k, amp, ph in phases) * lobe_w
            gnarl = sum(rough * math.sin(ang * (2 + q) + a0 + s * fq) for q, (a0, fq) in enumerate(bumps))
            rr = r * flare * (1.0 + lobe + gnarl)
            off = normal * math.cos(ang) + binormal * math.sin(ang)
            ring.append(b._vert(p + off * rr, 0.0, 0.0))
        rings.append(ring)
    for n in range(len(rings) - 1):
        va, vb = samples[n] / v_len, samples[n + 1] / v_len
        for j in range(sides):
            k = (j + 1) % sides
            b.faces.append((rings[n][j], rings[n][k], rings[n + 1][k], rings[n + 1][j]))
            ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
            b.uvs.append([(ua * u_repeat, va), (ub * u_repeat, va), (ub * u_repeat, vb), (ua * u_repeat, vb)])
    # Cap the top so a short leader never shows a hole.
    tip = b._vert(ring_pts[-1] + tangents[-1] * ring_r[-1] * 0.5, 0.0, 0.0)
    v = samples[-1] / v_len
    for j in range(sides):
        k = (j + 1) % sides
        b.faces.append((rings[-1][j], rings[-1][k], tip))
        b.uvs.append([(j / sides * u_repeat, v), ((j + 1) / sides * u_repeat, v), (0.5 * u_repeat, v + 0.01)])
    return (ring_r[1] if len(ring_r) > 1 else ring_r[0])


def emit_wood(b, desc, spec, lod):
    lods = spec["lods"][lod]
    max_level = lods["levels"]
    min_r = lods.get("min_radius", 0.0)
    seg = lods.get("step", 0.25)
    v_len = spec.get("bark_size", 1.0)
    fol = spec["foliage"]
    leaf_level = fol.get("level", len(spec["levels"]) - 1)
    for br in desc["branches"]:
        lvl = br["level"]
        if lvl > max_level or br["radii"][0] < min_r:
            continue
        sides = lods["sides"][min(lvl, len(lods["sides"]) - 1)]
        circ = math.tau * br["radii"][0]
        u_repeat = max(1, round(circ / v_len))
        if lvl == 0 and spec.get("trunk_flare", True):
            trunk_tube(b, br, spec, (0.0, 1.0), lods.get("trunk_sides", sides), seg, v_len, u_repeat)
            continue
        pts, radii = br["pts"], br["radii"]
        if lvl >= leaf_level:
            # The cluster card paints this twig's leafy end; model only the bare part below it.
            keep = br["length"] - fol["zone"] * lods.get("twig_cut", 0.8)
            if keep < 0.08:
                continue
            cut = next((i for i, s in enumerate(br["acc"]) if s >= keep), len(pts))
            pts, radii = pts[:cut + 1], radii[:cut + 1]
            if len(pts) < 2:
                continue
        spacing = br["length"] / max(len(br["pts"]) - 1, 1)
        stride = max(1, int(round(seg * (1.0 if lvl <= 1 else 0.7) / max(spacing, 1e-4))))
        idx = list(range(0, len(pts), stride))
        if idx[-1] != len(pts) - 1:
            idx.append(len(pts) - 1)
        if len(idx) < 2:
            continue
        b.tube([pts[i] for i in idx], [radii[i] for i in idx], sides, (0.0, float(u_repeat), 0.0, 1.0),
               v_length=v_len, v_offset=br["phase"] * 3.1, phase=br["phase"], flutter=0.0,
               cap=False, roll=br["phase"] * 6.28)


def emit_leaves(b, cards, atlas, keys, spec, lod):
    lods = spec["lods"][lod]
    rows, cols = lods.get("grid", (1, 1))
    fold = lods.get("fold", 0.0)
    for c in cards:
        key = keys[int(c["key"] * len(keys)) % len(keys)]
        size = c["size"]
        width = size * S.tile_aspect(atlas, key)
        up = Matrix.Rotation(c["spin"], 3, c["dir"]) @ c["up"]
        base = c["pos"] - c["dir"] * size * lods.get("back", 0.1)
        b.card(base, c["dir"], up, size, width, atlas.uv(key), rows=rows, cols=cols, fold=fold,
               droop=lods.get("droop", 0.1), phase=c["phase"], flutter=0.9, flutter_base=0.15)


def build_mesh(name, wood, leaves, bark_mat, leaf_mat, desc, spec):
    """One mesh with material slot 0 bark, 1 leaves; custom normals bend leaf cards toward the
    crown ellipsoid so the canopy shades as a volume."""
    nw = len(wood.verts)
    verts = wood.verts + leaves.verts
    faces = wood.faces + [tuple(i + nw for i in f) for f in leaves.faces]
    uvs = wood.uvs + leaves.uvs
    wind = wood.wind + leaves.wind
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name="UVMap")
    uv.data.foreach_set("uv", [c for face in uvs for loop in face for c in loop])
    attr = mesh.attributes.new("Wind", "BYTE_COLOR", "POINT")
    attr.data.foreach_set("color_srgb", [c for w in wind for c in w])
    mesh.materials.append(bark_mat)
    mesh.materials.append(leaf_mat)
    nwf = len(wood.faces)
    mesh.polygons.foreach_set("material_index", [0] * nwf + [1] * len(leaves.faces))
    mesh.shade_smooth()
    mesh.update()
    env = desc["env"]
    c = desc["center"]
    rx, ry = env.rx, env.ry
    rz = max(spec["height"] - c.z, c.z - spec["height"] * spec.get("crown_base", 0.3), 1.0)
    blend = spec.get("normal_blend", 0.65)
    vnorm = [Vector(v.normal) for v in mesh.vertices]
    loop_normals = []
    for poly in mesh.polygons:
        leaf = poly.material_index == 1
        for vi in poly.vertices:
            n = vnorm[vi]
            if leaf:
                p = Vector(verts[vi])
                e = Vector(((p.x - c.x) / (rx * rx), (p.y - c.y) / (ry * ry), (p.z - c.z) / (rz * rz)))
                e = e.normalized() if e.length > 1e-9 else UP
                # Keep the card's front side; bend it toward the crown surface normal.
                if n.dot(e) < 0:
                    n = -n
                n = (n * (1 - blend) + e * blend).normalized()
            loop_normals.append(tuple(n))
    mesh.normals_split_custom_set(loop_normals)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]
    return obj


def capsule(desc, spec):
    """Trunk capsule in cm (Unreal space, Z up): radius at breast height, from the ground to
    ``capsule_height`` metres (or the first fork)."""
    trunk = desc["trunk"][0]
    h = min(spec.get("capsule_height", 3.5), trunk["length"] * 0.8)
    node, _, i = S.at(trunk["pts"], trunk["acc"], min(1.3, trunk["length"] * 0.5))
    r = trunk["radii"][i] * spec.get("capsule_scale", 1.0)
    top, _, _ = S.at(trunk["pts"], trunk["acc"], h)
    base = trunk["pts"][0]
    center = (base + top) * 0.5
    length = max((top - base).length - 2 * r, 0.0)
    axis = (top - base).normalized() if (top - base).length > 1e-4 else UP.copy()
    return dict(center=center, radius=r, length=length, height=h, axis=axis)


def build(kit, spec, bark_atlas, leaf_atlas, cluster_keys, mass_keys, name):
    """Grow, emit four LODs with two material slots, and place the trunk base at the origin.
    Returns (objects, desc, capsule dict in metres relative to the exported pivot)."""
    rng = random.Random(spec["seed"])
    desc = grow(spec, rng)
    bark_mat = bark_atlas.material()
    bark_mat.name = f"M_{name}Bark"
    leaf_mat = leaf_atlas.material()
    leaf_mat.name = f"M_{name}Leaves"
    objs = []
    mrng = random.Random(spec["seed"] + 99)
    for lod in range(len(spec["lods"])):
        lods = spec["lods"][lod]
        wood = F.Batch(height=spec["height"])
        emit_wood(wood, desc, spec, lod)
        leaves = F.Batch(height=spec["height"])
        if lods.get("cell"):
            cards = merge_anchors(desc["anchors"], lods["cell"], mrng, lods.get("min_count", 1))
            for cd in cards:
                cd["size"] = lods["cell"] * lods.get("card_scale", 1.4) * mrng.uniform(0.85, 1.15)
            keys = mass_keys if lods.get("mass") else cluster_keys
        else:
            cards = desc["anchors"]
            keys = cluster_keys
        emit_leaves(leaves, cards, leaf_atlas, keys, spec, lod)
        obj = build_mesh(name if lod == 0 else f"{name}_LOD{lod}", wood, leaves, bark_mat, leaf_mat, desc, spec)
        print(f"HOMESTEAD_TREE_LOD {lod} wood={wood.triangles} leaves={leaves.triangles} cards={len(cards)}")
        objs.append(obj)
    # Pivot: the trunk's ground point (skirt bottom) at the origin, shared by every LOD.
    base = desc["trunk"][0]["pts"][0] - UP * spec.get("skirt", 0.25)
    shift = Vector((base.x, base.y, base.z))
    for obj in objs:
        obj.data.transform(Matrix.Translation(-shift))
        obj.data.update()
        obj["homestead_shift"] = list(shift)
        obj["homestead_finalized"] = True
    cap = capsule(desc, spec)
    cap["center"] = cap["center"] - shift
    return objs, desc, cap


def capsule_report(cap):
    ax = cap.get("axis", UP)
    return {"center_cm": [round(cap["center"].x * 100, 1), round(-cap["center"].y * 100, 1),
                          round(cap["center"].z * 100, 1)],
            "radius_cm": round(cap["radius"] * 100, 1), "length_cm": round(cap["length"] * 100, 1),
            "axis": [round(ax.x, 4), round(-ax.y, 4), round(ax.z, 4)],
            "note": "Unreal space (Y mirrored from Blender). Length is the cylinder part; the "
                    "capsule spans the ground to the first fork along the trunk axis."}
