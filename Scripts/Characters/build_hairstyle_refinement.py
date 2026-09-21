"""Reconstructible hair-only edits of admitted joined FBXs; offline CPU only."""
import argparse
import hashlib
import importlib.util
import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from bpy_extras import image_utils
from mathutils import Vector

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "HairstyleRefinement"
PREVIEW = ROOT / "Build" / "CharacterPreview" / "HairstyleRefinement"
CHAR = ROOT / "Assets" / "Characters"
DEST = "/Game/SurvivalGame/Characters/Heroine/"


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(filename))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


exporter = module("hairstyle_export", "export_heroine.py")
author = module("hairstyle_stage", "build_heroine.py")
is_receipt_file = module("hairstyle_receipts", "hairstyle_receipts.py").is_receipt_file
author.OUT = exporter.OUT = OUT
author.PREVIEW = exporter.PREVIEW = PREVIEW


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_path(body, style, apron):
    name = "SK_Heroine_" + (body + "_" if body != "Preferred" else "") + style
    name += "_Apron" if apron else ""
    folder = CHAR / "BodyPresets" / body if body != "Preferred" else CHAR / ("Variants" if apron else "Heroine")
    return folder / (name + ".fbx")


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # Legacy FBXs contain historical absolute paths. Resolve only admitted
    # textures in this worktree, never the original checkout or an image search.
    original_loader = image_utils.load_image
    def local_image(imagepath, dirname="", *args, **kwargs):
        filename = Path(imagepath).name
        body = next((b for b in ["Willow","Hazel"] if b in path.stem), None)
        candidates = [OUT/"Textures"/filename]
        if path.stem.startswith("SK_Modular_"):
            modular_body = next(b for b in ["Preferred","Willow","Hazel"] if b in path.stem)
            candidates.append(CHAR/"ModularClothing"/modular_body/"Textures"/filename)
        if body:
            candidates.append(CHAR/"BodyPresets"/body/"Textures"/filename)
        candidates += [CHAR/"Heroine"/"Textures"/filename, CHAR/"Variants"/"Textures"/filename]
        local = next((p for p in candidates if p.is_file()), None)
        if local is None:
            raise RuntimeError("Unadmitted FBX texture: "+filename)
        kwargs["recursive"] = False
        return original_loader(str(local), "", *args, **kwargs)
    image_utils.load_image = local_image
    try:
        bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False, use_image_search=False)
    finally:
        image_utils.load_image = original_loader
    for image in bpy.data.images:
        if image.source == "FILE":
            assert Path(bpy.path.abspath(image.filepath)).resolve().is_relative_to(ROOT), image.filepath
    bpy.context.preferences.filepaths.save_version = 0
    rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")
    exporter.reset(rig)
    return rig, mesh


def rig_record(rig):
    return {
        "matrix": [list(r) for r in rig.matrix_world],
        "bones": {b.name: {"parent": b.parent.name if b.parent else None,
                          "matrix": [list(r) for r in b.matrix_local]} for b in rig.data.bones},
    }


def assert_rig(a, b):
    assert a["bones"].keys() == b["bones"].keys()
    errors = [abs(x-y) for ra, rb in zip(a["matrix"], b["matrix"]) for x, y in zip(ra, rb)]
    for name, bone in a["bones"].items():
        other = b["bones"][name]
        assert bone["parent"] == other["parent"]
        errors += [abs(x-y) for ra, rb in zip(bone["matrix"], other["matrix"]) for x, y in zip(ra, rb)]
    error = max(errors)
    # FBX's Euler decomposition introduces ~1.5e-5 rotation-matrix noise.
    assert error < 0.00002, error
    return error


def surface_record(obj, hair=False):
    groups = {g.index: g.name for g in obj.vertex_groups}
    records = []
    for p in obj.data.polygons:
        mat = obj.data.materials[p.material_index].name
        if mat.startswith("M_Heroine_Hair_") != hair:
            continue
        corners = []
        for loop in p.loop_indices:
            v = obj.data.vertices[obj.data.loops[loop].vertex_index]
            corners.append({
                "co": list(obj.matrix_world @ v.co),
                "normal": list(obj.data.corner_normals[loop].vector),
                "weights": sorted((groups[g.group], g.weight) for g in v.groups),
                "uv": [list(layer.data[loop].uv) for layer in obj.data.uv_layers],
            })
        records.append({"material": mat, "corners": corners})
    return records


def nonhair_record(obj):
    return surface_record(obj)


def assert_surfaces(before, after, tolerance):
    assert len(before) == len(after), (len(before), len(after))
    error = 0.0
    for a, b in zip(before, after):
        assert a["material"] == b["material"]
        assert len(a["corners"]) == len(b["corners"])
        for x, y in zip(a["corners"], b["corners"]):
            assert [w[0] for w in x["weights"]] == [w[0] for w in y["weights"]]
            av = x["co"] + [w[1] for w in x["weights"]] + [v for uv in x["uv"] for v in uv]
            bv = y["co"] + [w[1] for w in y["weights"]] + [v for uv in y["uv"] for v in uv]
            assert len(av) == len(bv)
            error = max(error, max(abs(v-w) for v, w in zip(av, bv)))
    assert error <= tolerance, error
    return error


def restore_nonhair_normals(obj, records):
    normals = [tuple(n.vector) for n in obj.data.corner_normals]
    polygons = [p for p in obj.data.polygons
                if not obj.data.materials[p.material_index].name.startswith("M_Heroine_Hair_")]
    for poly, record in zip(polygons, records):
        for loop, corner in zip(poly.loop_indices, record["corners"]):
            normals[loop] = corner["normal"]
    obj.data.normals_split_custom_set(normals)


def assert_normals(before, after):
    error = max(abs(x-y) for a,b in zip(before,after)
                for c,d in zip(a["corners"],b["corners"])
                for x,y in zip(c["normal"],d["normal"]))
    assert error < .0001, error
    return error


def hair_faces(obj):
    return [p for p in obj.data.polygons if obj.data.materials[p.material_index].name.startswith("M_Heroine_Hair_")]


def endpoint(x, y):
    # Retain every upstream wave; remove the old ends instead of compressing them.
    # Rounded V-shaped lobes produce staggered, narrowing locks rather than a shelf.
    lobe = abs(math.sin(87.0*x + 9.0*y + .3*math.sin(31*x)))
    return 1.103 + 0.045 * (abs(x)/0.155)**1.2 + 0.044*lobe


def cut_waves(obj):
    faces = hair_faces(obj)
    uv = obj.data.uv_layers.active
    vertices, polygons, uvs = [], [], []
    original_upper = set()
    for p in faces:
        corners = [(obj.data.vertices[obj.data.loops[i].vertex_index].co.copy(), uv.data[i].uv.copy())
                   for i in p.loop_indices]
        for co, _ in corners:
            if co.z >= 1.20:
                original_upper.add(tuple(co))
        clipped = []
        for i, (co, tex) in enumerate(corners):
            prev, pt = corners[i-1]
            inside = co.z >= endpoint(co.x, co.y)
            pin = prev.z >= endpoint(prev.x, prev.y)
            if inside != pin:
                low, high = 0.0, 1.0
                for _ in range(24):
                    t = (low+high)/2
                    q = prev.lerp(co, t)
                    if (q.z >= endpoint(q.x, q.y)) == pin:
                        low = t
                    else:
                        high = t
                t = (low+high)/2
                clipped.append((prev.lerp(co, t), pt.lerp(tex, t)))
            if inside:
                clipped.append((co, tex))
        if len(clipped) >= 3:
            polygons.append(tuple(range(len(vertices), len(vertices)+len(clipped))))
            vertices.extend(c for c, _ in clipped)
            for co, tex in clipped:
                if co.z < 1.20:
                    end = endpoint(co.x, co.y)
                    blend = max(0, min(1, (1.20-co.z)/(1.20-end)))
                    blend = blend*blend*(3-2*blend)
                    # The admitted atlas has long locks on its left and shorter
                    # locks on its right. Reuse their real feathered alpha tips.
                    # Only this new end band changes hair UVs, never geometry.
                    tip_v = .282 if tex.x > .617 else .006
                    tex.y = tex.y*(1-blend)+tip_v*blend
                uvs.append(tex)
    mesh = bpy.data.meshes.new("RetainedCadenceMidback")
    mesh.from_pydata(vertices, [], polygons)
    layer = mesh.uv_layers.new(name=obj.data.uv_layers.active.name)
    for loop, tex in zip(layer.data, uvs):
        loop.uv = tex
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-8)
    bm.to_mesh(mesh)
    bm.free()
    result = bpy.data.objects.new("AuthoredMidbackWaves", mesh)
    bpy.context.collection.objects.link(result)
    new_upper = {tuple(v.co) for v in mesh.vertices if v.co.z >= 1.20}
    assert original_upper == new_upper
    return result, {"upper_crown_threshold_m": 1.20, "upper_vertices_unchanged": len(original_upper),
                    "old_min_z_m": min(obj.data.vertices[v].co.z for p in faces for v in p.vertices),
                    "new_min_z_m": min(v.co.z for v in mesh.vertices),
                    "end_alpha": "lower-than-120cm UV end band mapped to admitted atlas feather tips; original alpha unchanged",
                    "method": "clip existing evaluated cards at staggered tapered-lock contour; no coordinate compression"}


def bob(obj):
    # Original symmetric chin/nape cut: smooth crown, straight panels, short open fringe.
    # Uses the admitted body's head fit; no sampled third-party character geometry.
    old = [obj.data.vertices[v].co for p in hair_faces(obj) for v in p.vertices]
    top = max(v.z for v in old)
    width = (max(v.x for v in old)-min(v.x for v in old))/2
    rx, ry, cy = max(0.100, width*0.97), 0.112, -0.035
    columns, rows = 128, 32
    vertices, faces, uvvalues = [], [], []
    for row in range(rows+1):
        t = row/rows
        for col in range(columns+1):
            theta = 2*math.pi*col/columns
            front = math.cos(theta) < -0.70
            bottom = top-(0.084 if front else 0.216)
            # All angles share one rounded crown; only the straight panels
            # change length at the open fringe, not the skull curvature.
            crown = min(t/0.55, 1)
            angle = crown*1.40
            radius = math.sin(angle)
            crown_z = top-.095*(1-math.cos(angle))
            panel = max(0, (t-.55)/.45)
            z = crown_z*(1-panel)+bottom*panel
            inset = max(0.0, (t-0.86)/0.14)*0.003
            edge = 0.002*math.cos(theta*19) * max(0, (t-.85)/.15)
            vertices.append(((rx*radius-inset)*math.sin(theta),
                             cy+(ry*radius-inset)*math.cos(theta),
                             z+edge))
    for row in range(rows):
        for col in range(columns):
            a = row*(columns+1)+col
            faces.append((a, a+1, a+columns+2, a+columns+1))
            uvvalues.extend([(col/columns,row/rows),((col+1)/columns,row/rows),
                             ((col+1)/columns,(row+1)/rows),(col/columns,(row+1)/rows)])
    mesh = bpy.data.meshes.new("OriginalStraightBob")
    mesh.from_pydata(vertices, [], faces)
    layer = mesh.uv_layers.new(name=obj.data.uv_layers.active.name)
    for loop, tex in zip(layer.data, uvvalues):
        loop.uv = tex
    result = bpy.data.objects.new("AuthoredStraightBob", mesh)
    bpy.context.collection.objects.link(result)
    return result, {"method": "original symmetric parameterized crown, vertical chin/nape panels and short fringe",
                    "old_min_z_m": min(v.z for v in old), "new_min_z_m": min(v[2] for v in vertices),
                    "reference_status": "written direction only; not reference-approved"}


def neutral_material(style):
    path = OUT / "Textures" / ("T_" + style + "_Neutral.png")
    if style == "LongWave":
        source = CHAR / "Heroine" / "Textures" / "T_Long_Chestnut.png"
        image = bpy.data.images.load(str(source), check_existing=False)
        pixels = np.asarray(image.pixels[:], dtype=np.float32).reshape((-1, 4))
        # Image.pixels exposes the PNG's sRGB samples here, not shader-linear RGB.
        # Recover modulation in linear space, then encode a neutral sRGB PNG.
        red = np.clip(pixels[:, 0], 0, 1)
        linear = np.where(red <= .04045, red/12.92, ((red+.055)/1.055)**2.4)
        shade = np.clip(linear/(((.26+.055)/1.055)**2.4), 0, 1)
        srgb = np.where(shade <= .0031308, shade*12.92, 1.055*shade**(1/2.4)-.055)
        assert float(srgb.max()-srgb.min()) > .4
        pixels[:, :3] = srgb[:, None]
        w, h = image.size
    else:
        w, h = 1024, 512
        x = np.arange(w)[None, :]/w
        y = np.arange(h)[:, None]/h
        shade = .73+.035*np.cos(2*np.pi*x*193)+.018*np.cos(2*np.pi*x*307)+.045*np.cos(2*np.pi*x*21)
        shade = np.broadcast_to(shade, (h,w))
        pixels = np.ones((h,w,4),dtype=np.float32)
        pixels[:,:,:3] = shade[:,:,None]
        pixels = pixels.reshape((-1,4))
    output = bpy.data.images.new("T_"+style+"_Neutral", width=w, height=h, alpha=True)
    output.pixels.foreach_set(pixels.ravel())
    output.filepath_raw = str(path)
    output.file_format = "PNG"
    output.save()
    name = "M_Heroine_Hair_"+("long01" if style == "LongWave" else "bob01")+"_Neutral"
    return author.material(name, (1,1,1), .70, path, alpha=True)


def replace(obj, hair, rig, material):
    slots = [i for i,m in enumerate(obj.data.materials) if m.name.startswith("M_Heroine_Hair_")]
    assert len(slots) == 1
    slot = slots[0]
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    remove = [f for f in bm.faces if f.material_index == slot]
    bmesh.ops.delete(bm, geom=remove, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.materials[slot] = material
    hair.data.materials.append(material)
    author.bind_bone(hair, rig, "head")
    for face in hair.data.polygons:
        face.use_smooth = True
    exporter.active(obj)
    hair.select_set(True)
    bpy.ops.object.join()
    return slot


def restore_preview_materials(obj, style):
    records = {}
    for path in [CHAR/"Heroine"/"materials.json", CHAR/"Variants"/"materials.json"]:
        if path.exists():
            records.update(json.loads(path.read_text()))
    for mat in obj.data.materials:
        shader = mat.node_tree.nodes.get("Principled BSDF")
        if mat.name.endswith("_Neutral"):
            color = (.055,.011,.003) if style == "LongWave" else (.68,.46,.19)
            tex = next(n for n in mat.node_tree.nodes if n.type == "TEX_IMAGE")
            multiply = mat.node_tree.nodes.new("ShaderNodeMixRGB")
            multiply.blend_type = "MULTIPLY"
            multiply.inputs[0].default_value = 1
            multiply.inputs[2].default_value = (*color,1)
            mat.node_tree.links.new(tex.outputs["Color"],multiply.inputs[1])
            mat.node_tree.links.new(multiply.outputs[0],shader.inputs["Base Color"])
        elif mat.name in records:
            rec = records[mat.name]
            shader.inputs["Base Color"].default_value = rec["base_color"]
            shader.inputs["Roughness"].default_value = rec["roughness"]
            shader.inputs["Metallic"].default_value = rec["metallic"]
            if rec.get("texture"):
                candidates = [CHAR/"Heroine"/rec["texture"],CHAR/"Variants"/rec["texture"]]
                path = next((p for p in candidates if p.exists()), None)
                if path:
                    node = mat.node_tree.nodes.new("ShaderNodeTexImage")
                    node.image = bpy.data.images.load(str(path),check_existing=True)
                    mat.node_tree.links.new(node.outputs["Color"],shader.inputs["Base Color"])
                    if rec.get("alpha_mask"):
                        mat.node_tree.links.new(node.outputs["Alpha"],shader.inputs["Alpha"])


def preview(obj, rig, style, body):
    restore_preview_materials(obj, style)
    cam = author.stage()
    scene = bpy.context.scene
    scene.cycles.device = "CPU"
    scene.cycles.samples = 8
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 2
    scene.render.resolution_x = 480
    scene.render.resolution_y = 600
    for label, loc in [("back",(0,4,1.5)),("threequarter",(2,3,1.65)),("front",(0,-4,1.5))]:
        author.render(cam, body+"-"+style+"-"+label, loc, (0,0,1.36), .70)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--style", choices=["LongWave","Bob"], required=True)
    parser.add_argument("--preview", action="store_true")
    parser.add_argument("--body", choices=["Preferred","Willow","Hazel"])
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    if args.style == "Bob":
        raise RuntimeError("Parametric Bob authoring is superseded. Use Build-HairstyleRefinement.ps1 -Style Bob for the stock-bob final recipe.")
    (OUT/"Textures").mkdir(parents=True, exist_ok=True)
    (OUT/"Joined").mkdir(exist_ok=True)
    PREVIEW.mkdir(parents=True, exist_ok=True)
    protected = {str(p.relative_to(ROOT)):sha(p) for folder in ["Heroine","Variants","BodyPresets"]
                 for p in (CHAR/folder).rglob("*") if is_receipt_file(p)}
    manifest = {"status":"interchange-validated candidate; source visual and Unreal acceptance pending", "style":args.style,
                "rig":"original game_engine 53 bones; existing UE skeleton adds wrapper",
                "palette":"neutral sRGB texture multiplied by HomesteadLook::NeutralHairTint; legacy eyebrows/ponytail use HairTint",
                "entries":[], "protected_inputs":protected}
    for body in ["Preferred","Willow","Hazel"]:
        if args.body and args.body != body:
            continue
        for apron in [False, True]:
            source = source_path(body,args.style,apron)
            rig, obj = load(source)
            before = nonhair_record(obj)
            bind = rig_record(rig)
            old_slots = [m.name for m in obj.data.materials]
            hair, check = cut_waves(obj) if args.style == "LongWave" else bob(obj)
            slot = replace(obj,hair,rig,neutral_material(args.style))
            restore_nonhair_normals(obj, before)
            assert_surfaces(before, nonhair_record(obj), 0)
            assert assert_rig(bind, rig_record(rig)) == 0
            name = source.stem
            obj.name = name
            output = OUT/"Joined"/source.name
            exporter.validate(obj,rig)
            exporter.export_fbx(output,rig,[obj])
            bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"Joined"/(name+".blend")))
            rig, obj = load(output)
            error = assert_surfaces(before, nonhair_record(obj), 0.00001)
            normalerror = assert_normals(before, nonhair_record(obj))
            rigerror = assert_rig(bind,rig_record(rig))
            check.update({"nonhair_faces":len(before),"nonhair_preexport_max_error":0,
                          "nonhair_roundtrip_max_error":error,"bind_roundtrip_max_error":rigerror,
                          "nonhair_normal_roundtrip_max_error":normalerror,
                          "roundtrip":exporter.validate(obj,rig)})
            for i,mat in enumerate(obj.data.materials):
                if i != slot:
                    assert mat.name == old_slots[i]
            check["material_slot"] = slot
            manifest["entries"].append({"body":body,"outfit":"Apron" if apron else "Tunic",
                "source":str(source.relative_to(ROOT)), "fbx":str(output.relative_to(OUT)),
                "sha256":sha(output),"object_path":DEST+name+"."+name,
                "hair_material":obj.data.materials[slot].name,"checks":check})
            (OUT/(args.style+"-manifest.json")).write_text(json.dumps(manifest,indent=2))
            if args.preview and not apron:
                preview(obj,rig,args.style,body)
            print("HAIRSTYLE_EXPORTED",name, json.dumps(check), flush=True)
    for path, digest in protected.items():
        assert sha(ROOT/path) == digest, "Protected input changed: "+path
    matname = "M_Heroine_Hair_"+("long01" if args.style=="LongWave" else "bob01")+"_Neutral"
    texture = OUT/"Textures"/("T_"+args.style+"_Neutral.png")
    (OUT/(args.style+"-materials.json")).write_text(json.dumps({matname:{
        "base_color":[1,1,1,1],"roughness":.70,"metallic":0,"specular":.5,
        "two_sided":True,"alpha_mask":True,"texture":str(texture.relative_to(OUT)),
        "sha256":sha(texture),"sRGB":True,"runtime_tint":"HomesteadLook::NeutralHairTint",
        "note":"Never assign neutral tint to legacy eyebrows or ponytail."}},indent=2))
    print("HAIRSTYLE_SOURCE_VALIDATED",args.style,flush=True)


if __name__ == "__main__":
    main()
