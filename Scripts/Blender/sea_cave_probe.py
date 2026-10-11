"""Dev probe for Recipes/sea_cave.py: mesh + cull only, then quick EEVEE views from inside the cave.

    blender -b --factory-startup --python Scripts/Blender/sea_cave_probe.py -- <out_dir>
"""
import importlib.util
import math
import sys
import time
from pathlib import Path

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import homestead_kit as kit  # noqa: E402
import homestead_materials as mats  # noqa: E402
kit.mats = mats

spec = importlib.util.spec_from_file_location("sea_cave", HERE / "Recipes" / "sea_cave.py")
cave = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cave)
out = Path(sys.argv[sys.argv.index("--") + 1])
out.mkdir(parents=True, exist_ok=True)

kit.reset()
t = time.time()
site = cave.Site()
node, void, crag, ground, shell = cave.rock_node(site)
holes, verts = cave.hole_vertices(site, void)
print(f"PROBE holes {len(holes)} quads in {time.time() - t:.1f}s")
shore = cave.pool_shore(site)
hull = cave.footprint(void)
print(f"PROBE shore {len(shore)} pts, hull {len(hull)} pts")
t = time.time()
mat = bpy.data.materials.new("M_Probe")
mat.diffuse_color = (0.35, 0.33, 0.3, 1)
obj = cave.sdf.mesh("SM_SeaCave", node, voxel=0.2, material=mat)
print(f"PROBE meshed {len(obj.data.polygons)} faces in {time.time() - t:.1f}s")
cave._cull(obj, site, void, holes)
print(f"PROBE culled to {len(obj.data.polygons)} faces")
obj.data.shade_smooth()

# Landscape stand-in: the real ground around the site, with the hole quads removed.
import numpy as np
xs = np.arange(-12.0, 45.0, 1.0)
ys = np.arange(-20.0, 20.0, 1.0)
hole_set = set(map(tuple, holes))
verts_g, faces_g, index = [], [], {}
for i, x in enumerate(xs):
    for j, y in enumerate(ys):
        index[i, j] = len(verts_g)
        verts_g.append((x, y, float(site.ground(np.array([x]), np.array([y]))[0])))
for i in range(len(xs) - 1):
    for j in range(len(ys) - 1):
        cx, cy = site.to_world(np.array([xs[i] + 0.5]), np.array([ys[j] + 0.5]))
        if (int(math.floor(cx[0])), int(math.floor(cy[0]))) in hole_set:
            continue
        faces_g.append((index[i, j], index[i + 1, j], index[i + 1, j + 1], index[i, j + 1]))
gm = bpy.data.meshes.new("Ground")
gm.from_pydata(verts_g, [], faces_g)
g_obj = bpy.data.objects.new("Ground", gm)
bpy.context.scene.collection.objects.link(g_obj)
gmat = bpy.data.materials.new("M_Ground")
gmat.diffuse_color = (0.25, 0.4, 0.15, 1)
gm.materials.append(gmat)

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
world = bpy.data.worlds.new("W")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.5, 0.6, 0.8, 1)
world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
scene.world = world
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 4.0
sun.rotation_euler = (math.radians(50), 0, math.radians(200))
scene.collection.objects.link(sun)
lamp = bpy.data.objects.new("Lamp", bpy.data.lights.new("Lamp", "POINT"))
lamp.data.energy = 1500
lamp.location = (24, 2, 5)
scene.collection.objects.link(lamp)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
cam.data.lens = 18
scene.collection.objects.link(cam)
scene.camera = cam


def shot(name, loc, target):
    cam.location = loc
    d = Vector(target) - Vector(loc)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = str(out / f"{name}.png")
    bpy.ops.render.render(write_still=True)


shot("outside", (-22, -6, 4), (2, 0, 4))
lamp.hide_render = True
shot("outside_high", (-30, 14, 22), (6, 0, 6))
lamp.hide_render = False
shot("passage", (-1, 0, 1.7), (14, 3, 2))
shot("chamber", (15, 3.2, 1.7), (30, 0, 2))
shot("pool", (19, -1, 2.5), (26, 4.8, 0))
shot("back", (33, -5, 2.4), (16, 3, 2))
g_obj.hide_render = True
shot("top", (16, 0, 60), (16, 0.01, 0))
# Plan section: everything above 2.5 m clipped away, ortho from above.
cam.data.type = "ORTHO"
cam.data.ortho_scale = 50
cam.data.clip_start = 60 - 2.5
cam.data.clip_end = 70
lamp.location = (20, 0, 2.0)
lamp.data.energy = 30000
shot("plan", (14, 0, 60), (14, 0.01, 0))
cam.data.type = "PERSP"
cam.data.clip_start = 0.1
cam.data.clip_end = 1000
lamp.location = (24, 2, 5)
lamp.data.energy = 1500
g_obj.hide_render = False
bpy.ops.wm.save_as_mainfile(filepath=str(out / "probe.blend"))
