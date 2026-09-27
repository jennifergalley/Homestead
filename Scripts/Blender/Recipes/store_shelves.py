"""Wall shelving unit stocked for an 1851 Cornish general store.

Real-object research (written before modeling):
- Village shop wall shelves were open stained pine units: upright side boards, a back
  against the plaster/stone wall, a simple cornice and base plinth, with movable or
  fixed shelves about a foot deep. Joinery was plain; the visual richness came from
  goods rather than carving.
- Period stock used reusable vessels and anonymous packaging: salt-glazed stoneware
  jars with cork/wood bungs; japanned or tinplate tea/cocoa canisters without modern
  labels; dark green hand-blown bottles; brown paper packets tied with string; folded
  cloth bolts; bundles of hand-dipped candles. Shelves were not merchandised in a
  modern grid: goods cluster by use, heights vary, and older stock sits dusty at the
  back.
- This unit is 2.4 m wide, 0.36 m deep, 2.2 m tall, with four shelves plus cornice and
  base. Open/front side points -Y; the back is against the wall at +Y. All goods are
  modeled as part of the single mesh. Original geometry/procedural materials only.
"""
import math
import random
import sys
from pathlib import Path
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "StoreShelves"
DESCRIPTION = "Stained pine wall shelving unit stocked with period jars, tins, bottles, packets, cloth and candles; open front -Y."
COLLISION = "box"
TRIANGLE_BUDGET = 80000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.18, 1.25)}
REPORT = {"dimensions_m": {"width": 2.4, "depth": 0.36, "height": 2.2}, "front": "open shelving faces -Y; back touches wall at +Y"}


def tin_can(kit, name, x, y, z, radius, height, mat, lid_mat):
    parts=[C.cylinder_vessel(kit, name, radius, height, mat, sides=30, rings=6, loc=(x,y,z+height/2), taper=0.0, cap=True)]
    parts.append(kit.cylinder(name+"_Lid", radius*1.04, 0.012, location=(x,y,z+height+0.006), material=lid_mat, sides=30, bevel=0.002))
    parts.append(kit.cylinder(name+"_Foot", radius*1.02, 0.010, location=(x,y,z+0.005), material=lid_mat, sides=30, bevel=0.001))
    return parts


def candle_bundle(kit, name, x, y, z, count, wax, twine):
    parts=[]
    for i in range(count):
        dy=(i-(count-1)/2)*0.016
        dz=0.004*math.sin(i)
        parts.append(kit.tube(f"{name}_Candle_{i}", [Vector((x-0.105,y+dy,z+dz)), Vector((x+0.105,y+dy,z+dz+0.002))], radius=0.0095, sides=12, material=wax))
        parts.append(kit.tube(f"{name}_Wick_{i}", [Vector((x+0.108,y+dy,z+dz+0.002)), Vector((x+0.116,y+dy,z+dz+0.003))], radius=0.0012, sides=5, material=twine))
    for sx in (-0.045,0.052):
        parts.append(kit.tube(f"{name}_Tie_{sx}", [Vector((x+sx,y-0.048,z+0.011)), Vector((x+sx,y+0.048,z+0.011))], radius=0.0018, sides=6, material=twine))
    return parts


def build(kit):
    m=kit.mats
    pine=m.wood("M_StoreShelvesStainedPine", light=(0.215,0.145,0.078), dark=(0.120,0.075,0.038), grain=1.55, roughness=0.75, weathering=0.06, grime=0.30, seed=30.0, relief=0.45)
    darkpine=m.wood("M_StoreShelvesDarkBack", light=(0.15,0.085,0.042), dark=(0.040,0.024,0.014), grain=1.5, roughness=0.86, weathering=0.10, grime=0.62, seed=31.0, relief=0.65)
    stone=m.stoneware("M_StoreShelvesStoneware", glaze=(0.34,0.31,0.25), clay=(0.20,0.105,0.055), seed=32.0)
    stone_b=m.stoneware("M_StoreShelvesBrownStoneware", glaze=(0.30,0.20,0.12), clay=(0.18,0.08,0.035), seed=33.0)
    cork=m.aged_wood("M_StoreShelvesBungs", light=(0.36,0.25,0.14), dark=(0.13,0.08,0.04), roughness=0.9, saw=0.18, grime=0.18, seed=34.0)
    tin=m.tinplate("M_StoreShelvesTinCanisters", base=(0.48,0.47,0.43), rust=0.18, seed=35.0)
    glass=m.green_glass("M_StoreShelvesGreenGlass", tint=(0.06,0.12,0.075), grime=0.25, seed=36.0)
    paper=m.paper("M_StoreShelvesPaperPackets", color=(0.50,0.42,0.29), string_shadow=0.24, seed=37.0)
    twine=m.rawhide("M_StoreShelvesTwine", color=(0.22,0.17,0.10), strands=2, twist=110.0)
    wax=m.candle_wax("M_StoreShelvesCandleWax", color=(0.68,0.59,0.42), soot=0.08, seed=38.0)
    cloth_blue=m.hessian("M_StoreShelvesBlueCloth", base=(0.11,0.15,0.19), dark=(0.035,0.045,0.055), dust=0.04, seed=39.0)
    cloth_red=m.hessian("M_StoreShelvesBrownRedCloth", base=(0.25,0.09,0.055), dark=(0.080,0.027,0.018), dust=0.05, seed=40.0)
    C.zero_subsurface(pine,darkpine,stone,stone_b,cork,tin,glass,paper,twine,wax,cloth_blue,cloth_red)
    parts=[]
    # carcass: pivot will be centered at bottom; front at -Y, wall back at +Y.
    parts.append(C.board(kit,"Shelves_Back",(2.30,0.035,2.04),(0,0.165,1.04),darkpine,bevel=0.004,rough=0.0015,seed=1))
    parts.append(C.board(kit,"Shelves_LeftSide",(0.055,0.36,2.12),(-1.1725,0.0,1.06),pine,bevel=0.008,rough=0.0014,seed=2))
    parts.append(C.board(kit,"Shelves_RightSide",(0.055,0.36,2.12),(1.1725,0.0,1.06),pine,bevel=0.008,rough=0.0014,seed=3))
    shelf_z=[0.28,0.72,1.16,1.60]
    for i,z in enumerate(shelf_z):
        parts.append(C.board(kit,f"Shelf_{i}",(2.30,0.36,0.040),(0,0.0,z),pine,bevel=0.008,rough=0.0012,seed=10+i))
        parts.append(C.board(kit,f"Shelf_FrontLip_{i}",(2.32,0.040,0.052),(0,-0.195,z+0.027),pine,bevel=0.006,rough=0.001,seed=15+i))
    parts.append(C.board(kit,"Shelves_BasePlinth",(2.38,0.39,0.095),(0,0.0,0.055),pine,bevel=0.012,rough=0.0016,seed=20))
    parts.append(C.board(kit,"Shelves_Cornice",(2.40,0.42,0.090),(0,0.0,2.155),pine,bevel=0.014,rough=0.001,seed=21))
    parts.append(C.board(kit,"Shelves_CorniceBead",(2.34,0.048,0.045),(0,-0.205,2.095),pine,bevel=0.010,rough=0.001,seed=22))
    # goods - bottom shelf: sacks/jars/canisters pushed irregularly front/back.
    base=0.28+0.030
    for i,(x,y,h,mat) in enumerate([(-0.98,-0.04,0.25,stone),(-0.78,0.05,0.22,stone_b),(-0.55,-0.10,0.18,stone)]):
        parts.extend(C.jar(kit,f"Bottom_Jar_{i}",x,y,base,h,mat,cork,seed=50+i))
    for i,(x,y,r,h) in enumerate([(-0.20,-0.03,0.055,0.18),(0.00,0.04,0.048,0.16),(0.18,-0.07,0.052,0.20)]):
        parts.extend(tin_can(kit,f"Bottom_Tin_{i}",x,y,base,r,h,tin,tin))
    parts.extend(C.packet(kit,"Bottom_Packet_A",(0.55,-0.05,base+0.035),(0.24,0.14,0.070),paper,twine,seed=60,rot=(0,0,-4)))
    parts.extend(C.packet(kit,"Bottom_Packet_B",(0.84,0.04,base+0.045),(0.22,0.13,0.085),paper,twine,seed=61,rot=(0,0,5)))
    parts.extend(candle_bundle(kit,"Bottom_Candles",1.06,-0.06,base+0.025,5,wax,twine))
    # second shelf: bottles and more jars.
    base=0.72+0.030
    for i,(x,y,h) in enumerate([(-1.02,-0.06,0.28),(-0.88,0.02,0.24),(-0.70,-0.10,0.30),(-0.50,0.04,0.22)]):
        parts.append(C.bottle(kit,f"Second_Bottle_{i}",x,y,base,h,glass,seed=70+i))
    for i,(x,y,h,mat) in enumerate([(-0.18,0.02,0.22,stone_b),(0.05,-0.08,0.26,stone),(0.32,0.04,0.19,stone)]):
        parts.extend(C.jar(kit,f"Second_Jar_{i}",x,y,base,h,mat,cork,seed=80+i))
    parts.extend(C.packet(kit,"Second_Packet_Stack1",(0.73,-0.08,base+0.035),(0.28,0.15,0.07),paper,twine,seed=85,rot=(0,0,2)))
    parts.extend(C.packet(kit,"Second_Packet_Stack2",(0.78,-0.04,base+0.105),(0.24,0.13,0.06),paper,twine,seed=86,rot=(0,0,-6)))
    # third shelf: cloth bolts and tins.
    base=1.16+0.030
    parts.extend(C.folded_cloth(kit,"Third_BlueCloth",(-0.80,-0.04,base+0.055),(0.36,0.22,0.075),cloth_blue,seed=90))
    parts.extend(C.folded_cloth(kit,"Third_RedCloth",(-0.35,0.01,base+0.050),(0.32,0.20,0.070),cloth_red,seed=91))
    for i,(x,y,r,h) in enumerate([(0.15,-0.06,0.050,0.17),(0.34,0.03,0.046,0.15),(0.52,-0.02,0.056,0.19),(0.74,0.06,0.045,0.16)]):
        parts.extend(tin_can(kit,f"Third_Tin_{i}",x,y,base,r,h,tin,tin))
    parts.extend(candle_bundle(kit,"Third_Candles",1.02,-0.06,base+0.025,4,wax,twine))
    # top shelf: quieter dusty reserve stock.
    base=1.60+0.030
    for i,(x,y,h,mat) in enumerate([(-0.98,0.04,0.20,stone),(-0.72,-0.07,0.16,stone_b),(-0.49,0.02,0.18,stone)]):
        parts.extend(C.jar(kit,f"Top_Jar_{i}",x,y,base,h,mat,cork,seed=100+i))
    for i,(x,y,h) in enumerate([(-0.12,-0.06,0.22),(0.04,0.02,0.20),(0.20,-0.10,0.24)]):
        parts.append(C.bottle(kit,f"Top_Bottle_{i}",x,y,base,h,glass,seed=110+i))
    parts.extend(C.packet(kit,"Top_Packets_A",(0.56,-0.03,base+0.035),(0.26,0.16,0.070),paper,twine,seed=120,rot=(0,0,3)))
    parts.extend(C.packet(kit,"Top_Packets_B",(0.83,0.05,base+0.045),(0.20,0.13,0.085),paper,twine,seed=121,rot=(0,0,-5)))
    return kit.join(parts,"SM_Store_Shelves",unwrap=False,reshade=True,smooth_angle=58)
