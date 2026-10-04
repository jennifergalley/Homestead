"""Verify one-trout source/baked evidence, not artistic or Unreal acceptance.

Usage: python Tests\\BlenderFishBakeProofTests.py --source <folder> --baked <folder> --out <json>
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
MAPS = {"basecolor", "roughness", "normal", "ao", "metallic"}
RENDER_SIZE = (3840, 2160)
MAX_MEAN_DIFFERENCE = 1.5
MAX_RMS_DIFFERENCE = 3.0


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--baked", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    source = json.loads((args.source / "report.json").read_text(encoding="utf-8"))
    baked = json.loads((args.baked / "report.json").read_text(encoding="utf-8"))
    for report in (source, baked):
        assert report["draft_only"] is True, "Proof must not advertise production admission"
        assert report["authored_source_sha256"] == digest(ROOT / "Scripts" / "Blender" / "Recipes" / "caught_fish.py")
        assert report["material_source_sha256"] == digest(ROOT / "Scripts" / "Blender" / "homestead_materials.py")
        assert report["source_sha256"] == digest(ROOT / report["source"])
        assert report["wet_fish"] == {"coat_weight": .65, "coat_roughness": .06}
        assert set(report["meshes"]) == {"SM_RiverTrout"}, "Proof must contain only the one structural trout"
        assert not report["warnings"]
    assert source["draft_mode"] == "source" and baked["draft_mode"] == "baked"
    source_mesh, baked_mesh = (report["meshes"]["SM_RiverTrout"] for report in (source, baked))
    assert source_mesh["review"] == baked_mesh["review"], "Framing differs"
    assert source_mesh["triangles"] == baked_mesh["triangles"]
    assert source_mesh["size_cm"] == baked_mesh["size_cm"]
    assert set(baked_mesh["bake"]["maps"]) == MAPS
    assert baked_mesh["bake"]["normal"] == "OpenGL"
    evidence = {"draft_only": True, "not_art_or_unreal_acceptance": True,
                "authored_source_sha256": source["authored_source_sha256"],
                "material_source_sha256": source["material_source_sha256"], "maps": {}, "views": {}}
    for kind, filename in baked_mesh["bake"]["maps"].items():
        path = args.baked / "Textures" / filename
        with Image.open(path) as image:
            assert image.size == (4096, 4096), kind + " has wrong dimensions"
        evidence["maps"][kind] = {"file": filename, "sha256": digest(path)}
        if filename in baked["textures"]:
            assert evidence["maps"][kind]["sha256"] == baked["textures"][filename]
    for report, folder in ((source, args.source), (baked, args.baked)):
        mesh = report["meshes"]["SM_RiverTrout"]
        assert digest(folder / mesh["fbx"]) == mesh["sha256"], "Stale FBX"
        beauty = mesh["beauty"]
        assert beauty["samples"] == 192 and tuple(beauty["resolution"]) == RENDER_SIZE
        assert beauty["device"] == "OPTIX"
    for view in ("hero", "detail"):
        arrays = []
        for mesh, folder in ((source_mesh, args.source), (baked_mesh, args.baked)):
            with Image.open(folder / mesh["beauty"]["views"][view]) as image:
                assert image.size == RENDER_SIZE
                arrays.append(np.asarray(image.convert("RGB"), dtype=np.float32))
        difference = np.abs(arrays[0] - arrays[1])
        mean, rms = float(difference.mean()), float(np.sqrt(np.mean(difference ** 2)))
        assert mean <= MAX_MEAN_DIFFERENCE and rms <= MAX_RMS_DIFFERENCE, view + " loses source appearance in bake"
        evidence["views"][view] = {"mean_absolute_8bit_difference": mean, "rms_8bit_difference": rms,
                                 "percentile_99_difference": float(np.percentile(difference, 99))}
    args.out.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print("ORIGINAL_FISH_BAKE_PROOF_PASS 5x4096 maps, paired 4K views, current-source hashes; NOT art/UE acceptance")


if __name__ == "__main__":
    main()
