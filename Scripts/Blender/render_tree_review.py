"""Quick Cycles review renders for a built tree set, headless.

    blender --background <Assets/Props/Name/Name.blend> --python render_tree_review.py
        -- [--samples 96] [--width 1920 --height 1080] [--view eye|under|far|lods]

Views (``review_<view>.png`` beside report.json):
- ``eye``: LOD0 from a standing player's eye (1.6 m) about 1.2 crown widths away.
- ``under``: LOD0 from just outside the trunk, looking up into the canopy.
- ``far``: a small grove of LOD0 copies from about 150 m.
- ``lods``: LOD0..LOD3 side by side from about 150 m, to judge whether the far LODs hold the
  canopy's mass and colour.
"""
import argparse
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_kit as kit  # noqa: E402


def camera(location, target, lens):
    data = bpy.data.cameras.new("ReviewCam")
    data.lens = lens
    data.clip_start, data.clip_end = 0.05, 5000
    cam = bpy.data.objects.new("ReviewCam", data)
    bpy.context.scene.collection.objects.link(cam)
    cam.location = location
    cam.rotation_euler = (Vector(target) - Vector(location)).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = cam
    return cam


def copy(obj, location, yaw=0.0, scale=1.0):
    dup = bpy.data.objects.new(obj.name + "_copy", obj.data)
    bpy.context.scene.collection.objects.link(dup)
    dup.location = location
    dup.rotation_euler = (0, 0, yaw)
    dup.scale = (scale, scale, scale)
    return dup


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--samples", type=int, default=96)
    parser.add_argument("--width", type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    parser.add_argument("--view", action="append")
    args = parser.parse_args(argv)
    folder = Path(bpy.data.filepath).parent
    meshes = sorted([o for o in bpy.context.scene.objects if o.type == "MESH" and o.name.startswith("SM_")],
                    key=lambda o: o.name)
    lod0 = meshes[0]
    for obj in meshes:
        obj.hide_render = True
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    kit.use_gpu()
    scene.cycles.samples = args.samples
    scene.cycles.transparent_max_bounces = 64
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = args.width, args.height
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.35
    kit._sky(kit.DEFAULT_HDRI, rotation=40)
    soil = bpy.data.materials.new("M_ReviewGround")
    soil.use_nodes = True
    bsdf = soil.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.045, 0.05, 0.025, 1)
    bsdf.inputs["Roughness"].default_value = 0.95
    kit.box("ReviewGround", (1200, 1200, 0.02), (0, 0, -0.01), material=soil)
    lo, hi = kit.bounds(lod0)
    size = hi - lo
    width = max(size.x, size.y)
    ground = 0.3  # the skirt: the tree's ground line sits 30 cm above its pivot
    for obj in meshes:
        obj.location = (0, 0, -ground)
    views = args.view or ["eye", "under", "far", "lods"]
    for view in views:
        made = []
        for obj in meshes:
            obj.hide_render = True
        if view == "eye":
            lod0.hide_render = False
            d = width * 1.15 + 4
            camera((-0.55 * d, -0.84 * d, 1.6), (0, 0, size.z * 0.45), 24)
        elif view == "under":
            lod0.hide_render = False
            camera((-2.2, -3.0, 1.6), (4.0, 6.0, size.z * 0.75), 16)
        elif view == "far":
            spots = [(0, 0, 0.0, 1.0), (width * 0.95, width * 0.35, 1.9, 0.9), (-width * 0.85, width * 0.55, 4.1, 1.08),
                     (width * 0.35, width * 1.1, 2.7, 0.95), (-width * 0.3, -width * 0.2, 5.5, 0.85)]
            for x, y, yaw, sc in spots:
                made.append(copy(lod0, (x, y, -ground * sc), yaw, sc))
            camera((-150 * 0.55, -150 * 0.84, 1.7), (0, width * 0.3, size.z * 0.45), 50)
        elif view == "lods":
            gap = width * 1.15
            for i, obj in enumerate(meshes):
                made.append(copy(obj, ((i - (len(meshes) - 1) / 2) * gap, 0, -ground)))
            camera((0, -150, 1.7), (0, 0, size.z * 0.45), 50)
        scene.render.filepath = str(folder / f"review_{view}.png")
        bpy.ops.render.render(write_still=True)
        print("HOMESTEAD_TREE_REVIEW", view, scene.render.filepath)
        for dup in made:
            bpy.data.objects.remove(dup)


if __name__ == "__main__":
    main()
