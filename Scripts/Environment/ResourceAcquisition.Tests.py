"""Offline checks adapted from Tests/WoodlandSourceTests.py; no network/engine."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import MagicMock, patch

SPEC = importlib.util.spec_from_file_location("resource_fetch", Path(__file__).with_name("ResourceAcquisition.py"))
fetch = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fetch)


class AcquisitionTests(unittest.TestCase):
    def item(self):
        return {"name": "shrub_04_diff_1k.png", "bytes": 3,
                "url": "https://dl.polyhaven.org/file/ph-assets/Models/png/1k/shrub_04/shrub_04_diff_1k.png",
                "publisherMd5": hashlib.md5(b"abc").hexdigest()}

    def test_admitted_manifest(self):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads((root / "Assets/Environment/WoodlandResources/candidate01/asset-manifest.json").read_text())
        self.assertEqual(sum(len(asset["files"]) for asset in manifest["assets"]), 29)
        self.assertGreater(fetch.validate_manifest(manifest), 0)

    def test_exact_tree_palette_manifest(self):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads((root / "Assets/Environment/TreePalette20260921/candidate01/asset-manifest.json").read_text())
        self.assertEqual(tuple(a["id"] for a in manifest["assets"]), fetch.TREE_PALETTE)
        self.assertEqual(sum(len(asset["files"]) for asset in manifest["assets"]), 38)
        self.assertGreater(fetch.validate_manifest(manifest), 0)
        for change in ("profile", "asset", "fbx-size", "fbx-hash", "missing-map"):
            value = copy.deepcopy(manifest)
            if change == "profile":
                value.pop("profile")
            elif change == "asset":
                value["assets"][0]["id"] = "fir_tree_01"
            elif change == "fbx-size":
                value["assets"][0]["files"][0]["bytes"] += 1
            elif change == "fbx-hash":
                value["assets"][1]["files"][0]["publisherMd5"] = "0" * 32
            else:
                value["assets"][2]["files"].pop()
            with self.subTest(change=change), self.assertRaises(ValueError):
                fetch.validate_manifest(value)

    def test_exact_mature_fir_exception(self):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads((root / "Assets/Environment/MatureFir20260921/candidate01/asset-manifest.json").read_text())
        self.assertEqual(fetch.validate_manifest(manifest), 284614153)
        fbx = manifest["assets"][0]["files"][0]
        fetch.validate_file("fir_tree_01", fbx, allow_large_fir=True)
        with self.assertRaises(ValueError):
            fetch.validate_file("fir_tree_01", fbx)
        for change in ("profile", "size", "hash", "extra"):
            value = copy.deepcopy(manifest)
            if change == "profile":
                value.pop("profile")
            elif change == "size":
                value["assets"][0]["files"][0]["bytes"] -= 1
            elif change == "hash":
                value["assets"][0]["files"][0]["publisherMd5"] = "0" * 32
            else:
                value["assets"][0]["files"].append(copy.deepcopy(value["assets"][0]["files"][-1]))
            with self.subTest(change=change), self.assertRaises(ValueError):
                fetch.validate_manifest(value)

    def test_unsafe_names_and_paths(self):
        for name in ("..", "../x.png", r"..\x.png", r"C:\x.png", r"\\host\x", "x:ads.png", "CON.png", "trailing."):
            with self.subTest(name=name), self.assertRaises(ValueError):
                fetch.leaf_name(name)
        with tempfile.TemporaryDirectory() as directory, self.assertRaises(ValueError):
            fetch.contained(Path(directory), "..", "outside")

    def test_exact_midstory_source_only_probe(self):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads((root / "Assets/Environment/MidstoryShrub02/candidate01/asset-manifest.json").read_text())
        self.assertEqual(fetch.validate_manifest(manifest), 832300)
        for change in ("profile", "extra-map", "size", "hash", "wrong-asset"):
            value = copy.deepcopy(manifest)
            if change == "profile":
                value.pop("profile")
            elif change == "extra-map":
                value["assets"][0]["files"].append(self.item())
            elif change == "size":
                value["assets"][0]["files"][0]["bytes"] += 1
            elif change == "hash":
                value["assets"][0]["files"][0]["publisherMd5"] = "0" * 32
            else:
                value["assets"][0]["id"] = "shrub_04"
            with self.subTest(change=change), self.assertRaises(ValueError):
                fetch.validate_manifest(value)

    def test_url_and_redirect_rejection(self):
        for url in ("http://dl.polyhaven.org/x", "https://evil.invalid/x",
                    "https://dl.polyhaven.org.evil.invalid/x", "https://dl.polyhaven.org/x?y",
                    "https://dl.polyhaven.org/%2e%2e/x", "https://u@dl.polyhaven.org/x"):
            with self.subTest(url=url), self.assertRaises(ValueError):
                fetch.validate_url(url)
        with self.assertRaises(ValueError):
            fetch.NoRedirect().redirect_request(None, None, 302, "", {}, "https://other.invalid/x")

    def test_reparse_rejection_without_python312_path_api(self):
        path = MagicMock()
        path.lstat.return_value = SimpleNamespace(st_mode=0, st_file_attributes=0x400)
        with patch.object(fetch.stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400, create=True):
            self.assertTrue(fetch.is_reparse_path(path))
            with self.assertRaises(ValueError):
                fetch.contained(path, "source.fbx")
        path.lstat.return_value = SimpleNamespace(st_mode=0, st_file_attributes=0)
        self.assertFalse(fetch.is_reparse_path(path))

    def test_file_admission(self):
        fetch.validate_file("shrub_04", self.item())
        for key, value in (("bytes", 0), ("bytes", True), ("bytes", 129 * 1024**2),
                           ("publisherMd5", "invalid"), ("name", "shrub_04_1k.blend"),
                           ("url", self.item()["url"].replace("/1k/", "/8k/")),
                           ("url", self.item()["url"].replace("/shrub_04/", "/fir_sapling/"))):
            item = copy.deepcopy(self.item())
            item[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                fetch.validate_file("shrub_04", item)
        with self.assertRaises(ValueError):
            fetch.validate_file("dead_tree_trunk", self.item())

    def test_hash_checks_preserve_failed_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data"
            path.write_bytes(b"abc")
            self.assertEqual(fetch.verify(path, self.item()), hashlib.sha256(b"abc").hexdigest().upper())
            for data in (b"abcd", b"xyz"):
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    fetch.verify(path, self.item())
                self.assertEqual(path.read_bytes(), data)

    def test_publisher_omitted_leading_zero(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data"
            path.write_bytes(b"a")
            item = self.item() | {"bytes": 1, "publisherMd5": hashlib.md5(b"a").hexdigest().lstrip("0")}
            self.assertEqual(fetch.verify(path, item), hashlib.sha256(b"a").hexdigest().upper())

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "receipt.json"
            fetch.write_new(path, {"original": True})
            before = path.read_bytes()
            with self.assertRaises(FileExistsError):
                fetch.write_new(path, {"replacement": True})
            with patch.object(fetch, "build_opener") as opener, self.assertRaises(FileExistsError):
                fetch.fetch(self.item(), path, lambda: None)
            opener.assert_not_called()
            self.assertEqual(path.read_bytes(), before)

    def test_partial_is_retained(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data.png"
            response = MagicMock()
            response.__enter__.return_value.headers = {"Content-Length": "4"}
            opener = MagicMock()
            opener.open.return_value = response
            with patch.object(fetch, "build_opener", return_value=opener):
                with self.assertRaises(ValueError):
                    fetch.fetch(self.item(), path, lambda: None)
                self.assertTrue(path.with_suffix(".png.download").exists())
                self.assertFalse(path.exists())
                opener.reset_mock()
                with self.assertRaises(FileExistsError):
                    fetch.fetch(self.item(), path, lambda: None)
                opener.open.assert_not_called()

    def test_run_policy_and_stop(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.json"
            state = {"id": "current", "state": "running", "completionPolicy": "until-complete",
                     "deadlineUtc": "2000-01-01T00:00:00Z"}
            path.write_text(json.dumps(state))
            fetch.run_guard(path, "current")
            with self.assertRaises(RuntimeError):
                fetch.run_guard(path, "wrong")
            for change in ({"state": "paused"}, {"state": "stopped"}, {"completionPolicy": "bounded"}):
                path.write_text(json.dumps(state | change))
                with self.assertRaises(RuntimeError):
                    fetch.run_guard(path, "current")
            path.write_text(json.dumps(state | {"completionPolicy": "unknown"}))
            with self.assertRaises(ValueError):
                fetch.run_guard(path, "current")

    def test_stop_checked_before_network(self):
        def stopped():
            raise RuntimeError("Stopped")
        with tempfile.TemporaryDirectory() as directory, patch.object(fetch, "build_opener") as opener:
            with self.assertRaises(RuntimeError):
                fetch.fetch(self.item(), Path(directory) / "data.png", stopped)
            opener.assert_not_called()

    def test_manifest_pin_checked_before_network(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest = root / "manifest.json"
            manifest.write_text("{}")
            with patch.object(fetch, "build_opener") as opener, self.assertRaises(ValueError):
                fetch.acquire(manifest, "0" * 64, root / "source", root / "receipt.json", lambda: None)
            opener.assert_not_called()


if __name__ == "__main__":
    unittest.main()
