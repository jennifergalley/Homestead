import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Scripts"))
import inspect_woodland_sources as inspect


def scalar(value):
    if isinstance(value, str):
        data = value.encode("utf-8")
        return b"S" + struct.pack("<I", len(data)) + data
    if isinstance(value, float):
        return b"D" + struct.pack("<d", value)
    return b"L" + struct.pack("<q", value)


def numeric(kind, values, compressed=True, count=None):
    code = {b"d": "d", b"i": "i"}[kind]
    raw = struct.pack("<" + code * len(values), *values)
    stored = zlib.compress(raw) if compressed else raw
    return kind + struct.pack("<III", len(values) if count is None else count, int(compressed), len(stored)) + stored


def node(name, props=(), children=()):
    return name, props, children


def encode(item, offset, version):
    name, props, children = item
    fmt = "<QQQB" if version >= 7500 else "<IIIB"
    header_size = struct.calcsize(fmt)
    payload = b"".join(p if isinstance(p, bytes) else scalar(p) for p in props)
    position = offset + header_size + len(name) + len(payload)
    content = b""
    for c in children:
        encoded = encode(c, position + len(content), version)
        content += encoded
    if children:
        content += b"\0" * header_size
    return struct.pack(fmt, position + len(content), len(props), len(payload), len(name)) + name.encode() + payload + content


def fbx(nodes, version=7400):
    data = inspect.MAGIC + struct.pack("<I", version)
    for n in nodes:
        data += encode(n, len(data), version)
    return data + b"\0" * (25 if version >= 7500 else 13)


def fixture(version=7400):
    geo = node("Geometry", (1, "Quad\0\1Geometry", "Mesh"), (
        node("Vertices", (numeric(b"d", [0, 0, 0, 2, 0, 0, 2, 3, 0, 0, 3, 0]),)),
        node("PolygonVertexIndex", (numeric(b"i", [0, 1, 2, -4]),)),
        node("LayerElementMaterial", (0,), (
            node("MappingInformationType", ("AllSame",)),
            node("ReferenceInformationType", ("IndexToDirect",)),
            node("Materials", (numeric(b"i", [0], False),)),
        )),
    ))
    props = node("Properties70", (), (node("P", ("Lcl Translation", "Lcl Translation", "", "A", 1., 2., 3.)),))
    return fbx((
        node("GlobalSettings", (), (node("Properties70", (), (node("P", ("UnitScaleFactor", "double", "Number", "", 100.)),)),)),
        node("Objects", (), (
            geo, node("Model", (2, "InstanceA\0\1Model", "Mesh"), (props,)),
            node("Model", (3, "InstanceB\0\1Model", "Mesh"), (props,)),
            node("Material", (4, "Leaf\0\1Material", "")),
            node("Texture", (5, "Tex", ""), (node("FileName", (r"\\never-contact\share\evil.py",)),)),
        )),
        node("Connections", (), tuple(node("C", ("OO", a, b)) for a, b in ((1, 2), (1, 3), (4, 2), (4, 3), (2, 0), (3, 2)))),
    ), version)


class InspectionTests(unittest.TestCase):
    def parse(self, data):
        return inspect.FbxReader(io.BytesIO(data), len(data)).parse()

    def test_known_geometry_instances_materials_transforms_units_and_no_external_read(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "quad.fbx"
            for version in (7400, 7500):
                with self.subTest(version=version):
                    path.write_bytes(fixture(version))
                    original = Path.open
                    opened = []
                    def only_source(p, *args, **kwargs):
                        opened.append(p)
                        self.assertEqual(p, path)
                        return original(p, *args, **kwargs)
                    with patch.object(Path, "open", only_source):
                        result = inspect.inspect_fbx(path)
                    self.assertEqual(opened, [path])
                    self.assertEqual(result["uniqueGeometryFaces"], 1)
                    self.assertEqual(result["uniqueGeometryFanTriangles"], 2)
                    self.assertEqual(result["modelBoundFanTriangles"], 4)
                    self.assertEqual(result["geometryModelBindings"], 2)
                    self.assertEqual(result["globalSettings"]["UnitScaleFactor"], [100.])
                    self.assertEqual(result["geometries"][0]["localBounds"]["max"], [2., 3., 0.])
                    self.assertEqual(result["models"][0]["materialSlotsInConnectionOrder"], ["Leaf"])
                    self.assertEqual(result["models"][0]["properties"]["Lcl Translation"], [1., 2., 3.])
                    self.assertFalse(result["externalTextureReferencesNotFollowed"][0]["followed"])

    def test_wrong_header_version_and_truncation(self):
        for data in (b"wrong header" * 3, fbx([], 7600), fixture()[:-10]):
            with self.subTest(size=len(data)), self.assertRaises(ValueError):
                self.parse(data)

    def test_bad_end_offset(self):
        data = bytearray(fixture())
        struct.pack_into("<I", data, 27, len(data) + 1)
        with self.assertRaises(ValueError):
            self.parse(bytes(data))

    def test_property_length_and_count(self):
        for offset, value in ((31, 5000), (35, 999999)):
            data = bytearray(fixture())
            struct.pack_into("<I", data, offset, value)
            with self.assertRaises(ValueError):
                self.parse(bytes(data))

    def test_array_bomb_wrong_encoding_and_raw_length(self):
        arrays = [
            numeric(b"d", [0.] * 100, count=1),
            b"d" + struct.pack("<III", 50000000, 1, 1) + b"x",
            b"i" + struct.pack("<III", 1, 2, 4) + b"\0" * 4,
            b"i" + struct.pack("<III", 2, 0, 4) + b"\0" * 4,
        ]
        for value in arrays:
            with self.subTest(value=value[:13]), self.assertRaises(ValueError):
                self.parse(fbx((node("Vertices", (value,)),)))

    def test_depth_limit(self):
        item = node("leaf")
        for _ in range(66):
            item = node("nested", (), (item,))
        with self.assertRaises(ValueError):
            self.parse(fbx((item,)))

    def test_geometry_index_and_finite_checks(self):
        for points, indices in (([0., 0., 0.], [0, 0, -4]),
                                ([0., 0., 0.], [0, 0, 0]),
                                ([float("nan"), 0., 0.], [0, 0, -1])):
            nodes = self.parse(fbx((node("Geometry", (1, "Bad", "Mesh"), (
                node("Vertices", (numeric(b"d", points),)),
                node("PolygonVertexIndex", (numeric(b"i", indices),)),
            )),)))
            with self.assertRaises(ValueError):
                inspect.geometry(nodes[0])

    def test_png_alpha_and_invalid_header(self):
        from PIL import Image
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "alpha.png"
            Image.new("RGBA", (2, 3), (1, 2, 3, 4)).save(path)
            result = inspect.inspect_image(path)
            self.assertEqual(result["size"], [2, 3])
            self.assertTrue(result["hasEmbeddedAlpha"])
            self.assertEqual(result["pngHeader"]["bitDepth"], 8)
            data = bytearray(path.read_bytes())
            data[20] ^= 1
            path.write_bytes(data)
            with self.assertRaises(ValueError):
                inspect.inspect_image(path)

    def test_mislabeled_image(self):
        from PIL import Image
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not_jpeg.jpg"
            Image.new("RGB", (1, 1)).save(path, format="PNG")
            with self.assertRaises(ValueError):
                inspect.inspect_image(path)


if __name__ == "__main__":
    unittest.main()
