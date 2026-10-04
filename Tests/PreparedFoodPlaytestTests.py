"""Validate the frozen original runtime manifest, images and mappings without UE.

Usage: python Tests\\PreparedFoodPlaytestTests.py
"""
import hashlib
import json
import re
import subprocess
from pathlib import Path

from PIL import Image, ImageStat

ROOT = Path(__file__).resolve().parents[1]
FREEZE = "843324c17b85b5432785fc7a6b9e333702dc26ec"
ADMITTED = {
    "BakedPotatoes", "RoastedTurnips", "StewedCarrots", "HerbedBroadBeans",
    "CabbagePotatoStew", "BerryCompote", "StrawberryCompote", "RootVegetableHotpot",
    "RawFishSlices", "GrilledTrout", "GrilledPerch",
}
DEFERRED = {"GrilledMackerel", "FishSoup", "FishAndPotatoes", "HerbedCarp", "MackerelChowder"}
SPOON_DISHES = {"CabbagePotatoStew", "BerryCompote", "StrawberryCompote", "RootVegetableHotpot"}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    food = ROOT / "Assets" / "Props" / "PreparedFood"
    report = json.loads((food / "report.json").read_text())
    assert report["source_freeze_commit"] == FREEZE
    assert report["provisional_appearance"] and not report["final_realism_accepted"]
    assert set(report["item_meshes"]) == ADMITTED
    assert set(report["sources"]) == ADMITTED and set(report["deferred_items"]) == DEFERRED
    expected = {"SM_" + item + suffix for item in ADMITTED for suffix in ("", "Portion")}
    assert set(report["meshes"]) == expected
    assert {path.stem for path in food.glob("SM_*.fbx")} == expected
    maps = set()
    for name, mesh in report["meshes"].items():
        assert digest(food / mesh["fbx"]) == mesh["sha256"], name
        assert mesh["triangles"] <= report["triangle_budget"]
        assert all(size > 0 for size in mesh["size_cm"])
        bake = mesh["bake"]
        assert bake["size"] == 1024 and bake["samples"] == 16
        assert set(bake["maps"]) == {"basecolor", "roughness", "normal"}
        for kind, filename in bake["maps"].items():
            maps.add(filename)
            with Image.open(food / "Textures" / filename) as image:
                assert image.size == (1024, 1024)
                rgb = image.convert("RGB")
                assert max(high for _, high in rgb.getextrema()) > 32, filename
                if kind == "normal":
                    # Neutral tangent normals are valid; provisional admission is not a relief-quality gate.
                    assert rgb.getextrema()[2][1] >= 200, filename
                else:
                    assert max(ImageStat.Stat(rgb).stddev) > 1, (name, kind)
        item = name.removeprefix("SM_").removesuffix("Portion")
        if name.endswith("Portion") and item in SPOON_DISHES:
            assert mesh["derivative"]["kind"] == "frozen-edible-components-only"
            assert mesh["size_cm"][1] < 6, name
    assert len(maps) == 66
    assert {path.name for path in (food / "Textures").glob("*.png")} == maps
    for source in report["sources"].values():
        blend = ROOT / source["blend"]
        pointer = subprocess.check_output(["git", "show", FREEZE + ":" + blend.relative_to(ROOT).as_posix()],
                                          cwd=ROOT, text=True)
        oid = next(line.removeprefix("oid sha256:") for line in pointer.splitlines() if line.startswith("oid sha256:"))
        assert digest(blend) == source["sha256"] == oid
    header = (ROOT / "Source" / "SurvivalGame" / "HomesteadOriginalItemArt.h").read_text()
    entries = re.findall(r'\{Homestead::Item::(\w+), "([^"]+)", "([^"]+)", "([^"]+)", ("[^"]+"|nullptr), (true|false)\}', header)
    assert len(entries) == 18
    assert {item for item, _, _, _, portion, _ in entries if portion != "nullptr"} == ADMITTED
    assert not ({entry[0] for entry in entries} & DEFERRED)
    icons = ROOT / "Content" / "SurvivalGame" / "UI" / "ItemIcons"
    imagery = json.loads((icons / "report.json").read_text())
    assert set(imagery["images"]) == {entry[1] for entry in entries}
    for key, image in imagery["images"].items():
        path = icons / (key + ".png")
        assert digest(path) == image["sha256"]
        assert digest(ROOT / image["source"]) == image["source_sha256"]
        with Image.open(path) as thumbnail:
            assert thumbnail.size == (256, 256) and thumbnail.mode == "RGBA"
            assert thumbnail.getextrema()[3][1] == 255
    config = (ROOT / "Config" / "DefaultGame.ini").read_text()
    assert '+DirectoriesToAlwaysStageAsUFS=(Path="SurvivalGame/UI/ItemIcons")' in config
    print("Frozen playtest art: 11 original meals, 22 meshes, 66 maps, 18 images; zero deferred runtime assets.")


if __name__ == "__main__":
    main()
