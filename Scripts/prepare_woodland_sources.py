"""Bounded Poly Haven source acquisition; never imports assets or runs an engine."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import time
from datetime import datetime, timezone
from urllib.parse import urlsplit
from urllib.request import Request, build_opener, HTTPRedirectHandler

ROOT = Path(__file__).resolve().parents[1]
RUN = "20260920-182217-d1f84e39"
SOURCE = ROOT / "Assets" / "Source" / ("woodland-preparation-" + RUN)
RECORDS = ROOT / "Assets" / "Environment" / "woodland-preparation-01"
ASSETS = ("tree_small_02", "fern_02", "grass_medium_01", "grass_ground")
USER_AGENT = "HomesteadSourcePreparation/1.0 (Poly Haven asset acquisition)"


def run_guard():
    state = json.loads((ROOT / "Automation" / "run.json").read_text(encoding="utf-8-sig"))
    if state["id"] != RUN or state["state"] != "running":
        raise RuntimeError("Run changed or is not running; no further source work.")
    if datetime.now(timezone.utc) >= datetime.fromisoformat(state["deadlineUtc"]):
        raise RuntimeError("Run deadline reached.")


def leaf_name(name):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,180}", name) or ".." in name or name.endswith("."):
        raise ValueError("Unsafe source filename: " + repr(name))
    if name.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *("COM%d" % i for i in range(10)), *("LPT%d" % i for i in range(10))}:
        raise ValueError("Reserved filename.")
    return name


def contained(base, *parts):
    if base.is_symlink() or base.is_junction():
        raise ValueError("Isolated source root must not be a link/junction.")
    base = base.resolve()
    path = base.joinpath(*parts)
    if not path.resolve().is_relative_to(base):
        raise ValueError("Source path escapes isolated directory.")
    return path


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ValueError("Unexpected redirect; review explicitly: " + newurl)


def validate_url(url, host):
    parsed = urlsplit(url)
    if parsed.scheme != "https" or parsed.netloc != host or parsed.query or parsed.fragment:
        raise ValueError("Unexpected source URL: " + url)
    if "%" in parsed.path or "\\" in parsed.path or ".." in parsed.path:
        raise ValueError("Ambiguous source URL path.")
    return parsed


def fetch(url, path, maximum, host, expected_bytes=None):
    validate_url(url, host)
    run_guard()
    if path.exists() or path.with_suffix(path.suffix + ".download").exists():
        raise FileExistsError("Refusing to replace source or retained partial: " + str(path))
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".download")
    start = time.monotonic()
    with build_opener(NoRedirect).open(Request(url, headers={"User-Agent": USER_AGENT}), timeout=45) as response, temporary.open("xb") as output:
        length = response.headers.get("Content-Length")
        if length and (int(length) > maximum or (expected_bytes is not None and int(length) != expected_bytes)):
            raise ValueError("Unexpected HTTP content length.")
        count = 0
        while True:
            run_guard()
            if time.monotonic() - start > 300:
                raise TimeoutError("Source download exceeded five-minute limit.")
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            count += len(chunk)
            if count > maximum:
                raise ValueError("Source exceeds byte limit; partial retained.")
            output.write(chunk)
    if expected_bytes is not None and count != expected_bytes:
        raise ValueError("Source size mismatch; partial retained.")
    return temporary


def digest(path, algorithm):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, algorithm).hexdigest().upper()


def write_new(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=True, allow_nan=False)
        stream.write("\n")


def metadata(asset, kind):
    url = f"https://api.polyhaven.com/{kind}/{asset}"
    path = contained(SOURCE, "metadata", f"{asset}-{kind}.json")
    temporary = fetch(url, path, 2 * 1024 * 1024, "api.polyhaven.com")
    data = json.loads(temporary.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or not data:
        raise ValueError("Empty/invalid publisher metadata; partial retained.")
    temporary.rename(path)
    write_new(path.with_suffix(".receipt.json"), {
        "url": url, "accessedUtc": datetime.now(timezone.utc).isoformat(),
        "bytes": path.stat().st_size, "sha256": digest(path, "sha256"),
        "provider": "Poly Haven", "userAgent": USER_AGENT,
    })
    print(asset, kind, list(data) if kind == "files" else data)


def validate_file(asset, item):
    if asset not in ASSETS:
        raise ValueError("Asset outside admitted palette.")
    name = leaf_name(item["name"])
    parsed = validate_url(item["url"], "dl.polyhaven.org")
    if not parsed.path.startswith("/file/ph-assets/") or parsed.path.split("/")[-2:] != [asset, name]:
        raise ValueError("Source URL does not identify selected asset/file.")
    if Path(name).suffix not in (".fbx", ".png", ".jpg"):
        raise ValueError("Unapproved source file type.")
    resolution = "2k" if asset in ("tree_small_02", "grass_ground") else "1k"
    if f"/{resolution}/{asset}/" not in parsed.path or not name.endswith("_" + resolution + Path(name).suffix):
        raise ValueError("Unexpected source resolution.")
    if type(item["bytes"]) is not int or not 0 < item["bytes"] <= 128 * 1024 * 1024:
        raise ValueError("Invalid or excessive file size.")
    if not re.fullmatch(r"[0-9a-fA-F]{1,32}", item["publisherMd5"]):
        raise ValueError("Invalid publisher MD5.")


def plan():
    run_guard()
    selection = {
        "tree_small_02": [("fbx", "fbx"), ("Diffuse", "jpg"), ("nor_dx", "png"),
                          ("Rough", "png"), ("AO", "png"), ("branch_diff", "png"),
                          ("branch_nor_dx", "png"), ("branch_rough", "png"),
                          ("branch_ao", "png"), ("leaves_diff", "png"),
                          ("leaves_nor_dx", "png"), ("leaves_rough", "png"),
                          ("leaves_ao", "png"), ("leaves_alpha", "png")],
        "fern_02": [("fbx", "fbx"), ("Diffuse", "png"), ("nor_dx", "png"),
                    ("Rough", "png"), ("AO", "png"), ("Alpha", "png")],
        "grass_medium_01": [("fbx", "fbx"), ("Diffuse", "png"), ("dry_diff", "png"),
                            ("nor_dx", "png"), ("Rough", "png"), ("AO", "png"), ("Alpha", "png")],
        "grass_ground": [("Diffuse", "png"), ("nor_dx", "png"), ("Rough", "png")],
    }
    assets = []
    evidence = []
    for asset in ASSETS:
        info_path = SOURCE / "metadata" / (asset + "-info.json")
        files_path = SOURCE / "metadata" / (asset + "-files.json")
        page_path = SOURCE / "metadata" / (asset + "-page.html")
        info = json.loads(info_path.read_text(encoding="utf-8"))
        catalog = json.loads(files_path.read_text(encoding="utf-8"))
        page = page_path.read_text(encoding="utf-8")
        if '"license":"https://creativecommons.org/publicdomain/zero/1.0/"' not in page or not info["description"].startswith("Free "):
            raise ValueError("Publisher free/CC0 evidence missing: " + asset)
        resolution = "2k" if asset in ("tree_small_02", "grass_ground") else "1k"
        selected = []
        for channel, fmt in selection[asset]:
            record = catalog[channel][resolution][fmt]
            item = {"name": urlsplit(record["url"]).path.split("/")[-1],
                    "bytes": record["size"], "url": record["url"],
                    "publisherMd5": record["md5"], "channel": channel, "format": fmt}
            validate_file(asset, item)
            selected.append(item)
        assets.append({
            "id": asset, "name": info["name"], "authors": info["authors"],
            "license": "CC0-1.0", "source": "https://polyhaven.com/a/" + asset,
            "licenseUrl": "https://creativecommons.org/publicdomain/zero/1.0/",
            "publisherLicenseUrl": "https://polyhaven.com/license",
            "resolution": resolution, "publisherFilesHash": info["files_hash"],
            "publisherAggregatePolycount": info.get("polycount"),
            "publisherLods": info.get("lods"), "publisherGeometryNodes": info.get("geonodes"),
            "files": selected,
            "fbxAdvertisedDependenciesNotAutoFetched": list(catalog.get("fbx", {}).get(resolution, {}).get("fbx", {}).get("include", {})),
        })
        for path, url in [(info_path, f"https://api.polyhaven.com/info/{asset}"),
                          (files_path, f"https://api.polyhaven.com/files/{asset}"),
                          (page_path, f"https://polyhaven.com/a/{asset}")]:
            evidence.append({"path": str(path.relative_to(SOURCE)), "url": url,
                             "bytes": path.stat().st_size, "sha256": digest(path, "sha256")})
    license_path = SOURCE / "metadata" / "license.html"
    if "You can redistribute them" not in license_path.read_text(encoding="utf-8"):
        raise ValueError("Publisher redistribution declaration missing.")
    evidence.append({"path": str(license_path.relative_to(SOURCE)), "url": "https://polyhaven.com/license",
                     "bytes": license_path.stat().st_size, "sha256": digest(license_path, "sha256")})
    manifest = {"version": 1, "phase": "source-only; engine gate blocked", "provider": "Poly Haven",
                "refreshedUtc": datetime.now(timezone.utc).isoformat(),
                "sourceRoot": str(SOURCE.relative_to(ROOT)), "assets": assets, "evidence": evidence,
                "selectionNotes": [
                    "Original files retained unchanged; no automatic FBX dependency lookup.",
                    "DX normals and explicit PNG roughness/alpha/AO replace auto-linked GL/EXR choices in future manual material wiring, not in the source FBX.",
                    "Tree bark uses advertised JPG diffuse; alpha-bearing branch/leaves use PNG. Grass dry diffuse retained for source material inspection.",
                    "Ground uses only diffuse/DX normal/roughness. No displacement, blend/scatter scenes, archives or extra resolutions.",
                    "API access uses identified User-Agent under https://github.com/Poly-Haven/Public-API/blob/master/ToS.md; no account, key or checkout.",
                    "CC0 permits source/cooked redistribution; website/previews are not assumed CC0 and raw page evidence stays ignored.",
                ]}
    write_new(RECORDS / "asset-manifest.json", manifest)
    print("Selected", sum(len(a["files"]) for a in assets), "files;",
          sum(f["bytes"] for a in assets for f in a["files"]), "publisher-declared bytes.")


def verify(path, item):
    if path.stat().st_size != item["bytes"]:
        raise ValueError("Source size mismatch: " + path.name)
    # Publisher hexadecimal fields may omit leading zeroes; preserve the original.
    if digest(path, "md5") != item["publisherMd5"].zfill(32).upper():
        raise ValueError("Publisher MD5 mismatch: " + path.name)
    return digest(path, "sha256")


def download():
    manifest = json.loads((RECORDS / "asset-manifest.json").read_text(encoding="utf-8"))
    if tuple(a["id"] for a in manifest["assets"]) != ASSETS:
        raise ValueError("Manifest palette/order differs from approved preparation.")
    total = sum(f["bytes"] for a in manifest["assets"] for f in a["files"])
    if total > 512 * 1024 * 1024:
        raise ValueError("Preparation exceeds 512 MiB download cap.")
    seen = set()
    for asset in manifest["assets"]:
        for item in asset["files"]:
            validate_file(asset["id"], item)
            key = (asset["id"], item["name"])
            if key in seen:
                raise ValueError("Duplicate manifest destination.")
            seen.add(key)
    receipt_path = RECORDS / "download-receipt.json"
    prior = json.loads(receipt_path.read_text()) if receipt_path.exists() else None
    if prior is not None and prior["manifestSha256"] != digest(RECORDS / "asset-manifest.json", "sha256"):
        raise ValueError("Manifest changed since receipt; review instead of replacing.")
    entries = []
    for asset in manifest["assets"]:
        for item in asset["files"]:
            run_guard()
            path = contained(SOURCE, asset["id"], leaf_name(item["name"]))
            if not path.exists():
                if prior is not None:
                    raise FileNotFoundError("Receipted source is missing; do not silently reacquire.")
                temporary = fetch(item["url"], path, item["bytes"], "dl.polyhaven.org", item["bytes"])
                verify(temporary, item)
                temporary.rename(path)
            sha = verify(path, item)
            entry = {"asset": asset["id"], "file": item["name"], "bytes": path.stat().st_size,
                     "sha256": sha, "publisherMd5": item["publisherMd5"],
                     "actualMd5": digest(path, "md5"), "source": item["url"], "license": "CC0-1.0"}
            if prior is not None and entry not in prior["files"]:
                raise ValueError("Source differs from sealed receipt; no overwrite.")
            entries.append(entry)
            print("Verified", asset["id"], path.name, path.stat().st_size, sha, flush=True)
    if prior is not None:
        if entries != prior["files"]:
            raise ValueError("Receipt entries differ.")
    else:
        write_new(receipt_path, {"version": 1, "sourceRoot": str(SOURCE.relative_to(ROOT)),
                                "acquiredUtc": datetime.now(timezone.utc).isoformat(),
                                "manifestSha256": digest(RECORDS / "asset-manifest.json", "sha256"),
                                "totalBytes": total, "files": entries})
    print(f"Poly Haven: {len(entries)} files / {total} bytes verified; no engine import.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("metadata", "plan", "download"))
    parser.add_argument("--asset", choices=ASSETS)
    parser.add_argument("--kind", choices=("info", "files"))
    args = parser.parse_args()
    if args.action == "metadata":
        if not args.asset or not args.kind:
            parser.error("metadata requires --asset and --kind")
        metadata(args.asset, args.kind)
    elif args.action == "plan":
        plan()
    else:
        download()
