"""Ruined-manor masonry: broken granite wall runs, a window run and a chimney stack
(Unreal local cm on paper; see ``__init__``).

A wall run lies along X from -L/2 to +L/2, centred on Y = 0 with faces at Y = +-T/2, from a
buried footing at Z = -15. Its top is broken: a seeded profile of stepped, ragged heights
between ``low`` and ``high`` (cm). Stones are laid as random rubble brought to courses on both
faces; any stone whose top would stand above the profile at either of its ends has fallen, so
the broken top reads as missing masonry rather than a sliced edge. The rubble core shows on the
top, stepping with the profile, capped by a few loose stones. Ends are dressed quoins up to the
profile, so runs butt into corners and free-standing ends alike.
"""
import numpy as np

from stone_building.masonry import CM, Masonry, cm

MAT_STONE, MAT_MORTAR = 0, 1
FOOT = -15.0


def profile(length, low, high, seed, jagged=0.35):
    """A broken-top height function over u in [-L/2, L/2] (cm): a few plateaus joined by
    collapses, with small-scale raggedness."""
    rng = np.random.default_rng(seed)
    knots = max(3, int(length / 120.0))
    xs = np.linspace(-length / 2, length / 2, knots)
    ys = rng.uniform(low, high, knots)
    # Plateaus: hold some neighbouring knots level, the way a wall stands in sound sections.
    for i in range(1, knots):
        if rng.random() < 0.4:
            ys[i] = ys[i - 1] + rng.normal(0.0, (high - low) * 0.05)
    ys = np.clip(ys, low, high)
    rag_phase = rng.uniform(0, 100, 3)

    def top(u):
        u = np.asarray(u, dtype=np.float64)
        base = np.interp(u, xs, ys)
        rag = (np.sin(u * 0.043 + rag_phase[0]) * 0.5 + np.sin(u * 0.11 + rag_phase[1]) * 0.3
               + np.sin(u * 0.27 + rag_phase[2]) * 0.2)
        return base + rag * jagged * 30.0

    return top


def _courses(z0, z1, rng, low=18.0, high=32.0):
    bounds, z = [z0], z0
    while z1 - z > high:
        z += rng.uniform(low, high)
        bounds.append(z)
    bounds.append(z1)
    return list(zip(bounds[:-1], bounds[1:]))


def _cells(u0, u1, height, rng):
    cells, u = [], u0
    while u1 - u > 1e-6:
        end = u + float(np.clip(height * rng.uniform(1.3, 2.8), 24.0, 80.0))
        if u1 - end < 18.0:
            end = u1
        cells.append((u, end))
        u = end
    return cells


def _stone(m, rng, centre, normal, axis, hu, hv, tint_scale=1.0, up=(0.0, 0.0, 1.0)):
    offset, tint = m.identity(0.13)
    m.pillow(centre * CM, normal, np.array(up, dtype=np.float64), hu * CM, hv * CM, mat=MAT_STONE,
             bulge=rng.uniform(0.004, 0.02), max_out=0.014, identity=(offset, tint * tint_scale))


COLUMN = 15.0


def _free(u0, u1, za, zb, holes):
    """The parts of u0..u1 not covered by openings in the course za..zb."""
    spans = [(u0, u1)]
    for h0, h1, z0, z1 in holes:
        if za >= z1 or zb <= z0:
            continue
        spans = [piece for a, b in spans for piece in ((a, min(b, h0)), (max(a, h1), b)) if piece[1] - piece[0] > 1.0]
    return spans


def rubble_face(m, rng, y, normal, u0, u1, top, holes=(), z_top=None, swap=False, laid=None):
    """Rubble on the face at Y = ``y`` facing ``normal`` between u0 and u1 up to ``top(u)``.
    ``holes`` are (u0, u1, z0, z1) openings left bare. ``swap`` lays the face along Y at X = ``y``.
    ``laid`` (columns of COLUMN cm from u0) records the top of the masonry actually laid."""
    peak = float(np.max(top(np.linspace(u0, u1, 64)))) if z_top is None else z_top
    for za, zb in _courses(FOOT, peak, rng):
        for a, b in _free(u0, u1, za, zb, holes):
            for ua, ub in _cells(a, b, zb - za, rng):
                if zb > min(top(ua), top(ub), top((ua + ub) / 2)) + 4.0:
                    continue
                joint = rng.uniform(1.6, 2.8)
                hu = (ub - ua) / 2 - joint / 2
                if hu < 5.0:
                    continue
                hv = max((zb - za) / 2 - joint / 2 - rng.uniform(0.0, 0.8), 3.0)
                u = (ua + ub) / 2
                centre = np.array([y, u, za + joint / 2 + hv] if swap else [u, y, za + joint / 2 + hv])
                _stone(m, rng, centre, normal, None, hu, hv)
                if laid is not None:
                    i0, i1 = int((ua - u0) // COLUMN), int(np.ceil((ub - u0) / COLUMN))
                    laid[max(i0, 0):min(i1, len(laid))] = np.maximum(laid[max(i0, 0):min(i1, len(laid))], zb)

def quoin_end(m, rng, x, half_t, top, sign):
    """Dressed long-and-short quoins at the run's end X = ``x`` up to the profile there."""
    z, k, peak = FOOT, 0, float(top(x))
    while z < peak - 8.0:
        h = rng.uniform(24.0, 34.0)
        if z + h > peak + 6.0:
            break
        depth = 46.0 if k % 2 == 0 else 26.0
        lo = np.array([x - sign * depth if sign > 0 else x, -half_t - 1.0, z + 0.3])
        hi = np.array([x if sign > 0 else x + depth, half_t + 1.0, z + h - 0.3])
        offset, tint = m.identity(0.07)
        m.block(cm(*lo), cm(*hi), mat=MAT_STONE, radius=0.02, relief=0.004, step=0.2,
                bulge=0.005 * rng.uniform(0.5, 1.0), identity=(offset, tint), skip=(("z", -1),))
        z += h
        k += 1
    return z


def core(m, rng, u0, u1, half_t, heights, holes=()):
    """The rubble-and-lime core behind the faces, stepping just below the masonry laid on them
    (``heights`` per COLUMN from u0), capped with a few loose stones lying on it."""
    for i, h in enumerate(heights):
        ua = u0 + i * COLUMN
        ub = min(ua + COLUMN, u1)
        h = float(h) - 5.0
        for za, zb in [(a, b) for a, b in [(FOOT, h)]]:
            spans = [(za, zb)]
            for h0, h1, z0, z1 in holes:
                if ua < h1 and ub > h0:
                    spans = [piece for a, b in spans for piece in ((a, min(b, z0)), (max(a, z1), b))]
            for sa, sb in spans:
                if sb - sa > 1.0:
                    m.box(cm(ua, -half_t + 3.5, sa), cm(ub, half_t - 3.5, sb), MAT_MORTAR,
                          skip=(("z", -1),))
    for _ in range(int((u1 - u0) / 40.0)):
        i = int(rng.integers(1, max(2, len(heights) - 1)))
        h = float(min(heights[i - 1], heights[i], heights[min(i + 1, len(heights) - 1)])) - 5.0
        if h < 10.0:
            continue
        u = u0 + (i + 0.5) * COLUMN
        _stone(m, rng, np.array([u, rng.uniform(-half_t * 0.35, half_t * 0.35), h + 2.0]), (0, 0, 1),
               None, rng.uniform(9, 15), rng.uniform(7, 11), up=(1.0, 0.0, 0.0))

def window(m, rng, u0, u1, z0, z1, half_t):
    """A sash-window opening: dressed granite jambs, a sill and one lintel."""
    for u in (u0, u1):
        z, k = z0, 0
        while z < z1 - 1:
            h = min(rng.uniform(26.0, 36.0), z1 - z)
            width = 30.0 if k % 2 == 0 else 20.0
            lo, hi = (u - width, u) if u == u0 else (u, u + width)
            offset, tint = m.identity(0.07)
            m.block(cm(lo, -half_t - 0.8, z + 0.3), cm(hi, half_t + 0.8, z + h - 0.3), mat=MAT_STONE,
                    radius=0.018, relief=0.003, step=0.2, bulge=0.004, identity=(offset, tint))
            z += h
            k += 1
    offset, tint = m.identity(0.06)
    m.block(cm(u0 - 12, -half_t - 4.0, z0 - 9.0), cm(u1 + 12, half_t + 1.0, z0 - 0.3), mat=MAT_STONE,
            radius=0.02, relief=0.003, step=0.2, bulge=0.003, identity=(offset, tint))
    offset, tint = m.identity(0.06)
    m.block(cm(u0 - 28, -half_t - 1.5, z1 + 0.3), cm(u1 + 28, half_t + 1.5, z1 + 28.0), mat=MAT_STONE,
            radius=0.022, relief=0.003, step=0.2, bulge=0.004, identity=(offset, tint))


def wall_run(length, low, high, seed, thickness=56.0, windows=(), ends=(True, True)):
    """One broken run. ``windows``: (centre_u, width, sill_z, head_z) openings."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.72)
    half_t = thickness / 2
    top = profile(length, low, high, seed + 17)
    holes = [(c - w / 2, c + w / 2, sill - 9.0, head + 28.0) for c, w, sill, head in windows]
    # Openings keep their lintel only where the wall still stands above it.
    kept = [(c, w, sill, head) for c, w, sill, head in windows if top(c) > head + 30.0]
    fallen = [(c, w, sill, head) for c, w, sill, head in windows if top(c) <= head + 30.0]
    holes_face = [(c - w / 2 - 20, c + w / 2 + 20, sill - 9.0, head + 28.0) for c, w, sill, head in kept]
    holes_face += [(c - w / 2, c + w / 2, sill, 10000.0) for c, w, sill, head in fallen]
    # Face rubble runs under the long quoins, so the short ones leave no gap.
    u0, u1 = -length / 2 + (26.0 if ends[0] else 0.0), length / 2 - (26.0 if ends[1] else 0.0)
    c0, c1 = -length / 2 + 2, length / 2 - 2
    columns = int(np.ceil((c1 - c0) / COLUMN))
    faces = []
    for y, normal in ((-half_t, (0, -1, 0)), (half_t, (0, 1, 0))):
        laid = np.full(columns, FOOT)
        rubble_face(m, rng, y, normal, u0, u1, top, holes_face, laid=laid)
        faces.append(laid)
    # Near the ends the core sits under the quoins, as high as they stand.
    ends_top = [quoin_end(m, rng, -length / 2, half_t, top, -1) if ends[0] else FOOT,
                quoin_end(m, rng, length / 2, half_t, top, 1) if ends[1] else FOOT]
    heights = np.minimum(faces[0], faces[1])
    centres = c0 + (np.arange(columns) + 0.5) * COLUMN
    heights = np.where(centres < u0, ends_top[0], np.where(centres > u1, ends_top[1], heights))
    core(m, rng, c0, c1, half_t, heights,
         [(c - w / 2, c + w / 2, sill - 9.0, head + 28.0 if (c, w, sill, head) in kept else 10000.0)
          for c, w, sill, head in windows])
    for c, w, sill, head in kept:
        window(m, rng, c - w / 2, c + w / 2, sill, head, half_t)
    for c, w, sill, head in fallen:
        # The sill survives; the jambs stand only as high as the wall does.
        offset, tint = m.identity(0.06)
        m.block(cm(c - w / 2 - 12, -half_t - 4.0, sill - 9.0), cm(c + w / 2 + 12, half_t + 1.0, sill - 0.3),
                mat=MAT_STONE, radius=0.02, relief=0.003, step=0.2, bulge=0.003, identity=(offset, tint))
    return m


def chimney(seed, width=150.0, depth=110.0, height=880.0):
    """A tall granite chimney stack left standing when the gable fell: rubble faces between
    dressed quoins, a slight weathered cap and a sooted flue mouth."""
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.78)
    hx, hy = width / 2, depth / 2

    def top(u):
        return np.full_like(np.asarray(u, dtype=np.float64), height)

    rubble_face(m, rng, -hy, (0, -1, 0), -hx + 26, hx - 26, top)
    rubble_face(m, rng, hy, (0, 1, 0), -hx + 26, hx - 26, top)
    rubble_face(m, rng, -hx, (-1, 0, 0), -hy + 26, hy - 26, top, swap=True)
    rubble_face(m, rng, hx, (1, 0, 0), -hy + 26, hy - 26, top, swap=True)
    # Corner quoins, long and short.
    z, k = FOOT, 0
    while z < height - 1:
        h = min(rng.uniform(26.0, 34.0), height - z)
        for sx in (-1, 1):
            for sy in (-1, 1):
                along_x = k % 2 == 0
                lx = 44.0 if along_x else 26.0
                ly = 26.0 if along_x else 44.0
                x0, x1 = sorted((sx * hx, sx * (hx - lx)))
                y0, y1 = sorted((sy * hy, sy * (hy - ly)))
                offset, tint = m.identity(0.07)
                m.block(cm(x0 - 1, y0 - 1, z + 0.3), cm(x1 + 1, y1 + 1, z + h - 0.3), mat=MAT_STONE,
                        radius=0.018, relief=0.003, step=0.2, bulge=0.004, identity=(offset, tint))
        z += h
        k += 1
    m.box(cm(-hx + 4, -hy + 4, FOOT), cm(hx - 4, hy - 4, height - 2), MAT_MORTAR, skip=(("z", -1), ("z", 1)))
    # Cap course, then the dark flue mouth.
    offset, tint = m.identity(0.05)
    m.block(cm(-hx - 5, -hy - 5, height), cm(hx + 5, hy + 5, height + 16), mat=MAT_STONE, radius=0.025,
            relief=0.004, step=0.2, bulge=0.004, identity=(offset, tint), skip=(("z", 1),))
    for x0, x1, y0, y1 in ((-hx - 5, hx + 5, -hy - 5, -24), (-hx - 5, hx + 5, 24, hy + 5),
                           (-hx - 5, -34, -24, 24), (34, hx + 5, -24, 24)):
        offset, tint = m.identity(0.05)
        m.block(cm(x0, y0, height + 15.5), cm(x1, y1, height + 22), mat=MAT_STONE, radius=0.02, relief=0.003,
                step=0.2, identity=(offset, tint), skip=(("z", -1),))
    offset, _ = m.identity()
    m.box(cm(-34, -24, height - 30), cm(34, 24, height + 17), MAT_MORTAR, skip=(("z", 1),))
    return m


def materials(kit, name, seed):
    from stone_building.wall import stone_material
    return [stone_material(kit, f"M_{name[3:]}Stone", seed, lichen=0.7, moss=0.3, soil=0.55, streaks=0.45),
            kit.mats.lime_mortar(f"M_{name[3:]}Mortar", seed=seed, grime=0.85)]
