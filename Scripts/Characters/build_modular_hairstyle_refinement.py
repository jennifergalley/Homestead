"""Read-only stable modular bases -> separate hair-parity outputs; CPU/offline."""
import importlib.util
import json
import sys
from collections import Counter
from pathlib import Path

import bpy

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("hair_recipe", Path(__file__).with_name("build_hairstyle_refinement.py"))
recipe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)
SOURCE = recipe.CHAR/"ModularClothing"
OUT = recipe.OUT/"Modular"
STYLES = ["LongWave","Bob"]
BODIES = ["Preferred","Willow","Hazel"]


def materials(mesh):
    result = {}
    for material in mesh.data.materials:
        if material.name.startswith("M_Heroine_Hair_"):
            continue
        shader = next(n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        values = {}
        for name in ["Base Color","Metallic","Roughness","Alpha","Specular IOR Level"]:
            value = shader.inputs[name].default_value
            values[name] = list(value) if hasattr(value, "__iter__") else value
        values["images"] = sorted((Path(n.image.filepath).name,
                                   recipe.sha(Path(bpy.path.abspath(n.image.filepath))))
                                  for n in material.node_tree.nodes if n.type == "TEX_IMAGE" and n.image)
        values["links"] = sorted((link.from_node.type,link.from_socket.name,
                                 link.to_node.type,link.to_socket.name)
                                for link in material.node_tree.links)
        result[material.name] = values
    return result


def capture_hair(mesh):
    faces = recipe.hair_faces(mesh)
    ids = sorted({v for p in faces for v in p.vertices})
    lookup = {old:new for new,old in enumerate(ids)}
    return {
        "vertices":[list(mesh.data.vertices[i].co) for i in ids],
        "faces":[[lookup[i] for i in p.vertices] for p in faces],
        "uv_name":mesh.data.uv_layers.active.name,
        "uv":[list(mesh.data.uv_layers.active.data[i].uv) for p in faces for i in p.loop_indices],
        "normals":[list(mesh.data.corner_normals[i].vector) for p in faces for i in p.loop_indices],
    }


def instantiate_hair(data, mesh, rig):
    block = bpy.data.meshes.new("JoinedHairstyleParity")
    block.from_pydata(data["vertices"],[],data["faces"])
    layer = block.uv_layers.new(name=data["uv_name"])
    for loop,uv in zip(layer.data,data["uv"]):
        loop.uv = uv
    for poly in block.polygons:
        poly.use_smooth = True
    block.normals_split_custom_set(data["normals"])
    hair = bpy.data.objects.new("JoinedHairstyleParity",block)
    bpy.context.collection.objects.link(hair)
    hair.parent = rig
    hair.matrix_world = mesh.matrix_world.copy()
    return hair


def coverage_counts(mesh):
    counts = Counter(mesh.data.materials[p.material_index].name for p in mesh.data.polygons)
    return {name:counts[name] for name in ["M_Modular_BaseBra","M_Modular_BaseBriefs"]}


def restore_normals(mesh, nonhair, hair):
    normals = [tuple(n.vector) for n in mesh.data.corner_normals]
    records = {False:iter(nonhair),True:iter(hair)}
    for poly in mesh.data.polygons:
        is_hair = mesh.data.materials[poly.material_index].name.startswith("M_Heroine_Hair_")
        record = next(records[is_hair])
        for loop,corner in zip(poly.loop_indices,record["corners"]):
            normals[loop] = corner["normal"]
    mesh.data.normals_split_custom_set(normals)


def transferred_normal_error(before, after):
    # Reconstructed hair gets a second compact custom-normal encoding. Keep
    # that bounded separately from the unchanged base normals (strict 1e-4).
    error = max(abs(x-y) for a,b in zip(before,after)
                for c,d in zip(a["corners"],b["corners"])
                for x,y in zip(c["normal"],d["normal"]))
    assert error < .001, error
    return error


def preview(mesh, style, body):
    recipe.restore_preview_materials(mesh,style)
    records = json.loads((SOURCE/body/"materials.json").read_text())
    for material in mesh.data.materials:
        if material.name not in ["M_Modular_BaseBra","M_Modular_BaseBriefs"]:
            continue
        shader = material.node_tree.nodes.get("Principled BSDF")
        record = records[material.name]
        for link in list(shader.inputs["Base Color"].links):
            material.node_tree.links.remove(link)
        shader.inputs["Base Color"].default_value = record["base_color"]
        shader.inputs["Roughness"].default_value = record["roughness"]
    cam = recipe.author.stage()
    scene = bpy.context.scene
    scene.cycles.device = "CPU"
    scene.cycles.samples = 8
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 2
    scene.render.resolution_x = 360
    scene.render.resolution_y = 600
    for name,loc in [("front",(0,-4,1.25)),("back",(0,4,1.25))]:
        recipe.author.render(cam,"Modular-"+body+"-"+style+"-"+name,loc,(0,0,1.05),1.28)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    protected = {str(p.relative_to(recipe.ROOT)):recipe.sha(p)
                 for p in SOURCE.rglob("*") if p.is_file()}
    original_manifest = json.loads((SOURCE/"manifest.json").read_text())
    joined = {style:json.loads((recipe.OUT/(style+"-manifest.json")).read_text()) for style in STYLES}
    manifest = {
        "status":"interchange-validated modular parity; wave visual finish and gameplay acceptance still pending",
        "stable_base_authorization":"2026-09-20 23:53 Arizona; read-only modular source with separate permanent bra and briefs",
        "source_namespace":str(SOURCE.relative_to(recipe.ROOT)),
        "skeleton":original_manifest["unreal_skeleton"],
        "protected_modular_inputs":protected,
        "entries":[],
    }
    for body in BODIES:
        for style in STYLES:
            entry = next(e for e in joined[style]["entries"] if e["body"]==body and e["outfit"]=="Tunic")
            donor = recipe.OUT/entry["fbx"]
            assert recipe.sha(donor) == entry["sha256"]
            donor_rig,donor_mesh = recipe.load(donor)
            donor_bind = recipe.rig_record(donor_rig)
            donor_hair = recipe.surface_record(donor_mesh,hair=True)
            hair_data = capture_hair(donor_mesh)
            name = f"SK_Modular_{body}_Base_{style}"
            source = SOURCE/body/(name+".fbx")
            rig,mesh = recipe.load(source)
            before = recipe.nonhair_record(mesh)
            before_materials = materials(mesh)
            bind = recipe.rig_record(rig)
            recipe.assert_rig(donor_bind,bind)
            slots = [m.name for m in mesh.data.materials]
            assert slots[:3] == ["M_Heroine_Skin","M_Modular_BaseBra","M_Modular_BaseBriefs"], slots
            assert slots[3].startswith("M_Heroine_Hair_")
            assert len(slots) == 9
            coverage = coverage_counts(mesh)
            assert all(n > 1000 for n in coverage.values()), coverage
            hair = instantiate_hair(hair_data,mesh,rig)
            material_record = json.loads((recipe.OUT/(style+"-materials.json")).read_text())
            material_name,record = next(iter(material_record.items()))
            mat = recipe.author.material(material_name,(1,1,1),record["roughness"],
                                         recipe.OUT/record["texture"],alpha=True)
            slot = recipe.replace(mesh,hair,rig,mat)
            assert slot == 3
            restore_normals(mesh,before,donor_hair)
            assert recipe.assert_surfaces(before,recipe.nonhair_record(mesh),0) == 0
            assert recipe.assert_rig(bind,recipe.rig_record(rig)) == 0
            assert materials(mesh) == before_materials
            assert coverage_counts(mesh) == coverage
            mesh.name = name
            output = OUT/(name+".fbx")
            recipe.exporter.validate(mesh,rig)
            recipe.exporter.export_fbx(output,rig,[mesh])
            bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(name+".blend")))
            rig,mesh = recipe.load(output)
            after = recipe.nonhair_record(mesh)
            new_hair = recipe.surface_record(mesh,hair=True)
            assert materials(mesh) == before_materials
            assert coverage_counts(mesh) == coverage
            assert [m.name for i,m in enumerate(mesh.data.materials) if i != 3] == [
                m for i,m in enumerate(slots) if i != 3]
            head = mesh.vertex_groups["head"].index
            for i in {v for p in recipe.hair_faces(mesh) for v in p.vertices}:
                assert [(g.group,g.weight) for g in mesh.data.vertices[i].groups if g.weight>1e-6] == [(head,1.0)]
            checks = {
                "nonhair_preexport_max_error":0,
                "nonhair_roundtrip_max_error":recipe.assert_surfaces(before,after,1e-5),
                "nonhair_normal_error":recipe.assert_normals(before,after),
                "nonhair_materials_values_links_texture_hashes_exact":True,
                "bind_roundtrip_error":recipe.assert_rig(bind,recipe.rig_record(rig)),
                "joined_hair_surface_error":recipe.assert_surfaces(donor_hair,new_hair,1e-5),
                "joined_hair_normal_error":transferred_normal_error(donor_hair,new_hair),
                "permanent_coverage_faces_unchanged":coverage,
                "hair_head_only":True,
                "roundtrip":recipe.exporter.validate(mesh,rig),
            }
            row = {
                "body":body,"style":style,
                "fbx":str(output.relative_to(recipe.OUT)),"sha256":recipe.sha(output),
                "source":str(source.relative_to(recipe.ROOT)),"source_sha256":recipe.sha(source),
                "joined_hair_source":entry["fbx"],"joined_hair_sha256":entry["sha256"],
                "object_path":f"/Game/SurvivalGame/Characters/ModularClothing/{body}/{name}.{name}",
                "material_slots":[m.name for m in mesh.data.materials],
                "hair_material_slot":3,"tint_helper":"HomesteadLook::NeutralHairTint",
                "coverage_basis":original_manifest["bodies"][body]["coverage"],
                "checks":checks,
            }
            manifest["entries"].append(row)
            (recipe.OUT/"Modular-manifest.json").write_text(json.dumps(manifest,indent=2))
            if "--preview" in sys.argv:
                preview(mesh,style,body)
            print("MODULAR_HAIRSTYLE_VERIFIED",name,json.dumps(checks),flush=True)
    for path,digest in protected.items():
        assert recipe.sha(recipe.ROOT/path) == digest, "Stable modular input changed: "+path
    print("MODULAR_HAIRSTYLE_PARITY_VERIFIED",len(manifest["entries"]),flush=True)


if __name__ == "__main__":
    main()
