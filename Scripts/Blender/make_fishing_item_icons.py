"""Make original item thumbnails from existing hero images; no new renders.

Meal, catch and pole icons are 256px thumbnails of their hero renders. The raw beach
gatherables already have hand-framed 256px icons committed beside their props; those are
copied byte-for-byte rather than re-thumbnailed.

Usage: python Scripts\\Blender\\make_fishing_item_icons.py
"""
import hashlib
import json
import shutil
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ICON_SIZE = 256
# (item, committed 256px icon) for raw beach gatherables; copied verbatim.
COPIED_ICONS = (
    ("Seashell", Path("Assets") / "Props" / "BeachShells" / "icon_seashell_256.png"),
    ("Driftwood", Path("Assets") / "Props" / "Driftwood" / "icon_driftwood_256.png"),
    ("Mussels", Path("Assets") / "Props" / "ShoreRock" / "icon_mussels_256.png"),
    ("Laver", Path("Assets") / "Props" / "ShoreRock" / "icon_laver_256.png"),
)


def icon_key(item: str) -> str:
    return "".join(("-" + char.lower()) if char.isupper() and index else char.lower()
                   for index, char in enumerate(item))


def main() -> None:
    food = ROOT / "Assets" / "Props" / "PreparedFood"
    report = json.loads((food / "report.json").read_text())
    entries = []
    for item, source in report["sources"].items():
        directory = (ROOT / source["blend"]).parent
        image = directory / ("beauty_SM_" + item + "_hero.png")
        if not image.is_file():
            raise RuntimeError("Missing original meal hero: " + str(image))
        entries.append((item, image))
    for item in ("RiverTrout", "RiverSalmon", "LakePerch", "LakeCarp", "SeaMackerel", "SeaBass"):
        entries.append((item, ROOT / "Assets" / "Props" / "CaughtFish" / ("beauty_SM_" + item + "_hero.png")))
    entries.append(("FishingPole", ROOT / "Assets" / "Props" / "FishingPole" / "beauty_SM_FishingPole_hero.png"))
    out = ROOT / "Content" / "SurvivalGame" / "UI" / "ItemIcons"
    out.mkdir(parents=True, exist_ok=True)
    images = {}
    for item, source in entries:
        key = icon_key(item)
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
    if len(images) != 21 or len(report["item_meshes"]) != 14:
        raise RuntimeError("Original imagery must cover fourteen admitted meals, six catches and the pole.")
    for item, relative in COPIED_ICONS:
        source = ROOT / relative
        with Image.open(source) as icon:
            if icon.size != (ICON_SIZE, ICON_SIZE) or icon.mode != "RGBA":
                raise RuntimeError("Copied icon must be a 256px RGBA PNG: " + str(source))
        destination = out / (icon_key(item) + ".png")
        shutil.copyfile(source, destination)
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if hashlib.sha256(destination.read_bytes()).hexdigest() != digest:
            raise RuntimeError("Copied icon differs from its source: " + str(source))
        images[icon_key(item)] = {"item": item, "source": relative.as_posix(), "copied": True,
                                  "source_sha256": digest, "sha256": digest}
    if len(images) != 25:
        raise RuntimeError("Item imagery must cover 21 rendered and 4 copied beach icons.")
    (out / "report.json").write_text(json.dumps({
        "source_freeze_commit": report["source_freeze_commit"], "provisional_appearance": True,
        "source": "Original existing hero images and committed original beach-prop icons; "
                  "no reused game artwork or new render.",
        "size_px": ICON_SIZE, "images": images}, indent=2) + "\n", encoding="utf-8")
    print("ORIGINAL_ITEM_IMAGES_READY", len(images))


if __name__ == "__main__":
    main()
