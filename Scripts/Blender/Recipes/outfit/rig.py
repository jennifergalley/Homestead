"""Skinning, cloth thickness, small sewn accessories, FBX export, test poses and body coverage."""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

from .body import mesh_arrays, Surface
from .detail import STRIP
from . import textures as T

MAX_INFLUENCES = 8


# ------------------------------------------------------------------- weights

def weight_source(body_obj, face_obj, arm):
    """Body + face (the face owns the neck and tops of the shoulders) as one transfer source,
    keeping only groups that are bones of the body skeleton."""
    bones = {b.name for b in arm.data.bones}
    objs = []
    for src in (body_obj, face_obj):
        dup = src.copy()
        dup.data = src.data.copy()
        bpy.context.scene.collection.objects.link(dup)
        for mod in list(dup.modifiers):
            dup.modifiers.remove(mod)
        for vg in list(dup.vertex_groups):
            if vg.name not in bones:
                dup.vertex_groups.remove(vg)
        objs.append(dup)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    src = bpy.context.view_layer.objects.active
    src.name = "_WeightSource"
    src.hide_render = True
    return src


def transfer_weights(garment, src):
    for vg in src.vertex_groups:
        if vg.name not in garment.vertex_groups:
            garment.vertex_groups.new(name=vg.name)
    mod = garment.modifiers.new("WeightTransfer", "DATA_TRANSFER")
    mod.object = src
    mod.use_vert_data = True
    mod.data_types_verts = {"VGROUP_WEIGHTS"}
    mod.vert_mapping = "POLYINTERP_NEAREST"
    mod.layers_vgroup_select_src = "ALL"
    mod.layers_vgroup_select_dst = "NAME"
    bpy.ops.object.select_all(action="DESELECT")
    garment.select_set(True)
    bpy.context.view_layer.objects.active = garment
    bpy.ops.object.modifier_apply(modifier=mod.name)


def read_weights(obj):
    names = [g.name for g in obj.vertex_groups]
    W = np.zeros((len(obj.data.vertices), len(names)), np.float64)
    for v in obj.data.vertices:
        for g in v.groups:
            W[v.index, g.group] = g.weight
    return names, W


def write_weights(obj, names, W):
    for vg in list(obj.vertex_groups):
        obj.vertex_groups.remove(vg)
    used = np.where(W.max(0) > 0)[0]
    for j in used:
        vg = obj.vertex_groups.new(name=names[j])
        nz = np.where(W[:, j] > 0)[0]
        for i in nz:
            vg.add([int(i)], float(W[i, j]), "REPLACE")


def clean_weights(W, alpha=None, E=None, iters=0, max_inf=MAX_INFLUENCES, floor=0.01):
    """Optional per-vertex Laplacian smoothing, then limit influences, prune and normalize."""
    if E is not None and iters:
        deg = np.bincount(E.ravel(), minlength=len(W)).astype(np.float64)
        for _ in range(iters):
            avg = np.zeros_like(W)
            np.add.at(avg, E[:, 0], W[E[:, 1]])
            np.add.at(avg, E[:, 1], W[E[:, 0]])
            avg /= np.maximum(deg, 1)[:, None]
            W = W + alpha[:, None] * (avg - W)
    order = np.argsort(-W, axis=1)
    keep = np.zeros_like(W, bool)
    rows = np.arange(len(W))[:, None]
    keep[rows, order[:, :max_inf]] = True
    W = np.where(keep & (W >= floor), W, 0.0)
    W /= np.maximum(W.sum(1, keepdims=True), 1e-12)
    return W


# ----------------------------------------------------------------- geometry

def solidify(obj, thick_w, base=0.0013, hem=0.0030):
    """Give the cloth real thickness outward from the fitted (inner) surface, thicker at the
    rolled hems/bindings. Vertex groups and UVs carry over to the new shell."""
    vg = obj.vertex_groups.new(name="_hem")
    for i, w in enumerate(thick_w):
        vg.add([i], float(w), "REPLACE")
    mod = obj.modifiers.new("Thickness", "SOLIDIFY")
    mod.thickness = hem
    mod.offset = 1.0
    mod.use_rim = True
    mod.use_even_offset = True
    mod.use_quality_normals = True
    mod.vertex_group = "_hem"
    mod.thickness_vertex_group = base / hem
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.vertex_groups.remove(obj.vertex_groups["_hem"])


def tube_mesh(name, pts, radius, sides, uv_rect):
    """Swept tube (parallel-transport frames) with UVs in ``uv_rect`` = (u0, u1, v0, v1)."""
    pts = [Vector(p) for p in pts]
    n = len(pts)
    tang = [(pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(n)]
    ref = Vector((0, 0, 1)) if abs(tang[0].z) < 0.9 else Vector((1, 0, 0))
    nrm = tang[0].cross(ref).normalized()
    frames = []
    for i in range(n):
        if i:
            axis = tang[i - 1].cross(tang[i])
            if axis.length > 1e-8:
                ang = tang[i - 1].angle(tang[i])
                nrm = (Matrix.Rotation(ang, 3, axis.normalized()) @ nrm).normalized()
        frames.append((nrm.copy(), tang[i].cross(nrm).normalized()))
    arc = [0.0]
    for i in range(1, n):
        arc.append(arc[-1] + (pts[i] - pts[i - 1]).length)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    rings = []
    for i, p in enumerate(pts):
        r = radius(arc[i] / arc[-1]) if callable(radius) else radius
        a, b = frames[i]
        rings.append([bm.verts.new(p + (a * math.cos(2 * math.pi * k / sides) + b * math.sin(2 * math.pi * k / sides)) * r)
                      for k in range(sides)])
    u0, u1, v0, v1 = uv_rect
    for i in range(n - 1):
        for k in range(sides):
            kn = (k + 1) % sides
            f = bm.faces.new((rings[i][k], rings[i][kn], rings[i + 1][kn], rings[i + 1][k]))
            for loop, (ii, kk) in zip(f.loops, ((i, k), (i, k + 1), (i + 1, k + 1), (i + 1, k))):
                loop[uvl].uv = (u0 + (u1 - u0) * arc[ii] / arc[-1], v0 + (v1 - v0) * kk / sides)
    bmesh.ops.contextual_create(bm, geom=rings[-1])
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def accessories(garment, fields, V, rng, drawstring=False):
    """A tied drawstring at the front of the shorts, and a few loose threads at the hems."""
    surf_V, surf_N, surf_T = mesh_arrays(garment, evaluated=False)
    gs = Surface(surf_V, surf_N, surf_T)
    parts = []
    down = np.array([0.0, 0.0, -1.0])

    def on_cloth(p, lift):
        _, loc, n = gs.signed(np.asarray(p, float)[None])
        return loc[0] + n[0] * lift

    if drawstring:
        dw, _ = fields["waist"]
        top_front = V[(dw < 0.004) & (V[:, 1] < -0.05)]
        zt = top_front[np.argmin(np.abs(top_front[:, 0])), 2]
        for k, sx in enumerate((0.013, -0.013)):
            exit_pt = on_cloth((sx, -0.2, zt - 0.014), 0.0022)
            length = 0.095 + 0.02 * k
            pts = []
            for s in np.linspace(0, 1, 26):
                wob = 0.004 * math.sin(s * 5.0 + k) + sx * 0.6 * s
                p = exit_pt + np.array([wob, 0, 0]) + down * (0.008 + length * s)
                pts.append(on_cloth(p, 0.0032 + 0.003 * s))
            pts = [exit_pt] + pts
            parts.append(tube_mesh(f"_cord{k}", pts, lambda t: 0.0021 * (1.25 if t > 0.965 else 1.0), 8,
                                   (0.02 + 0.47 * k, 0.47 + 0.47 * k, 0.0, STRIP * 0.62)))
        knot = on_cloth((0.0, -0.2, zt - 0.022), 0.0045)
        ring = [knot + np.array([0.0048 * math.cos(a), 0.0015 * math.sin(a) - 0.001, 0.0038 * math.sin(a)])
                for a in np.linspace(0, 2 * math.pi, 18)]
        parts.append(tube_mesh("_knot", ring, 0.0026, 8, (0.0, 0.3, 0.0, STRIP * 0.62)))
    # loose threads
    edge_names = [n for n in ("hem", "leghem") if n in fields]
    for name in edge_names:
        d, _ = fields[name]
        cand = np.where(d < 0.0008)[0]
        picks = rng.choice(cand, size=3 if name == "leghem" else 2, replace=False)
        for t, vi in enumerate(picks):
            start = V[vi]
            length = rng.uniform(0.014, 0.034)
            phase = rng.uniform(0, 6.28)
            pts = []
            for s in np.linspace(0, 1, 14):
                p = start + down * length * s + np.array([0.004 * math.sin(phase + s * 4.5), 0, 0]) * s
                pts.append(on_cloth(p, 0.0012) if s < 0.15 else p)
            u0 = 0.05 + 0.1 * (t + 3 * (name == "leghem"))
            parts.append(tube_mesh(f"_thread_{name}{t}", pts, lambda q: 0.00038 * (1 - 0.5 * q), 5,
                                   (u0, u0 + 0.08, STRIP * 0.66, STRIP * 0.98)))
    return parts


# ---------------------------------------------------------------------- poses

def rotate_bone_world(arm, name, axis, deg):
    pb = arm.pose.bones[name]
    M = arm.matrix_world.inverted() @ (arm.matrix_world @ pb.matrix)
    head = M.to_translation()
    R = Matrix.Rotation(math.radians(deg), 4, axis)
    pb.matrix = Matrix.Translation(head) @ R @ Matrix.Translation(-head) @ M
    bpy.context.view_layer.update()


def reset_pose(arm):
    for pb in arm.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    arm.location = (0, 0, 0)
    bpy.context.view_layer.update()


X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
SPINE = ["spine_01", "spine_02", "spine_03", "spine_04", "spine_05"]

POSES = {
    "bend_forward_60": [("pelvis", X, 25), ("thigh_l", X, -25), ("thigh_r", X, -25)]
    + [(s, X, 7) for s in SPINE],
    "twist_45": [(s, Z, 9) for s in SPINE],
    "arms_overhead": [("clavicle_l", Y, -22), ("clavicle_r", Y, 22), ("upperarm_l", Y, -118), ("upperarm_r", Y, 118)],
    "kneel_left_forward": [("thigh_l", X, -88), ("calf_l", X, 88), ("thigh_r", X, -8), ("calf_r", X, 100),
                           ("foot_r", X, -20)],
    "squat": [("pelvis", X, 18), ("thigh_l", X, -118), ("thigh_r", X, -118), ("thigh_l", Y, -12), ("thigh_r", Y, 12),
              ("calf_l", X, 128), ("calf_r", X, 128), ("foot_l", X, -18), ("foot_r", X, -18)]
    + [(s, X, 3) for s in SPINE],
}


def apply_pose(arm, name):
    reset_pose(arm)
    for bone, axis, deg in POSES[name]:
        rotate_bone_world(arm, bone, axis, deg)


def ground(arm, objs):
    """Drop the posed character so her lowest point rests on z = 0."""
    lo = min(mesh_arrays(o)[0][:, 2].min() for o in objs)
    arm.location.z -= lo
    bpy.context.view_layer.update()


def poke_metrics(garment, body_surf, edge_dist):
    """Garment vertices inside the posed body: all, and within 2 cm of an open edge (where the
    body stays visible even after hiding covered skin)."""
    V, _, _ = mesh_arrays(garment)
    s, _, _ = body_surf.signed(V)
    inside = s < -0.0005
    near = edge_dist < 0.02
    return dict(verts=int(len(V)), inside=int(inside.sum()), inside_pct=float(100 * inside.mean()),
                inside_near_edge=int((inside & near).sum()),
                max_depth_mm=float(max(0.0, -s.min()) * 1000),
                p99_depth_mm=float(max(0.0, -np.percentile(s, 1)) * 1000))


# ------------------------------------------------------------------- coverage

def coverage_mask(body_obj, garments, edge_attr, path, R=2048, margin=0.012, max_dist=0.04, z_min=0.6):
    """White where the body is fully under cloth (safe to hide), in the body's UV0 space.
    A body vertex is covered when a ray along its normal meets a garment within `max_dist` at a
    point at least ``margin`` inside the garment's open edges; faces need all corners covered."""
    Vb, Nb, Tb = mesh_arrays(body_obj)
    cover = np.zeros(len(Vb), bool)
    per_garment = {}
    for g in garments:
        Vg, Ng, Tg = mesh_arrays(g)
        gs = Surface(Vg, Ng, Tg)
        ed = edge_attr[g.name]
        c = np.zeros(len(Vb), bool)
        for i in range(len(Vb)):
            if Vb[i, 2] < z_min or Vb[i, 2] > 1.6:
                continue
            hit = gs.bvh.ray_cast(Vector(Vb[i] + Nb[i] * 0.0005), Vector(Nb[i]), max_dist)
            if hit[0] is None:
                continue
            tri = Tg[hit[2]]
            if ed[tri].min() >= margin:
                c[i] = True
        per_garment[g.name] = c
        cover |= c
    me = body_obj.data
    me.calc_loop_triangles()
    nt = len(me.loop_triangles)
    tv = np.empty(nt * 3, np.int64)
    me.loop_triangles.foreach_get("vertices", tv)
    tl = np.empty(nt * 3, np.int64)
    me.loop_triangles.foreach_get("loops", tl)
    tv, tl = tv.reshape(-1, 3), tl.reshape(-1, 3)
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers[0].data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    u_off = np.floor(uv[:, 0].min())
    uvn = uv - np.array([u_off, 0.0])
    covered_tri = cover[tv].all(1)
    vals = np.ones((covered_tri.sum(), 3, 1), np.float32)
    img, _ = T.rasterize(uvn[tl[covered_tri]], vals, R)
    T.write_png(path, img[..., 0])
    faces = {}
    for name, c in per_garment.items():
        faces[name] = int(c[tv].all(1).sum())
    return dict(resolution=R, uv_channel=0, uv_name=me.uv_layers[0].name, udim_u_offset=float(u_off),
                covered_triangles=int(covered_tri.sum()), total_triangles=int(nt),
                covered_vertices=int(cover.sum()), per_garment_triangles=faces, margin_m=margin), cover


# --------------------------------------------------------------------- export

def export_fbx(path, arm, meshes, log=print):
    """Export like Unreal's own FBX: cm units (UnitScaleFactor 1), Z up, -Y forward, armature
    object 'root' as the root bone, only bones that deform these meshes (plus their parents),
    no leaf bones, no animation, no body."""
    scene = bpy.context.scene
    used = set()
    for m in meshes:
        used |= {vg.name for vg in m.vertex_groups}
    keep = set()
    for name in used:
        b = arm.data.bones.get(name)
        while b is not None:
            keep.add(b.name)
            b = b.parent
    saved = {b.name: b.use_deform for b in arm.data.bones}
    for b in arm.data.bones:
        b.use_deform = b.name in keep
    # temporary 100x copy so FBX values are raw centimeters like the Unreal export
    dups = []
    arm2 = arm.copy()
    arm2.data = arm.data.copy()
    scene.collection.objects.link(arm2)
    for m in meshes:
        d = m.copy()
        d.data = m.data.copy()
        scene.collection.objects.link(d)
        d.parent = arm2
        for mod in d.modifiers:
            if mod.type == "ARMATURE":
                mod.object = arm2
        dups.append(d)
    names = [m.name for m in meshes]
    arm_name = arm.name
    arm.name = "_root_src"
    for m in meshes:
        m.name = "_src_" + m.name
        m.data.name = "_src_" + m.data.name
    arm2.name = arm_name
    for d, n in zip(dups, names):
        d.name = n
        d.data.name = n
    reset_pose(arm2)
    for d in dups:
        mw = d.matrix_world.copy()
        d.parent = None
        d.matrix_world = mw
    bpy.ops.object.select_all(action="DESELECT")
    for o in [arm2] + dups:
        o.select_set(True)
    bpy.context.view_layer.objects.active = arm2
    arm2.scale = (100, 100, 100)
    for d in dups:
        d.scale = (100, 100, 100)
    bpy.context.view_layer.update()
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for d in dups:
        d.parent = arm2
    old_len = scene.unit_settings.scale_length
    scene.unit_settings.scale_length = 0.01
    bpy.ops.object.select_all(action="DESELECT")
    for o in [arm2] + dups:
        o.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"ARMATURE", "MESH"},
        apply_unit_scale=True, global_scale=1.0, apply_scale_options="FBX_SCALE_ALL",
        axis_forward="Y", axis_up="Z", use_space_transform=True,
        add_leaf_bones=False, primary_bone_axis="Y", secondary_bone_axis="X",
        use_armature_deform_only=True, armature_nodetype="NULL", bake_anim=False,
        mesh_smooth_type="FACE", use_tspace=True, use_mesh_modifiers=False,
        path_mode="STRIP", embed_textures=False)
    scene.unit_settings.scale_length = old_len
    nbones = sum(1 for b in arm2.data.bones if b.use_deform)
    for d in dups:
        me = d.data
        bpy.data.objects.remove(d)
        bpy.data.meshes.remove(me)
    ad = arm2.data
    bpy.data.objects.remove(arm2)
    bpy.data.armatures.remove(ad)
    arm.name = arm_name
    for m, n in zip(meshes, names):
        m.name = n
        m.data.name = n
    for b in arm.data.bones:
        b.use_deform = saved[b.name]
    log(f"  exported {path.name}: {names}, {nbones} bones")
    return nbones, sorted(keep)
