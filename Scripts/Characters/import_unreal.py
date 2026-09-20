"""Run ONLY after Unreal installation is complete, with Python Editor Script Plugin enabled.

UnrealEditor-Cmd.exe SurvivalGame.uproject -run=pythonscript -script=<this file> -unattended -nosplash
No engine installation, plugin configuration, gameplay code or external content is modified.
"""
import json
import math
from pathlib import Path

import unreal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"Assets"/"Characters"/"Heroine"
DEST="/Game/SurvivalGame/Characters/Heroine"
REPORT=ROOT/"Build"/"CharacterPreview"/"unreal-import-report.json"
CONTRACT=SOURCE/"appearance-contract.json"
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
LIB=unreal.EditorAssetLibrary
ROLES={
    "M_Heroine_Skin":"skin",
    "M_Heroine_Hair_long01":"hair",
    "M_Heroine_Hair_bob01":"hair",
    "M_Heroine_Eyebrows":"brows",
    "M_Heroine_LightEyes":"eyes",
    "M_Heroine_MossLinen":"tunic",
    "M_Heroine_LinenTrim":"tunic_trim",
}


def save_checked(asset):
    if not LIB.save_loaded_asset(asset,only_if_is_dirty=False):
        raise RuntimeError("Asset save failed: "+asset.get_path_name())


def expression(mat,kind,x,y):
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,kind,x,y)
    if node is None:
        raise RuntimeError(f"Expression creation failed: {mat.get_name()} {kind}")
    return node


def connect(source,output,target,input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source,output,target,input_name):
        available=unreal.MaterialEditingLibrary.get_material_expression_input_names(target)
        raise RuntimeError(f"Material connection failed: {source.get_name()}.{output} -> {target.get_name()}.{input_name}; inputs={available}")


def output_property(node,output,prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(node,output,prop):
        raise RuntimeError(f"Material output connection failed: {node.get_name()} -> {prop}")


def constant(mat,value,x,y):
    node=expression(mat,unreal.MaterialExpressionConstant,x,y)
    node.set_editor_property("r",value)
    return node


def reference_bones(component):
    names=[str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
    required=["Root","head","foot_l","foot_r","ball_l","ball_r"]
    if not all(name in names for name in required):
        raise RuntimeError("Required reference bones missing: "+str(names))
    positions={}
    for name in required:
        point=unreal.Vector(0,0,0)
        current=name
        visited=set()
        while current in names:
            if current in visited:
                raise RuntimeError("Cycle in reference skeleton")
            visited.add(current)
            transform=component.get_ref_pose_transform(component.get_bone_index(current))
            point=transform.transform_location(point)
            current=str(component.get_parent_bone(current))
        positions[name]=[point.x,point.y,point.z]
    forward=[sum(positions["ball_"+side][i]-positions["foot_"+side][i] for side in ["l","r"])/2
             for i in range(2)]
    length=math.hypot(*forward)
    if length<0.001:
        raise RuntimeError("Degenerate reference-pose foot direction")
    forward=[value/length for value in forward]+[0.0]
    yaw=math.degrees(math.atan2(forward[1],forward[0]))
    return names,{"reference_positions_cm":positions,"component_space_forward":forward,
                  "yaw_degrees_from_positive_x":yaw,"mesh_yaw_to_face_positive_x":-yaw,
                  "method":"Reference local transforms composed through all parents; average ball-minus-foot projected onto XY"}


def iris_recolor(mat,source):
    # Select chromatic, mid-brightness iris pixels, not white sclera or dark pupils.
    channels=[]
    for i,channel in enumerate("rgb"):
        node=expression(mat,unreal.MaterialExpressionComponentMask,-1200,i*110)
        for c in "rgba":
            node.set_editor_property(c,c==channel)
        connect(source,"RGB",node,"")
        channels.append(node)
    def binary(kind,a,b,x,y):
        node=expression(mat,kind,x,y)
        connect(a,"",node,"A")
        connect(b,"",node,"B")
        return node
    maximum=binary(unreal.MaterialExpressionMax,channels[0],channels[1],-1000,0)
    maximum=binary(unreal.MaterialExpressionMax,maximum,channels[2],-800,0)
    minimum=binary(unreal.MaterialExpressionMin,channels[0],channels[1],-1000,250)
    minimum=binary(unreal.MaterialExpressionMin,minimum,channels[2],-800,250)
    chroma=binary(unreal.MaterialExpressionSubtract,maximum,minimum,-600,250)
    def ramp(value,offset,reverse,y):
        threshold=constant(mat,offset,-600,y+90)
        diff=binary(unreal.MaterialExpressionSubtract,
                    threshold if reverse else value,value if reverse else threshold,-400,y)
        scaled=binary(unreal.MaterialExpressionMultiply,diff,constant(mat,8,-400,y+90),-200,y)
        clamped=expression(mat,unreal.MaterialExpressionSaturate,0,y)
        connect(scaled,"",clamped,"")
        return clamped
    chromatic=ramp(chroma,0.08,False,350)
    not_black=ramp(maximum,0.08,False,650)
    not_white=ramp(maximum,0.90,True,950)
    mask=binary(unreal.MaterialExpressionMultiply,chromatic,not_black,200,500)
    mask=binary(unreal.MaterialExpressionMultiply,mask,not_white,400,500)
    mix=expression(mat,unreal.MaterialExpressionScalarParameter,200,850)
    mix.set_editor_property("parameter_name","IrisMix")
    mix.set_editor_property("default_value",0.0)
    clamped_mix=expression(mat,unreal.MaterialExpressionSaturate,400,850)
    connect(mix,"",clamped_mix,"")
    weight=binary(unreal.MaterialExpressionMultiply,mask,clamped_mix,600,500)
    color=expression(mat,unreal.MaterialExpressionVectorParameter,200,0)
    color.set_editor_property("parameter_name","IrisColor")
    color.set_editor_property("default_value",unreal.LinearColor(0.15,0.40,0.55,1))
    luminance=expression(mat,unreal.MaterialExpressionDesaturation,-600,-250)
    connect(source,"RGB",luminance,"")
    connect(constant(mat,1,-800,-350),"",luminance,"Fraction")
    shaded=binary(unreal.MaterialExpressionMultiply,color,luminance,400,0)
    lerp=expression(mat,unreal.MaterialExpressionLinearInterpolate,800,0)
    connect(source,"RGB",lerp,"A")
    connect(shaded,"",lerp,"B")
    connect(weight,"",lerp,"Alpha")
    return lerp


def import_task(filename,directory,name,options=None):
    task=unreal.AssetImportTask()
    task.set_editor_property("filename",str(filename))
    task.set_editor_property("destination_path",directory)
    task.set_editor_property("destination_name",name)
    task.set_editor_property("automated",True)
    task.set_editor_property("replace_existing",True)
    task.set_editor_property("replace_existing_settings",True)
    task.set_editor_property("save",True)
    if options:
        task.set_editor_property("options",options)
        # Explicit legacy FBX factory keeps FbxImportUI options authoritative.
        task.set_editor_property("factory",unreal.FbxFactory())
    TOOLS.import_asset_tasks([task])
    paths=list(task.get_editor_property("imported_object_paths"))
    if not paths:
        raise RuntimeError(f"Import produced no assets: {filename}")
    return paths


def fbx_options(skeleton=None,animation=False):
    options=unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type",False)
    options.set_editor_property("import_as_skeletal",True)
    options.set_editor_property("import_mesh",not animation)
    options.set_editor_property("import_animations",animation)
    options.set_editor_property("import_materials",False)
    options.set_editor_property("import_textures",False)
    options.set_editor_property("create_physics_asset",False)
    options.set_editor_property("mesh_type_to_import",
        unreal.FBXImportType.FBXIT_ANIMATION if animation else unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    if skeleton:
        options.set_editor_property("skeleton",skeleton)
    data=options.get_editor_property("anim_sequence_import_data" if animation else "skeletal_mesh_import_data")
    data.set_editor_property("convert_scene",True)
    data.set_editor_property("convert_scene_unit",True)
    data.set_editor_property("import_uniform_scale",1.0)
    return options


def create_material(name,record):
    path=DEST+"/Materials/"+name
    mat=LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if not mat:
        mat=TOOLS.create_asset(name,DEST+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
    if not mat:
        raise RuntimeError(f"Could not create character material: {name}")
    edit=unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(mat)
    mat.set_editor_property("used_with_skeletal_mesh",True)
    mat.set_editor_property("two_sided",record["two_sided"])
    mat.set_editor_property("blend_mode",
        unreal.BlendMode.BLEND_MASKED if record["alpha_mask"] else unreal.BlendMode.BLEND_OPAQUE)
    if record["texture"]:
        file=SOURCE/record["texture"]
        texture_name="T_"+file.stem
        paths=import_task(file,DEST+"/Textures",texture_name)
        texture=next((LIB.load_asset(p) for p in paths if isinstance(LIB.load_asset(p),unreal.Texture2D)),None)
        if not texture:
            raise RuntimeError(f"Missing texture after import: {file}")
        save_checked(texture)
        node=expression(mat,unreal.MaterialExpressionTextureSample,-1600,0)
        node.set_editor_property("texture",texture)
        base_output="RGB"
        if record["alpha_mask"]:
            output_property(node,"A",unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        node=expression(mat,unreal.MaterialExpressionConstant3Vector,-1600,0)
        node.set_editor_property("constant",unreal.LinearColor(*record["base_color"]))
        base_output=""
    role=ROLES.get(name,"fixed")
    if role in {"skin","hair","brows","tunic","tunic_trim"}:
        tint=expression(mat,unreal.MaterialExpressionVectorParameter,-1300,-180)
        tint.set_editor_property("parameter_name","ColorTint")
        tint.set_editor_property("default_value",unreal.LinearColor(1,1,1,1))
        tinted=expression(mat,unreal.MaterialExpressionMultiply,-1000,0)
        connect(node,base_output,tinted,"A")
        connect(tint,"",tinted,"B")
        node,base_output=tinted,""
    elif role=="eyes":
        node,base_output=iris_recolor(mat,node),""
    output_property(node,base_output,unreal.MaterialProperty.MP_BASE_COLOR)
    for index,(key,prop) in enumerate([
        ("roughness",unreal.MaterialProperty.MP_ROUGHNESS),
        ("metallic",unreal.MaterialProperty.MP_METALLIC),
        ("specular",unreal.MaterialProperty.MP_SPECULAR)]):
        value=expression(mat,unreal.MaterialExpressionConstant,-1600,500+index*100)
        value.set_editor_property("r",record[key])
        output_property(value,"",prop)
    edit.recompile_material(mat)
    if not mat.get_editor_property("used_with_skeletal_mesh"):
        raise RuntimeError("Missing packaged skeletal-mesh material usage: "+name)
    vectors={str(n) for n in edit.get_vector_parameter_names(mat)}
    scalars={str(n) for n in edit.get_scalar_parameter_names(mat)}
    expected_vectors={"ColorTint"} if role in {"skin","hair","brows","tunic","tunic_trim"} else (
        {"IrisColor"} if role=="eyes" else set())
    expected_scalars={"IrisMix"} if role=="eyes" else set()
    if not expected_vectors.issubset(vectors) or not expected_scalars.issubset(scalars):
        raise RuntimeError(f"Missing compiled appearance parameters: {name}")
    if "ColorTint" in expected_vectors:
        value=edit.get_material_default_vector_parameter_value(mat,"ColorTint")
        if any(abs(channel-1)>1e-6 for channel in [value.r,value.g,value.b,value.a]):
            raise RuntimeError("Non-neutral ColorTint default: "+name)
    if "IrisMix" in expected_scalars and abs(edit.get_material_default_scalar_parameter_value(mat,"IrisMix"))>1e-6:
        raise RuntimeError("Non-neutral IrisMix default: "+name)
    save_checked(mat)
    return mat


def main():
    REPORT.unlink(missing_ok=True)
    CONTRACT.unlink(missing_ok=True)
    # UE 5.8 can route FBX tasks through Interchange despite an FbxFactory.
    # Keep this process's FBX operations on the backend matching FbxImportUI.
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world,"Interchange.FeatureFlags.Import.FBX 0")
    for directory in ["Materials","Textures","Animations"]:
        path=DEST+"/"+directory
        if not LIB.does_directory_exist(path) and not LIB.make_directory(path):
            raise RuntimeError("Could not create destination: "+path)
    manifest=json.loads((SOURCE/"materials.json").read_text())
    materials={name:create_material(name,record) for name,record in manifest.items()}
    result={"engine_version":unreal.SystemLibrary.get_engine_version(),"meshes":{},"animations":{},
            "runtime_integration":"Not performed by this script; preview asset import only."}
    skeleton=None
    contract={"schema":1,"material_defaults_verified":True,"used_with_skeletal_mesh":True,"meshes":{},"parameters":{
        "ColorTint":{"type":"vector","default":[1,1,1,1],"operation":"authored RGB multiplied by tint; alpha unchanged"},
        "IrisColor":{"type":"vector","default":[0.15,0.40,0.55,1],"operation":"brightness-shaded iris target"},
        "IrisMix":{"type":"scalar","default":0,"range":[0,1],"operation":"masked iris interpolation; zero preserves original RGB"}
    },"iris_mask":"saturate(8*(maxRGB-minRGB-.08))*saturate(8*(maxRGB-.08))*saturate(8*(.90-maxRGB))",
        "limits":["Multiplicative tint is not absolute color replacement and cannot brighten black texels.",
                  "Iris recolor is a blue-atlas chroma/brightness heuristic, not a semantic iris segmentation.",
                  "Sclera whites and pupil blacks are protected by the mask; alpha remains the source texture alpha."]}
    for style in ["LongWave","Bob"]:
        name="SK_Heroine_"+style
        mesh_path=DEST+"/"+name
        mesh=LIB.load_asset(mesh_path) if LIB.does_asset_exist(mesh_path) else None
        geometry_imported=False
        if mesh is None or not 155<mesh.get_bounds().box_extent.z*2<175:
            if mesh:
                # Atomic FBX reimport can prefer the asset's cached settings over
                # task options. Set both explicitly when repairing/importing geometry.
                data=mesh.get_editor_property("asset_import_data")
                if isinstance(data,unreal.FbxAssetImportData):
                    data.set_editor_property("convert_scene",True)
                    data.set_editor_property("convert_scene_unit",True)
                    data.set_editor_property("import_uniform_scale",1.0)
            paths=import_task(SOURCE/(name+".fbx"),DEST,name,fbx_options(skeleton))
            mesh=next((LIB.load_asset(p) for p in paths if isinstance(LIB.load_asset(p),unreal.SkeletalMesh)),None)
            geometry_imported=True
        if mesh is None:
            raise RuntimeError(f"No skeletal mesh for {style}")
        imported_skeleton=mesh.get_editor_property("skeleton")
        if imported_skeleton is None:
            raise RuntimeError("Mesh has no skeleton; restore/reimport from the retained FBX before appearance-only updates: "+mesh_path)
        if skeleton is None:
            skeleton=imported_skeleton
        if skeleton!=imported_skeleton:
            raise RuntimeError("Preset imported onto a different skeleton")
        # FBX task imported_object_paths/save does not reliably include its newly
        # created skeleton package. Persist it before saving referencing assets.
        save_checked(skeleton)
        slots=mesh.get_editor_property("materials")
        slot_contract=[]
        for index,slot in enumerate(slots):
            name=str(slot.get_editor_property("material_slot_name"))
            if name not in materials:
                raise RuntimeError(f"Unrecognized exported material slot: {name}")
            slot.set_editor_property("material_interface",materials[name])
            slots[index]=slot
            role=ROLES.get(name,"fixed")
            slot_contract.append({"index":index,"slot_name":name,"material":materials[name].get_path_name(),
                                  "role":role,"alpha_mask":manifest[name]["alpha_mask"],
                                  "appearance_group":{"brows":"hair","tunic_trim":"tunic"}.get(role,role),
                                  "parameters":["IrisColor","IrisMix"] if role=="eyes" else (
                                      ["ColorTint"] if role!="fixed" else [])})
        mesh.set_editor_property("materials",slots)
        save_checked(mesh)
        component=unreal.SkeletalMeshComponent()
        component.set_skeletal_mesh_asset(mesh)
        bone_names,facing=reference_bones(component)
        contract["meshes"][style]={"asset":mesh.get_path_name(),"slots":slot_contract,
                                  "bone_names":bone_names,"facing_inference_bones":{
                                      "left_ankle":"foot_l","left_toe":"ball_l",
                                      "right_ankle":"foot_r","right_toe":"ball_r","head":"head"},
                                  "facing":facing}
        bounds=mesh.get_bounds()
        height=bounds.box_extent.z*2
        if not 155<height<175:
            raise RuntimeError(f"Incorrect imported scale: {height} cm")
        result["meshes"][style]={"asset":mesh.get_path_name(),"height_cm":height,
                                "skeleton":skeleton.get_path_name(),"material_slots":len(slots),
                                "geometry_imported":geometry_imported}
    for name in ["AN_Heroine_Idle","AN_Heroine_Walk"]:
        path=DEST+"/Animations/"+name
        animation=LIB.load_asset(path) if LIB.does_asset_exist(path) else None
        if animation is None or animation.get_editor_property("skeleton") is None:
            paths=import_task(SOURCE/(name+".fbx"),DEST+"/Animations",name,fbx_options(skeleton,animation=True))
            animation=next((LIB.load_asset(p) for p in paths if isinstance(LIB.load_asset(p),unreal.AnimSequence)),None)
        if not animation:
            raise RuntimeError(f"Animation import failed: {name}")
        if animation.get_editor_property("skeleton")!=skeleton:
            raise RuntimeError("Animation does not reference the shared skeleton: "+name)
        result["animations"][name]=animation.get_path_name()
        save_checked(animation)
    REPORT.parent.mkdir(parents=True,exist_ok=True)
    REPORT.write_text(json.dumps(result,indent=2))
    CONTRACT.write_text(json.dumps(contract,indent=2))
    unreal.log("HEROINE_IMPORT_VERIFIED "+str(REPORT))


if __name__=="__main__":
    main()
