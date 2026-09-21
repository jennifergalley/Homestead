"""Offline modular parity from exact locally cached published wave FBXs; no sculpting."""
import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
HAIR = ROOT/"Assets"/"Characters"/"HairstyleRefinement"
OUT = HAIR/"PublishedWaveParity"
COMMIT = "a3a6dee"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def published(relative):
    return subprocess.check_output(["git","show",COMMIT+":"+relative],cwd=ROOT)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/"Joined").mkdir(exist_ok=True)
    (OUT/"Textures").mkdir(exist_ok=True)
    frozen = json.loads((HAIR/"BobReuse"/"manifest.json").read_text())["frozen_wave_and_appearance_hashes"]
    for name in ["EARLY-WAVES.md","incumbent-neutral-tint.patch"]:
        path = HAIR/name
        frozen[str(path.relative_to(ROOT))] = sha(path)
    prefix = "Assets/Characters/HairstyleRefinement/"
    manifest_bytes = published(prefix+"LongWave-manifest.json")
    materials_bytes = published(prefix+"LongWave-materials.json")
    source_manifest = json.loads(manifest_bytes)
    material = json.loads(materials_bytes)["M_Heroine_Hair_long01_Neutral"]
    texture = HAIR/material["texture"]
    assert sha(texture)==material["sha256"], "Published neutral texture/calibration mismatch"
    for name in ["HomesteadAppearance.h","HomesteadAppearance.cpp"]:
        path = ROOT/"Source"/"SurvivalGame"/name
        assert published(path.relative_to(ROOT).as_posix()).decode().splitlines()==path.read_text().splitlines()
    media = next(line.split("=",1)[1] for line in subprocess.check_output(
        ["git","lfs","env"],cwd=ROOT,text=True).splitlines() if line.startswith("LocalMediaDir="))
    for entry in source_manifest["entries"]:
        digest = entry["sha256"]
        obj = Path(media)/digest[:2]/digest[2:4]/digest
        if not obj.is_file():
            raise RuntimeError("Published LFS object is not cached locally; no download attempted: "+digest)
        assert sha(obj)==digest
        (OUT/entry["fbx"]).write_bytes(obj.read_bytes())
    (OUT/"LongWave-manifest.json").write_bytes(manifest_bytes)
    (OUT/"LongWave-materials.json").write_bytes(materials_bytes)
    (OUT/material["texture"]).write_bytes(texture.read_bytes())
    spec = importlib.util.spec_from_file_location(
        "published_wave_parity_helpers",Path(__file__).with_name("build_modular_hairstyle_refinement.py"))
    parity = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parity)
    parity.STYLES = ["LongWave"]
    parity.OUT = OUT/"Modular"
    parity.recipe.OUT = OUT
    parity.main()
    path = OUT/"Modular-manifest.json"
    report = json.loads(path.read_text())
    report.update({
        "status":"technical parity with exact published wave checkpoint; gameplay/art acceptance pending",
        "published_commit":COMMIT,
        "published_input_manifest_sha256":hashlib.sha256(manifest_bytes).hexdigest(),
        "published_input_materials_sha256":hashlib.sha256(materials_bytes).hexdigest(),
        "acquisition":"exact SHA-verified local Git LFS objects only; no download or other checkout modification",
        "frozen_wave_palette_and_parent_hashes":frozen,
        "qualification":"Copies in Joined are byte-identical a3a6dee inputs, not the later frozen local feather candidate.",
    })
    path.write_text(json.dumps(report,indent=2))
    for name,digest in frozen.items():
        assert sha(ROOT/name)==digest,"Frozen or parent-owned file changed: "+name
    print("PUBLISHED_WAVE_MODULAR_PARITY_VERIFIED",COMMIT,len(report["entries"]),flush=True)


if __name__=="__main__":
    main()
