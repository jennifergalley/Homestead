"""Dry-goods oak barrel for flour or oats, with loose lid slightly ajar.

Real-object research (written before modeling):
- Nineteenth-century dry-goods barrels were coopered from narrow oak staves, each
  crozed into circular heads and drawn tight by iron hoops. A flour/oat barrel about
  30-38 in high had a pronounced bilge, smaller heads, small daylight cracks between
  staves, four to six hoops, and a loose head/lid that could be lifted or left ajar.
- Staves are thicker at the bilge, knife/adze dressed inside, and show vertical grain,
  tannin stains and dark grime under the hoops. Iron hoops are blackened, pitted and
  slightly rusty at seams. Dry-goods barrels are dusty rather than wet.
- Dimensions here: 0.95 m tall, 0.62 m bilge diameter, 0.52 m head diameter, 32 oak
  staves, five iron hoops and a plank lid resting high and slightly tilted. The lid's
  lifted lip faces -Y for readability. Original geometry and procedural materials only.
"""
import math
import random
import sys
from pathlib import Path
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "StoreBarrel"
DESCRIPTION = "Dry-goods coopered oak barrel with iron hoops and a loose lid ajar; lid lift faces -Y."
COLLISION = "convex"
TRIANGLE_BUDGET = 15000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -22), "focus": (0.0, -0.18, 0.72)}
REPORT = {"dimensions_m": {"height": 0.95, "bilge_diameter": 0.62, "head_diameter": 0.52},
          "front": "barrel is round; the ajar lid opens toward -Y"}

H = 0.95
STAVES = 32

def rbar(t):
    # 0.26 m at heads, 0.31 m at the bilge with a very slight waist irregularity.
    return 0.260 + 0.050 * math.sin(math.pi * t) ** 0.92


def loose_lid(kit, mat):
    radius = 0.262
    thickness = 0.022
    z0 = H + 0.002
    tilt = math.radians(4.0)  # front (-Y) just lifted; back edge rests on the chime
    sides = 96
    rings = [0.0, 0.38, 0.72, 1.0]
    verts, faces, coords = [], [], []
    for layer, dz in enumerate((-thickness * 0.5, thickness * 0.5)):
        for rfrac in rings:
            for i in range(sides):
                a = 2 * math.pi * i / sides
                x = radius * rfrac * math.cos(a)
                y = radius * rfrac * math.sin(a)
                # Pivot the disc around the back (+Y) rim so it rests on the chime there.
                z = z0 + dz + (radius - y) * math.sin(tilt)
                verts.append((x, y, z))
                coords.append((y, dz, x))  # grain runs across the edge-joined boards
    def idx(layer, r, i): return layer * len(rings) * sides + r * sides + (i % sides)
    for layer in (0, 1):
        for r in range(len(rings)-1):
            for i in range(sides):
                face = (idx(layer,r,i), idx(layer,r,i+1), idx(layer,r+1,i+1), idx(layer,r+1,i))
                faces.append(face if layer == 1 else tuple(reversed(face)))
    outer = len(rings)-1
    for i in range(sides):
        faces.append((idx(0,outer,i), idx(0,outer,i+1), idx(1,outer,i+1), idx(1,outer,i)))
    lid = kit.mesh("LooseCircularHead", verts, faces, material=mat)
    kit.tag_coords(lid.data, coords)
    kit.recalc_normals(lid)
    parts = [lid]
    # Tight board joints: dark, flat incised lines on the top surface, not open gaps.
    seam_mat = kit.material("M_StoreBarrelLidSeamDark", (0.030, 0.020, 0.012), roughness=0.9)
    for j, x in enumerate((-0.105, 0.0, 0.105)):
        half = math.sqrt(max(radius * radius - x * x, 0.0)) * 0.92
        w = 0.003
        y0, y1 = -half, half
        verts = []
        for xx, yy in ((x-w/2,y0), (x+w/2,y0), (x+w/2,y1), (x-w/2,y1)):
            verts.append((xx, yy, z0 + thickness * 0.5 + (radius - yy) * math.sin(tilt) + 0.0007))
        parts.append(kit.mesh(f"Lid_TightJoint_{j}", verts, [(0,1,2,3)], material=seam_mat))
    # One cleat across the top, following the same tilt and sitting on the circular head.
    parts.append(C.board(kit, "Lid_SingleCleat", (0.43, 0.045, 0.020), (0.0, -0.010, z0 + 0.030),
                         mat, rotation=(4,0,0), bevel=0.004, rough=0.0008, seed=180))
    return parts


def build(kit):
    m = kit.mats
    oak = m.aged_wood("M_StoreBarrelOakStaves", light=(0.205, 0.125, 0.060), dark=(0.060, 0.033, 0.016), roughness=0.88, saw=0.26, grime=0.52, seed=20.0)
    head = m.aged_wood("M_StoreBarrelHeadBoards", light=(0.245, 0.155, 0.080), dark=(0.075, 0.043, 0.022), roughness=0.84, saw=0.42, grime=0.34, seed=21.0)
    iron = m.steel("M_StoreBarrelBlackIronHoops", bevel=0.002, scale_from=0.006, patina=0.75, rust=0.45, seed=22.0)
    dust = kit.material("M_StoreBarrelFlourDust", (0.46, 0.43, 0.35), roughness=0.98)
    C.zero_subsurface(oak, head, iron, dust)
    parts=[]
    rng=random.Random(4521)
    gap=math.radians(0.35)
    for i in range(STAVES):
        a0=2*math.pi*i/STAVES + gap*0.5
        a1=2*math.pi*(i+1)/STAVES - gap*0.5
        stave=C.bulged_vessel_shell(kit, f"Stave_{i:02d}", H, rbar, oak, sides=1, rings=18,
                                    thickness=0.022+rng.uniform(-0.002,0.002), start_angle=a0, end_angle=a1,
                                    seed=100+i)
        def relief(co, pco, i=i):
            t=pco.z/H
            long=noise.noise(Vector((pco.x*36+i, pco.y*36, pco.z*4))) if False else 0
            return 0.0008*math.sin((t*11+i)*math.pi)*0.2
        # Slight individual height/lean chips by moving top/bottom a hair.
        kit.roughen(stave, strength=0.0012, scale=22.0, seed=150+i, subdivide=0)
        parts.append(stave)
    # barrel heads visible in the dark openings/croze line
    for z, nm in ((0.018, "BottomHead"), (H-0.020, "TopHeadUnderLid")):
        parts.append(C.cylinder_vessel(kit, nm, 0.252, 0.024, head, sides=64, rings=2, loc=(0,0,z), taper=0.02, cap=True))
    # Five forged hoops following the bulge.
    for j, z in enumerate((0.105, 0.285, 0.485, 0.685, 0.850)):
        rr=rbar(z/H)+0.012
        parts.append(C.ring_band(kit, f"IronHoop_{j}", rr, z, 0.035 if j not in (0,4) else 0.040, 0.018, iron, sides=96, wobble=0.0015, seed=40+j))
    # Hoop rivets / overlap plates on the viewer side, enough to catch highlights.
    for j,z in enumerate((0.105,0.285,0.485,0.685,0.850)):
        for off in (-0.018,0.018):
            parts.append(kit.cylinder(f"HoopRivet_{j}_{off}", 0.007, 0.005, location=(off, -rbar(z/H)-0.023, z), rotation=(90,0,0), material=iron, sides=10, bevel=0.001))
    # Loose circular barrel head: tight boards with shallow joints, one cleat, tipped on the rear chime.
    parts.extend(loose_lid(kit, head))
    return kit.join(parts, "SM_Store_Barrel", unwrap=False, reshade=True, smooth_angle=54)

