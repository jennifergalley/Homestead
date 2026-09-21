"""Cheap receipt, texture and palette checks; does not launch any engine/toolchain."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "HairstyleRefinement"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--refresh-receipts",action="store_true",help="Refresh hash metadata only; never meshes, textures or previews.")
    parser.add_argument("--refresh-previews",action="store_true",help="Explicitly rewrite source contact sheets; do not use during a wave freeze.")
    args = parser.parse_args(argv)
    total = 0
    for style in ["LongWave", "Bob"]:
        manifest = json.loads((OUT / (style+"-manifest.json")).read_text())
        assert len(manifest["entries"]) == 6
        assert {(e["body"],e["outfit"]) for e in manifest["entries"]} == {
            (b,o) for b in ["Preferred","Willow","Hazel"] for o in ["Tunic","Apron"]}
        for entry in manifest["entries"]:
            path = OUT/entry["fbx"]
            assert sha(path) == entry["sha256"], path
            assert entry["object_path"] == "/Game/SurvivalGame/Characters/Heroine/"+path.stem+"."+path.stem
            check = entry["checks"]
            assert check["nonhair_preexport_max_error"] == 0
            assert check["nonhair_roundtrip_max_error"] < 1e-5
            assert check["nonhair_normal_roundtrip_max_error"] < 1e-4
            assert check["bind_roundtrip_max_error"] < 2e-5
            assert check["roundtrip"]["bones"] == 53
            assert check["roundtrip"]["max_influences"] <= 4
            assert check["material_slot"] == 7
            if style == "LongWave":
                assert 1.10 < check["new_min_z_m"] < 1.13
                assert check["old_min_z_m"] < .90
                assert check["upper_vertices_unchanged"] > 6900
            total += 1
        for path, digest in manifest["protected_inputs"].items():
            assert sha(ROOT/path) == digest, path
        materials = json.loads((OUT/(style+"-materials.json")).read_text())
        for name, material in materials.items():
            assert name.endswith("_Neutral")
            texture = OUT/material["texture"]
            assert sha(texture) == material["sha256"]
            image = Image.open(texture).convert("RGBA")
            red,green,blue,alpha = image.split()
            assert red.tobytes() == green.tobytes() == blue.tobytes()
            lo,hi = red.getextrema()
            assert hi-lo >= (100 if style == "LongWave" else 25), (style,lo,hi)
            if style == "LongWave":
                original = Image.open(ROOT/"Assets"/"Characters"/"Heroine"/"Textures"/"T_Long_Chestnut.png")
                assert alpha.tobytes() == original.convert("RGBA").getchannel("A").tobytes()
    header = (ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.h").read_text()
    cpp = (ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.cpp").read_text()
    assert "constexpr int32 HairColorCount = 5;" in header
    assert "HairColor < HomesteadLook::HairColorCount" in header
    assert "int32 HairColor = 0;" in header
    assert 'TEXT("Blonde")' in cpp and "FLinearColor NeutralHairTint(int32 Index)" in cpp
    modular_count = 0
    modular_path = OUT/"Modular-manifest.json"
    if modular_path.exists():
        modular = json.loads(modular_path.read_text())
        assert len(modular["entries"]) == 6
        assert {(e["body"],e["style"]) for e in modular["entries"]} == {
            (b,s) for b in ["Preferred","Willow","Hazel"] for s in ["LongWave","Bob"]}
        for entry in modular["entries"]:
            path = OUT/entry["fbx"]
            assert sha(path) == entry["sha256"]
            assert sha(ROOT/entry["source"]) == entry["source_sha256"]
            assert sha(OUT/entry["joined_hair_source"]) == entry["joined_hair_sha256"]
            assert entry["object_path"] == (
                f"/Game/SurvivalGame/Characters/ModularClothing/{entry['body']}/{path.stem}.{path.stem}")
            assert len(entry["material_slots"]) == 9
            assert entry["material_slots"][1:3] == ["M_Modular_BaseBra","M_Modular_BaseBriefs"]
            assert entry["hair_material_slot"] == 3
            assert entry["material_slots"][3].endswith("_Neutral")
            checks = entry["checks"]
            assert checks["nonhair_preexport_max_error"] == 0
            assert checks["nonhair_roundtrip_max_error"] < 1e-5
            assert checks["nonhair_normal_error"] < 1e-4
            assert checks["nonhair_materials_values_links_texture_hashes_exact"]
            assert checks["bind_roundtrip_error"] < 2e-5
            assert checks["joined_hair_surface_error"] < 1e-5
            assert checks["joined_hair_normal_error"] < .001
            assert all(n>1000 for n in checks["permanent_coverage_faces_unchanged"].values())
            assert checks["roundtrip"]["bones"] == 53 and checks["hair_head_only"]
            modular_count += 1
        for path,digest in modular["protected_modular_inputs"].items():
            assert sha(ROOT/path) == digest, path
    preview = ROOT/"Build"/"CharacterPreview"/"HairstyleRefinement"
    for style in (["LongWave","Bob"] if args.refresh_previews else []):
        images = [preview/(body+"-"+style+"-"+view+".png")
                  for body in ["Preferred","Willow","Hazel"]
                  for view in ["back","threequarter","front"]]
        if style=="Bob" and (OUT/"canonical-consolidation.json").exists():
            images = [preview/"BobReuse"/(body+"-"+view+".png")
                      for body in ["Preferred","Willow","Hazel"]
                      for view in ["back","threequarter","front"]]
        if all(p.is_file() for p in images):
            sheet = Image.new("RGB",(960,1320),"#171e20")
            draw = ImageDraw.Draw(sheet)
            draw.text((12,8),"CPU SOURCE CANDIDATE - NOT GAMEPLAY OR VISUAL ACCEPTANCE",fill="white")
            draw.text((12,25),"Wave end finish remains open (1.1)" if style=="LongWave"
                      else "Stock bob01 adaptation - not reference-approved",fill="white")
            for i,path in enumerate(images):
                x,y = (i%3)*320,50+(i//3)*420
                sheet.paste(Image.open(path).convert("RGB").resize((320,400)),(x,y+20))
                draw.text((x+8,y+4),path.stem,fill="white")
            (OUT/"Previews").mkdir(exist_ok=True)
            sheet.save(OUT/"Previews"/(style+"-CPU-source.png"))
    if modular_count and args.refresh_previews and not (OUT/"canonical-consolidation.json").exists():
        images = [preview/f"Modular-{body}-{style}-{view}.png"
                  for body in ["Preferred","Willow","Hazel"] for style in ["LongWave","Bob"]
                  for view in ["front","back"]]
        if all(p.is_file() for p in images):
            sheet = Image.new("RGB",(960,1310),"#171e20")
            draw = ImageDraw.Draw(sheet)
            draw.text((12,8),"CPU MODULAR SOURCE PARITY - NOT GAMEPLAY OR ART ACCEPTANCE",fill="white")
            for i,path in enumerate(images):
                x,y = (i%4)*240,30+(i//4)*420
                sheet.paste(Image.open(path).convert("RGB").resize((240,400)),(x,y+20))
                draw.text((x+4,y+4),path.stem.removeprefix("Modular-"),fill="white")
            sheet.save(OUT/"Previews"/"Modular-CPU-source.png")
    artifacts = [p for p in OUT.rglob("*") if p.is_file() and p != OUT/"artifact-hashes.json"]
    artifacts += list((ROOT/"Scripts"/"Characters").glob("*hairstyle*.py"))
    artifacts += [ROOT/"Scripts"/"Characters"/"Build-HairstyleRefinement.ps1",
                  ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.h",
                  ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.cpp"]
    if args.refresh_receipts:
        (OUT/"artifact-hashes.json").write_text(json.dumps({
            str(p.relative_to(ROOT)):{"sha256":sha(p),"bytes":p.stat().st_size}
            for p in sorted(artifacts)
        },indent=2))
    print(f"HAIRSTYLE_RECEIPTS_VERIFIED: {total} joined + {modular_count} modular FBXs; protected inputs, alpha, coverage, rig and palette verified")


if __name__ == "__main__":
    main()
