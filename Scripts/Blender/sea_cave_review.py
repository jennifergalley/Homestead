"""Cycles review renders of the built sea cave from inside (Assets/Props/SeaCave/SeaCave.blend): the mouth from
the sand, the passage, the chamber and the pool under the fissure's light. Review aid only; Jenny's review media
come from the game.

    blender -b Assets/Props/SeaCave/SeaCave.blend --python Scripts/Blender/sea_cave_review.py -- <out_dir> [samples]
"""
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

out = Path(sys.argv[sys.argv.index("--") + 1])
samples = int(sys.argv[sys.argv.index("--") + 2]) if len(sys.argv) > sys.argv.index("--") + 2 else 48
out.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
for obj in list(scene.objects):
    if obj.type == "MESH" and obj.name != "SM_SeaCave":
        obj.hide_render = True
scene.render.engine = "CYCLES"
scene.cycles.samples = samples
scene.cycles.use_denoising = True
scene.cycles.max_bounces = 6
scene.render.resolution_x, scene.render.resolution_y = 1600, 900
scene.view_settings.view_transform = "AgX"
world = bpy.data.worlds.new("Sky")
world.use_nodes = True
bg = world.node_tree.nodes["Background"]
bg.inputs[0].default_value = (0.55, 0.68, 0.9, 1)
bg.inputs[1].default_value = 1.6
scene.world = world

# Beach stand-in under the mouth, and water in the basin.
def plane(name, size, loc, color, rough):
    bpy.ops.mesh.primitive_plane_add(size=size, location=loc)
    p = bpy.context.active_object
    p.name = name
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Roughness"].default_value = rough
    if name == "Water":
        b.inputs["Transmission Weight"].default_value = 1.0
        b.inputs["IOR"].default_value = 1.33
    p.data.materials.append(m)
    return p

plane("Beach", 60, (-30, 0, 0.02), (0.36, 0.31, 0.22), 0.9)
plane("Water", 9, (26.0, 4.8, 0.22), (0.8, 0.95, 0.9), 0.02)

sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 4.0
sun.data.angle = math.radians(1.0)
sun.rotation_euler = (math.radians(55), 0, math.radians(-100))     # low sun from out over the bay
scene.collection.objects.link(sun)
shaft = bpy.data.objects.new("Fissure", bpy.data.lights.new("Fissure", "AREA"))
shaft.data.shape = "RECTANGLE"
shaft.data.size, shaft.data.size_y = 5.0, 0.5
shaft.data.energy = 9000
shaft.data.color = (0.93, 0.97, 1.0)
shaft.location = (26.4, 4.7, 11.6)
shaft.rotation_euler = (0, 0, math.radians(28))
scene.collection.objects.link(shaft)
lamp = bpy.data.objects.new("Lamp", bpy.data.lights.new("Lamp", "POINT"))   # her oil lamp, carried
lamp.data.energy = 60
lamp.data.color = (1.0, 0.72, 0.42)
lamp.data.shadow_soft_size = 0.05
scene.collection.objects.link(lamp)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
cam.data.lens = 20
scene.collection.objects.link(cam)
scene.camera = cam


def shot(name, loc, target, lamp_at=None):
    cam.location = loc
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    lamp.hide_render = lamp_at is None
    if lamp_at:
        lamp.location = lamp_at
    scene.render.filepath = str(out / f"{name}.png")
    bpy.ops.render.render(write_still=True)


shot("mouth", (-16, -5, 1.7), (2, 0.5, 4))
shot("passage", (-1.5, 0.2, 1.6), (12, 2.5, 2.0))
shot("chamber", (14.5, 3.0, 1.7), (28, 0, 2.2), lamp_at=(15.2, 2.6, 1.4))
shot("pool", (20.5, 0.5, 2.2), (26.2, 4.8, 0.4))
shot("fissure", (24.0, 2.0, 1.2), (26.4, 4.7, 11.0))
shot("ledge", (33.5, -5.5, 2.5), (18, 3, 2.0), lamp_at=(33.0, -5.2, 2.2))
