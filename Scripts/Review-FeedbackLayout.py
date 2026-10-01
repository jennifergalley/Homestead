"""Validate native draw evidence and compose the two bounded feedback review sets."""
import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    reports = []
    for size in ("720", "4k"):
        folder = args.directory / size
        result = json.loads((folder / "feedback-result.json").read_text(encoding="utf-8-sig"))
        layouts = sorted(folder.glob("*.layout.json"))
        sheet = Image.new("RGB", (1920, ((len(layouts) + 2) // 3) * 400), "#132019")
        draw = ImageDraw.Draw(sheet)
        for index, path in enumerate(layouts):
            data = json.loads(path.read_text(encoding="utf-8-sig"))
            drawn = " ".join(line["text"] for line in data["drawnToastLines"])
            if data["source"].split() != drawn.split():
                raise ValueError(f"Draw calls omitted feedback text: {path}")
            if not data["insideViewport"] or data["overlap"] != (result["baseline"] and data.get("surface") != "native-notice-card"):
                raise ValueError(f"Unexpected bounds/intersection: {path}")
            panel = data["toastPanel"]
            if not (0 <= panel["left"] < panel["right"] <= data["viewportWidth"]
                    and 0 <= panel["top"] < panel["bottom"] <= data["viewportHeight"]):
                raise ValueError(f"Panel leaves viewport: {path}")
            for line in data["drawnToastLines"]:
                if not (panel["left"] <= line["left"] <= line["right"] <= panel["right"]
                        and panel["top"] <= line["top"] <= line["bottom"] <= panel["bottom"]):
                    raise ValueError(f"Drawn text leaves its backing: {path}")
            for region in data["protected"]:
                intersection = (panel["left"] < region["right"] and panel["right"] > region["left"]
                                and panel["top"] < region["bottom"] and panel["bottom"] > region["top"])
                if intersection != region["overlap"]:
                    raise ValueError(f"Native intersection report differs from recorded draw bounds: {path}")
            image_path = path.with_name(path.name.replace(".layout.json", ".png"))
            with Image.open(image_path) as image:
                if image.size != (result["width"], result["height"]):
                    raise ValueError(f"Incorrect native dimensions: {path}")
                thumb = image.convert("RGB").resize((640, 360), Image.Resampling.LANCZOS)
            x, y = (index % 3) * 640, (index // 3) * 400
            sheet.paste(thumb, (x, y + 36))
            draw.text((x + 8, y + 5), f"{size} {path.stem.removesuffix('.layout')}", fill="#f1e7c9")
            reports.append({
                "capture": str(image_path),
                "source": data["source"],
                "lines": len(data["drawnToastLines"]),
                "intersections": [r["role"] for r in data["protected"] if r["overlap"]],
                "panel": data["toastPanel"],
            })
        sheet.save(args.directory / f"feedback-{size}-sheet.png")
    (args.directory / "analysis.json").write_text(json.dumps(reports, indent=2), encoding="utf-8")
    print(f"Verified {len(reports)} actual active-toast draw/capture records.")


if __name__ == "__main__":
    main()
