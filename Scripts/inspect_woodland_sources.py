"""Read receipted FBX/image data only. No scene import, script or texture-path execution."""
import argparse
import array
from collections import Counter, defaultdict
import json
import math
from pathlib import Path
import struct
import sys
import time
import zlib

from prepare_woodland_sources import RECORDS, SOURCE, contained, digest, leaf_name, run_guard, write_new

MAGIC = b"Kaydara FBX Binary  \x00\x1a\x00"
ARRAY_TYPES = {b"f": ("f", 4), b"d": ("d", 8), b"i": ("i", 4),
               b"l": ("q", 8), b"b": ("b", 1), b"c": ("B", 1)}
SCALARS = {b"Y": "<h", b"C": "<?", b"I": "<i", b"F": "<f", b"D": "<d", b"L": "<q"}
KEEP_ARRAYS = {b"Vertices", b"PolygonVertexIndex", b"Materials"}


class FbxReader:
    def __init__(self, stream, size):
        if not 27 <= size <= 128 * 1024**2:
            raise ValueError("FBX size outside bounded reader policy.")
        self.stream, self.size = stream, size
        self.nodes = self.decoded_bytes = 0
        self.started = time.monotonic()
        if self.read(23, size) != MAGIC:
            raise ValueError("Only binary FBX is admitted; header invalid.")
        self.version = self.unpack("<I", size)[0]
        if self.version not in (7400, 7500):
            raise ValueError("Unreviewed FBX version: " + str(self.version))
        self.header = "<QQQB" if self.version >= 7500 else "<IIIB"
        self.header_size = struct.calcsize(self.header)

    def read(self, length, limit):
        if length < 0 or self.stream.tell() + length > min(limit, self.size):
            raise ValueError("FBX read crosses declared bounds.")
        data = self.stream.read(length)
        if len(data) != length:
            raise ValueError("Truncated FBX.")
        return data

    def unpack(self, fmt, limit):
        return struct.unpack(fmt, self.read(struct.calcsize(fmt), limit))

    def property(self, name, limit):
        kind = self.read(1, limit)
        if kind in SCALARS:
            result = self.unpack(SCALARS[kind], limit)[0]
            if isinstance(result, float) and not math.isfinite(result):
                raise ValueError("Non-finite scalar.")
            return result
        if kind in (b"S", b"R"):
            length = self.unpack("<I", limit)[0]
            if kind == b"R":
                if self.stream.tell() + length > limit:
                    raise ValueError("Raw property crosses bounds.")
                self.stream.seek(length, 1)
                return {"rawBytesNotInterpreted": length}
            if length > 1024**2:
                raise ValueError("FBX string too large.")
            return self.read(length, limit).decode("utf-8")
        if kind not in ARRAY_TYPES:
            raise ValueError("Unsupported FBX property type: " + repr(kind))
        count, encoding, stored = self.unpack("<III", limit)
        code, width = ARRAY_TYPES[kind]
        expected = count * width
        if expected > 128 * 1024**2 or encoding not in (0, 1):
            raise ValueError("Array expansion/type exceeds policy.")
        if self.stream.tell() + stored > limit or (encoding == 0 and stored != expected):
            raise ValueError("Array length exceeds bounds or disagrees with count.")
        if name not in KEEP_ARRAYS:
            self.stream.seek(stored, 1)
            return {"arrayCount": count, "type": code, "decoded": False}
        self.decoded_bytes += expected
        if self.decoded_bytes > 256 * 1024**2:
            raise ValueError("Total decoded-array budget exceeded.")
        raw = self.read(stored, limit)
        if encoding == 1:
            decoder = zlib.decompressobj()
            raw = decoder.decompress(raw, expected + 1)
            if not decoder.eof or decoder.unconsumed_tail or decoder.unused_data:
                raise ValueError("Invalid or oversized compressed array.")
        if len(raw) != expected:
            raise ValueError("Decoded array size differs from declaration.")
        values = array.array(code)
        values.frombytes(raw)
        if sys.byteorder != "little":
            values.byteswap()
        return values

    def node(self, limit, depth=0):
        if depth > 64 or self.nodes >= 100000 or time.monotonic() - self.started > 120:
            raise ValueError("FBX depth/node/time budget exceeded.")
        end, count, prop_bytes, name_size = self.unpack(self.header, limit)
        if end == 0:
            if count or prop_bytes or name_size:
                raise ValueError("Invalid null node.")
            return None
        self.nodes += 1
        if end > limit or end < self.stream.tell() + name_size + prop_bytes or count > 4096:
            raise ValueError("Invalid node offsets/property count.")
        name = self.read(name_size, end)
        prop_end = self.stream.tell() + prop_bytes
        props = [self.property(name, prop_end) for _ in range(count)]
        if self.stream.tell() != prop_end:
            raise ValueError("Property-list length mismatch.")
        children = []
        while self.stream.tell() < end:
            child = self.node(end, depth + 1)
            if child is None:
                if self.stream.tell() != end:
                    raise ValueError("Null child before declared node end.")
                break
            children.append(child)
        return {"name": name.decode("ascii"), "props": props, "children": children}

    def parse(self):
        result = []
        while self.stream.tell() < self.size:
            item = self.node(self.size)
            if item is None:
                break
            result.append(item)
        return result


def child(node, name):
    matches = [c for c in node["children"] if c["name"] == name]
    if len(matches) != 1:
        raise ValueError(f"Expected one {name}, found {len(matches)}.")
    return matches[0]


def properties(node):
    groups = [c for c in node["children"] if c["name"] == "Properties70"]
    return {p["props"][0]: p["props"][4:] for g in groups for p in g["children"] if p["name"] == "P"}


def geometry(node):
    vertices = child(node, "Vertices")["props"][0]
    indices = child(node, "PolygonVertexIndex")["props"][0]
    if not isinstance(vertices, array.array) or vertices.typecode not in ("d", "f") or len(vertices) % 3 or not vertices:
        raise ValueError("Invalid vertex array.")
    if not isinstance(indices, array.array) or indices.typecode != "i" or not indices:
        raise ValueError("Invalid polygon-index array.")
    if not all(math.isfinite(v) for v in vertices):
        raise ValueError("Non-finite vertex.")
    count = len(vertices) // 3
    histogram = Counter()
    corners = 0
    for index in indices:
        vertex = -index - 1 if index < 0 else index
        if vertex >= count:
            raise ValueError("Polygon index outside vertex array.")
        corners += 1
        if index < 0:
            if corners < 3:
                raise ValueError("Degenerate source polygon.")
            histogram[corners] += 1
            corners = 0
    if corners:
        raise ValueError("Unterminated source polygon.")
    material_layers = []
    for layer in node["children"]:
        if layer["name"] == "LayerElementMaterial":
            material_layers.append({
                "mapping": child(layer, "MappingInformationType")["props"],
                "reference": child(layer, "ReferenceInformationType")["props"],
                "indexCounts": dict(Counter(child(layer, "Materials")["props"][0])),
            })
    return {
        "id": node["props"][0], "name": node["props"][1].split("\x00")[0],
        "vertices": count, "faces": sum(histogram.values()), "faceVertexHistogram": dict(histogram),
        "fanTriangleEstimate": sum((n - 2) * c for n, c in histogram.items()),
        "localBounds": {"min": [min(vertices[a::3]) for a in range(3)],
                        "max": [max(vertices[a::3]) for a in range(3)]},
        "materialLayers": material_layers,
        "layerTypes": [c["name"] for c in node["children"] if c["name"].startswith("LayerElement")],
    }


def inspect_fbx(path):
    with path.open("rb") as stream:
        reader = FbxReader(stream, path.stat().st_size)
        nodes = reader.parse()
    root = {"children": nodes}
    objects = child(root, "Objects")["children"]
    identifiers = [n["props"][0] for n in objects]
    if len(identifiers) != len(set(identifiers)):
        raise ValueError("Duplicate source object identity.")
    connections = [c["props"] for c in child(root, "Connections")["children"] if c["name"] == "C"]
    attached_to, parents_of = defaultdict(list), defaultdict(list)
    for connection in connections:
        if len(connection) < 3:
            raise ValueError("Invalid source connection.")
        if connection[0] == "OO":
            attached_to[connection[2]].append(connection[1])
            parents_of[connection[1]].append(connection[2])
    meshes = [geometry(n) for n in objects if n["name"] == "Geometry" and n["props"][2] == "Mesh"]
    models = [n for n in objects if n["name"] == "Model"]
    materials = {n["props"][0]: n["props"][1].split("\x00")[0] for n in objects if n["name"] == "Material"}
    mesh_ids = {m["id"] for m in meshes}
    model_ids = {m["props"][0] for m in models}
    hierarchy = []
    for model in models:
        if time.monotonic() - reader.started > 120:
            raise TimeoutError("Source hierarchy inspection exceeded time budget.")
        identifier = model["props"][0]
        attached = attached_to[identifier]
        hierarchy.append({
            "id": identifier, "name": model["props"][1].split("\x00")[0],
            "type": model["props"][2],
            "parentIds": [i for i in parents_of[identifier] if i in model_ids or i == 0],
            "geometryIds": [i for i in attached if i in mesh_ids],
            "materialSlotsInConnectionOrder": [materials[i] for i in attached if i in materials],
            "properties": properties(model),
        })
    references, indicators = [], []

    def visit(node, parent=""):
        location = parent + "/" + node["name"]
        if node["name"].lower() in ("filename", "relativefilename"):
            references.append({"field": location, "values": node["props"], "followed": False})
        if "lod" in node["name"].lower() or any(isinstance(p, str) and ("lod" in p.lower() or "wind" in p.lower()) for p in node["props"]):
            indicators.append({"field": location, "strings": [p for p in node["props"] if isinstance(p, str)]})
        for c in node["children"]:
            visit(c, location)
    for node in nodes:
        visit(node)
    mesh_by_id = {m["id"]: m for m in meshes}
    instance_ids = [i for m in hierarchy for i in m["geometryIds"]]
    return {
        "fbxVersion": reader.version, "parsedNodes": reader.nodes,
        "decodedArrayBytes": reader.decoded_bytes,
        "globalSettings": properties(child(root, "GlobalSettings")),
        "objectTypes": dict(Counter(n["name"] for n in objects)),
        "geometries": meshes, "models": hierarchy, "materials": materials,
        "uniqueGeometryFaces": sum(m["faces"] for m in meshes),
        "uniqueGeometryFanTriangles": sum(m["fanTriangleEstimate"] for m in meshes),
        "geometryModelBindings": len(instance_ids),
        "modelBoundFanTriangles": sum(mesh_by_id[i]["fanTriangleEstimate"] for i in instance_ids),
        "lodWindNameIndicators": indicators,
        "externalTextureReferencesNotFollowed": references,
        "runtimeEvidence": "Unmeasured; source polygons/fan estimates and raw local transforms only. No evaluated scene, imported LOD, wind or render proof.",
    }


def inspect_image(path):
    from PIL import Image
    Image.MAX_IMAGE_PIXELS = 16 * 1024**2
    with path.open("rb") as stream:
        header = stream.read(33)
    png = None
    if path.suffix == ".png":
        if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR" or len(header) != 33:
            raise ValueError("Invalid PNG header.")
        width, height, bits, color, compression, filtering, interlace = struct.unpack(">IIBBBBB", header[16:29])
        if zlib.crc32(header[12:29]) & 0xffffffff != struct.unpack(">I", header[29:33])[0]:
            raise ValueError("PNG header CRC mismatch.")
        png = {"width": width, "height": height, "bitDepth": bits, "colorType": color,
               "compression": compression, "filter": filtering, "interlace": interlace}
    with Image.open(path) as image:
        expected_format = {".png": "PNG", ".jpg": "JPEG"}.get(path.suffix)
        if image.format != expected_format or image.width * image.height > Image.MAX_IMAGE_PIXELS:
            raise ValueError("Unsupported or oversized source image.")
        image.verify()
    with Image.open(path) as image:
        image.load()
        return {"format": image.format, "size": list(image.size), "decodedMode": image.mode,
                "decodedBands": list(image.getbands()), "decodedExtrema": image.getextrema(),
                "hasEmbeddedAlpha": "A" in image.getbands() or "transparency" in image.info,
                "pngHeader": png,
                "note": "Pillow decoded extrema may be downconverted for multichannel16-bit PNG; IHDR bit depth is authoritative. Separate alpha-map values are data, not an embedded alpha channel."}


def inspect():
    run_guard()
    receipt = json.loads((RECORDS / "download-receipt.json").read_text(encoding="utf-8"))
    if receipt["manifestSha256"] != digest(RECORDS / "asset-manifest.json", "sha256"):
        raise ValueError("Manifest differs from acquired receipt.")
    output = {"scope": "SOURCE ONLY; task1.3 blocked; no Blender/UE/external lookups", "files": []}
    for entry in receipt["files"]:
        run_guard()
        path = contained(SOURCE, leaf_name(entry["asset"]), leaf_name(entry["file"]))
        if path.stat().st_size != entry["bytes"] or digest(path, "sha256") != entry["sha256"]:
            raise ValueError("Source no longer matches sealed receipt: " + str(path))
        result = inspect_fbx(path) if path.suffix == ".fbx" else inspect_image(path)
        output["files"].append({"asset": entry["asset"], "file": entry["file"],
                                "sha256": entry["sha256"], "sourceInspection": result})
        print("Inspected", entry["file"], flush=True)
    inventory = RECORDS / "source-inventory.json"
    if inventory.exists():
        normalized = json.loads(json.dumps(output, allow_nan=False))
        if json.loads(inventory.read_text(encoding="utf-8")) != normalized:
            raise ValueError("Inspection differs from preserved inventory; review, do not overwrite.")
        print("Existing inventory exactly reproduced; not rewritten.")
    else:
        write_new(inventory, output)


if __name__ == "__main__":
    argparse.ArgumentParser(description=__doc__).parse_args()
    inspect()
