"""Additive offline variants; never writes the working Heroine assets or Unreal packages."""
import hashlib
import importlib.util
import json
import math
import shutil
import sys
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
ORIGINAL=ROOT/"Assets"/"Characters"/"Heroine"
OUT=ROOT/"Assets"/"Characters"/"Variants"
PREVIEW=ROOT/"Build"/"CharacterPreview"/"Variants"
PACK=ROOT/"Assets"/"Characters"/"Source"/"SystemAssets"
MASTER=OUT/"Heroine_Variants.blend"
VARIANTS=[
    ("SK_Heroine_Ponytail","Ponytail",False),
    ("SK_Heroine_LongWave_Apron","LongWave",True),
    ("SK_Heroine_Bob_Apron","Bob",True),
    ("SK_Heroine_Ponytail_Apron","Ponytail",True),
]


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    value=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


author=module("existing_authoring_helpers","build_heroine.py")
exporter=module("existing_export_helpers","export_heroine.py")
# Redirect only the helper module instances in this process, not their source files.
author.OUT=exporter.OUT=OUT
author.PREVIEW=exporter.PREVIEW=PREVIEW


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def original_hashes():
    files=list(ORIGINAL.rglob("*"))
    files.extend(ROOT/"Scripts"/"Characters"/n for n in
                 ["build_heroine.py","export_heroine.py","import_unreal.py"])
    files.append(ROOT/"docs"/"character-pipeline.md")
    return {str(p.relative_to(ROOT)):sha(p) for p in files if p.is_file()}


def rig_signature(rig):
    return {
        "object_matrix":[list(row) for row in rig.matrix_world],
        "bones":{b.name:{"parent":b.parent.name if b.parent else None,
                         "matrix":[list(row) for row in b.matrix_local]}
                 for b in rig.data.bones},
    }


def compare_rigs(expected,actual):
    if expected["bones"].keys()!=actual["bones"].keys():
        raise RuntimeError("Variant bone names differ from the working rig")
    error=0.0
    for name,record in expected["bones"].items():
        other=actual["bones"][name]
        if record["parent"]!=other["parent"]:
            raise RuntimeError("Changed bone parent: "+name)
        error=max(error,max(abs(record["matrix"][r][c]-other["matrix"][r][c])
                            for r in range(4) for c in range(4)))
    error=max(error,max(abs(expected["object_matrix"][r][c]-actual["object_matrix"][r][c])
                        for r in range(4) for c in range(4)))
    if error>1e-5:
        raise RuntimeError(f"Changed bind transform: maximum difference {error}")
    return error


def fitted_ponytail(rig,body):
    """Reconstruct only an ephemeral fitting donor from the saved exact recipe."""
    recipe=json.loads((ORIGINAL/"recipe.json").read_text())
    human=author.service("HumanService")
    targets=author.service("TargetService")
    donor=human.create_human(macro_detail_dict=recipe["macro"])
    donor.name="VariantFittingDonor"
    target_root=Path(author.service("LocationService").get_mpfb_data("targets"))
    for name,value in recipe["targets"].items():
        targets.load_target(donor,str(target_root/(name+".target.gz")),weight=value)
    bpy.context.view_layer.update()
    evaluated=donor.evaluated_get(bpy.context.evaluated_depsgraph_get())
    height=max(v.co.z for v in evaluated.data.vertices)-min(v.co.z for v in evaluated.data.vertices)
    factor=recipe["height_m"]/height
    tree=KDTree(len(body.data.vertices))
    for vertex in body.data.vertices:
        tree.insert(vertex.co,vertex.index)
    tree.balance()
    distances=[tree.find(vertex.co*factor)[2] for vertex in evaluated.data.vertices
               if vertex.co.z*factor>1.52]
    if not distances or max(distances)>1e-4:
        raise RuntimeError(f"Saved body cannot be faithfully reconstructed for hair fitting: {max(distances,default=-1)}")
    path=PACK/"hair"/"ponytail01"/"ponytail01.mhclo"
    hair=human.add_mhclo_asset(str(path),donor,asset_type="Hair",subdiv_levels=0,
                               material_type="GAMEENGINE",set_up_rigging=False,import_subrig=False)
    hair.name="Heroine_Hair_Ponytail"
    author.freeze_mesh(hair)
    for vertex in hair.data.vertices:
        vertex.co*=factor
    hair.location*=factor
    author.bind_bone(hair,rig,"head")
    texture=author.chestnut_texture(PACK/"hair"/"ponytail01"/"ponytail01_diffuse.png",
                                    "T_Ponytail_Chestnut")
    mat=author.material("M_Heroine_Hair_ponytail01",(0.12,0.048,0.021),0.70,texture,alpha=True)
    mat.node_tree.nodes.get("Principled BSDF").inputs["Specular IOR Level"].default_value=0.10
    author.assign(hair,mat)
    author.smooth(hair)
    sub=hair.modifiers.new("Surface refinement","SUBSURF")
    sub.levels=sub.render_levels=1
    bpy.data.objects.remove(donor,do_unlink=True)
    return hair,{"method":"Exact saved macro/detail recipe, ephemeral donor only; existing body and rig retained",
                 "crown_samples":len(distances),"maximum_crown_match_error_m":max(distances),
                 "normalization_factor":factor}


def apron_material():
    import numpy as np
    size=512
    y,x=np.mgrid[0:size,0:size].astype(np.float32)
    variation=1+0.025*np.sin(x*math.tau/5)+0.025*np.sin(y*math.tau/7)
    variation+=0.012*np.sin(x*0.037+y*0.022)
    pixels=np.ones((size,size,4),np.float32)
    pixels[:,:,:3]=variation[:,:,None]*np.array([0.67,0.60,0.46])
    image=bpy.data.images.new("T_ApronLinen",width=size,height=size,alpha=False)
    image.pixels.foreach_set(pixels.ravel())
    path=OUT/"Textures"/"T_ApronLinen.png"
    path.parent.mkdir(parents=True,exist_ok=True)
    image.filepath_raw=str(path)
    image.file_format="PNG"
    image.save()
    mat=author.material("M_Heroine_ApronLinen",(0.67,0.60,0.46),0.92,path)
    mat.node_tree.nodes.get("Principled BSDF").inputs["Specular IOR Level"].default_value=0.25
    return mat


def skirt_weights(obj,rig):
    for v in obj.data.vertices:
        if v.co.z>=0.91:
            continue
        for index in [group.group for group in v.groups]:
            obj.vertex_groups[index].remove([v.index])
        leg=max(0,min(0.70,(0.91-v.co.z)*2.5))
        left=max(0.05,min(0.95,0.5+v.co.x*3.5))
        for name,weight in [("pelvis",1-leg),("thigh_l",leg*left),("thigh_r",leg*(1-left))]:
            group=obj.vertex_groups.get(name) or obj.vertex_groups.new(name=name)
            group.add([v.index],weight,"REPLACE")


def make_apron(rig):
    dress=bpy.data.objects["Heroine_WrapTunic"]
    bvh=BVHTree.FromPolygons([v.co for v in dress.data.vertices],
                            [p.vertices[:] for p in dress.data.polygons])
    linen=apron_material()
    trim_mat=author.material("M_Heroine_ApronTrim",(0.74,0.67,0.53),0.86)
    def front_surface(x,z):
        # Below an asymmetric hem a ray can hit the BACK of the tunic.
        # Sample above every hem edge, then continue its radial flare downward.
        sample_z=max(0.745,z)
        hit,_,_,_=bvh.ray_cast(Vector((x,-0.5,sample_z)),Vector((0,1,0)),1)
        if hit is None or hit.y>=0:
            raise RuntimeError(f"Apron sample failed to find original tunic: {x},{sample_z}")
        radius=math.hypot(x,hit.y)+max(0,sample_z-z)*0.28
        return -math.sqrt(max(0,radius*radius-x*x))-0.024
    cols,rows=40,40
    vertices,faces=[],[]
    for j in range(rows+1):
        t=j/rows
        half_width=0.112+0.075*math.sin(t*math.pi/2)
        for i in range(cols+1):
            u=i/cols
            x=(2*u-1)*half_width
            z=0.984*(1-t)+0.525*t+0.014*(abs(2*u-1)**6)*t**8
            y=front_surface(x,z)-0.003*math.sin(u*math.pi*8)*t
            if y>=-0.035:
                raise RuntimeError("Apron panel folded behind the tunic")
            vertices.append((x,y,z))
    for j in range(rows):
        for i in range(cols):
            k=j*(cols+1)+i
            faces.append((k,k+cols+1,k+cols+2,k+1))
    apron=author.mesh("Heroine_ApronPanel",vertices,faces,linen)
    uv=apron.data.uv_layers.new(name="UVMap")
    for loop in apron.data.loops:
        j,i=divmod(loop.vertex_index,cols+1)
        uv.data[loop.index].uv=(i/cols*2,j/rows*3)
    author.skin_weights(apron,dress,rig)
    skirt_weights(apron,rig)
    pieces=[apron]
    # A narrow bound hem covers the lower edge, using exactly the same deformation.
    tv,tf=[],[]
    for j in range(2):
        for i in range(cols+1):
            p=Vector(vertices[rows*(cols+1)+i])
            p.y-=0.002
            p.z+=0.009*j
            tv.append(p)
    for i in range(cols):
        tf.append((i,i+cols+1,i+cols+2,i+1))
    trim=author.mesh("Heroine_ApronHem",tv,tf,trim_mat)
    author.skin_weights(trim,apron,rig)
    pieces.append(trim)
    # Slim waistband sits below, rather than covering, the original belt/buckle.
    bv,bf=[],[]
    count=96
    for z in [0.968,0.985]:
        for i in range(count):
            a=math.tau*i/count
            direction=Vector((math.cos(a),math.sin(a),0))
            hit,_,_,_=bvh.ray_cast(Vector((0,0,z)),direction,0.4)
            if hit is None:
                raise RuntimeError("Waistband fitting failed")
            bv.append(hit+direction*0.011)
    for i in range(count):
        n=(i+1)%count
        bf.append((i,n,count+n,count+i))
    band=author.mesh("Heroine_ApronWaistband",bv,bf,linen)
    author.skin_weights(band,dress,rig)
    pieces.append(band)
    # Two small hanging tie ends are original geometry, not another costume asset.
    for side in [-1,1]:
        sv,sf=[],[]
        for j in range(17):
            t=j/16
            z=0.98-0.19*t
            x=side*(0.015+0.042*t)
            hit,_,_,_=bvh.ray_cast(Vector((x,0.5,z)),Vector((0,-1,0)),1)
            if hit is None:
                raise RuntimeError("Apron tie fitting failed")
            y=hit.y+0.015+0.005*math.sin(t*math.pi)
            sv.extend([(x-0.012,y,z),(x+0.012,y,z)])
            if j:
                k=(j-1)*2
                sf.append((k,k+1,k+3,k+2))
        tie=author.mesh("Heroine_ApronTie_"+str(side),sv,sf,linen)
        author.skin_weights(tie,dress,rig)
        pieces.append(tie)
    return pieces


def choose(style,apron):
    for obj in bpy.data.objects:
        if obj.name.startswith("Heroine_Hair_"):
            hide=obj.name!="Heroine_Hair_"+style
        elif obj.name.startswith("Heroine_Apron"):
            hide=not apron
        else:
            continue
        obj.hide_render=hide
        obj.hide_set(hide)


def build_master():
    bpy.ops.wm.open_mainfile(filepath=str(ORIGINAL/"Heroine.blend"))
    rig=bpy.data.objects["Heroine_Rig"]
    exporter.reset(rig)
    signature=rig_signature(rig)
    body=bpy.data.objects["Heroine_Body"]
    body_points=[tuple(v.co) for v in body.data.vertices]
    _,fit=fitted_ponytail(rig,body)
    make_apron(rig)
    if body_points!=[tuple(v.co) for v in body.data.vertices]:
        raise RuntimeError("Original body geometry was modified")
    compare_rigs(signature,rig_signature(rig))
    choose("Ponytail",True)
    rig.animation_data_create()
    rig.animation_data.action=bpy.data.actions["AN_Heroine_Idle"]
    rig.animation_data.action_slot=rig.animation_data.action.slots[0]
    bpy.context.scene.frame_set(1)
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(MASTER))
    return fit


def export_variants():
    material_union={}
    stats={}
    for name,style,apron in VARIANTS:
        bpy.ops.wm.open_mainfile(filepath=str(MASTER))
        rig=bpy.data.objects["Heroine_Rig"]
        exporter.reset(rig)
        choose(style,apron)
        objects=[o for o in bpy.data.objects if o.type=="MESH" and o.name.startswith("Heroine_")
                 and not o.hide_render]
        exporter.prepare_materials(objects)
        material_union.update(json.loads((OUT/"materials.json").read_text()))
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
        combined=bpy.context.object
        combined.name=name
        exporter.normalize(combined,rig)
        stats[name]=exporter.validate(combined,rig)
        exporter.export_fbx(OUT/(name+".fbx"),rig,[combined])
    for name,record in material_union.items():
        if name=="M_Heroine_Hair_ponytail01":
            record.update(role="hair",appearance_group="hair",runtime_parameter="ColorTint",
                          runtime_default=[1,1,1,1])
        elif name.startswith("M_Heroine_Apron"):
            record.update(role="apron",appearance_group="tunic",runtime_parameter="ColorTint",
                          runtime_default=[1,1,1,1])
    (OUT/"materials.json").write_text(json.dumps(material_union,indent=2))
    return stats


def load_fbx(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps=30
    bpy.ops.import_scene.fbx(filepath=str(path))
    rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
    obj=next(o for o in bpy.data.objects if o.type=="MESH")
    return rig,obj


def clip(rig,name):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(ORIGINAL/(name+".fbx")))
    added=set(bpy.data.objects)-before
    imported=next(o for o in added if o.type=="ARMATURE")
    compare_rigs(rig_signature(rig),rig_signature(imported))
    action=imported.animation_data.action
    slot=imported.animation_data.action_slot
    rig.animation_data_create()
    rig.animation_data.action=action
    rig.animation_data.action_slot=slot
    for obj in added:
        bpy.data.objects.remove(obj,do_unlink=True)
    scene=bpy.context.scene
    scene.frame_start=int(action.frame_range[0])
    scene.frame_end=int(action.frame_range[1])
    scene.frame_set(scene.frame_start)
    return action


def restore_materials(obj):
    records=json.loads((OUT/"materials.json").read_text())
    for index,old in enumerate(obj.data.materials):
        record=records[old.name]
        texture=OUT/record["texture"] if record["texture"] else None
        mat=author.material(old.name+"_Review",record["base_color"][:3],record["roughness"],
                            texture,record["alpha_mask"])
        shader=mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Specular IOR Level"].default_value=record["specular"]
        shader.inputs["Metallic"].default_value=record["metallic"]
        obj.data.materials[index]=mat


def apron_clearance(obj):
    evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    data=evaluated.data
    vertices=[evaluated.matrix_world@v.co for v in data.vertices]
    tunic_faces=[p.vertices[:] for p in data.polygons if
                 obj.data.materials[p.material_index].name.startswith("M_Heroine_MossLinen")]
    apron_indices={i for p in data.polygons if
                   obj.data.materials[p.material_index].name.startswith("M_Heroine_Apron")
                   for i in p.vertices}
    tree=BVHTree.FromPolygons(vertices,tunic_faces)
    signed=[]
    low_nonfront=0
    for index in apron_indices:
        point=vertices[index]
        if point.z<0.70 and point.y>=-0.035:
            low_nonfront+=1
        if point.y>-0.035:
            continue
        hit,normal,_,distance=tree.find_nearest(point)
        signed.append((point-hit).dot(normal))
    return {"front_vertex_samples":len(signed),"minimum_signed_distance_m":min(signed),
            "samples_behind_nearest_tunic_surface":sum(v < -0.0005 for v in signed),
            "low_apron_nonfront_vertices":low_nonfront,
            "note":"Nearest-normal vertex diagnostic on the original tunic shell, not a full triangle/self-collision or cloth simulation test."}


def check_and_render(stats):
    baseline,_=load_fbx(ORIGINAL/"SK_Heroine_LongWave.fbx")
    reference=rig_signature(baseline)
    for name,style,apron in VARIANTS:
        rig,obj=load_fbx(OUT/(name+".fbx"))
        result=exporter.validate(obj,rig)
        result["maximum_bind_matrix_error"]=compare_rigs(reference,rig_signature(rig))
        height=max((obj.matrix_world@v.co).z for v in obj.data.vertices)-min(
            (obj.matrix_world@v.co).z for v in obj.data.vertices)
        if not 1.55<height<1.75:
            raise RuntimeError("Variant FBX scale mismatch")
        result["height_m"]=height
        result["material_slots"]=[m.name for m in obj.data.materials]
        restore_materials(obj)
        cam=author.stage()
        scene=bpy.context.scene
        scene.cycles.samples=20
        scene.render.resolution_x=720
        scene.render.resolution_y=960
        clip(rig,"AN_Heroine_Idle")
        if apron:
            result["apron_clearance"]={"idle":apron_clearance(obj)}
        author.render(cam,name+"-threequarter",(2,-4,1.6),(0,0,0.85))
        author.render(cam,name+"-back",(1.8,4,1.6),(0,0,0.85))
        if style=="Ponytail" and not apron:
            author.render(cam,name+"-portrait",(0.25,-3,1.5),(0,-0.015,1.405),0.50)
        if style=="Ponytail" and apron:
            author.render(cam,name+"-front",(0,-4,1.2),(0,0,0.85))
        action=clip(rig,"AN_Heroine_Walk")
        first={b.name:b.matrix.copy() for b in rig.pose.bones}
        frames=[int(action.frame_range[0])+7,int(action.frame_range[0])+22]
        for frame in frames:
            scene.frame_set(frame)
            changed=[b.name for b in rig.pose.bones if first[b.name].to_quaternion().rotation_difference(
                b.matrix.to_quaternion()).angle>0.0005]
            if not changed:
                raise RuntimeError("Existing walk did not animate variant")
            if apron:
                result["apron_clearance"]["walk_"+str(frame)]=apron_clearance(obj)
            author.render(cam,name+f"-walk-{frame:02}",(2,-4,1.6),(0,0,0.85))
        result["verified_clips"]=["AN_Heroine_Idle","AN_Heroine_Walk"]
        result["walk_review_frames"]=frames
        stats[name]["round_trip"]=result
        if name=="SK_Heroine_Ponytail_Apron":
            bpy.ops.file.pack_all()
            bpy.ops.wm.save_as_mainfile(filepath=str(PREVIEW/"Ponytail_Apron_ExportReview.blend"))
    return stats


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    PREVIEW.mkdir(parents=True,exist_ok=True)
    (OUT/"variant-manifest.json").unlink(missing_ok=True)
    (PREVIEW/"validation.json").unlink(missing_ok=True)
    before=original_hashes()
    fit=build_master()
    stats=check_and_render(export_variants())
    after=original_hashes()
    if before!=after:
        raise RuntimeError("An original file changed during additive variant work; inspect concurrent changes")
    manifest={"schema":1,"new_body_presets":0,"original_files_unchanged":True,
              "original_sha256":before,"hair_fitting":fit,"variants":{},"materials":"materials.json",
              "source_blend":"Heroine_Variants.blend","animation_sources":str(ORIGINAL.relative_to(ROOT)),
              "import_status":"Offline Blender export only; not imported into Unreal",
              "rig_contract":"Same 53 authoring bones and bind matrices as original FBX; existing Unreal imports use 54 including Heroine_Rig wrapper",
              "coverage":"Original clothed body mask and tunic retained for every export. Apron is additive; no extra skin is exposed.",
              "limits":["Ponytail is rigidly weighted to head, with no hair physics.",
                        "Apron uses approximate pelvis/thigh skinning, not cloth simulation.",
                        "Apron variants have different material slot ordering; bind colors by slot/material name, not original indices.",
                        "Only rest, idle and two existing walk poses rendered; crouching, tool swings and extreme poses are untested.",
                        "Hair/cloth intersections and card silhouettes require gameplay review; no high-fidelity claim.",
                        "New hair/apron materials require explicit skeletal usage and the declared neutral ColorTint when later imported."]}
    for name,style,apron in VARIANTS:
        manifest["variants"][name]={"fbx":name+".fbx","hair":style,"outfit":"Apron" if apron else "Tunic",
                                   "sha256":sha(OUT/(name+".fbx")),"validation":stats[name]}
    (OUT/"variant-manifest.json").write_text(json.dumps(manifest,indent=2))
    provenance={"source_pack":"../Source/SystemAssets","inherited_rights":"../provenance.json",
                "new_external_asset":{"name":"ponytail01","author":"MakeHuman Team / makehuman_system",
                    "copyright_holders":["Data Collection AB","Joel Palmius","Jonas Hauquier"],
                    "license":"CC0-1.0","source":"https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html",
                    "local_mhclo":str((PACK/"hair"/"ponytail01"/"ponytail01.mhclo").relative_to(ROOT)),
                    "modification":"Fit to unchanged saved adult phenotype; chestnut recolor retaining alpha",
                    "acquisition":"Existing offline official system pack; no new download"},
                "original_work":{"author":"Procedural authoring by GitHub Copilot for Jenny's project",
                                 "assets":["Half-apron panel, waistband, hem and ties","Generated linen texture"],
                                 "source":"Scripts\\Characters\\extend_variants.py"},
                "private_reference":"Not copied, embedded or uploaded by this task"}
    (OUT/"provenance.json").write_text(json.dumps(provenance,indent=2))
    (PREVIEW/"validation.json").write_text(json.dumps({"hair_fitting":fit,"variants":stats,
                                                     "original_files_unchanged":True},indent=2))
    print("OFFLINE_VARIANTS_VERIFIED",str(OUT/"variant-manifest.json"))


if __name__=="__main__":
    main()
