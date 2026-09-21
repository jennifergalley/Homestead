"""Reconstruct covered modular bodies and export source-only wardrobe assets."""
import argparse
import hashlib
import importlib
import json
import math
import sys
from pathlib import Path

import bpy
import bmesh
import numpy as np
from mathutils import Vector
from mathutils.kdtree import KDTree

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).parent))
import export_heroine as exporter
import build_heroine as author
import extend_variants as variants

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "ModularClothing"
PREVIEW = ROOT / "Build" / "CharacterPreview" / "ModularClothing"
SOURCES = {
    "Preferred": ROOT / "Assets" / "Characters" / "Variants" / "Heroine_Variants.blend",
    "Willow": ROOT / "Assets" / "Characters" / "BodyPresets" / "Willow" / "Willow.blend",
    "Hazel": ROOT / "Assets" / "Characters" / "BodyPresets" / "Hazel" / "Hazel.blend",
}
HAIR = ("LongWave", "Bob", "Ponytail")
TUNIC_PARTS = (
    "WrapTunic", "HemBinding", "NecklineBinding", "WrapOverlapBinding",
    "ShoulderStrap_-1", "ShoulderStrap_1", "LeatherBelt", "BeltBuckle",
)
FACE_PARTS = ("Eyes", "Eyebrows", "Eyelashes", "Teeth", "Tongue")
GARMENTS = {
    "Tunic": {"definition": "linen-tunic", "definition_id": 0, "slots": ["Torso", "Legs"],
              "dyes": [0, 1, 2, 3]},
    "Apron": {"definition": "linen-apron", "definition_id": 1, "slots": ["Apron"],
              "requires": "linen-tunic", "dyes": [0, 1, 2, 3]},
    "Shoes": {"definition": "legacy-laceup-shoes", "definition_id": 2, "slots": ["Feet"],
              "dyes": [0]},
    "Footwraps": {"definition": "woven-footwraps", "definition_id": 3, "slots": ["Feet"],
                  "dyes": [0]},
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def recipe_path(body):
    return (ROOT / "Assets" / "Characters" / "Heroine" / "recipe.json" if body == "Preferred"
            else SOURCES[body].parent / "recipe.json")


def enable_mpfb():
    module = "bl_ext.user_default.mpfb"
    bpy.ops.preferences.addon_enable(module=module)
    addon = importlib.import_module(module)
    require(addon.VERSION == (2, 0, 17), "Expected verified MPFB 2.0.17")
    return Path(addon.__file__).parent


def freeze(obj, rig):
    exporter.active(obj)
    for modifier in list(obj.modifiers):
        if modifier.type != "ARMATURE":
            bpy.ops.object.modifier_apply(modifier=modifier.name)
    exporter.normalize(obj, rig)


def reconstruct_body(body_name, rig):
    old = bpy.data.objects["Heroine_Body"]
    recipe = json.loads(recipe_path(body_name).read_text())
    require(recipe["macro"]["age"] == 0.5, "Adult recipe anchor changed")
    human = author.service("HumanService")
    targets = author.service("TargetService")
    donor = human.create_human(macro_detail_dict=recipe["macro"])
    target_root = Path(author.service("LocationService").get_mpfb_data("targets"))
    target_hashes = {}
    for name, weight in recipe["targets"].items():
        path = target_root / (name + ".target.gz")
        target_hashes[str(path.relative_to(target_root))] = digest(path)
        targets.load_target(donor, str(path), weight=weight)
    donor_rig = human.add_builtin_rig(donor, "game_engine")
    author.freeze_mesh(donor)
    height = max(v.co.z for v in donor.data.vertices) - min(v.co.z for v in donor.data.vertices)
    factor = recipe["height_m"] / height
    for vertex in donor.data.vertices:
        vertex.co *= factor
    donor.location *= factor
    tree = KDTree(len(donor.data.vertices))
    for vertex in donor.data.vertices:
        tree.insert(vertex.co, vertex.index)
    tree.balance()
    error = max(tree.find(v.co)[2] for v in old.data.vertices)
    require(error < 0.0001, f"{body_name}: reconstructed body differs from source by {error} m")
    for modifier in donor.modifiers:
        if modifier.type == "ARMATURE":
            modifier.object = rig
    donor.parent = rig
    donor.matrix_parent_inverse.identity()
    donor.matrix_basis.identity()
    donor.data.materials.clear()
    donor.data.materials.append(old.data.materials[0])
    bpy.data.objects.remove(old, do_unlink=True)
    bpy.data.objects.remove(donor_rig, do_unlink=True)
    donor.name = "Heroine_Body"
    exporter.normalize(donor, rig)
    feet = [sum(v.co.z < 0.145 and v.co.x * sign > 0 for v in donor.data.vertices)
            for sign in (-1, 1)]
    require(min(feet) > 100, "Reconstruction did not recover both feet")
    record = {"recipe": str(recipe_path(body_name).relative_to(ROOT)),
              "recipe_sha256": digest(recipe_path(body_name)),
              "maximum_retained_vertex_error_m": error,
              "source_vertices": len(donor.data.vertices),
              "source_faces": len(donor.data.polygons), "feet_vertices_per_side": feet,
              "normalization_factor": factor, "detail_target_sha256": target_hashes}
    return donor, record


def woven_material(name, color):
    mat = author.material(name, color, 0.92)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    tex = nodes.new("ShaderNodeTexImage")
    path = OUT / "Textures" / "T_ModularWeave.png"
    if not path.exists():
        size = 256
        y, x = np.mgrid[0:size, 0:size]
        value = 0.90 + 0.055 * np.sin(x * math.tau / 8) + 0.045 * np.sin(y * math.tau / 8)
        pixels = np.ones((size, size, 4), dtype=np.float32)
        pixels[:, :, :3] = value[:, :, None]
        image = bpy.data.images.new("T_ModularWeave", width=size, height=size, alpha=False)
        image.pixels.foreach_set(pixels.ravel())
        image.filepath_raw = str(path)
        image.file_format = "PNG"
        image.save()
    tex.image = bpy.data.images.load(str(path), check_existing=True)
    coordinates = nodes.new("ShaderNodeTexCoord")
    scale = nodes.new("ShaderNodeVectorMath")
    scale.operation = "SCALE"
    scale.inputs[3].default_value = 8
    links.new(coordinates.outputs["UV"], scale.inputs[0])
    links.new(scale.outputs[0], tex.inputs["Vector"])
    multiply = nodes.new("ShaderNodeMixRGB")
    multiply.blend_type = "MULTIPLY"
    multiply.inputs[0].default_value = 1
    multiply.inputs[2].default_value = (*color, 1)
    links.new(tex.outputs["Color"], multiply.inputs[1])
    links.new(multiply.outputs[0], nodes.get("Principled BSDF").inputs["Base Color"])
    mat["ModularTextureMultiply"] = True
    return mat


def add_permanent_coverage(body, rig):
    """Replace the surface with opaque cloth: coverage cannot separate from skin."""
    bra_material = woven_material("M_Modular_BaseBra", (0.16, 0.175, 0.15))
    briefs_material = woven_material("M_Modular_BaseBriefs", (0.16, 0.175, 0.15))
    sub = body.modifiers.new("Retained body refinement", "SUBSURF")
    sub.levels = sub.render_levels = 1
    freeze(body, rig)
    bm = bmesh.new()
    bm.from_mesh(body.data)
    for z in (0.91, 1.08, 1.205):
        bmesh.ops.bisect_plane(bm, geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                              dist=1e-7, plane_co=(0, 0, z), plane_no=(0, 0, 1))
    for x in (-0.165, 0.165):
        bmesh.ops.bisect_plane(bm, geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                              dist=1e-7, plane_co=(x, 0, 0), plane_no=(1, 0, 0))
    for sign in (-1, 1):
        bmesh.ops.bisect_plane(bm, geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                              dist=1e-7, plane_co=(0, 0, 0.74), plane_no=(-sign*0.38, 0, 1))
    bm.to_mesh(body.data)
    bm.free()
    body.data.materials.append(bra_material)
    body.data.materials.append(briefs_material)
    covered = set()
    for face in body.data.polygons:
        points = [body.data.vertices[i].co for i in face.vertices]
        center = sum(points, Vector()) / len(points)
        # Plane-cut seams avoid stair-stepped color changes on the original topology.
        if 1.08 < center.z < 1.205 and abs(center.x) < 0.165:
            face.material_index = 1
            covered.update(face.vertices)
        elif 0.74 + 0.38*abs(center.x) < center.z < 0.91 and abs(center.x) < 0.225:
            face.material_index = 2
            covered.update(face.vertices)
    require(len(covered) > 1000, "Permanent layer has insufficient coverage")
    # Remove anatomical high-frequency detail beneath the permanently sewn cloth.
    neighbours = {i: set() for i in covered}
    for edge in body.data.edges:
        a, b = edge.vertices
        if a in covered and b in covered:
            neighbours[a].add(b)
            neighbours[b].add(a)
    boundary = {i for p in body.data.polygons if p.material_index == 0 for i in p.vertices}
    interior = covered - boundary
    for _ in range(4):
        updates = {}
        for i in interior:
            if neighbours[i]:
                vertex = body.data.vertices[i]
                mean = sum((body.data.vertices[j].co for j in neighbours[i]), Vector()) / len(neighbours[i])
                updates[i] = vertex.co.lerp(mean, 0.35)
        for i, co in updates.items():
            body.data.vertices[i].co = co
    straps = []
    for sign in (-1, 1):
        source = bpy.data.objects["Heroine_ShoulderStrap_" + str(sign)]
        strap = source.copy()
        strap.data = source.data.copy()
        bpy.context.collection.objects.link(strap)
        strap.name = "Modular_BaseStrap_" + str(sign)
        strap.data.materials.clear()
        strap.data.materials.append(bra_material)
        for vertex in strap.data.vertices:
            vertex.co.x = sign*0.108 + (vertex.co.x-sign*0.108)*0.65
            vertex.co.y -= math.copysign(0.004, vertex.co.y)
        straps.append(strap)
    panels = []
    face_counts = {}
    for index, label, material in ((1, "Bra", bra_material), (2, "Briefs", briefs_material)):
        panel = body.copy()
        panel.data = body.data.copy()
        bpy.context.collection.objects.link(panel)
        panel.name = "Modular_Base" + label
        bm = bmesh.new()
        bm.from_mesh(panel.data)
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index != index], context="FACES")
        bm.to_mesh(panel.data)
        bm.free()
        panel.data.materials.clear()
        panel.data.materials.append(material)
        for face in panel.data.polygons:
            face.material_index = 0
        face_counts[label] = len(panel.data.polygons)
        require(face_counts[label] > 100, label + " coverage missing")
        exporter.normalize(panel, rig)
        panels.append(panel)
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index != 0], context="FACES")
    bm.to_mesh(body.data)
    bm.free()
    skin = body.data.materials[0]
    body.data.materials.clear()
    body.data.materials.append(skin)
    exporter.normalize(body, rig)
    return {"method": "Separate permanent opaque bra and briefs replace covered body surface faces; no underlying removable nude mesh.",
            "materials": [bra_material.name, briefs_material.name], "cloth_faces": face_counts,
            "bra_z_m": [1.08, 1.205], "briefs_top_m": 0.91,
            "briefs_lower_seam": "z = 0.74 + 0.38 * abs(x)",
            "body_occlusion_masks": []}, panels + straps


def footwraps(body, rig):
    """Original closed cloth slippers with cross-laced ankle bands, fitted per body."""
    from mathutils.bvhtree import BVHTree
    cloth = woven_material("M_Modular_FootwrapCloth", (0.49, 0.40, 0.26))
    binding = woven_material("M_Modular_FootwrapBinding", (0.25, 0.19, 0.11))
    pieces = []
    for side, sign in (("l", 1), ("r", -1)):
        vertices = [v.co.copy() for v in body.data.vertices if v.co.z < 0.21 and v.co.x * sign > 0]
        require(len(vertices) > 100, "Foot fitting source missing")
        edges = [(body.data.vertices[e.vertices[0]].co, body.data.vertices[e.vertices[1]].co)
                 for e in body.data.edges
                 if all(body.data.vertices[i].co.z < 0.27 and body.data.vertices[i].co.x*sign > 0
                        for i in e.vertices)]
        def section(axis, value):
            points = []
            for a, b in edges:
                if min(a[axis], b[axis]) <= value <= max(a[axis], b[axis]) and abs(b[axis]-a[axis]) > 1e-8:
                    points.append(a.lerp(b, (value-a[axis])/(b[axis]-a[axis])))
            require(bool(points), f"Missing fitted {side} foot section {axis}={value}")
            return points
        # Cross-sections of the fitted foot create a smooth closed toe box rather
        # than cloning the legacy shoe or leaving individual cloth-covered toes.
        sole_points = [v for v in vertices if v.z < 0.10]
        min_y, max_y = min(v.y for v in sole_points), max(v.y for v in sole_points)
        verts, faces = [], []
        rows, columns = 36, 48
        for j in range(rows + 1):
            y = min_y - 0.005 + (max_y - min_y + 0.010) * j / rows
            samples = section(1, max(min_y+0.001, min(max_y-0.001, y)))
            samples = [v for v in samples if v.z < 0.15]
            require(bool(samples), f"No foot cross section at {side} {y}")
            xmin, xmax = min(v.x for v in samples) - 0.007, max(v.x for v in samples) + 0.007
            zmin, zmax = min(v.z for v in samples) - 0.003, max(v.z for v in samples) + 0.006
            cx, cz = (xmin + xmax) / 2, (zmin + zmax) / 2
            cap = 0.15 if j in (0, rows) else 1.0
            for i in range(columns):
                angle = math.tau * i / columns
                x = math.copysign(abs(math.cos(angle)) ** 0.65, math.cos(angle))
                z = math.copysign(abs(math.sin(angle)) ** 0.65, math.sin(angle))
                verts.append((cx + (xmax - xmin) / 2 * x * cap, y,
                              max(0.001, cz + (zmax - zmin) / 2 * z * cap)))
        for j in range(rows):
            for i in range(columns):
                n = (i + 1) % columns
                k = j * columns
                faces.append((k+i, k+n, k+columns+n, k+columns+i))
        faces += [tuple(reversed(range(columns))), tuple(rows*columns+i for i in range(columns))]
        shoe = author.mesh("Modular_Footwrap_" + side, verts, faces, cloth)
        uv = shoe.data.uv_layers.new(name="UVMap")
        for loop in shoe.data.loops:
            row, column = divmod(loop.vertex_index, columns)
            uv.data[loop.index].uv = (column / columns * 5, row / rows * 6)
        author.skin_weights(shoe, body, rig)
        pieces.append(shoe)
        # The cuff follows actual ankle sections and overlaps the slipper.
        cuff_v, cuff_f = [], []
        count, levels = 48, 16
        for j in range(levels + 1):
            z = 0.095 + 0.10 * j / levels
            samples = section(2, z)
            cx = sum(v.x for v in samples) / len(samples)
            cy = sum(v.y for v in samples) / len(samples)
            rx = max(abs(v.x-cx) for v in samples) + 0.008
            ry = max(abs(v.y-cy) for v in samples) + 0.008
            for i in range(count):
                angle = math.tau*i/count
                cuff_v.append((cx+rx*math.cos(angle), cy+ry*math.sin(angle), z))
        for j in range(levels):
            for i in range(count):
                n = (i+1) % count
                k = j*count
                cuff_f.append((k+i, k+n, k+count+n, k+count+i))
        cuff = author.mesh("Modular_FootwrapCuff_" + side, cuff_v, cuff_f, cloth)
        author.skin_weights(cuff, body, rig)
        pieces.append(cuff)
        tree = BVHTree.FromPolygons([Vector(v) for v in cuff_v], cuff_f)
        for winding in (-1, 1):
            band_v, band_f = [], []
            for j in range(161):
                t = j/160
                angle = winding * t * math.tau * 2.2
                z = 0.11 + t*0.075
                samples = [Vector(v) for v in cuff_v if abs(v[2]-z) < 0.004]
                center = sum(samples, Vector()) / len(samples)
                direction = Vector((math.cos(angle), math.sin(angle), 0))
                hit, _, _, _ = tree.ray_cast(Vector((center.x, center.y, z)), direction, 0.2)
                require(hit is not None, "Footwrap band fitting missed cuff")
                for dz in (-0.004, 0.004):
                    band_v.append(hit + direction*0.002 + Vector((0, 0, dz)))
                if j:
                    k = (j-1)*2
                    band_f.append((k, k+1, k+3, k+2))
            band = author.mesh(f"Modular_FootwrapBand_{side}_{winding}", band_v, band_f, binding)
            author.skin_weights(band, body, rig)
            pieces.append(band)
    return pieces


def join_copies(name, objects, rig):
    copies = []
    for source in objects:
        copy = source.copy()
        copy.data = source.data.copy()
        bpy.context.collection.objects.link(copy)
        copy.hide_set(False)
        copy.hide_render = False
        freeze(copy, rig)
        copies.append(copy)
    exporter.active(copies[0])
    for obj in copies:
        obj.select_set(True)
    if len(copies) > 1:
        bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    exporter.normalize(obj, rig)
    return obj


def prepare_materials(objects):
    records = {}
    for obj in objects:
        for mat in obj.data.materials:
            if mat.name in records:
                continue
            shader = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
            record = {"base_color": list(shader.inputs["Base Color"].default_value),
                      "roughness": shader.inputs["Roughness"].default_value,
                      "metallic": shader.inputs["Metallic"].default_value,
                      "specular": shader.inputs["Specular IOR Level"].default_value,
                      "alpha_mask": bool(shader.inputs["Alpha"].is_linked),
                      "two_sided": True, "texture": None,
                      "texture_multiply": bool(mat.get("ModularTextureMultiply", False)),
                      "uv_scale": 8 if mat.get("ModularTextureMultiply", False) else 1}
            for node in mat.node_tree.nodes:
                if node.type != "TEX_IMAGE" or not node.image:
                    continue
                image = node.image
                require(len(image.pixels) > 0, f"Unreadable texture {mat.name}/{image.name}")
                require(image.has_data, f"Missing packed texture {mat.name}/{image.name}")
                filename = Path(bpy.path.abspath(image.filepath)).name
                dest = OUT / "Textures" / filename
                image.filepath_raw = str(dest)
                image.file_format = "PNG"
                image.save()
                record.update(texture=str(dest.relative_to(OUT)), sha256=digest(dest))
                break
            if mat.name in ("M_Heroine_MossLinen", "M_Heroine_ApronLinen"):
                record.update(role="garment_linen", parameter="ColorTint",
                              dye_multipliers=[[1,1,1,1], [1.1,0.17,0.56,1],
                                               [0.5,0.6,2.4,1], [2.75,1.78,2.61,1]])
            elif mat.name in ("M_Heroine_LinenTrim", "M_Heroine_ApronTrim"):
                record.update(role="garment_trim", parameter=None,
                              note="Retain authored neutral trim; never flat-tint all garment slots.")
            elif mat.name == "M_Heroine_Skin":
                record.update(role="skin", parameter="ColorTint")
            elif mat.name.startswith("M_Heroine_Hair_") or mat.name == "M_Heroine_Eyebrows":
                record.update(role="hair", parameter="ColorTint")
            elif mat.name == "M_Heroine_LightEyes":
                record.update(role="eyes", parameters=["IrisColor", "IrisMix"])
            else:
                record.update(role="fixed", parameter=None)
            records[mat.name] = record
    return records


def build(body_name):
    directory = OUT / body_name
    directory.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCES[body_name]), load_ui=False, use_scripts=False)
    rig = bpy.data.objects["Heroine_Rig"]
    require(rig.get("AdultPhenotypeAnchor") == 0.5, "Source adult anchor missing")
    exporter.reset(rig)
    signature = variants.rig_signature(rig)
    body, reconstruction = reconstruct_body(body_name, rig)
    wraps = footwraps(body, rig)
    coverage, straps = add_permanent_coverage(body, rig)
    groups = {
        "Tunic": [bpy.data.objects["Heroine_" + p] for p in TUNIC_PARTS],
        "Apron": [o for o in bpy.data.objects if o.name.startswith("Heroine_Apron")],
        "Shoes": [bpy.data.objects["Heroine_LaceupShoes"]],
        "Footwraps": wraps,
    }
    for style in HAIR:
        groups["Base_" + style] = [body] + straps + [bpy.data.objects["Heroine_Hair_" + style]] + [
            bpy.data.objects["Heroine_" + p] for p in FACE_PARTS]
    objects = set(o for group in groups.values() for o in group)
    materials = prepare_materials(objects)
    for obj in objects:
        freeze(obj, rig)
    for obj in list(bpy.data.objects):
        if obj not in objects and obj != rig:
            bpy.data.objects.remove(obj, do_unlink=True)
    for obj in objects:
        obj.hide_render = obj not in groups["Base_LongWave"]
        obj.hide_set(obj.hide_render)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.context.scene.render.fps = 30
    variants.compare_rigs(signature, variants.rig_signature(rig))
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(directory / f"{body_name}_Modular.blend"))
    exports = {}
    for key, group in groups.items():
        name = f"SK_Modular_{body_name}_{key}"
        obj = join_copies(name, group, rig)
        record = exporter.validate(obj, rig)
        record["materials"] = [m.name for m in obj.data.materials]
        record["fbx"] = str(Path(body_name) / (name + ".fbx"))
        record["unreal_object"] = f"/Game/SurvivalGame/Characters/ModularClothing/{body_name}/{name}.{name}"
        exporter.export_fbx(OUT / record["fbx"], rig, [obj])
        record["sha256"] = digest(OUT / record["fbx"])
        exports[key] = record
        bpy.data.objects.remove(obj, do_unlink=True)
    result = {"body_index": list(SOURCES).index(body_name), "source": str(SOURCES[body_name].relative_to(ROOT)),
              "source_sha256": digest(SOURCES[body_name]), "reconstruction": reconstruction,
              "coverage": coverage, "rig": signature, "exports": exports}
    write_json(directory / "materials.json", materials)
    write_json(directory / "source-report.json", result)
    print("MODULAR_BODY_EXPORTED", body_name, flush=True)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--body", choices=list(SOURCES))
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    require(bpy.app.version[:2] == (4, 5), "This pipeline requires Blender 4.5 LTS")
    (OUT / "Textures").mkdir(parents=True, exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    addon = enable_mpfb()
    bodies = [args.body] if args.body else list(SOURCES)
    for body in bodies:
        build(body)
    if not args.body:
        write_json(OUT / "manifest.json", {
            "schema": 1, "status": "source-exported-UE-validation-pending",
            "blender": bpy.app.version_string, "mpfb": "2.0.17",
            "mpfb_base_sha256": digest(addon / "data" / "3dobjs" / "base.obj"),
            "rig": "MakeHuman game_engine; original unmodified 53-bone authoring bind",
            "unreal_skeleton": "/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton",
            "coordinate_contract": {"blender": "metres, +Z up, -Y forward",
                                    "fbx": "FBX_SCALE_UNITS; importer converts to centimetres"},
            "body_indices": dict(enumerate(SOURCES)), "hair_indices": dict(enumerate(HAIR)),
            "dyes": ["Moss", "Wine", "Slate", "Flax"], "garments": GARMENTS,
            "bodies": {name: json.loads((OUT/name/"source-report.json").read_text()) for name in SOURCES},
            "engine_imported": False, "gameplay_validated": False,
        })


if __name__ == "__main__":
    main()
