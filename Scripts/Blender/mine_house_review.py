"""Cycles review renders of the built mine house (Assets/Props/MineHouse/MineHouse.blend) from outside and
inside, with the furnishing appended if built. Review aid only; Jenny's review media come from the game.

    blender -b Assets/Props/MineHouse/MineHouse.blend --python Scripts/Blender/mine_house_review.py -- <out_dir> [samples] [views]

Views are named below (comma-separated to pick some). Blender frame: Unreal Y is negated.
"""
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "Recipes"))
from mine import layout as L  # noqa: E402

argv = sys.argv[sys.argv.index("--") + 1:]
out = Path(argv[0])
samples = int(argv[1]) if len(argv) > 1 else 48
pick = set(argv[2].split(",")) if len(argv) > 2 else None
out.mkdir(parents=True, exist_ok=True)
ROOT = HERE.parents[1]

scene = bpy.context.scene
for obj in list(scene.objects):
    if obj.type == "MESH" and "_LOD" in obj.name:
        obj.hide_render = True


def append_set(name):
    blend = ROOT / "Assets" / "Props" / name / f"{name}.blend"
    if not blend.exists():
        return []
    with bpy.data.libraries.load(str(blend)) as (src, dst):
        dst.objects = [n for n in src.objects if n.startswith("SM_") and "_LOD" not in n]
    objs = [o for o in dst.objects if o]
    for o in objs:
        scene.collection.objects.link(o)
    return objs


def place(objs, name, frame, loc, yaw):
    for o in objs:
        if o.name == name:
            x, y, z = loc
            o.location = (x, -y, z)
            o.rotation_euler = (0, 0, -math.radians(yaw))


fur = append_set("MineFurnishing")
for label, path, frame, loc, yaw in L.dressing():
    if frame == "house" and "/MineFurnishing/" in path:
        place(fur, path.rsplit("/", 1)[-1], frame, loc, yaw)
for o in fur:
    if o.name == "SM_MineTrapdoorLid":
        tx, ty = L.TRAPDOOR["centre"]
        o.location = (tx, -(ty + L.TRAPDOOR["size"][1] / 2), L.FLOOR_Z)
        o.rotation_euler = (0, -math.radians(100), math.radians(90))    # thrown open past upright

scene.render.engine = "CYCLES"
scene.cycles.samples = samples
scene.cycles.use_denoising = True
scene.cycles.max_bounces = 8
scene.render.resolution_x, scene.render.resolution_y = 1920, 1080
scene.view_settings.view_transform = "AgX"
scene.view_settings.exposure = -0.2
prefs = bpy.context.preferences.addons["cycles"].preferences
for kind in ("OPTIX", "CUDA"):
    try:
        prefs.compute_device_type = kind
        prefs.get_devices()
        for d in prefs.devices:
            d.use = True
        scene.cycles.device = "GPU"
        break
    except Exception:
        continue

world = bpy.data.worlds.new("Sky")
world.use_nodes = True
tree = world.node_tree
hdri = ROOT / "Assets" / "Source" / "Blender" / "polyhaven" / "kloofendal_48d_partly_cloudy_puresky_2k.exr"
bg = tree.nodes["Background"]
if hdri.exists():
    env = tree.nodes.new("ShaderNodeTexEnvironment")
    env.image = bpy.data.images.load(str(hdri))
    tree.links.new(env.outputs["Color"], bg.inputs["Color"])
bg.inputs["Strength"].default_value = 1.0
scene.world = world
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 3.5
sun.data.angle = math.radians(1.0)
sun.rotation_euler = (math.radians(50), 0, math.radians(140))
scene.collection.objects.link(sun)

# Yard: a trodden earth plane.
bpy.ops.mesh.primitive_plane_add(size=80, location=(0, 0, 0))
yard = bpy.context.active_object
m = bpy.data.materials.new("Yard")
m.use_nodes = True
b = m.node_tree.nodes["Principled BSDF"]
b.inputs["Base Color"].default_value = (0.16, 0.13, 0.09, 1)
b.inputs["Roughness"].default_value = 0.95
yard.data.materials.append(m)
# Cut the yard under the shaft.
tx, ty = L.TRAPDOOR["centre"]
bpy.ops.mesh.primitive_cube_add(size=1, location=(tx, -ty, 0))
cut = bpy.context.active_object
cut.scale = (1.3, 1.3, 1.0)
mod = yard.modifiers.new("cut", "BOOLEAN")
mod.operation, mod.object = "DIFFERENCE", cut
cut.hide_render = True

# The desk lamp: a warm point light.
lx, ly, lz = L.LAMP_ON_DESK
lamp = bpy.data.objects.new("DeskLamp", bpy.data.lights.new("DeskLamp", "POINT"))
lamp.data.energy = 25.0
lamp.data.color = (1.0, 0.62, 0.3)
lamp.data.shadow_soft_size = 0.03
lamp.location = (lx, -ly, lz + 0.22)
scene.collection.objects.link(lamp)

cam_data = bpy.data.cameras.new("Cam")
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam

VIEWS = {
    # name: (eye (Unreal frame m), target, lens mm)
    "path": ((26.0, 3.0, 3.0), (0.0, 0.0, 6.0), 35),
    "east34": ((14.0, -11.0, 3.5), (0.0, 0.0, 6.0), 30),
    "sea": ((-6.0, 17.0, 3.0), (-1.0, 0.0, 6.5), 28),
    "bobwall": ((-16.0, 9.0, 4.0), (-3.0, 1.0, 7.0), 30),
    "stacktop": ((-12.0, 14.0, 18.0), (-3.7, 2.2, 13.0), 45),
    "door": ((8.0, -1.2, 1.7), (4.5, 0.0, 1.8), 30),
    "inside_door": ((3.4, 0.0, 1.6), (-3.0, 0.0, 1.0), 20),
    "inside_corner": ((3.2, -1.9, 2.4), (-2.2, 1.0, 0.6), 18),
    "desk": ((0.4, -0.9, 1.6), (-3.0, 0.0, 1.0), 24),
    "shaft": ((-0.1, -0.6, 2.2), (-1.0, 0.0, -1.5), 22),
}
for name, (eye, target, lens) in VIEWS.items():
    if pick and name not in pick:
        continue
    cam.location = Vector((eye[0], -eye[1], eye[2]))
    direction = Vector((target[0], -target[1], target[2])) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    cam_data.lens = lens
    cam_data.clip_start = 0.05
    scene.render.filepath = str(out / f"{name}.png")
    bpy.ops.render.render(write_still=True)
    print("HOMESTEAD_MINE_REVIEW", name)
