"""Select admitted grass geometry without importing a scene or opening image references."""
import hashlib
import json
import os
from pathlib import Path
import stat
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from TreePreparation import geometry_only_fbx


ROOT = Path(__file__).resolve().parents[2]
RUN = "20260921-033354-2d257ba0"
NAMES = ("mid_b", "small_b", "tall_a", "tiny_a")
TRIANGLES = (1257, 653, 290, 79)


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest().upper()


def ordinary(path):
    for part in (path, *path.parents):
        if part.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
            raise ValueError(f"Source/output ancestor is a reparse point: {part}")


def active():
    run = json.loads((ROOT / "Automation" / "run.json").read_text(encoding="utf-8-sig"))
    if run["id"] != RUN or run["state"] != "running" or run["completionPolicy"] != "until-complete":
        raise RuntimeError("The admitted completion-driven run is no longer active.")
    stop = os.environ.get("HOMESTEAD_SOURCE_STOP_PATH")
    if not stop:
        raise RuntimeError("The owned source supervisor's stop path is required.")
    if Path(stop).exists():
        raise RuntimeError("Source preparation cancelled at a polling boundary.")


def main():
    active()
    receipt_path = ROOT / "Assets" / "Environment" / "woodland-preparation-01" / "download-receipt.json"
    inventory_path = receipt_path.with_name("source-inventory.json")
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    if receipt["sourceRoot"] != r"Assets\Source\woodland-preparation-20260920-182217-d1f84e39":
        raise ValueError("Original source receipt root differs.")
    source = ROOT / receipt["sourceRoot"] / "grass_medium_01" / "grass_medium_01_1k.fbx"
    ordinary(source)
    records = [entry for entry in receipt["files"]
               if entry["asset"] == "grass_medium_01" and entry["file"] == source.name]
    if (len(records) != 1 or source.stat().st_size != records[0]["bytes"]
            or digest(source) != "EF823EBA5C7114B98FB544A85AB7B17DD1C2DDD6F3E785CD232029B980AE7C44"
            or records[0]["sha256"].upper() != digest(source)):
        raise ValueError("Admitted original grass FBX identity differs.")
    inventory = json.loads(inventory_path.read_text(encoding="utf-8-sig"))
    entries = [entry for entry in inventory["files"] if entry["file"] == source.name]
    if len(entries) != 1 or entries[0]["sha256"].upper() != digest(source):
        raise ValueError("Original source inventory does not identify this FBX.")
    inspection = entries[0]["sourceInspection"]
    names = [f"grass_medium_01_{name}_LOD0" for name in NAMES]
    models = [model for name in names for model in inspection["models"] if model["name"] == name]
    if len(models) != 4 or [model["name"] for model in models] != names:
        raise ValueError("Selected source model inventory differs.")
    output = ROOT / "Assets" / "Environment" / "GrassMedium01Prepared" / "v1"
    if output.exists():
        raise FileExistsError("Fresh selected-grass output required.")
    ordinary(ROOT / "Assets" / "Environment")
    if output.parent.exists():
        ordinary(output.parent)
    active()
    output.mkdir(parents=True)
    target = output / "GrassMedium01_Selected.fbx"
    result = geometry_only_fbx(source, target, names)
    active()
    if digest(source) != records[0]["sha256"].upper():
        raise ValueError("Original source changed during preparation.")
    provenance = {
        "state": "selected-source-only-not-unreal-imported",
        "source": str(source.relative_to(ROOT)),
        "sourceSha256": digest(source),
        "sourceReceiptSha256": digest(receipt_path),
        "sourceInventorySha256": digest(inventory_path),
        "toolSha256": digest(Path(__file__)),
        "geometryHelperSha256": digest(Path(__file__).with_name("TreePreparation.py")),
        "file": target.name,
        "sha256": digest(target),
        "bytes": target.stat().st_size,
        "selection": result,
        "models": models,
        "sourceFanTriangles": dict(zip(names, TRIANGLES)),
        "totalSourceFanTriangles": sum(TRIANGLES),
        "unchanged": "Retained object IDs, geometry arrays, UVs, normals, material and model transforms compared after FBX reparse.",
        "operations": "Binary parser/encoder only; no bpy scene import, decimation, image load, shader/render or scale edit.",
        "limits": "Source fan estimates and source transforms only; actual UE bounds/triangles/slots/units/materials/anchors remain unverified.",
    }
    with (output / "provenance.json").open("x", encoding="utf-8") as target_file:
        json.dump(provenance, target_file, indent=2)
        target_file.write("\n")
    print(json.dumps(provenance, indent=2))


if __name__ == "__main__":
    main()
