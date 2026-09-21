import importlib.util
import pathlib
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "native_module", pathlib.Path(__file__).resolve().parents[1] / "Scripts" / "Inspect-NativeModule.py")
native = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native)


class ModuleEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.dll = pathlib.Path(self.temp.name) / "fixture.dll"
        self.pdb = pathlib.Path(self.temp.name) / "fixture.pdb"
        pe = bytearray(1024)
        pe[:2] = b"MZ"
        struct.pack_into("<I", pe, 60, 128)
        pe[128:134] = b"PE\0\0\x64\x86"
        struct.pack_into("<H", pe, 134, 1)
        struct.pack_into("<HHH", pe, 148, 240, 0x2000, 0x20B)
        struct.pack_into("<II", pe, 264, 0x1000, 40)
        struct.pack_into("<II", pe, 312, 0x1100, 28)
        struct.pack_into("<IIII", pe, 400, 512, 0x1000, 512, 512)
        struct.pack_into("<I", pe, 524, 0x1080)
        struct.pack_into("<I", pe, 536, 1)
        struct.pack_into("<I", pe, 544, 0x10A0)
        pe[640:652] = b"fixture.dll\0"
        struct.pack_into("<I", pe, 672, 0x10B0)
        marker = b"ThisIsAnUnrealEngineModule\0"
        pe[688:688 + len(marker)] = marker
        record = b"RSDS" + bytes(range(16)) + struct.pack("<I", 1) + b"fixture.pdb\0"
        struct.pack_into("<IIII", pe, 780, 2, len(record), 0x1140, 832)
        pe[832:832 + len(record)] = record
        self.dll.write_bytes(pe)
        pdb = bytearray(2048)
        pdb[:32] = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0"
        struct.pack_into("<6I", pdb, 32, 512, 1, 4, 16, 0, 1)
        struct.pack_into("<I", pdb, 512, 2)
        struct.pack_into("<4I", pdb, 1024, 2, 0, 28, 3)
        struct.pack_into("<3I", pdb, 1536, 20000404, 1, 1)
        pdb[1548:1564] = bytes(range(16))
        self.pdb.write_bytes(pdb)

    def test_matching_identity(self):
        result = native.inspect(self.dll, self.pdb)
        self.assertEqual(result["moduleName"], "fixture.dll")
        self.assertEqual(result["debugAge"], 1)
        self.assertFalse(result["codeLoaded"])

    def test_rejections(self):
        cases = [(self.dll, 132, b"\0\0"), (self.dll, 688, b"X"),
                 (self.dll, 264, b"\xff" * 4), (self.dll, 784, b"\xff" * 4),
                 (self.pdb, 1548, b"\xff"), (self.pdb, 0, b"X"),
                 (self.pdb, 52, b"\xff" * 4), (self.pdb, 1032, b"\xff" * 4)]
        for path, at, replacement in cases:
            with self.subTest(path=path.name, at=at):
                original = path.read_bytes()
                changed = bytearray(original)
                changed[at:at + len(replacement)] = replacement
                path.write_bytes(changed)
                with self.assertRaises((ValueError, struct.error)):
                    native.inspect(self.dll, self.pdb)
                path.write_bytes(original)


if __name__ == "__main__":
    unittest.main()
