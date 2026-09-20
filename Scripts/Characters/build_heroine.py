"""Offline MPFB authoring recipe. Run with the isolated Blender profile (see build.ps1)."""
import argparse
import importlib
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils import Quaternion
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Source" / "SystemAssets"
OUT = ROOT / "Assets" / "Characters" / "Heroine"
PREVIEW = ROOT / "Build" / "CharacterPreview"


def service(name):
    return getattr(importlib.import_module("bl_ext.user_default.mpfb.services." + name.lower()), name)


def active(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def smooth(obj):
    for face in obj.data.polygons:
        face.use_smooth = True


def material(name, color, roughness=0.6, texture=None, alpha=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = (*color, 1)
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1)
    shader.inputs["Roughness"].default_value = roughness
    if texture:
        image = mat.node_tree.nodes.new("ShaderNodeTexImage")
        image.image = bpy.data.images.load(str(texture), check_existing=True)
        mat.node_tree.links.new(image.outputs["Color"], shader.inputs["Base Color"])
        if alpha:
            mat.node_tree.links.new(image.outputs["Alpha"], shader.inputs["Alpha"])
            mat.surface_render_method = "DITHERED"
    return mat


def assign(obj, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat)


def chestnut_texture(source,name):
    import numpy as np
    image=bpy.data.images.load(str(source),check_existing=True)
    pixels=np.array(image.pixels[:],dtype=np.float32).reshape((-1,4))
    luminance=pixels[:,:3] @ np.array([0.2126,0.7152,0.0722])
    shade=0.24+0.76*np.sqrt(np.clip(luminance,0,1))
    pixels[:,:3]=shade[:,None]*np.array([0.26,0.105,0.039])
    result=bpy.data.images.new(name,width=image.size[0],height=image.size[1],alpha=True)
    result.pixels.foreach_set(pixels.ravel())
    directory=OUT/"Textures"
    directory.mkdir(exist_ok=True)
    result.filepath_raw=str(directory/(name+".png"))
    result.file_format="PNG"
    result.save()
    return Path(result.filepath_raw)


def mesh(name, vertices, faces, mat):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    assign(obj, mat)
    smooth(obj)
    return obj


def skin_weights(obj, body, rig, tree=None):
    """Transfer nearest fitted body vertex weights, capped and normalized to four."""
    if tree is None:
        tree = KDTree(len(body.data.vertices))
        for v in body.data.vertices:
            tree.insert(v.co, v.index)
        tree.balance()
    bone_names = {b.name for b in rig.data.bones}
    mapping = {g.index: g.name for g in body.vertex_groups if g.name in bone_names}
    for name in mapping.values():
        if name not in obj.vertex_groups:
            obj.vertex_groups.new(name=name)
    for v in obj.data.vertices:
        _, idx, _ = tree.find(v.co)
        weights = sorted(((mapping[g.group], g.weight) for g in body.data.vertices[idx].groups
                          if g.group in mapping and g.weight > 0), key=lambda item: -item[1])[:4]
        total = sum(w for _, w in weights)
        for name, weight in weights:
            obj.vertex_groups[name].add([v.index], weight / total, "REPLACE")
    obj.parent = rig
    mod = obj.modifiers.new("Heroine skinning", "ARMATURE")
    mod.object = rig


def bind_bone(obj, rig, name):
    obj.vertex_groups.clear()
    obj.vertex_groups.new(name=name).add(list(range(len(obj.data.vertices))), 1, "REPLACE")
    for mod in list(obj.modifiers):
        if mod.type == "ARMATURE":
            obj.modifiers.remove(mod)
    obj.parent = rig
    mod = obj.modifiers.new("Heroine skinning", "ARMATURE")
    mod.object = rig


def freeze_mesh(obj):
    """Freeze shape and visibility before adding custom outfit; retain authored weights."""
    active(obj)
    if obj.data.shape_keys:
        obj.shape_key_add(name="Authored shape", from_mix=True)
        for key in list(obj.data.shape_keys.key_blocks)[:-1]:
            obj.shape_key_remove(key)
        obj.shape_key_remove(obj.data.shape_keys.key_blocks[0])
    for mod in list(obj.modifiers):
        if mod.type not in {"ARMATURE"}:
            bpy.ops.object.modifier_apply(modifier=mod.name)


def add_asset(human, subdir, name, asset_type):
    path = SOURCE / subdir / name / (name + ".mhclo")
    obj = service("HumanService").add_mhclo_asset(
        str(path), human, asset_type=asset_type, material_type="GAMEENGINE", subdiv_levels=0)
    material_line=next(line for line in path.read_text().splitlines() if line.startswith("material "))
    mhmat=(path.parent/material_line.split(maxsplit=1)[1]).resolve()
    lines=mhmat.read_text().splitlines()
    diffuse=next((line for line in lines if line.startswith("diffuseTexture ")),None)
    if diffuse:
        texture=(mhmat.parent/diffuse.split(maxsplit=1)[1]).resolve()
        assign(obj,material("M_Heroine_"+asset_type,(0.25,0.13,0.07),0.58,texture,
                            alpha=asset_type in {"Eyebrows","Eyelashes","Hair"}))
    return obj


def create_body():
    human_service = service("HumanService")
    macro = service("TargetService").get_default_macro_info_dict()
    # MPFB's documented anchors are baby=0, child=.1875, young adult=.5, old=1.
    # Age is NOT a fraction of 100 years; retain the adult anchor for this heroine.
    macro.update(gender=0.0, age=0.5, muscle=0.38, weight=0.38,
                 height=0.40, proportions=0.65, cupsize=0.62, firmness=0.55,
                 race={"caucasian": 1.0, "asian": 0.0, "african": 0.0})
    stack=service("TargetService").calculate_target_stack_from_macro_info_dict(macro)
    if any(("-child" in name or "-baby" in name) and weight>0 for name,weight in stack):
        raise RuntimeError("The heroine recipe must use adult-only age targets")
    body = human_service.create_human(macro_detail_dict=macro)
    body.name = "Heroine_Body"
    targets = {
        "head/head-oval": 0.25,
        "chin/chin-width-decr": 0.18,
        "mouth/mouth-upperlip-volume-incr": 0.30,
        "mouth/mouth-lowerlip-volume-incr": 0.30,
        "mouth/mouth-scale-horiz-incr": 0.10,
        "nose/nose-scale-horiz-decr": 0.16,
        "eyes/l-eye-scale-incr": 0.08,
        "eyes/r-eye-scale-incr": 0.08,
        "cheek/l-cheek-bones-incr": 0.14,
        "cheek/r-cheek-bones-incr": 0.14,
    }
    target_root = Path(service("LocationService").get_mpfb_data("targets"))
    for target, value in targets.items():
        service("TargetService").load_target(body, str(target_root / (target + ".target.gz")), weight=value)
    rig = human_service.add_builtin_rig(body, "game_engine")
    rig.name = "Heroine_Rig"
    rig.data.name = "SKEL_Heroine"
    rig["AdultPhenotypeAnchor"]=macro["age"]
    skin = material("M_Heroine_Skin", (0.66, 0.43, 0.32), 0.48,
                    SOURCE / "skins" / "young_caucasian_female" / "young_lightskinned_female_diffuse.png")
    skin.node_tree.nodes.get("Principled BSDF").inputs["Subsurface Weight"].default_value = 0.07
    assign(body, skin)
    parts = [body]
    for folder, name, kind in [
        ("eyes", "high-poly", "Eyes"),
        ("eyebrows", "eyebrow001", "Eyebrows"),
        ("eyelashes", "eyelashes01", "Eyelashes"),
        ("teeth", "teeth_base", "Teeth"),
        ("tongue", "tongue01", "Tongue"),
    ]:
        obj = add_asset(body, folder, name, kind)
        obj.name = "Heroine_" + kind
        parts.append(obj)
    hairs = {}
    for style, name in [("LongWave", "long01"), ("Bob", "bob01")]:
        hair = add_asset(body, "hair", name, "Hair")
        hair.name = "Heroine_Hair_" + style
        hairs[style] = hair
    bpy.context.view_layer.update()
    visible_body=body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    full_body_height=max(v.co.z for v in visible_body.data.vertices)-min(
        v.co.z for v in visible_body.data.vertices)
    shoes=add_asset(body,"clothes","shoes01","Clothes")
    shoes.name="Heroine_LaceupShoes"
    shoes.data.materials[0].name="M_Heroine_LeatherShoes"
    parts.append(shoes)
    # Keep a MPFB reconstruction record before destructive export cleanup.
    human_service.serialize_to_json_file(body, str(OUT / "mpfb-authoring-preset.json"), save_clothes=False)
    for obj in parts + list(hairs.values()):
        freeze_mesh(obj)
        smooth(obj)
    # Uniformly normalize the complete rest rig and all meshes to petite adult height.
    factor = 1.60 / full_body_height
    for obj in parts + list(hairs.values()):
        for v in obj.data.vertices:
            v.co *= factor
        obj.location *= factor
    active(rig)
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in rig.data.edit_bones:
        bone.head *= factor
        bone.tail *= factor
    bpy.ops.object.mode_set(mode="OBJECT")
    for hair in hairs.values():
        bind_bone(hair, rig, "head")
        source_name = "long01" if hair == hairs["LongWave"] else "bob01"
        texture=SOURCE / "hair" / source_name / (source_name + "_diffuse.png")
        texture=chestnut_texture(texture,"T_Bob_Chestnut" if source_name=="bob01" else "T_Long_Chestnut")
        mat = material("M_Heroine_Hair_" + source_name, (0.12, 0.048, 0.021), 0.56,
                       texture, alpha=True)
        mat.node_tree.nodes.get("Principled BSDF").inputs["Specular IOR Level"].default_value = 0.10
        mat.node_tree.nodes.get("Principled BSDF").inputs["Roughness"].default_value = 0.70
        assign(hair, mat)
    eye_material = SOURCE / "eyes" / "materials" / "blue.mhmat"
    eye_texture_line = next(line for line in eye_material.read_text().splitlines()
                            if line.startswith("diffuseTexture"))
    eye_texture = eye_material.parent / eye_texture_line.split(maxsplit=1)[1]
    # The bundled high-poly eyes use alpha to hide their outer atlas shell;
    # treating this texture as opaque makes both eyes appear completely black.
    assign(parts[1], material("M_Heroine_LightEyes", (0.25, 0.47, 0.53), 0.22, eye_texture,alpha=True))
    # Broad waves add reversible volume to the bundled long-card hairstyle.
    long_hair = hairs["LongWave"]
    bvh=BVHTree.FromPolygons([v.co for v in body.data.vertices],
                            [p.vertices[:] for p in body.data.polygons])
    for v in long_hair.data.vertices:
        z = v.co.z
        fall = max(0.0, min(1.0, (1.49 - z) / 0.23))
        side = 1 if v.co.x >= 0 else -1
        v.co.x += side * fall * (0.013 + 0.012 * math.sin((1.5 - z) * 34))
        v.co.y += fall * 0.009 * math.sin((1.5 - z) * 30 + abs(v.co.x) * 7)
        if z<1.34:
            radial=Vector((v.co.x,v.co.y,0)).normalized()
            hit,_,_,_=bvh.ray_cast(Vector((0,0,z)),radial,0.3)
            if hit:
                minimum=min(0.175,math.hypot(hit.x,hit.y))+0.022
                current=math.hypot(v.co.x,v.co.y)
                if current<minimum:
                    v.co.x=radial.x*minimum
                    v.co.y=radial.y*minimum
    for obj in [body] + list(hairs.values()):
        mod = obj.modifiers.new("Surface refinement", "SUBSURF")
        mod.levels = 1
        mod.render_levels = 1
    return body, rig, parts, hairs, {
        "macro":macro,"targets":targets,"height_m":1.60,
        "age_anchor":"MPFB young adult (.5), no child/baby target contribution",
        "macro_target_stack":stack}


def make_outfit(body, rig):
    """Original scoop-neck linen wrap tunic, shoulder straps, belt and buckle."""
    linen = material("M_Heroine_MossLinen", (0.20, 0.27, 0.115), 0.87)
    lining = material("M_Heroine_LinenTrim", (0.44, 0.40, 0.24), 0.8)
    leather = material("M_Heroine_ChestnutLeather", (0.095, 0.037, 0.016), 0.7)
    brass = material("M_Heroine_Brass", (0.30, 0.19, 0.065), 0.32)
    brass.node_tree.nodes.get("Principled BSDF").inputs["Metallic"].default_value = 0.75
    bvh = BVHTree.FromPolygons([v.co for v in body.data.vertices],
                              [p.vertices[:] for p in body.data.polygons])
    # Trace from inside the torso, not inward from outside: arms must not shape the bodice.
    def radius(z, theta):
        direction = Vector((math.cos(theta), math.sin(theta), 0))
        origin = Vector((0, 0, z))
        hit, _, _, _ = bvh.ray_cast(origin, direction, 0.29)
        if hit is None:
            return 0.12
        return math.hypot(hit.x, hit.y)

    count, rows = 96, 36
    verts, faces = [], []
    for j in range(rows):
        t = j / (rows - 1)
        for i in range(count):
            a = 2 * math.pi * i / count
            # Front is Blender -Y. Scoop neckline; slightly asymmetric skirt.
            front = max(0, -math.sin(a))
            top = 1.165 + 0.070 * abs(math.sin(a)) - 0.038 * front ** 6
            bottom = 0.665 + 0.045 * math.cos(a + 0.5)
            z = bottom * (1-t) + top * t
            hip_r = radius(0.89, a)
            r = radius(max(z, 0.89), a) + 0.012
            if z < 0.91:
                r = max(r, hip_r + 0.014 + (0.91-z)*0.20)
                r += 0.004 * math.sin(a*12+0.6) * min(1, (0.91-z)*6)
            verts.append((r*math.cos(a), r*math.sin(a), z))
    for j in range(rows-1):
        for i in range(count):
            n = (i+1) % count
            faces.append((j*count+i, j*count+n, (j+1)*count+n, (j+1)*count+i))
    dress = mesh("Heroine_WrapTunic", verts, faces, linen)
    solid = dress.modifiers.new("Linen thickness", "SOLIDIFY")
    solid.thickness = 0.0025
    solid.offset = 0
    active(dress)
    bpy.ops.object.modifier_apply(modifier=solid.name)
    skin_weights(dress, body, rig)
    # A smooth skirt weighting avoids the discontinuous nearest-left/right-leg seam.
    for v in dress.data.vertices:
        if v.co.z < 0.91:
            for group in dress.vertex_groups:
                group.remove([v.index])
            leg = max(0, min(0.70, (0.91-v.co.z)*2.5))
            left = max(0.05, min(0.95, 0.5+v.co.x*3.5))
            for name, value in [("pelvis",1-leg), ("thigh_l",leg*left),("thigh_r",leg*(1-left))]:
                if name not in dress.vertex_groups:
                    dress.vertex_groups.new(name=name)
                dress.vertex_groups[name].add([v.index],value,"REPLACE")
    pieces = [dress]
    for label,row,delta in [("Hem",0,0.009),("Neckline",rows-1,-0.006)]:
        tv,tf=[],[]
        for layer in range(2):
            for i in range(count):
                co=Vector(verts[row*count+i])
                radial=Vector((co.x,co.y,0)).normalized()
                tv.append(co+radial*0.003+Vector((0,0,delta*layer)))
        for i in range(count):
            n=(i+1)%count
            tf.append((i,n,count+n,count+i))
        trim=mesh("Heroine_"+label+"Binding",tv,tf,lining)
        skin_weights(trim,dress,rig)
        pieces.append(trim)
    # A diagonal stitched overlap reads as a wrap garment rather than a plain tube.
    dbvh=BVHTree.FromPolygons([v.co for v in dress.data.vertices],
                             [p.vertices[:] for p in dress.data.polygons])
    sv,sf=[],[]
    for j in range(40):
        t=j/39
        a=-2.4+t*1.5
        bottom=0.665+0.045*math.cos(a+0.5)+0.008
        z=(1-t)*0.996+t*bottom
        radial=Vector((math.cos(a),math.sin(a),0))
        tangent=Vector((-math.sin(a),math.cos(a),0))
        hit,_,_,_=dbvh.ray_cast(Vector((0,0,z)),radial,0.4)
        center=hit+radial*0.005
        sv.extend([center-tangent*0.003,center+tangent*0.003])
        if j:
            k=(j-1)*2
            sf.append((k,k+1,k+3,k+2))
    seam=mesh("Heroine_WrapOverlapBinding",sv,sf,lining)
    skin_weights(seam,dress,rig)
    pieces.append(seam)

    # Sewn broad straps use the body surface projected from the front and back.
    for sign in [-1, 1]:
        sv, sf = [], []
        neckline=[Vector(co) for co in verts[(rows-1)*count:rows*count]]
        for j in range(33):
            t = j / 32
            for i in range(5):
                x = sign*(0.108 + (i/4-0.5)*0.037)
                front_edge=min((co for co in neckline if co.y<0),key=lambda co:abs(co.x-x))
                back_edge=min((co for co in neckline if co.y>0),key=lambda co:abs(co.x-x))
                shoulder,_,_,_=bvh.ray_cast(Vector((x,-0.005,1.36)),Vector((0,0,-1)),0.2)
                shoulder_z=shoulder.z+0.010 if shoulder else 1.31
                edge=front_edge if t<=0.5 else back_edge
                z=(edge.z-0.008)+(shoulder_z-edge.z+0.008)*math.sin(t*math.pi)
                front=t<=0.5
                hit,_,_,_=bvh.ray_cast(Vector((x,-0.35 if front else 0.35,z)),
                                      Vector((0,1 if front else -1,0)),0.35)
                y=(hit.y+(-0.016 if front else 0.016)) if hit else (
                    front_edge.y*(1-t)+back_edge.y*t)
                sv.append((x,y,z))
        for j in range(32):
            for i in range(4):
                k = j*5+i
                sf.append((k,k+1,k+6,k+5))
        strap = mesh("Heroine_ShoulderStrap_" + str(sign), sv, sf, linen)
        skin_weights(strap, body, rig)
        pieces.append(strap)

    # Narrow belt follows waist rays and has enough clearance over cloth.
    bv, bf = [], []
    for z in [0.989, 1.022]:
        for i in range(count):
            a = i*2*math.pi/count
            r = radius(z, a) + 0.018
            bv.append((r*math.cos(a), r*math.sin(a), z))
    for i in range(count):
        n = (i+1)%count
        bf.append((i,n,count+n,count+i))
    belt = mesh("Heroine_LeatherBelt", bv, bf, leather)
    skin_weights(belt, body, rig)
    pieces.append(belt)
    bpy.ops.mesh.primitive_torus_add(major_radius=0.013, minor_radius=0.0025,
                                   major_segments=24, minor_segments=8,
                                   location=(0.035,-radius(1.005,-math.pi/2)-0.023,1.005),
                                   rotation=(math.pi/2,0,0))
    buckle=bpy.context.object
    buckle.name="Heroine_BeltBuckle"
    assign(buckle,brass)
    active(buckle)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    skin_weights(buckle,body,rig)
    pieces.append(buckle)
    return pieces


def pose_world(rig, bone_name, axis, angle):
    bone=rig.pose.bones[bone_name]
    basis=bone.bone.matrix_local.to_quaternion()
    bone.rotation_mode="QUATERNION"
    bone.rotation_quaternion=basis.inverted() @ Quaternion(Vector(axis),angle) @ basis


def create_animation(rig,name,frames,walk=False):
    rig.animation_data_create()
    action=bpy.data.actions.new(name)
    action.use_fake_user=True
    rig.animation_data.action=action
    for frame in range(1,frames+1):
        t=(frame-1)/(frames-1)*math.tau
        for bone in rig.pose.bones:
            bone.rotation_mode="QUATERNION"
            bone.rotation_quaternion=(1,0,0,0)
            bone.location=(0,0,0)
        for side,sign in [("l",1),("r",-1)]:
            pose_world(rig,"upperarm_"+side,(0,1,0),sign*0.52)
            if walk:
                bone=rig.pose.bones["upperarm_"+side]
                basis=bone.bone.matrix_local.to_quaternion()
                bone.rotation_quaternion=basis.inverted() @ (
                    Quaternion(Vector((1,0,0)),sign*0.17*math.sin(t)) @
                    Quaternion(Vector((0,1,0)),sign*0.52)) @ basis
                pose_world(rig,"thigh_"+side,(1,0,0),sign*0.25*math.sin(t))
                pose_world(rig,"calf_"+side,(1,0,0),max(0,sign*math.sin(t))*0.32)
                pose_world(rig,"foot_"+side,(1,0,0),-max(0,sign*math.sin(t))*0.10)
        pose_world(rig,"spine_03",(1,0,0),0.008*math.sin(t))
        if walk:
            bpy.context.view_layer.update()
            graph=bpy.context.evaluated_depsgraph_get()
            feet=[bpy.data.objects["Heroine_LaceupShoes"].evaluated_get(graph)]
            minimum=min((o.matrix_world@v.co).z for o in feet for v in o.data.vertices)
            rig.pose.bones["Root"].location.z=0.004-minimum
        rig.pose.bones["Root"].keyframe_insert(data_path="location",frame=frame,group="Root")
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion",frame=frame,group=bone.name)
    return action


def aim(obj, point):
    obj.rotation_euler = (Vector(point)-obj.location).to_track_quat("-Z","Y").to_euler()


def stage():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 40
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 1000
    scene.render.resolution_y = 1200
    scene.render.resolution_percentage = 100
    if scene.world is None:
        scene.world=bpy.data.worlds.new("CharacterPreviewWorld")
    scene.world.color = (0.18,0.18,0.18)
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.12,0.16,0.21,1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.45
    floor_mat = material("PreviewGround", (0.085,0.095,0.088),0.85)
    bpy.ops.mesh.primitive_plane_add(size=200)
    floor = bpy.context.object
    floor.name = "PreviewGround"
    assign(floor, floor_mat)
    for name, loc, energy, color, size in [
        ("Key",(-2,-3,4),300,(1,0.89,0.78),4),
        ("Fill",(2,-1,2),120,(0.72,0.83,1),3),
        ("Rim",(0,2,3),250,(1,0.92,0.78),2)]:
        data = bpy.data.lights.new(name,"AREA")
        data.energy, data.color, data.shape, data.size = energy, color, "DISK", size
        light = bpy.data.objects.new(name,data)
        bpy.context.collection.objects.link(light)
        light.location = loc
        aim(light,(0,0,1))
    data = bpy.data.cameras.new("PreviewCamera")
    cam = bpy.data.objects.new("PreviewCamera",data)
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    data.type="ORTHO"
    data.ortho_scale=1.92
    return cam


def render(cam, name, loc, look, scale=1.92):
    cam.location = loc
    cam.data.ortho_scale = scale
    aim(cam, look)
    bpy.context.scene.render.filepath = str(PREVIEW / (name + ".png"))
    bpy.ops.render.render(write_still=True)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--skip-render", action="store_true")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    OUT.mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    body, rig, parts, hairs, recipe = create_body()
    outfit = make_outfit(body, rig)
    idle=create_animation(rig,"AN_Heroine_Idle",91)
    create_animation(rig,"AN_Heroine_Walk",31,walk=True)
    rig.animation_data.action=idle
    rig.animation_data.action_slot=idle.slots[0]
    bpy.context.scene.frame_start=1
    bpy.context.scene.frame_end=91
    bpy.context.scene.frame_set(1)
    cam = stage()
    hairs["Bob"].hide_render = True
    hairs["Bob"].hide_set(True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.context.scene.render.fps = 30
    (OUT / "recipe.json").write_text(json.dumps(recipe, indent=2))
    report = {
        "meshes": {obj.name: {"vertices":len(obj.data.vertices), "faces":len(obj.data.polygons)}
                   for obj in parts+outfit+list(hairs.values())},
        "bones": {b.name: {"head":list(b.head_local),"tail":list(b.tail_local)} for b in rig.data.bones},
        "body_bounds": {axis:[min(getattr(v.co,axis) for v in body.data.vertices),
                              max(getattr(v.co,axis) for v in body.data.vertices)] for axis in "xyz"},
    }
    (PREVIEW / "authoring-report.json").write_text(json.dumps(report, indent=2))
    if not args.skip_render:
        render(cam,"heroine-long-front",(0,-4,1.1),(0,0,0.85))
        render(cam,"heroine-long-threequarter",(2,-4,1.7),(0,0,0.86))
        render(cam,"heroine-long-portrait",(0.35,-3,1.5),(0,-0.015,1.405),0.48)
    cam.location=(2,-4,1.7)
    cam.data.ortho_scale=1.92
    aim(cam,(0,0,0.86))
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "Heroine.blend"))
    print("HEROINE_AUTHORING_COMPLETE", str(OUT))


if __name__=="__main__":
    main()
