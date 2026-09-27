"""Mid-19th-century Cornish village shop counter for the general store interior.

Real-object research (written before modeling):
- Village shop counters of the 1840s-1850s were joiner-built pine carcasses with a
  harder polished mahogany/oak plank top. The customer side showed framed raised-and-
  fielded panels, a plinth/skirting board and an upper moulding; the shopkeeper side
  was plainer, often with drawers, cubbies or an open lower shelf. Counters were long
  working furniture, roughly 34-40 in high so goods could be weighed and wrapped while
  standing, and 24-30 in deep.
- Painted pine was commonly grained or oil-painted dark brown/green and wore through
  on arrises, around drawer pulls and at the plinth. Tops were hand-polished but
  scratched, darker from hand oil, and pale/abraded where sacks and parcels slid.
- This counter is 5.2 m long, 0.66 m body depth, 0.96 m body height, with a 0.78 m
  deep x 40 mm thick hardwood top at total height ~1.0 m. Customer-facing show front
  points toward Blender -Y; shopkeeper drawers and shelf face +Y. Original geometry
  and procedural materials only; no third-party sources.
"""
import math
import sys
from pathlib import Path
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "StoreCounter"
DESCRIPTION = "1851 Cornish village shop counter, raised-panel customer front -Y, plainer drawer/shelf back +Y."
COLLISION = "box"
TRIANGLE_BUDGET = 40000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.40, 0.68)}
REPORT = {"dimensions_m": {"length": 5.2, "body_depth": 0.66, "body_height": 0.96, "top_depth": 0.78, "top_thickness": 0.04},
          "front": "customer-facing raised-panel front points -Y"}


def build(kit):
    m = kit.mats
    paint = m.painted_wood("M_StoreCounterPaintedPine", paint=(0.045, 0.082, 0.055), under=(0.16, 0.095, 0.045), wear=0.58, grime=0.5, seed=10.0)
    top = m.wood("M_StoreCounterPolishedTop", light=(0.26, 0.135, 0.060), dark=(0.105, 0.047, 0.020), grain=0.75,
                 roughness=0.42, weathering=0.03, grime=0.22, seed=11.0, polish=0.9, polish_center=[-0.16, 0.0, 0.18], polish_length=0.18, relief=0.55)
    drawer = m.painted_wood("M_StoreCounterDrawerPine", paint=(0.070, 0.055, 0.038), under=(0.18, 0.10, 0.05), wear=0.48, grime=0.42, seed=12.0)
    inside = m.aged_wood("M_StoreCounterInteriorShelf", light=(0.26, 0.17, 0.095), dark=(0.10, 0.060, 0.030), roughness=0.83, saw=0.35, grime=0.38, seed=13.0)
    brass = m.brass("M_StoreCounterAgedBrass", polished=(0.50, 0.34, 0.15), tarnish=(0.08, 0.065, 0.035), wear=0.35)
    C.zero_subsurface(paint, top, drawer, inside, brass)

    parts = []
    # Pine carcass behind the joinery: kept dark in creases but still physical so side gaps do not show through.
    parts.append(C.board(kit, "Counter_Carcass", (5.20, 0.60, 0.90), (0, 0.00, 0.48), paint, bevel=0.010, rough=0.001, seed=1))
    parts.append(C.board(kit, "Counter_Top", (5.34, 0.78, 0.040), (0, -0.045, 0.98), top, bevel=0.018, rough=0.0018, seed=2))
    parts.append(C.board(kit, "Counter_WornFrontLip", (5.30, 0.035, 0.035), (0, -0.445, 0.955), top, bevel=0.010, rough=0.0015, seed=3))
    parts.append(C.board(kit, "Counter_BackLip", (5.22, 0.028, 0.025), (0, 0.345, 0.952), top, bevel=0.006, rough=0.001, seed=4))

    # Customer side (-Y): one row of tall raised-and-fielded panels, not drawers.
    front_y = -0.342
    parts.append(C.board(kit, "Front_Plinth", (5.24, 0.055, 0.105), (0, front_y-0.018, 0.065), paint, bevel=0.010, rough=0.0012, seed=5))
    parts.append(C.board(kit, "Front_TopMoulding", (5.24, 0.050, 0.070), (0, front_y-0.020, 0.905), paint, bevel=0.012, rough=0.0012, seed=6))
    bay_w = 5.20 / 6
    for i in range(7):
        x = -2.60 + i * bay_w
        parts.append(C.board(kit, f"Front_Stile_{i}", (0.068, 0.055, 0.760), (x, front_y-0.026, 0.475), paint, bevel=0.009, rough=0.0012, seed=20+i))
    parts.append(C.board(kit, "Front_BottomRail", (5.18, 0.043, 0.052), (0, front_y-0.024, 0.205), paint, bevel=0.008, rough=0.001, seed=31))
    parts.append(C.board(kit, "Front_TopRail", (5.18, 0.043, 0.052), (0, front_y-0.024, 0.765), paint, bevel=0.008, rough=0.001, seed=32))
    for i in range(6):
        x = -2.60 + bay_w*(i+0.5)
        w = bay_w - 0.15
        zc = 0.485
        parts.append(C.dark_gap(kit, f"Front_PanelShadow_{i}", (w+0.030, 0.008, 0.500), (x, front_y-0.045, zc)))
        parts.append(C.board(kit, f"Front_PanelRaised_{i}", (w, 0.042, 0.470), (x, front_y-0.062, zc), paint, bevel=0.020, rough=0.0018, seed=40+i))
        parts.append(C.board(kit, f"Front_PanelField_{i}", (w*0.72, 0.050, 0.310), (x, front_y-0.074, zc), paint, bevel=0.016, rough=0.0015, seed=60+i))
    parts.append(C.board(kit, "Front_PlinthWearStrip", (5.05, 0.012, 0.026), (0, front_y-0.060, 0.126), inside, bevel=0.004, rough=0.0015, seed=75))

    # Plain shopkeeper side (+Y): shelf, drawer bank, knobs and a few cubby dividers.
    back_y = 0.335
    parts.append(C.board(kit, "Back_WorkRail", (5.16, 0.042, 0.070), (0, back_y+0.018, 0.850), drawer, bevel=0.010, rough=0.001, seed=80))
    parts.append(C.board(kit, "Back_LowerShelf", (4.78, 0.040, 0.035), (0, back_y+0.020, 0.340), inside, bevel=0.006, rough=0.0014, seed=81))
    parts.append(C.board(kit, "Back_ShelfBackShadow", (4.90, 0.010, 0.040), (0, back_y+0.044, 0.360), inside, bevel=0.002, rough=0.0, seed=82))
    for i in range(5):
        x = -2.0 + i*1.0
        parts.append(C.board(kit, f"Back_Drawer_{i}", (0.76, 0.045, 0.145), (x, back_y+0.045, 0.705), drawer, bevel=0.008, rough=0.0012, seed=90+i))
        parts.append(C.dark_gap(kit, f"Back_DrawerGap_{i}", (0.80, 0.006, 0.160), (x, back_y+0.071, 0.705)))
        # two brass knobs per drawer, projected on +Y face
        for s in (-0.18, 0.18):
            knob = kit.cylinder(f"Back_DrawerKnob_{i}_{s}", 0.018, 0.020, location=(x+s, back_y+0.077, 0.705), rotation=(90,0,0), material=brass, sides=18, bevel=0.002)
            parts.append(knob)
    for i in range(6):
        x = -2.50 + i*1.0
        parts.append(C.board(kit, f"Back_ShelfDivider_{i}", (0.035, 0.035, 0.260), (x, back_y+0.018, 0.475), inside, bevel=0.004, rough=0.001, seed=110+i))
    # end panels with simpler fields
    for side, x in (("L", -2.62), ("R", 2.62)):
        parts.append(C.board(kit, f"End_{side}_Stile", (0.055, 0.60, 0.78), (x, 0.0, 0.48), paint, bevel=0.010, rough=0.0013, seed=120+(0 if side=='L' else 1)))
        parts.append(C.board(kit, f"End_{side}_Field", (0.060, 0.43, 0.48), (x, -0.01, 0.50), paint, bevel=0.014, rough=0.0017, seed=130+(0 if side=='L' else 1)))

    # A few soft central wear paths from parcels sliding over the polished top.
    wear_mat = kit.material("M_StoreCounterTopSoftWear", (0.19, 0.105, 0.055), roughness=0.58)
    for i, (x, y, length, angle) in enumerate(((-0.55, -0.04, 0.70, 2.0), (0.25, 0.02, 0.55, -3.0), (0.90, -0.10, 0.42, 1.0))):
        parts.append(C.board(kit, f"Top_SoftWearPath_{i}", (length, 0.010, 0.0007), (x, y, 1.001), wear_mat,
                             rotation=(0,0,angle), bevel=0.0004, rough=0.0, seed=150+i))

    return kit.join(parts, "SM_Store_Counter", unwrap=False, reshade=True, smooth_angle=58)
