"""Fresh-process, read-only verification of the persisted heroine material contract."""
import json
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[2]
CONTRACT=ROOT/"Assets"/"Characters"/"Heroine"/"appearance-contract.json"
REPORT=ROOT/"Build"/"CharacterPreview"/"unreal-import-report.json"
OUTPUT=ROOT/"Build"/"CharacterPreview"/"skeletal-material-usage-verification.json"
LIB=unreal.EditorAssetLibrary
EDIT=unreal.MaterialEditingLibrary


def load(path):
    if not LIB.does_asset_exist(path):
        raise RuntimeError("Asset missing on fresh process load: "+path)
    asset=LIB.load_asset(path)
    if asset is None:
        raise RuntimeError("Cannot load persisted asset: "+path)
    return asset


def main():
    OUTPUT.unlink(missing_ok=True)
    contract=json.loads(CONTRACT.read_text())
    imported=json.loads(REPORT.read_text())
    result={"materials":{},"meshes":{},"animations":{},"fresh_process":True,
            "material_usage_checked_before_mesh_load":True}
    paths={slot["material"] for record in contract["meshes"].values() for slot in record["slots"]}
    for path in sorted(paths):
        # Check saved flags before loading a mesh could trigger editor auto-usage.
        if not load(path).get_editor_property("used_with_skeletal_mesh"):
            raise RuntimeError("Skeletal usage was not persisted: "+path)
    shared=None
    for style,record in contract["meshes"].items():
        mesh=load(record["asset"])
        skeleton=mesh.get_editor_property("skeleton")
        if skeleton is None:
            raise RuntimeError("Mesh lost its skeleton after process restart: "+style)
        skeleton_path=skeleton.get_path_name()
        load(skeleton_path)
        if shared is None:
            shared=skeleton
        if shared!=skeleton:
            raise RuntimeError("Preset skeleton identity mismatch")
        height=mesh.get_bounds().box_extent.z*2
        if not 155<height<175:
            raise RuntimeError("Persisted scale is invalid: "+style)
        result["meshes"][style]={"height_cm":height,"skeleton":skeleton_path}
        slots=mesh.get_editor_property("materials")
        for slot in record["slots"]:
            mat=load(slot["material"])
            actual=slots[slot["index"]]
            if str(actual.get_editor_property("material_slot_name"))!=slot["slot_name"]:
                raise RuntimeError("Persisted slot ordering changed")
            if actual.get_editor_property("material_interface")!=mat:
                raise RuntimeError("Persisted material assignment changed")
            if not mat.get_editor_property("used_with_skeletal_mesh"):
                raise RuntimeError("Missing saved skeletal material usage: "+slot["material"])
            expected=unreal.BlendMode.BLEND_MASKED if slot["alpha_mask"] else unreal.BlendMode.BLEND_OPAQUE
            if mat.get_editor_property("blend_mode")!=expected:
                raise RuntimeError("Persisted alpha mode changed")
            if "ColorTint" in slot["parameters"]:
                color=EDIT.get_material_default_vector_parameter_value(mat,"ColorTint")
                if any(abs(v-1)>1e-6 for v in [color.r,color.g,color.b,color.a]):
                    raise RuntimeError("Persisted ColorTint is not neutral")
            if "IrisMix" in slot["parameters"] and abs(EDIT.get_material_default_scalar_parameter_value(mat,"IrisMix"))>1e-6:
                raise RuntimeError("Persisted IrisMix is not neutral")
            result["materials"][slot["material"]]={"used_with_skeletal_mesh":True,"alpha_mask":slot["alpha_mask"],
                                                  "neutral_parameters_verified":slot["parameters"]}
    for name,path in imported["animations"].items():
        animation=load(path)
        if animation.get_editor_property("skeleton")!=shared:
            raise RuntimeError("Persisted animation lost shared skeleton: "+name)
        result["animations"][name]={"asset":path,"skeleton":shared.get_path_name()}
    OUTPUT.write_text(json.dumps(result,indent=2))
    unreal.log("HEROINE_PERSISTED_MATERIALS_VERIFIED "+str(OUTPUT))


if __name__=="__main__":
    main()
