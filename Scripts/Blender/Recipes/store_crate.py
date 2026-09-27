"""Nailed deal-board shipping crate for the general store.

Real-object research (written before modeling):
- Mid-19th-century shipping crates were simple deal (cheap pine/fir) boards nailed to
  corner battens or cleats. Boards were rough-sawn, not plywood: widths varied, gaps
  opened between planks, end grain showed at corners, nail heads sat proud or rusted,
  and edges splintered from hand trucks and damp floors.
- Crates were often close to cubic but made from boards of available widths; no modern
  printed markings are used here. Dimensions: about 0.60 m cube, plank thickness 22-28
  mm, corner battens 45-55 mm. The most readable slatted face points -Y. Original
  geometry and procedural materials only.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "StoreCrate"
DESCRIPTION = "0.6 m cube nailed deal-board shipping crate; slatted front points -Y."
COLLISION = "box"
TRIANGLE_BUDGET = 8000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -18), "focus": (0.0, -0.31, 0.35)}
REPORT = {"dimensions_m": {"width": 0.60, "depth": 0.60, "height": 0.60}, "front": "slatted crate face points -Y"}


def nail(kit, name, loc, mat, axis="Y"):
    rot = (90,0,0) if axis == "Y" else (0,90,0) if axis == "X" else (0,0,0)
    return kit.cylinder(name, 0.008, 0.006, location=loc, rotation=rot, material=mat, sides=6, bevel=0.0)


def build(kit):
    m=kit.mats
    deal=m.aged_wood("M_StoreCrateRoughDeal", light=(0.38,0.27,0.15), dark=(0.13,0.080,0.038), roughness=0.88, saw=0.75, grime=0.32, seed=50.0)
    endgrain=m.aged_wood("M_StoreCrateDarkerEndgrain", light=(0.26,0.17,0.09), dark=(0.075,0.044,0.023), roughness=0.92, saw=0.85, grime=0.45, seed=51.0)
    iron=m.steel("M_StoreCrateRustyNails", bevel=0.0015, scale_from=0.004, patina=0.70, rust=0.65, seed=52.0)
    C.zero_subsurface(deal,endgrain,iron)
    parts=[]
    # front/back planks
    zcenters=[0.085,0.225,0.370,0.520]
    heights=[0.13,0.125,0.135,0.115]
    for face,y,axis in (("Front",-0.289,"Y"),("Back",0.289,"Y")):
        for i,(z,h) in enumerate(zip(zcenters,heights)):
            parts.append(C.board(kit,f"{face}_Board_{i}",(0.60,0.026,h),(0,y,z),deal,bevel=0.004,rough=0.0024,seed=10+i+(0 if face=='Front' else 20)))
            if face == 'Front':
                for x in (-0.255,0.255):
                    parts.append(nail(kit,f"{face}_Nail_{i}_{x}",(x,y-0.004 if y<0 else y+0.004,z),iron,axis))
    # side planks
    for face,x,axis in (("Left",-0.289,"X"),("Right",0.289,"X")):
        for i,(z,h) in enumerate(zip(zcenters,heights)):
            parts.append(C.board(kit,f"{face}_Board_{i}",(0.026,0.60,h),(x,0,z),deal,bevel=0.004,rough=0.0022,seed=40+i+(0 if face=='Left' else 20)))
            if face == 'Right':
                for y in (-0.255,0.255):
                    parts.append(nail(kit,f"{face}_Nail_{i}_{y}",(x-0.004 if x<0 else x+0.004,y,z),iron,axis))
    # top and bottom boards with slight gaps.
    for face,z in (("Top",0.589),("Bottom",0.011)):
        for i,x in enumerate((-0.225,-0.075,0.080,0.230)):
            w=[0.13,0.14,0.135,0.12][i]
            parts.append(C.board(kit,f"{face}_Board_{i}",(w,0.60,0.026),(x,0,z),deal,bevel=0.004,rough=0.0023,seed=70+i+(0 if face=='Top' else 10)))
    # corner battens and mid cleats.
    for ix,x in enumerate((-0.280,0.280)):
        for iy,y in enumerate((-0.280,0.280)):
            parts.append(C.board(kit,f"CornerBatten_{ix}_{iy}",(0.040,0.040,0.58),(x,y,0.30),endgrain,bevel=0.005,rough=0.0012,seed=100+ix*2+iy))
    # dark gaps read between boards.
    for face,y in (("Front",-0.302),("Back",0.302)):
        for z in (0.155,0.296,0.445):
            parts.append(C.dark_gap(kit,f"{face}_Gap_{z}",(0.57,0.006,0.010),(0,y,z)))
    for face,x in (("Left",-0.302),("Right",0.302)):
        for z in (0.155,0.296,0.445):
            parts.append(C.dark_gap(kit,f"{face}_Gap_{z}",(0.006,0.57,0.010),(x,0,z)))
    return kit.join(parts,"SM_Store_Crate",unwrap=False,reshade=True,smooth_angle=50)
