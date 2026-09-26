"""Build Homestead static props in Blender, headless.

    blender --background --factory-startup --python build_prop.py -- --recipe <recipe.py>
    blender --background --factory-startup --python build_prop.py -- --blend <file.blend>

Recipe mode runs ``build(kit)`` from a recipe module; blend mode exports every
``SM_*`` mesh object from a hand-edited .blend. Both write, per asset set, to
``Assets/Props/<Name>/``: one FBX per mesh, the source .blend, a contact-sheet
preview per mesh and ``report.json`` (consumed by the Unreal importer).
"""
import argparse
import hashlib
import importlib.util
import json
import re
import sys
from pathlib import Path

import bpy

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
import homestead_kit as kit  # noqa: E402
import homestead_materials as mats  # noqa: E402

if not bpy.app.background:
    # A live session keeps modules loaded between requests; pick up kit edits.
    kit = importlib.reload(kit)
    mats = importlib.reload(mats)
kit.mats = mats


def load_recipe(path):
    spec = importlib.util.spec_from_file_location("homestead_recipe", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    for required in ("NAME", "build"):
        if not hasattr(module, required):
            raise RuntimeError(f"Recipe {path} must define {required}")
    return module


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--recipe")
    source.add_argument("--blend")
    parser.add_argument("--out")
    parser.add_argument("--name", help="Asset set name for --blend (default: file name)")
    parser.add_argument("--no-preview", action="store_true")
    parser.add_argument("--keep-pivot", action="store_true")
    args = parser.parse_args(argv)

    if args.recipe:
        source_path = Path(args.recipe).resolve()
        recipe = load_recipe(source_path)
        name, meta = recipe.NAME, {
            "description": getattr(recipe, "DESCRIPTION", ""),
            "collision": getattr(recipe, "COLLISION", "box"),
            "triangle_budget": getattr(recipe, "TRIANGLE_BUDGET", 5000),
        }
        kit.reset()
        built = recipe.build(kit)
        meshes = list(built) if isinstance(built, (list, tuple)) else [built]
        for obj in meshes:
            if not obj.get("homestead_finalized"):
                kit.finalize(obj)
        # Drop anything the recipe left behind that is not an exported mesh.
        for obj in list(bpy.context.scene.objects):
            if obj not in meshes:
                bpy.data.objects.remove(obj)
    else:
        source_path = Path(args.blend).resolve()
        bpy.ops.wm.open_mainfile(filepath=str(source_path))
        name = args.name or source_path.stem
        scene_meta = bpy.context.scene.get("homestead", {})
        meta = {"description": scene_meta.get("description", ""),
                "collision": scene_meta.get("collision", "box"),
                "triangle_budget": scene_meta.get("triangle_budget", 5000)}
        meshes = [o for o in bpy.context.scene.objects if o.type == "MESH" and o.name.startswith("SM_")]
        if not meshes:
            raise RuntimeError("No mesh objects named SM_* in " + str(source_path))
        for obj in meshes:
            kit.finalize(obj, pivot=None if args.keep_pivot else "base")

    if meta["collision"] not in ("none", "box", "convex"):
        raise RuntimeError("COLLISION must be none, box or convex")
    out = Path(args.out).resolve() if args.out else ROOT / "Assets" / "Props" / name
    out.mkdir(parents=True, exist_ok=True)

    report = {"schema": 1, "name": name, **meta, "blender": bpy.app.version_string,
              "source": source_path.relative_to(ROOT).as_posix() if source_path.is_relative_to(ROOT)
              else str(source_path),
              "source_sha256": hashlib.sha256(source_path.read_bytes()).hexdigest(),
              "provenance": getattr(recipe, "PROVENANCE", None) if args.recipe else None,
              "meshes": {}, "warnings": []}
    report["provenance"] = report["provenance"] or \
        "Original project-authored geometry; no third-party asset or texture"
    if args.recipe and getattr(recipe, "NOTES", None):
        report["notes"] = recipe.NOTES
    if args.recipe:
        report.update(getattr(recipe, "REPORT", {}))
    bake_spec = getattr(recipe, "BAKE", None) if args.recipe else None
    baked_materials = {}
    for obj in meshes:
        baked = None
        lod = re.match(r"^(SM_.+)_LOD\d+$", obj.name)
        if bake_spec and lod and lod.group(1) in baked_materials:
            # LODs decimated from an unwrapped LOD0 share its UVs and texture set.
            obj.data.materials.clear()
            obj.data.materials.append(baked_materials[lod.group(1)][0])
            baked = dict(baked_materials[lod.group(1)][1], shared_with=lod.group(1))
        elif bake_spec:
            if bake_spec.get("repack", True):
                kit.pack_uvs(obj, margin=bake_spec.get("margin", 0.004))
            print(f"HOMESTEAD_BAKING {obj.name} {bake_spec.get('size', 2048)}px", flush=True)
            # Hide the other meshes (notably LODs sitting exactly on top of LOD0) so they don't
            # occlude the AO bake or catch its rays.
            hidden = [o for o in meshes if o is not obj and not o.hide_render]
            for other in hidden:
                other.hide_render = True
            try:
                baked = kit.bake(obj, out / "Textures", obj.name[3:], size=bake_spec.get("size", 2048),
                                 samples=bake_spec.get("samples", 96),
                                 maps=tuple(bake_spec.get("maps", kit.BAKE_MAPS)))
            finally:
                for other in hidden:
                    other.hide_render = False
            if hasattr(recipe, "after_bake"):
                recipe.after_bake(kit, obj)
            baked_materials[obj.name] = (obj.material_slots[0].material, baked)
        info = kit.stats(obj)
        if baked:
            info["bake"] = baked
        review = dict(getattr(recipe, "BEAUTY", {})) if args.recipe else {}
        review.update(review.pop("meshes", {}).get(obj.name, {}))
        if review.get("focus") is not None:
            shift = obj.get("homestead_shift", (0, 0, 0))
            review["focus"] = [f - s for f, s in zip(review["focus"], shift)]
        info["review"] = review
        if info["triangles"] == 0 or min(info["size_cm"]) <= 0:
            raise RuntimeError(f"{obj.name} is empty or flat: {info}")
        if not info["materials"] or len(info["materials"]) != len(obj.material_slots):
            raise RuntimeError(f"{obj.name} has an empty material slot")
        if info["triangles"] > meta["triangle_budget"]:
            report["warnings"].append(
                f"{obj.name}: {info['triangles']} triangles exceeds budget {meta['triangle_budget']}")
        info["fbx"] = obj.name + ".fbx"
        info["sha256"] = kit.export_fbx(obj, out / info["fbx"])
        report["meshes"][obj.name] = info

    if any("textures" in m for info in report["meshes"].values() for m in info["materials"]):
        report["textures"] = kit.copy_textures(meshes, out / "Textures")

    if args.recipe:
        bpy.context.scene["homestead"] = meta
        bpy.ops.wm.save_as_mainfile(filepath=str(out / (name + ".blend")), compress=True, copy=True,
                                    relative_remap=True)
    if not args.no_preview:
        for obj in meshes:
            preview = out / f"preview_{obj.name}.png"
            kit.render_preview(obj, preview)
            report["meshes"][obj.name]["preview"] = preview.name

    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    kit.focus(meshes)
    for warning in report["warnings"]:
        print("HOMESTEAD_PROP_WARNING " + warning)
    for mesh_name, info in report["meshes"].items():
        print(f"HOMESTEAD_PROP_MESH {mesh_name} tris={info['triangles']} size_cm={info['size_cm']}")
    print("HOMESTEAD_PROP_BUILT " + str(out / "report.json"))


if __name__ == "__main__":
    main()
