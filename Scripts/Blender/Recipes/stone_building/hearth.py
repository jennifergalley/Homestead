"""Granite hearth for the manor's standing room: an open kitchen fireplace built out from the
wall on the piece's own edge (Unreal local cm on paper; see ``__init__``).

The wall's inner face is at Y = 130. The chimney breast projects 64 cm into the room
(Y 66..130) and is 170 cm wide, rising to the wall head at Z 258 under the roof slab. The
fire opening is 112 cm wide and 105 cm high between two dressed granite jambs, spanned by
one granite lintel. Inside, the firebox's back and cheeks are soot-blackened rubble over an
ash bed. A pair of wrought-iron firedogs carries three charred split logs. In front, a large
granite hearthstone runs from Y 42 to the wall.
"""
import numpy as np

from stone_building.masonry import CM, Masonry, cm

WALL = 130.0
FRONT = 66.0
HALF_WIDTH = 85.0
OPEN_HALF = 56.0
OPEN_TOP = 108.0
LINTEL_TOP = 140.0
HEAD = 258.0
BACK = 122.0
MAT_STONE, MAT_MORTAR, MAT_IRON, MAT_WOOD, MAT_ASH, MAT_SOOT = 0, 1, 2, 3, 4, 5


def _courses(z0, z1, rng, low=17.0, high=30.0):
    bounds, z = [z0], z0
    while z1 - z > high:
        z += rng.uniform(low, high)
        bounds.append(z)
    bounds.append(z1)
    return list(zip(bounds[:-1], bounds[1:]))


def _cells(u0, u1, height, rng):
    cells, u = [], u0
    while u1 - u > 1e-6:
        end = u + float(np.clip(height * rng.uniform(1.2, 2.6), 22.0, 70.0))
        if u1 - end < 16.0:
            end = u1
        cells.append((u, end))
        u = end
    return cells


def rubble(m, rng, origin, normal, axis, u0, u1, z0, z1, soot=0.0):
    """Random rubble brought to courses on a vertical face through ``origin`` (cm) facing
    ``normal``, running along ``axis`` from u0 to u1 and up from z0 to z1. ``soot`` darkens
    the stones toward the fire (0 = clean, 1 = black), fading with height."""
    origin, normal, axis = (np.asarray(v, dtype=np.float64) for v in (origin, normal, axis))
    up = np.array([0.0, 0.0, 1.0])
    for za, zb in _courses(z0, z1, rng):
        for ua, ub in _cells(u0, u1, zb - za, rng):
            joint = rng.uniform(1.5, 2.6)
            hu = (ub - ua) / 2 - joint / 2
            hv = max((zb - za) / 2 - joint / 2 - rng.uniform(0.0, 0.8), 3.0)
            centre = origin + axis * ((ua + ub) / 2) + up * (za + joint / 2 + hv)
            offset, tint = m.identity()
            if soot > 0.0:
                fade = float(np.clip(1.0 - (centre[2] - OPEN_TOP) / 90.0, 0.0, 1.0)) if centre[2] > OPEN_TOP else 1.0
                tint = tint * (1.0 - 0.93 * soot * fade * rng.uniform(0.9, 1.0))
            m.pillow(centre * CM, normal, up, hu * CM, hv * CM, mat=MAT_STONE,
                     bulge=rng.uniform(0.004, 0.016), max_out=0.012, identity=(offset, tint))


def _dressed(m, rng, lo, hi, soot=0.0, skip=(), bulge=0.006):
    offset, tint = m.identity(0.08)
    if soot:
        tint = tint * (1.0 - soot)
    m.block(cm(*lo), cm(*hi), mat=MAT_STONE, radius=0.02, relief=0.003, step=0.2,
            bulge=bulge * rng.uniform(0.5, 1.0), skip=skip, identity=(offset, tint))


def build(seed=5):
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.84)
    # Hearthstone: one big slab, worn smooth, a little sooted where the fire spills.
    _dressed(m, rng, (-96.0, 40.0, -3.0), (96.0, WALL - 1.0, 3.0), bulge=0.002)
    # Jambs: a long and a short dressed stone each side.
    split = 58.0
    for side in (-1, 1):
        lo, hi = sorted((side * (OPEN_HALF + 0.4), side * HALF_WIDTH))
        _dressed(m, rng, (lo, FRONT, 3.2), (hi, WALL - 1.0, split), soot=0.15)
        _dressed(m, rng, (lo, FRONT + 2.0, split + 0.5), (hi, WALL - 1.0, OPEN_TOP), soot=0.2)
    # Lintel: one granite beam, slightly proud, sooted on its underside and inner half.
    _dressed(m, rng, (-HALF_WIDTH - 5.0, FRONT - 2.5, OPEN_TOP + 0.4), (HALF_WIDTH + 5.0, WALL - 1.0, LINTEL_TOP),
             soot=0.28, bulge=0.004)
    # Chimney breast above the lintel: rubble front and sides over a mortar core.
    rubble(m, rng, (0.0, FRONT, 0.0), (0, -1, 0), (1, 0, 0), -HALF_WIDTH, HALF_WIDTH, LINTEL_TOP + 0.5, HEAD)
    for side in (-1, 1):
        rubble(m, rng, (side * HALF_WIDTH, 0.0, 0.0), (side, 0, 0), (0, 1, 0), FRONT + 1.0, WALL - 1.0,
               LINTEL_TOP + 0.5, HEAD)
    m.box(cm(-HALF_WIDTH + 2.5, FRONT + 2.5, LINTEL_TOP), cm(HALF_WIDTH - 2.5, WALL, HEAD), MAT_MORTAR,
          skip=(("y", 1), ("z", 1)))
    # Firebox: sooted rubble back and cheeks, a dark mortar lining and a flue throat.
    rubble(m, rng, (0.0, BACK, 0.0), (0, -1, 0), (1, 0, 0), -OPEN_HALF, OPEN_HALF, 3.0, OPEN_TOP, soot=1.0)
    for side in (-1, 1):
        rubble(m, rng, (side * OPEN_HALF, 0.0, 0.0), (-side, 0, 0), (0, 1, 0), FRONT + 3.0, BACK,
               3.0, OPEN_TOP, soot=0.9)
    m.box(cm(-OPEN_HALF - 3.0, BACK + 1.5, 0.0), cm(OPEN_HALF + 3.0, WALL, OPEN_TOP + 1.0), MAT_SOOT,
          skip=(("y", 1),))
    # Ash bed and embers' floor.
    offset, _ = m.identity()
    m.block(cm(-44.0, 84.0, 2.5), cm(44.0, BACK - 1.0, 6.5), mat=MAT_ASH, radius=0.018, relief=0.006,
            step=0.08, bulge=0.012, skip=(("z", -1),), identity=(offset, np.ones(3)))
    # Firedogs: a bar front to back on each side with a knobbed front upright.
    for x in (-27.0, 27.0):
        m.block(cm(x - 1.4, 76.0, 9.0), cm(x + 1.4, BACK - 3.0, 12.0), mat=MAT_IRON, radius=0.006, step=0.06,
                relief=0.0008)
        m.block(cm(x - 1.8, 74.0, 3.0), cm(x + 1.8, 77.5, 30.0), mat=MAT_IRON, radius=0.007, step=0.06,
                relief=0.0008)
        m.block(cm(x - 3.0, 73.0, 29.0), cm(x + 3.0, 78.5, 35.0), mat=MAT_IRON, radius=0.02, step=0.03,
                relief=0.001)
        for y in (78.0, BACK - 5.0):
            m.block(cm(x - 1.5, y - 1.5, 3.0), cm(x + 1.5, y + 1.5, 10.0), mat=MAT_IRON, radius=0.006, step=0.06)
    # Three charred split logs across the firedogs.
    # Split billets: rounded on the bark side, the lower two resting on the firedogs' bars.
    for (y0, z0, size) in ((86.0, 12.2, 13.0), (102.0, 12.2, 14.5), (94.5, 25.5, 12.0)):
        length = rng.uniform(78.0, 90.0)
        x0 = rng.uniform(-7.0, 7.0) - length / 2
        m.block(cm(x0, y0, z0), cm(x0 + length, y0 + size, z0 + size * rng.uniform(0.85, 0.95)),
                mat=MAT_WOOD, radius=size * 0.0045, relief=0.006, step=0.04, pcoord_axis=0, relief_scale=18.0)
    return m


def make(kit, name, seed=5):
    from stone_building.wall import stone_material
    mats = [stone_material(kit, f"M_{name[3:]}Stone", seed, lichen=0.12, moss=0.02, soil=0.2),
            kit.mats.lime_mortar(f"M_{name[3:]}Mortar", seed=seed),
            kit.mats.wrought_iron(f"M_{name[3:]}Iron", rust=0.25, wear=0.2, seed=seed),
            kit.mats.bark(f"M_{name[3:]}Charred", light=(0.035, 0.028, 0.022), dark=(0.008, 0.007, 0.006), scale=2.6,
                          roughness=0.95),
            kit.mats.soil(f"M_{name[3:]}Ash", damp=(0.10, 0.095, 0.09), dry=(0.24, 0.23, 0.215), seed=seed),
            kit.mats.lime_mortar(f"M_{name[3:]}Soot", color=(0.035, 0.032, 0.03), dirt=(0.012, 0.011, 0.01),
                                 seed=seed)]
    m = build(seed=seed)
    return m.build(name, mats)
