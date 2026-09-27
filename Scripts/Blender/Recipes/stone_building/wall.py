"""Stone wall and doorway layout (Unreal local cm on paper; see ``__init__``).

The wall runs along X from -150 to +150 with its faces on Y = 130 (inner, toward the
cell centre) and Y = 158 (outer), i.e. centred on Y = 144 like the wooden wall, from a
buried plinth at Z = -10 up to the coping at Z = 260.

Corners: a 28 cm wall centred 6 cm inside the cell edge leaves an 8 x 8 cm notch at
every outer corner, and any symmetric toothing either gaps in straight runs or z-fights
at corners. So each wall carries a dressed long-and-short quoin pier on its +X end only
(X 128..160, Y 128..160, 2 cm proud of both faces). With the four wall yaws 0/90/180/270
about the cell centre, every corner joins one wall's +X end to another's -X end: the pier
fills the corner square and the other wall's plain -X end tucks inside it; in a straight
run the neighbour's -X end (at X = +150) also tucks inside the pier. Nothing coincides.
"""
import numpy as np

from stone_building.masonry import CM, Masonry, cm, triangles, unwrap

INNER, OUTER = 130.0, 158.0
PIER = (128.0, 160.0)            # pier X and Y range (cm)
QUOIN_LONG = 100.0               # long quoins reach back to X = 100
BOTTOM, LINTEL_Z, COPING_Z, TOP = -10.0, 220.0, 248.0, 260.0
DOOR_HALF = 65.0                 # clear opening 130 cm wide, 220 cm tall
LINTEL_HALF = 96.0
MORTAR_RECESS = 3.0
MAT_STONE, MAT_MORTAR = 0, 1


def course_bounds(seed=7):
    """Shared course heights: 8 courses from the plinth to the lintel bed, then the lintel course."""
    rng = np.random.default_rng(seed)
    weights = rng.uniform(0.82, 1.18, 8)
    weights[0] *= 1.2
    heights = weights / weights.sum() * (LINTEL_Z - BOTTOM)
    return [BOTTOM] + list(BOTTOM + np.cumsum(heights)[:-1]) + [LINTEL_Z, COPING_Z]


def _segment(x0, x1, length, rng, joints_below=(), min_last=16.0):
    """Split x0..x1 into stone cells of ``length()`` cm, breaking joints with those below."""
    cells, x = [], x0
    while x1 - x > 1e-6:
        end = x + length()
        for joint in joints_below:
            if abs(end - joint) < 7.0:
                end = joint + (7.0 if end >= joint else -7.0)
        if x1 - end < min_last:
            end = x1
        cells.append((x, min(end, x1)))
        x = min(end, x1)
    if len(cells) > 1 and cells[-1][1] - cells[-1][0] < min_last * 0.85:
        cells[-2] = (cells[-2][0], cells[-1][1])
        cells.pop()
    return cells


def _stone(m, xa, xb, za, zb, y, normal, rng):
    joint = rng.uniform(1.5, 2.6)
    angle = rng.normal(0.0, np.radians(2.2))
    hu = (xb - xa) / 2 - joint / 2 - abs(angle) * (zb - za) * 0.5
    hv = (zb - za) / 2 - joint / 2 - rng.uniform(0.0, 1.0) - abs(angle) * (xb - xa) * 0.5
    hv = max(hv, 3.0)
    cz = za + joint / 2 + hv + rng.uniform(0.0, 0.4)
    up = np.array([np.sin(angle), 0.0, np.cos(angle)])
    m.pillow(cm((xa + xb) / 2, y, cz), normal, up, hu * CM, hv * CM, mat=MAT_STONE,
             bulge=rng.uniform(0.004, 0.02), max_out=0.012)


def rubble_face(m, x0, x1, z0, z1, face, rng, joints_below):
    """Random rubble brought to course z0..z1 on one face between x0 and x1 (cm): mostly
    course-high stones, with some cells built of two smaller stones stacked."""
    y, normal = (INNER, (0, -1, 0)) if face == "inner" else (OUTER, (0, 1, 0))
    height = z1 - z0
    cells = _segment(x0, x1, lambda: float(np.clip(height * rng.uniform(1.3, 2.7), 26.0, 88.0)), rng,
                     joints_below)
    for xa, xb in cells:
        if height >= 22.0 and xb - xa > 28.0 and rng.random() < 0.3:
            zm = z0 + height * rng.uniform(0.32, 0.68)
            for a, b in ((z0, zm), (zm, z1)):
                for sa, sb in _segment(xa, xb, lambda: float(np.clip((b - a) * rng.uniform(1.8, 3.6), 18.0, 70.0)),
                                       rng, min_last=14.0):
                    _stone(m, sa, sb, a, b, y, normal, rng)
        else:
            _stone(m, xa, xb, z0, z1, y, normal, rng)
    return [xb for _, xb in cells[:-1]]


def rubble_end(m, z0, z1, rng):
    """Stones on the plain -X end face, for a wall end left free-standing."""
    joint = rng.uniform(1.6, 2.6)
    hv = (z1 - z0) / 2 - joint / 2 - rng.uniform(0.0, 1.2)
    m.pillow(cm(-150.0, (INNER + OUTER) / 2, z0 + joint / 2 + hv), (-1, 0, 0), np.array([0, 0, 1.0]),
             (14.0 - joint / 2) * CM, hv * CM, mat=MAT_STONE, bulge=0.006)


def quoin(m, x0, x1, y0, y1, z0, z1, rng, bulge=0.006, skip=()):
    m.block(cm(x0, y0, z0 + 0.25), cm(x1, y1, z1 - 0.25), mat=MAT_STONE, radius=0.02,
            relief=0.003, step=0.2, bulge=bulge * rng.uniform(0.5, 1.0), skip=skip)


def build(name, doorway=False, seed=1):
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=0.84)
    bounds = course_bounds()
    courses = list(zip(bounds[:-1], bounds[1:]))
    joints = {"inner": [], "outer": []}
    for k, (z0, z1) in enumerate(courses):
        long = k % 2 == 0
        pier_x0 = QUOIN_LONG if long else PIER[0]
        hidden = (("z", -1), ("z", 1)) + ((("x", -1),) if not long else ())
        quoin(m, pier_x0, PIER[1], PIER[0], PIER[1], z0, z1, rng, skip=hidden)
        segments = [(-150.0, pier_x0 - 0.8)]
        if doorway and z1 <= LINTEL_Z + 1e-6:
            jamb = 44.0 if k % 2 else 26.0
            for side in (-1, 1):
                lo, hi = sorted((side * (DOOR_HALF + 0.5), side * (DOOR_HALF + jamb)))
                quoin(m, lo, hi, INNER - 1.0, OUTER + 1.0, z0, z1, rng, bulge=0.005,
                      skip=(("z", -1), ("z", 1)))
            segments = [(-150.0, -DOOR_HALF - jamb - 0.8), (DOOR_HALF + jamb + 0.8, pier_x0 - 0.8)]
        elif doorway:
            quoin(m, -LINTEL_HALF, LINTEL_HALF, INNER - 1.2, OUTER + 1.2, z0, z1, rng, bulge=0.006)
            segments = [(-150.0, -LINTEL_HALF - 0.8), (LINTEL_HALF + 0.8, pier_x0 - 0.8)]
        for face in ("inner", "outer"):
            below = []
            for x0, x1 in segments:
                below += rubble_face(m, x0, x1, z0, z1, face, rng,
                                     [j for j in joints[face] if x0 < j < x1])
            joints[face] = below
        rubble_end(m, z0, z1, rng)
    # Coping: a course of long capstones across the full width, ending in the pier cap.
    x = -150.0
    while x < PIER[0] - 1e-6:
        end = x + rng.uniform(34.0, 58.0)
        if PIER[0] - end < 24.0:
            end = PIER[0]
        m.block(cm(x + 0.3, INNER - 1.0, COPING_Z + 0.3), cm(end - 0.3, OUTER + 1.0, TOP),
                mat=MAT_STONE, radius=0.025, relief=0.004, step=0.2, bulge=0.005, skip=(("z", -1),))
        x = end
    m.block(cm(PIER[0] + 0.3, PIER[0] - 0.5, COPING_Z + 0.3), cm(PIER[1] + 0.5, PIER[1] + 0.5, TOP),
            mat=MAT_STONE, radius=0.025, relief=0.004, step=0.2, bulge=0.005, skip=(("z", -1),))
    # Lime-mortar core, recessed behind the stone faces (hidden bottom and top); the pier gets its
    # own, shallower one so its joints read as mortar rather than holes.
    m.box(cm(PIER[0] + 0.5, PIER[0] + 1.0, BOTTOM), cm(PIER[1] - 1.0, PIER[1] - 1.0, COPING_Z + 0.5), MAT_MORTAR,
          skip=(("z", -1), ("z", 1), ("x", -1)))
    y0, y1 = INNER + MORTAR_RECESS, OUTER - MORTAR_RECESS
    top = COPING_Z + 0.5
    if doorway:
        reveal = DOOR_HALF + 1.3
        m.box(cm(-150 + MORTAR_RECESS, y0, BOTTOM), cm(-reveal, y1, top), MAT_MORTAR, skip=(("z", -1), ("z", 1)))
        m.box(cm(reveal, y0, BOTTOM), cm(PIER[1] - 2.0, y1, top), MAT_MORTAR, skip=(("z", -1), ("z", 1)))
        m.box(cm(-reveal, y0, LINTEL_Z + 1.5), cm(reveal, y1, top), MAT_MORTAR,
              skip=(("x", -1), ("x", 1), ("z", 1)))
    else:
        m.box(cm(-150 + MORTAR_RECESS, y0, BOTTOM), cm(PIER[1] - 2.0, y1, top), MAT_MORTAR,
              skip=(("z", -1), ("z", 1)))
    return m


def stone_material(kit, name, seed, **overrides):
    """Weathered granite fieldstone shared by the kit (per-stone offsets and tints)."""
    params = dict(grain=0.0035, scale=0.45, patina=0.9, lichen=0.45, moss=0.12, iron=0.35, streaks=0.3,
                  soil=0.45, soil_height=0.18, enclaves=0.25, film=0.75, seed=seed,
                  offset_attr="stone_offset", tint_attr="stone_tint")
    params.update(overrides)
    return kit.mats.granite(name, **params)


def make(kit, name, doorway=False, seed=1):
    mats = [stone_material(kit, f"M_{name[3:]}Stone", seed),
            kit.mats.lime_mortar(f"M_{name[3:]}Mortar", seed=seed)]
    m = build(name, doorway=doorway, seed=seed)
    obj = m.build(name, mats)
    unwrap(obj, shrink={MAT_MORTAR: 0.4})
    return obj
