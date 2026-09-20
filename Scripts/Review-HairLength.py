"""Same-scale local sheets for the fixed fixture; never includes private references."""
import argparse
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw


def frame(path):
    return dict(line.split("=", 1) for line in path.read_text().splitlines() if "=" in line)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline", type=Path)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--candidate-label", default="candidate")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    labels = [(body, outfit) for body in ("preferred", "willow", "hazel") for outfit in ("tunic", "apron")]
    checks = {}
    for body, outfit in labels:
        columns = 4 if args.candidate else 2
        sheet = Image.new("RGB", (310 * columns, 580), "#202428")
        draw = ImageDraw.Draw(sheet)
        for view_index, view in enumerate(("back", "angle")):
            name = f"hair-{body}-{outfit}-{view}"
            old = frame(args.baseline / (name + ".frame.txt"))
            sources = [("baseline", args.baseline)]
            if args.candidate:
                new = frame(args.candidate / (name + ".frame.txt"))
                keys = ("viewport", "actor", "camera", "camera_rotation", "mesh_location",
                        "mesh_rotation", "head", "head_projected", "neck_01", "spine_01")
                for key in keys:
                    old_numbers = [float(n) for n in re.findall(r"-?\d+(?:\.\d+)?", old[key])]
                    new_numbers = [float(n) for n in re.findall(r"-?\d+(?:\.\d+)?", new[key])]
                    if not old_numbers or old_numbers != new_numbers:
                        raise RuntimeError(f"Nonmatching fixture {name}: {key}: {old[key]} != {new[key]}")
                checks[name] = {"matched_framing": True, "dimensions": [1920, 1080],
                                "candidate_mesh": new["mesh_asset"]}
                sources.append((args.candidate_label, args.candidate))
            for source_index, (label, directory) in enumerate(sources):
                image = Image.open(directory / (name + ".png"))
                if image.size != (1920, 1080):
                    raise RuntimeError("Wrong actual dimensions: " + str(directory / name))
                column = view_index * len(sources) + source_index
                sheet.paste(image.crop((650, 0, 1270, 1080)).resize((310, 540)), (column * 310, 40))
                draw.text((column * 310 + 8, 8), f"{body}/{outfit} {view} {label}", fill="white")
        sheet.save(args.output / f"{body}-{outfit}.png")
    (args.output / "framing-verification.json").write_text(json.dumps(checks, indent=2) + "\n")
    width = 1240 if args.candidate else 620
    overview = Image.new("RGB", (width * 2, 580 * 3), "#202428")
    for index, (body, outfit) in enumerate(labels):
        overview.paste(Image.open(args.output / f"{body}-{outfit}.png"),
                       ((index % 2) * width, (index // 2) * 580))
    overview.save(args.output / "overview.png")
    print("HAIR_REVIEW_SHEETS", args.output)


if __name__ == "__main__":
    main()
