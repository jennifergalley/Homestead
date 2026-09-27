import math
import random
from mathutils import Vector, Matrix, noise


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def zero_subsurface(*materials):
    for mat in materials:
        if mat and mat.node_tree and "Principled BSDF" in mat.node_tree.nodes:
            bsdf = mat.node_tree.nodes["Principled BSDF"]
            if "Subsurface Weight" in bsdf.inputs:
                bsdf.inputs["Subsurface Weight"].default_value = 0.0


def board(kit, name, size, loc, mat, rotation=(0, 0, 0), bevel=0.004, rough=0.0015, seed=0):
    obj = kit.box(name, size, location=loc, rotation=rotation, material=mat, bevel=bevel, bevel_segments=2)
    sx, sy, sz = size
    # Wood materials read pcoord.z as grain length and pcoord.x/y as board cross-section.
    # Boxes default to world/object Z, which makes horizontal shelves show end-grain bullseyes.
    longest = max(range(3), key=lambda i: (sx, sy, sz)[i])
    coords = []
    for v in obj.data.vertices:
        x, y, z = v.co.x, v.co.y, v.co.z
        if longest == 0:
            coords.append((y, z, x))
        elif longest == 1:
            coords.append((x, z, y))
        else:
            coords.append((x, y, z))
    kit.tag_coords(obj.data, coords)
    if rough:
        kit.roughen(obj, strength=rough, scale=35.0, seed=seed, subdivide=0)
    return obj


def dark_gap(kit, name, size, loc):
    mat = kit.material("M_Store_ShadowGap", (0.012, 0.010, 0.008), roughness=0.95)
    return kit.box(name, size, location=loc, material=mat)


def ring_band(kit, name, radius, z, height, thickness, mat, sides=96, wobble=0.0, seed=0):
    rng = random.Random(seed)
    verts, faces, coords = [], [], []
    for k, rr in enumerate((radius - thickness * 0.5, radius + thickness * 0.5)):
        for zz in (z - height * 0.5, z + height * 0.5):
            for i in range(sides):
                a = 2 * math.pi * i / sides
                w = wobble * noise.noise(Vector((math.cos(a) * 2.1 + seed, math.sin(a) * 2.1, z * 3.0)))
                r = rr + w
                verts.append((r * math.cos(a), r * math.sin(a), zz))
                coords.append((math.cos(a) * rr, math.sin(a) * rr, zz))
    def idx(k, j, i): return k * 2 * sides + j * sides + (i % sides)
    for i in range(sides):
        faces.append((idx(1,0,i), idx(1,0,i+1), idx(1,1,i+1), idx(1,1,i)))  # outside
        faces.append((idx(0,0,i+1), idx(0,0,i), idx(0,1,i), idx(0,1,i+1)))  # inside
        faces.append((idx(0,1,i), idx(1,1,i), idx(1,1,i+1), idx(0,1,i+1)))  # top edge
        faces.append((idx(0,0,i+1), idx(1,0,i+1), idx(1,0,i), idx(0,0,i)))  # bottom edge
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def bulged_vessel_shell(kit, name, height, radius_fn, mat, sides=32, rings=24, thickness=0.012,
                        start_angle=0.0, end_angle=2*math.pi, cap_top=False, cap_bottom=False,
                        seed=0, gap=0.0):
    verts, faces, coords = [], [], []
    rng = random.Random(seed)
    steps = sides if abs(end_angle - start_angle) >= 2*math.pi - 1e-5 else sides + 1
    for layer, inset in enumerate((0.0, -thickness)):
        for j in range(rings + 1):
            t = j / rings
            z = height * t
            for i in range(steps):
                u = i / (steps if abs(end_angle - start_angle) >= 2*math.pi - 1e-5 else steps - 1)
                a = start_angle + (end_angle - start_angle) * u
                if gap and i in (0, steps-1):
                    a += math.copysign(gap * 0.5, i - (steps-1)/2)
                r = radius_fn(t) + inset
                r += 0.0013 * noise.noise(Vector((math.cos(a)*4 + seed, math.sin(a)*4, t*8)))
                verts.append((r * math.cos(a), r * math.sin(a), z))
                coords.append((math.cos(a) * r, math.sin(a) * r, z))
    def idx(layer, j, i): return layer * (rings + 1) * steps + j * steps + (i % steps)
    cyclic = abs(end_angle - start_angle) >= 2*math.pi - 1e-5
    imax = steps if cyclic else steps - 1
    for layer in (0, 1):
        for j in range(rings):
            for i in range(imax):
                if layer == 0:
                    faces.append((idx(layer,j,i), idx(layer,j,i+1), idx(layer,j+1,i+1), idx(layer,j+1,i)))
                else:
                    faces.append((idx(layer,j,i+1), idx(layer,j,i), idx(layer,j+1,i), idx(layer,j+1,i+1)))
    if not cyclic:
        for j in range(rings):
            faces.append((idx(1,j,0), idx(0,j,0), idx(0,j+1,0), idx(1,j+1,0)))
            faces.append((idx(0,j,steps-1), idx(1,j,steps-1), idx(1,j+1,steps-1), idx(0,j+1,steps-1)))
    if cap_bottom:
        for i in range(imax): faces.append((idx(1,0,i+1), idx(0,0,i+1), idx(0,0,i), idx(1,0,i)))
    if cap_top:
        for i in range(imax): faces.append((idx(1,rings,i), idx(0,rings,i), idx(0,rings,i+1), idx(1,rings,i+1)))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def cylinder_vessel(kit, name, radius, height, mat, sides=36, rings=10, loc=(0,0,0), taper=0.0, cap=True):
    rows, coords = [], []
    for j in range(rings+1):
        t = j / rings
        z = -height/2 + height*t
        r = radius * (1 + taper * (0.5 - abs(t - 0.5)))
        row, c = [], []
        for i in range(sides):
            a = 2*math.pi*i/sides
            row.append(Vector((r*math.cos(a), r*math.sin(a), z)) + Vector(loc))
            c.append((r*math.cos(a), r*math.sin(a), z + height/2))
        rows.append(row); coords.append(c)
    return kit.loft(name, rows, material=mat, coords=coords, cap_start=cap, cap_end=cap)


def bottle(kit, name, x, y, z, height, mat, seed=0):
    pts = []
    sides = 28
    profile = [(0.0,0.030),(0.05,0.036),(0.18,0.040),(0.58,0.040),(0.68,0.030),(0.78,0.017),(0.95,0.017),(1.0,0.022)]
    rows=[]; coords=[]
    for t,r in profile:
        row=[]; c=[]
        for i in range(sides):
            a=2*math.pi*i/sides
            wob=0.0012*noise.noise(Vector((math.cos(a)*4+seed, math.sin(a)*4, t*5)))
            rr=r+wob
            row.append((x+rr*math.cos(a), y+rr*math.sin(a), z+height*t))
            c.append((rr*math.cos(a), rr*math.sin(a), height*t))
        rows.append(row); coords.append(c)
    return kit.loft(name, rows, material=mat, coords=coords, cap_start=True, cap_end=True)


def jar(kit, name, x, y, z, height, mat, bung_mat=None, seed=0):
    obj = cylinder_vessel(kit, name, 0.055, height, mat, sides=32, rings=8, loc=(x,y,z+height/2), taper=0.18)
    parts=[obj]
    if bung_mat:
        parts.append(kit.cylinder(name+"_Bung", 0.036, 0.018, location=(x,y,z+height+0.006), material=bung_mat, sides=22, bevel=0.002))
    return parts


def packet(kit, name, loc, size, paper_mat, twine_mat, seed=0, rot=(0,0,0)):
    parts=[board(kit, name, size, loc, paper_mat, rotation=rot, bevel=0.006, rough=0.0012, seed=seed)]
    x,y,z=loc; sx,sy,sz=size
    # crossed string slightly proud of packet
    parts.append(kit.tube(name+"_StringX", [Vector((x-sx*0.52,y,z+sz*0.54)), Vector((x+sx*0.52,y,z+sz*0.54))], radius=0.0018, sides=6, material=twine_mat))
    parts.append(kit.tube(name+"_StringY", [Vector((x,y-sy*0.52,z+sz*0.55)), Vector((x,y+sy*0.52,z+sz*0.55))], radius=0.0018, sides=6, material=twine_mat))
    return parts


def folded_cloth(kit, name, loc, size, mat, seed=0):
    parts=[]
    x,y,z=loc; sx,sy,sz=size
    for i in range(3):
        dz=(i-1)*sz*0.22
        parts.append(board(kit, f"{name}_Fold{i}", (sx*(1-0.07*i), sy, sz*0.34), (x, y, z+dz), mat,
                           bevel=0.012, rough=0.002, seed=seed+i))
    return parts
