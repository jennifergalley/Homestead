"""Consolidate the validated stock Bob into canonical paths without touching waves."""
import importlib.util
import json
import sys
from pathlib import Path

import bpy

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("final_hair_helpers",Path(__file__).with_name("build_hairstyle_refinement.py"))
recipe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)
helpers = recipe.module("final_modular_helpers","build_modular_hairstyle_refinement.py")
ROOT,OUT = recipe.ROOT,recipe.OUT


def transfer(donor_path,source,output,material,slot):
    assert output.stem.endswith(("Bob","Bob_Apron")), "Only Bob targets may be written"
    donor_rig,donor = recipe.load(donor_path)
    donor_bind = recipe.rig_record(donor_rig)
    hair_data = helpers.capture_hair(donor)
    hair_before = recipe.surface_record(donor,hair=True)
    rig,mesh = recipe.load(source)
    before = recipe.nonhair_record(mesh)
    before_materials = helpers.materials(mesh)
    bind = recipe.rig_record(rig)
    recipe.assert_rig(donor_bind,bind)
    coverage = helpers.coverage_counts(mesh)
    hair = helpers.instantiate_hair(hair_data,mesh,rig)
    mat = recipe.author.material("M_Heroine_Hair_bob01_Neutral",(1,1,1),
                                 material["roughness"],OUT/material["texture"],alpha=True)
    assert recipe.replace(mesh,hair,rig,mat)==slot
    helpers.restore_normals(mesh,before,hair_before)
    assert recipe.assert_surfaces(before,recipe.nonhair_record(mesh),0)==0
    assert recipe.assert_rig(bind,recipe.rig_record(rig))==0
    assert helpers.materials(mesh)==before_materials
    mesh.name = output.stem
    recipe.exporter.export_fbx(output,rig,[mesh])
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(output.with_suffix(".blend")))
    rig,mesh = recipe.load(output)
    assert helpers.materials(mesh)==before_materials
    assert helpers.coverage_counts(mesh)==coverage
    head = mesh.vertex_groups["head"].index
    for i in {v for p in recipe.hair_faces(mesh) for v in p.vertices}:
        assert [(g.group,g.weight) for g in mesh.data.vertices[i].groups if g.weight>1e-6]==[(head,1.)]
    checks = {
        "nonhair_preexport_max_error":0,
        "nonhair_roundtrip_max_error":recipe.assert_surfaces(before,recipe.nonhair_record(mesh),1e-5),
        "nonhair_normal_roundtrip_max_error":recipe.assert_normals(before,recipe.nonhair_record(mesh)),
        "nonhair_materials_values_links_texture_hashes_exact":True,
        "bind_roundtrip_max_error":recipe.assert_rig(bind,recipe.rig_record(rig)),
        "donor_hair_surface_error":recipe.assert_surfaces(hair_before,recipe.surface_record(mesh,hair=True),1e-5),
        "donor_hair_normal_error":helpers.transferred_normal_error(hair_before,recipe.surface_record(mesh,hair=True)),
        "hair_head_only":True,"material_slot":slot,"roundtrip":recipe.exporter.validate(mesh,rig),
    }
    return checks,[m.name for m in mesh.data.materials],coverage


def main():
    reused = json.loads((OUT/"BobReuse"/"manifest.json").read_text())
    frozen = dict(reused["frozen_wave_and_appearance_hashes"])
    for name in ["EARLY-WAVES.md","incumbent-neutral-tint.patch"]:
        path = OUT/name
        frozen[str(path.relative_to(ROOT))] = recipe.sha(path)
    materials = json.loads((OUT/"BobReuse"/"materials.json").read_text())
    material = materials["M_Heroine_Hair_bob01_Neutral"]
    texture = OUT/"BobReuse"/material["texture"]
    assert recipe.sha(texture)==material["sha256"]
    (OUT/material["texture"]).write_bytes(texture.read_bytes())
    result = {
        "status":"final stock-bob technical source; no runtime/gameplay/visual acceptance",
        "style":"Bob","source_choice":"validated BobReuse stock bob01, transferred without further shaping",
        "protected_inputs":reused["protected_source_hashes"],"entries":[],
    }
    for entry in reused["joined"]:
        donor = OUT/"BobReuse"/entry["fbx"]
        assert recipe.sha(donor)==entry["sha256"]
        source = ROOT/entry["source"]
        output = OUT/entry["fbx"]
        checks,slots,_ = transfer(donor,source,output,material,7)
        result["entries"].append({
            "body":entry["body"],"outfit":entry["outfit"],
            "source":entry["source"],"source_sha256":recipe.sha(source),
            "fbx":str(output.relative_to(OUT)),"sha256":recipe.sha(output),
            "blend_sha256":recipe.sha(output.with_suffix(".blend")),
            "object_path":entry["object_path"],"hair_material":"M_Heroine_Hair_bob01_Neutral",
            "reuse_donor":str(donor.relative_to(OUT)),"reuse_donor_sha256":entry["sha256"],
            "material_slots":slots,"checks":checks,
        })
        print("FINAL_JOINED_BOB_VERIFIED",output.name,flush=True)
    (OUT/"Bob-manifest.json").write_text(json.dumps(result,indent=2))
    (OUT/"Bob-materials.json").write_text(json.dumps(materials,indent=2))
    modular_path = OUT/"Modular-manifest.json"
    modular = json.loads(modular_path.read_text())
    wave_entries = [e for e in modular["entries"] if e["style"]=="LongWave"]
    wave_snapshot = json.dumps(wave_entries,sort_keys=True)
    entries = list(wave_entries)
    for body in ["Preferred","Willow","Hazel"]:
        donor_entry = next(e for e in result["entries"] if e["body"]==body and e["outfit"]=="Tunic")
        donor = OUT/donor_entry["fbx"]
        name = f"SK_Modular_{body}_Base_Bob"
        source = recipe.CHAR/"ModularClothing"/body/(name+".fbx")
        output = OUT/"Modular"/(name+".fbx")
        checks,slots,coverage = transfer(donor,source,output,material,3)
        assert all(n>1000 for n in coverage.values())
        checks.update({
            "nonhair_normal_error":checks["nonhair_normal_roundtrip_max_error"],
            "bind_roundtrip_error":checks["bind_roundtrip_max_error"],
            "joined_hair_surface_error":checks["donor_hair_surface_error"],
            "joined_hair_normal_error":checks["donor_hair_normal_error"],
            "permanent_coverage_faces_unchanged":coverage,
        })
        entries.append({
            "body":body,"style":"Bob","fbx":str(output.relative_to(OUT)),
            "sha256":recipe.sha(output),"blend_sha256":recipe.sha(output.with_suffix(".blend")),
            "source":str(source.relative_to(ROOT)),"source_sha256":recipe.sha(source),
            "joined_hair_source":donor_entry["fbx"],"joined_hair_sha256":donor_entry["sha256"],
            "object_path":f"/Game/SurvivalGame/Characters/ModularClothing/{body}/{name}.{name}",
            "material_slots":slots,"hair_material_slot":3,
            "tint_helper":"HomesteadLook::NeutralHairTint","checks":checks,
        })
        print("FINAL_MODULAR_BOB_VERIFIED",output.name,flush=True)
    assert json.dumps([e for e in entries if e["style"]=="LongWave"],sort_keys=True)==wave_snapshot
    modular["entries"] = entries
    modular["status"] = "final parity: latest local feather-tip waves plus stock-based Bob; not a3a6dee or visual acceptance"
    modular_path.write_text(json.dumps(modular,indent=2))
    for name,digest in {**frozen,**reused["protected_source_hashes"]}.items():
        assert recipe.sha(ROOT/name)==digest,"Protected input changed: "+name
    (OUT/"canonical-consolidation.json").write_text(json.dumps({
        "scope":"Bob-only technical consolidation; latest wave geometry/UV/texture/manifest and palette not rewritten",
        "frozen_files":frozen,"original_inputs":reused["protected_source_hashes"],
        "joined_bob_exports":6,"modular_bob_exports":3,
        "wave_entries_unchanged":True,"visual_acceptance":False,"gameplay_acceptance":False,
    },indent=2))
    print("CANONICAL_HAIRSTYLE_BUNDLE_VERIFIED: 12 joined + 6 modular; Bob consolidated; waves/palette untouched",flush=True)


if __name__=="__main__":
    main()
