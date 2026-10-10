"""Village noticeboard: a roofed oak board on two posts with pinned parchment notices and a little ledge.

Real-object research (written before modeling):
- English village noticeboards (parish and green-side boards) are a plank panel hung between two
  chunky oak posts set 40-60 cm into the ground, usually 1.4-1.8 m wide and about a metre tall, its
  top at 2 m so a standing adult can reach every notice. Vertical tongue-and-groove or butt-jointed
  boards are nailed to horizontal rails; a thin lipped frame keeps the ends of the boards dry.
- A small hipped or pitched hood (a cap of oak shingles or boards on a header beam let into the
  posts) throws rain off the paper. Barge boards close the gable ends and a ridge cap laps the top.
- Notices are pinned with a single nail or drawing pin at the top edge, and bigger sheets with four.
  Paper curls at the corners, overlaps older sheets, and stains cream to buff. A small shelf at hand
  height carries a stub of pencil, a bundle of slips or a jar.
- Oak left outside silvers, checks along the grain, and goes dark and mossy where it meets the soil;
  the foot of the post is packed with earth that mounds a little at the base.

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Units are metres, Z up. SM_VillageNoticeboard PIVOT: bottom centre on the ground line (z = 0), the
post feet sink 6 cm below it. The board FACES +X (front = +X); width runs along Y, 25 cm deep.
The notices carry only blank lines and ink dashes - no legible glyphs - so quests or villager
requests can be written onto it later.
"""
import importlib.util
import math
import os
import random
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_rocks as rocks  # noqa: E402

_spec = importlib.util.spec_from_file_location("farm_common", Path(__file__).with_name("farm") / "common.py")
common = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(common)

NAME = "VillageNoticeboard"
DESCRIPTION = ("Weathered oak village noticeboard: two chunky posts, nine-plank board with framing rails and a "
               "ledge, small oak-shingle hood, pinned blank parchment notices; front +X, pivot bottom centre (original).")
COLLISION = "box"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 64,
        "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 205), "focus": (0.0, 0.0, 1.20), "views": ["hero", "detail", "eye"], "eye_distance": 5.0}
NOTES = {"SM_VillageNoticeboard": ("Pivot = bottom centre on the ground line, post feet 6 cm below z = 0. Front faces +X; "
                                   "160 cm wide (Y) x 25 cm deep (X) x ~235 cm tall. Box collision.")}
REPORT = {"pivot": "bottom centre on the ground line", "facing": "+X", "height_m": 2.35, "width_m": 1.6,
          "collision": "box (posts and board)"}

POST_Y = 0.675
POST_W = 0.13
POST_FOOT = -0.06
ALPHA = math.radians(35.0)
COS_A, SIN_A = math.cos(ALPHA), math.sin(ALPHA)
HALF_RUN = 0.118
SLOPE_LEN = HALF_RUN / COS_A
RIDGE_Z = 2.225
EAVE_Z = RIDGE_Z - SLOPE_LEN * SIN_A
POST_TOP = EAVE_Z + 0.003
HEADER_DEPTH = 0.14
BOARD_TOP = POST_TOP - HEADER_DEPTH - 0.045
BOARD_BOTTOM = BOARD_TOP - 1.02
PLANK_COUNT = 9
BOARD_HALF = 0.74
PLANK_X = 0.080
PLANK_T = 0.028
PAPER_X = PLANK_X + PLANK_T / 2 + 0.001
RAIL_X = PLANK_X + PLANK_T / 2 + 0.014
SHELF_TOP = BOARD_BOTTOM - 0.02
ROOF_HALF_Y = 0.785
COURSES = 3

# (y, z, width, height, tilt degrees, layer, paper variant, pins, dash rows, curl)
NOTICES = [
    (-0.52, 1.62, 0.20, 0.27, -4.0, 0, 0, 1, 6, False),
    (-0.24, 1.70, 0.17, 0.22, 5.0, 0, 1, 2, 5, False),
    (-0.20, 1.38, 0.24, 0.18, -3.0, 0, 2, 1, 4, False),
    (0.10, 1.58, 0.27, 0.34, 2.0, 0, 0, 2, 8, True),
    (0.47, 1.72, 0.18, 0.15, -6.0, 0, 3, 1, 3, False),
    (0.52, 1.36, 0.20, 0.26, 4.0, 0, 1, 1, 6, False),
    (-0.56, 1.28, 0.15, 0.20, 7.0, 0, 2, 1, 4, False),
    (0.13, 1.37, 0.14, 0.12, -9.0, 1, 3, 1, 2, False),
    (-0.58, 1.07, 0.12, 0.10, -5.0, 0, 0, 1, 2, False),
]


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def roof_point(y, side, v, w=0.0):
    """Point on the roof plane: v down the slope from the ridge, w along the roof normal."""
    return Vector((side * (v * COS_A + w * SIN_A), y, RIDGE_Z - v * SIN_A + w * COS_A))


def local_box(kit, name, origin, axes, ranges, material, seed):
    """Closed box in a local (u, v, w) frame; pcoord = (u, w, v) so oak grain runs along v."""
    ux, vx, wx = (Vector(a) for a in axes)
    (u0, u1), (v0, v1), (w0, w1) = ranges
    corners = [(u, v, w) for u in (u0, u1) for v in (v0, v1) for w in (w0, w1)]
    verts = [Vector(origin) + ux * u + vx * v + wx * w for u, v, w in corners]
    quads = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    obj = kit.mesh(name, verts, quads, material=material)
    kit.tag_coords(obj.data, [(u + seed * 3.1, w, v + seed * 1.7) for u, v, w in corners])
    return kit.recalc_normals(obj)


def shingle(kit, side, y0, y1, v_butt, length, rng, material, tag):
    """One rounded oak shingle lying on the roof plane, its butt raised on the course beneath."""
    hw = 0.5 * (y1 - y0)
    cy = 0.5 * (y0 + y1)
    v_head = v_butt - length
    shoulder = min(0.022, 0.4 * length)
    outline = [(-hw, v_head), (hw, v_head), (hw + rng.uniform(-0.002, 0.002), v_butt - shoulder),
               (hw - 0.010, v_butt), (-hw + 0.010, v_butt), (-hw + rng.uniform(-0.002, 0.002), v_butt - shoulder)]
    lift = rng.uniform(0.010, 0.018)
    tilt_u = rng.uniform(-0.008, 0.008)

    def height(u, v):
        t = (v - v_head) / length
        bottom = lift * t + tilt_u * u
        return bottom, bottom + 0.005 + 0.010 * t + rng.uniform(-0.0006, 0.0006)

    u_dir = Vector((0.0, 1.0, 0.0))
    d_dir = Vector((side * COS_A, 0.0, -SIN_A))
    n_dir = Vector((side * SIN_A, 0.0, COS_A))
    origin = Vector((0.0, cy, RIDGE_Z))
    tops, bottoms, coords = [], [], []
    for u, v in outline:
        lo, hi = height(u, v)
        bottoms.append(origin + u_dir * u + d_dir * v + n_dir * lo)
        tops.append(origin + u_dir * u + d_dir * v + n_dir * hi)
        coords.append((u, hi, v))
    n = len(outline)
    verts = tops + bottoms
    centre = sum(tops, Vector()) / n
    verts.append(centre + n_dir * 0.0015)
    faces = [(i, (i + 1) % n, len(verts) - 1) for i in range(n)]
    faces += [(n + i, n + (i + 1) % n, (i + 1) % n, i) for i in range(n)]
    if u_dir.cross(d_dir).dot(n_dir) < 0:
        faces = [tuple(reversed(f)) for f in faces]
    obj = kit.mesh(f"Shingle{tag}", verts, faces, material=material)
    off = (rng.uniform(-9, 9), rng.uniform(-9, 9), rng.uniform(-9, 9))
    pco = [(u + off[0], w + off[1], v + off[2]) for u, w, v in coords]
    pco = pco + [(off[0], off[1], off[2])] * n + [(off[0], off[1], off[2])]
    kit.tag_coords(obj.data, pco)
    return obj


def moss_blob(kit, name, centre, radius, material, seed, rotation=(0, 0, 0), flat=0.28):
    blob = kit.sphere(name, radius, location=centre, rotation=rotation, material=material, segments=10, rings=6,
                      scale=(1.0, 1.0, flat))
    kit.roughen(blob, strength=radius * 0.35, scale=22.0, seed=seed)
    return blob


def notice_frame(spec, seed):
    """Mapping from a notice's local (a, b) to a point on the board, with a curled top-right corner."""
    y, z, w, h, tilt, layer, _, _, _, curl = spec
    ang = math.radians(tilt)
    ca, sa = math.cos(ang), math.sin(ang)
    base = PAPER_X + 0.0013 + layer * 0.0018

    def lift_of(a, b):
        s = 0.5 * (a / (w / 2) + b / (h / 2))
        d = max(0.0, s - 0.52) / 0.48 if curl else 0.0
        wave = 0.0010 * math.sin(a * 41.0 + seed) * math.cos(b * 33.0 + seed * 0.7)
        return base + wave + 0.052 * d ** 2.0, d

    def point(a, b):
        lift, d = lift_of(a, b)
        shrink = 1.0 - 0.15 * d ** 2.0
        a2, b2 = a * shrink, b * shrink
        return Vector((lift, y + a2 * ca - b2 * sa, z + a2 * sa + b2 * ca)), d

    return point


def notice(kit, index, spec, material, ink, rng):
    """Closed paper sheet (top, back and edge strip) on a warped grid, plus ink dash rows."""
    y, z, w, h, tilt, layer, _, _, rows, curl = spec
    nx, ny = (7, 8) if curl else (3, 4)
    point = notice_frame(spec, index * 3.7)
    verts, coords = [], []
    for j in range(ny + 1):
        for i in range(nx + 1):
            a, b = -w / 2 + w * i / nx, -h / 2 + h * j / ny
            p, _ = point(a, b)
            verts.append(p)
            coords.append((a + index * 5.3, b, 0.0))
    count = len(verts)
    thickness = 0.0011
    verts += [v + Vector((-thickness, 0.0, 0.0)) for v in verts]
    coords += coords
    faces = []
    for j in range(ny):
        for i in range(nx):
            q = j * (nx + 1) + i
            faces.append((q, q + 1, q + nx + 2, q + nx + 1))
            faces.append((count + q, count + q + nx + 1, count + q + nx + 2, count + q + 1))
    ring = ([j * (nx + 1) for j in range(ny + 1)][::-1] + list(range(1, nx + 1))
            + [j * (nx + 1) + nx for j in range(1, ny + 1)] + [ny * (nx + 1) + i for i in range(nx - 1, -1, -1)])
    ring = list(dict.fromkeys(ring))
    for k in range(len(ring)):
        a, b = ring[k], ring[(k + 1) % len(ring)]
        faces.append((a, count + a, count + b, b))
    sheet = kit.mesh(f"Notice{index}", verts, faces, material=material)
    kit.tag_coords(sheet.data, coords)
    kit.recalc_normals(sheet)
    parts = [sheet]

    # Ink: short dark dashes in ruled rows (a bolder heading row first), never forming letters.
    inks = []
    margin = 0.022
    row_pitch = (h - 2 * margin - 0.02) / max(rows, 1)
    for r in range(rows):
        b = h / 2 - margin - 0.012 - r * row_pitch
        heading = r == 0
        a = -w / 2 + margin
        end = w / 2 - margin - (0.0 if heading else rng.uniform(0.0, 0.07))
        thick = 0.0075 if heading else 0.0032
        if heading:
            a += w * 0.12
            end -= w * 0.12
        while a < end - 0.012:
            seg = min(rng.uniform(0.018, 0.05) if not heading else rng.uniform(0.03, 0.065), end - a)
            quad = []
            for aa, bb in ((a, b - thick / 2), (a + seg, b - thick / 2), (a + seg, b + thick / 2), (a, b + thick / 2)):
                p, d = point(aa, bb)
                quad.append(p + Vector((0.0007, 0.0, 0.0)) if d < 0.12 else None)
            if None not in quad:
                inks.append(quad)
            a += seg + rng.uniform(0.008, 0.02)
    if inks:
        iv, ifaces = [], []
        for quad in inks:
            base = len(iv)
            iv += quad
            ifaces.append((base, base + 1, base + 2, base + 3))
        mark = kit.mesh(f"Ink{index}", iv, ifaces, material=ink)
        kit.tag_coords(mark.data, [(v.y * 3.0, v.z * 3.0, 0.0) for v in iv])
        kit.recalc_normals(mark)
        parts.append(mark)
    return parts


def build(kit):
    rng = random.Random(7741)
    oak = common.weathered_oak(kit, "M_VillageNoticeboardOak", seed=19.0, lichen=0.28, grime=0.42)
    oak_post = common.weathered_oak(kit, "M_VillageNoticeboardPostOak", seed=29.0, lichen=0.40, grime=0.55)
    oak_board = common.weathered_oak(kit, "M_VillageNoticeboardBoardOak", seed=37.0, lichen=0.20, grime=0.50)
    shingle_oak = common.weathered_oak(kit, "M_VillageNoticeboardShingle", seed=43.0, lichen=0.50, grime=0.55)
    moss = common.moss_lichen(kit, "M_VillageNoticeboardMoss", seed=12.0)
    iron = common.rusted_iron(kit, "M_VillageNoticeboardIron", seed=7.0)
    loam = common.loam(kit, "M_VillageNoticeboardLoam", seed=3.0)
    papers = [kit.mats.paper("M_VillageNoticeboardPaperCream", color=(0.78, 0.72, 0.58), string_shadow=0.15, seed=1.0),
              kit.mats.paper("M_VillageNoticeboardPaperBuff", color=(0.72, 0.64, 0.48), string_shadow=0.20, seed=4.0),
              kit.mats.paper("M_VillageNoticeboardPaperWhite", color=(0.82, 0.79, 0.70), string_shadow=0.12, seed=8.0),
              kit.mats.paper("M_VillageNoticeboardPaperGrey", color=(0.74, 0.71, 0.62), string_shadow=0.18, seed=11.0)]
    ink = kit.mats.Graph("M_VillageNoticeboardInk")
    ink.set("Base Color", (0.035, 0.026, 0.020, 1.0))
    ink.set("Roughness", 0.55)
    ink = ink.mat
    common.zero_subsurface(oak, oak_post, oak_board, shingle_oak, moss, loam, *papers)

    parts = []
    # Posts: chunky oak, feet sunk below the ground line and packed round with a little earth and moss.
    for side, label in ((-1, "W"), (1, "E")):
        y = side * POST_Y
        parts.append(common.beam(kit, f"Post{label}", [(0.0, y, POST_FOOT), (0.0, y, POST_TOP)], POST_W, POST_W,
                                 oak_post, 71 + side, spacing=0.2))
        parts.append(moss_blob(kit, f"FootSoil{label}", (0.0, y, 0.004), 0.095, loam, 150 + side, flat=0.2))
        parts.append(moss_blob(kit, f"FootMoss{label}", (-0.05, y + side * 0.02, 0.022), 0.065, moss, 155 + side))
        parts.append(moss_blob(kit, f"FootMossFront{label}", (0.07, y - side * 0.03, 0.016), 0.04, moss, 158 + side))

    # Header beam let into the posts, carrying the hood.
    header_z = POST_TOP - HEADER_DEPTH / 2
    parts.append(common.beam(kit, "Header", [(0.0, -0.775, header_z), (0.0, 0.775, header_z)], 0.15, HEADER_DEPTH,
                             oak, 83, spacing=0.3))

    # Board: nine vertical planks, nailed to a top and a bottom rail, each a hair different in length.
    pitch = 2 * BOARD_HALF / PLANK_COUNT
    for index in range(PLANK_COUNT):
        yc = -BOARD_HALF + pitch * (index + 0.5)
        top = BOARD_TOP + rng.uniform(-0.012, 0.004)
        bottom = BOARD_BOTTOM + rng.uniform(-0.004, 0.014)
        parts.append(common.beam(kit, f"Plank{index}", [(PLANK_X, yc, bottom), (PLANK_X, yc, top)], PLANK_T,
                                 pitch - 0.006, oak_board, 200 + index, spacing=0.26))
    rail_h = 0.055
    for label, zc in (("Top", BOARD_TOP - 0.052), ("Bottom", BOARD_BOTTOM + 0.060)):
        parts.append(common.beam(kit, f"Rail{label}", [(RAIL_X, -BOARD_HALF + 0.004, zc), (RAIL_X, BOARD_HALF - 0.004, zc)],
                                 0.028, rail_h, oak, 230 + (1 if label == "Top" else 0), spacing=0.3))
        for k in range(8):
            yn = -BOARD_HALF + 0.07 + k * (2 * BOARD_HALF - 0.14) / 7 + rng.uniform(-0.01, 0.01)
            parts.append(kit.cylinder(f"RailNail{label}{k}", 0.0055, 0.004,
                                      location=(RAIL_X + 0.014 + 0.0005, yn, zc + rng.uniform(-0.006, 0.006)),
                                      rotation=(0, 90, 0), material=iron, sides=6))

    # Ledge: a shelf with a front lip, housed into the posts.
    parts.append(common.beam(kit, "Shelf", [(0.0945, -0.70, SHELF_TOP - 0.012), (0.0945, 0.70, SHELF_TOP - 0.012)],
                             0.062, 0.024, oak, 240, spacing=0.35))
    parts.append(common.beam(kit, "ShelfLip", [(0.118, -0.70, SHELF_TOP + 0.010), (0.118, 0.70, SHELF_TOP + 0.010)],
                             0.014, 0.040, oak, 241, spacing=0.35))
    # The shelf ends are housed in the posts; a thin apron under its front edge stiffens it.
    parts.append(common.beam(kit, "ShelfApron", [(0.104, -0.66, SHELF_TOP - 0.05), (0.104, 0.66, SHELF_TOP - 0.05)],
                             0.020, 0.038, oak, 245, spacing=0.3))

    # Hood: deck boards, gable infill, oak shingles in lapped courses, ridge cap and barge boards.
    exposure = (SLOPE_LEN + 0.025) / COURSES
    axes_for = {s: ((0.0, 1.0, 0.0), (s * COS_A, 0.0, -SIN_A), (s * SIN_A, 0.0, COS_A)) for s in (-1, 1)}
    for side in (-1, 1):
        boards = 6
        span = 2 * ROOF_HALF_Y / boards
        for b in range(boards):
            u0 = -ROOF_HALF_Y + b * span + 0.003
            parts.append(local_box(kit, f"Deck{side}{b}", (0.0, 0.0, RIDGE_Z), axes_for[side],
                                   ((u0, u0 + span - 0.006), (0.0, SLOPE_LEN + 0.012), (-0.022, -0.004)),
                                   oak, b + 4 * (side + 2)))
        for c in range(COURSES):
            v_butt = SLOPE_LEN + 0.022 - c * exposure + rng.uniform(-0.004, 0.004)
            length = min(0.095 + rng.uniform(-0.006, 0.006), v_butt + 0.02)
            y = -ROOF_HALF_Y + 0.005 - rng.uniform(0.0, 0.06)
            number = 0
            while y < ROOF_HALF_Y - 0.03:
                w = rng.uniform(0.075, 0.098)
                y0, y1 = max(y, -ROOF_HALF_Y + 0.01), min(y + w, ROOF_HALF_Y - 0.01)
                if y1 - y0 > 0.035:
                    parts.append(shingle(kit, side, y0, y1, v_butt, length, rng, shingle_oak, f"{side}_{c}_{number}"))
                y += w + 0.005
                number += 1
        cap_centre = roof_point(0.0, side, 0.040, 0.040 if side > 0 else 0.047)
        parts.append(common.beam(kit, f"RidgeCap{side}", [(cap_centre.x, -0.80, cap_centre.z), (cap_centre.x, 0.80, cap_centre.z)],
                                 0.115, 0.020, oak, 100 + side, spacing=0.4, roll=side * ALPHA))
        for end in (-1, 1):
            a, b = roof_point(end * 0.795, side, 0.0, -0.030), roof_point(end * 0.795, side, SLOPE_LEN + 0.012, -0.030)
            parts.append(common.beam(kit, f"Barge{side}{end}", [tuple(a), tuple(b)], 0.024, 0.062, oak_post,
                                     105 + end + side, spacing=0.4, roll=0.0))
    for end in (-1, 1):
        gable = [(-HALF_RUN + 0.004, EAVE_Z + 0.001), (HALF_RUN - 0.004, EAVE_Z + 0.001), (0.0, RIDGE_Z - 0.026)]
        y = end * 0.775
        verts = [(x, y - 0.010, z) for x, z in gable] + [(x, y + 0.010, z) for x, z in gable]
        faces = [(0, 1, 2), (3, 5, 4), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)]
        parts.append(kit.recalc_normals(kit.mesh(f"Gable{end}", verts, faces, material=oak_post)))

    # Moss: shaded back slope, header ends, ledge ends.
    for index, (y, v) in enumerate(((-0.52, 0.10), (0.34, 0.09), (0.62, 0.05), (-0.12, 0.12))):
        centre = roof_point(y, -1, v, 0.022)
        parts.append(moss_blob(kit, f"RoofMoss{index}", tuple(centre), rng.uniform(0.030, 0.050), moss, 120 + index,
                               rotation=(0, -math.degrees(ALPHA), 0), flat=0.30))
    for index, (y, x) in enumerate(((-0.66, 0.05), (0.69, 0.07), (-0.2, 0.062))):
        parts.append(moss_blob(kit, f"ShelfMoss{index}", (x, y, SHELF_TOP + 0.004), rng.uniform(0.022, 0.034), moss, 170 + index))

    # Notices: blank parchment slips with ink dashes, each pinned by a nail; one with a curling corner.
    for index, spec in enumerate(NOTICES):
        y, z, w, h, tilt, layer, variant, pins, _, curl = spec
        parts += notice(kit, index, spec, papers[variant], ink, rng)
        point = notice_frame(spec, index * 3.7)
        spots = [(0.0, h / 2 - 0.025)] if pins == 1 else [(-w / 2 + 0.03, h / 2 - 0.025), (w / 2 - 0.03, h / 2 - 0.03)]
        for k, (a, b) in enumerate(spots):
            if curl and a > 0:
                a, b = a * 0.4, b - 0.04
            p, _ = point(a, b)
            parts.append(kit.cylinder(f"Pin{index}_{k}", 0.0042, 0.0035, location=(p.x + 0.0018, p.y, p.z),
                                      rotation=(0, 90, 0), material=iron, sides=6))

    obj = kit.join(parts, "SM_VillageNoticeboard", pivot=None, unwrap=True, reshade=True, smooth_angle=48)
    meshes = [obj]
    low = rocks.lod(kit, obj, "SM_VillageNoticeboard_LOD1", 0.30)
    meshes.append(kit.finalize(low, pivot=None, unwrap=False, reshade=True, smooth_angle=48))
    return meshes
