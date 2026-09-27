"""Build and render stages for the footwear recipe (Scripts/Blender/Recipes/footwear.py)."""
import hashlib
import json
import math
import time
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

from outfit import body as B
from outfit import render as Rn
from outfit import rig as R
from outfit import textures as T
from . import leather as LT
from . import pairs as PR

X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
SPINE = ["spine_01", "spine_02", "spine_03", "spine_04", "spine_05"]

# World-axis bone rotations (outfit.rig.rotate_bone_world), applied in order. About +X: thigh
# negative = hip flexion, calf positive = knee flexion, foot positive = plantarflexion (toes
# down), ball negative = toes bent up (MTP dorsiflexion).
POSES = {
    # double support: left heel strike (ankle neutral, foot 20 deg toes-up), right toe-off
    "walk_heel_strike": [("thigh_l", X, -25), ("calf_l", X, 5), ("thigh_r", X, 12), ("calf_r", X, 40),
                         ("foot_r", X, 5), ("ball_r", X, -50)],
    "walk_toe_off": [("thigh_r", X, -25), ("calf_r", X, 5), ("thigh_l", X, 12), ("calf_l", X, 40),
                     ("foot_l", X, 5), ("ball_l", X, -50)],
    # kneeling upright on both knees with the toes tucked under (MTP ~80 deg)
    "kneel_toes_flexed": [("thigh_l", X, -8), ("thigh_r", X, -8), ("calf_l", X, 100), ("calf_r", X, 100),
                          ("foot_l", X, -5), ("foot_r", X, -5), ("ball_l", X, -80), ("ball_r", X, -80)],
    # flat-footed deep squat: 118 deg hip, 135 deg knee, 35 deg ankle dorsiflexion
    "deep_squat": [("pelvis", X, 18), ("thigh_l", X, -118), ("thigh_r", X, -118), ("thigh_l", Y, -12),
                   ("thigh_r", Y, 12), ("calf_l", X, 135), ("calf_r", X, 135), ("foot_l", X, -35),
                   ("foot_r", X, -35)] + [(s, X, 3) for s in SPINE],
    # up on the balls of the feet: 40 deg plantarflexion, toes flat (MTP 40 deg)
    "tiptoe": [("foot_l", X, 40), ("foot_r", X, 40), ("ball_l", X, -40), ("ball_r", X, -40)],
}
TOE_BONES = ("bigtoe", "indextoe", "middletoe", "ringtoe", "littletoe")
WARMTH = {"WovenSandals": 0, "TurnShoes": 1, "FurBoots": 4}


def log(msg):
    from footwear import log as _log
    _log(msg)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


# ------------------------------------------------------------------ material

def make_material(name, paths):
    mat = bpy.data.materials.new(name)
    if mat.node_tree is None:
        mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]

    def tex(path, srgb):
        node = nt.nodes.new("ShaderNodeTexImage")
        node.image = bpy.data.images.load(str(path), check_existing=True)
        node.image.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
        return node

    base, ao = tex(paths["basecolor"], True), tex(paths["ao"], False)
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    nt.links.new(base.outputs["Color"], mix.inputs["A"])
    nt.links.new(ao.outputs["Color"], mix.inputs["B"])
    nt.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    rough = tex(paths["roughness"], False)
    nt.links.new(rough.outputs["Color"], bsdf.inputs["Roughness"])
    nrm = tex(paths["normal"], False)
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(nrm.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["Specular IOR Level"].default_value = 0.4
    return mat


# ------------------------------------------------------------------- weights

def rig(obj, arm, body_obj):
    """Nearest-face interpolated weights from the body; toe-bone weight folded into ball_l/_r so
    the toe box and sole flex as one piece at the ball; light smoothing; cap 8; normalize."""
    bones = {b.name for b in arm.data.bones}
    src = body_obj.copy()
    src.data = body_obj.data.copy()
    link(src)
    for mod in list(src.modifiers):
        src.modifiers.remove(mod)
    for vg in list(src.vertex_groups):
        if vg.name not in bones:
            src.vertex_groups.remove(vg)
    R.transfer_weights(obj, src)
    names, W = R.read_weights(obj)
    idx = {n: i for i, n in enumerate(names)}
    for side in "lr":
        ball = idx.get(f"ball_{side}")
        for n, i in idx.items():
            if n.startswith(TOE_BONES) and n.endswith(f"_{side}") and ball is not None:
                W[:, ball] += W[:, i]
                W[:, i] = 0
    me = obj.data
    E = np.empty(len(me.edges) * 2, np.int64)
    me.edges.foreach_get("vertices", E)
    W = R.clean_weights(W, np.full(len(W), 0.35), E.reshape(-1, 2), iters=4)
    R.write_weights(obj, names, W)
    bpy.data.objects.remove(src)
    mod = obj.modifiers.new("Armature", "ARMATURE")
    mod.object = arm
    obj.parent = arm
    used = [vg.name for vg in obj.vertex_groups]
    return dict(bones_weighted=len(used), max_influences=int((W > 0).sum(1).max()))


def apply_pose(arm, name):
    R.reset_pose(arm)
    for bone, axis, deg in POSES[name]:
        R.rotate_bone_world(arm, bone, axis, deg)


def vattr(obj, name):
    a = np.empty(len(obj.data.vertices), np.float32)
    obj.data.attributes[name].data.foreach_get("value", a)
    return a


def poke(obj, body_obj, opening):
    """Footwear vertices inside the (posed) body: count, % and depth; and how many lie within
    2 cm of an opening, where skin stays visible after the coverage mask hides the rest."""
    Vb, Nb, Tb = B.mesh_arrays(body_obj)
    surf = B.Surface(Vb, Nb, Tb)
    V, _, _ = B.mesh_arrays(obj)
    s, _, _ = surf.signed(V)
    inside = s < -0.0005
    near = opening < 0.02
    side = np.where(V[:, 0] > 0, "l", "r")
    return dict(verts=int(len(V)), inside=int(inside.sum()), inside_pct=round(float(100 * inside.mean()), 3),
                inside_near_opening=int((inside & near).sum()),
                max_depth_mm=round(float(max(0.0, -s.min()) * 1000), 2),
                p999_depth_mm=round(float(max(0.0, -np.percentile(s, 0.1)) * 1000), 2),
                inside_left=int((inside & (side == "l")).sum()), inside_right=int((inside & (side == "r")).sum()))


# ------------------------------------------------------------------ coverage

def coverage_mask(body_obj, obj, path, R_=2048, margin=0.012, reach=0.07):
    """Body UV0 mask, white = triangle fully covered by the footwear shell and at least
    ``margin`` from any opening (rays along each body normal must hit the shell, not a thong or
    cord, within ``reach``). Same convention as PrimitiveOutfit/BodyCoverageMask.png."""
    Vb, Nb, Tb = B.mesh_arrays(body_obj)
    Vg, Ng, Tg = B.mesh_arrays(obj)
    gs = B.Surface(Vg, Ng, Tg)
    opening = vattr(obj, "open")
    tube = vattr(obj, "per") > 0
    cover = np.zeros(len(Vb), bool)
    cand = np.where(Vb[:, 2] < 0.40)[0]
    for i in cand:
        hit = gs.bvh.ray_cast(Vector(Vb[i] + Nb[i] * 0.0003), Vector(Nb[i]), reach)
        if hit[0] is None:
            continue
        tri = Tg[hit[2]]
        if tube[tri].any():
            continue
        if opening[tri].min() >= margin:
            cover[i] = True
    me = body_obj.data
    me.calc_loop_triangles()
    nt = len(me.loop_triangles)
    tv = np.empty(nt * 3, np.int64); me.loop_triangles.foreach_get("vertices", tv); tv = tv.reshape(-1, 3)
    tl = np.empty(nt * 3, np.int64); me.loop_triangles.foreach_get("loops", tl); tl = tl.reshape(-1, 3)
    uv = np.empty(len(me.loops) * 2); me.uv_layers[0].data.foreach_get("uv", uv); uv = uv.reshape(-1, 2)
    u_off = np.floor(uv[:, 0].min())
    uvn = uv - np.array([u_off, 0.0])
    covered = cover[tv].all(1)
    img, _ = T.rasterize(uvn[tl[covered]], np.ones((covered.sum(), 3, 1), np.float32), R_)
    T.write_png(path, img[..., 0])
    # how much of the foot (below 4 cm, i.e. sole, toes, heel) is hidden
    foot = (Vb[tv][:, :, 2].max(1) < 0.04)
    info = dict(resolution=R_, uv_channel=0, uv_name=me.uv_layers[0].name, udim_u_offset=float(u_off),
                covered_triangles=int(covered.sum()), total_triangles=int(nt), margin_m=margin,
                foot_triangles_below_4cm=int(foot.sum()),
                foot_triangles_below_4cm_covered_pct=round(float(100 * covered[foot].mean()), 2))
    return info, covered


def visible_group(body_obj, covered):
    """Vertex group of body vertices on uncovered triangles, for a Mask modifier that hides
    covered skin in review renders the way the in-game mask does."""
    me = body_obj.data
    me.calc_loop_triangles()
    tv = np.empty(len(me.loop_triangles) * 3, np.int64)
    me.loop_triangles.foreach_get("vertices", tv)
    tv = tv.reshape(-1, 3)
    keep = np.unique(tv[~covered])
    vg = body_obj.vertex_groups.get("_visible") or body_obj.vertex_groups.new(name="_visible")
    vg.add(keep.tolist(), 1.0, "REPLACE")
    mod = body_obj.modifiers.new("CoverageMask", "MASK")
    mod.vertex_group = "_visible"
    return mod


# ------------------------------------------------------------ review extras

def add_face(arm, face_fbx):
    face_arm, face_meshes = B.import_skeletal(face_fbx)
    face = face_meshes[0]
    face.name = "SKM_MHC_Heroine_FaceMesh"
    Rn.prepare_face(face, face_arm, arm)
    return face


def add_outfit(arm, blend):
    """Append the primitive tank top and shorts for context and re-skin them to our armature."""
    names = ["SKM_PrimitiveTankTop", "SKM_PrimitiveShorts"]
    with bpy.data.libraries.load(str(blend), link=False) as (src, dst):
        dst.objects = [n for n in src.objects if n in names]
    out = []
    for o in dst.objects:
        if o is None:
            continue
        link(o)
        o.parent = arm
        for m in o.modifiers:
            if m.type == "ARMATURE":
                m.object = arm
        out.append(o)
    for o in list(bpy.data.objects):
        if o.type == "ARMATURE" and o is not arm and o.users_collection == ():
            bpy.data.objects.remove(o)
    return out


def trouser_standin(arm, body_obj):
    """Snug wool trousers stand-in: the legs and hips pushed out 7 mm, hem at the ankle bone."""
    import bmesh
    t = body_obj.copy()
    t.data = body_obj.data.copy()
    t.name = t.data.name = "Review_TrouserStandIn"
    link(t)
    for m in list(t.modifiers):
        t.modifiers.remove(m)
    for g in list(t.vertex_groups):
        if g.name.startswith("_"):
            t.vertex_groups.remove(g)
    names, w = B.group_matrix(t)
    leg = np.zeros(len(t.data.vertices))
    armw = np.zeros(len(t.data.vertices))
    tot = np.maximum(w.sum(1), 1e-6)
    for j, n in enumerate(names):
        if B.LEG_RE.match(n) or n.startswith("pelvis"):
            leg += w[:, j]
        if B.ARM_RE.match(n):
            armw += w[:, j]
    leg /= tot
    armw /= tot
    co = np.array([v.co for v in t.data.vertices])
    keep = (leg > 0.5) & (armw < 0.05) & (co[:, 2] > 0.088) & (co[:, 2] < 1.0)
    bm = bmesh.new()
    bm.from_mesh(t.data)
    bm.verts.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not all(keep[v.index] for v in f.verts)], context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=2e-5)
    bm.normal_update()
    for v in bm.verts:
        v.co += v.normal * 0.007
    bm.to_mesh(t.data)
    bm.free()
    t.parent = arm
    mod = t.modifiers.new("Armature", "ARMATURE")
    mod.object = arm
    sol = t.modifiers.new("Thick", "SOLIDIFY")
    sol.thickness = 0.0025
    sol.offset = 1.0
    mat = bpy.data.materials.new("M_ReviewWoolTrousers")
    if mat.node_tree is None:
        mat.use_nodes = True
    nt = mat.node_tree
    b = nt.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.075, 0.062, 0.05, 1)
    b.inputs["Roughness"].default_value = 0.95
    b.inputs["Sheen Weight"].default_value = 0.0
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 900.0
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.25
    nt.links.new(noise.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], b.inputs["Normal"])
    t.data.materials.clear()
    t.data.materials.append(mat)
    t.data.polygons.foreach_set("material_index", [0] * len(t.data.polygons))
    for s in t.material_slots:
        s.link = "OBJECT"
        s.material = mat
    return t


# ---------------------------------------------------------------------- build

def finish_build(name, built, arm, body_obj, VNT, bones, args):
    import footwear as FW
    out = FW.OUT / name
    tex_dir = out / "Textures"
    tex_dir.mkdir(parents=True, exist_ok=True)
    obj = built.obj
    t0 = time.time()
    paths, tex_stats = LT.synthesize(obj, built, tex_dir, name, R=FW.TEX_SIZE, seed=FW.SEED,
                                     extra=getattr(built, "tex_extra", {}), log=log)
    log(f"{name}: textures in {time.time() - t0:.0f}s {tex_stats}")
    obj.data.uv_layers.remove(obj.data.uv_layers["Param"])
    obj.data.materials.clear()
    obj.data.materials.append(make_material(f"M_{name}", paths))
    rig_info = rig(obj, arm, body_obj)
    opening = vattr(obj, "open")
    me = obj.data
    me.calc_loop_triangles()
    geo = dict(vertices=len(me.vertices), triangles=len(me.loop_triangles), **rig_info)
    log(f"{name}: rigged {geo}")
    R.reset_pose(arm)
    bind = poke(obj, body_obj, opening)
    log(f"{name}: bind pose {bind}")
    poses = {}
    for pose in POSES:
        apply_pose(arm, pose)
        poses[pose] = poke(obj, body_obj, opening)
        log(f"{name}: pose {pose} {poses[pose]}")
    R.reset_pose(arm)
    fbx = out / f"SKM_{name}.fbx"
    nb, bone_names = R.export_fbx(fbx, arm, [obj], log=log)
    mask = out / f"BodyCoverageMask_{name}.png"
    cov_info, covered = coverage_mask(body_obj, obj, mask)
    log(f"{name}: coverage {cov_info}")
    body_obj["footwear_covered_tris"] = np.flatnonzero(covered).astype(np.int32).tolist()
    report = dict(
        recipe="Scripts/Blender/Recipes/footwear.py", seed=FW.SEED, blender=bpy.app.version_string,
        inputs={p.name: sha256(p) for p in (FW.BODY_FBX, FW.FACE_FBX)},
        asset=f"SKM_{name}", material=f"M_{name}", geometry=geo,
        bones_exported=nb, bone_names=bone_names,
        sole_thickness_mm=round(built.sole_thickness_mm, 2),
        character_offset_cm=round(built.sole_thickness_mm / 10, 2),
        footbed_z_mm=round(built.footbed_z_mm, 2),
        fit=built.info, textures=tex_stats, bind_pose=bind, test_poses=poses,
        test_pose_definitions={k: [(b, "XYZ"[[X, Y, Z].index(a)], d) for b, a, d in v] for k, v in POSES.items()},
        coverage=cov_info, suggested_warmth_0_10=WARMTH[name],
        export_settings=dict(units="centimeters (FBX UnitScaleFactor 1.0, raw cm values like the UE export)",
                             axes="Z up, -Y forward (FBX UpAxis Z, FrontAxis -Y)",
                             armature="object 'root' exported as the root bone above pelvis",
                             leaf_bones=False, animation=False, deform_only=True, smoothing="FACE",
                             tangents=True, normal_maps="OpenGL (+Y): flip green in Unreal"))
    if name == "FurBoots":
        report["loft_mean_mm"] = round(built.info["l"]["loft_mean_mm"], 2)
    report["files"] = {fbx.name: sha256(fbx), mask.name: sha256(mask)}
    report["files"].update({f"Textures/{p.name}": sha256(p) for p in sorted(tex_dir.glob("*.png"))})
    (out / "report.json").write_text(json.dumps(report, indent=1, default=float) + "\n")
    # review scene: face, primitive outfit for context, review skin; saved for the render stage
    try:
        add_face(arm, FW.FACE_FBX)
    except Exception as e:  # the face only matters for full-figure renders
        log(f"face import failed: {e}")
    if FW.OUTFIT_BLEND.exists():
        add_outfit(arm, FW.OUTFIT_BLEND)
    face = bpy.data.objects.get("SKM_MHC_Heroine_FaceMesh")
    Rn.assign_review_materials(body_obj, face) if face else None
    for o in list(bpy.data.objects):
        if o.name == "PreviewCam":
            bpy.data.objects.remove(o)
    bpy.ops.wm.save_as_mainfile(filepath=str(out / f"{name}.blend"), relative_remap=True)
    log(f"saved {out / (name + '.blend')}")


# --------------------------------------------------------------------- render

VIEWS = {
    # name: (camera direction, lens, resolution, f-stop, framing margin, subject)
    "hero": ((-0.75, -1, 0.45), 60, (3840, 2160), None, 1.12, "shoes"),
    "side": ((1, -0.05, 0.10), 70, (3840, 2160), None, 1.10, "left"),
    "back": ((0.55, 1, 0.35), 60, (3840, 2160), None, 1.12, "shoes"),
    "top": ((0.15, -0.45, 1), 60, (3840, 2160), None, 1.10, "shoes"),
    "full": ((-0.35, -1, 0.05), 70, (2160, 3840), None, 1.06, "figure"),
}


def fit_view(points, direction, lens, res, margin=1.1, sensor=36.0):
    """Target (bbox centre) and camera distance that frame ``points``."""
    P = np.asarray(points, float)
    target = 0.5 * (P.min(0) + P.max(0))
    d = np.asarray(direction, float); d /= np.linalg.norm(d)
    fwd = -d
    right = np.cross(fwd, [0.0, 0.0, 1.0]); right /= np.linalg.norm(right)
    up = np.cross(right, fwd)
    rel = P - target
    x, y, z = rel @ right, rel @ up, rel @ fwd
    t = (sensor / 2) / lens
    if res[0] >= res[1]:
        tx, ty = t, t * res[1] / res[0]
    else:
        ty, tx = t, t * res[0] / res[1]
    dist = max(np.max(np.abs(x) / tx - z), np.max(np.abs(y) / ty - z)) * margin
    return tuple(target), float(dist)
DETAIL = {
    "FurBoots": {"detail_cuff": ((0.14, -0.05, 0.29), (0.9, -1, 0.35), 0.42, 85, 8),
                 "detail_ankle": ((0.14, -0.10, 0.07), (0.55, -1, 0.45), 0.40, 85, 8),
                 "detail_sole": ((0.15, -0.16, 0.01), (0.9, -1, 0.12), 0.36, 85, 8)},
    "WovenSandals": {"detail_toe": ((0.15, -0.14, 0.02), (0.35, -1, 0.55), 0.36, 85, 8),
                     "detail_ankle": ((0.14, 0.0, 0.05), (1, 0.5, 0.35), 0.38, 85, 8),
                     "detail_rim": ((0.19, -0.06, -0.005), (1, -0.4, 0.2), 0.30, 85, 8)},
    "TurnShoes": {"detail_laces": ((0.14, -0.08, 0.07), (0.25, -1, 0.9), 0.34, 85, 8),
                  "detail_heel": ((0.13, 0.04, 0.04), (0.6, 1, 0.3), 0.34, 85, 8),
                  "detail_toe": ((0.16, -0.17, 0.02), (0.55, -1, 0.3), 0.33, 85, 8)},
}


def render_stage(name, args):
    import footwear as FW
    out = FW.OUT / name
    bpy.ops.wm.open_mainfile(filepath=str(out / f"{name}.blend"))
    rdir = out / "Renders"
    rdir.mkdir(parents=True, exist_ok=True)
    backend = Rn.setup_scene(FW.HDRI, args.samples)
    log(f"{name}: render device {backend}")
    arm = bpy.data.objects["root"]
    body = bpy.data.objects["SKM_MHC_Heroine_BodyMesh"]
    shoe = bpy.data.objects[f"SKM_{name}"]
    report = json.loads((out / "report.json").read_text())
    lift = report["sole_thickness_mm"] / 1000.0
    body.data.calc_loop_triangles()
    covered = np.zeros(len(body.data.loop_triangles), bool)
    covered[np.asarray(body["footwear_covered_tris"], np.int64)] = True
    visible_group(body, covered)
    extras = [o for o in bpy.data.objects if o.name.startswith("SKM_Primitive")]
    trousers = trouser_standin(arm, body) if name == "FurBoots" else None
    if trousers is not None:
        for o in extras:
            if "Shorts" in o.name:
                o.hide_render = True
    want = set(args.views or [])
    done = []

    def shoot(vname, tgt, d, dist, lens, res, fstop):
        tgt = (tgt[0], tgt[1], tgt[2] + lift)
        done.append(Rn.shoot(rdir / f"{name}_{vname}.png", tgt, d, dist, lens, res, fstop))
        log(f"{name}: rendered {vname}")

    R.reset_pose(arm)
    arm.location.z = lift
    bpy.context.view_layer.update()
    Vs = B.mesh_arrays(shoe)[0]
    face = bpy.data.objects.get("SKM_MHC_Heroine_FaceMesh")
    fig = np.vstack([B.mesh_arrays(o)[0] for o in (body, face) if o is not None])
    subjects = {"shoes": np.vstack([Vs, fig[fig[:, 2] < Vs[:, 2].max() + 0.05]]) if name != "FurBoots" else Vs,
                "left": Vs[Vs[:, 0] > 0], "figure": np.vstack([fig, Vs])}
    for vname, (d, lens, res, fstop, margin, subj) in VIEWS.items():
        if want and vname not in want:
            continue
        tgt, dist = fit_view(subjects[subj], d, lens, res, margin)
        done.append(Rn.shoot(rdir / f"{name}_{vname}.png", tgt, d, dist, lens, res, fstop))
        log(f"{name}: rendered {vname}")
    for vname, (tgt, d, dist, lens, fstop) in DETAIL[name].items():
        if want and vname not in want:
            continue
        shoot(vname, tgt, d, dist, lens, (3840, 2160), fstop)
    if trousers is not None and (not want or "trousers" in want):
        shoot("trousers", (0.02, -0.05, 0.22), (-0.7, -1, 0.22), 2.1, 70, (3840, 2160), None)
    for pose in POSES:
        if want and pose not in want:
            continue
        apply_pose(arm, pose)
        R.ground(arm, [body, shoe])
        V = B.mesh_arrays(shoe)[0]
        Vb = B.mesh_arrays(body)[0]
        pts = np.vstack([V, Vb[Vb[:, 2] < V[:, 2].max() + 0.12]])
        tgt, dist = fit_view(pts, (-0.85, -1, 0.35), 50, (3840, 2160), 1.08)
        done.append(Rn.shoot(rdir / f"{name}_pose_{pose}.png", tgt, (-0.85, -1, 0.35), dist, 50,
                             (3840, 2160), None))
        log(f"{name}: rendered pose {pose}")
        arm.location.z = 0
    R.reset_pose(arm)
    return done
