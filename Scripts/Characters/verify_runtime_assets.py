"""Check persisted character references in a fresh editor process before gameplay."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
DEST = "/Game/SurvivalGame/Characters/Heroine"
REPORT = ROOT / "Build" / "CharacterPreview" / "runtime-assets-verification.json"


def main():
    REPORT.unlink(missing_ok=True)
    result = {}
    failures = []
    for body in ("", "Willow_", "Hazel_"):
        for outfit in ("", "_Apron"):
            for hair in ("LongWave", "Bob", "Ponytail"):
                name = f"SK_Heroine_{body}{hair}{outfit}"
                mesh = unreal.EditorAssetLibrary.load_asset(DEST + "/" + name)
                if not mesh:
                    failures.append(name + ": missing mesh")
                    continue
                slots = []
                for slot in mesh.get_editor_property("materials"):
                    label = str(slot.get_editor_property("material_slot_name"))
                    material = slot.get_editor_property("material_interface")
                    slots.append({"slot": label, "material": material.get_path_name() if material else None})
                    if not material:
                        failures.append(name + ": missing " + label)
                result[name] = {"skeleton": mesh.get_editor_property("skeleton").get_path_name(), "materials": slots}
    REPORT.write_text(json.dumps({"meshes": result, "failures": failures}, indent=2), encoding="utf-8")
    if failures:
        raise RuntimeError("Persisted character references failed: " + "; ".join(failures))
    unreal.log("ALL_CHARACTER_REFERENCES_VERIFIED")


if __name__ == "__main__":
    main()
