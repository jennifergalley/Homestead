"""Procedural estate debris for the ruined manor grounds: burst barrel, smashed crate, midden heap and collapsed lean-to.
Original geometry/materials only; Z-up metres; pivots are bottom-centre. Cornish rain-weathered wood is silver-grey/brown,
rust is muted orange-brown, slate is Delabole blue-grey, and crockery whites are deliberately dull.
"""
import math, os, random, sys
from pathlib import Path
from mathutils import Matrix, Vector, noise
sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME="EstateDebris"
DESCRIPTION="Ruined Cornish estate debris: broken barrel, broken crate, household midden and collapsed lean-to."
COLLISION="convex"
TRIANGLE_BUDGET=70000
PROVENANCE="Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT=os.environ.get("HOMESTEAD_DRAFT")=="1"
BAKE={"size":1024 if DRAFT else 4096,"samples":16 if DRAFT else 96,"maps":("basecolor","roughness","normal","ao","metallic")}
BEAUTY={"pose":(0,0,0),"views":["hero","detail","eye"],"eye_distance":4.2,"meshes":{
 "SM_BrokenBarrel":{"focus":(0.18,-0.05,0.28),"eye_distance":3.2},
 "SM_BrokenCrate":{"focus":(0.05,-0.10,0.22),"eye_distance":3.2},
 "SM_RubbishHeap":{"focus":(0.58,0.25,0.22),"eye_distance":4.2},
 "SM_CollapsedLeanTo":{"focus":(0.75,0.0,0.28),"eye_distance":5.0}}}
REPORT={"unreal_frame":"Blender (x,y,z) imports as Unreal (x,-y,z).","pivot":"Each mesh pivot is bottom centre at z=0.","orientation":{
 "SM_BrokenBarrel":"Axis runs Blender/Unreal X; burst open end faces +X.",
 "SM_BrokenCrate":"Skewed slatted face generally faces Blender -Y / Unreal +Y.",
 "SM_RubbishHeap":"No preferred facing.",
 "SM_CollapsedLeanTo":"Wall side is local -X in both Blender and Unreal; back edge is flat along x=-1.1 m."},
 "materials":"The prop builder has one BAKE spec per recipe, so final build uses 4096px for all meshes; drafts use 1024px via HOMESTEAD_DRAFT."}
SEED=185109

def smoothstep(a,b,x):
    t=min(max((x-a)/((b-a) or 1),0),1); return t*t*(3-2*t)

def frame(direction, roll=0.0):
    t=Vector(direction).normalized(); seed=Vector((0,0,1)) if abs(t.z)<.92 else Vector((0,1,0))
    side=t.cross(seed).normalized(); up=side.cross(t).normalized()
    if roll:
        q=Matrix.Rotation(roll,4,t); side=q@side; up=q@up
    return t,side,up

def mtx(loc,yaw=0,pitch=0,roll=0):
    return Matrix.Translation(Vector(loc))@Matrix.Rotation(yaw,4,"Z")@Matrix.Rotation(pitch,4,"X")@Matrix.Rotation(roll,4,"Y")

def settle(parts):
    lo=min((o.matrix_world@v.co).z for o in parts for v in o.data.vertices)
    for o in parts: o.location.z-=lo

def zero(*mats): C.zero_subsurface(*mats)

def board_matrix(loc,rot):
    rx,ry,rz=[math.radians(a) for a in rot]
    return Matrix.Translation(Vector(loc))@Matrix.Rotation(rx,4,"X")@Matrix.Rotation(ry,4,"Y")@Matrix.Rotation(rz,4,"Z")

def board(kit,n,size,loc,mat,rot=(0,0,0),seed=0,rough=.002,bevel=.005):
    # Weathered plank, not a perfect box: rounded arrises, cupping, twist, checks and ragged ends.
    rng=random.Random(seed); sx,sy,sz=size
    long=max(range(3), key=lambda i:(sx,sy,sz)[i])
    if long==2:
        # Convert uncommon vertical planks to local X length, then rotate back around Y.
        obj=board(kit,n,(sz,sy,sx),loc,mat,(rot[0],rot[1]+90,rot[2]),seed,rough,bevel)
        return obj
    if long==1:
        obj=board(kit,n,(sy,sx,sz),loc,mat,(rot[0],rot[1],rot[2]+90),seed,rough,bevel)
        return obj
    L,W,T=sx,sy,sz
    cuts=8 if not DRAFT else 5
    bev=min(W,T)*0.22
    section=[(-W/2+bev,-T/2), (W/2-bev,-T/2), (W/2,-T/2+bev), (W/2,T/2-bev),
             (W/2-bev,T/2), (-W/2+bev,T/2), (-W/2,T/2-bev), (-W/2,-T/2+bev)]
    rows=[]; coords=[]; M=board_matrix(loc,rot)
    twist=rng.uniform(-0.045,0.045); cup=rng.uniform(-0.012,0.012); bow=rng.uniform(-0.020,0.020)
    for i in range(cuts+1):
        u=i/cuts; x=(u-.5)*L
        end_jag=(rng.uniform(-0.035,0.020) if i in (0,cuts) else 0.0)
        row=[]; cr=[]
        for y,z in section:
            yy=y*(1+.035*noise.noise(Vector((u*4,seed,0))))
            zz=z + cup*(1-(2*y/max(W,1e-4))**2)*math.sin(math.pi*u) + bow*math.sin(math.pi*u)
            # worn/splintered arrises: edges undulate instead of staying knife-straight
            edge=1.0 if abs(y)>W*.42 or abs(z)>T*.42 else .35
            zz += edge*rough*noise.noise(Vector((u*18,y*50,seed)))
            yy += edge*rough*noise.noise(Vector((u*16,z*60,seed+3)))
            p=Vector((x+end_jag,yy,zz))
            q=Matrix.Rotation((u-.5)*twist,4,"X")@p
            row.append(tuple(M@q)); cr.append((yy,zz,x))
        rows.append(row); coords.append(cr)
    obj=kit.loft(n,rows,material=mat,coords=coords,cap_start=True,cap_end=True)
    # long checks as shallow dark V grooves in geometry
    if L>.25:
        kit.roughen(obj,strength=rough*.7,scale=45,seed=seed,subdivide=0)
    return obj

def rect_beam(kit,n,points,w,d,mat,seed=0,roll=0,cuts=1):
    rng=random.Random(seed); pts=[Vector(p) for p in points]; dense=[]
    for a,b in zip(pts,pts[1:]):
        for i in range(max(1,cuts)): dense.append(a.lerp(b,i/max(1,cuts)))
    dense.append(pts[-1]); rows=[]; coords=[]
    for i,p in enumerate(dense):
        t,s,u=frame(dense[min(len(dense)-1,i+1)]-dense[max(0,i-1)],roll+.02*math.sin(i+seed))
        ww=w*(1+rng.uniform(-.025,.025)); dd=d*(1+rng.uniform(-.025,.025))
        rows.append([tuple(p-s*ww/2-u*dd/2),tuple(p+s*ww/2-u*dd/2),tuple(p+s*ww/2+u*dd/2),tuple(p-s*ww/2+u*dd/2)])
        coords.append([(-ww/2,-dd/2,i*.2),(ww/2,-dd/2,i*.2),(ww/2,dd/2,i*.2),(-ww/2,dd/2,i*.2)])
    obj=kit.loft(n,rows,material=mat,coords=coords,cap_start=True,cap_end=True); kit.roughen(obj,strength=.0015,scale=35,seed=seed,subdivide=0); return obj

def nail(kit,n,loc,normal,mat,r=.005,l=.025):
    nn=Vector(normal).normalized(); obj=kit.cylinder(n,r,l,location=tuple(Vector(loc)+nn*l/2),material=mat,sides=6,bevel=.0007); obj.rotation_euler=nn.to_track_quat("Z","Y").to_euler(); return obj

def extruded_poly(kit,n,poly,thick,mat,matx=None,seed=0):
    verts=[]; coords=[]
    for z in (0,thick):
        for x,y in poly:
            v=Vector((x,y,z)); verts.append(tuple(matx@v if matx else v)); coords.append(Vector((x,y,z+seed)))
    L=len(poly); faces=[tuple(range(L-1,-1,-1)),tuple(range(L,2*L))]
    for i in range(L): faces.append((i,(i+1)%L,L+(i+1)%L,L+i))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def shard(kit,n,loc,yaw,sx,sy,mat,seed,thick=.005,pitch=0,roll=0,blue=False,blue_mat=None):
    rng=random.Random(seed); sides=rng.choice((4,5,6,7)); pts=[]
    for i in range(sides):
        a=math.tau*i/sides+rng.uniform(-.22,.22); pts.append((math.cos(a)*sx*rng.uniform(.45,1),math.sin(a)*sy*rng.uniform(.45,1)))
    pts.sort(key=lambda p:math.atan2(p[1],p[0])); M=mtx(loc,yaw,pitch,roll); out=[extruded_poly(kit,n,pts,thick,mat,M,seed)]
    if blue and blue_mat:
        for j in range(rng.randint(1,3)):
            y=rng.uniform(-sy*.45,sy*.45); w=rng.uniform(.0025,.0045); strip=[(-sx*.75,y-w),(sx*.75,y-w*.7),(sx*.75,y+w*.7),(-sx*.75,y+w)]
            out.append(extruded_poly(kit,f"{n}_BlueLine{j}",strip,.0005,blue_mat,M@Matrix.Translation(Vector((0,0,thick+.0007))),seed+j))
    return out

def slate_piece(kit,n,loc,yaw,w,l,mat,seed,pitch=0,roll=0,whole=False):
    rng=random.Random(seed)
    pts=[(-w/2,-l/2),(w/2,-l/2+rng.uniform(-.01,.01)),(w*.48,l/2),(-w*.48,l/2+rng.uniform(-.01,.01))] if whole else []
    if not whole:
        for i in range(rng.choice((4,4,5,6))):
            a=math.tau*i/rng.choice((4,5,6))+rng.uniform(-.18,.18); pts.append((math.cos(a)*w*rng.uniform(.34,.55),math.sin(a)*l*rng.uniform(.34,.55)))
        pts.sort(key=lambda p:math.atan2(p[1],p[0]))
    detailed=[]
    for a,b in zip(pts,pts[1:]+pts[:1]):
        va=Vector((a[0],a[1],0)); vb=Vector((b[0],b[1],0)); e=vb-va; nn=Vector((-e.y,e.x,0)).normalized() if e.length>1e-6 else Vector((0,1,0))
        for i in range(rng.randint(2,5)): detailed.append(tuple((va.lerp(vb,i/4)+nn*rng.uniform(-.004,.004))[:2]))
    return extruded_poly(kit,n,detailed,rng.uniform(.007,.012),mat,mtx(loc,yaw,pitch,roll),seed)

def roof_slate_rect(kit,n,center,length,width,mat,seed,slope=-0.78,askew=0.0):
    rng=random.Random(seed)
    d=Vector((1,0,slope)).normalized()
    side=Vector((0,1,0))
    if askew:
        q=Matrix.Rotation(askew,4,Vector((0,0,1)))
        side=q@side
    normal=d.cross(side).normalized()
    L=length*rng.uniform(.96,1.04); W=width*rng.uniform(.94,1.05); T=rng.uniform(.008,.010)
    c=Vector(center)+normal*0.045
    verts=[]; coords=[]
    for nz in (-.5,.5):
        for sx,sy in [(-.5,-.5),(.5,-.5),(.5,.5),(-.5,.5)]:
            chip=Vector((rng.uniform(-.006,.006),rng.uniform(-.004,.004),0))
            p=c+d*(sx*L)+side*(sy*W)+normal*(nz*T)+chip
            verts.append(tuple(p)); coords.append(Vector((sx*L,sy*W,nz*T)))
    faces=[(3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def loose_stave(kit,n,loc,yaw,length,mat,seed,curl=.0):
    rng=random.Random(seed); width=rng.uniform(.055,.085); M=mtx(loc,yaw,math.radians(rng.uniform(-3,3)),0); verts=[]; faces=[]; coords=[]
    for ix in range(7):
        u=ix/6; x=(u-.5)*length
        for iy in range(3):
            v=iy/2-.5; y=v*width; z=.012+curl*math.sin(math.pi*u)*(1-abs(v)*.5)+.004*noise.noise(Vector((u*8,v*4,seed)))
            verts.append(tuple(M@Vector((x,y,z)))); coords.append(Vector((y,0,x)))
    def idx(ix,iy): return ix*3+iy
    for ix in range(6):
        for iy in range(2): faces.append((idx(ix,iy),idx(ix+1,iy),idx(ix+1,iy+1),idx(ix,iy+1)))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); kit.roughen(obj,strength=.0018,scale=42,seed=seed,subdivide=0); return kit.recalc_normals(obj)

def bottle_along(kit,n,start,end,mat,seed=0,scale=1):
    start,end=Vector(start),Vector(end); axis=end-start; t,s,u=frame(axis); rows=[]; coords=[]
    prof=[(0,.026),(.06,.034),(.18,.041),(.56,.040),(.66,.030),(.78,.016),(.94,.016),(1,.021)]
    for q,r0 in prof:
        c=start.lerp(end,q); row=[]; cr=[]
        for i in range(24):
            a=math.tau*i/24; r=(r0+.0012*noise.noise(Vector((math.cos(a)*4+seed,math.sin(a)*4,q*5))))*scale
            row.append(tuple(c+s*math.cos(a)*r+u*math.sin(a)*r)); cr.append((math.cos(a)*r,math.sin(a)*r,axis.length*q))
        rows.append(row); coords.append(cr)
    return kit.loft(n,rows,material=mat,coords=coords,cap_start=True,cap_end=True)

def cloth(kit,n,loc,yaw,sx,sy,mat,seed,pitch=0,roll=0):
    rng=random.Random(seed); cols=5; rows=4; M=mtx(loc,yaw,pitch,roll); verts=[]; faces=[]; coords=[]
    for iy in range(rows+1):
        y=(iy/rows-.5)*sy
        for ix in range(cols+1):
            x=(ix/cols-.5)*sx; z=.002*math.sin(ix*1.7+seed)+.003*math.sin(iy*2.1); v=M@Vector((x+rng.uniform(-.006,.006),y+rng.uniform(-.006,.006),z))
            verts.append(tuple(v)); coords.append(Vector((x,y,z)))
    for iy in range(rows):
        for ix in range(cols):
            a=iy*(cols+1)+ix; faces.append((a,a+1,a+cols+2,a+cols+1))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def midden_ground(kit,mat):
    cols,rows=(72,52) if DRAFT else (132,96); verts=[]; faces=[]; coords=[]
    def outline_radius(x,y):
        a=math.atan2(y/.82,x/1.12)
        return 1.0+0.11*noise.noise(Vector((math.cos(a)*2.9,math.sin(a)*2.9,4.0)))+0.045*math.sin(5*a+0.7)+0.025*math.sin(9*a-1.2)
    def mound_z(x,y):
        edge=outline_radius(x,y)
        rr=math.sqrt((x/(1.12*edge))**2+(y/(.82*edge))**2)
        ell=(x/1.08)**2+(y/.78)**2
        z=(.19*math.exp(-1.70*ell)+
           .110*math.exp(-7.5*((x+.42)**2+(y-.16)**2))+
           .090*math.exp(-9.0*((x-.34)**2+(y+.18)**2))+
           .052*math.exp(-14.0*((x-.02)**2+(y-.34)**2))+
           .018*noise.noise(Vector((x*8+4,y*8,.7)))+
           .010*noise.noise(Vector((x*18-2,y*18+1,3.1)))-.004)
        # Feather to zero before the broken outline so there is no raised skirt edge.
        return max(0.0,z)*smoothstep(1.00,.58,rr)
    for iy in range(rows+1):
        y=-.83+1.66*iy/rows
        for ix in range(cols+1):
            x=-1.12+2.24*ix/cols
            z=mound_z(x,y)
            verts.append((x,y,z)); coords.append(Vector((x,y,z)))
    def idx(ix,iy): return iy*(cols+1)+ix
    for iy in range(rows):
        for ix in range(cols):
            cx=-1.12+2.24*(ix+.5)/cols; cy=-.83+1.66*(iy+.5)/rows
            edge=outline_radius(cx,cy)
            rr=math.sqrt((cx/(1.12*edge))**2+(cy/(.82*edge))**2)
            if rr<.68:
                faces.append((idx(ix,iy),idx(ix+1,iy),idx(ix+1,iy+1),idx(ix,iy+1)))
    seg=96 if DRAFT else 192
    ring_rows=[]
    for rr in (.64,.74,.84,.93,1.0):
        row=[]
        for j in range(seg):
            a=math.tau*j/seg
            edge=1.0+0.11*noise.noise(Vector((math.cos(a)*2.9,math.sin(a)*2.9,4.0)))+0.045*math.sin(5*a+0.7)+0.025*math.sin(9*a-1.2)
            x=math.cos(a)*1.08*rr*edge; y=math.sin(a)*.80*rr*edge
            z=mound_z(x,y)
            row.append(len(verts)); verts.append((x,y,z)); coords.append(Vector((x,y,z)))
        ring_rows.append(row)
    for ri in range(len(ring_rows)-1):
        arow,brow=ring_rows[ri],ring_rows[ri+1]
        for j in range(seg):
            faces.append((arow[j],brow[j],brow[(j+1)%seg],arow[(j+1)%seg]))
    obj=kit.mesh("AshSoilMiddenMound",verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def bucket_shell(kit,n,loc,yaw,mat,dark,seed=0):
    rng=random.Random(seed); sides=40; M=mtx(loc,yaw,math.radians(86),math.radians(-8)); verts=[]; faces=[]; coords=[]
    for layer,inset in enumerate((0,-.006)):
        for z,r0 in ((0,.09),(.22,.125)):
            for i in range(sides):
                a=math.tau*i/sides; r=r0+inset+.002*noise.noise(Vector((math.cos(a)*3+seed,math.sin(a)*3,z*5))); v=M@Vector((math.cos(a)*r,math.sin(a)*r,z))
                verts.append(tuple(v)); coords.append(Vector((math.cos(a)*r,math.sin(a)*r,z)))
    def idx(layer,j,i): return layer*2*sides+j*sides+(i%sides)
    for layer in (0,1):
        for i in range(sides):
            ang=math.atan2(math.sin(math.tau*i/sides-.2),math.cos(math.tau*i/sides-.2))
            if layer==0 and abs(ang)<.34 and i%3!=0: continue
            faces.append((idx(layer,0,i),idx(layer,0,i+1),idx(layer,1,i+1),idx(layer,1,i)) if layer==0 else (idx(layer,0,i+1),idx(layer,0,i),idx(layer,1,i),idx(layer,1,i+1)))
    for i in range(sides): faces.append((idx(1,0,i+1),idx(0,0,i+1),idx(0,0,i),idx(1,0,i)))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); parts=[kit.recalc_normals(obj)]
    for j in range(10):
        a=.2+rng.uniform(-.36,.36); z=rng.uniform(.07,.16); p=M@Vector((math.cos(a)*.128,math.sin(a)*.128,z))
        parts.append(kit.sphere(f"{n}_RustHoleEdge_{j}",rng.uniform(.006,.014),location=tuple(p),material=dark,segments=8,rings=4,scale=(1,.55,.2)))
    pts=[M@Vector((0,-.128,.17)),M@Vector((.045,0,.095)),M@Vector((0,.128,.16))]; parts.append(kit.tube(n+"_BentBail",pts,radius=.004,sides=8,material=mat)); return parts

def mat_white(kit):
    g=kit.mats.Graph("M_EstateDebris_CrazedWhiteware"); p=g.coord(); x,y,z=g.separate(p)
    cloud=g.noise(p,scale=36,detail=6,roughness=.62).outputs["Fac"]; edge=g.voronoi(g.combine(x,y,g.math("MULTIPLY",z,.2)),scale=92,feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack=g.remap(edge,0,.012,1,0); base=g.mix((.52,.50,.45),(.33,.31,.27),g.remap(cloud,.45,.85,0,.35)); base=g.mix(base,(.035,.030,.024),crack)
    grime=g.noise(p,scale=18,detail=5,roughness=.7).outputs["Fac"]
    base=g.mix(base,(.070,.060,.045),g.remap(grime,.45,.78,0,.55))
    base=g.mix(base,(.40,.38,.33),.18,blend="MULTIPLY")
    g.set("Base Color",base); g.set("Roughness",.78); g.set("Normal",g.bump(g.math("MULTIPLY",crack,-.55),strength=.2,distance=.0008)); return g.mat

def mat_blue(kit):
    g=kit.mats.Graph("M_EstateDebris_DullCobaltTransfer"); p=g.coord(); m=g.noise(p,scale=42,detail=5,roughness=.65).outputs["Fac"]
    g.set("Base Color",g.ramp(m,[(.1,(.010,.018,.045)),(.68,(.025,.045,.095)),(1,(.050,.070,.125))])); g.set("Roughness",.82); g.set("Normal",g.bump(m,strength=.1,distance=.0004)); return g.mat

def mat_rag(kit):
    g=kit.mats.Graph("M_EstateDebris_RottingRag"); p=g.coord(); f=g.noise(p,scale=65,detail=6).outputs["Fac"]; s=g.noise(p,scale=12,detail=5).outputs["Fac"]
    c=g.ramp(f,[(.2,(.060,.055,.047)),(.7,(.150,.135,.112)),(1,(.21,.19,.16))]); c=g.mix(c,(.035,.030,.024),g.remap(s,.52,.82,0,.65))
    g.set("Base Color",c); g.set("Roughness",.97); g.set("Normal",g.bump(f,strength=.28,distance=.0009)); return g.mat

def mat_midden_soil(kit):
    g=kit.mats.Graph("M_EstateDebris_BrownGreyMiddenSoil")
    p=g.coord(); x,y,z=g.separate(p)
    rx=g.math("DIVIDE",x,1.10); ry=g.math("DIVIDE",y,.82)
    r=g.vmath("LENGTH",g.combine(rx,ry,0.0))
    ash=g.noise(g.combine(g.math("MULTIPLY",x,7.0),g.math("MULTIPLY",y,7.0),g.math("MULTIPLY",z,2.0)),scale=10.0,detail=6.0,roughness=.66).outputs["Fac"]
    cinder=g.noise(g.combine(g.math("MULTIPLY",x,16.0),g.math("MULTIPLY",y,16.0),0.0),scale=20.0,detail=3.0).outputs["Fac"]
    base=g.ramp(ash,[(.18,(.042,.036,.030)),(.58,(.070,.061,.048)),(1.0,(.095,.080,.060))])
    base=g.mix(base,(.026,.025,.023),g.remap(cinder,.72,.90,0.0,.40))
    edge=g.remap(r,.58,.98,0.0,.72)
    moss=g.ramp(g.noise(p,scale=18.0,detail=5.0).outputs["Fac"],[(.2,(.040,.042,.031)),(.8,(.056,.058,.041))])
    base=g.mix(base,moss,edge)
    g.set("Base Color",base); g.set("Roughness",.98)
    height=g.math("ADD",g.math("MULTIPLY",ash,.35),g.math("MULTIPLY",cinder,.18))
    g.set("Normal",g.bump(height,strength=.34,distance=.0012)); return g.mat

def mat_weathered_wood(kit,name,light,dark,seed=0,lichen=.25):
    g=kit.mats.Graph(name); p=g.coord(); x,y,z=g.separate(p)
    stretched=g.combine(g.math("MULTIPLY",x,28.0),g.math("MULTIPLY",y,42.0),g.math("MULTIPLY",z,1.35))
    fibre=g.noise(stretched,scale=9.0,detail=10.0,roughness=.60).outputs["Fac"]
    streak=g.noise(g.combine(g.math("MULTIPLY",x,5.0),g.math("MULTIPLY",y,8.0),g.math("MULTIPLY",z,.45)),scale=5.0,detail=5.0,roughness=.62).outputs["Fac"]
    checks=g.voronoi(g.combine(g.math("MULTIPLY",x,.45),g.math("MULTIPLY",y,.80),g.math("MULTIPLY",z,.10)),scale=42.0,feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack=g.remap(checks,0.0,.014,.75,0.0)
    tone=g.math("ADD",g.math("MULTIPLY",fibre,.72),g.math("MULTIPLY",streak,.28))
    col=g.ramp(tone,[(.18,dark),(.55,light),(.92,tuple(min(.32,c*1.18) for c in light))])
    col=g.mix(col,(.022,.019,.015),crack)
    rot=g.noise(g.combine(g.math("MULTIPLY",x,9.0),g.math("MULTIPLY",y,9.0),g.math("MULTIPLY",z,.8)),scale=12.0,detail=5.0).outputs["Fac"]
    col=g.mix(col,(.030,.046,.032),g.remap(rot,.60,.78,0.0,.42))
    if lichen:
        crust=g.noise(g.combine(g.math("MULTIPLY",x,2.2),g.math("MULTIPLY",y,2.2),g.math("MULTIPLY",z,.22)),scale=17.0,detail=6.0).outputs["Fac"]
        col=g.mix(col,(.105,.125,.082),g.remap(crust,.63,.71,0.0,lichen))
    g.set("Base Color",col); g.set("Roughness",.94)
    height=g.math("SUBTRACT",g.math("ADD",g.math("MULTIPLY",fibre,.55),g.math("MULTIPLY",streak,.15)),g.math("MULTIPLY",crack,.75))
    g.set("Normal",g.bump(height,strength=.55,distance=.0018)); return g.mat

def make_materials(kit):
    m=kit.mats; d={}
    d["barrel_wood"]=mat_weathered_wood(kit,"M_EstateDebris_SilverRottenOak",(.170,.160,.132),(.046,.040,.032),10,.33)
    d["barrel_head"]=mat_weathered_wood(kit,"M_EstateDebris_GreyHeadBoards",(.180,.168,.138),(.048,.040,.032),11,.24)
    d["crate_wood"]=mat_weathered_wood(kit,"M_EstateDebris_RottenDealBoards",(.175,.158,.120),(.042,.036,.027),12,.30)
    d["lean_timber"]=mat_weathered_wood(kit,"M_EstateDebris_SilveredLeanToTimber",(.162,.154,.130),(.040,.035,.028),13,.38)
    d["rust_iron"]=m.wrought_iron("M_EstateDebris_RustyWroughtIron",rust=.88,wear=.08,seed=14)
    d["midden_soil"]=mat_midden_soil(kit)
    d["slate"]=m.slate("M_EstateDebris_DelaboleSlate",dark=(.036,.044,.053),light=(.095,.112,.126),lichen=.04,moss=.02,seed=16)
    d["slate_dark"]=m.slate("M_EstateDebris_WetDarkSlate",dark=(.024,.031,.040),light=(.070,.086,.100),lichen=.025,moss=.015,seed=17)
    d["ash_dark"]=kit.material("M_EstateDebris_ColdAshCinders",(.040,.035,.030),roughness=.98)
    d["dark_gap"]=kit.material("M_EstateDebris_DarkInteriorGaps",(.010,.008,.006),roughness=.99)
    d["whiteware"]=mat_white(kit); d["blue_transfer"]=mat_blue(kit)
    d["brown_stoneware"]=kit.material("M_EstateDebris_BrownSaltGlazeStoneware",(.155,.083,.037),roughness=.64)
    d["green_glass"]=kit.material("M_EstateDebris_DarkOliveBottleGlass",(.025,.055,.030),roughness=.38)
    d["brown_glass"]=kit.material("M_EstateDebris_BrownBottleGlass",(.075,.038,.018),roughness=.38)
    d["boot_leather"]=m.leather("M_EstateDebris_RottedBootLeather",color=(.145,.075,.036),dark=(.030,.020,.014),roughness=.92,creases=1.4,soil=0.8,stains=0.25,seed=18)
    d["rag"]=mat_rag(kit); zero(*d.values()); return d

def barrel_radius(t): return .255+.075*math.sin(math.pi*t)**.9

def barrel_stave(kit,n,i,a0,a1,mat,seed):
    rng=random.Random(seed); xs=[-.45,-.30,-.12,.10,.29,.45]; angles=[a0,(a0+a1)/2,a1]; verts=[]; faces=[]; coords=[]; zc=.330; thick=.025
    for x in xs:
        t=(x+.45)/.90
        for radial in (0,1):
            for a in angles:
                r=barrel_radius(t)-thick*radial+rng.uniform(-.0015,.0015); jag=rng.uniform(-.025,.018) if x>.30 and radial==0 else 0
                s=math.sin(a); c=math.cos(a); ss=(-.48+(s+.48)*.32) if s<-.48 else s
                verts.append((x+(rng.uniform(-.006,.006) if x>.32 else 0),c*r,zc+ss*r+jag)); coords.append(Vector((c*r,ss*r,x)))
    def idx(ix,rad,ia): return ix*6+rad*3+ia
    for ix in range(len(xs)-1):
        for ia in (0,1):
            faces.append((idx(ix,0,ia),idx(ix,0,ia+1),idx(ix+1,0,ia+1),idx(ix+1,0,ia)))
            faces.append((idx(ix,1,ia+1),idx(ix,1,ia),idx(ix+1,1,ia),idx(ix+1,1,ia+1)))
        for ia in (0,2): faces.append((idx(ix,1,ia),idx(ix,0,ia),idx(ix+1,0,ia),idx(ix+1,1,ia)))
    for ix in (0,len(xs)-1):
        for ia in (0,1): faces.append((idx(ix,1,ia),idx(ix,1,ia+1),idx(ix,0,ia+1),idx(ix,0,ia)))
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); kit.roughen(obj,strength=.002,scale=32,seed=seed,subdivide=1); return kit.recalc_normals(obj)

def barrel_head(kit,n,x,mat,dark):
    parts=[]; r=.245; zc=.330
    for j,y in enumerate([-.198,-.118,-.039,.040,.120,.199]):
        w=.065; chord=2*math.sqrt(max(0,r*r-(y+w*.5)**2))*.98
        parts.append(board(kit,f"{n}_Board_{j}",(.026,w,chord),(x,y,zc),mat,seed=SEED+700+j,rough=.0028))
        if j<5: parts.append(board(kit,f"{n}_DarkGap_{j}",(.030,.006,chord*.94),(x+.001,y+w*.55,zc),dark,seed=SEED+730+j,rough=0,bevel=0))
    return parts

def hoop_x(kit,n,x,radius,width,thick,a0,a1,mat,seed=0,zc=.330,sides=48):
    steps=max(8,int(abs(a1-a0)/math.tau*sides)); verts=[]; faces=[]; coords=[]
    for xi in (x-width/2,x+width/2):
        for rr in (radius-thick/2,radius+thick/2):
            for j in range(steps+1):
                a=a0+(a1-a0)*j/steps; r=rr+.002*noise.noise(Vector((math.cos(a)*3+seed,math.sin(a)*3,x)))
                verts.append((xi,math.cos(a)*r,zc+math.sin(a)*r)); coords.append(Vector((math.cos(a)*r,math.sin(a)*r,xi)))
    def idx(ix,ir,j): return ix*2*(steps+1)+ir*(steps+1)+j
    for j in range(steps):
        faces += [(idx(1,1,j),idx(1,1,j+1),idx(0,1,j+1),idx(0,1,j)),(idx(0,0,j+1),idx(1,0,j+1),idx(1,0,j),idx(0,0,j)),(idx(0,1,j),idx(0,1,j+1),idx(0,0,j+1),idx(0,0,j)),(idx(1,0,j),idx(1,0,j+1),idx(1,1,j+1),idx(1,1,j))]
    obj=kit.mesh(n,verts,faces,mat); kit.tag_coords(obj.data,coords); return kit.recalc_normals(obj)

def hoop_ground_ellipse(kit,n,loc,rx,ry,mat,seed=0):
    pts=[Vector((loc[0]+math.cos(math.tau*i/96)*rx, loc[1]+math.sin(math.tau*i/96)*ry, loc[2]+.005*math.sin(i*.37+seed))) for i in range(97)]
    obj=kit.tube(n,pts,radius=.0065,sides=8,material=mat)
    kit.roughen(obj,strength=.0012,scale=40,seed=seed,subdivide=0)
    return obj

def build_broken_barrel(kit,m):
    rng=random.Random(SEED+10); parts=[]; missing={2,3,4,23,24}
    for i in range(28):
        if i in missing: continue
        parts.append(barrel_stave(kit,f"BarrelStave_{i:02d}",i,-math.pi+math.tau*(i+.08)/28,-math.pi+math.tau*(i+.88)/28,m["barrel_wood"],SEED+20+i))
    parts.extend(barrel_head(kit,"RemainingHeadWithRottenCracks",-.455,m["barrel_head"],m["dark_gap"]))
    parts.append(hoop_x(kit,"DarkOpenInteriorRim",.455,.245,.010,.006,-math.pi*.96,math.pi*.96,m["dark_gap"],SEED+88,sides=40))
    for j,x in enumerate((-.36,-.18,.13)):
        parts.append(hoop_x(kit,f"RustedHoopStillOn_{j}",x,barrel_radius((x+.45)/.90)+.018,.035,.012,-math.pi*.92,math.pi*.86,m["rust_iron"],SEED+100+j,sides=60))
    parts.append(hoop_ground_ellipse(kit,"SlippedWholeFlattenedHoopOnGround",(.44,-.22,.024),.39,.27,m["rust_iron"],SEED+130))
    for k,(x,y,z,yaw,L) in enumerate(((.52,-.28,.065,-18,.58),(.62,-.03,.045,7,.54),(.50,.28,.050,22,.50),(.18,.32,.03,42,.55))):
        parts.append(loose_stave(kit,f"SprungStave_{k}",(x,y,z),math.radians(yaw),L,m["barrel_wood"],SEED+150+k,curl=.025))
    parts.append(loose_stave(kit,"LooseStaveNearby",(-.34,-.34,.030),math.radians(-26),.58,m["barrel_wood"],SEED+170,curl=.012))
    for k in range(34 if not DRAFT else 14):
        x=rng.uniform(-.47,.64); y=rng.uniform(-.52,.52)
        if rng.random()<.55: parts.append(loose_stave(kit,f"BarrelWoodSplinter_{k}",(x,y,rng.uniform(.008,.030)),rng.uniform(-math.pi,math.pi),rng.uniform(.05,.18),m["barrel_wood"],SEED+190+k,curl=.002))
        else: parts.append(kit.sphere(f"BarrelRustFlake_{k}",rng.uniform(.006,.014),location=(x,y,rng.uniform(.004,.020)),material=m["rust_iron"],segments=8,rings=4,scale=(1,.7,.22)))
    settle(parts); return kit.join(parts,"SM_BrokenBarrel",unwrap=True,reshade=True,smooth_angle=48)

def build_broken_crate(kit,m):
    rng=random.Random(SEED+400); parts=[]; deal=m["crate_wood"]
    for i,y in enumerate((-.22,-.06,.105,.255)):
        parts.append(board(kit,f"CrateFloorSlat_{i}",(.78,.050,.026),(.02+.04*i,y,.035+.006*i),deal,rot=(rng.uniform(-2,2),rng.uniform(-8,8),rng.uniform(-7,7)),rough=.0025,seed=SEED+410+i))
    for i,z in enumerate((.13,.255,.38)):
        parts.append(board(kit,f"CrateBackSlatCollapsed_{i}",(.78,.034,.095),(0,.305,z),deal,rot=(math.degrees(-.65),0,rng.uniform(-6,4)),rough=.0028,seed=SEED+430+i))
    for i,z in enumerate((.14,.29,.44)):
        parts.append(board(kit,f"CrateLeftSideBoard_{i}",(.034,.52,.090),(-.39,.02,z),deal,rot=(rng.uniform(-5,8),math.degrees(.25),rng.uniform(-4,5)),rough=.0025,seed=SEED+450+i))
        if i!=1: parts.append(board(kit,f"CrateRightSideBoard_{i}",(.034,.44,.084),(.39,0,z-.025*i),deal,rot=(rng.uniform(-9,4),math.degrees(-.42),rng.uniform(-5,4)),rough=.0025,seed=SEED+470+i))
    parts += [board(kit,"OffBoardLongOne",(.68,.060,.024),(-.10,-.46,.030),deal,rot=(1.5,-2,-15),rough=.003,seed=SEED+500), board(kit,"OffBoardSplitTwo",(.54,.055,.022),(.44,-.35,.035),deal,rot=(-2,4,24),rough=.003,seed=SEED+501)]
    for i,(x,y,h,rx,ry) in enumerate(((-.38,.30,.46,12,18),(.37,.28,.32,-18,-24),(-.38,-.26,.25,62,6),(.36,-.24,.18,72,-10))):
        parts.append(board(kit,f"RottenCornerBatten_{i}",(.046,.046,h),(x,y,h/2+.02),deal,rot=(rx,ry,rng.uniform(-8,8)),rough=.0025,seed=SEED+520+i))
    for i,loc in enumerate(((-.31,-.475,.055),(.13,-.455,.055),(.50,-.34,.060),(-.40,.18,.31),(.39,.16,.22))): parts.append(nail(kit,f"ProtrudingRustyNail_{i}",loc,(.15,-.25,.96),m["rust_iron"],r=.004,l=.028))
    for k in range(22 if not DRAFT else 8): parts.append(loose_stave(kit,f"CrateSplinter_{k}",(rng.uniform(-.50,.55),rng.uniform(-.48,.34),rng.uniform(.006,.035)),rng.uniform(-math.pi,math.pi),rng.uniform(.045,.16),deal,SEED+550+k,curl=.002))
    settle(parts); return kit.join(parts,"SM_BrokenCrate",unwrap=True,reshade=True,smooth_angle=45)

def build_rubbish_heap(kit,m):
    rng=random.Random(SEED+800); parts=[midden_ground(kit,m["midden_soil"])]
    def h_at(x,y):
        ell=(x/1.08)**2+(y/.78)**2
        return max(0.0,(.20*math.exp(-1.75*ell)+.115*math.exp(-7.5*((x+.42)**2+(y-.16)**2))+.090*math.exp(-9*((x-.34)**2+(y+.18)**2))+.055*math.exp(-14*((x-.02)**2+(y-.34)**2))-.006)*smoothstep(1.05,.72,min(1.05,math.sqrt(max(ell,0)))))
    for i in range(46 if not DRAFT else 20):
        x=rng.uniform(-1.02,1.02); y=rng.uniform(-.72,.72)
        if (x/1.05)**2+(y/.78)**2>rng.uniform(.55,1.15): continue
        mat=m["ash_dark"] if rng.random()<.62 else m["midden_soil"]
        parts.append(kit.sphere(f"CinderClod_{i:02d}",rng.uniform(.010,.030),location=(x,y,h_at(x,y)+rng.uniform(.002,.020)),material=mat,segments=8,rings=4,scale=(1,rng.uniform(.6,1.1),rng.uniform(.16,.36))))
    for i in range(22 if not DRAFT else 12):
        x=rng.uniform(-.98,.98); y=rng.uniform(-.68,.68); z=h_at(x,y)+rng.uniform(.004,.026)
        is_blue=rng.random()<.62; parts.extend(shard(kit,f"CrockeryShard_{i:02d}",(x,y,z),rng.uniform(-math.pi,math.pi),rng.uniform(.035,.105),rng.uniform(.025,.070),m["whiteware"] if is_blue else m["brown_stoneware"],SEED+850+i,thick=rng.uniform(.003,.007),pitch=rng.uniform(-.45,.45),roll=rng.uniform(-.30,.30),blue=is_blue,blue_mat=m["blue_transfer"]))
    for i in range(10 if not DRAFT else 5):
        x=rng.uniform(-.95,.95); y=rng.uniform(-.65,.65); parts.extend(shard(kit,f"GlassShard_{i:02d}",(x,y,h_at(x,y)+rng.uniform(.004,.020)),rng.uniform(-math.pi,math.pi),rng.uniform(.025,.075),rng.uniform(.018,.060),m["green_glass"] if rng.random()<.70 else m["brown_glass"],SEED+910+i,thick=rng.uniform(.002,.005),pitch=rng.uniform(-.25,.25),roll=rng.uniform(-.20,.20)))
    parts.append(bottle_along(kit,"WholeBottleGreenHalfBuried",(-.62,.25,h_at(-.62,.25)+.035),(-.29,.34,h_at(-.29,.34)+.055),m["green_glass"],SEED+940,1.08))
    parts.append(bottle_along(kit,"BrokenBrownBottleNeck",(.40,-.33,h_at(.40,-.33)+.025),(.58,-.21,h_at(.58,-.21)+.050),m["brown_glass"],SEED+941,.75))
    parts.extend(bucket_shell(kit,"RustedBucketWithHole",(.58,.30,h_at(.58,.30)+.020),math.radians(-23),m["rust_iron"],m["ash_dark"],SEED+960))
    parts.append(kit.sphere("RottedBootSole",.13,location=(-.34,-.34,.115),material=m["boot_leather"],segments=18,rings=8,scale=(1.75,.62,.22)))
    parts.append(kit.sphere("RottedBootUpperCollapsed",.11,location=(-.26,-.31,.185),material=m["boot_leather"],segments=18,rings=8,scale=(1.05,.55,.50)))
    parts.append(kit.sphere("DarkBootOpening",.055,location=(-.20,-.31,.205),material=m["ash_dark"],segments=12,rings=6,scale=(1.2,.8,.25)))
    for i in range(2): 
        x=rng.uniform(-.45,.55); y=rng.uniform(-.45,.45)
        parts.append(cloth(kit,f"RottenRag_{i}",(x,y,h_at(x,y)+.018),rng.uniform(-math.pi,math.pi),rng.uniform(.18,.34),rng.uniform(.10,.22),m["rag"],SEED+980+i,pitch=rng.uniform(-.12,.12),roll=rng.uniform(-.1,.1)))
    for i in range(3):
        x=rng.uniform(-.75,.75); y=rng.uniform(-.55,.55)
        parts.append(board(kit,f"MiddenBrokenBoard_{i}",(rng.uniform(.32,.66),rng.uniform(.045,.070),rng.uniform(.018,.026)),(x,y,h_at(x,y)+.025),m["crate_wood"],rot=(rng.uniform(-5,5),rng.uniform(-6,6),rng.uniform(-75,75)),rough=.003,seed=SEED+1000+i))
    settle(parts); return kit.join(parts,"SM_RubbishHeap",unwrap=True,reshade=True,smooth_angle=50)

def build_collapsed_leanto(kit,m):
    rng=random.Random(SEED+1200); parts=[]; timber=m["lean_timber"]; darkwood=m["barrel_wood"]; slates=(m["slate"],m["slate_dark"])
    roof_z=lambda x: 1.76 + (x+1.10)*(-0.78)
    parts.append(rect_beam(kit,"BackWallplateLedgerPinnedToWall",[(-1.10,-1.66,1.78),(-1.10,1.66,1.75)],.10,.11,timber,SEED+1210,roll=.08,cuts=8))
    parts.append(rect_beam(kit,"FrontFallenSill",[(1.02,-1.62,.07),(1.07,1.62,.06)],.08,.09,timber,SEED+1211,cuts=8))
    for i,y in enumerate((-1.18,1.18)):
        parts.append(rect_beam(kit,f"SnappedFrontPost_{i}",[(.88,y,.04),(.86+rng.uniform(-.08,.06),y+rng.uniform(-.04,.04),.42),(.96+rng.uniform(-.06,.08),y+rng.uniform(-.06,.06),.68)],.10,.10,timber,SEED+1220+i,roll=rng.uniform(-.3,.3),cuts=4))
        parts.append(rect_beam(kit,f"FallenUpperFrontPost_{i}",[(.72,y+.10,.10),(.25,y+.18,.24),(-.18,y+.22,.38)],.085,.090,timber,SEED+1224+i,roll=rng.uniform(-.4,.4),cuts=4))
        for k in range(5 if not DRAFT else 2):
            base=Vector((.92+rng.uniform(-.04,.04),y+rng.uniform(-.04,.04),rng.uniform(.34,.68))); tip=base+Vector((rng.uniform(-.08,.10),rng.uniform(-.03,.03),rng.uniform(.10,.22)))
            parts.append(rect_beam(kit,f"Post_{i}_Splinter_{k}",[base,tip],rng.uniform(.012,.030),rng.uniform(.008,.020),timber,SEED+1230+i*10+k))
    for i,y in enumerate([-1.48,-.89,-.30,.30,.89,1.48]):
        front_z=rng.uniform(.06,.20); fx=rng.uniform(.88,1.16); mid=(-.15+rng.uniform(-.05,.08),y+rng.uniform(-.04,.04),rng.uniform(.72,1.0))
        parts.append(rect_beam(kit,f"DroppedRafter_{i}",[(-1.10,y,1.72+rng.uniform(-.035,.035)),mid,(fx,y+rng.uniform(-.08,.08),front_z)],.070,.050,timber,SEED+1250+i,roll=rng.uniform(-.25,.25),cuts=7))
    for j,u in enumerate((.20,.36,.52,.68)):
        x=-1.10+2.05*u; z=1.70*(1-u)+.10*u+rng.uniform(-.04,.04)
        parts.append(rect_beam(kit,f"BrokenCrossBattenPurlin_{j}",[(x,-1.60,z),(x+rng.uniform(-.05,.10),1.60,z+rng.uniform(-.04,.03))],.045,.055,timber,SEED+1270+j,roll=rng.uniform(-.5,.5),cuts=8))
    for i in range(10 if not DRAFT else 4):
        u=rng.uniform(.05,.95); y=rng.uniform(-1.55,1.55); x=-1.08+2*u+rng.uniform(-.04,.04); z=1.66*(1-u)+.10*u+rng.uniform(-.08,.08)
        parts.append(board(kit,f"TornWeatherBoard_{i}",(rng.uniform(.55,1.25),rng.uniform(.045,.075),.020),(x,y,z),darkwood,rot=(rng.uniform(-35,-10),rng.uniform(-8,8),rng.uniform(70,105)),rough=.003,seed=SEED+1290+i))
    # Ledged-and-braced door leaf lying low against the fallen foot, not a stray upright panel.
    for i,off in enumerate((-.20,-.10,0,.10,.20)):
        parts.append(board(kit,f"DoorLeafVerticalBoard_{i}",(1.05,.085,.026),(.58,-1.02+off,.095),darkwood,rot=(4,rng.uniform(-4,4),-7+rng.uniform(-2,2)),rough=.003,seed=SEED+1320+i))
    for j,x in enumerate((.24,.78)):
        parts.append(board(kit,f"DoorLeafLedger_{j}",(.060,.54,.030),(x,-1.02,.122),darkwood,rot=(4,0,-7),rough=.0025,seed=SEED+1327+j))
    parts.append(rect_beam(kit,"DoorLeafZBrace",[(.16,-1.25,.135),(.55,-1.03,.155),(.94,-.79,.145)],.045,.030,darkwood,SEED+1329,roll=.15,cuts=3))
    for j,x in enumerate((.23,.78)): parts.append(nail(kit,f"DoorRustyHingePin_{j}",(x,-1.31,.145),(0,-1,0),m["rust_iron"],r=.010,l=.030))
    # Legible rows of surviving slates on the upper half, then loose talus at the foot.
    slate_i=0
    for row,x in enumerate((-.86,-.63,-.40,-.17,.06)):
        for col,y in enumerate([-1.35,-.90,-.45,0,.45,.90,1.35]):
            if rng.random()<.18: continue
            z=roof_z(x)+.012*row
            parts.append(roof_slate_rect(kit,f"LeanToSlateCourse_{row}_{col}",(x+rng.uniform(-.015,.015),y+rng.uniform(-.018,.018),z),.50,.25,rng.choice(slates),SEED+1360+slate_i,slope=-.78,askew=rng.uniform(-.035,.035))); slate_i+=1
    for i in range(38 if not DRAFT else 16):
        x=rng.uniform(.50,.94); y=rng.uniform(-1.45,1.45); z=rng.uniform(.014,.12)+.07*max(0,1-(x-.72)**2*3)
        if rng.random()<.55:
            parts.append(roof_slate_rect(kit,f"LeanToSlidSlate_{i:03d}",(x,y,z),rng.uniform(.28,.42),rng.uniform(.18,.25),rng.choice(slates),SEED+1390+i,slope=rng.uniform(-.05,.12),askew=rng.uniform(-.65,.65)))
        else:
            parts.append(slate_piece(kit,f"LeanToSlate_{i:03d}",(x,y,z),rng.uniform(-math.pi,math.pi),rng.uniform(.16,.24),rng.uniform(.22,.40),rng.choice(slates),SEED+1430+i,pitch=rng.uniform(-.08,.16),roll=rng.uniform(-.12,.12),whole=rng.random()<.12))
    for i in range(38 if not DRAFT else 14): parts.append(kit.sphere(f"LeanToSoilSlateDust_{i}",rng.uniform(.010,.034),location=(rng.uniform(.20,1.15),rng.uniform(-1.45,1.45),rng.uniform(.004,.026)),material=m["midden_soil"],segments=8,rings=4,scale=(1,rng.uniform(.5,1.1),.18)))
    for i in range(14 if not DRAFT else 5): parts.append(nail(kit,f"LeanToScatteredRoofNail_{i}",(rng.uniform(-.75,1.08),rng.uniform(-1.45,1.45),rng.uniform(.012,.08)),(rng.uniform(-.5,.5),rng.uniform(-.5,.5),1),m["rust_iron"],r=rng.uniform(.003,.006),l=rng.uniform(.018,.040)))
    parts.append(cloth(kit,"TornSackingCaughtOnRafter",(.18,1.05,.45),math.radians(12),.42,.20,m["rag"],SEED+1450,pitch=math.radians(-28),roll=math.radians(4)))
    settle(parts); return kit.join(parts,"SM_CollapsedLeanTo",unwrap=True,reshade=True,smooth_angle=52)

def build(kit):
    m=make_materials(kit)
    builders={
        "SM_BrokenBarrel": lambda: build_broken_barrel(kit,m),
        "SM_BrokenCrate": lambda: build_broken_crate(kit,m),
        "SM_RubbishHeap": lambda: build_rubbish_heap(kit,m),
        "SM_CollapsedLeanTo": lambda: build_collapsed_leanto(kit,m),
    }
    only=[s.strip() for s in os.environ.get("HOMESTEAD_DEBRIS_ONLY","").split(",") if s.strip()]
    names=only or list(builders)
    return [builders[name]() for name in names]
