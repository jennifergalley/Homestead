"""Apply the mine house's grass clearance (``mine_ground.house_ground``) to the committed T_EstateGround in
place, without re-running the whole ground bake. Three steps (the editor's Python has no numpy):

    . .\\Scripts\\McpHelpers.ps1 -Port <p>; pyfile Scripts\\Mine\\patch_ground_texture.py   # 1. export the source
    python Scripts\\Mine\\mine_ground.py                                                   # 2. patch it
    pyfile Scripts\\Mine\\patch_ground_texture.py                                         # 3. re-import it

Step 2 zeroes grass density (R) and raises wear (A) round the house only; step 3 re-imports with
Scripts/Terrain/build_ground.py's settings for data textures. Idempotent.
"""
import sys
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Terrain"))
import build_ground  # noqa: E402

ASSET = "/Game/SurvivalGame/Estate/Ground/T_EstateGround"
WORK = ROOT / "Saved" / "Ground" / "MinePatch"
SOURCE, PATCHED = WORK / "T_EstateGround_source.tga", WORK / "T_EstateGround.png"


def export():
    WORK.mkdir(parents=True, exist_ok=True)
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset(ASSET)
    task.filename = str(SOURCE)
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = unreal.TextureExporterTGA()
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError("Could not export " + ASSET)
    print(f"mine ground: exported {SOURCE}; now run python Scripts\\Mine\\mine_ground.py, then this again")


def main():
    if PATCHED.exists() and SOURCE.exists() and PATCHED.stat().st_mtime > SOURCE.stat().st_mtime:
        build_ground.import_texture(str(PATCHED), "T_EstateGround", "data")
        SOURCE.unlink()
        print(f"mine ground: re-imported {ASSET} from {PATCHED}")
    else:
        export()


main()
