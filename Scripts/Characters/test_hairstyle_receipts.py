"""Regression checks for durable authoring-receipt file selection."""
import tempfile
import unittest
from pathlib import Path

from hairstyle_receipts import is_receipt_file


class ReceiptFilesTests(unittest.TestCase):
    def test_backups_do_not_become_required_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            names = ("body.blend", "body.fbx", "texture.png", "manifest.json",
                     "body.blend1", "body.blend2", "body.blend123", "BODY.BLEND1",
                     "body.blend.bak", "manifest.BAK")
            for name in names:
                (root / name).touch()
            self.assertEqual({p.name for p in root.iterdir() if is_receipt_file(p)},
                             {"body.blend", "body.fbx", "texture.png", "manifest.json"})

    def test_missing_files_and_directories_are_not_receipt_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.assertFalse(is_receipt_file(root))
            self.assertFalse(is_receipt_file(root / "missing.blend"))


if __name__ == "__main__":
    unittest.main()
