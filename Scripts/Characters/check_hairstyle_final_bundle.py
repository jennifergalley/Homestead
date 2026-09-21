"""Validate an explicit Bob/modular selection and distinguish published/local waves."""
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
HAIR = ROOT/"Assets"/"Characters"/"HairstyleRefinement"
PUB = HAIR/"PublishedWaveParity"
COMMIT = "a3a6dee"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git_bytes(commit,path):
    return subprocess.check_output(["git","show",commit+":"+path.relative_to(ROOT).as_posix()],cwd=ROOT)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--write-receipts",action="store_true",help="Write audit/selection/hash metadata only.")
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location("final_bob_check",Path(__file__).with_name("check_hairstyle_bob_reuse.py"))
    bob_check = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bob_check)
    bob_check.main()
    spec = importlib.util.spec_from_file_location("canonical_hair_check",Path(__file__).with_name("check_hairstyle_refinement.py"))
    canonical_check = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(canonical_check)
    canonical_check.main([])
    consolidation = json.loads((HAIR/"canonical-consolidation.json").read_text())
    for name,digest in {**consolidation["frozen_files"],**consolidation["original_inputs"]}.items():
        assert sha(ROOT/name)==digest,name
    published_bytes = git_bytes(COMMIT,HAIR/"LongWave-manifest.json")
    published = json.loads(published_bytes)
    assert json.loads((PUB/"LongWave-manifest.json").read_text())==published
    for entry in published["entries"]:
        assert sha(PUB/entry["fbx"])==entry["sha256"]
    material_bytes = git_bytes(COMMIT,HAIR/"LongWave-materials.json")
    assert json.loads((PUB/"LongWave-materials.json").read_text())==json.loads(material_bytes)
    material = json.loads(material_bytes)["M_Heroine_Hair_long01_Neutral"]
    assert sha(PUB/material["texture"])==sha(HAIR/material["texture"])==material["sha256"]
    modular = json.loads((PUB/"Modular-manifest.json").read_text())
    assert modular["published_commit"]==COMMIT
    assert len(modular["entries"])==3
    assert {e["body"] for e in modular["entries"]}=={"Preferred","Willow","Hazel"}
    for entry in modular["entries"]:
        assert sha(PUB/entry["fbx"])==entry["sha256"]
        assert sha(ROOT/entry["source"])==entry["source_sha256"]
        assert sha(PUB/entry["joined_hair_source"])==entry["joined_hair_sha256"]
        source = next(e for e in published["entries"] if e["body"]==entry["body"] and e["outfit"]=="Tunic")
        assert entry["joined_hair_sha256"]==source["sha256"]
        checks = entry["checks"]
        assert checks["nonhair_preexport_max_error"]==0
        assert checks["nonhair_roundtrip_max_error"]<1e-5
        assert checks["nonhair_normal_error"]<1e-4
        assert checks["nonhair_materials_values_links_texture_hashes_exact"]
        assert checks["bind_roundtrip_error"]<2e-5
        assert checks["joined_hair_surface_error"]<1e-5
        assert checks["joined_hair_normal_error"]<.001
        assert all(n>1000 for n in checks["permanent_coverage_faces_unchanged"].values())
        assert checks["roundtrip"]["bones"]==53 and checks["hair_head_only"]
        assert entry["material_slots"][3]=="M_Heroine_Hair_long01_Neutral"
    for name,digest in {**modular["protected_modular_inputs"],
                        **modular["frozen_wave_palette_and_parent_hashes"]}.items():
        assert sha(ROOT/name)==digest,name
    audit = {"published_wave_commit":COMMIT,"published_palette_commit":"013c70b",
             "scope":"Existing differences predate the freeze; this check never changes source assets.",
             "files":[]}
    for entry in published["entries"]:
        for suffix in [".fbx",".blend"]:
            path = (HAIR/entry["fbx"]).with_suffix(suffix)
            pointer = git_bytes(COMMIT,path)
            match = re.search(rb"oid sha256:([0-9a-f]{64})",pointer)
            assert match,"Expected published LFS pointer: "+str(path)
            expected = match.group(1).decode()
            current = sha(path)
            audit["files"].append({"file":str(path.relative_to(ROOT)),"published_sha256":expected,
                                   "working_sha256":current,"same_bytes":expected==current})
    for name in ["LongWave-manifest.json","LongWave-materials.json"]:
        path = HAIR/name
        content = git_bytes(COMMIT,path)
        audit["files"].append({"file":str(path.relative_to(ROOT)),
            "published_sha256":hashlib.sha256(content).hexdigest(),"working_sha256":sha(path),
            "same_normalized_content":content.decode().splitlines()==path.read_text().splitlines()})
    audit["neutral_texture_matches_published"] = sha(HAIR/material["texture"])==material["sha256"]
    audit["palette_source_matches_published"] = {}
    for name in ["HomesteadAppearance.h","HomesteadAppearance.cpp"]:
        path = ROOT/"Source"/"SurvivalGame"/name
        same = git_bytes("013c70b",path).decode().splitlines()==path.read_text().splitlines()
        assert same
        audit["palette_source_matches_published"][name] = same
    canonical_waves = json.loads((HAIR/"LongWave-manifest.json").read_text())
    canonical_bob = json.loads((HAIR/"Bob-manifest.json").read_text())
    canonical_modular = json.loads((HAIR/"Modular-manifest.json").read_text())
    selection = {
        "status":"final uncommitted technical source bundle; no gameplay or visual acceptance",
        "wave_revision":"Latest feather-tip/local-reference candidate, explicitly different from immutable a3a6dee; parent requested this final revision.",
        "joined_wave_manifest":"LongWave-manifest.json",
        "joined_bob_manifest":"Bob-manifest.json",
        "modular_manifest":"Modular-manifest.json",
        "wave_materials":"LongWave-materials.json",
        "bob_materials":"Bob-materials.json",
        "reference_only":["BobReuse (stock-authoring donor/reference)",
                          "PublishedWaveParity (immutable a3a6dee inputs and technical parity)"],
        "published_checkpoint_audit":"publication-audit.json",
        "entries":[],
    }
    for group,folder,entries in [
        ("joined_waves",HAIR,canonical_waves["entries"]),
        ("joined_bob",HAIR,canonical_bob["entries"]),
        ("modular",HAIR,canonical_modular["entries"]),
    ]:
        for entry in entries:
            selection["entries"].append({"group":group,
                "fbx":str((folder/entry["fbx"]).relative_to(HAIR)),
                "sha256":entry["sha256"],"object_path":entry["object_path"]})
    assert len(selection["entries"])==18
    assert len({e["object_path"] for e in selection["entries"]})==18
    if args.write_receipts:
        (HAIR/"publication-audit.json").write_text(json.dumps(audit,indent=2))
        (HAIR/"final-bundle.json").write_text(json.dumps(selection,indent=2))
        files = [HAIR/e["fbx"] for e in selection["entries"]]
        files += [p.with_suffix(".blend") for p in list(files)]
        files += [HAIR/name for name in [
            "LongWave-manifest.json","LongWave-materials.json","Bob-manifest.json",
            "Bob-materials.json","Modular-manifest.json","canonical-consolidation.json",
            "README.md","FINAL-HANDOFF.md","provenance.json","publication-audit.json","final-bundle.json",
            "EARLY-WAVES.md","incumbent-neutral-tint.patch"]]
        files += [HAIR/m["texture"] for material_file in ["LongWave-materials.json","Bob-materials.json"]
                  for m in json.loads((HAIR/material_file).read_text()).values()]
        files += list((ROOT/"Scripts"/"Characters").glob("*hairstyle*.py"))
        files += [ROOT/"Scripts"/"Characters"/"Build-HairstyleRefinement.ps1",
                  ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.h",
                  ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.cpp"]
        (HAIR/"final-bundle-hashes.json").write_text(json.dumps({
            str(p.relative_to(ROOT)):{"sha256":sha(p),"bytes":p.stat().st_size} for p in sorted(set(files))
        },indent=2))
    else:
        assert json.loads((HAIR/"final-bundle.json").read_text())==selection,"Stale final selection metadata"
        for receipt in ["final-bundle-hashes.json","artifact-hashes.json"]:
            for name,record in json.loads((HAIR/receipt).read_text()).items():
                assert sha(ROOT/name)==record["sha256"],"Stale hash receipt: "+name
    changed = [Path(e["file"]).name for e in audit["files"] if
               not e.get("same_bytes",e.get("same_normalized_content",False))]
    print("FINAL_TECHNICAL_BUNDLE_VERIFIED: 12 joined + 6 modular; latest waves + stock-based Bob")
    print("IMMUTABLE_REFERENCE_VERIFIED:",COMMIT,"; palette/neutral texture unchanged; original inputs and frozen/parent files unchanged")
    print("PRE_FREEZE_LOCAL_DIFFERENCES:",", ".join(changed))


if __name__=="__main__":
    main()
