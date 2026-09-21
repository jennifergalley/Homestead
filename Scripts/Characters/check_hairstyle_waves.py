"""Read-only waves-only publication check; no Bob, wardrobe or engine dependency."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/"Assets"/"Characters"/"HairstyleRefinement"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--published-commit",help="Require current wave files/materials/palette to match this Git checkpoint.")
    args = parser.parse_args()
    manifest = json.loads((OUT/"LongWave-manifest.json").read_text())
    if args.published_commit:
        def published(path):
            relative = path.relative_to(ROOT).as_posix()
            return subprocess.check_output(["git","show",args.published_commit+":"+relative],cwd=ROOT)
        manifest = json.loads(published(OUT/"LongWave-manifest.json"))
        for entry in manifest["entries"]:
            path = OUT/entry["fbx"]
            if sha(path)!=entry["sha256"]:
                raise SystemExit(f"PUBLISHED_WAVE_MISMATCH: {path.name} differs from {args.published_commit}. "
                                 "Do not substitute this working-copy candidate for the published checkpoint.")
        assert json.loads((OUT/"LongWave-materials.json").read_text()) == json.loads(
            published(OUT/"LongWave-materials.json")), "Published material contract mismatch"
        for name in ["HomesteadAppearance.h","HomesteadAppearance.cpp"]:
            path = ROOT/"Source"/"SurvivalGame"/name
            assert path.read_text().splitlines()==published(path).decode().splitlines(), "Published palette mismatch: "+name
    assert manifest["style"] == "LongWave"
    assert len(manifest["entries"]) == 6
    assert {(e["body"],e["outfit"]) for e in manifest["entries"]} == {
        (b,o) for b in ["Preferred","Willow","Hazel"] for o in ["Tunic","Apron"]}
    for entry in manifest["entries"]:
        path = OUT/entry["fbx"]
        assert path.name.endswith(("LongWave.fbx","LongWave_Apron.fbx"))
        assert sha(path) == entry["sha256"], path
        assert entry["object_path"] == "/Game/SurvivalGame/Characters/Heroine/"+path.stem+"."+path.stem
        checks = entry["checks"]
        assert checks["material_slot"] == 7
        assert checks["nonhair_preexport_max_error"] == 0
        assert checks["nonhair_roundtrip_max_error"] < 1e-5
        assert checks["nonhair_normal_roundtrip_max_error"] < 1e-4
        assert checks["bind_roundtrip_max_error"] < 2e-5
        assert checks["roundtrip"]["bones"] == 53
        assert checks["upper_vertices_unchanged"] > 6900
        assert 1.10 < checks["new_min_z_m"] < 1.13
        assert checks["old_min_z_m"] < .90
    materials = json.loads((OUT/"LongWave-materials.json").read_text())
    assert set(materials) == {"M_Heroine_Hair_long01_Neutral"}
    material = materials["M_Heroine_Hair_long01_Neutral"]
    assert sha(OUT/material["texture"]) == material["sha256"]
    assert material["runtime_tint"] == "HomesteadLook::NeutralHairTint"
    for path,digest in manifest["protected_inputs"].items():
        assert sha(ROOT/path) == digest, path
    header = (ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.h").read_text()
    cpp = (ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.cpp").read_text()
    assert "constexpr int32 HairColorCount = 5;" in header
    assert "HairColor < HomesteadLook::HairColorCount" in header
    assert "int32 HairColor = 0;" in header
    assert "FLinearColor NeutralHairTint(int32 Index)" in cpp
    print("WAVES_ONLY_VERIFIED: 6 FBXs + neutral texture/material + Appearance; original inputs unchanged")
    print("Publication scope: "+(args.published_commit if args.published_commit else
          "working copy only; use --published-commit to validate an exact published checkpoint"))
    print("Source candidate only: task 1.1 cut-edge finish and cooked gameplay acceptance remain open.")


if __name__ == "__main__":
    main()
