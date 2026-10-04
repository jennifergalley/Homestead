"""Verify held six-catch bake receipts, not artistic or Unreal acceptance.

Usage: python Tests\\BlenderFishFamilyProofTests.py --folder <folder> --out <json>
Run BlenderCaughtFishTests.py in the matching scene for anatomy/catalogue checks.
"""
import argparse
import json
from pathlib import Path

from PIL import Image

from BlenderFishBakeProofTests import MAPS, RENDER_SIZE, ROOT, digest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--folder", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    report = json.loads((args.folder / "report.json").read_text(encoding="utf-8"))
    assert report["draft_only"] is True and report["draft_mode"] == "family_baked"
    assert not report["warnings"]
    assert report["authored_source_sha256"] == digest(ROOT / "Scripts" / "Blender" / "Recipes" / "caught_fish.py")
    assert report["material_source_sha256"] == digest(ROOT / "Scripts" / "Blender" / "homestead_materials.py")
    assert report["source_sha256"] == digest(ROOT / report["source"])
    assert report["wet_fish"] == {"coat_weight": .65, "coat_roughness": .06}
    species = {entry["item"]: entry["scientific_name"] for entry in report["species"]}
    assert len(species) == 6
    assert set(report["meshes"]) == {"SM_" + key for key in species}
    textures = set()
    evidence = {"draft_only": True, "not_art_or_unreal_acceptance": True,
                "authored_source_sha256": report["authored_source_sha256"],
                "material_source_sha256": report["material_source_sha256"], "meshes": {}}
    for name, mesh in report["meshes"].items():
        assert 25000 <= mesh["triangles"] <= 65000
        assert digest(args.folder / mesh["fbx"]) == mesh["sha256"], name + " has a stale FBX"
        assert set(mesh["bake"]["maps"]) == MAPS
        assert mesh["bake"]["normal"] == "OpenGL"
        maps = {}
        for kind, filename in mesh["bake"]["maps"].items():
            assert filename not in textures, "Species share a baked map"
            textures.add(filename)
            path = args.folder / "Textures" / filename
            checksum = digest(path)
            # AO is exported separately; the Blender Principled material references only four maps.
            if kind != "ao":
                assert checksum == report["textures"][filename], name + " has a stale " + kind
            with Image.open(path) as image:
                assert image.size == (4096, 4096)
            maps[kind] = {"file": filename, "sha256": checksum}
        beauty = mesh["beauty"]
        assert beauty["samples"] == 192 and tuple(beauty["resolution"]) == RENDER_SIZE
        assert beauty["device"] == "OPTIX"
        assert set(beauty["views"]) == {"hero", "detail"}
        views = {}
        for view, filename in beauty["views"].items():
            path = args.folder / filename
            with Image.open(path) as image:
                assert image.size == RENDER_SIZE
            views[view] = {"file": filename, "sha256": digest(path)}
        evidence["meshes"][name] = {"species": species[name.removeprefix("SM_")],
                                    "triangles": mesh["triangles"], "maps": maps, "views": views}
    material_textures = {filename for filename in textures if not filename.endswith("_ao.png")}
    assert len(textures) == 30 and material_textures == set(report["textures"])
    args.out.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print("ORIGINAL_FISH_FAMILY_RECEIPTS_PASS 6 meshes, 30x4096 maps, 12x4K views; NOT art/UE acceptance")


if __name__ == "__main__":
    main()
