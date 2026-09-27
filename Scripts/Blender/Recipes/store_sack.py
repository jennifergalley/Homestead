"""Slumped hessian/jute grain sack for the general store.

Real-object research (written before modeling):
- Flour, oats and grain were stored in coarse woven jute/hessian sacks. A filled sack is
  not a neat cylinder: the bottom spreads under weight, corners wrinkle, the shoulders
  sag, and the mouth is gathered into pleats and tied with twine. The weave is visible,
  with slubs, frayed seams and a dusty flour bloom around the base and neck. Store
  sacks of this size were anonymous utility packaging; no stencil text is used.
- Dimensions here: about 0.65 m tall, 0.50 m wide, 0.33 m deep, slumped forward-left,
  gathered neck tied with twine. The broad face with seam/creases points -Y.
  Original geometry and procedural materials only.
"""
import math
import random
import sys
from pathlib import Path
from mathutils import Vector, noise

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "StoreSack"
DESCRIPTION = "Slumped hessian grain sack, gathered neck tied with twine, flour dust at base; broad face -Y."
COLLISION = "convex"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
BEAUTY = {"pose": (0, 0, -12), "focus": (0.0, -0.14, 0.38)}
REPORT = {"dimensions_m": {"height": 0.65, "width": 0.50, "depth": 0.33}, "front": "broad slumped face and seam face -Y"}


def build_sack_body(kit, mat):
    sides=40
    ts=[0.0,0.025,0.06,0.12,0.20,0.32,0.45,0.58,0.70,0.80,0.88,0.94,0.985,1.0]
    rows=[]; coords=[]
    for j,t in enumerate(ts):
        z=0.65*t
        # Slump: broad base spreads, mid body bulges, neck pinches.
        body=math.sin(math.pi*min(t,0.92)/0.92)**0.24 if t<0.92 else 0.48*(1-t)+0.05
        neck=C.smoothstep(0.78,0.94,t)
        rx=0.080 + 0.170*body*(1-0.74*neck)
        ry=0.060 + 0.105*body*(1-0.70*neck)
        if t<0.10:
            rx*=1.04+0.14*(0.10-t)/0.10; ry*=1.08+0.10*(0.10-t)/0.10
        cx=-0.035*t + 0.018*math.sin(2.4*t)
        cy=-0.025*math.sin(math.pi*t) - 0.030*C.smoothstep(0.25,0.85,t)
        row=[]; c=[]
        for i in range(sides):
            a=2*math.pi*i/sides
            square=1+0.12*math.cos(4*a)*C.smoothstep(0.0,0.16,t)*(1-C.smoothstep(0.78,0.92,t))
            wrinkle=0.012*noise.noise(Vector((math.cos(a)*4.0+1.5, math.sin(a)*5.0, t*9.0)))
            pleat=1.0-0.20*C.smoothstep(0.78,0.96,t)*max(0, math.cos(10*a+0.7))
            x=cx+(rx*square*pleat+wrinkle)*math.cos(a)
            y=cy+(ry*square*pleat+0.65*wrinkle)*math.sin(a)
            # Flatten bottom into a soft foot.
            zz=z + (0.006*math.sin(3*a+0.3)+0.004*noise.noise(Vector((i,3,t))))*C.smoothstep(0.0,0.08,t)
            if t<0.025: zz=0.004*math.sin(a)**2
            row.append((x,y,zz)); c.append((x,y,z))
        rows.append(row); coords.append(c)
    obj=kit.loft("SackBody", rows, material=mat, coords=coords, cap_start=True, cap_end=True)
    def folds(co,pco):
        a=math.atan2(pco.y,pco.x)
        z=pco.z
        vertical=math.sin(a*13.0+1.2)*C.smoothstep(0.42,0.62,z)*C.smoothstep(0.65,0.20,z)
        weave=noise.noise(Vector((pco.x*75,pco.y*75,pco.z*8)))
        neck=0.0035*math.sin(a*10.0+0.5)*C.smoothstep(0.50,0.62,z)
        return 0.006*vertical + 0.0015*weave + neck
    kit.displace(obj, folds)
    return obj


def ellipse_disc(kit, name, rx, ry, z, mat, seed=0):
    verts=[(0,0,z)]
    faces=[]; sides=48
    for i in range(sides):
        a=2*math.pi*i/sides
        r=1+0.10*noise.noise(Vector((math.cos(a)*3+seed, math.sin(a)*3, 0)))
        verts.append((rx*r*math.cos(a), ry*r*math.sin(a)-0.035, z+0.0005*math.sin(5*a)))
    for i in range(sides):
        faces.append((0,1+i,1+((i+1)%sides)))
    return kit.mesh(name, verts, faces, material=mat)


def build(kit):
    m=kit.mats
    cloth=m.hessian("M_StoreSackHessian", base=(0.32,0.265,0.17), dark=(0.105,0.083,0.052), dust=0.18, seed=60.0)
    twine=m.rawhide("M_StoreSackTwine", color=(0.21,0.16,0.09), strands=2, twist=125.0)
    flour=kit.material("M_StoreSackFlourDust", (0.50,0.47,0.38), roughness=0.99)
    C.zero_subsurface(cloth,twine,flour)
    parts=[build_sack_body(kit,cloth)]
    # gathered collar wrinkles and twine loop around the neck
    neck_z=0.565
    pts=[]
    for i in range(49):
        a=2*math.pi*i/48
        rx=0.082+0.005*math.sin(5*a)
        ry=0.050+0.004*math.cos(4*a)
        pts.append(Vector((-0.030+rx*math.cos(a), -0.050+ry*math.sin(a), neck_z+0.004*math.sin(3*a))))
    parts.append(kit.tube("NeckTieTwine", pts, radius=0.0032, sides=7, material=twine))
    # knot and loose ends on front (-Y)
    parts.append(kit.sphere("TwineKnot",0.013,location=(-0.030,-0.106,neck_z+0.001),material=twine,segments=12,rings=6,scale=(1.2,0.7,0.8)))
    parts.append(kit.tube("TwineLooseEndA",[Vector((-0.035,-0.112,neck_z)),Vector((-0.070,-0.145,0.515)),Vector((-0.085,-0.150,0.470))],radius=0.0022,sides=6,material=twine))
    parts.append(kit.tube("TwineLooseEndB",[Vector((-0.025,-0.112,neck_z)),Vector((0.005,-0.150,0.530)),Vector((0.020,-0.155,0.495))],radius=0.0020,sides=6,material=twine))
    # side seam and stitch knots down the front-left edge
    seam_x=-0.205
    stitch_pts=[]
    for k in range(17):
        z=0.055+k*0.026
        parts.append(kit.tube(f"SeamStitch_{k}",[Vector((seam_x,-0.122,z)),Vector((seam_x+0.026,-0.128,z+0.010))],radius=0.0014,sides=5,material=twine))
    parts.append(kit.tube("RaisedSideSeam",[Vector((seam_x,-0.118,0.035)),Vector((seam_x+0.010,-0.135,0.24)),Vector((seam_x-0.006,-0.125,0.48))],radius=0.0030,sides=7,material=cloth))
    # flour dust on the floor contact and a pale bloom on the lower sack.
    parts.append(ellipse_disc(kit,"FlourDustOnFloor",0.27,0.19,0.002,flour,seed=5))
    return kit.join(parts,"SM_Store_Sack",unwrap=False,reshade=True,smooth_angle=60)
