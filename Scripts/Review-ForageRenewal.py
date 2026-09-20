"""Verify bounded renewal evidence and assemble actual-frame comparisons."""
import hashlib
import json
import pathlib
import struct
import sys
import zlib
from PIL import Image, ImageDraw


def review(folder):
    root = pathlib.Path(folder)
    read = lambda name: json.loads((root / name).read_text(encoding="utf-8-sig"))
    write, reload, wrapper = (read(name) for name in ("write-result.json", "reload-result.json", "renewal-result.json"))
    assert all(result["status"] == "passed" for result in (write, reload, wrapper))
    assert write["sleeps"] == 3 and abs(write["ordinarySleepHours"] - 24) < 1e-6
    assert write["harvests"] == 3 and write["rejectedGathers"] == 6
    assert write["saves"] == 2 and write["exactRoundTrips"] == 1
    assert wrapper["separateProcessReload"] and reload["worldId"] == write["worldId"]
    assert reload["initialHour"] == write["hour"] == reload["hour"]
    assert reload["saves"] == reload["harvests"] == reload["sleeps"] == 0
    assert abs(write["naturalGameHours"] - write["engineUnpausedSeconds"] / 150) < 1e-5
    rows = [json.loads(line) for line in (root / "write-visuals.jsonl").read_text(encoding="utf-8-sig").splitlines()]
    loaded = [json.loads(line) for line in (root / "reload-visuals.jsonl").read_text(encoding="utf-8-sig").splitlines()]
    for node, count in ((8, 3), (12, 15), (10, 8)):
        select = lambda phase: next(row for row in rows if row["node"] == node and row["phase"] == phase)
        initial = select("initial")
        for phase in ("early", "rest1", "rest2", "rest3", "ready"):
            row = select(phase)
            assert row["deadline"] == initial["deadline"]
        assert select("early")["produceComponents"] == 0
        assert "(renewing)" in select("early")["focusTitle"]
        assert select("ready")["produceComponents"] == count and "(renewing)" not in select("ready")["focusTitle"]
        depleted = select("depleted-again")
        assert depleted["produceComponents"] == 0 and depleted["deadline"] > initial["deadline"]
        assert depleted["toast"].startswith("This patch needs more time to regrow")
        again = next(row for row in loaded if row["node"] == node)
        assert again["deadline"] == depleted["deadline"] and again["produceComponents"] == 0
        for rest in (1, 2, 3):
            assert select(f"rest{rest}")["ready"] == (node != 10 or rest == 3)
    for row in rows + loaded:
        if row["node"] == 14:
            assert row["cleared"] and row["baseComponents"] == row["produceComponents"] == 0
    saves = []
    for path in sorted((root / "SmokeSave").glob("*")):
        data = path.read_bytes()
        assert data[:8] == b"HOMESAV1"
        assert struct.unpack("<I", data[8:12])[0] == zlib.crc32(data[12:])
        sha = hashlib.sha256(data).hexdigest()
        assert wrapper["saveSha256"][str(path.resolve())].lower() == sha
        saves.append({"file": path.name, "sha256": sha, "crcValid": True})
    for phase in ("write", "reload"):
        log = (root / f"{phase}.log").read_text(encoding="utf-8-sig", errors="replace")
        assert not any(term in log for term in ("Fatal error:", "Assertion failed:", "Ensure condition failed:", "Unhandled Exception"))
    frames = sorted((root / "Frames").glob("*.png"))
    assert len(frames) == write["frames"] == 10
    sheet = Image.new("RGB", (1920, 1224), "#202020")
    crops = Image.new("RGB", (1200, 1044), "#202020")
    draw, detail = ImageDraw.Draw(sheet), ImageDraw.Draw(crops)
    for y, node in enumerate((8, 12, 10)):
        for x, phase in enumerate(("early", "ready", "depleted-again")):
            path = next(p for p in frames if p.name.endswith(f"-{phase}-node{node}.png"))
            row = next(r for r in rows if r["node"] == node and r["phase"] == phase)
            with Image.open(path) as image:
                assert image.size == (1920, 1080)
                resized = image.resize((640, 360))
                sheet.paste(resized, (x * 640, y * 408 + 42))
                sx, sy = row["screenX"], row["screenY"]
                box = (int(sx - 160), int(sy - 170), int(sx + 160), int(sy + 90))
                assert box[0] >= 0 and box[1] >= 0 and box[2] <= 1920 and box[3] <= 1080
                crops.paste(image.crop(box), (x * 400 + 40, y * 348 + 66))
            caption = f"Node{node} {phase} | hour{row['hour']:.3f} readyAt{row['deadline']:.3f}"
            draw.text((x * 640 + 8, y * 408 + 8), caption, fill="white")
            detail.text((x * 400 + 6, y * 348 + 8), f"Node{node} {phase}: {row['produceComponents']} produce meshes", fill="white")
            detail.text((x * 400 + 6, y * 348 + 28), row["focusTitle"], fill="white")
    sheet.save(root / "renewal-sheet.png")
    crops.save(root / "renewal-node-details.png")
    analysis = {"write": write, "reload": reload, "validSaves": saves,
                "sameNodeIds": [8, 12, 10], "clearedControl": 14, "actualFrames": 10,
                "limits": "Offscreen actual game frames plus component/render-state/collision checks. Rest advances24hours; no natural45-minute or scanout/performance claim."}
    (root / "analysis.json").write_text(json.dumps(analysis, indent=2) + "\n", encoding="utf-8")
    print(f"Renewal evidence verified: 3 nodes, 3 normal rests, {len(saves)} unchanged CRC-valid saves, 10 frames.")


if __name__ == "__main__":
    review(sys.argv[1])
