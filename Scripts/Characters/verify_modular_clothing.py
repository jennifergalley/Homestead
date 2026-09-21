"""Focused Blender/FBX checks and optional CPU contact sheets; never launches UE."""
import argparse
import json
import math
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).parent))
import build_modular_clothing as modular

ROOT, OUT, PREVIEW = modular.ROOT, modular.OUT, modular.PREVIEW
require = modular.require
CLIPS = {
    "Idle": "Locomotion/AN_Heroine_RelaxedIdle.fbx",
    "Walk": "Locomotion/AN_Heroine_GroundedWalk.fbx",
    "GatherWeed": "Gathering/AN_Heroine_Gather.fbx",
    "Water": "Watering/AN_Heroine_Water.fbx",
    "Clear": "Clearing/AN_Heroine_Clear.fbx",
}


def rig_signature(rig):
    return modular.variants.rig_signature(rig)


def import_mesh(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    added = set(bpy.data.objects) - before
    rigs = [o for o in added if o.type == "ARMATURE"]
    meshes = [o for o in added if o.type == "MESH"]
    require(len(rigs) == len(meshes) == 1, f"{path}: expected one mesh and one rig")
    return rigs[0], meshes[0]


def bind_clip(rig, key):
    before = set(bpy.data.objects)
    path = ROOT / "Assets" / "Characters" / "Heroine" / CLIPS[key]
    bpy.ops.import_scene.fbx(filepath=str(path))
    added = set(bpy.data.objects) - before
    source = next(o for o in added if o.type == "ARMATURE")
    error = modular.variants.compare_rigs(rig_signature(rig), rig_signature(source))
    action = source.animation_data.action
    rig.animation_data_create()
    rig.animation_data.action = action
    rig.animation_data.action_slot = source.animation_data.action_slot
    for obj in added:
        bpy.data.objects.remove(obj, do_unlink=True)
    return action, error


def material_restore(mesh, records):
    for i, old in enumerate(mesh.data.materials):
        record = records[old.name]
        name = old.name
        # Remove unused FBX material so slot names remain the contract names.
        mat = modular.author.material(
            name + "_Review", record["base_color"][:3], record["roughness"],
            OUT / record["texture"] if record["texture"] else None, record["alpha_mask"])
        shader = mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Metallic"].default_value = record["metallic"]
        shader.inputs["Specular IOR Level"].default_value = record["specular"]
        if record["texture_multiply"]:
            texture = next(n for n in mat.node_tree.nodes if n.type == "TEX_IMAGE")
            coordinates = mat.node_tree.nodes.new("ShaderNodeTexCoord")
            scale = mat.node_tree.nodes.new("ShaderNodeVectorMath")
            scale.operation = "SCALE"
            scale.inputs[3].default_value = record["uv_scale"]
            mat.node_tree.links.new(coordinates.outputs["UV"], scale.inputs[0])
            mat.node_tree.links.new(scale.outputs[0], texture.inputs["Vector"])
            multiply = mat.node_tree.nodes.new("ShaderNodeMixRGB")
            multiply.blend_type = "MULTIPLY"
            multiply.inputs[0].default_value = 1
            multiply.inputs[2].default_value = record["base_color"]
            mat.node_tree.links.new(texture.outputs["Color"], multiply.inputs[1])
            mat.node_tree.links.new(multiply.outputs[0], shader.inputs["Base Color"])
        mesh.data.materials[i] = mat


def mesh_points(mesh):
    evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    return [evaluated.matrix_world @ v.co for v in evaluated.data.vertices]


def bounds(points):
    return [[min(p[i] for p in points), max(p[i] for p in points)] for i in range(3)]


def base_coverage(mesh):
    cloth_names = ("M_Modular_BaseBra", "M_Modular_BaseBriefs")
    cloth = {name: {i for p in mesh.data.polygons if mesh.data.materials[p.material_index].name == name
                    for i in p.vertices} for name in cloth_names}
    require(all(len(indices) > 100 for indices in cloth.values()), "Missing separate bra or briefs")
    feet = [sum(v.co.z < 0.10 and v.co.x*side > 0 for v in mesh.data.vertices)
            for side in (-1, 1)]
    require(min(feet) > 100, "One or both complete feet missing")
    for face in mesh.data.polygons:
        mat = mesh.data.materials[face.material_index]
        if mat.name != "M_Heroine_Skin":
            continue
        co = sum((mesh.data.vertices[i].co for i in face.vertices), Vector()) / len(face.vertices)
        protected = ((1.085 < co.z < 1.195 and abs(co.x) < 0.16)
                     or (0.745 + 0.38*abs(co.x) < co.z < 0.905 and abs(co.x) < 0.18))
        require(not protected, f"Skin surface exposed inside permanent bra/brief coverage: {face.index}")
    waist = sum(p.material_index == 0 and 0.94 < p.center.z < 1.04 for p in mesh.data.polygons)
    require(waist > 50, "Base is still a one-piece undergarment instead of separate bra and briefs")
    return {"permanent_vertices": {name: len(indices) for name,indices in cloth.items()},
            "exposed_waist_faces": waist, "complete_feet_vertices_per_side": feet,
            "protected_region_skin_faces": 0}


def validate_round_trips(body_name):
    source = json.loads((OUT / body_name / "source-report.json").read_text())
    records = json.loads((OUT / body_name / "materials.json").read_text())
    bpy.ops.wm.read_factory_settings(use_empty=True)
    reference, _ = import_mesh(ROOT / "Assets" / "Characters" / "Heroine" / "SK_Heroine_LongWave.fbx")
    reference_signature = rig_signature(reference)
    results = {}
    for key, expected in source["exports"].items():
        bpy.ops.wm.read_factory_settings(use_empty=True)
        rig, mesh = import_mesh(OUT / expected["fbx"])
        record = modular.exporter.validate(mesh, rig)
        record["maximum_bind_error"] = modular.variants.compare_rigs(reference_signature, rig_signature(rig))
        require(record["triangles"] == expected["triangles"], f"{key}: FBX changed triangle count")
        require(record["bones"] == expected["bones"], f"{key}: FBX changed bone count")
        names = [m.name for m in mesh.data.materials]
        require(names == expected["materials"], f"{key}: material slots changed in FBX")
        require(all(name in records for name in names), f"{key}: unmapped material")
        require(all(p.material_index < len(names) for p in mesh.data.polygons), "Invalid material index")
        require(any(m.type == "ARMATURE" and m.object == rig for m in mesh.modifiers), "Lost skin binding")
        require(all(math.isfinite(c) for v in mesh.data.vertices for c in v.co), "Nonfinite mesh coordinates")
        record["bounds_m"] = bounds(mesh_points(mesh))
        if key.startswith("Base_"):
            height = record["bounds_m"][2][1] - record["bounds_m"][2][0]
            require(1.55 < height < 1.75, "Base FBX changed scale")
            record["coverage"] = base_coverage(mesh)
        else:
            require(record["bounds_m"][2][1] < 1.4, "Garment FBX changed units")
        record["materials"] = names
        record["sha256"] = modular.digest(OUT / expected["fbx"])
        results[key] = record
    return results


def load_ensemble(body_name):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30
    source = json.loads((OUT / body_name / "source-report.json").read_text())
    records = json.loads((OUT / body_name / "materials.json").read_text())
    rig, base = import_mesh(OUT / source["exports"]["Base_Bob"]["fbx"])
    parts = {"Base": base}
    for key in modular.GARMENTS:
        child_rig, mesh = import_mesh(OUT / source["exports"][key]["fbx"])
        modular.variants.compare_rigs(rig_signature(rig), rig_signature(child_rig))
        matrix = mesh.matrix_world.copy()
        mesh.parent = rig
        mesh.matrix_world = matrix
        for modifier in mesh.modifiers:
            if modifier.type == "ARMATURE":
                modifier.object = rig
        bpy.data.objects.remove(child_rig, do_unlink=True)
        parts[key] = mesh
    return rig, parts, records


def clearance(under, outer, material_name=None):
    points = mesh_points(outer)
    evaluated = outer.evaluated_get(bpy.context.evaluated_depsgraph_get())
    polygons = [p.vertices[:] for p in evaluated.data.polygons
                if material_name is None or outer.data.materials[p.material_index].name == material_name]
    tree = BVHTree.FromPolygons(points, polygons)
    distances = []
    for point in mesh_points(under):
        hit, normal, _, distance = tree.find_nearest(point)
        if hit is not None and distance < 0.025:
            distances.append((point-hit).dot(normal))
    return {"near_samples": len(distances),
            "maximum_outside_nearest_surface_m": max(distances, default=0),
            "outside_over_3mm": sum(d > 0.003 for d in distances),
            "note": "Nearest-surface diagnostic, not a full collision proof; open edges and double shells can affect signs."}


def verify_motion(body_name, render=False):
    rig, parts, records = load_ensemble(body_name)
    results = {}
    for key in CLIPS:
        action, error = bind_clip(rig, key)
        samples = []
        start, end = action.frame_range
        reference = None
        maximum_motion = 0
        for fraction in (0, 0.25, 0.5, 0.75, 1):
            frame = round(start+(end-start)*fraction)
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            current = {name: mesh_points(mesh) for name, mesh in parts.items()}
            if reference is None:
                reference = current
            maximum_motion = max(maximum_motion, max((p-q).length for name in current
                                 for p, q in zip(reference[name], current[name])))
            for name, points in current.items():
                require(all(math.isfinite(c) for p in points for c in p), f"{key}/{name}: invalid deformation")
                box = bounds(points)
                require(all(b-a < 2.5 for a,b in box), f"{key}/{name}: exploded deformation")
            samples.append({"frame": frame, "bounds_m": {name: bounds(p) for name,p in current.items()},
                            "body_tunic": clearance(parts["Base"], parts["Tunic"], "M_Heroine_MossLinen"),
                            "body_shoes": clearance(parts["Base"], parts["Shoes"]),
                            "body_footwraps": clearance(parts["Base"], parts["Footwraps"])})
        require(maximum_motion > 0.00001, f"{key}: animation did not deform components")
        results[key] = {"maximum_bind_error": error, "maximum_vertex_motion_m": maximum_motion,
                        "samples": samples}
    if render:
        render_sheet(body_name, rig, parts, records)
    return results


def render_sheet(body_name, rig, parts, records):
    for mesh in parts.values():
        material_restore(mesh, records)
    modular.author.PREVIEW = PREVIEW
    camera = modular.author.stage()
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 8
    scene.cycles.use_denoising = True
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 2
    width, height = 360, 540
    scene.render.resolution_x, scene.render.resolution_y = width, height
    sheet = np.zeros((height*2, width*3, 4), dtype=np.float32)
    states = [
        ("base-front", "Idle", 0, set(), (0,-4,1.0)),
        ("base-back", "Idle", 0, set(), (0,4,1.0)),
        ("footwraps", "Idle", 0, {"Footwraps"}, (1.6,-4,1.0)),
        ("tunic-shoes", "Idle", 0, {"Tunic","Shoes"}, (1.6,-4,1.0)),
        ("apron-walk", "Walk", 0.25, {"Tunic","Apron","Footwraps"}, (1.6,-4,1.0)),
        ("apron-gather", "GatherWeed", 0.5, {"Tunic","Apron","Shoes"}, (1.6,-4,1.0)),
    ]
    for index, (label, clip, fraction, visible, location) in enumerate(states):
        action, _ = bind_clip(rig, clip)
        scene.frame_set(round(action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])*fraction))
        for name, mesh in parts.items():
            mesh.hide_render = name != "Base" and name not in visible
        camera.location = location
        modular.author.aim(camera, (0,0,0.85))
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 1.80
        image_path = PREVIEW / f"{body_name}-{label}.png"
        scene.render.filepath = str(image_path)
        bpy.ops.render.render(write_still=True)
        image = bpy.data.images.load(str(image_path), check_existing=False)
        pixels = np.array(image.pixels[:], dtype=np.float32).reshape((height,width,4))
        row, col = divmod(index,3)
        sheet[(1-row)*height:(2-row)*height,col*width:(col+1)*width] = pixels
        bpy.data.images.remove(image)
    result = bpy.data.images.new(body_name+" modular contact sheet", width=width*3, height=height*2, alpha=True)
    result.pixels.foreach_set(sheet.ravel())
    result.filepath_raw = str(PREVIEW / f"{body_name}-contact-sheet.png")
    result.file_format = "PNG"
    result.save()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--body", choices=list(modular.SOURCES))
    parser.add_argument("--render", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    PREVIEW.mkdir(parents=True, exist_ok=True)
    result = {"schema": 1, "engine_imported": False, "gameplay_validated": False,
              "limits": ["Sampled CPU source/FBX checks only; no Unreal import, ordinary-play footage or art acceptance.",
                         "Nearest-surface diagnostics are not full triangle collision checks."], "bodies": {}}
    for name in [args.body] if args.body else modular.SOURCES:
        result["bodies"][name] = {"round_trips": validate_round_trips(name),
                                  "motion": verify_motion(name, args.render)}
        print("MODULAR_SOURCE_VERIFIED", name, flush=True)
    modular.write_json(OUT / ("validation-"+args.body+".json" if args.body else "validation.json"), result)


if __name__ == "__main__":
    main()
