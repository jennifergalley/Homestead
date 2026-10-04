"""Export Jenny's frozen original eleven meals without rebuilding art.

Usage: Invoke-BlenderLive.ps1 -File Scripts\\Blender\\export_fishing_playtest.py
       -Arguments @('--set','PreparedFood','--out','<E: scratch>')
1024px/16-sample provisional maps; no beauty renders, causal probes or UE work.
Frozen source files remain untouched. Integration owns import and release checks.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector
import homestead_kit as kit

ROOT = Path(__file__).resolve().parents[2]
FREEZE = "843324c17b85b5432785fc7a6b9e333702dc26ec"
DEFERRED = ("GrilledMackerel", "FishSoup", "FishAndPotatoes", "HerbedCarp", "MackerelChowder")
MEALS = (
    ("BakedPotatoes", "PotatoesReloadedSourceProof", "PreparedPotatoesSource.blend"),
    ("RoastedTurnips", "RoastedTurnipsSourceProof", "RoastedTurnipsSource.blend"),
    ("StewedCarrots", "StewedCarrotsSourceProof", "StewedCarrotsSource.blend"),
    ("HerbedBroadBeans", "HerbedBroadBeansSourceProof", "HerbedBroadBeansSource.blend"),
    ("CabbagePotatoStew", "CabbagePotatoStewSourceProof", "CabbagePotatoStewSource.blend"),
    ("BerryCompote", "BerryCompoteSourceProof", "BerryCompoteSource.blend"),
    ("StrawberryCompote", "StrawberryCompoteSourceProof", "StrawberryCompoteSource.blend"),
    ("RootVegetableHotpot", "RootVegetableHotpotSourceProof", "RootVegetableHotpotSource.blend"),
    ("RawFishSlices", "RawFishSlicesSourceProof", "RawFishSlicesSource.blend"),
    ("GrilledTrout", "GrilledTroutSourceProof", "GrilledTroutSource.blend"),
    ("GrilledPerch", "GrilledPerchSourceProof", "GrilledPerchSource.blend"),
)
TEXTURE_SIZE = 1024
BAKE_SAMPLES = 16
SPOON_MATERIALS = {
    "SM_CabbagePotatoStewPortion": "M_CabbageStewMapleSpoon",
    "SM_BerryCompotePortion": "M_BerryReductionMapleSpoon",
    "SM_StrawberryCompotePortion": "M_StrawberryReductionMapleSpoon",
    "SM_RootVegetableHotpotPortion": "M_RootHotpotMapleSpoon",
}


def edible_only(obj: bpy.types.Object) -> dict:
    """Keep frozen food components; utensils need a different, unauthorised clip."""
    material_name = SPOON_MATERIALS.get(obj.name)
    if material_name is None:
        return {}
    indices = {index for index, material in enumerate(obj.data.materials)
               if material and material.name == material_name}
    if len(indices) != 1:
        raise RuntimeError("Frozen spoon material is ambiguous: " + obj.name)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        removed = [face for face in mesh.faces if face.material_index in indices]
        if not removed or len(removed) == len(mesh.faces):
            raise RuntimeError("Frozen portion does not separate utensil and food: " + obj.name)
        bmesh.ops.delete(mesh, geom=removed, context="FACES")
        if any(len(edge.link_faces) != 2 for edge in mesh.edges):
            raise RuntimeError("Removing utensil opened a food component: " + obj.name)
        low = Vector(tuple(min(vertex.co[axis] for vertex in mesh.verts) for axis in range(3)))
        high = Vector(tuple(max(vertex.co[axis] for vertex in mesh.verts) for axis in range(3)))
        pivot = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z))
        for vertex in mesh.verts:
            vertex.co -= pivot
        mesh.to_mesh(obj.data)
        obj.data.update()
        return {"kind": "frozen-edible-components-only", "excluded_material": material_name,
                "original_geometry_added": False, "pivot_translation_m": list(pivot)}
    finally:
        mesh.free()


def frozen_hash(path: Path) -> str:
    relative = path.relative_to(ROOT).as_posix()
    pointer = subprocess.check_output(["git", "show", FREEZE + ":" + relative], cwd=ROOT, text=True)
    expected = next((line.split("sha256:", 1)[1] for line in pointer.splitlines()
                     if line.startswith("oid sha256:")), None)
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if expected is None or actual != expected:
        raise RuntimeError("Source differs from frozen original: " + relative)
    return actual


def geometry_hash(obj: bpy.types.Object) -> str:
    digest = hashlib.sha256()
    for vertex in obj.data.vertices:
        digest.update(struct.pack("<3f", *vertex.co))
    for face in obj.data.polygons:
        digest.update(struct.pack("<I", len(face.vertices)))
        digest.update(struct.pack("<" + str(len(face.vertices)) + "I", *face.vertices))
    for entry in obj.data.uv_layers.active.data:
        digest.update(struct.pack("<2f", *entry.uv))
    return digest.hexdigest()


def export_mesh(name: str, source_info: dict, out: Path) -> dict:
    obj = bpy.data.objects[name]
    if (obj.location.length > 1e-7 or any(abs(value) > 1e-7 for value in obj.rotation_euler)
            or any(abs(value - 1) > 1e-7 for value in obj.scale)):
        raise RuntimeError("Frozen source is not at its export origin: " + name)
    frozen_geometry = geometry_hash(obj)
    derivative = edible_only(obj)
    before = geometry_hash(obj)
    expected = kit.stats(obj) if derivative else source_info
    hidden = [other for other in bpy.context.scene.objects
              if other.type == "MESH" and other is not obj and not other.hide_render]
    for other in hidden:
        other.hide_render = True
    try:
        maps = ("basecolor", "roughness", "normal")
        window = bpy.context.window_manager.windows[0]
        area = next(area for area in window.screen.areas if area.type == "VIEW_3D")
        region = next(region for region in area.regions if region.type == "WINDOW")
        with bpy.context.temp_override(window=window, area=area, region=region):
            baked = kit.bake(obj, out / "Textures", name[3:], size=TEXTURE_SIZE,
                             samples=BAKE_SAMPLES, maps=maps)
    finally:
        for other in hidden:
            other.hide_render = False
    if geometry_hash(obj) != before:
        raise RuntimeError("Provisional bake changed frozen geometry/UVs: " + name)
    info = kit.stats(obj)
    if info["triangles"] != expected["triangles"] or info["size_cm"] != expected["size_cm"]:
        raise RuntimeError("Provisional export changed frozen shape: " + name)
    info.update({"bake": baked, "review": source_info.get("review", {}),
                 "fbx": name + ".fbx", "frozen_geometry_uv_sha256": frozen_geometry,
                 "export_geometry_uv_sha256": before, "derivative": derivative})
    with bpy.context.temp_override(window=window, area=area, region=region):
        info["sha256"] = kit.export_fbx(obj, out / info["fbx"])
    for file in baked["maps"].values():
        if not (out / "Textures" / file).is_file():
            raise RuntimeError("Missing provisional texture: " + file)
    print("PLAYTEST_ORIGINAL_EXPORT", name, info["triangles"], flush=True)
    return info


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--set", choices=("PreparedFood",), required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--refresh-bites", action="store_true",
                        help="Replace only four utensil-free portions in an existing complete export.")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    previous = json.loads((out / "report.json").read_text()) if args.refresh_bites else None
    if previous and previous["source_freeze_commit"] != FREEZE:
        raise RuntimeError("Partial refresh requires the exact frozen export.")
    report = {"schema": 1, "name": args.set, "collision": "none",
              "source_freeze_commit": FREEZE, "provisional_appearance": True,
              "final_realism_accepted": False, "blender": bpy.app.version_string,
              "provenance": "Frozen original Homestead geometry/procedural art; no reused older game assets.",
              "meshes": {}, "sources": {}, "warnings": [
                  "Provisional appearance: known baked relief patching is not resolved.",
                  "Unreal imports, material compilation and eating alignment require Integration verification."]}
    if args.set == "PreparedFood":
        report["triangle_budget"] = 80000
        report["deferred_items"] = list(DEFERRED)
        report["item_meshes"] = {}
        for item, folder, filename in MEALS:
            source = ROOT / "Assets" / "Props" / "PreparedFood" / folder
            blend = source / filename
            source_hash = frozen_hash(blend)
            original = json.loads((source / "report.json").read_text())
            bpy.ops.wm.open_mainfile(filepath=str(blend))
            names = ("SM_" + item, "SM_" + item + "Portion")
            if set(original["meshes"]) != set(names):
                raise RuntimeError("Frozen meal has unexpected meshes: " + item)
            report["sources"][item] = {"blend": blend.relative_to(ROOT).as_posix(),
                                       "sha256": source_hash, "ingredients": original["ingredients"]}
            report["item_meshes"][item] = {"serving": names[0], "portion": names[1]}
            for name in names:
                if previous and name not in SPOON_MATERIALS:
                    report["meshes"][name] = previous["meshes"][name]
                else:
                    report["meshes"][name] = export_mesh(name, original["meshes"][name], out)
        if len(report["meshes"]) != 22:
            raise RuntimeError("The eleven-meal playtest requires eleven servings and eleven edible portions")
    for name, info in report["meshes"].items():
        if info["triangles"] > report["triangle_budget"]:
            raise RuntimeError("Frozen source exceeds the established mesh ceiling: " + name)
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("PLAYTEST_ORIGINAL_SET_READY", args.set, len(report["meshes"]), flush=True)


if __name__ == "__main__":
    main()
