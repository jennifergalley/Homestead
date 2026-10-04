"""Make original item thumbnails from existing hero images; no new renders.

Usage: python Scripts\\Blender\\make_fishing_item_icons.py
"""
import hashlib
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ICON_SIZE = 256


def main() -> None:
    food = ROOT / "Assets" / "Props" / "PreparedFood"
    report = json.loads((food / "report.json").read_text())
    entries = []
    for item, source in report["sources"].items():
        directory = (ROOT / source["blend"]).parent
        image = directory / ("beauty_SM_" + item + "_hero.png")
        if not image.is_file():
            raise RuntimeError("Missing frozen original meal hero: " + str(image))
        entries.append((item, image))
    for item in ("RiverTrout", "RiverSalmon", "LakePerch", "LakeCarp", "SeaMackerel", "SeaBass"):
        entries.append((item, ROOT / "Assets" / "Props" / "CaughtFish" / ("beauty_SM_" + item + "_hero.png")))
    entries.append(("FishingPole", ROOT / "Assets" / "Props" / "FishingPole" / "beauty_SM_FishingPole_hero.png"))
    out = ROOT / "Content" / "SurvivalGame" / "UI" / "ItemIcons"
    out.mkdir(parents=True, exist_ok=True)
    images = {}
    for item, source in entries:
        key = "".join(("-" + char.lower()) if char.isupper() and index else char.lower()
                      for index, char in enumerate(item))
        with Image.open(source) as original:
            thumbnail = original.convert("RGBA")
            thumbnail.thumbnail((ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS)
            canvas = Image.new("RGBA", (ICON_SIZE, ICON_SIZE))
            canvas.alpha_composite(thumbnail, ((ICON_SIZE - thumbnail.width) // 2, (ICON_SIZE - thumbnail.height) // 2))
            destination = out / (key + ".png")
            canvas.save(destination, optimize=True)
        images[key] = {"item": item, "source": source.relative_to(ROOT).as_posix(),
                       "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                       "sha256": hashlib.sha256(destination.read_bytes()).hexdigest()}
    if len(images) != 18 or len(report["item_meshes"]) != 11:
        raise RuntimeError("Original imagery must cover eleven admitted meals, six catches and the pole.")
    (out / "report.json").write_text(json.dumps({
        "source_freeze_commit": report["source_freeze_commit"], "provisional_appearance": True,
        "source": "Original existing hero images; no reused game artwork or new render.",
        "size_px": ICON_SIZE, "images": images}, indent=2) + "\n", encoding="utf-8")
    print("ORIGINAL_ITEM_IMAGES_READY", len(images))


if __name__ == "__main__":
    main()
