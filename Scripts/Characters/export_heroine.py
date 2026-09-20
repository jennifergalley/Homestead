"""Export joined, clothed full presets and original animations; validate by FBX round trip."""
import hashlib
import json
import math
import shutil
from pathlib import Path

import bmesh
import bpy

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/"Assets"/"Characters"/"Heroine"
PREVIEW=ROOT/"Build"/"CharacterPreview"


def active(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj


def reset(rig):
    rig.animation_data_clear()
    for bone in rig.pose.bones:
        bone.rotation_mode="QUATERNION"
        bone.rotation_quaternion=(1,0,0,0)
        bone.location=(0,0,0)
        bone.scale=(1,1,1)
    bpy.context.view_layer.update()


def export_fbx(path,rig,objects,animation=False):
    active(rig)
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=str(path),use_selection=True,object_types={"ARMATURE","MESH"},
        global_scale=1,apply_unit_scale=True,apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y",axis_up="Z",add_leaf_bones=False,
        primary_bone_axis="Y",secondary_bone_axis="X",use_armature_deform_only=True,
        bake_anim=animation,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0,bake_anim_step=1,path_mode="RELATIVE",
        use_mesh_modifiers=True,mesh_smooth_type="FACE")


def prepare_materials(objects):
    directory=OUT/"Textures"
    directory.mkdir(exist_ok=True)
    materials={}
    for obj in objects:
        for mat in obj.data.materials:
            if not mat or mat.name in materials:
                continue
            shader=next(n for n in mat.node_tree.nodes if n.type=="BSDF_PRINCIPLED")
            record={
                "base_color":list(shader.inputs["Base Color"].default_value),
                "roughness":shader.inputs["Roughness"].default_value,
                "metallic":shader.inputs["Metallic"].default_value,
                "specular":shader.inputs["Specular IOR Level"].default_value,
                "two_sided":True,
                "alpha_mask":bool(shader.inputs["Alpha"].is_linked),
                "texture":None,
            }
            images=[n for n in mat.node_tree.nodes if n.type=="TEX_IMAGE" and n.image]
            if images:
                image=images[0].image
                source=Path(bpy.path.abspath(image.filepath))
                dest=directory/source.name
                if source.resolve()!=dest.resolve():
                    shutil.copy2(source,dest)
                image.filepath=str(dest)
                record["texture"]=str(dest.relative_to(OUT))
                record["sha256"]=hashlib.sha256(dest.read_bytes()).hexdigest()
            materials[mat.name]=record
    (OUT/"materials.json").write_text(json.dumps(materials,indent=2))


def prepare_body(body):
    # Under-outfit skin is intentionally absent from the playable mesh, preventing
    # accidental unclothed gameplay and limiting cloth/skin intersections in motion.
    bm=bmesh.new()
    bm.from_mesh(body.data)
    hidden=[v for v in bm.verts if
            (0.725<v.co.z<1.155 and abs(v.co.x)<0.19)
            or v.co.z<0.145]
    bmesh.ops.delete(bm,geom=hidden,context="VERTS")
    bm.to_mesh(body.data)
    bm.free()


def validate(mesh,rig):
    bones={b.name for b in rig.data.bones}
    groups={g.index:g.name for g in mesh.vertex_groups}
    errors=[]
    max_influences=0
    for v in mesh.data.vertices:
        weights=[g.weight for g in v.groups if groups[g.group] in bones and g.weight>1e-6]
        max_influences=max(max_influences,len(weights))
        if not weights or abs(sum(weights)-1)>0.015 or len(weights)>4:
            errors.append(v.index)
    if errors:
        raise RuntimeError(f"Invalid normalized <=4 bone influences on {len(errors)} vertices")
    if len([b for b in rig.data.bones if b.parent is None])!=1:
        raise RuntimeError("Expected exactly one root")
    return {"vertices":len(mesh.data.vertices),"triangles":sum(len(p.vertices)-2 for p in mesh.data.polygons),
            "bones":len(bones),"max_influences":max_influences,"material_slots":len(mesh.data.materials)}


def normalize(obj,rig):
    bones={b.name for b in rig.data.bones}
    for group in list(obj.vertex_groups):
        if group.name not in bones:
            obj.vertex_groups.remove(group)
    for v in obj.data.vertices:
        original=[(g.group,g.weight) for g in v.groups]
        weights=sorted([(group,weight) for group,weight in original if weight>1e-6],key=lambda g:-g[1])[:4]
        for group,_ in original:
            obj.vertex_groups[group].remove([v.index])
        if not weights:
            raise RuntimeError(f"Unweighted vertex {obj.name}:{v.index}")
        total=sum(w for _,w in weights)
        for group,weight in weights:
            obj.vertex_groups[group].add([v.index],weight/total,"REPLACE")


def main():
    results={"presets":{},"animations":{},"coordinate_contract":{
        "blender":"metres, +Z up, character faces -Y",
        "fbx":"FBX_SCALE_UNITS, -Y forward, +Z up; importer converts scene units to cm",
        "intended_body_height_cm":160,"unreal_expected_facing":"+X after FBX scene conversion",
        "rig":"MakeHuman game_engine (not an Epic Manny compatible skeleton)"}}
    for style in ["LongWave","Bob"]:
        bpy.ops.wm.open_mainfile(filepath=str(OUT/"Heroine.blend"))
        rig=bpy.data.objects["Heroine_Rig"]
        if rig.get("AdultPhenotypeAnchor")!=0.5:
            raise RuntimeError("Rebuild the verified adult phenotype before exporting")
        reset(rig)
        remove={"Heroine_Hair_Bob"} if style=="LongWave" else {"Heroine_Hair_LongWave"}
        objects=[o for o in bpy.data.objects if o.type=="MESH" and o.name.startswith("Heroine_")
                 and o.name not in remove]
        if style=="LongWave":
            # Include Bob material in the shared manifest even though hidden in the default preset.
            prepare_materials(objects+[bpy.data.objects["Heroine_Hair_Bob"]])
        for obj in objects:
            active(obj)
            if obj.name=="Heroine_Body":
                prepare_body(obj)
            for modifier in list(obj.modifiers):
                if modifier.type!="ARMATURE":
                    bpy.ops.object.modifier_apply(modifier=modifier.name)
            normalize(obj,rig)
        active(objects[0])
        for obj in objects:
            obj.select_set(True)
        bpy.ops.object.join()
        combined=bpy.context.object
        combined.name="SK_Heroine_"+style
        normalize(combined,rig)
        results["presets"][style]=validate(combined,rig)
        export_fbx(OUT/(combined.name+".fbx"),rig,[combined])
        if style=="LongWave":
            bpy.ops.wm.save_as_mainfile(filepath=str(PREVIEW/"ExportRig.blend"))
    for name in ["AN_Heroine_Idle","AN_Heroine_Walk"]:
        bpy.ops.wm.open_mainfile(filepath=str(PREVIEW/"ExportRig.blend"))
        rig=bpy.data.objects["Heroine_Rig"]
        action=bpy.data.actions[name]
        rig.animation_data_create()
        rig.animation_data.action=action
        rig.animation_data.action_slot=action.slots[0]
        scene=bpy.context.scene
        scene.frame_start=int(action.frame_range[0])
        scene.frame_end=int(action.frame_range[1])
        scene.frame_set(scene.frame_start)
        # Keep a clothed reference mesh so animation FBXs carry an unambiguous
        # skin bind pose. Armature-only FBX can acquire a posed reference frame.
        export_fbx(OUT/(name+".fbx"),rig,[bpy.data.objects["SK_Heroine_LongWave"]],animation=True)
        results["animations"][name]={"frames":scene.frame_end-scene.frame_start+1,"fps":scene.render.fps,
                                     "duration_seconds":(scene.frame_end-scene.frame_start)/scene.render.fps,
                                     "motion":"in-place original procedural prototype"}
    # A fresh FBX import verifies the interchange file rather than merely the authoring scene.
    for style in ["LongWave","Bob"]:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(OUT/("SK_Heroine_"+style+".fbx")))
        rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
        obj=next(o for o in bpy.data.objects if o.type=="MESH")
        bpy.context.view_layer.update()
        check=validate(obj,rig)
        height=max((obj.matrix_world@v.co).z for v in obj.data.vertices)-min(
            (obj.matrix_world@v.co).z for v in obj.data.vertices)
        if not 1.55<height<1.75:
            raise RuntimeError(f"Unexpected FBX round-trip height: {height}")
        check["height_m"]=height
        check["rigged"]=any(m.type=="ARMATURE" and m.object==rig for m in obj.modifiers)
        if not check["rigged"]:
            raise RuntimeError("FBX round trip lost armature binding")
        results["presets"][style]["round_trip"]=check
    for name in ["AN_Heroine_Idle","AN_Heroine_Walk"]:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.context.scene.render.fps=30
        bpy.ops.import_scene.fbx(filepath=str(OUT/(name+".fbx")))
        rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
        action=rig.animation_data.action
        expected=results["animations"][name]
        duration=(action.frame_range[1]-action.frame_range[0])/bpy.context.scene.render.fps
        if abs(duration-expected["duration_seconds"])>0.001:
            raise RuntimeError(f"Animation duration did not survive FBX: {name}: {duration}")
        bpy.context.scene.frame_set(int(action.frame_range[0]))
        first={b.name:b.matrix.copy() for b in rig.pose.bones}
        idle_arm_angles={}
        if name=="AN_Heroine_Idle":
            for side in ["l","r"]:
                arm=rig.pose.bones["upperarm_"+side]
                direction=rig.matrix_world.to_3x3()@(arm.tail-arm.head)
                angle=math.degrees(math.atan2(abs(direction.x),-direction.z))
                idle_arm_angles[side]=angle
                if angle>30:
                    raise RuntimeError(f"Idle lost relaxed arm pose during FBX export: {side} {angle}")
        bpy.context.scene.frame_set(int(action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])/4))
        changed=[b.name for b in rig.pose.bones if (first[b.name].to_quaternion().rotation_difference(
            b.matrix.to_quaternion())).angle>0.0005]
        if not changed:
            raise RuntimeError(f"Animation is static after export: {name}")
        bpy.context.scene.frame_set(int(action.frame_range[1]))
        loop_error=max((first[b.name].translation-b.matrix.translation).length for b in rig.pose.bones)
        if loop_error>0.001:
            raise RuntimeError(f"Non-looping animation endpoints: {name}: {loop_error}")
        expected["round_trip"]={"duration_seconds":duration,"moving_bones":changed,"loop_error_m":loop_error,
                                "idle_upperarm_degrees_from_vertical":idle_arm_angles}
    for file in OUT.glob("*.fbx"):
        results.setdefault("files",{})[file.name]={"bytes":file.stat().st_size,
            "sha256":hashlib.sha256(file.read_bytes()).hexdigest()}
    (PREVIEW/"export-validation.json").write_text(json.dumps(results,indent=2))
    print("HEROINE_EXPORT_VERIFIED",json.dumps(results))


if __name__=="__main__":
    main()
