"""Cycles review renders: neutral skin, homespun garments, HDRI sky, 4K."""
import math

import bpy
from mathutils import Vector

KEEP_FACE = ("MI_Face_Skin_Baked_LOD0_VT", "MI_EyeL_Baked", "MI_EyeR_Baked", "MI_Teeth_Baked")


def prepare_face(face_obj, face_arm, arm):
    """Keep LOD0 skin, eyes and teeth; rigidly follow the body's head bone for test poses."""
    import bmesh
    me = face_obj.data
    keep = {i for i, m in enumerate(me.materials) if m and m.name in KEEP_FACE}
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index not in keep], context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bm.to_mesh(me)
    bm.free()
    for mod in list(face_obj.modifiers):
        face_obj.modifiers.remove(mod)
    # skin the kept face to the body skeleton: its body-bone weights (neck, head, spine) as-is,
    # the remainder (facial joints) folded into 'head', so the neck seam stays closed in poses
    bones = {b.name for b in arm.data.bones}
    gname = {vg.index: vg.name for vg in face_obj.vertex_groups}
    rest = {}
    for v in face_obj.data.vertices:
        tot = sum(g.weight for g in v.groups if gname[g.group] in bones)
        rest[v.index] = max(0.0, 1.0 - tot)
    for vg in list(face_obj.vertex_groups):
        if vg.name not in bones:
            face_obj.vertex_groups.remove(vg)
    head = face_obj.vertex_groups.get("head") or face_obj.vertex_groups.new(name="head")
    for i, w in rest.items():
        if w > 1e-4:
            head.add([i], w, "ADD")
    mw = face_obj.matrix_world.copy()
    face_obj.parent = arm
    face_obj.matrix_world = mw
    mod = face_obj.modifiers.new("Armature", "ARMATURE")
    mod.object = arm
    ad = face_arm.data
    bpy.data.objects.remove(face_arm)
    bpy.data.armatures.remove(ad)


def skin_material():
    mat = bpy.data.materials.new("M_ReviewSkin")
    if mat.node_tree is None:
        mat.use_nodes = True
    nt = mat.node_tree
    b = nt.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.50, 0.33, 0.25, 1)
    b.inputs["Roughness"].default_value = 0.48
    b.inputs["Subsurface Weight"].default_value = 0.25
    b.inputs["Subsurface Radius"].default_value = (1.0, 0.45, 0.25)
    b.inputs["Subsurface Scale"].default_value = 0.006
    b.inputs["Specular IOR Level"].default_value = 0.45
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 300.0
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.06
    nt.links.new(noise.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], b.inputs["Normal"])
    return mat


def eye_material():
    mat = bpy.data.materials.new("M_ReviewEye")
    if mat.node_tree is None:
        mat.use_nodes = True
    nt = mat.node_tree
    b = nt.nodes["Principled BSDF"]
    geo = nt.nodes.new("ShaderNodeNewGeometry")
    dot = nt.nodes.new("ShaderNodeVectorMath")
    dot.operation = "DOT_PRODUCT"
    dot.inputs[1].default_value = (0, -1, 0)
    nt.links.new(geo.outputs["Normal"], dot.inputs[0])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    el = ramp.color_ramp.elements
    el[0].position, el[0].color = 0.90, (0.62, 0.58, 0.54, 1)
    el[1].position, el[1].color = 0.93, (0.09, 0.05, 0.025, 1)
    e = el.new(0.985)
    e.color = (0.012, 0.01, 0.008, 1)
    nt.links.new(dot.outputs["Value"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], b.inputs["Base Color"])
    b.inputs["Roughness"].default_value = 0.05
    return mat


def assign_review_materials(body_obj, face_obj):
    skin, eye = skin_material(), eye_material()
    body_obj.data.materials.clear()
    body_obj.data.materials.append(skin)
    names = [m.name if m else "" for m in face_obj.data.materials]
    face_obj.data.materials.clear()
    for n in names:
        face_obj.data.materials.append(eye if "Eye" in n else skin)


def use_gpu():
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for backend in ("OPTIX", "CUDA"):
        try:
            prefs.compute_device_type = backend
        except TypeError:
            continue
        prefs.get_devices()
        gpus = [d for d in prefs.devices if d.type == backend]
        if gpus:
            for d in prefs.devices:
                d.use = d.type == backend
            bpy.context.scene.cycles.device = "GPU"
            return backend
    bpy.context.scene.cycles.device = "CPU"
    return "CPU"


def setup_scene(hdri, samples):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    backend = use_gpu()
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.cycles.use_adaptive_sampling = True
    sc.render.film_transparent = False
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    sc.view_settings.view_transform = "AgX"
    sc.view_settings.look = "AgX - Medium High Contrast"
    sc.view_settings.exposure = -0.35
    world = bpy.data.worlds.new("ReviewSky")
    sc.world = world
    if world.node_tree is None:
        world.use_nodes = True
    tree = world.node_tree
    env = tree.nodes.new("ShaderNodeTexEnvironment")
    env.image = bpy.data.images.load(str(hdri), check_existing=True)
    mapping = tree.nodes.new("ShaderNodeMapping")
    coords = tree.nodes.new("ShaderNodeTexCoord")
    mapping.inputs["Rotation"].default_value[2] = math.radians(40)
    tree.links.new(coords.outputs["Generated"], mapping.inputs["Vector"])
    tree.links.new(mapping.outputs["Vector"], env.inputs["Vector"])
    bg = tree.nodes["Background"]
    bg.inputs["Strength"].default_value = 0.8
    tree.links.new(env.outputs["Color"], bg.inputs["Color"])
    floor_me = bpy.data.meshes.new("ReviewFloor")
    s = 30
    floor_me.from_pydata([(-s, -s, 0), (s, -s, 0), (s, s, 0), (-s, s, 0)], [], [(0, 1, 2, 3)])
    floor = bpy.data.objects.new("ReviewFloor", floor_me)
    sc.collection.objects.link(floor)
    fm = bpy.data.materials.new("M_ReviewFloor")
    if fm.node_tree is None:
        fm.use_nodes = True
    fb = fm.node_tree.nodes["Principled BSDF"]
    fb.inputs["Base Color"].default_value = (0.11, 0.09, 0.07, 1)
    fb.inputs["Roughness"].default_value = 0.95
    floor_me.materials.append(fm)
    for name, loc, energy, size, color in (("Key", (-1.6, -2.2, 2.4), 320, 1.6, (1.0, 0.95, 0.88)),
                                           ("Rim", (1.5, 2.0, 2.2), 260, 1.2, (0.9, 0.95, 1.0))):
        ld = bpy.data.lights.new(name, "AREA")
        ld.energy, ld.size, ld.color = energy, size, color
        lo = bpy.data.objects.new(name, ld)
        sc.collection.objects.link(lo)
        lo.location = loc
        lo.rotation_euler = (Vector((0, 0, 1.2)) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    return backend


def shoot(path, target, cam_dir, distance, lens=85, resolution=(2160, 3840), fstop=None):
    sc = bpy.context.scene
    cd = bpy.data.cameras.new("ReviewCam")
    cd.lens = lens
    if fstop:
        cd.dof.use_dof = True
        cd.dof.focus_distance = distance
        cd.dof.aperture_fstop = fstop
    cam = bpy.data.objects.new("ReviewCam", cd)
    sc.collection.objects.link(cam)
    target = Vector(target)
    cam.location = target + Vector(cam_dir).normalized() * distance
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    sc.camera = cam
    sc.render.resolution_x, sc.render.resolution_y = resolution
    sc.render.resolution_percentage = 100
    sc.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
    bpy.data.cameras.remove(cd)
    return str(path)
