"""Victorian pine travelling trunk for her storage chest: iron-bound, painted and worn, with oak battens.

Real-object research (written before modeling):
- The ordinary household or servant's box of 1840-1870 was a flat-topped deal (pine) trunk of
  dovetailed or nailed boards about 20 mm thick, painted (brown, ox-blood or a dark green, often
  grained) and strengthened with hardwood battens nailed across the lid and down the front and back.
- Iron fittings: bent corner caps on all eight corners, angle strips up the four vertical edges,
  two strap hinges at the back running onto the lid, a hasp hanging from the lid onto a lock plate
  with a keyhole in the middle of the front, and handles at the ends (iron drop handles, or looped
  leather straps through iron plates on cheaper boxes). All held on with round-headed nails.
- The lid is a shallow box that closes over the body; a moulded plinth runs round the foot.
- Wear after years in a damp cottage: paint rubbed through to the pine on the edges, round the hasp
  and on the lid where it is sat on; rust blooms on the iron; dirt low down.
Coral Island's chests are mood reference only (warmth, readability); nothing here is copied.

Size: it replaces the game's current storage chest, so it keeps that chest's box exactly: 0.70 m wide
(X) x 0.55 m deep (Y) x 0.58 m tall, with the hasp and lock on the front (-Y here; +Y in the engine,
where AHomesteadWorld draws the old chest's latch). The end handles fold flat inside the 0.70 m.
Pivot bottom centre. Original procedural geometry and materials only.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "VictorianTrunk"
DESCRIPTION = ("Victorian painted-pine travelling trunk, iron-bound with oak battens, hasp and lock plate "
               "(original). 0.70 x 0.55 x 0.58 m, the storage chest's footprint; front (-Y) = hasp.")
COLLISION = "box"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -25), "focus": (0.0, -0.28, 0.40)}
REPORT = {"dimensions_m": {"width": 0.70, "depth": 0.55, "height": 0.58},
          "front": "hasp and lock plate face -Y (+Y in the engine), as the old chest's latch",
          "replaces": "AHomesteadWorld Piece::Chest box (70 x 55 x 58 cm); same footprint and collision box"}

W, D, H = 0.70, 0.55, 0.58
PLINTH_H = 0.035
BODY_W, BODY_D = 0.676, 0.526          # the carcass inside the plinth and lid overhang
LID_H = 0.095
BODY_TOP = H - LID_H                    # where the lid's skirt closes over the body
BOARD_T = 0.02
TOP_SURF = H - 0.012                    # the lid's top boards; the oak battens stand on them to H
IRON_T = 0.003


def boards_between(z0, z1, count, seed):
    """Heights of ``count`` boards filling z0..z1, slightly uneven like hand-picked deal."""
    import random
    rng = random.Random(seed)
    weights = [1.0 + rng.uniform(-0.12, 0.12) for _ in range(count)]
    total = sum(weights)
    out, z = [], z0
    for w in weights:
        h = (z1 - z0) * w / total
        out.append((z + h * 0.5, h))
        z += h
    return out


def carcass(kit, paint, parts):
    """The body: boards on each face between the plinth and the lid, with dark gaps between them."""
    z0, z1 = PLINTH_H, BODY_TOP + 0.02
    for face, (size_fn, loc_fn) in {
        "Front": (lambda h: (BODY_W, BOARD_T, h), lambda z: (0, -BODY_D * 0.5 + BOARD_T * 0.5, z)),
        "Back": (lambda h: (BODY_W, BOARD_T, h), lambda z: (0, BODY_D * 0.5 - BOARD_T * 0.5, z)),
        "Left": (lambda h: (BOARD_T, BODY_D - 2 * BOARD_T, h), lambda z: (-BODY_W * 0.5 + BOARD_T * 0.5, 0, z)),
        "Right": (lambda h: (BOARD_T, BODY_D - 2 * BOARD_T, h), lambda z: (BODY_W * 0.5 - BOARD_T * 0.5, 0, z)),
    }.items():
        seed = sum(map(ord, face))
        rows = boards_between(z0, z1, 3, seed)
        for i, (z, h) in enumerate(rows):
            parts.append(C.board(kit, "%s_Board%d" % (face, i), size_fn(h - 0.002), loc_fn(z), paint,
                                 bevel=0.004, rough=0.0015, seed=seed + i))
        for z, h in rows[:-1]:
            zg = z + h * 0.5
            if face in ("Front", "Back"):
                y = -BODY_D * 0.5 - 0.0005 if face == "Front" else BODY_D * 0.5 + 0.0005
                parts.append(C.dark_gap(kit, "%s_Gap%.3f" % (face, zg), (BODY_W - 0.03, 0.003, 0.004), (0, y, zg)))
            else:
                x = -BODY_W * 0.5 - 0.0005 if face == "Left" else BODY_W * 0.5 + 0.0005
                parts.append(C.dark_gap(kit, "%s_Gap%.3f" % (face, zg), (0.003, BODY_D - 0.03, 0.004), (x, 0, zg)))
    parts.append(C.board(kit, "Floor", (BODY_W - 0.01, BODY_D - 0.01, BOARD_T), (0, 0, PLINTH_H + BOARD_T * 0.5), paint,
                         bevel=0.002, seed=91))


def plinth(kit, paint, parts):
    """The moulded foot: a stepped rail round all four sides."""
    for i, (inset, h) in enumerate(((0.0, PLINTH_H), (0.006, PLINTH_H + 0.012))):
        w, d = W - 0.004 - 2 * inset, D - 0.004 - 2 * inset
        z = h * 0.5
        t = 0.024
        parts.append(C.board(kit, "PlinthF%d" % i, (w, t, h), (0, -d * 0.5 + t * 0.5, z), paint, bevel=0.005, seed=60 + i))
        parts.append(C.board(kit, "PlinthB%d" % i, (w, t, h), (0, d * 0.5 - t * 0.5, z), paint, bevel=0.005, seed=62 + i))
        parts.append(C.board(kit, "PlinthL%d" % i, (t, d - 2 * t, h), (-w * 0.5 + t * 0.5, 0, z), paint, bevel=0.005, seed=64 + i))
        parts.append(C.board(kit, "PlinthR%d" % i, (t, d - 2 * t, h), (w * 0.5 - t * 0.5, 0, z), paint, bevel=0.005, seed=66 + i))


def lid(kit, paint, oak, parts):
    """A shallow lid box closing over the body: skirt boards, a top of three boards, oak battens."""
    skirt_h = LID_H - BOARD_T
    z = BODY_TOP + skirt_h * 0.5
    t = 0.022
    lw, ld = W - 0.006, D - 0.006
    parts.append(C.board(kit, "LidSkirtF", (lw, t, skirt_h), (0, -ld * 0.5 + t * 0.5, z), paint, bevel=0.005, seed=70))
    parts.append(C.board(kit, "LidSkirtB", (lw, t, skirt_h), (0, ld * 0.5 - t * 0.5, z), paint, bevel=0.005, seed=71))
    parts.append(C.board(kit, "LidSkirtL", (t, ld - 2 * t, skirt_h), (-lw * 0.5 + t * 0.5, 0, z), paint, bevel=0.005, seed=72))
    parts.append(C.board(kit, "LidSkirtR", (t, ld - 2 * t, skirt_h), (lw * 0.5 - t * 0.5, 0, z), paint, bevel=0.005, seed=73))
    # The top: three boards running side to side, their joints a hair open.
    top_z = TOP_SURF - BOARD_T * 0.5
    widths = (0.19, 0.17, 0.184)
    y = -ld * 0.5
    for i, bw in enumerate(widths):
        parts.append(C.board(kit, "LidTop%d" % i, (lw, bw - 0.002, BOARD_T), (0, y + bw * 0.5, top_z), paint,
                             bevel=0.004, rough=0.0018, seed=80 + i))
        if i:
            parts.append(C.dark_gap(kit, "LidGap%d" % i, (lw - 0.04, 0.003, 0.004), (0, y, top_z + BOARD_T * 0.5 + 0.0005)))
        y += bw
    # Two oak battens across the lid, front to back, standing 12 mm proud.
    for i, x in enumerate((-0.19, 0.19)):
        parts.append(C.board(kit, "LidBatten%d" % i, (0.05, ld + 0.004, 0.012), (x, 0, H - 0.006), oak,
                             bevel=0.004, rough=0.002, seed=85 + i))


def battens(kit, oak, parts):
    """Oak battens down the front and back (two each), over the paint; the ends carry the handles."""
    h = BODY_TOP - PLINTH_H - 0.014
    z = PLINTH_H + 0.007 + h * 0.5
    for i, x in enumerate((-0.19, 0.19)):
        for face, y in (("F", -BODY_D * 0.5 - 0.006), ("B", BODY_D * 0.5 + 0.006)):
            parts.append(C.board(kit, "Batten%s%d" % (face, i), (0.05, 0.012, h), (x, y, z), oak, bevel=0.004,
                                 rough=0.002, seed=100 + i + (0 if face == "F" else 5)))


def nail_row(kit, name, points, iron, axis, parts):
    rot = {"Y": (90, 0, 0), "X": (0, 90, 0), "Z": (0, 0, 0)}[axis]
    for i, p in enumerate(points):
        parts.append(kit.cylinder("%s_%02d" % (name, i), 0.0055, 0.004, location=p, rotation=rot, material=iron,
                                  sides=10, radius_top=0.0035, bevel=0.0008))


def corner_irons(kit, iron, parts):
    """Bent iron caps on the eight corners and angle strips up the four vertical edges."""
    leg = 0.075
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = sx * (BODY_W * 0.5 + 0.0015), sy * (BODY_D * 0.5 + 0.0015)
            # Edge strip: two leaves, one on each face.
            h = BODY_TOP - PLINTH_H - 0.01
            zc = PLINTH_H + 0.005 + h * 0.5
            parts.append(kit.box("EdgeX_%d%d" % (sx, sy), (0.035, IRON_T, h), (x - sx * 0.0175, y, zc),
                                 material=iron, bevel=0.0012))
            parts.append(kit.box("EdgeY_%d%d" % (sx, sy), (IRON_T, 0.035, h), (x, y - sy * 0.0175, zc),
                                 material=iron, bevel=0.0012))
            nail_row(kit, "EdgeNailsX_%d%d" % (sx, sy),
                     [(x - sx * 0.02, y + sy * 0.002, PLINTH_H + 0.04 + k * (h - 0.06) / 4) for k in range(5)], iron, "Y", parts)
            nail_row(kit, "EdgeNailsY_%d%d" % (sx, sy),
                     [(x + sx * 0.002, y - sy * 0.02, PLINTH_H + 0.06 + k * (h - 0.06) / 4) for k in range(4)], iron, "X", parts)
            # Lid corner caps: three leaves wrapping the lid's corner.
            lx, ly = sx * (W * 0.5 - 0.0015), sy * (D * 0.5 - 0.0015)
            cap_h = TOP_SURF - BODY_TOP - 0.002
            zl = BODY_TOP + 0.001 + cap_h * 0.5
            parts.append(kit.box("CapX_%d%d" % (sx, sy), (leg, IRON_T, cap_h), (lx - sx * leg * 0.5, ly, zl),
                                 material=iron, bevel=0.0012))
            parts.append(kit.box("CapY_%d%d" % (sx, sy), (IRON_T, leg, cap_h), (lx, ly - sy * leg * 0.5, zl),
                                 material=iron, bevel=0.0012))
            parts.append(kit.box("CapTop_%d%d" % (sx, sy), (leg, leg, IRON_T), (lx - sx * leg * 0.5, ly - sy * leg * 0.5, TOP_SURF + 0.0015),
                                 material=iron, bevel=0.0012))
            nail_row(kit, "CapNails_%d%d" % (sx, sy),
                     [(lx - sx * 0.02, ly - sy * 0.05, TOP_SURF + 0.004), (lx - sx * 0.05, ly - sy * 0.02, TOP_SURF + 0.004),
                      (lx - sx * 0.045, ly - sy * 0.045, TOP_SURF + 0.004)], iron, "Z", parts)
            # Foot caps over the plinth corners.
            parts.append(kit.box("FootX_%d%d" % (sx, sy), (0.06, IRON_T, PLINTH_H + 0.01),
                                 (sx * (W * 0.5 - 0.03), sy * (D * 0.5 - 0.0005), (PLINTH_H + 0.01) * 0.5), material=iron, bevel=0.001))
            parts.append(kit.box("FootY_%d%d" % (sx, sy), (IRON_T, 0.06, PLINTH_H + 0.01),
                                 (sx * (W * 0.5 - 0.0005), sy * (D * 0.5 - 0.03), (PLINTH_H + 0.01) * 0.5), material=iron, bevel=0.001))


def hasp_and_lock(kit, iron, parts):
    """A lock plate with a keyhole on the front, and the hasp hanging from the lid onto its staple."""
    # The hasp lies on the lid's front face and hangs down past its edge to the staple.
    y = -(D - 0.006) * 0.5 - 0.0005
    plate_z = BODY_TOP - 0.06
    parts.append(kit.box("LockPlate", (0.085, IRON_T, 0.095), (0, -BODY_D * 0.5 - 0.0035, plate_z), material=iron, bevel=0.002))
    keyhole = kit.material("M_TrunkKeyhole", (0.01, 0.008, 0.006), roughness=0.95)
    parts.append(kit.cylinder("KeyholeRound", 0.0045, 0.002, location=(0, -BODY_D * 0.5 - 0.0055, plate_z - 0.012),
                              rotation=(90, 0, 0), material=keyhole, sides=12))
    parts.append(kit.box("KeyholeSlot", (0.004, 0.002, 0.012), (0, -BODY_D * 0.5 - 0.0055, plate_z - 0.02), material=keyhole))
    nail_row(kit, "LockNails", [(sx * 0.033, -BODY_D * 0.5 - 0.0055, plate_z + sz * 0.038) for sx in (-1, 1) for sz in (-1, 1)],
             iron, "Y", parts)
    # The staple the hasp drops over, and the hasp itself: a strap from the lid's front with a slot.
    parts.append(kit.tube("Staple", [(-0.009, -BODY_D * 0.5 - 0.005, plate_z + 0.03), (-0.009, -BODY_D * 0.5 - 0.016, plate_z + 0.03),
                                     (0.009, -BODY_D * 0.5 - 0.016, plate_z + 0.03), (0.009, -BODY_D * 0.5 - 0.005, plate_z + 0.03)],
                          radius=0.0025, sides=8, material=iron))
    hasp_top = H - 0.02
    parts.append(kit.box("Hasp", (0.032, 0.004, hasp_top - (plate_z + 0.012)), (0, y - 0.0025, (hasp_top + plate_z + 0.012) * 0.5),
                         material=iron, bevel=0.0015))
    parts.append(kit.cylinder("HaspKnuckle", 0.006, 0.036, location=(0, y - 0.004, hasp_top), rotation=(0, 90, 0),
                              material=iron, sides=12))
    nail_row(kit, "HaspNails", [(0, y - 0.002, hasp_top + 0.02)], iron, "Y", parts)


def hinges(kit, iron, parts):
    """Two strap hinges: a long leaf on the lid, a short one down the back, round knuckles between."""
    for i, x in enumerate((-0.19 + 0.06, 0.19 - 0.06)):
        y = D * 0.5 - 0.0015
        parts.append(kit.box("HingeLid%d" % i, (0.03, 0.2, IRON_T), (x, y - 0.1, TOP_SURF + 0.0015), material=iron, bevel=0.0012))
        parts.append(kit.box("HingeBack%d" % i, (0.03, IRON_T, 0.1), (x, BODY_D * 0.5 + 0.0015, BODY_TOP - 0.045), material=iron, bevel=0.0012))
        parts.append(kit.cylinder("HingeKnuckle%d" % i, 0.006, 0.034, location=(x, y + 0.004, BODY_TOP + 0.004),
                                  rotation=(0, 90, 0), material=iron, sides=12))
        nail_row(kit, "HingeLidNails%d" % i, [(x, y - 0.04 - k * 0.05, TOP_SURF + 0.004) for k in range(3)], iron, "Z", parts)


def end_handles(kit, leather, iron, parts):
    """Looped leather handles through iron plates on each end, lying flat inside the box's width."""
    for s, face in ((-1, "L"), (1, "R")):
        x = s * (BODY_W * 0.5 + 0.001)
        z = BODY_TOP - 0.085
        for j, y in enumerate((-0.07, 0.07)):
            parts.append(kit.box("HandlePlate%s%d" % (face, j), (IRON_T, 0.045, 0.05), (x + s * 0.0015, y, z),
                                 material=iron, bevel=0.0015))
        # Hanging flat against the end: within the lid's overhang, so the 0.70 m box still holds.
        pts = [(x + s * 0.004, -0.07, z), (x + s * 0.005, -0.045, z - 0.014), (x + s * 0.005, 0.0, z - 0.022),
               (x + s * 0.005, 0.045, z - 0.014), (x + s * 0.004, 0.07, z)]
        parts.append(kit.tube("Handle%s" % face, pts, radius=0.005, sides=10, material=leather))


def build(kit):
    m = kit.mats
    paint = m.painted_wood("M_TrunkPaintedPine", paint=(0.075, 0.035, 0.022), under=(0.20, 0.13, 0.07), wear=0.5,
                           grime=0.4, seed=41.0)
    oak = m.wood("M_TrunkOak", light=(0.26, 0.18, 0.105), dark=(0.10, 0.065, 0.035), grain=1.0, roughness=0.7,
                 weathering=0.15, grime=0.3, seed=42.0)
    iron = m.wrought_iron("M_TrunkIron", rust=0.5, wear=0.4, seed=43.0)
    leather = m.harness_leather("M_TrunkHandleLeather", color=(0.14, 0.075, 0.035), dark=(0.05, 0.028, 0.015),
                                roughness=0.58, scuff=0.6, dubbin=0.3, grime=0.45, seed=44.0)
    C.zero_subsurface(paint, oak, iron, leather)
    parts = []
    plinth(kit, paint, parts)
    carcass(kit, paint, parts)
    lid(kit, paint, oak, parts)
    battens(kit, oak, parts)
    corner_irons(kit, iron, parts)
    hasp_and_lock(kit, iron, parts)
    hinges(kit, iron, parts)
    end_handles(kit, leather, iron, parts)
    return kit.join(parts, "SM_VictorianTrunk", unwrap=True, reshade=True, smooth_angle=40)
