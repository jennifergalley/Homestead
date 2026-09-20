import copy
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch, MagicMock

SCRIPT = Path(__file__).resolve().parents[1] / "Scripts" / "prepare_woodland_sources.py"
SPEC = importlib.util.spec_from_file_location("woodland_fetch", SCRIPT)
fetch = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fetch)


class AcquisitionTests(unittest.TestCase):
    def item(self):
        return {"name": "fern_02_diff_1k.png", "bytes": 3,
                "url": "https://dl.polyhaven.org/file/ph-assets/Models/png/1k/fern_02/fern_02_diff_1k.png",
                "publisherMd5": hashlib.md5(b"abc").hexdigest()}

    def test_admitted_file(self):
        fetch.validate_file("fern_02", self.item())

    def test_unsafe_names(self):
        for name in ("..", "../x.png", r"..\x.png", r"C:\x.png", r"\\host\x", "x:ads.png", "CON.png", "trailing."):
            with self.subTest(name=name), self.assertRaises(ValueError):
                fetch.leaf_name(name)

    def test_unapproved_urls(self):
        for url in ("http://dl.polyhaven.org/x", "https://evil.invalid/x",
                    "https://dl.polyhaven.org.evil.invalid/x", "https://dl.polyhaven.org/x?y",
                    "https://dl.polyhaven.org/%2e%2e/x", "https://u@dl.polyhaven.org/x"):
            with self.subTest(url=url), self.assertRaises(ValueError):
                fetch.validate_url(url, "dl.polyhaven.org")

    def test_redirect_rejected(self):
        with self.assertRaises(ValueError):
            fetch.NoRedirect().redirect_request(None, None, 302, "", {}, "https://other.invalid/x")

    def test_wrong_size_resolution_type_asset_checksum(self):
        for key, value in (("bytes", 0), ("bytes", True), ("bytes", 129 * 1024**2),
                           ("publisherMd5", "not-a-hash"), ("name", "fern_02_1k.blend"),
                           ("url", self.item()["url"].replace("/1k/", "/8k/")),
                           ("url", self.item()["url"].replace("/fern_02/", "/fir_sapling/"))):
            item = copy.deepcopy(self.item())
            item[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                fetch.validate_file("fern_02", item)
        with self.assertRaises(ValueError):
            fetch.validate_file("fir_sapling", self.item())

    def test_no_escape(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                fetch.contained(Path(directory), "..", "outside")

    def test_size_and_md5_mismatches_preserve_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data"
            path.write_bytes(b"abc")
            self.assertEqual(fetch.verify(path, self.item()), hashlib.sha256(b"abc").hexdigest().upper())
            for data in (b"abcd", b"xyz"):
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    fetch.verify(path, self.item())
                self.assertEqual(path.read_bytes(), data)

    def test_receipt_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "receipt.json"
            fetch.write_new(path, {"original": True})
            before = path.read_bytes()
            with self.assertRaises(FileExistsError):
                fetch.write_new(path, {"replacement": True})
            self.assertEqual(path.read_bytes(), before)

    def test_http_length_failure_retains_partial(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data.png"
            response = MagicMock()
            response.__enter__.return_value.headers = {"Content-Length": "4"}
            opener = MagicMock()
            opener.open.return_value = response
            with patch.object(fetch, "run_guard"), patch.object(fetch, "build_opener", return_value=opener):
                with self.assertRaises(ValueError):
                    fetch.fetch(self.item()["url"], path, 3, "dl.polyhaven.org", 3)
                self.assertTrue(path.with_suffix(".png.download").exists())
                self.assertFalse(path.exists())
                opener.reset_mock()
                with self.assertRaises(FileExistsError):
                    fetch.fetch(self.item()["url"], path, 3, "dl.polyhaven.org", 3)
                opener.open.assert_not_called()

    def test_existing_file_never_replaced_or_refetched(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "data.png"
            path.write_bytes(b"original")
            with patch.object(fetch, "run_guard"), patch.object(fetch, "build_opener") as opener:
                with self.assertRaises(FileExistsError):
                    fetch.fetch(self.item()["url"], path, 3, "dl.polyhaven.org", 3)
                opener.assert_not_called()
            self.assertEqual(path.read_bytes(), b"original")


if __name__ == "__main__":
    unittest.main()
