"""Independent fresh-FBX material, bind, surface and head-weight verification."""
import importlib.util
import json
import sys
from pathlib import Path

import bpy

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("hair_recipe", Path(__file__).with_name("build_hairstyle_refinement.py"))
recipe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)


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


report = {"source_only":True, "entries":[]}
for style in ["LongWave","Bob"]:
    manifest = json.loads((recipe.OUT/(style+"-manifest.json")).read_text())
    for entry in manifest["entries"]:
        rig, mesh = recipe.load(recipe.ROOT/entry["source"])
        original_materials = materials(mesh)
        original_rig = recipe.rig_record(rig)
        original = recipe.nonhair_record(mesh)
        original_scale = list(mesh.matrix_world.to_scale())
        midpoint = ((rig.matrix_world@rig.data.bones["upperarm_l"].head_local).z
                    +(rig.matrix_world@rig.data.bones["spine_02"].head_local).z)/2
        rig, mesh = recipe.load(recipe.OUT/entry["fbx"])
        assert original_materials == materials(mesh), entry["fbx"]
        scale_error = max(abs(a-b) for a,b in zip(original_scale,mesh.matrix_world.to_scale()))
        assert scale_error < 1e-6, (original_scale,list(mesh.matrix_world.to_scale()))
        surfaces = recipe.nonhair_record(mesh)
        head = mesh.vertex_groups["head"].index
        ids = {i for p in recipe.hair_faces(mesh) for i in p.vertices}
        for i in ids:
            weights = [(g.group,g.weight) for g in mesh.data.vertices[i].groups if g.weight > 1e-6]
            assert weights == [(head,1.0)], (i,weights)
        row = {
            "fbx":entry["fbx"],
            "nonhair_material_nodes_values_texture_names_exact":True,
            "original_mesh_scale":original_scale,
            "mesh_scale_roundtrip_error":scale_error,
            "nonhair_surface_error":recipe.assert_surfaces(original,surfaces,1e-5),
            "nonhair_normal_error":recipe.assert_normals(original,surfaces),
            "bind_error":recipe.assert_rig(original_rig,recipe.rig_record(rig)),
            "head_only_hair_vertices":len(ids),
            "height_m":max((mesh.matrix_world@v.co).z for v in mesh.data.vertices)
                       -min((mesh.matrix_world@v.co).z for v in mesh.data.vertices),
        }
        if style == "LongWave":
            row["shoulder_waist_midpoint_m"] = midpoint
            row["wave_tip_above_midpoint_m"] = entry["checks"]["new_min_z_m"]-midpoint
            assert abs(row["wave_tip_above_midpoint_m"]) < .025
        assert 1.55 < row["height_m"] < 1.65
        report["entries"].append(row)
        print("HAIRSTYLE_INDEPENDENT_CHECK",entry["fbx"],flush=True)
(recipe.OUT/"roundtrip-validation.json").write_text(json.dumps(report,indent=2))
print("HAIRSTYLE_INDEPENDENT_ROUNDTRIPS_VERIFIED",len(report["entries"]))
