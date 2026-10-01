"""Tests for the Animation Inspector's sheet builder (Scripts/anim_inspector_sheet.py) on a synthetic recording.

    python -m unittest Tests/AnimInspectorSheetTests.py

The recording is the forward-kinematics skeleton from JointLimitsTests bending its left elbow back past
hyperextension, written in AHomesteadAnimInspector's file layout with plain grey captures.
"""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Scripts"))
sys.path.insert(0, str(ROOT / "Tests"))
import anim_inspector_sheet as sheet  # noqa: E402
import JointLimitsTests as fk  # noqa: E402
from PIL import Image  # noqa: E402

SIZE = 256
MESH = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]


def flat(pose):
    return [list(pose[b][0]) + list(pose[b][1]) for b, _, _ in fk.SKELETON]


def write_recording(directory, frames, every=1):
    bones = [b for b, _, _ in fk.SKELETON]
    parents = {b: (p or "") for b, p, _ in fk.SKELETON}
    rec = {"action": "Synthetic", "fps": 30, "every": every, "bones": bones, "frames": [
        {"i": i, "t": i / 30.0, "busy": True, "weight": 1.0, "mesh": MESH, "pose": flat(p), "props": [
            {"name": "Held", "mesh": "SM_Test", "xform": MESH, "centre": [0, 30, 100], "extent": [5, 5, 40]}],
         "ground": {"foot_l": 0.0, "foot_r": 0.0}} for i, p in enumerate(frames)]}
    (directory / "frames.json").write_text(json.dumps(rec))
    (directory / "neutral.json").write_text(json.dumps({"bones": dict(zip(bones, flat(fk.NEUTRAL))), "parents": parents}))
    views = [
        {"name": "front", "ortho": True, "width": 240, "fov": 35, "location": [0, 600, 95],
         "forward": [0, -1, 0], "right": [-1, 0, 0], "up": [0, 0, 1]},
        {"name": "threequarter", "ortho": False, "width": 240, "fov": 40, "location": [270, 270, 155],
         "forward": [-0.7, -0.7, -0.15], "right": [0.7, -0.7, 0], "up": [0, 0, 1]},
    ]
    (directory / "cameras.json").write_text(json.dumps({"resolution": SIZE, "views": views}))
    for view in views:
        (directory / view["name"]).mkdir()
        for i in range(0, len(frames), every):
            Image.new("RGB", (SIZE, SIZE), (110, 110, 105)).save(directory / view["name"] / f"f{i:04d}.png")


class SheetBuilder(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp(prefix="anim-inspector-test-"))

    def tearDown(self):
        shutil.rmtree(self.dir, ignore_errors=True)

    def test_overlays_sheets_gif_and_index(self):
        frames = [fk.pose({"lowerarm_l": [(fk.X, 30 - 5 * i)]}) for i in range(10)]
        write_recording(self.dir, frames)
        index = Path(sheet.run(str(self.dir), thumb=128))
        for name in ("sheet_front.png", "sheet_threequarter.png", "keyframes.png", "motion.gif", "index.md"):
            self.assertTrue((self.dir / name).exists(), name)
        self.assertTrue((self.dir / "front" / "o0000.png").exists())
        text = index.read_text(encoding="utf-8")
        self.assertIn("elbow_l.flex", text)
        self.assertIn("| 9 |", text)
        # The hyperextended forearm is drawn red on the last frame's front view.
        with Image.open(self.dir / "front" / "o0009.png") as image:
            colours = [c for _, c in image.convert("RGB").getcolors(1 << 20)]
        self.assertIn(sheet.RED, colours)
        with Image.open(self.dir / "sheet_front.png") as image:
            self.assertLessEqual(image.width, 2000)

    def test_capture_every_other_frame(self):
        write_recording(self.dir, [fk.NEUTRAL] * 6, every=2)
        sheet.run(str(self.dir), thumb=96)
        self.assertTrue((self.dir / "front" / "o0004.png").exists())
        self.assertFalse((self.dir / "front" / "o0003.png").exists())

    def test_recipe_frames_from_source(self):
        self.assertEqual(sheet.recipe_frames("axe_fell")["strike1"], 34)
        self.assertEqual(sheet.recipe_frames("ground_strike"), sheet.recipe_frames("axe_fell"))
        self.assertIn("grab1", sheet.recipe_frames("kneel_gather"))

    def test_issue_bones(self):
        self.assertEqual(sheet.issue_bones("elbow_l.flex"), ["lowerarm_l"])
        self.assertEqual(sheet.issue_bones("index_pip_r.flex"), ["index_02_r"])
        self.assertEqual(sheet.issue_bones("thumb_mcp_l.flex"), ["thumb_02_l"])
        self.assertEqual(sheet.issue_bones("lumbar.twist"), ["spine_01", "spine_02"])


if __name__ == "__main__":
    unittest.main()
