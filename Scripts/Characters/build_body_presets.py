"""Two additive MPFB body/face presets. Offline only; original art/scripts are read-only."""
import hashlib
import importlib.util
import json
import math
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
ORIGINAL=ROOT/"Assets"/"Characters"/"Heroine"
OUT=ROOT/"Assets"/"Characters"/"BodyPresets"
PREVIEW=ROOT/"Build"/"CharacterPreview"/"BodyPresets"
BASE_RECIPE=json.loads((ORIGINAL/"recipe.json").read_text())

spec=importlib.util.spec_from_file_location("body_preset_variant_helpers",Path(__file__).with_name("extend_variants.py"))
variants=importlib.util.module_from_spec(spec)
spec.loader.exec_module(variants)
author=variants.author
exporter=variants.exporter
author.OUT=exporter.OUT=variants.OUT=OUT
author.PREVIEW=exporter.PREVIEW=variants.PREVIEW=PREVIEW

PRESETS={
    "Willow":{
        "description":"Slightly leaner, more athletic build; wider mouth and more defined cheek/jaw structure",
        "macro_changes":{"muscle":0.50,"weight":0.33,"cupsize":0.54},
        "target_changes":{
            "head/head-oval":0.10,"head/head-diamond":0.14,
            "chin/chin-width-decr":0.06,
            "mouth/mouth-upperlip-volume-incr":0.20,"mouth/mouth-lowerlip-volume-incr":0.24,
            "mouth/mouth-scale-horiz-incr":0.18,"nose/nose-scale-horiz-decr":0.06,
            "cheek/l-cheek-bones-incr":0.26,"cheek/r-cheek-bones-incr":0.26,
        },
    },
    "Hazel":{
        "description":"Slightly softer, fuller build; rounder cheeks, shorter chin and fuller lips",
        "macro_changes":{"muscle":0.30,"weight":0.45,"cupsize":0.64,"firmness":0.50},
        "target_changes":{
            "head/head-oval":0.10,"head/head-round":0.18,
            "chin/chin-width-decr":0.06,"chin/chin-height-decr":0.10,
            "mouth/mouth-upperlip-volume-incr":0.35,"mouth/mouth-lowerlip-volume-incr":0.38,
            "mouth/mouth-scale-horiz-incr":0.06,"nose/nose-scale-horiz-decr":0.12,
            "cheek/l-cheek-bones-incr":0.06,"cheek/r-cheek-bones-incr":0.06,
            "cheek/l-cheek-volume-incr":0.18,"cheek/r-cheek-volume-incr":0.18,
            "eyes/l-eye-height2-incr":0.08,"eyes/r-eye-height2-incr":0.08,
        },
    },
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def protect_originals():
    files=[]
    for directory in [ORIGINAL,ROOT/"Assets"/"Characters"/"Variants"]:
        files.extend(p for p in directory.rglob("*") if p.is_file())
    files.extend(ROOT/"Scripts"/"Characters"/name for name in
                 ["build_heroine.py","export_heroine.py","extend_variants.py","import_unreal.py"])
    files.append(ROOT/"docs"/"character-pipeline.md")
    return {str(p.relative_to(ROOT)):digest(p) for p in files}


def set_output(directory):
    directory.mkdir(parents=True,exist_ok=True)
    author.OUT=exporter.OUT=variants.OUT=directory


def body_measurements(reference,current):
    if reference.shape!=current.shape:
        raise RuntimeError("Body topology changed; vertexwise shape comparison is not valid")
    delta=np.linalg.norm(current-reference,axis=1)
    scale=float(np.sum(reference*current)/np.sum(reference*reference))
    nonuniform=np.linalg.norm(current-reference*scale,axis=1)
    face=reference[:,2]>1.36
    torso=(reference[:,2]>0.78)&(reference[:,2]<1.28)&(np.abs(reference[:,0])<0.22)
    result={
        "vertices":len(reference),
        "rms_displacement_m":float(np.sqrt(np.mean(delta**2))),
        "maximum_displacement_m":float(delta.max()),
        "best_uniform_scale":scale,
        "rms_after_best_uniform_scale_m":float(np.sqrt(np.mean(nonuniform**2))),
        "face_rms_displacement_m":float(np.sqrt(np.mean(delta[face]**2))),
        "torso_rms_displacement_m":float(np.sqrt(np.mean(delta[torso]**2))),
    }
    if result["rms_after_best_uniform_scale_m"]<0.001:
        raise RuntimeError("Preset is insufficiently different from a uniform scale")
    if result["face_rms_displacement_m"]<0.001 or result["torso_rms_displacement_m"]<0.001:
        raise RuntimeError("Both face and body must have a measurable non-color shape change")
    return result


def create_preset(name,definition):
    directory=OUT/name
    set_output(directory)
    bpy.ops.wm.open_mainfile(filepath=str(ORIGINAL/"Heroine.blend"))
    reference_rig=bpy.data.objects["Heroine_Rig"]
    exporter.reset(reference_rig)
    reference_signature=variants.rig_signature(reference_rig)
    reference_data=reference_rig.data.copy()
    reference_data.use_fake_user=True
    reference_matrix=reference_rig.matrix_world.copy()
    reference_body=np.array([tuple(v.co) for v in bpy.data.objects["Heroine_Body"].data.vertices])
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    # Keep the original actions, but avoid material-name suffixes from unreferenced originals.
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    macro=json.loads(json.dumps(BASE_RECIPE["macro"]))
    macro.update(definition["macro_changes"])
    targets=dict(BASE_RECIPE["targets"])
    targets.update(definition["target_changes"])
    human=author.service("HumanService")
    target_service=author.service("TargetService")
    stack=target_service.calculate_target_stack_from_macro_info_dict(macro)
    if macro["age"]!=0.5 or any(("-child" in t or "-baby" in t) and w>0 for t,w in stack):
        raise RuntimeError("Only the verified adult anchor is allowed")
    body=human.create_human(macro_detail_dict=macro)
    body.name="Heroine_Body"
    target_root=Path(author.service("LocationService").get_mpfb_data("targets"))
    for target,value in targets.items():
        path=target_root/(target+".target.gz")
        if not path.is_file():
            raise RuntimeError("Unavailable target: "+str(path))
        target_service.load_target(body,str(path),weight=value)
    rig=human.add_builtin_rig(body,"game_engine")
    rig.name="Heroine_Rig"
    rig["AdultPhenotypeAnchor"]=0.5
    skin=author.material("M_Heroine_Skin",(0.66,0.43,0.32),0.48,
        author.SOURCE/"skins"/"young_caucasian_female"/"young_lightskinned_female_diffuse.png")
    author.assign(body,skin)
    parts=[body]
    for folder,asset,kind in [
        ("eyes","high-poly","Eyes"),("eyebrows","eyebrow001","Eyebrows"),
        ("eyelashes","eyelashes01","Eyelashes"),("teeth","teeth_base","Teeth"),
        ("tongue","tongue01","Tongue"),
    ]:
        obj=author.add_asset(body,folder,asset,kind)
        obj.name="Heroine_"+kind
        parts.append(obj)
    hairs={}
    for style,asset in [("LongWave","long01"),("Bob","bob01"),("Ponytail","ponytail01")]:
        obj=author.add_asset(body,"hair",asset,"Hair")
        obj.name="Heroine_Hair_"+style
        hairs[style]=obj
    bpy.context.view_layer.update()
    evaluated=body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    full_height=max(v.co.z for v in evaluated.data.vertices)-min(v.co.z for v in evaluated.data.vertices)
    factor=1.60/full_height
    shoes=author.add_asset(body,"clothes","shoes01","Clothes")
    shoes.name="Heroine_LaceupShoes"
    shoes.data.materials[0].name="M_Heroine_LeatherShoes"
    parts.append(shoes)
    human.serialize_to_json_file(body,str(directory/"mpfb-authoring-preset.json"),save_clothes=True)
    for obj in parts+list(hairs.values()):
        author.freeze_mesh(obj)
        for vertex in obj.data.vertices:
            vertex.co*=factor
        obj.location*=factor
        author.smooth(obj)
    author.active(rig)
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in rig.data.edit_bones:
        bone.head*=factor
        bone.tail*=factor
    bpy.ops.object.mode_set(mode="OBJECT")
    joint_deltas={}
    for bone in rig.data.bones:
        original=reference_data.bones[bone.name]
        joint_deltas[bone.name]=max((bone.head_local-original.head_local).length,
                                   (bone.tail_local-original.tail_local).length)
    max_joint=max(joint_deltas.values())
    shared_bind=max_joint<=0.012
    if shared_bind:
        # Standard shared-skeleton skinning: retain the authored shape and weights,
        # use the existing rest skeleton only when anatomical pivots remain within 12 mm.
        rig.data=reference_data.copy()
        rig.matrix_world=reference_matrix
        variants.compare_rigs(reference_signature,variants.rig_signature(rig))
    else:
        # No mesh warping to force compatibility: keep the fitted anatomy and make matching clips.
        for action in list(bpy.data.actions):
            bpy.data.actions.remove(action)
    reference_data.use_fake_user=False
    for style,hair in hairs.items():
        asset={"LongWave":"long01","Bob":"bob01","Ponytail":"ponytail01"}[style]
        author.bind_bone(hair,rig,"head")
        texture=author.chestnut_texture(author.SOURCE/"hair"/asset/(asset+"_diffuse.png"),"T_"+style+"_Chestnut")
        mat=author.material("M_Heroine_Hair_"+asset,(0.12,0.048,0.021),0.70,texture,alpha=True)
        mat.node_tree.nodes.get("Principled BSDF").inputs["Specular IOR Level"].default_value=0.10
        author.assign(hair,mat)
    author.assign(parts[1],author.material("M_Heroine_LightEyes",(0.25,0.47,0.53),0.22,
                                         author.SOURCE/"eyes"/"materials"/"blue_eye.png",alpha=True))
    bvh=BVHTree.FromPolygons([v.co for v in body.data.vertices],[p.vertices[:] for p in body.data.polygons])
    for v in hairs["LongWave"].data.vertices:
        z=v.co.z
        fall=max(0,min(1,(1.49-z)/0.23))
        sign=1 if v.co.x>=0 else -1
        v.co.x+=sign*fall*(0.013+0.012*math.sin((1.5-z)*34))
        v.co.y+=fall*0.009*math.sin((1.5-z)*30+abs(v.co.x)*7)
        if z<1.34:
            radial=Vector((v.co.x,v.co.y,0)).normalized()
            hit,_,_,_=bvh.ray_cast(Vector((0,0,z)),radial,0.3)
            if hit:
                minimum=min(0.175,math.hypot(hit.x,hit.y))+0.022
                if math.hypot(v.co.x,v.co.y)<minimum:
                    v.co.x,v.co.y=radial.x*minimum,radial.y*minimum
    differences=body_measurements(reference_body,np.array([tuple(v.co) for v in body.data.vertices]))
    for obj in [body]+list(hairs.values()):
        mod=obj.modifiers.new("Surface refinement","SUBSURF")
        mod.levels=mod.render_levels=1
    author.make_outfit(body,rig)
    variants.make_apron(rig)
    if not shared_bind:
        author.create_animation(rig,"AN_"+name+"_Idle",91)
        author.create_animation(rig,"AN_"+name+"_Walk",31,walk=True)
    idle="AN_Heroine_Idle" if shared_bind else "AN_"+name+"_Idle"
    rig.animation_data_create()
    rig.animation_data.action=bpy.data.actions[idle]
    rig.animation_data.action_slot=rig.animation_data.action.slots[0]
    scene=bpy.context.scene
    scene.unit_settings.system="METRIC"
    scene.unit_settings.scale_length=1
    scene.render.fps=30
    scene.frame_start=1
    scene.frame_end=91
    scene.frame_set(1)
    variants.choose("LongWave",False)
    cam=author.stage()
    cam.location=(2,-4,1.6)
    author.aim(cam,(0,0,0.85))
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(directory/(name+".blend")))
    info={"description":definition["description"],"directory":name,"source_blend":name+".blend",
          "materials":"materials.json","macro":macro,"targets":targets,"height_m":1.60,
          "same_demographic_anchors_as_preferred":True,"adult_age_anchor":0.5,
          "maximum_fitted_joint_delta_m":max_joint,"fitted_joint_deltas_m":joint_deltas,
          "shared_preferred_bind":shared_bind,"shape_comparison_to_preferred":differences,
          "macro_target_stack":stack,"exports":{}}
    info["animation_contract"]={
        "mode":"reuse_preferred" if shared_bind else "preset_specific",
        "idle":str((ORIGINAL/"AN_Heroine_Idle.fbx").relative_to(ROOT)) if shared_bind else f"AN_{name}_Idle.fbx",
        "walk":str((ORIGINAL/"AN_Heroine_Walk.fbx").relative_to(ROOT)) if shared_bind else f"AN_{name}_Walk.fbx",
        "unreal_skeleton":"/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton" if shared_bind else None,
    }
    (directory/"recipe.json").write_text(json.dumps(info,indent=2))
    print("BODY_PRESET_AUTHORED",name,json.dumps({"shared_bind":shared_bind,"max_joint_delta":max_joint,**differences}),flush=True)
    return info


def export_preset(name,info):
    directory=OUT/name
    set_output(directory)
    materials={}
    for style in ["LongWave","Bob","Ponytail"]:
        for apron in [False,True]:
            asset=f"SK_Heroine_{name}_{style}"+("_Apron" if apron else "")
            bpy.ops.wm.open_mainfile(filepath=str(directory/(name+".blend")))
            rig=bpy.data.objects["Heroine_Rig"]
            exporter.reset(rig)
            variants.choose(style,apron)
            objects=[o for o in bpy.data.objects if o.type=="MESH" and o.name.startswith("Heroine_") and not o.hide_render]
            exporter.prepare_materials(objects)
            materials.update(json.loads((directory/"materials.json").read_text()))
            for obj in objects:
                exporter.active(obj)
                if obj.name=="Heroine_Body":
                    exporter.prepare_body(obj)
                for mod in list(obj.modifiers):
                    if mod.type!="ARMATURE":
                        bpy.ops.object.modifier_apply(modifier=mod.name)
                exporter.normalize(obj,rig)
            exporter.active(objects[0])
            for obj in objects:
                obj.select_set(True)
            bpy.ops.object.join()
            mesh=bpy.context.object
            mesh.name=asset
            exporter.normalize(mesh,rig)
            entry=exporter.validate(mesh,rig)
            exporter.export_fbx(directory/(asset+".fbx"),rig,[mesh])
            entry.update(fbx=asset+".fbx",hair=style,outfit="Apron" if apron else "Tunic",
                         sha256=digest(directory/(asset+".fbx")))
            info["exports"][asset]=entry
            if style=="LongWave" and not apron:
                bpy.ops.wm.save_as_mainfile(filepath=str(PREVIEW/(name+"_ExportRig.blend")))
    (directory/"materials.json").write_text(json.dumps(materials,indent=2))
    if not info["shared_preferred_bind"]:
        for suffix,frames in [("Idle",91),("Walk",31)]:
            bpy.ops.wm.open_mainfile(filepath=str(PREVIEW/(name+"_ExportRig.blend")))
            rig=bpy.data.objects["Heroine_Rig"]
            action=bpy.data.actions[f"AN_{name}_{suffix}"]
            rig.animation_data_create()
            rig.animation_data.action=action
            rig.animation_data.action_slot=action.slots[0]
            bpy.context.scene.frame_start=1
            bpy.context.scene.frame_end=frames
            bpy.context.scene.frame_set(1)
            file=directory/(f"AN_{name}_{suffix}.fbx")
            mesh=bpy.data.objects[f"SK_Heroine_{name}_LongWave"]
            exporter.export_fbx(file,rig,[mesh],animation=True)
    return info


def assign_clip(rig,path):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    added=set(bpy.data.objects)-before
    source=next(o for o in added if o.type=="ARMATURE")
    error=variants.compare_rigs(variants.rig_signature(rig),variants.rig_signature(source))
    action=source.animation_data.action
    slot=source.animation_data.action_slot
    rig.animation_data_create()
    rig.animation_data.action=action
    rig.animation_data.action_slot=slot
    for obj in added:
        bpy.data.objects.remove(obj,do_unlink=True)
    scene=bpy.context.scene
    scene.frame_start=int(action.frame_range[0])
    scene.frame_end=int(action.frame_range[1])
    scene.frame_set(scene.frame_start)
    return action,error


def check_and_render(name,info,preferred_signature):
    directory=OUT/name
    set_output(directory)
    reference=None
    for asset,entry in info["exports"].items():
        rig,mesh=variants.load_fbx(directory/entry["fbx"])
        signature=variants.rig_signature(rig)
        if reference is None:
            reference=signature
        check=exporter.validate(mesh,rig)
        check["bind_difference_across_combinations"]=variants.compare_rigs(reference,signature)
        if info["shared_preferred_bind"]:
            check["preferred_bind_max_error"]=variants.compare_rigs(preferred_signature,signature)
        height=max((mesh.matrix_world@v.co).z for v in mesh.data.vertices)-min(
            (mesh.matrix_world@v.co).z for v in mesh.data.vertices)
        if not 1.55<height<1.75:
            raise RuntimeError("Unexpected body-preset export height")
        check["height_m"]=height
        check["material_slots"]=[m.name for m in mesh.data.materials]
        variants.restore_materials(mesh)
        cam=author.stage()
        scene=bpy.context.scene
        scene.render.resolution_x=720
        scene.render.resolution_y=960
        scene.cycles.samples=18
        base=ORIGINAL if info["shared_preferred_bind"] else directory
        prefix="AN_Heroine_" if info["shared_preferred_bind"] else f"AN_{name}_"
        _,check["idle_bind_error"]=assign_clip(rig,base/(prefix+"Idle.fbx"))
        author.render(cam,asset+"-threequarter",(2,-4,1.6),(0,0,0.85))
        if entry["hair"]=="Ponytail" and entry["outfit"]=="Tunic":
            author.render(cam,name+"-portrait-front",(0,-3,1.47),(0,-0.015,1.46),0.36)
            author.render(cam,name+"-portrait-threequarter",(1.3,-3,1.48),(0,-0.015,1.46),0.40)
            author.render(cam,name+"-body-front",(0,-4,1.1),(0,0,0.85))
        if entry["outfit"]=="Apron":
            check["apron_clearance_idle"]=variants.apron_clearance(mesh)
        action,check["walk_bind_error"]=assign_clip(rig,base/(prefix+"Walk.fbx"))
        first={b.name:b.matrix.copy() for b in rig.pose.bones}
        scene.frame_set(int(action.frame_range[0])+7)
        moving=[b.name for b in rig.pose.bones if first[b.name].to_quaternion().rotation_difference(
            b.matrix.to_quaternion()).angle>0.0005]
        if not moving:
            raise RuntimeError("Exported preset did not animate")
        author.render(cam,asset+"-walk",(2,-4,1.6),(0,0,0.85))
        if entry["outfit"]=="Apron":
            check["apron_clearance_walk"]=variants.apron_clearance(mesh)
        check["moving_bones"]=moving
        entry["round_trip"]=check
    return info


def render_preferred_comparison():
    variants.OUT=ROOT/"Assets"/"Characters"/"Variants"
    rig,mesh=variants.load_fbx(variants.OUT/"SK_Heroine_Ponytail.fbx")
    variants.restore_materials(mesh)
    cam=author.stage()
    scene=bpy.context.scene
    scene.render.resolution_x=720
    scene.render.resolution_y=960
    scene.cycles.samples=18
    assign_clip(rig,ORIGINAL/"AN_Heroine_Idle.fbx")
    author.render(cam,"Preferred-portrait-front",(0,-3,1.47),(0,-0.015,1.46),0.36)
    author.render(cam,"Preferred-body-front",(0,-4,1.1),(0,0,0.85))


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    PREVIEW.mkdir(parents=True,exist_ok=True)
    manifest_path=OUT/"body-preset-manifest.json"
    manifest_path.unlink(missing_ok=True)
    before=protect_originals()
    manifest={"schema":1,"presets":{},"baseline_original_sha256":before,
              "engine_imported":False,
              "rights":"New shape recipes and original garment derivations over existing CC0 MakeHuman/MPFB assets. See Assets\\Characters\\provenance.json and Variants\\provenance.json.",
              "limits":["No new demographic assumptions: original adult age/race anchors, skin/eye/hair colors retained.",
                        "Measured face/torso deformations are real MPFB targets, not recolors or uniform scaling.",
                        "Shared original bind is used only when regenerated anatomical joint endpoints differ by at most 12 mm.",
                        "Clothing is regenerated/fitted to each body; no cloth/hair simulation or extreme-pose guarantee.",
                        "Body choice is complete presets, not independent runtime face/body sliders.",
                        "New exports have not been imported or exercised in Unreal."]}
    # Generate both before factory-reset FBX review so MPFB stays registered.
    for name,definition in PRESETS.items():
        manifest["presets"][name]=export_preset(name,create_preset(name,definition))
    rig,_=variants.load_fbx(ORIGINAL/"SK_Heroine_LongWave.fbx")
    preferred=variants.rig_signature(rig)
    for name in PRESETS:
        manifest["presets"][name]=check_and_render(name,manifest["presets"][name],preferred)
    render_preferred_comparison()
    if before!=protect_originals():
        raise RuntimeError("Original outputs changed during generation; inspect concurrent writes")
    manifest["original_files_unchanged"]=True
    manifest_path.write_text(json.dumps(manifest,indent=2))
    (PREVIEW/"validation.json").write_text(json.dumps(manifest["presets"],indent=2))
    print("BODY_PRESETS_VERIFIED",str(manifest_path),flush=True)


if __name__=="__main__":
    main()
