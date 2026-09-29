"""Hand-clearable estate rubbish for the Coral Island-style clear-out round the manor: a small
pile of rotten deal boards, a small household midden and a heap of rusty scrap iron.

Real-object research:
- Deal boards (sawn pine/spruce) from a fallen fence or cold frame were 4-9 in wide and about an
  inch thick. Left in wet Cornish grass they silver on top, rot soft and punky at the ends where
  water wicks into end grain, grow moss/lichen along the damp lower arrises, and snap across
  weak knots leaving torn fibre splinters. Cut nails (square, iron) stay in them and rust.
- A cottage midden is ash, cinders, broken whiteware/transfer-printed crockery, bottle glass,
  rags and an odd rusted hoop, trodden into a low mound that grades into the turf.
- Farm scrap: coopered-barrel hoops (1.5 in flat iron bands), a cracked three-legged cast-iron
  cooking pot, a broken spade blade with its socket snapped off, a length of chain and loose cut
  nails, all long rusted to a muted orange-brown and staining the soil under them.

Geometry: metres, Z up, pivot at the ground-centre of each pile (z=0 is the ground line; the
lowest parts sit a centimetre or so below it so the turf, not a mesh edge, forms the boundary).
Original procedural geometry and materials only.
"""
import math, os, random, sys
from pathlib import Path
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import estate_debris as D
import homestead_rocks as rocks
import store.common as C

NAME="EstateRubbish"
DESCRIPTION="Hand-clearable estate rubbish: rotten plank pile, small midden heap and rusty scrap-iron heap."
COLLISION="convex"
TRIANGLE_BUDGET=20000
PROVENANCE="Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT=os.environ.get("HOMESTEAD_DRAFT")=="1"
BAKE={"size":1024 if DRAFT else 2048,"samples":16 if DRAFT else 96,"maps":("basecolor","roughness","normal","ao","metallic")}
BEAUTY={"pose":(0,0,0),"views":["hero","detail","eye"],"eye_distance":3.0,"meshes":{
 "SM_RottenPlanks":{"focus":(0.45,0.0,0.06),"eye_distance":3.0},
 "SM_RubbishHeapSmall":{"focus":(0.10,0.05,0.10),"eye_distance":2.8},
 "SM_ScrapHeap":{"focus":(0.0,0.05,0.12),"eye_distance":2.6}}}
REPORT={"unreal_frame":"Blender (x,y,z) imports as Unreal (x,-y,z).",
 "pivot":"Ground-centre of each pile at z=0; up to ~2 cm of each pile sits below z=0 so it reads sunk into turf.",
 "orientation":{"SM_RottenPlanks":"Longest board runs roughly X.","SM_RubbishHeapSmall":"No preferred facing.",
  "SM_ScrapHeap":"No preferred facing; pot mouth faces roughly +X+Y."},
 "lods":"LOD1/LOD2 are collapse-decimated from LOD0 (0.45 / 0.18) and share its UVs and textures."}
SEED=186221

smoothstep=D.smoothstep

# ------------------------------------------------------------------ materials

def mat_rotten_deal(kit):
    # D.board pcoord: x across the width, y through the thickness, z along the length. We rescale
    # z to +-0.8 at the board ends so the punky-end mask works for any length.
    g=kit.mats.Graph("M_EstateRubbish_RottenDeal"); p=g.coord(); x,y,z=g.separate(p)
    stretched=g.combine(g.math("MULTIPLY",x,26.0),g.math("MULTIPLY",y,40.0),g.math("MULTIPLY",z,1.3))
    fibre=g.noise(stretched,scale=9.0,detail=10.0,roughness=.6).outputs["Fac"]
    streak=g.noise(g.combine(g.math("MULTIPLY",x,5.0),g.math("MULTIPLY",y,8.0),g.math("MULTIPLY",z,.5)),scale=5.0,detail=5.0,roughness=.62).outputs["Fac"]
    checks=g.voronoi(g.combine(g.math("MULTIPLY",x,1.1),g.math("MULTIPLY",y,1.1),g.math("MULTIPLY",z,.035)),scale=34.0,feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack=g.remap(checks,0.0,.010,.55,0.0)
    tone=g.math("ADD",g.math("MULTIPLY",fibre,.7),g.math("MULTIPLY",streak,.3))
    # Rain-silvered grey-brown with browner late-wood streaks; light enough to read at 10-15 m.
    col=g.ramp(tone,[(.15,(.062,.052,.040)),(.45,(.150,.132,.108)),(.70,(.205,.190,.162)),(.95,(.245,.232,.205))])
    col=g.mix(col,(.105,.078,.052),g.remap(streak,.52,.72,0.0,.45))
    col=g.mix(col,(.028,.023,.018),crack)
    # Punky ends: dark, soft brown rot creeping in from the end grain, broken by noise.
    endn=g.noise(p,scale=11.0,detail=5.0).outputs["Fac"]
    endm=g.remap(g.math("ADD",g.math("ABSOLUTE",z),g.math("MULTIPLY",endn,.14)),.64,.84,0.0,.88)
    col=g.mix(col,g.ramp(endn,[(.3,(.050,.036,.024)),(.7,(.095,.070,.045))]),endm)
    # Moss on the damp underside and lower arrises; pale lichen crust on the weathered top.
    mossn=g.noise(g.combine(g.math("MULTIPLY",x,3.0),g.math("MULTIPLY",y,3.0),g.math("MULTIPLY",z,.9)),scale=14.0,detail=6.0).outputs["Fac"]
    mossm=g.math("MULTIPLY",g.remap(y,.004,-.010,0.0,1.0),g.remap(mossn,.35,.6,.25,1.0))
    mossm=g.math("MAXIMUM",mossm,g.math("MULTIPLY",endm,g.remap(mossn,.55,.7,0.0,.6)))
    col=g.mix(col,g.ramp(mossn,[(.3,(.040,.060,.022)),(.75,(.085,.110,.040))]),mossm)
    crust=g.noise(g.combine(g.math("MULTIPLY",x,2.4),g.math("MULTIPLY",y,2.4),g.math("MULTIPLY",z,.25)),scale=17.0,detail=6.0).outputs["Fac"]
    col=g.mix(col,(.165,.180,.135),g.math("MULTIPLY",g.remap(crust,.64,.70,0.0,.55),g.remap(y,0.0,.008)))
    g.set("Base Color",col)
    g.set("Roughness",g.remap(endm,0.0,1.0,.90,.98))
    height=g.math("SUBTRACT",g.math("ADD",g.math("MULTIPLY",fibre,.55),g.math("MULTIPLY",streak,.15)),g.math("MULTIPLY",crack,.75))
    height=g.math("SUBTRACT",height,g.math("MULTIPLY",endm,g.math("MULTIPLY",endn,.6)))
    g.set("Normal",g.bump(height,strength=.6,distance=.002)); return g.mat

def mat_scrap_rust(kit,name,scale_amt=.25,seed=0.0):
    # Long-exposed iron: mostly muted orange-brown rust with some black mill/forge scale left.
    g=kit.mats.Graph(name); p=g.coord(); ps=g.vmath("ADD",p,(seed*.7,seed*1.9,seed*1.3))
    fine=g.noise(ps,scale=60.0,detail=5.0,roughness=.62).outputs["Fac"]
    blooms=g.noise(ps,scale=12.0,detail=5.0,roughness=.7).outputs["Fac"]
    pits=g.noise(ps,scale=320.0,detail=2.0).outputs["Fac"]
    col=g.ramp(fine,[(.20,(.040,.022,.013)),(.50,(.088,.043,.021)),(.80,(.140,.068,.032))])
    col=g.mix(col,(.185,.082,.034),g.math("MULTIPLY",g.remap(blooms,.50,.64,0.0,.7),g.remap(fine,.35,.65,.4,1.0)))
    col=g.mix(col,(.070,.046,.030),g.remap(g.noise(ps,scale=4.0,detail=3.0).outputs["Fac"],.55,.72,0.0,.5))
    scale_m=g.math("MULTIPLY",g.remap(blooms,.42,.49,1.0,0.0),scale_amt)
    col=g.mix(col,g.mix((.040,.034,.030),(.075,.066,.058),g.remap(fine,.3,.7)),scale_m)
    col=g.mix(col,(.045,.022,.012),g.remap(pits,.68,.76,0.0,.5))
    g.set("Base Color",col); g.set("Metallic",g.math("MULTIPLY",scale_m,.55))
    g.set("Roughness",g.remap(scale_m,0.0,1.0,.90,.62))
    height=g.math("ADD",g.math("MULTIPLY",fine,.4),g.math("MULTIPLY",g.remap(pits,.6,.8),.6))
    g.set("Normal",g.bump(height,strength=.5,distance=.0012)); return g.mat

def mat_rust_soil(kit):
    # Same r mapping as D.mat_midden_soil: pcoord normalised so r=len(x/1.10, y/.82) is 1 at the rim.
    g=kit.mats.Graph("M_EstateRubbish_RustStainedSoil"); p=g.coord(); x,y,z=g.separate(p)
    r=g.vmath("LENGTH",g.combine(g.math("DIVIDE",x,1.10),g.math("DIVIDE",y,.82),0.0))
    soil=g.noise(g.combine(g.math("MULTIPLY",x,6.0),g.math("MULTIPLY",y,6.0),z),scale=9.0,detail=6.0,roughness=.65).outputs["Fac"]
    base=g.ramp(soil,[(.2,(.050,.041,.031)),(.6,(.082,.068,.050)),(1,(.110,.092,.068))])
    stain=g.noise(g.combine(g.math("MULTIPLY",x,2.2),g.math("MULTIPLY",y,2.2),0.0),scale=6.0,detail=4.0).outputs["Fac"]
    core=g.remap(r,.75,.2,0.0,1.0)
    base=g.mix(base,g.ramp(soil,[(.3,(.095,.044,.020)),(.8,(.150,.072,.032))]),g.math("MULTIPLY",core,g.remap(stain,.40,.62,.2,.9)))
    edge=g.remap(r,.60,.98,0.0,.85)
    moss=g.ramp(g.noise(p,scale=18.0,detail=5.0).outputs["Fac"],[(.2,(.046,.056,.026)),(.8,(.072,.086,.036))])
    base=g.mix(base,moss,edge)
    g.set("Base Color",base); g.set("Roughness",.97)
    g.set("Normal",g.bump(soil,strength=.35,distance=.0012)); return g.mat

def mat_ash_midden(kit):
    # Lighter than the big heap: fresh-ish wood ash and grey-brown soil, so a field of small heaps
    # reads as pale ash spots in the turf rather than dark holes. Same rim mapping as mat_rust_soil.
    g=kit.mats.Graph("M_EstateRubbish_AshMidden"); p=g.coord(); x,y,z=g.separate(p)
    r=g.vmath("LENGTH",g.combine(g.math("DIVIDE",x,1.10),g.math("DIVIDE",y,.82),0.0))
    ash=g.noise(g.combine(g.math("MULTIPLY",x,7.0),g.math("MULTIPLY",y,7.0),g.math("MULTIPLY",z,2.0)),scale=10.0,detail=6.0,roughness=.66).outputs["Fac"]
    cinder=g.noise(g.combine(g.math("MULTIPLY",x,16.0),g.math("MULTIPLY",y,16.0),0.0),scale=20.0,detail=3.0).outputs["Fac"]
    base=g.ramp(ash,[(.18,(.066,.060,.052)),(.55,(.112,.103,.088)),(.85,(.160,.150,.132))])
    base=g.mix(base,(.030,.028,.025),g.remap(cinder,.72,.90,0.0,.45))
    edge=g.remap(r,.60,.98,0.0,.85)
    moss=g.ramp(g.noise(p,scale=18.0,detail=5.0).outputs["Fac"],[(.2,(.046,.056,.026)),(.8,(.072,.086,.036))])
    base=g.mix(base,moss,edge)
    g.set("Base Color",base); g.set("Roughness",.98)
    g.set("Normal",g.bump(g.math("ADD",g.math("MULTIPLY",ash,.35),g.math("MULTIPLY",cinder,.18)),strength=.34,distance=.0012)); return g.mat

def make_materials(kit):
    m=D.make_materials(kit)
    m["deal"]=mat_rotten_deal(kit)
    m["scrap"]=mat_scrap_rust(kit,"M_EstateRubbish_ScrapRust",.22,31)
    m["cast"]=mat_scrap_rust(kit,"M_EstateRubbish_CastIronRust",.42,32)
    m["rust_soil"]=mat_rust_soil(kit)
    m["terracotta"]=kit.material("M_EstateRubbish_WeatheredTerracotta",(.230,.105,.058),roughness=.92)
    m["shell"]=kit.material("M_EstateRubbish_LimpetShell",(.330,.300,.250),roughness=.80)
    m["bone"]=kit.material("M_EstateRubbish_WeatheredBone",(.215,.195,.155),roughness=.85)
    m["ash_soil"]=mat_ash_midden(kit)
    D.zero(m["ash_soil"])
    D.zero(m["deal"],m["scrap"],m["cast"],m["rust_soil"],m["terracotta"],m["bone"],m["shell"]); return m

# ------------------------------------------------------------------ shared helpers

def edge_radius(a,seed):
    return 1.0+.10*noise.noise(Vector((math.cos(a)*2.7,math.sin(a)*2.7,seed)))+.045*math.sin(5*a+seed)+.02*math.sin(9*a-seed)

def ground_patch(kit,n,rx,ry,height,mat,seed,rings=20,seg=96,dip=.03):
    """Polar ground patch whose broken rim sinks `dip` below z=0 so the terrain overlaps the edge."""
    verts=[(0.0,0.0,height(0.0,0.0))]; coords=[Vector((0,0,verts[0][2]))]; faces=[]
    for i in range(1,rings+1):
        r=(i/rings)**.85
        for j in range(seg):
            a=math.tau*j/seg; e=edge_radius(a,seed); x=math.cos(a)*rx*r*e; y=math.sin(a)*ry*r*e
            z=height(x,y)*smoothstep(1.0,.55,r)-dip*smoothstep(.72,1.0,r)
            verts.append((x,y,z)); coords.append(Vector((math.cos(a)*1.10*r*e,math.sin(a)*.82*r*e,z)))
    for j in range(seg): faces.append((0,1+j,1+(j+1)%seg))
    for i in range(rings-1):
        a0=1+i*seg; b0=a0+seg
        for j in range(seg): faces.append((a0+j,b0+j,b0+(j+1)%seg,a0+(j+1)%seg))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def world_verts(o):
    mw=o.matrix_world; return [mw@v.co for v in o.data.vertices]

def support_tree(objs):
    verts=[]; polys=[]
    for o in objs:
        base=len(verts); verts+=world_verts(o); polys+=[[base+i for i in p.vertices] for p in o.data.polygons]
    return BVHTree.FromPolygons(verts,polys) if polys else None

def support_z(tree,x,y):
    if tree:
        hit=tree.ray_cast(Vector((x,y,3.0)),Vector((0,0,-1)))
        if hit[0] is not None: return max(0.0,hit[0].z)
    return 0.0

def move(group,M):
    for o in group: o.matrix_world=M@o.matrix_world

def rest(body,group,tree,max_deg=14):
    """Drop `body` (and its attached group) onto the ground/earlier parts, then tip it about the
    first contact until its far end touches too, so nothing floats."""
    def clearance(): return [(v,v.z-support_z(tree,v.x,v.y)) for v in world_verts(body)]
    c,gap=min(clearance(),key=lambda t:t[1]); move(group,Matrix.Translation((0,0,-gap)))
    vs=world_verts(body); c=min(vs,key=lambda v:v.z-support_z(tree,v.x,v.y))
    d=body.matrix_world.to_3x3()@Vector((1,0,0)); d.z=0; d.normalize()
    far=max(vs,key=lambda v:abs((v-c).dot(d))); axis=d.cross(Vector((0,0,1)))
    step=math.radians(.25)
    for sign in (1,-1):
        R=Matrix.Translation(c)@Matrix.Rotation(sign*step,4,axis)@Matrix.Translation(-c)
        if (R@far).z<far.z: break
    for _ in range(int(max_deg/.25)):
        move(group,R)
        if min(t[1] for t in clearance())< -.0015:
            move(group,R.inverted()); break

def lods(kit,obj,smooth=48):
    meshes=[obj]
    for idx,ratio in enumerate((.45,.18),1):
        lod=rocks.lod(kit,obj,f"{obj.name}_LOD{idx}",ratio)
        meshes.append(kit.finalize(lod,pivot=None,unwrap=False,reshade=True,smooth_angle=smooth))
    print("HOMESTEAD_LODS",obj.name,[rocks.triangles(o) for o in meshes]); return meshes

def centre_xy(parts):
    ws=[v for o in parts for v in world_verts(o)]
    cx=(min(v.x for v in ws)+max(v.x for v in ws))/2; cy=(min(v.y for v in ws)+max(v.y for v in ws))/2
    move(parts,Matrix.Translation((-cx,-cy,0)))

def band_ring(kit,n,radius,width,thick,mat,seed,sides=72,squash=(1,1),arc=None):
    # Flat iron band ring in the local XY plane (axis Z), slightly out of round. `arc`=(a0,a1)
    # builds only that part (the rest would be buried), with capped ends.
    rows=[]; coords=[]
    count=sides if arc is None else max(8,int(sides*(arc[1]-arc[0])/math.tau))+1
    for i in range(count):
        a=math.tau*i/sides if arc is None else arc[0]+(arc[1]-arc[0])*i/(count-1); r=radius*(1+.03*noise.noise(Vector((math.cos(a)*2+seed,math.sin(a)*2,.3))))
        c=Vector((math.cos(a)*r*squash[0],math.sin(a)*r*squash[1],0)); o=Vector((math.cos(a),math.sin(a),0))
        rows.append([tuple(c-o*thick/2-Vector((0,0,width/2))),tuple(c+o*thick/2-Vector((0,0,width/2))),
                     tuple(c+o*thick/2+Vector((0,0,width/2))),tuple(c-o*thick/2+Vector((0,0,width/2)))])
        s=radius*a; coords.append([(-thick/2,-width/2,s),(thick/2,-width/2,s),(thick/2,width/2,s),(-thick/2,width/2,s)])
    closed=arc is None
    obj=kit.loft(n,rows,material=mat,coords=coords,cap_start=not closed,cap_end=not closed,cyclic=closed)
    kit.roughen(obj,strength=.0012,scale=40,seed=seed,subdivide=0); return obj

# ------------------------------------------------------------------ SM_RottenPlanks

def plank(kit,n,L,W,T,mat,seed,snapped=(False,False),rot_ends=True):
    rng=random.Random(seed)
    obj=D.board(kit,n,(L,W,T),(0,0,0),mat,seed=seed,rough=.0025)
    attr=obj.data.attributes["pcoord"]; pc=[0.0]*(3*len(obj.data.vertices)); attr.data.foreach_get("vector",pc)
    for i in range(len(obj.data.vertices)): pc[3*i+2]*=1.6/L
    attr.data.foreach_set("vector",pc)
    for v in obj.data.vertices:
        x=v.co.x; side=1 if x>0 else 0; f=smoothstep(L/2-.13,L/2,abs(x))
        if snapped[side] and abs(x)>L/2-.045:
            v.co.x+=math.copysign(rng.uniform(-.075,.012),x)
        elif rot_ends and f>0:
            nz=noise.noise(Vector((v.co.y*30,v.co.z*30,seed+side)))
            k=1-f*(.14+.10*nz); v.co.y*=k; v.co.z*=1-f*(.22+.12*nz)
            v.co.x-=math.copysign(f*(.010+.018*abs(nz)),x)
    obj.data.update(); parts=[obj]
    for side,sn in enumerate(snapped):
        if not sn: continue
        sx=L/2 if side else -L/2; sgn=1 if side else -1
        for k in range(rng.randint(5,8)):
            y=rng.uniform(-W*.44,W*.44); z=rng.uniform(-T*.3,T*.35); ln=rng.uniform(.03,.10)
            pts=[(sx-sgn*.05,y,z),(sx+sgn*ln*.5,y+rng.uniform(-.005,.005),z+rng.uniform(-.003,.004)),(sx+sgn*ln,y+rng.uniform(-.010,.010),z+rng.uniform(-.004,.006))]
            parts.append(kit.tube(f"{n}_Splinter{k}",pts,radii=[rng.uniform(.0035,.0055),.0022,.0005],sides=5,material=mat))
    return parts

def bent_nail(kit,n,loc,mat,seed,bend=True):
    rng=random.Random(seed); h=rng.uniform(.018,.03); a=rng.uniform(0,math.tau)
    pts=[Vector(loc)+Vector((0,0,-.004)),Vector(loc)+Vector((0,0,h*.6))]
    pts.append(pts[-1]+Vector((math.cos(a)*h*.55,math.sin(a)*h*.55,h*.25)) if bend else Vector(loc)+Vector((0,0,h)))
    return kit.tube(n,pts,radii=[.0026,.0024,.0015],sides=6,material=mat)

def build_rotten_planks(kit,m):
    rng=random.Random(SEED+10); deal=m["deal"]; iron=m["scrap"]
    # (L, W, T, (x, y), yaw deg, snapped ends, nails [(x_local, y_local)])
    spec=[(1.52,.195,.026,(0.02,-.10),6,(False,False),[(-.66,.05),(.64,-.06)]),
          (1.36,.150,.024,(-.04,.12),-19,(False,True),[(-.58,.0)]),
          (1.14,.135,.022,(.10,.00),27,(True,False),[(.45,.03),(.45,-.035)]),
          (.62,.170,.025,(-.40,-.14),78,(True,True),[]),
          (.40,.120,.022,(.42,.17),-52,(True,False),[])]
    placed=[]; allparts=[]
    for i,(L,W,T,(x,y),yaw,sn,nails) in enumerate(spec):
        group=plank(kit,f"DealBoard_{i}",L,W,T,deal,SEED+20+i*7,snapped=sn)
        for k,(nx,ny) in enumerate(nails):
            group.append(bent_nail(kit,f"DealBoard_{i}_CutNail{k}",(nx,ny,T/2),iron,SEED+60+i*3+k,bend=(k%2==0)))
        M=Matrix.Translation((x,y,.5))@Matrix.Rotation(math.radians(yaw),4,"Z")@Matrix.Rotation(math.radians(rng.uniform(-2.5,2.5)),4,"X")
        move(group,M)
        rest(group[0],group,support_tree(placed))
        placed.append(group[0]); allparts+=group
    for k in range(10 if not DRAFT else 4):
        x=rng.uniform(-.7,.7); y=rng.uniform(-.4,.4)
        allparts.append(D.loose_stave(kit,f"RotFlake_{k}",(x,y,.0),rng.uniform(-math.pi,math.pi),rng.uniform(.04,.12),deal,SEED+90+k,curl=.002))
    centre_xy(allparts); move(allparts,Matrix.Translation((0,0,-.011)))
    obj=kit.join(allparts,"SM_RottenPlanks",pivot=None,unwrap=True,reshade=True,smooth_angle=45)
    return lods(kit,obj,45)

# ------------------------------------------------------------------ SM_RubbishHeapSmall

RX_S,RY_S=.57,.43

def small_h(x,y):
    nx,ny=x/RX_S,y/RY_S; ell=nx*nx+ny*ny
    z=(.090*math.exp(-1.9*ell)+.050*math.exp(-9*((nx+.33)**2+(ny-.22)**2))+.040*math.exp(-10*((nx-.30)**2+(ny+.26)**2))
       +.008*noise.noise(Vector((x*14+2,y*14,.4)))+.005*noise.noise(Vector((x*32,y*32-1,2.2)))-.003)
    return max(0.0,z)

def build_rubbish_heap_small(kit,m):
    rng=random.Random(SEED+300)
    def h(x,y):
        a=math.atan2(y/RY_S,x/RX_S); rr=math.hypot(x/RX_S,y/RY_S)/edge_radius(a,7.0)
        return small_h(x,y)*smoothstep(1.0,.55,rr)-.03*smoothstep(.72,1.0,rr)
    parts=[ground_patch(kit,"SmallMiddenMound",RX_S,RY_S,small_h,m["ash_soil"],7.0,rings=22 if not DRAFT else 14,seg=96 if not DRAFT else 64)]
    def inside(x,y,lim=.82): return (x/RX_S)**2+(y/RY_S)**2<lim*lim
    for i in range(26 if not DRAFT else 12):
        x=rng.uniform(-RX_S,RX_S); y=rng.uniform(-RY_S,RY_S)
        if not inside(x,y,rng.uniform(.6,.9)): continue
        parts.append(kit.sphere(f"CinderClod_{i:02d}",rng.uniform(.010,.026),location=(x,y,h(x,y)+rng.uniform(.0,.012)),material=m["ash_dark"] if rng.random()<.6 else m["midden_soil"],segments=8,rings=4,scale=(1,rng.uniform(.6,1.1),rng.uniform(.18,.36))))
    for i in range(16 if not DRAFT else 8):
        x=rng.uniform(-.38,.38); y=rng.uniform(-.27,.27)
        if not inside(x,y): continue
        blue=rng.random()<.6
        parts.extend(D.shard(kit,f"CrockeryShard_{i:02d}",(x,y,h(x,y)+rng.uniform(.002,.012)),rng.uniform(-math.pi,math.pi),rng.uniform(.035,.085),rng.uniform(.025,.060),m["whiteware"] if blue else m["brown_stoneware"],SEED+320+i,thick=rng.uniform(.003,.006),pitch=rng.uniform(-.35,.35),roll=rng.uniform(-.25,.25),blue=blue,blue_mat=m["blue_transfer"]))
    for i in range(4):
        x=rng.uniform(-.38,.38); y=rng.uniform(-.28,.28)
        parts.extend(D.shard(kit,f"GlassShard_{i}",(x,y,h(x,y)+.006),rng.uniform(-math.pi,math.pi),rng.uniform(.025,.06),rng.uniform(.018,.045),m["green_glass"] if i%3 else m["brown_glass"],SEED+350+i,thick=.004,pitch=rng.uniform(-.2,.2)))
    parts.append(D.bottle_along(kit,"HalfBuriedGreenBottle",(-.30,.10,h(-.30,.10)+.012),(-.06,.19,h(-.06,.19)+.042),m["green_glass"],SEED+360,.95))
    # A sprung barrel hoop trodden into the mound: band on edge, dipping in and out of the ash.
    pts=[]
    for i in range(55):
        a=-.35+(math.tau-.75)*i/54; x=.10+math.cos(a)*.215; y=-.08+math.sin(a)*.165
        pts.append((x,y,h(x,y)-.010+.013*math.sin(2*a+1.1)+.004*math.sin(a*5)))
    parts.append(D.rect_beam(kit,"TroddenRustyHoop",pts,.006,.034,m["scrap"],SEED+370,cuts=1))
    for i in range(3):
        x,y=((.20,.17),(-.24,-.13),(.36,-.05))[i]
        rag=D.cloth(kit,f"RottenRag_{i}",(x,y,h(x,y)+.010),rng.uniform(-math.pi,math.pi),rng.uniform(.15,.22),rng.uniform(.10,.15),m["rag"],SEED+380+i,pitch=rng.uniform(-.12,.12),roll=rng.uniform(-.1,.1))
        kit.roughen(rag,strength=.014,scale=16,seed=SEED+385+i,subdivide=2); parts.append(rag)
    parts.append(D.board(kit,"MiddenBoardEnd",(.34,.055,.020),(.30,.20,h(.30,.20)+.018),m["crate_wood"],rot=(0,-4,-38),rough=.003,seed=SEED+390))
    # Limpet shells (a Cornish kitchen midden staple): pale ribbed cones, some upside down.
    for i in range(9 if not DRAFT else 4):
        x=rng.uniform(-.40,.40); y=rng.uniform(-.28,.28)
        if not inside(x,y,.85): continue
        r=rng.uniform(.018,.028); up=rng.random()<.6
        sh=kit.cylinder(f"LimpetShell_{i}",r,r*.75,location=(x,y,h(x,y)+(r*.3 if up else r*.45)),material=m["shell"],sides=14,radius_top=r*.12,cap=True)
        sh.rotation_euler=(rng.uniform(-.3,.3) if up else math.pi+rng.uniform(-.3,.3),rng.uniform(-.3,.3),rng.uniform(0,math.tau))
        kit.roughen(sh,strength=.0015,scale=90,seed=SEED+400+i,subdivide=1); parts.append(sh)
    # Half a broken terracotta flowerpot and two weathered bones.
    pot=C.bulged_vessel_shell(kit,"BrokenFlowerpotHalf",.12,lambda t:.045+.028*t,m["terracotta"],sides=14,rings=4,thickness=.008,start_angle=.2,end_angle=math.pi-.3,cap_top=True,cap_bottom=True,seed=SEED+410)
    pot.matrix_world=Matrix.Translation((-.10,-.20,h(-.10,-.20)+.030))@Matrix.Rotation(.6,4,"Z")@Matrix.Rotation(math.radians(-96),4,"X")@Matrix.Translation((0,0,-.06))
    parts.append(pot)
    for i,(a,b) in enumerate((((.02,.24),(.18,.29)),((-.36,.02),(-.27,-.07)))):
        p0=Vector((a[0],a[1],h(*a)+.008)); p1=Vector((b[0],b[1],h(*b)+.008)); mid=p0.lerp(p1,.5)+Vector((0,0,.003))
        parts.append(kit.tube(f"WeatheredBone_{i}",[p0,p0.lerp(mid,.2),mid,mid.lerp(p1,.8),p1],radii=[.013,.008,.007,.008,.012],sides=10,material=m["bone"]))
    obj=kit.join(parts,"SM_RubbishHeapSmall",pivot=None,unwrap=True,reshade=True,smooth_angle=50)
    return lods(kit,obj,50)

# ------------------------------------------------------------------ SM_ScrapHeap

def pot_radius(t):
    # Squat cast-iron cooking pot: rounded base, belly low, gently drawn-in shoulder, thick rim.
    if t<.40: return .006+.158*math.sin(min(1.0,(t+.02)/.42)*math.pi/2)**.5
    return .164-.022*smoothstep(.40,.86,t)+.012*smoothstep(.91,.96,t)

def build_cast_pot(kit,m,M,seed):
    cast=m["cast"]; H=.20; gap0=math.radians(-28); gap1=math.radians(26)
    body=C.bulged_vessel_shell(kit,"CrackedCastIronPot",H,pot_radius,cast,sides=44 if not DRAFT else 28,rings=22 if not DRAFT else 14,thickness=.009,start_angle=gap1,end_angle=gap0+math.tau,cap_top=True,cap_bottom=True,seed=seed)
    parts=[body]
    for k in range(3):
        a=math.pi/3+math.tau*k/3
        parts.append(kit.cylinder(f"PotLeg_{k}",.016,.045,location=(math.cos(a)*.085,math.sin(a)*.085,-.012),material=cast,sides=10,radius_top=.020,bevel=.003))
    for k,a in enumerate((math.pi/2,-math.pi/2)):
        c=Vector((math.cos(a)*.162,math.sin(a)*.162,H-.035)); o=Vector((math.cos(a),math.sin(a),0))
        parts.append(kit.tube(f"PotEarLug_{k}",[c-Vector((0,0,.018)),c+o*.022,c+Vector((0,0,.018))],radius=.0065,sides=8,material=cast))
    moved=list(parts); move(moved,M)
    # The broken-out shard lies beside the pot, concave side up.
    shard=C.bulged_vessel_shell(kit,"PotShard",H*.62,lambda t:pot_radius(.18+.62*t),cast,sides=10,rings=10,thickness=.009,start_angle=gap0+.05,end_angle=gap1-.08,cap_top=True,cap_bottom=True,seed=seed+3)
    return parts,shard

def spade_blade(kit,n,mat,seed):
    rng=random.Random(seed); nu,nv=10,16; T=.0045; verts=[]; faces=[]; coords=[]
    def half_w(v): return .095*(1-smoothstep(.72,1.0,v)**2*.55)
    def P(u,v,side):
        x=v*.27; y=u*half_w(v)
        z=.07*smoothstep(.35,1.0,v)**1.6+.012*u*u+side*T/2+.002*noise.noise(Vector((u*4,v*6,seed)))
        return Vector((x,y+.02*smoothstep(.5,1,v)*u,z))
    for side in (-1,1):
        for j in range(nv+1):
            for i in range(nu+1):
                u=-1+2*i/nu; v=j/nv; p=P(u,v,side); verts.append(tuple(p)); coords.append(Vector((u*.1,side*T/2,v*.27)))
    def idx(s,i,j): return s*(nv+1)*(nu+1)+j*(nu+1)+i
    for j in range(nv):
        for i in range(nu):
            faces.append((idx(1,i,j),idx(1,i+1,j),idx(1,i+1,j+1),idx(1,i,j+1)))
            faces.append((idx(0,i,j+1),idx(0,i+1,j+1),idx(0,i+1,j),idx(0,i,j)))
    loop=[(i,0) for i in range(nu)]+[(nu,j) for j in range(nv)]+[(i,nv) for i in range(nu,0,-1)]+[(0,j) for j in range(nv,0,-1)]
    for a,b in zip(loop,loop[1:]+loop[:1]): faces.append((idx(0,*a),idx(0,*b),idx(1,*b),idx(1,*a)))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); kit.recalc_normals(obj)
    # Snapped socket stub where the ash handle rotted out.
    sock=kit.tube(n+"_SnappedSocket",[(-.005,0,.006),(-.07,0,.012),(-.115,0,.016)],radii=[.021,.018,.017],sides=14,material=mat,cap=False)
    return [obj,sock]

def chain(kit,m,path,seed):
    rng=random.Random(seed); parts=[]; Lk,Wk,r=.056,.034,.0055
    pts=[Vector(p) for p in path]; lens=[0]
    for a,b in zip(pts,pts[1:]): lens.append(lens[-1]+(b-a).length)
    def at(s):
        for i in range(len(pts)-1):
            if lens[i+1]>=s: return pts[i].lerp(pts[i+1],(s-lens[i])/((lens[i+1]-lens[i]) or 1)),(pts[i+1]-pts[i]).normalized()
        return pts[-1],(pts[-1]-pts[-2]).normalized()
    pitch=Lk-2*r*1.05; count=int(lens[-1]/pitch)
    for k in range(count):
        c,t=at(k*pitch+pitch/2); upright=k%2==1
        rows=[]; coords=[]; N=18
        for i in range(N):
            a=math.tau*i/N; ax,ay=Lk/2-r,Wk/2-r
            ctr=Vector((math.cos(a)*ax,math.sin(a)*ay,0)); tan=Vector((-math.sin(a)*ax,math.cos(a)*ay,0)).normalized(); side=tan.cross(Vector((0,0,1))).normalized()
            ring=[]; cr=[]
            for q in range(7):
                b=math.tau*q/7; off=side*math.cos(b)*r+Vector((0,0,1))*math.sin(b)*r; ring.append(ctr+off); cr.append((math.cos(b)*r,math.sin(b)*r,a*.02))
            rows.append(ring); coords.append(cr)
        yaw=math.atan2(t.y,t.x)
        R=Matrix.Rotation(yaw,4,"Z")@(Matrix.Rotation(math.pi/2,4,"X") if upright else Matrix.Identity(4))
        z=(Wk/2-r*.3) if upright else r*.8
        M=Matrix.Translation((c.x,c.y,c.z+z+rng.uniform(-.002,.002)))@R
        link=kit.loft(f"ChainLink_{k}",[[tuple(M@v) for v in ring] for ring in rows],material=m["scrap"],coords=coords,cap_start=False,cap_end=False,cyclic=True)
        parts.append(link)
    return parts

def build_scrap_heap(kit,m):
    rng=random.Random(SEED+500); scrap=m["scrap"]
    RX,RY=.47,.41
    def gh(x,y):
        nx,ny=x/RX,y/RY; return max(0.0,.022*math.exp(-2.5*(nx*nx+ny*ny))+.006*noise.noise(Vector((x*12,y*12,1.3))))
    parts=[ground_patch(kit,"RustStainedEarth",RX,RY,gh,m["rust_soil"],3.0,rings=16 if not DRAFT else 10,seg=80 if not DRAFT else 48,dip=.025)]
    # Cracked three-legged pot tipped on its side, the cracked-out wedge uppermost and its mouth
    # half-facing the approach (-Y) so the open rim reads.
    yaw=math.radians(-40)
    Mpot=Matrix.Translation((-.07,.04,.150))@Matrix.Rotation(yaw,4,"Z")@Matrix.Rotation(math.radians(80),4,"Y")@Matrix.Rotation(math.radians(172),4,"Z")@Matrix.Translation((0,0,-.10))
    pot,shard=build_cast_pot(kit,m,Mpot,SEED+510); parts+=pot
    Ms=Matrix.Translation((.20,-.24,.0))@Matrix.Rotation(math.radians(-70),4,"Z")@Matrix.Rotation(math.radians(90),4,"Y")@Matrix.Translation((-.05,0,-.08))
    move([shard],Ms); rest(shard,[shard],None,max_deg=10); parts.append(shard)
    # A flat, sprung barrel hoop round the pot, and a second hoop half-buried on edge.
    hoopA=band_ring(kit,"SprungBarrelHoopFlat",.30,.036,.0065,scrap,SEED+520,squash=(1.05,.93))
    move([hoopA],Matrix.Translation((.04,-.02,.012))@Matrix.Rotation(math.radians(20),4,"Z")@Matrix.Rotation(math.radians(3),4,"X")); parts.append(hoopA)
    hoopB=band_ring(kit,"HalfBuriedHoopOnEdge",.20,.034,.0065,scrap,SEED+521,squash=(1,.97),arc=(-.45,math.pi+.45))
    move([hoopB],Matrix.Translation((.30,.26,.020))@Matrix.Rotation(math.radians(-58),4,"Z")@Matrix.Rotation(math.radians(48),4,"X")); parts.append(hoopB)
    # Broken spade blade resting over the flat hoop, bent tip curled up.
    sp=spade_blade(kit,"BentSpadeBlade",scrap,SEED+530)
    move(sp,Matrix.Translation((.14,-.30,.0))@Matrix.Rotation(math.radians(12),4,"Z"))
    rest(sp[0],sp,support_tree([hoopA]),max_deg=8); parts+=sp
    # A short length of chain snaking out from under the pot.
    parts+=chain(kit,m,[(-.22,-.10,0),(-.33,-.02,0),(-.38,.10,0),(-.33,.21,0),(-.40,.30,0)],SEED+540)
    for k in range(7):
        x=rng.uniform(-.40,.42); y=rng.uniform(-.36,.36); a=rng.uniform(0,math.tau)
        parts.append(kit.tube(f"LooseCutNail_{k}",[(x,y,.0035),(x+math.cos(a)*.035,y+math.sin(a)*.035,.0035+rng.uniform(0,.004))],radii=[.0032,.0022],sides=6,material=scrap))
    for k in range(14 if not DRAFT else 6):
        x=rng.uniform(-.40,.40); y=rng.uniform(-.34,.34)
        parts.append(kit.sphere(f"RustFlake_{k}",rng.uniform(.006,.016),location=(x,y,gh(x,y)+.002),material=scrap,segments=8,rings=4,scale=(1,.7,.2)))
    obj=kit.join(parts,"SM_ScrapHeap",pivot=None,unwrap=True,reshade=True,smooth_angle=40)
    return lods(kit,obj,40)

def build(kit):
    m=make_materials(kit)
    builders={"SM_RottenPlanks":lambda:build_rotten_planks(kit,m),
              "SM_RubbishHeapSmall":lambda:build_rubbish_heap_small(kit,m),
              "SM_ScrapHeap":lambda:build_scrap_heap(kit,m)}
    only=[s.strip() for s in os.environ.get("HOMESTEAD_RUBBISH_ONLY","").split(",") if s.strip()]
    out=[]
    for name in (only or list(builders)): out+=builders[name]()
    return out
