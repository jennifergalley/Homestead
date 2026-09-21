"""Read PE exports and matching MSF/PDB identity without loading native code."""
import argparse
import hashlib
import json
import pathlib
import struct
import uuid


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def inspect(dll, pdb):
    if not 512 <= dll.stat().st_size <= 64 * 1024 * 1024:
        raise ValueError("DLL size outside inspection bounds")
    data = dll.read_bytes()
    pe = u32(data, 60)
    if data[:2] != b"MZ" or data[pe:pe + 6] != b"PE\0\0\x64\x86":
        raise ValueError("Not an AMD64 PE")
    count, = struct.unpack_from("<H", data, pe + 6)
    optional, flags = struct.unpack_from("<HH", data, pe + 20)
    if not flags & 0x2000 or data[pe + 24:pe + 26] != b"\x0b\x02":
        raise ValueError("Not a PE32+ DLL")
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

    export = offset(u32(data, pe + 24 + 112), 40)
    names_count = u32(data, export + 24)
    if not 1 <= names_count <= 100000:
        raise ValueError("Invalid export count")
    names_at = offset(u32(data, export + 32), names_count * 4)
    names = [text(offset(u32(data, names_at + i * 4))) for i in range(names_count)]
    if "ThisIsAnUnrealEngineModule" not in names:
        raise ValueError("Missing Unreal module marker")
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

    return {"dll": receipt(dll), "pdb": receipt(pdb), "machine": "AMD64", "isDll": True,
            "moduleName": text(offset(u32(data, export + 12))),
            "moduleMarker": "ThisIsAnUnrealEngineModule", "exportCount": names_count,
            "probeExports": [name for name in names if "HomesteadAuthoringProbeCommandlet" in name],
            "debugGuid": str(uuid.UUID(bytes_le=guid)), "debugAge": age,
            "recordedPdb": records[0][2], "codeLoaded": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("dll", type=pathlib.Path)
    parser.add_argument("pdb", type=pathlib.Path)
    args = parser.parse_args()
    print(json.dumps(inspect(args.dll, args.pdb), indent=2))
