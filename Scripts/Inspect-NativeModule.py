"""Read PE exports and matching MSF/PDB identity without loading native code."""
import argparse
import hashlib
import json
import pathlib
import struct
import uuid
import xml.etree.ElementTree as ET


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def require_manifest_inputs(actual, inputs):
    def parse(value):
        if not value or len(value) > 1024 * 1024 or b"<!DOCTYPE" in value.upper() or b"<!ENTITY" in value.upper():
            raise ValueError("Manifest outside safe inspection bounds")
        root = ET.fromstring(value)
        if root.tag != "{urn:schemas-microsoft-com:asm.v1}assembly":
            raise ValueError("Expected assembly manifest")
        return root

    def tag(element):
        return element.tag.replace("urn:schemas-microsoft-com:asm.v2", "urn:schemas-microsoft-com:asm.v3")

    def contains(have, need):
        if tag(have) != tag(need) or any(have.get(k) != v for k, v in need.attrib.items()):
            return False
        if (need.text or "").strip() and (have.text or "").strip() != need.text.strip():
            return False
        remaining = list(have)
        for child in need:
            found = next((i for i, candidate in enumerate(remaining) if contains(candidate, child)), None)
            if found is None:
                return False
            remaining.pop(found)
        return True

    final = parse(actual)
    if not inputs:
        raise ValueError("Manifest inputs required")
    for value in inputs:
        if not contains(final, parse(value)):
            raise ValueError("Embedded manifest omits or changes an input contract")


def inspect(dll, pdb, executable=False, manifest_inputs=()):
    if not 512 <= dll.stat().st_size <= (512 if executable else 64) * 1024 * 1024:
        raise ValueError("Image size outside inspection bounds")
    data = dll.read_bytes()
    pe = u32(data, 60)
    if data[:2] != b"MZ" or data[pe:pe + 6] != b"PE\0\0\x64\x86":
        raise ValueError("Not an AMD64 PE")
    count, = struct.unpack_from("<H", data, pe + 6)
    optional, flags = struct.unpack_from("<HH", data, pe + 20)
    if bool(flags & 0x2000) == executable or not flags & 0x2 or data[pe + 24:pe + 26] != b"\x0b\x02":
        raise ValueError("Unexpected PE32+ image kind")
    if not 1 <= count <= 96 or optional < 168:
        raise ValueError("Invalid PE sections/optional header")
    sections = []
    for i in range(count):
        at = pe + 24 + optional + 40 * i
        size, rva, raw_size, raw = struct.unpack_from("<IIII", data, at + 8)
        if raw + raw_size > len(data):
            raise ValueError("Section outside file")
        sections.append((rva, raw_size, raw))

    def offset(rva, size=1):
        for start, length, raw in sections:
            if start <= rva and rva + size <= start + length:
                return raw + rva - start
        raise ValueError("RVA outside file-backed sections")

    def text(at):
        end = data.find(b"\0", at, min(at + 4096, len(data)))
        if end < 0:
            raise ValueError("Unterminated PE string")
        return data[at:end].decode("ascii")

    names = []
    module_name = dll.name
    if not executable:
        export = offset(u32(data, pe + 24 + 112), 40)
        names_count = u32(data, export + 24)
        if not 1 <= names_count <= 100000:
            raise ValueError("Invalid export count")
        names_at = offset(u32(data, export + 32), names_count * 4)
        names = [text(offset(u32(data, names_at + i * 4))) for i in range(names_count)]
        if "ThisIsAnUnrealEngineModule" not in names:
            raise ValueError("Missing Unreal module marker")
        module_name = text(offset(u32(data, export + 12)))
    else:
        offset(u32(data, pe + 24 + 16))
    manifest = None
    if manifest_inputs:
        if not executable:
            raise ValueError("This manifest contract requires an executable")
        resource_rva, resource_size = struct.unpack_from("<II", data, pe + 24 + 112 + 2 * 8)
        resource = offset(resource_rva, resource_size)

        def entries(relative):
            if relative < 0 or relative + 16 > resource_size:
                raise ValueError("Invalid resource directory")
            named, ids = struct.unpack_from("<HH", data, resource + relative + 12)
            count = named + ids
            if not 1 <= count <= 4096 or relative + 16 + count * 8 > resource_size:
                raise ValueError("Invalid resource entry count")
            return [struct.unpack_from("<II", data, resource + relative + 16 + i * 8) for i in range(count)]

        def directory(parent, wanted):
            matches = [child for name, child in entries(parent) if name == wanted]
            if len(matches) != 1 or not matches[0] & 0x80000000:
                raise ValueError("Missing unique manifest resource directory")
            return matches[0] & 0x7fffffff

        languages = entries(directory(directory(0, 24), 1))
        if len(languages) != 1 or languages[0][1] & 0x80000000:
            raise ValueError("Expected one RT_MANIFEST1 language")
        leaf = languages[0][1]
        if leaf + 16 > resource_size:
            raise ValueError("Invalid manifest data entry")
        rva, size = struct.unpack_from("<II", data, resource + leaf)
        if not 1 <= size <= 1024 * 1024:
            raise ValueError("Invalid embedded manifest size")
        at = offset(rva, size)
        content = data[at:at + size]
        require_manifest_inputs(content, [path.read_bytes() for path in manifest_inputs])
        manifest = {"resourceType": 24, "resourceId": 1, "language": languages[0][0],
                    "bytes": size, "sha256": hashlib.sha256(content).hexdigest().upper(),
                    "verifiedInputs": [str(path) for path in manifest_inputs]}
    debug_rva, debug_size = struct.unpack_from("<II", data, pe + 24 + 112 + 6 * 8)
    if debug_size % 28 or not 28 <= debug_size <= 4096:
        raise ValueError("Invalid PE debug directory")
    debug = offset(debug_rva, debug_size)
    records = []
    for at in range(debug, debug + debug_size, 28):
        if u32(data, at + 12) != 2:
            continue
        size, _, raw = struct.unpack_from("<III", data, at + 16)
        if size < 25 or raw + size > len(data) or data[raw:raw + 4] != b"RSDS":
            raise ValueError("Invalid CodeView record")
        records.append((data[raw + 4:raw + 20], u32(data, raw + 20), text(raw + 24)))
    if len(records) != 1:
        raise ValueError("Expected one RSDS identity")
    with pdb.open("rb") as stream:
        length = pdb.stat().st_size

        def read(at, size):
            if at < 0 or size < 0 or at + size > length or size > 16 * 1024 * 1024:
                raise ValueError("PDB read outside bounds")
            stream.seek(at)
            result = stream.read(size)
            if len(result) != size:
                raise ValueError("Truncated PDB")
            return result

        header = read(0, 56)
        if header[:32] != b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0":
            raise ValueError("Invalid MSF signature")
        block, _, blocks, directory_size, _, block_map = struct.unpack_from("<6I", header, 32)
        if block not in (512, 1024, 2048, 4096) or blocks * block != length:
            raise ValueError("Invalid MSF block layout")
        directory_blocks = (directory_size + block - 1) // block
        if not 12 <= directory_size <= 16 * 1024 * 1024 or directory_blocks * 4 > block:
            raise ValueError("MSF directory exceeds inspection bounds")
        indices = read(block_map * block, directory_blocks * 4)
        directory = b"".join(read(u32(indices, i * 4) * block, block)
                             for i in range(directory_blocks))[:directory_size]
        streams = u32(directory, 0)
        if not 2 <= streams <= 100000 or 4 + streams * 4 > len(directory):
            raise ValueError("Invalid MSF stream directory")
        size0, size1 = struct.unpack_from("<II", directory, 4)
        skip0 = 0 if size0 == 0xFFFFFFFF else (size0 + block - 1) // block
        if not 28 <= size1 <= 16 * 1024 * 1024:
            raise ValueError("Invalid PDB identity stream")
        index1 = 4 + streams * 4 + skip0 * 4
        identity = read(u32(directory, index1) * block, 28)
        age, = struct.unpack_from("<I", identity, 8)
        guid = identity[12:28]
    if (guid, age) != records[0][:2] or pathlib.PureWindowsPath(records[0][2]).name != pdb.name:
        raise ValueError("DLL/PDB identity mismatch")

    def receipt(path):
        with path.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest().upper()
        return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest}

    return {"exe" if executable else "dll": receipt(dll), "pdb": receipt(pdb), "machine": "AMD64", "isDll": not executable,
            "moduleName": module_name, "manifest": manifest,
            "moduleMarker": None if executable else "ThisIsAnUnrealEngineModule", "exportCount": len(names),
            "probeExports": [name for name in names if "HomesteadAuthoringProbeCommandlet" in name],
            "debugGuid": str(uuid.UUID(bytes_le=guid)), "debugAge": age,
            "recordedPdb": records[0][2], "codeLoaded": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("dll", type=pathlib.Path)
    parser.add_argument("pdb", type=pathlib.Path)
    parser.add_argument("--executable", action="store_true")
    parser.add_argument("--manifest-input", type=pathlib.Path, action="append", default=[])
    args = parser.parse_args()
    print(json.dumps(inspect(args.dll, args.pdb, args.executable, args.manifest_input), indent=2))
