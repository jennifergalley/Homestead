"""Bounded CC0 bob01 adaptation; writes only BobReuse assets and CPU diagnostics."""
import argparse
import json
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np

sys.dont_write_bytecode = True
import importlib.util
spec = importlib.util.spec_from_file_location("bob_reuse_helpers",Path(__file__).with_name("build_hairstyle_refinement.py"))
recipe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)
modular = recipe.module("bob_reuse_modular_helpers","build_modular_hairstyle_refinement.py")
ROOT = recipe.ROOT
HAIR_ROOT = recipe.OUT
OUT = HAIR_ROOT/"BobReuse"
PREVIEW = recipe.PREVIEW/"BobReuse"
recipe.OUT = OUT
recipe.author.PREVIEW = PREVIEW
MATERIAL = "M_Heroine_Hair_bob01_Neutral"


def neutral_texture():
    source = recipe.CHAR/"Heroine"/"Textures"/"T_Bob_Chestnut.png"
    image = bpy.data.images.load(str(source),check_existing=False)
    pixels = np.asarray(image.pixels[:],dtype=np.float32).reshape((-1,4))
    red = np.clip(pixels[:,0],0,1)
    linear = np.where(red<=.04045,red/12.92,((red+.055)/1.055)**2.4)
    shade = np.clip(linear/(((.26+.055)/1.055)**2.4),0,1)
    srgb = np.where(shade<=.0031308,shade*12.92,1.055*shade**(1/2.4)-.055)
    assert float(srgb.max()-srgb.min())>.4
    pixels[:,:3] = srgb[:,None]
    output = bpy.data.images.new("T_BobReuse_Neutral",width=image.size[0],height=image.size[1],alpha=True)
    output.pixels.foreach_set(pixels.ravel())
    path = OUT/"Textures"/"T_BobReuse_Neutral.png"
    output.filepath_raw = str(path)
    output.file_format = "PNG"
    output.save()
    return path


def face_window(co):
    forehead = 1.527+.010*max(0,1-(abs(co.x)/.072)**2)
    return max(co.y+.105,abs(co.x)-.072,co.z-forehead)


def chin_end(co):
    return co.z-(1.403+.003*min(1,abs(co.x)/.12))


def clip(corners,field):
    result = []
    for i,(co,uv,normal) in enumerate(corners):
        prev,puv,pnormal = corners[i-1]
        inside,pinside = field(co)>=0,field(prev)>=0
        if inside != pinside:
            low,high = 0.,1.
            for _ in range(25):
                t = (low+high)/2
                if (field(prev.lerp(co,t))>=0)==pinside:
                    low = t
                else:
                    high = t
            t = (low+high)/2
            result.append((prev.lerp(co,t),puv.lerp(uv,t),pnormal.lerp(normal,t).normalized()))
        if inside:
            result.append((co,uv,normal))
    return result


def adapt_stock(mesh):
    positions,faces,uvs,normals = [],[],[],[]
    upper = set()
    original_count = 0
    face_removed = 0
    layer = mesh.data.uv_layers.active
    for p in recipe.hair_faces(mesh):
        original_count += 1
        corners = [(mesh.data.vertices[mesh.data.loops[i].vertex_index].co.copy(),
                    layer.data[i].uv.copy(),mesh.data.corner_normals[i].vector.copy())
                   for i in p.loop_indices]
        upper.update(tuple(co) for co,_,_ in corners if co.z>=1.555)
        corners = clip(clip(corners,face_window),chin_end)
        if len(corners)<3:
            face_removed += 1
            continue
        faces.append(tuple(range(len(positions),len(positions)+len(corners))))
        positions.extend(co for co,_,_ in corners)
        uvs.extend(uv for _,uv,_ in corners)
        normals.extend(n for _,_,n in corners)
    block = bpy.data.meshes.new("CC0Bob01TargetedFringeAdaptation")
    block.from_pydata(positions,[],faces)
    uv = block.uv_layers.new(name=layer.name)
    for loop,point in zip(uv.data,uvs):
        loop.uv = point
    bm = bmesh.new()
    bm.from_mesh(block)
    normal_layer = bm.loops.layers.float_vector.new("StockBobCornerNormal")
    for loop,normal in zip((loop for face in bm.faces for loop in face.loops),normals):
        loop[normal_layer] = normal
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-8)
    normals = [loop[normal_layer].copy() for face in bm.faces for loop in face.loops]
    bm.loops.layers.float_vector.remove(normal_layer)
    bm.to_mesh(block)
    bm.free()
    for poly in block.polygons:
        poly.use_smooth = True
    block.normals_split_custom_set(normals)
    hair = bpy.data.objects.new("AdaptedCC0Bob01",block)
    bpy.context.collection.objects.link(hair)
    assert upper == {tuple(v.co) for v in block.vertices if v.co.z>=1.555}
    normal_records = []
    offset = 0
    for poly in block.polygons:
        normal_records.append({"corners":[{"normal":list(n)} for n in normals[offset:offset+len(poly.vertices)]]})
        offset += len(poly.vertices)
    return hair,normal_records,{
        "basis":"already-admitted fitted CC0 bob01; existing layered cards/cap/UV atlas",
        "method":"remove only diagonal central face-covering fringe and ends below chin contour; no cap reconstruction or vertical compression",
        "upper_preserved_threshold_m":1.555,"upper_vertices_exact":len(upper),
        "stock_hair_faces":original_count,"removed_hair_faces":face_removed,
        "result_hair_faces":len(block.polygons),
        "new_min_z_m":min(v.co.z for v in block.vertices),
    }


def preview(mesh,body):
    recipe.restore_preview_materials(mesh,"Bob")
    cam = recipe.author.stage()
    scene = bpy.context.scene
    scene.cycles.device = "CPU"
    scene.cycles.samples = 8
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 2
    scene.render.resolution_x = 480
    scene.render.resolution_y = 600
    for name,loc in [("front",(0,-4,1.52)),("threequarter",(2,-3,1.55)),("back",(0,4,1.55))]:
        recipe.author.render(cam,body+"-"+name,loc,(0,-.025,1.48),.43)


def export_case(body,apron):
    source = recipe.source_path(body,"Bob",apron)
    rig,mesh = recipe.load(source)
    before = recipe.nonhair_record(mesh)
    old_materials = modular.materials(mesh)
    bind = recipe.rig_record(rig)
    hair,hair_normals,checks = adapt_stock(mesh)
    path = neutral_texture()
    mat = recipe.author.material(MATERIAL,(1,1,1),.70,path,alpha=True)
    slot = recipe.replace(mesh,hair,rig,mat)
    assert slot==7
    modular.restore_normals(mesh,before,hair_normals)
    assert recipe.assert_surfaces(before,recipe.nonhair_record(mesh),0)==0
    assert recipe.assert_rig(bind,recipe.rig_record(rig))==0
    assert old_materials==modular.materials(mesh)
    mesh.name = source.stem
    output = OUT/"Joined"/source.name
    recipe.exporter.validate(mesh,rig)
    recipe.exporter.export_fbx(output,rig,[mesh])
    bpy.ops.wm.save_as_mainfile(filepath=str(output.with_suffix(".blend")))
    rig,mesh = recipe.load(output)
    assert old_materials==modular.materials(mesh)
    checks.update({
        "nonhair_preexport_error":0,
        "nonhair_roundtrip_error":recipe.assert_surfaces(before,recipe.nonhair_record(mesh),1e-5),
        "nonhair_normal_error":recipe.assert_normals(before,recipe.nonhair_record(mesh)),
        "nonhair_material_values_links_texture_hashes_exact":True,
        "bind_error":recipe.assert_rig(bind,recipe.rig_record(rig)),
        "roundtrip":recipe.exporter.validate(mesh,rig),
    })
    return rig,mesh,{
        "body":body,"outfit":"Apron" if apron else "Tunic",
        "fbx":str(output.relative_to(OUT)),"sha256":recipe.sha(output),
        "source":str(source.relative_to(ROOT)),"source_sha256":recipe.sha(source),
        "object_path":recipe.DEST+source.stem+"."+source.stem,
        "hair_material":MATERIAL,"hair_material_slot":7,"checks":checks,
    }


def modular_case(body,donor_path):
    donor_rig,donor = recipe.load(donor_path)
    hair_record = recipe.surface_record(donor,hair=True)
    data = modular.capture_hair(donor)
    name = f"SK_Modular_{body}_Base_Bob"
    source = recipe.CHAR/"ModularClothing"/body/(name+".fbx")
    rig,mesh = recipe.load(source)
    before = recipe.nonhair_record(mesh)
    old_materials = modular.materials(mesh)
    coverage = modular.coverage_counts(mesh)
    bind = recipe.rig_record(rig)
    hair = modular.instantiate_hair(data,mesh,rig)
    mat = recipe.author.material(MATERIAL,(1,1,1),.70,OUT/"Textures"/"T_BobReuse_Neutral.png",alpha=True)
    assert recipe.replace(mesh,hair,rig,mat)==3
    modular.restore_normals(mesh,before,hair_record)
    assert recipe.assert_surfaces(before,recipe.nonhair_record(mesh),0)==0
    mesh.name = name
    output = OUT/"Modular"/(name+".fbx")
    recipe.exporter.export_fbx(output,rig,[mesh])
    bpy.ops.wm.save_as_mainfile(filepath=str(output.with_suffix(".blend")))
    rig,mesh = recipe.load(output)
    assert old_materials==modular.materials(mesh)
    assert modular.coverage_counts(mesh)==coverage
    return {
        "body":body,"fbx":str(output.relative_to(OUT)),"sha256":recipe.sha(output),
        "source":str(source.relative_to(ROOT)),"source_sha256":recipe.sha(source),
        "object_path":f"/Game/SurvivalGame/Characters/ModularClothing/{body}/{name}.{name}",
        "hair_material":MATERIAL,"hair_material_slot":3,
        "checks":{
            "nonhair_roundtrip_error":recipe.assert_surfaces(before,recipe.nonhair_record(mesh),1e-5),
            "nonhair_normal_error":recipe.assert_normals(before,recipe.nonhair_record(mesh)),
            "nonhair_material_values_links_texture_hashes_exact":True,
            "bind_error":recipe.assert_rig(bind,recipe.rig_record(rig)),
            "joined_hair_surface_error":recipe.assert_surfaces(hair_record,recipe.surface_record(mesh,hair=True),1e-5),
            "joined_hair_normal_error":modular.transferred_normal_error(hair_record,recipe.surface_record(mesh,hair=True)),
            "permanent_coverage_faces_unchanged":coverage,
            "roundtrip":recipe.exporter.validate(mesh,rig),
        },
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--body",choices=["Preferred","Willow","Hazel"])
    parser.add_argument("--preview",action="store_true")
    parser.add_argument("--joined-only",action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    for folder in ["Joined","Modular","Textures"]:
        (OUT/folder).mkdir(parents=True,exist_ok=True)
    PREVIEW.mkdir(parents=True,exist_ok=True)
    frozen = {str(p.relative_to(ROOT)):recipe.sha(p) for p in HAIR_ROOT.rglob("*")
              if recipe.is_receipt_file(p) and "LongWave" in p.name}
    for name in ["HomesteadAppearance.h","HomesteadAppearance.cpp"]:
        p = ROOT/"Source"/"SurvivalGame"/name
        frozen[str(p.relative_to(ROOT))] = recipe.sha(p)
    protected = {str(p.relative_to(ROOT)):recipe.sha(p)
                 for namespace in ["Heroine","Variants","BodyPresets","ModularClothing"]
                 for p in (recipe.CHAR/namespace).rglob("*") if recipe.is_receipt_file(p)}
    manifest = {
        "status":"stock-reuse source candidate; not imported, cooked or reference-approved",
        "source_choice":"targeted adaptation of admitted CC0 bob01, preserving fitted cap and layered strand atlas",
        "frozen_wave_and_appearance_hashes":frozen,"protected_source_hashes":protected,
        "joined":[],"modular":[],
    }
    for body in ["Preferred","Willow","Hazel"]:
        if args.body and body!=args.body:
            continue
        for apron in [False,True]:
            rig,mesh,entry = export_case(body,apron)
            manifest["joined"].append(entry)
            if args.preview and not apron:
                preview(mesh,body)
            print("BOB_REUSE_JOINED_VERIFIED",entry["fbx"],flush=True)
        if not args.joined_only:
            source = next(e for e in manifest["joined"] if e["body"]==body and e["outfit"]=="Tunic")
            manifest["modular"].append(modular_case(body,OUT/source["fbx"]))
        (OUT/"manifest.json").write_text(json.dumps(manifest,indent=2))
        print("BOB_REUSE_MODULAR_VERIFIED",body,flush=True)
    texture = OUT/"Textures"/"T_BobReuse_Neutral.png"
    (OUT/"materials.json").write_text(json.dumps({MATERIAL:{
        "base_color":[1,1,1,1],"roughness":.70,"metallic":0,"specular":.5,
        "two_sided":True,"alpha_mask":True,"sRGB":True,
        "texture":str(texture.relative_to(OUT)),"sha256":recipe.sha(texture),
        "runtime_tint":"HomesteadLook::NeutralHairTint",
        "derived_from":"Assets\\Characters\\Heroine\\Textures\\T_Bob_Chestnut.png",
        "alpha":"original admitted bob01 alpha retained unchanged",
    }},indent=2))
    for path,digest in {**frozen,**protected}.items():
        assert recipe.sha(ROOT/path)==digest,"Protected/frozen input changed: "+path
    print("BOB_REUSE_SOURCE_VERIFIED",len(manifest["joined"]),len(manifest["modular"]),flush=True)


if __name__=="__main__":
    main()
