"""Apply the mine's ground changes (``mine_ground.py``) to the committed T_EstateGround, T_EstateCanopy and
EstateGround.bin in place, without re-running the whole ground bake. Three steps (the editor's Python has
no numpy):

    . .\\Scripts\\McpHelpers.ps1 -Port <p>; pyfile Scripts\\Mine\\patch_ground_texture.py   # 1. export the sources
    python Scripts\\Mine\\mine_ground.py                                                   # 2. patch them
    pyfile Scripts\\Mine\\patch_ground_texture.py                                         # 3. re-import them

Step 3 re-imports with Scripts/Terrain/build_ground.py's settings for data textures.
"""
import sys
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Terrain"))
import build_ground  # noqa: E402

FOLDER = "/Game/SurvivalGame/Estate/Ground"
NAMES = ("T_EstateGround", "T_EstateCanopy")
WORK = ROOT / "Saved" / "Ground" / "MinePatch"


def export(name):
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset(f"{FOLDER}/{name}")
    task.filename = str(WORK / f"{name}_source.tga")
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = unreal.TextureExporterTGA()
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError("Could not export " + name)


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    sources = [WORK / f"{n}_source.tga" for n in NAMES]
    patched = [WORK / f"{n}.png" for n in NAMES]
    if all(p.exists() for p in patched + sources) and min(p.stat().st_mtime for p in patched) > max(
            s.stat().st_mtime for s in sources):
        for name, path in zip(NAMES, patched):
            build_ground.import_texture(str(path), name, "data")
        for s in sources:
            s.unlink()
        print("mine ground: re-imported", NAMES)
    else:
        for name in NAMES:
            export(name)
        print("mine ground: exported", NAMES, "- now run python Scripts\\Mine\\mine_ground.py, then this again")


main()
