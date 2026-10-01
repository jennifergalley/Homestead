"""Builds the UI gallery's contact sheets, viewable copies and index from a Capture-UiGallery run.

Usage: python Scripts/ui_gallery_sheet.py <stamp folder>

The stamp folder holds one folder per resolution and input (720p, 4K-pad, ...), each with the run's
<id>.png captures and gallery-index.tsv (id <tab> description). For each folder it writes:
  view/<id>.jpg   - a copy at most VIEW_WIDTH wide, small enough for an agent's image viewer
  contact.jpg     - every capture as a labelled thumbnail grid
and in the stamp folder index.md (id, description, the capture at each resolution), plus, where a
<res>-classic and a <res>-parchment folder both exist, side-by-side-<res>-NN.jpg pages: each screen's
classic capture beside its parchment one, for the theme decision. Generated files; do not edit by hand.
"""
from __future__ import annotations

import pathlib
import sys

from PIL import Image, ImageDraw, ImageFont

VIEW_WIDTH = 1920
VIEW_QUALITY = 88
THUMB_WIDTH = 480
COLUMNS = 4
LABEL_HEIGHT = 28
SHEET_QUALITY = 85
BACKGROUND = (24, 28, 26)
LABEL_COLOUR = (236, 226, 200)


def read_index(folder: pathlib.Path) -> list[tuple[str, str]]:
    index = folder / "gallery-index.tsv"
    if not index.exists():
        return []
    rows = []
    for line in index.read_text(encoding="utf-8").splitlines():
        if line.strip():
            identifier, _, description = line.partition("\t")
            rows.append((identifier, description))
    return rows


def font(size: int):
    for name in ("segoeui.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def build_folder(folder: pathlib.Path, rows: list[tuple[str, str]]) -> list[str]:
    view = folder / "view"
    view.mkdir(exist_ok=True)
    present = []
    thumbs = []
    for identifier, _ in rows:
        capture = folder / f"{identifier}.png"
        if not capture.exists():
            continue
        present.append(identifier)
        with Image.open(capture) as image:
            image = image.convert("RGB")
            scaled = image if image.width <= VIEW_WIDTH else image.resize(
                (VIEW_WIDTH, round(image.height * VIEW_WIDTH / image.width)), Image.LANCZOS)
            scaled.save(view / f"{identifier}.jpg", quality=VIEW_QUALITY)
            thumbs.append((identifier, image.resize((THUMB_WIDTH, round(image.height * THUMB_WIDTH / image.width)), Image.LANCZOS)))
    if thumbs:
        cell_height = max(thumb.height for _, thumb in thumbs) + LABEL_HEIGHT
        rows_needed = (len(thumbs) + COLUMNS - 1) // COLUMNS
        sheet = Image.new("RGB", (COLUMNS * THUMB_WIDTH, rows_needed * cell_height), BACKGROUND)
        draw = ImageDraw.Draw(sheet)
        label_font = font(18)
        for number, (identifier, thumb) in enumerate(thumbs):
            x = (number % COLUMNS) * THUMB_WIDTH
            y = (number // COLUMNS) * cell_height
            sheet.paste(thumb, (x, y + LABEL_HEIGHT))
            draw.text((x + 6, y + 4), identifier, fill=LABEL_COLOUR, font=label_font)
        sheet.save(folder / "contact.jpg", quality=SHEET_QUALITY)
    return present


PAIR_WIDTH = 900
PAIRS_PER_PAGE = 8


def build_side_by_side(stamp: pathlib.Path, order: list[str], descriptions: dict[str, str]) -> list[pathlib.Path]:
    pages = []
    for classic in sorted(path for path in stamp.glob("*-classic*") if path.is_dir()):
        parchment = stamp / classic.name.replace("-classic", "-parchment")
        if not parchment.is_dir():
            continue
        both = [i for i in order if (classic / f"{i}.png").exists() and (parchment / f"{i}.png").exists()]
        label_font = font(22)
        for page in range(0, len(both), PAIRS_PER_PAGE):
            chunk = both[page:page + PAIRS_PER_PAGE]
            rows = []
            for identifier in chunk:
                images = []
                for folder in (classic, parchment):
                    with Image.open(folder / f"{identifier}.png") as image:
                        image = image.convert("RGB")
                        images.append(image.resize((PAIR_WIDTH, round(image.height * PAIR_WIDTH / image.width)), Image.LANCZOS))
                rows.append((identifier, images))
            row_height = max(images[0].height for _, images in rows) + LABEL_HEIGHT + 8
            sheet = Image.new("RGB", (PAIR_WIDTH * 2 + 12, 40 + row_height * len(rows)), BACKGROUND)
            draw = ImageDraw.Draw(sheet)
            draw.text((8, 8), "classic", fill=LABEL_COLOUR, font=label_font)
            draw.text((PAIR_WIDTH + 20, 8), "parchment", fill=LABEL_COLOUR, font=label_font)
            for number, (identifier, images) in enumerate(rows):
                y = 40 + number * row_height
                draw.text((8, y + 2), f"{identifier}: {descriptions.get(identifier, '')}"[:160], fill=LABEL_COLOUR, font=font(16))
                sheet.paste(images[0], (0, y + LABEL_HEIGHT))
                sheet.paste(images[1], (PAIR_WIDTH + 12, y + LABEL_HEIGHT))
            resolution = classic.name.replace("-classic", "")
            out = stamp / f"side-by-side-{resolution}-{page // PAIRS_PER_PAGE + 1:02d}.jpg"
            sheet.save(out, quality=SHEET_QUALITY)
            pages.append(out)
    return pages


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    stamp = pathlib.Path(sys.argv[1])
    folders = sorted(path for path in stamp.iterdir() if path.is_dir() and (path / "gallery-index.tsv").exists())
    descriptions: dict[str, str] = {}
    order: list[str] = []
    captured: dict[str, list[str]] = {}
    for folder in folders:
        rows = read_index(folder)
        for identifier, description in rows:
            if identifier not in descriptions:
                descriptions[identifier] = description
                order.append(identifier)
        captured[folder.name] = build_folder(folder, rows)
    lines = ["# UI gallery", "", f"Captured into `{stamp}`. Contact sheets: "
             + ", ".join(f"[{name}]({name}/contact.jpg)" for name in captured) + ".", ""]
    lines.append("| id | what she should see | " + " | ".join(captured) + " |")
    lines.append("|---|---|" + "---|" * len(captured))
    for identifier in order:
        cells = [f"[view]({name}/view/{identifier}.jpg)" if identifier in present else "skipped"
                 for name, present in captured.items()]
        lines.append(f"| `{identifier}` | {descriptions[identifier]} | " + " | ".join(cells) + " |")
    sides = build_side_by_side(stamp, order, descriptions)
    if sides:
        lines += ["", "## Classic | parchment", ""] + [f"- [{path.name}]({path.name})" for path in sides]
    (stamp / "index.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(stamp / "index.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
