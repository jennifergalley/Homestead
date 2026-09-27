"""Stone foundation layout (Unreal local cm on paper; see ``__init__``).

A 300 x 300 cm platform, walking surface at Z = 0, pivot at its centre: a ring of long
granite kerb stones (34 cm wide, 14 cm deep) round an infill of split flagstones pointed
with lime mortar, on a rubble footing down to Z = -40. Everything stays inside
X, Y = -150..150 above ground, so neighbouring foundations at 300 cm spacing butt without
overlapping; relief only bulges outward below the kerbs, inside the neighbour.
"""
import numpy as np

from stone_building.masonry import CM, Masonry, cm, unwrap
from stone_building.wall import stone_material

HALF, KERB, KERB_DEPTH, DEPTH = 150.0, 34.0, 14.0, 40.0
MAT_STONE, MAT_MORTAR = 0, 1


def _split(lo, hi, rng, short=34.0, long=78.0):
    """Cut lo..hi into pieces short..long cm."""
    cuts = [lo]
    while hi - cuts[-1] > long:
        cuts.append(cuts[-1] + rng.uniform(short, long))
    if hi - cuts[-1] < short and len(cuts) > 1:
        cuts[-1] = (cuts[-2] + hi) / 2
    cuts.append(hi)
    return list(zip(cuts[:-1], cuts[1:]))


def _flags(lo, hi, rng, out, depth=0):
    """Guillotine-split a rectangle into flagstones 38..95 cm."""
    size = hi - lo
    if size.max() <= 95.0 and (size.max() < 60.0 or rng.random() < 0.55 or depth > 5):
        out.append((lo, hi))
        return
    axis = int(np.argmax(size + rng.uniform(0, 15, 2)))
    if size[axis] < 76.0:
        axis = 1 - axis
    if size[axis] < 76.0:
        out.append((lo, hi))
        return
    cut = lo[axis] + rng.uniform(38.0, size[axis] - 38.0)
    a_hi, b_lo = hi.copy(), lo.copy()
    a_hi[axis], b_lo[axis] = cut, cut
    _flags(lo, a_hi, rng, out, depth + 1)
    _flags(b_lo, hi, rng, out, depth + 1)


def build(seed=3):
    rng = np.random.default_rng(seed)
    m = Masonry(seed, pcoord_shift=(0.0, 0.0, 0.06), tint_mean=0.86)
    inner = HALF - KERB

    def kerb(x0, x1, y0, y1):
        m.block(cm(x0, y0, -KERB_DEPTH), cm(x1, y1, 0.0), mat=MAT_STONE, radius=0.014, relief=0.0022,
                step=0.2, bulge=0.002, skip=(("z", -1),), relief_scale=6.0)

    for sx in (-1, 1):
        for sy in (-1, 1):
            xs = sorted((sx * (inner + 0.5), sx * HALF))
            ys = sorted((sy * (inner + 0.5), sy * HALF))
            kerb(xs[0], xs[1], ys[0], ys[1])
    for a, b in _split(-inner, inner, rng, 40.0, 72.0):
        kerb(a + 0.5, b - 0.5, inner + 0.5, HALF)
    for a, b in _split(-inner, inner, rng, 40.0, 72.0):
        kerb(a + 0.5, b - 0.5, -HALF, -inner - 0.5)
    for a, b in _split(-inner, inner, rng, 40.0, 72.0):
        kerb(inner + 0.5, HALF, a + 0.5, b - 0.5)
    for a, b in _split(-inner, inner, rng, 40.0, 72.0):
        kerb(-HALF, -inner - 0.5, a + 0.5, b - 0.5)
    # Flagstone infill, laid nearly flat and walkable.
    flags = []
    _flags(np.array([-inner, -inner]), np.array([inner, inner]), rng, flags)
    for lo, hi in flags:
        joint = rng.uniform(1.0, 1.7)
        centre = (lo + hi) / 2
        half = (hi - lo) / 2 - joint / 2
        m.pillow(cm(centre[0], centre[1], 0.0), (0, 0, 1), np.array([0, 1.0, 0]), half[0] * CM, half[1] * CM,
                 mat=MAT_STONE, bulge=0.0025, max_out=0.0025, relief=0.002, tilt=0.004, recess=(-0.004, -0.001),
                 roll=0.004, skirt=0.03, cuts=(0, 1), relief_scale=4.0)
    # Rubble footing below the kerbs on all four sides.
    for normal, up in (((0, 1, 0), (0, 0, 1)), ((0, -1, 0), (0, 0, 1)), ((1, 0, 0), (0, 0, 1)), ((-1, 0, 0), (0, 0, 1))):
        n = np.array(normal, dtype=np.float64)
        along = np.cross(np.array(up, dtype=np.float64), n)
        z_rows = [(-DEPTH, -27.5), (-27.5, -KERB_DEPTH - 0.6)] if rng.random() < 0.6 else [(-DEPTH, -KERB_DEPTH - 0.6)]
        for z0, z1 in z_rows:
            for a, b in _split(-HALF, HALF, rng, 24.0, 60.0):
                joint = rng.uniform(1.2, 2.0)
                hv = (z1 - z0) / 2 - joint / 2
                centre = n * HALF + along * (a + b) / 2 + np.array([0, 0, (z0 + z1) / 2])
                m.pillow(centre * CM, n, np.array(up, dtype=np.float64), ((b - a) / 2 - joint / 2) * CM, hv * CM,
                         mat=MAT_STONE, bulge=0.008, max_out=0.006, relief=0.006)
    # Mortar core: its top shows in the flag joints.
    m.box(cm(-HALF + 1.5, -HALF + 1.5, -DEPTH), cm(HALF - 1.5, HALF - 1.5, -1.0), MAT_MORTAR, skip=(("z", -1),))
    return m


def make(kit, name, seed=3):
    mats = [stone_material(kit, f"M_{name[3:]}Stone", seed, lichen=0.15, moss=0.05, soil=0.35,
                           soil_height=0.1, film=0.55, iron=0.25, streaks=0.05),
            kit.mats.lime_mortar(f"M_{name[3:]}Mortar", seed=seed, grime=0.8)]
    m = build(seed)
    obj = m.build(name, mats)
    unwrap(obj, shrink={MAT_MORTAR: 0.35})
    return obj
