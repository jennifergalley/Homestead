"""Crop stage sheet: plots at chosen growth values, with their produce, from the gameplay camera.

    blender --background Assets/Props/CropWheat/CropWheat.blend --python Scripts/Blender/render_crop_sheet.py
        -- [--growth 0.25 0.5 0.75 1.0] [--light day|dusk|rain] [--distance 7] [--samples 48]
           [--width 1920 --height 1080] [--extra path/to/Other.blend:SM_Name] [--out sheet.png]

A quick offline check of what a crop plot looks like in the garden before importing it: each plot
shows the stage mesh for its growth (Homestead::StageOf) on the tilled bed, with the produce
instanced on the report anchors, scaled, pushed up and tinted exactly as
AHomesteadWorld::AddCropProduce does (LOOKS mirrors HomesteadWorld.cpp's Looks[]; UNRIPE mirrors
Content/Python/homestead_agent/crop_produce.py). ``--extra`` adds another mesh as a last plot (for
example a withered set). Writes sheet_<light>.png beside the report unless --out is given.
"""
import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Euler, Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_kit as kit  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
# Visual: (Appear, RiseCm, MinScale, MinLength, ColourPower, Boost) - keep in step with Looks[].
LOOKS = {
    "CropTurnip": (0.30, 3.0, 0.50, 0.50, 1.0, 1.3),
    "CropCarrot": (0.30, 3.0, 0.50, 0.50, 1.0, 1.9),
    "CropPotato": (0.30, 3.0, 0.45, 0.45, 1.2, 2.3),
    "CropCabbage": (0.30, 0.0, 0.25, 0.25, 1.2, 1.0),
    "CropBroadBean": (0.50, 0.0, 0.45, 0.30, 1.3, 1.8),
    "CropStrawberry": (0.55, 0.0, 0.35, 0.35, 1.8, 2.0),
    "CropPea": (0.52, 0.0, 0.40, 0.30, 1.5, 1.8),
    "CropWheat": (0.55, 0.0, 0.55, 0.45, 2.5, 1.35),
    "CropBarley": (0.55, 0.0, 0.55, 0.45, 2.5, 1.4),
    "CropLeek": (0.30, 4.0, 0.35, 0.45, 1.2, 1.25),
    "CropWinterBroccoli": (0.55, 0.0, 0.25, 0.25, 1.4, 1.5),
}
UNRIPE = {
    "CropTurnip": (0.46, 0.55, 0.30), "CropCarrot": (0.34, 0.46, 0.16), "CropPotato": (0.42, 0.44, 0.26),
    "CropCabbage": (0.30, 0.48, 0.16), "CropBroadBean": (0.36, 0.52, 0.20), "CropStrawberry": (0.58, 0.64, 0.36),
    "CropPea": (0.42, 0.55, 0.26), "CropWheat": (0.20, 0.34, 0.12), "CropBarley": (0.22, 0.36, 0.13),
    "CropLeek": (0.40, 0.52, 0.28), "CropWinterBroccoli": (0.38, 0.50, 0.24),
}
STAGE_NAMES = ("Sprout", "Young", "Growing", "Mature", "Ripe")


def stage_of(growth):
    if growth >= 1.0:
        return "Ripe"
    if growth < 0.06:
        return None
    for limit, name in ((0.30, "Sprout"), (0.55, "Young"), (0.80, "Growing")):
        if growth < limit:
            return name
    return "Mature"


def _ripeness_material(source, tint):
    """The produce material with M_CropProduce's unripe lerp driven by the object colour (R)."""
    mat = source.copy()
    mat.name = source.name + "_Sheet"
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    base_link = next((l for l in links if l.to_socket == bsdf.inputs["Base Color"]), None)
    if base_link is None:
        return mat
    base = base_link.from_socket
    info = nodes.new("ShaderNodeObjectInfo")
    sep = nodes.new("ShaderNodeSeparateColor")
    links.new(info.outputs["Color"], sep.inputs["Color"])
    lum = nodes.new("ShaderNodeRGBToBW")
    links.new(base, lum.inputs["Color"])
    lift = nodes.new("ShaderNodeMath")
    lift.operation = "MULTIPLY_ADD"
    lift.inputs[1].default_value = 2.0
    lift.inputs[2].default_value = 0.35
    lift.use_clamp = True
    links.new(lum.outputs["Val"], lift.inputs[0])
    unripe = nodes.new("ShaderNodeMix")
    unripe.data_type = "RGBA"
    unripe.blend_type = "MULTIPLY"
    unripe.inputs["Factor"].default_value = 1.0
    unripe.inputs[6].default_value = (*tint, 1.0)
    links.new(lift.outputs["Value"], unripe.inputs[7])
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    links.new(sep.outputs["Red"], mix.inputs["Factor"])
    links.new(unripe.outputs[2], mix.inputs[6])
    links.new(base, mix.inputs[7])
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    return mat


def _light(scene, mode):
    world = kit._sky(kit.DEFAULT_HDRI, strength={"day": 1.0, "dusk": 0.10, "rain": 0.32}[mode], rotation=40)
    if mode == "dusk":
        sun = bpy.data.lights.new("SheetSun", "SUN")
        sun.energy = 1.6
        sun.color = (1.0, 0.55, 0.30)
        sun.angle = math.radians(2.0)
        obj = bpy.data.objects.new("SheetSun", sun)
        scene.collection.objects.link(obj)
        obj.rotation_euler = Euler((math.radians(86), 0, math.radians(-60)))
    if mode == "rain":
        # Overcast: grey out the sky so light comes flat from above.
        bg = world.node_tree.nodes["Background"]
        mix = world.node_tree.nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        mix.inputs["Factor"].default_value = 0.85
        mix.inputs[7].default_value = (0.55, 0.58, 0.62, 1.0)
        env_link = next(l for l in world.node_tree.links if l.to_socket == bg.inputs["Color"])
        world.node_tree.links.new(env_link.from_socket, mix.inputs[6])
        world.node_tree.links.new(mix.outputs[2], bg.inputs["Color"])
    return world


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--growth", type=float, nargs="*", default=[0.25, 0.5, 0.75, 1.0])
    ap.add_argument("--light", choices=("day", "dusk", "rain"), default="day")
    ap.add_argument("--distance", type=float, default=7.0)
    ap.add_argument("--samples", type=int, default=48)
    ap.add_argument("--width", type=int, default=1920)
    ap.add_argument("--height", type=int, default=1080)
    ap.add_argument("--extra", action="append", default=[])
    ap.add_argument("--out")
    args = ap.parse_args(argv)

    folder = Path(bpy.data.filepath).parent
    visual = folder.name
    report = json.loads((folder / "report.json").read_text(encoding="utf-8"))
    scene = bpy.context.scene
    for obj in scene.objects:
        obj.hide_render = True
    bed = kit.append(ROOT / "Assets" / "Props" / "TilledBed" / "TilledBed.blend", ["SM_TilledBed"])["SM_TilledBed"]
    produce = report.get("produce") or {}
    look = LOOKS.get(visual)
    produce_obj = bpy.data.objects.get(produce.get("mesh", "")) if produce else None
    sheet_mat = None
    if produce_obj is not None:
        sheet_mat = _ripeness_material(produce_obj.material_slots[0].material, UNRIPE.get(visual, (0.4, 0.5, 0.25)))

    plots = [(g, None) for g in args.growth]
    for extra in args.extra:
        path, name = extra.rsplit(":", 1)
        plots.append((None, kit.append(Path(path), [name])[name]))
    spacing = 1.35
    x0 = -spacing * (len(plots) - 1) / 2
    for i, (growth, template) in enumerate(plots):
        at = Vector((x0 + i * spacing, 0, 0))
        kit.instance(bed, f"Bed{i}", location=at + Vector((0, 0, -0.012)))
        if template is not None:
            kit.instance(template, f"Extra{i}", location=at)
            continue
        stage = stage_of(growth)
        if stage is None:
            continue
        plant = bpy.data.objects[f"SM_{visual}_{stage}"]
        copy = plant.copy()
        copy.name = f"Plot{i}"
        scene.collection.objects.link(copy)
        copy.hide_render = False
        copy.matrix_world = Matrix.Translation(at)
        if not (look and produce_obj and stage != "Sprout" and growth >= look[0]):
            continue
        appear, rise, min_scale, min_len, power, boost = look
        t = min(max((growth - appear) / (1.0 - appear), 0.0), 1.0)
        ripeness = 1.0 if growth >= 1.0 else t ** power
        for x, y, z, yaw, scale in produce["anchors"].get(stage, []):
            across = (min_scale + (1 - min_scale) * t) * scale * boost
            along = (min_len + (1 - min_len) * t) * scale * boost
            inst = produce_obj.copy()
            scene.collection.objects.link(inst)
            inst.hide_render = False
            inst.material_slots[0].link = "OBJECT"
            inst.material_slots[0].material = sheet_mat
            inst.color = (ripeness, 0.0, 0.0, 1.0)
            inst.matrix_world = (Matrix.Translation(at + Vector((x, y, z - (1 - t) * rise / 100.0)))
                                 @ Matrix.Rotation(math.radians(yaw), 4, "Z")
                                 @ Matrix.Diagonal((across, across, along, 1.0)))

    scene.render.engine = "CYCLES"
    kit.use_gpu()
    scene.cycles.samples = args.samples
    scene.cycles.transparent_max_bounces = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = args.width, args.height
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = {"day": -0.35, "dusk": 0.0, "rain": -0.1}[args.light]
    _light(scene, args.light)
    soil = bpy.data.materials.new("SheetGround")
    soil.use_nodes = True
    bsdf = soil.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.045, 0.060, 0.028, 1) if args.light != "rain" else (0.03, 0.04, 0.02, 1)
    bsdf.inputs["Roughness"].default_value = 0.9 if args.light != "rain" else 0.45
    kit.box("SheetGround", (80, 80, 0.02), (0, 0, -0.035), material=soil)

    cam_data = bpy.data.cameras.new("SheetCam")
    cam_data.lens = 35
    cam = bpy.data.objects.new("SheetCam", cam_data)
    scene.collection.objects.link(cam)
    # The third-person camera sits behind and above her, looking down about 25 degrees.
    target = Vector((0, 0, 0.25))
    direction = Vector((0, -math.cos(math.radians(25)), math.sin(math.radians(25))))
    cam.location = target + direction * args.distance
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    out = Path(args.out) if args.out else folder / f"sheet_{args.light}.png"
    scene.render.filepath = str(out)
    bpy.ops.render.render(write_still=True)
    print("HOMESTEAD_SHEET", out)


if __name__ == "__main__":
    main()
