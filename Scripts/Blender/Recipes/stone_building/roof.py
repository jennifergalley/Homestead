"""Stone roof layout (Unreal local cm on paper; see ``__init__``).

A near-flat roof of split stone slates on a timber frame, pivot at the cell centre on the
floor so the mesh sits at roof height:
- wall plates Z 258..268 (they bed 2 cm into the wall coping at Z 260),
- five joists along X, Z 268..279, and a board deck along Y, Z 279..281.5,
- slates in 15 courses laid up the Y axis at a 20 cm gauge (44 cm slates, 4 cm head lap),
  each tilted ~6 degrees with its tail on the deck and its butt on the course below;
  the eaves course sits on a tilting fillet along -Y. Top of the slates ~288.

Tiling at 300 cm with no coplanar faces between neighbours (roofs placed at yaw 0):
- Y: the courses are periodic across cells (300 = 15 x 20), so a roof's top (+Y) course
  runs on under the next roof's eaves course exactly as the courses inside one roof do.
- X: slates stop at X = +-150; a thin undercloak strip (X 150..160 and -160..-150) lies
  below slate level and slides under the neighbour's slates, or shows as the verge.
- Frame members are offset so neighbours' plates, joists and boards never overlap.
"""
import numpy as np

from stone_building.masonry import CM, Masonry, cm, unwrap

MAT_SLATE, MAT_TIMBER = 0, 1
PLATE = (258.0, 268.0)
JOIST = (268.0, 279.0)
DECK = (279.0, 281.5)
GAUGE, SLATE_LEN, SLATE_T = 20.0, 44.0, 2.0
EAVE_Y = -158.0
COURSES = 15
RISE = SLATE_T * SLATE_LEN / GAUGE


def build(seed=4):
    rng = np.random.default_rng(seed)
    m = Masonry(seed, tint_mean=1.0)

    def timber(lo, hi, axis, radius=0.012, relief=0.002):
        m.block(cm(*lo), cm(*hi), mat=MAT_TIMBER, radius=radius, relief=relief, step=0.5, pcoord_axis=axis,
                relief_scale=3.0)

    # Wall plates: +X/+Y ones at 141..154, -X/-Y ones at -144..-131 so neighbours never overlap.
    timber((141, -150, PLATE[0]), (154, 150, PLATE[1]), 1)
    timber((-144, -150, PLATE[0]), (-131, 150, PLATE[1]), 1)
    timber((-130.5, 141, PLATE[0]), (140.5, 154, PLATE[1]), 0)
    timber((-130.5, -144, PLATE[0]), (140.5, -131, PLATE[1]), 0)
    # Joists along X resting on the X plates.
    for y in (-110.0, -55.0, 0.0, 55.0, 110.0):
        y += rng.uniform(-3, 3)
        timber((-144, y - 5, JOIST[0]), (154, y + 5, JOIST[1]), 0)
    # Deck boards along Y.
    x = -144.0
    while x < 154.0 - 1e-6:
        w = rng.uniform(16.0, 26.0)
        end = min(x + w, 154.0)
        if 154.0 - end < 10.0:
            end = 154.0
        timber((x + 0.2, -144, DECK[0] + rng.uniform(-0.2, 0.0)), (end - 0.2, 154, DECK[1]), 1, radius=0.006,
               relief=0.001)
        x = end
    # Tilting fillet under the eaves course, and the undercloak strips at the verges.
    timber((-150, EAVE_Y, DECK[1] + 0.1), (150, -144.5, DECK[1] + RISE - 0.6), 0, radius=0.006)
    for x0, x1 in ((150.0, 160.0), (-160.0, -150.0)):
        m.block(cm(x0, EAVE_Y, DECK[0] + 1.0), cm(x1, EAVE_Y + COURSES * GAUGE + SLATE_LEN - GAUGE, DECK[1] - 0.3),
                mat=MAT_SLATE, radius=0.004, relief=0.0015, step=0.4)

    # Slates.
    joints_below = []
    for j in range(COURSES):
        butt = EAVE_Y + j * GAUGE
        joints, x = [], -150.0
        while x < 150.0 - 1e-6:
            end = x + rng.uniform(22.0, 40.0)
            for joint in joints_below:
                if abs(end - joint) < 7.0:
                    end = joint + (7.0 if end >= joint else -7.0)
            if 150.0 - end < 14.0:
                end = 150.0
            gap = rng.uniform(0.4, 0.9)
            width = end - x - gap
            thickness = SLATE_T * rng.uniform(0.8, 1.2)
            y0 = butt + rng.uniform(-0.8, 0.8)
            length = SLATE_LEN + rng.uniform(-1.5, 0.5)
            rise = RISE + rng.uniform(-0.3, 0.3)
            cx = (x + end) / 2 + rng.uniform(-0.3, 0.3)
            yaw = np.radians(rng.normal(0.0, 0.8))

            def frame(p, cx=cx, y0=y0, rise=rise, length=length, yaw=yaw):
                p = np.asarray(p, dtype=np.float64) * CM
                x_, y_ = p[:, 0] * np.cos(yaw) - p[:, 1] * np.sin(yaw), p[:, 0] * np.sin(yaw) + p[:, 1] * np.cos(yaw)
                z_ = DECK[1] * CM + rise * CM * (1.0 - p[:, 1] / (length * CM)) + p[:, 2]
                return np.stack([cx * CM + x_, y0 * CM + y_, z_], axis=1)

            m.plate(frame, width, length, thickness, mat=MAT_SLATE, sides=12, relief=0.25, pcoord_scale=CM,
                    extra={"slate_tail": lambda local, length=length: np.clip(1.0 - local[:, 1] / (GAUGE * 1.1), 0, 1)})
            joints.append(end)
            x = end
        joints_below = joints[:-1]
    return m


def make(kit, name, seed=4):
    mats = [kit.mats.slate(f"M_{name[3:]}Slate", dark=(0.05, 0.052, 0.052), light=(0.13, 0.128, 0.122),
                           lichen=0.32, moss=0.18, seed=seed, offset_attr="stone_offset"),
            kit.mats.wood(f"M_{name[3:]}Timber", light=(0.21, 0.17, 0.125), dark=(0.085, 0.064, 0.045),
                          weathering=0.55, grime=0.25, roughness=0.78, seed=seed)]
    m = build(seed)
    obj = m.build(name, mats)
    unwrap(obj)
    return obj
