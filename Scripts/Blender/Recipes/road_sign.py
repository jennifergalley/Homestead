"""Oak road sign with a blank board (Menu's public-road signs: AHomesteadRoadSign, which paints the place
names on both faces with TextRender, so the board carries no lettering of its own).

Real-object research (written before modeling):
- Country road and estate direction boards of the 1850s were oak or elm: a square post about 5 in
  (12-13 cm), set 2 ft or more into the ground, with a painted board about 3.5-4 ft by 1 ft (1.1 m x 0.3 m),
  1.5 in (3.5-4 cm) thick. A board on a single post was tenoned onto the post's top and steadied by two
  shaped knee braces; a narrow capping strip along its top edge shed the rain off the end grain.
- The board face was painted (white letters on a dark ground, or black on white) inside a thin raised
  bead that framed the lettering; both faces were painted for traffic from either way.
- Weathering: silver-grey oak with dark checks, green algae low on the post, the paint chalky and
  flaking at the edges of the board and worn through on the capping.

Style and frame match fingerpost.py (the cove route's "To the Cove" post): same oak, same post section.
Pivot at the post's foot on the ground. The board faces along +X and -X (TextRender reads from +X at yaw 0
and from -X at yaw 180) and runs across Y; it is symmetric in Y, so the FBX Y mirror changes nothing.
The post is bedded 0.35 m below the pivot. No collision (the actor disables it; she walks up to it).

Dimensions match Menu's stand-in (HomesteadRoadSign.cpp RoadSignStyle): board 1.10 m wide and 0.30 m
high with its top at 1.90 m, so the text centre sits at 1.75 m. The painted field inside the bead is
0.98 m x 0.20 m; its face is FACE_X from the post's centreline, and the bead stands BEAD_PROUD proud of it.
"""
import math

NAME = "RoadSign"
DESCRIPTION = ("Silvered oak road sign: a blank painted board tenoned onto a square post with two knee braces "
               "(original). Pivot = post foot; the board faces +X and -X, 1.10 x 0.30 m, top at 1.90 m.")
COLLISION = "none"
TRIANGLE_BUDGET = 8000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, -65), "focus": (0.0, 0.3, 1.75)}

POST = 0.125
BURY = 0.35
BOARD_W = 1.10
BOARD_H = 0.30
BOARD_T = 0.04
BOARD_TOP = 1.90
BOARD_Z = BOARD_TOP - BOARD_H * 0.5        # 1.75: the text centre
FACE_X = BOARD_T * 0.5                     # 0.02: the painted face, either side of the centreline
BEAD = 0.012
BEAD_PROUD = 0.006
FIELD_W = BOARD_W - 2 * (BEAD + 0.048)     # 0.98
FIELD_H = BOARD_H - 2 * (BEAD + 0.038)     # 0.20
CAP_W = 0.07
CAP_T = 0.022
BRACE_REACH = 0.30
BRACE_DROP = 0.30
BRACE_R = 0.021

REPORT = {
    "pivot": "foot of the post on the ground; the board faces +X and -X",
    "board_cm": {"width": BOARD_W * 100, "height": BOARD_H * 100, "top": BOARD_TOP * 100,
                 "centre_z": BOARD_Z * 100, "face_x": FACE_X * 100, "bead_proud": BEAD_PROUD * 100},
    "painted_field_cm": {"width": round(FIELD_W * 100, 1), "height": round(FIELD_H * 100, 1)},
    "text_standoff_cm": round((FACE_X + BEAD_PROUD * 0.5) * 100, 2),
    "note": "Blank board: AHomesteadRoadSign paints the words with TextRender on both faces.",
}


def brace_points(side):
    """A knee brace from the post's side up under the board, bowed out a little (side +1: +Y, -1: -Y)."""
    top_y = side * (POST * 0.5 + BRACE_REACH)
    low_y = side * POST * 0.5
    low_z = BOARD_TOP - BOARD_H - BRACE_DROP
    top_z = BOARD_TOP - BOARD_H
    pts = []
    for i in range(13):
        t = i / 12
        # A quarter-ellipse bow: up the post, then out under the board.
        a = t * math.pi * 0.5
        y = low_y + (top_y - low_y) * (1.0 - math.cos(a))
        z = low_z + (top_z - low_z) * math.sin(a)
        pts.append((0.0, y, z - BRACE_R * 0.4 * math.sin(math.pi * t)))
    return pts


def build(kit):
    oak = kit.mats.wood("M_RoadSignOak", light=(0.30, 0.28, 0.25), dark=(0.10, 0.095, 0.085), grain=1.1,
                        roughness=0.8, weathering=0.8, grime=0.3, seed=83.0, relief=1.2)
    # The painted ground inside the bead: a dark, chalky green-black, as on period estate boards.
    ground = kit.material("M_RoadSignPaint", (0.045, 0.055, 0.045), roughness=0.88)
    post_top = BOARD_TOP - BOARD_H
    height = post_top + BURY
    parts = [kit.box("Post", (POST, POST, height), location=(0.0, 0.0, post_top - height * 0.5), material=oak,
                     bevel=0.01, bevel_segments=2)]
    parts.append(kit.box("Board", (BOARD_T, BOARD_W, BOARD_H), location=(0.0, 0.0, BOARD_Z), material=oak,
                         bevel=0.004, bevel_segments=2))
    # The painted field, a skin 0.5 mm proud of each face so it bakes to its own colour.
    for face in (1.0, -1.0):
        parts.append(kit.box(f"Field{'Front' if face > 0 else 'Back'}", (0.001, FIELD_W + 2 * BEAD, FIELD_H + 2 * BEAD),
                             location=(face * (FACE_X + 0.0005), 0.0, BOARD_Z), material=ground))
        # The raised bead framing the field: top, bottom and the two ends.
        x = face * (FACE_X + BEAD_PROUD * 0.5)
        for name, size, at in (
                ("Top", (BEAD_PROUD, FIELD_W + 2 * BEAD, BEAD), (x, 0.0, BOARD_Z + FIELD_H * 0.5 + BEAD * 0.5)),
                ("Bottom", (BEAD_PROUD, FIELD_W + 2 * BEAD, BEAD), (x, 0.0, BOARD_Z - FIELD_H * 0.5 - BEAD * 0.5)),
                ("Left", (BEAD_PROUD, BEAD, FIELD_H), (x, FIELD_W * 0.5 + BEAD * 0.5, BOARD_Z)),
                ("Right", (BEAD_PROUD, BEAD, FIELD_H), (x, -FIELD_W * 0.5 - BEAD * 0.5, BOARD_Z))):
            parts.append(kit.box(f"Bead{name}{'F' if face > 0 else 'B'}", size, location=at, material=oak,
                                 bevel=0.002, bevel_segments=1))
    # The capping strip along the top edge, overhanging both faces to shed the rain.
    parts.append(kit.box("Cap", (CAP_W, BOARD_W + 0.03, CAP_T), location=(0.0, 0.0, BOARD_TOP + CAP_T * 0.5),
                         material=oak, bevel=0.005, bevel_segments=2))
    # Knee braces either side, and the oak pegs through the post's tenon under the board.
    for side in (1.0, -1.0):
        parts.append(kit.tube(f"Brace{'L' if side > 0 else 'R'}", brace_points(side), radius=BRACE_R, sides=12,
                              material=oak))
    parts.append(kit.cylinder("Peg", 0.009, POST + 0.012, location=(0.0, 0.0, post_top - 0.06),
                              rotation=(0, 90, 0), material=oak, sides=12))
    return kit.join(parts, "SM_RoadSign", pivot=None, unwrap=True, reshade=True, smooth_angle=40)
