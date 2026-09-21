"""Fast source handoff checks without Blender, Unreal or third-party packages."""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "ModularClothing"


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    manifest = json.loads((OUT / "manifest.json").read_text())
    validation = json.loads((OUT / "validation.json").read_text())
    check(not manifest["engine_imported"] and not validation["gameplay_validated"],
          "Source-only handoff must not claim engine acceptance")
    bodies = ("Preferred", "Willow", "Hazel")
    check(set(manifest["bodies"]) == set(validation["bodies"]) == set(bodies), "Missing body fit")
    check([manifest["garments"][key]["definition"] for key in ("Tunic","Apron","Shoes","Footwraps")] ==
          ["linen-tunic","linen-apron","legacy-laceup-shoes","woven-footwraps"], "Catalog key drift")
    expected = {"Tunic","Apron","Shoes","Footwraps","Base_LongWave","Base_Bob","Base_Ponytail"}
    total = 0
    for body in bodies:
        source = manifest["bodies"][body]
        materials = json.loads((OUT / body / "materials.json").read_text())
        check(sha(ROOT / source["source"]) == source["source_sha256"], f"{body}: original source changed")
        recipe = source["reconstruction"]
        check(sha(ROOT / recipe["recipe"]) == recipe["recipe_sha256"], f"{body}: recipe changed")
        check(recipe["maximum_retained_vertex_error_m"] < 0.0001, f"{body}: body was not faithfully reconstructed")
        check(min(recipe["feet_vertices_per_side"]) > 100, f"{body}: source feet missing")
        check(set(source["exports"]) == expected, f"{body}: incomplete export set")
        for key, record in source["exports"].items():
            path = OUT / record["fbx"]
            verified = validation["bodies"][body]["round_trips"][key]
            check(sha(path) == record["sha256"] == verified["sha256"], f"{path}: stale export or validation")
            with path.open("rb") as stream:
                magic = b"Kaydara FBX Binary "
                check(stream.read(len(magic)) == magic, f"{path}: not a real FBX (possibly LFS pointer)")
            check(verified["bones"] == 53 and verified["max_influences"] <= 4, f"{path}: rig/weight contract")
            check(verified["maximum_bind_error"] <= 0.00001, f"{path}: incompatible bind")
            check(record["triangles"] == verified["triangles"], f"{path}: topology round-trip drift")
            check(record["materials"] == verified["materials"], f"{path}: material slot drift")
            if key.startswith("Base_"):
                check(all(m in record["materials"] for m in ("M_Modular_BaseBra","M_Modular_BaseBriefs")),
                      f"{path}: no separate permanent bra and briefs")
                check(verified["coverage"]["protected_region_skin_faces"] == 0, f"{path}: exposed protected region")
                check(verified["coverage"]["exposed_waist_faces"] > 50, f"{path}: one-piece base remains")
            for name in record["materials"]:
                mat = materials[name]
                if mat["texture"]:
                    check(sha(OUT / mat["texture"]) == mat["sha256"], f"{name}: changed texture")
                if name.startswith("M_Modular_"):
                    check(mat["role"] == "fixed" and not mat["alpha_mask"], f"{name}: removable or transparent base/wrap")
            total += 1
        motion = validation["bodies"][body]["motion"]
        check(set(motion) == {"Idle","Walk","GatherWeed","Water","Clear"}, f"{body}: action samples missing")
        check(all(len(m["samples"]) == 5 and m["maximum_vertex_motion_m"] > 0.00001
                  for m in motion.values()), f"{body}: static or untested animation")
    inventory_path = OUT / "asset-hashes.json"
    assets = {str(p.relative_to(OUT)): {"bytes": p.stat().st_size, "sha256": sha(p)}
              for p in sorted(OUT.rglob("*")) if p.suffix in (".blend", ".fbx", ".png")}
    if "--write-inventory" in sys.argv:
        inventory_path.write_text(json.dumps(assets, indent=2) + "\n", encoding="utf-8")
    else:
        check(inventory_path.is_file(), "Asset hash inventory missing; finalize with --write-inventory")
        check(json.loads(inventory_path.read_text()) == assets, "Packed scene/FBX/texture/review asset inventory changed")
    print(f"Modular source handoff passed: {len(bodies)} bodies, {total} FBXs, 75 action/body samples.")


if __name__ == "__main__":
    main()
