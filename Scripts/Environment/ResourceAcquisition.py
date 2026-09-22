"""Acquire exact admitted resource palettes through the bounded HTTPS recipe."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import stat
import time
from urllib.parse import urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener

ASSETS = ("shrub_04", "dry_branches_medium_01", "fir_sapling", "flower_empodium")
TREE_PALETTE = ("jacaranda_tree", "island_tree_02", "fir_sapling_medium")
MATURE_FIR = ("fir_tree_01",)
ROOT = Path(__file__).resolve().parents[2]
USER_AGENT = "HomesteadSourcePreparation/1.0 (Poly Haven asset acquisition)"


def digest(path, algorithm="sha256"):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, algorithm).hexdigest().upper()


def run_guard(control, expected_id):
    state = json.loads(control.read_text(encoding="utf-8-sig"))
    if state["id"] != expected_id or state["state"] != "running":
        raise RuntimeError("Run changed, paused or stopped; no further acquisition.")
    policy = state.get("completionPolicy", "bounded")
    if policy not in ("bounded", "until-complete"):
        raise ValueError("Unknown completion policy.")
    if policy == "bounded":
        deadline = datetime.fromisoformat(state["deadlineUtc"].replace("Z", "+00:00"))
        if deadline.tzinfo is None or datetime.now(timezone.utc) >= deadline:
            raise RuntimeError("Bounded run deadline reached or missing timezone.")


def leaf_name(name):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,180}", name) or ".." in name or name.endswith("."):
        raise ValueError("Unsafe source filename.")
    if name.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *("COM%d" % n for n in range(10)), *("LPT%d" % n for n in range(10))}:
        raise ValueError("Reserved source filename.")
    return name


def is_reparse_path(path):
    try:
        info = path.lstat()
    except FileNotFoundError:
        return False
    return stat.S_ISLNK(info.st_mode) or bool(
        getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))


def contained(base, *parts):
    if is_reparse_path(base):
        raise ValueError("Source root must not be a link or junction.")
    base = base.resolve()
    path = base.joinpath(*parts)
    if not path.resolve().is_relative_to(base):
        raise ValueError("Source path escapes its root.")
    current = base
    for part in parts:
        current /= part
        if is_reparse_path(current):
            raise ValueError("Source path contains a link or junction.")
    return path


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ValueError("Unexpected redirect; review explicitly: " + newurl)


def validate_url(url, host="dl.polyhaven.org"):
    parsed = urlsplit(url)
    if parsed.scheme != "https" or parsed.netloc != host or parsed.query or parsed.fragment:
        raise ValueError("Unexpected source URL.")
    if "%" in parsed.path or "\\" in parsed.path or ".." in parsed.path:
        raise ValueError("Ambiguous source URL.")
    return parsed


def validate_file(asset, item, allow_large_fir=False):
    if asset not in (*ASSETS, *TREE_PALETTE, *MATURE_FIR, "shrub_02"):
        raise ValueError("Asset outside the agreed source palettes.")
    name = leaf_name(item["name"])
    parsed = validate_url(item["url"])
    extension = Path(name).suffix
    expected = f"/file/ph-assets/Models/{extension[1:]}/1k/{asset}/{name}"
    allowed_extensions = (".fbx", ".png", ".jpg", ".blend") if allow_large_fir and asset == "fir_tree_01" else (".fbx", ".png", ".jpg")
    if extension not in allowed_extensions or parsed.path != expected:
        raise ValueError("Source format/path/resolution differs from admission.")
    if not name.endswith("_1k" + extension) or not name.startswith(asset + "_"):
        raise ValueError("Source basename differs from asset/resolution.")
    maximum = 256 * 1024**2 if allow_large_fir and asset == "fir_tree_01" else 128 * 1024**2
    if type(item["bytes"]) is not int or not 0 < item["bytes"] <= maximum:
        raise ValueError("Invalid or excessive source bytes.")
    if not re.fullmatch(r"[0-9a-fA-F]{1,32}", item["publisherMd5"]):
        raise ValueError("Invalid publisher MD5.")


def validate_manifest(manifest):
    probe = manifest.get("profile") == "midstory-shrub02-source-probe"
    tree_palette = manifest.get("profile") == "tree-palette-source-01"
    mature_fir = manifest.get("profile") == "mature-fir-source-01"
    mature_fir_blend = manifest.get("profile") == "mature-fir-uv-source-02"
    expected = ("shrub_02",) if probe else TREE_PALETTE if tree_palette else MATURE_FIR if mature_fir or mature_fir_blend else ASSETS
    if (manifest["version"] != 1 or tuple(a["id"] for a in manifest["assets"]) != expected
            or manifest.get("profile") not in (None, "midstory-shrub02-source-probe", "tree-palette-source-01",
                                                "mature-fir-source-01", "mature-fir-uv-source-02")):
        raise ValueError("Manifest palette/order/version differs from admission.")
    if probe:
        files = manifest["assets"][0]["files"]
        if (len(files) != 1 or files[0]["name"] != "shrub_02_1k.fbx"
                or files[0]["bytes"] != 832300
                or files[0]["publisherMd5"].lower() != "f4d00cbfa3dd3a6594e5b63c6324cb42"):
            raise ValueError("Shrub02 probe admits only the exact original FBX, not maps or other models.")
    if tree_palette:
        exact = {
            "jacaranda_tree": (14, "jacaranda_tree_1k.fbx", 132437628, "98f9827599dd42b18c1e9dfab3062d2f"),
            "island_tree_02": (14, "island_tree_02_1k.fbx", 32258924, "52f6b7264dc026f9496d5cb7d51f7c04"),
            "fir_sapling_medium": (10, "fir_sapling_medium_1k.fbx", 51634508, "82d67e5714eab0c493c8c9260bba8d28"),
        }
        for asset in manifest["assets"]:
            count, name, size, md5 = exact[asset["id"]]
            files = asset["files"]
            if (len(files) != count or files[0]["name"] != name or files[0]["bytes"] != size
                    or files[0]["publisherMd5"].lower() != md5):
                raise ValueError("Tree palette admits only the exact reviewed FBX and one-kilometer map set.")
    if mature_fir:
        files = manifest["assets"][0]["files"]
        if (len(files) != 14 or files[0]["name"] != "fir_tree_01_1k.fbx"
                or files[0]["bytes"] != 249300492
                or files[0]["publisherMd5"].lower() != "ab79788fc818ce7eadd40ccbfe987918"):
            raise ValueError("Mature-fir exception admits only the exact reviewed FBX/map closure.")
    if mature_fir_blend:
        files = manifest["assets"][0]["files"]
        if (len(files) != 1 or files[0]["name"] != "fir_tree_01_1k.blend"
                or files[0]["bytes"] != 218986261
                or files[0]["publisherMd5"].lower() != "a08031ea8ffb49711b294e1c8213a909"):
            raise ValueError("Mature-fir UV repair admits only the exact reviewed provider Blend source.")
    seen = set()
    total = 0
    for asset in manifest["assets"]:
        if asset["license"] != "CC0-1.0" or asset["resolution"] != "1k":
            raise ValueError("Unexpected source license/resolution.")
        for item in asset["files"]:
            validate_file(asset["id"], item, mature_fir or mature_fir_blend)
            key = (asset["id"], item["name"])
            if key in seen:
                raise ValueError("Duplicate source destination.")
            seen.add(key)
            total += item["bytes"]
    maximum_total = 384 * 1024**2 if mature_fir or mature_fir_blend else 512 * 1024**2
    if not 0 < total <= maximum_total:
        raise ValueError("Invalid total source size.")
    return total


def verify(path, item):
    if path.stat().st_size != item["bytes"]:
        raise ValueError("Source size mismatch: " + path.name)
    if digest(path, "md5") != item["publisherMd5"].zfill(32).upper():
        raise ValueError("Publisher MD5 mismatch: " + path.name)
    return digest(path)


def write_new(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=True, allow_nan=False)
        stream.write("\n")


def fetch(item, destination, guard):
    validate_url(item["url"])
    guard()
    temporary = destination.with_suffix(destination.suffix + ".download")
    if destination.exists() or temporary.exists():
        raise FileExistsError("Refusing to replace original or retained partial.")
    destination.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    request = Request(item["url"], headers={"User-Agent": USER_AGENT})
    with build_opener(NoRedirect).open(request, timeout=45) as response, temporary.open("xb") as output:
        length = response.headers.get("Content-Length")
        if length and int(length) != item["bytes"]:
            raise ValueError("Unexpected HTTP content length; partial retained.")
        count = 0
        while True:
            guard()
            if time.monotonic() - started > 300:
                raise TimeoutError("Individual source transfer stalled; partial retained.")
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            count += len(chunk)
            if count > item["bytes"]:
                raise ValueError("Transfer exceeds declared bytes; partial retained.")
            output.write(chunk)
    guard()
    verify(temporary, item)
    temporary.rename(destination)


def acquire(manifest_path, expected_sha, source_root, receipt_path, guard):
    if not re.fullmatch(r"[a-fA-F0-9]{64}", expected_sha) or digest(manifest_path) != expected_sha.upper():
        raise ValueError("Manifest hash differs from the reviewed input.")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    total = validate_manifest(manifest)
    guard()
    prior = json.loads(receipt_path.read_text()) if receipt_path.exists() else None
    if prior is not None and prior["manifestSha256"] != expected_sha.upper():
        raise ValueError("Manifest changed since sealed receipt.")
    entries = []
    for asset in manifest["assets"]:
        for item in asset["files"]:
            guard()
            path = contained(source_root, asset["id"], leaf_name(item["name"]))
            if not path.exists():
                if prior is not None:
                    raise FileNotFoundError("Receipted source missing; no silent reacquisition.")
                fetch(item, path, guard)
            sha = verify(path, item)
            entry = {
                "asset": asset["id"], "file": item["name"], "bytes": path.stat().st_size,
                "sha256": sha, "publisherMd5": item["publisherMd5"],
                "actualMd5": digest(path, "md5"), "source": item["url"], "license": asset["license"],
            }
            if prior is not None and entry not in prior["files"]:
                raise ValueError("Source differs from sealed receipt; no overwrite.")
            entries.append(entry)
            print(f"Verified {asset['id']} / {path.name}: {sha}", flush=True)
    guard()
    if prior is not None:
        if entries != prior["files"]:
            raise ValueError("Receipt entries differ.")
    else:
        write_new(receipt_path, {
            "version": 1, "sourceRoot": str(source_root.resolve()),
            "acquiredUtc": datetime.now(timezone.utc).isoformat(),
            "manifestSha256": expected_sha.upper(), "totalBytes": total, "files": entries,
        })
    print(f"Verified {len(entries)} original files; {total} bytes. No authoring/import.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--source-root", type=Path, default=ROOT / "Assets" / "Source" / "woodland-resources-20260921")
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
    if manifest["runId"] != args.run_id:
        raise ValueError("Run ID differs from admitted manifest.")
    acquire(args.manifest, args.expected_sha256, args.source_root, args.receipt,
            lambda: run_guard(args.control, args.run_id))


if __name__ == "__main__":
    main()
