"""Small, deterministic Blender helpers for authoring Homestead static props.

Recipes receive this module as ``kit``. Conventions match the rest of the
project's Blender exports: meters, Z up, -Y forward, pivot at bottom-center,
one material slot per flat-tinted part (imported into Unreal as M_Field
instances with the same Tint and Roughness).
"""
import hashlib
import math
import os
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

HEROINE_HEIGHT_M = 1.63


# --------------------------------------------------------------------------- scene

def reset():
    """Start from an empty metric scene. In a live GUI session this clears the
    current scene's data instead of reloading factory settings (which would
    reset the user's window layout)."""
    if bpy.app.background:
        bpy.ops.wm.read_factory_settings(use_empty=True)
    else:
        if bpy.context.object and bpy.context.object.mode != "OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")
        for collection in (bpy.data.objects, bpy.data.meshes, bpy.data.materials,
                           bpy.data.cameras, bpy.data.lights):
            for item in list(collection):
                collection.remove(item)
        bpy.context.view_layer.update()
    units = bpy.context.scene.unit_settings
    units.system = "METRIC"
    units.scale_length = 1.0
    units.length_unit = "METERS"


def focus(objects):
    """Live sessions only: show material colors and frame ``objects``."""
    if bpy.app.background:
        return
    _select_only(objects)
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            if area.type != "VIEW_3D":
                continue
            space = area.spaces.active
            space.shading.type = "SOLID"
            space.shading.color_type = "MATERIAL"
            region = next(r for r in area.regions if r.type == "WINDOW")
            with bpy.context.temp_override(window=window, area=area, region=region):
                bpy.ops.view3d.view_selected()


def _link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


def _select_only(objects, active=None):
    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = active or objects[0]


# ------------------------------------------------------------------------ materials

def material(name, color, roughness=0.85):
    """Create or reuse a flat-tinted material. ``color`` is linear RGB 0..1."""
    if not name.startswith("M_"):
        raise ValueError("Material names must start with M_: " + name)
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    rgba = (*color[:3], 1.0)
    mat.diffuse_color = rgba
    mat.roughness = roughness
    if mat.node_tree is None:
        mat.use_nodes = True
    bsdf = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = rgba
        bsdf.inputs["Roughness"].default_value = roughness
    mat["homestead_color"] = list(color[:3])
    mat["homestead_roughness"] = roughness
    return mat


def material_spec(mat):
    color = mat.get("homestead_color")
    roughness = mat.get("homestead_roughness")
    if color is None:
        bsdf = mat.node_tree and next(
            (n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if bsdf is not None:
            color = list(bsdf.inputs["Base Color"].default_value)[:3]
            roughness = bsdf.inputs["Roughness"].default_value
        else:
            color = list(mat.diffuse_color)[:3]
            roughness = mat.roughness
    return {"name": mat.name, "color": [round(float(c), 4) for c in color],
            "roughness": round(float(roughness), 3)}


# ----------------------------------------------------------------------- primitives

def _from_bmesh(name, bm, material_, location, rotation, scale):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    if not mesh.uv_layers:
        mesh.uv_layers.new(name="UVMap")
    obj = _link(bpy.data.objects.new(name, mesh))
    obj.location = location
    obj.rotation_euler = [math.radians(a) for a in rotation]
    obj.scale = scale if isinstance(scale, (tuple, list)) else (scale, scale, scale)
    if material_ is not None:
        mesh.materials.append(material_)
    return obj


def _bevel(obj, width, segments):
    if width > 0:
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = width
        mod.segments = segments
        mod.limit_method = "ANGLE"
        mod.harden_normals = False
    return obj


def box(name, size, location=(0, 0, 0), rotation=(0, 0, 0), material=None,
        bevel=0.0, bevel_segments=2):
    """Axis-aligned box of ``size`` (x, y, z) meters centered on ``location``.
    ``rotation`` is XYZ Euler degrees."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0, calc_uvs=True)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    return _bevel(_from_bmesh(name, bm, material, location, rotation, 1), bevel, bevel_segments)


def cylinder(name, radius, depth, location=(0, 0, 0), rotation=(0, 0, 0), material=None,
             sides=16, radius_top=None, bevel=0.0, bevel_segments=2, cap=True):
    """Z-aligned cylinder/cone/frustum centered on ``location``."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=sides, radius1=radius,
                          radius2=radius if radius_top is None else radius_top,
                          depth=depth, calc_uvs=True)
    return _bevel(_from_bmesh(name, bm, material, location, rotation, 1), bevel, bevel_segments)


def sphere(name, radius, location=(0, 0, 0), rotation=(0, 0, 0), material=None,
           segments=16, rings=8, scale=1.0):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius,
                              calc_uvs=True)
    return _from_bmesh(name, bm, material, location, rotation, scale)


def mesh(name, vertices, faces, material=None, location=(0, 0, 0), rotation=(0, 0, 0)):
    """Arbitrary polygons (counter-clockwise when viewed from outside)."""
    data = bpy.data.meshes.new(name)
    data.from_pydata([tuple(v) for v in vertices], [], [tuple(f) for f in faces])
    data.update()
    data.uv_layers.new(name="UVMap")
    obj = _link(bpy.data.objects.new(name, data))
    obj.location = location
    obj.rotation_euler = [math.radians(a) for a in rotation]
    if material is not None:
        data.materials.append(material)
    return obj


# ------------------------------------------------------------------------ shaping

def roughen(obj, strength=0.01, scale=6.0, seed=0, subdivide=0):
    """Displace vertices along their normals with smooth noise for a hand-made
    silhouette. ``subdivide`` adds edge cuts first so flat faces can deform."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    if subdivide:
        bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=subdivide, use_grid_fill=True)
    bm.normal_update()
    offset = Vector((seed * 17.31, seed * 5.77, seed * 11.13))
    for vert in bm.verts:
        amount = noise.noise(vert.co * scale + offset) * strength
        vert.co += vert.normal * amount
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return obj


def taper(obj, top_scale=0.8):
    """Scale XY linearly from 1 at the lowest vertex to ``top_scale`` at the highest."""
    zs = [v.co.z for v in obj.data.vertices]
    low, high = min(zs), max(zs)
    span = (high - low) or 1.0
    for vert in obj.data.vertices:
        factor = 1.0 + (top_scale - 1.0) * (vert.co.z - low) / span
        vert.co.x *= factor
        vert.co.y *= factor
    obj.data.update()
    return obj


def recalc_normals(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    return obj


# ----------------------------------------------------------------------- finalize

def join(parts, name, pivot="base", smooth_angle=35.0):
    """Apply modifiers, merge ``parts`` into one ``SM_`` mesh and finalize it."""
    parts = [p for p in parts if p is not None]
    _select_only(parts)
    bpy.ops.object.convert(target="MESH")
    if len(parts) > 1:
        _select_only(parts, parts[0])
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name
    return finalize(obj, pivot=pivot, smooth_angle=smooth_angle)


def finalize(obj, pivot="base", smooth_angle=35.0):
    """Apply transforms and modifiers, set the pivot, shading and UVs."""
    if not obj.name.startswith("SM_"):
        raise ValueError("Exported static meshes must be named SM_*: " + obj.name)
    _select_only([obj])
    if obj.modifiers:
        bpy.ops.object.convert(target="MESH")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if pivot == "base":
        lo, hi = bounds(obj)
        shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
        obj.data.transform(Matrix.Translation(-shift))
    elif pivot == "center":
        lo, hi = bounds(obj)
        obj.data.transform(Matrix.Translation(-(lo + hi) / 2))
    obj.location = (0, 0, 0)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(smooth_angle))
    obj.data.update()
    obj["homestead_finalized"] = True
    return obj


def bounds(obj):
    points = [obj.matrix_world @ v.co for v in obj.data.vertices]
    lo = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    hi = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    return lo, hi


def stats(obj):
    lo, hi = bounds(obj)
    size = hi - lo
    return {
        "vertices": len(obj.data.vertices),
        "triangles": sum(len(p.vertices) - 2 for p in obj.data.polygons),
        "size_cm": [round(size.x * 100, 2), round(size.y * 100, 2), round(size.z * 100, 2)],
        "materials": [material_spec(slot.material) for slot in obj.material_slots if slot.material],
    }


# ------------------------------------------------------------------------- export

def export_fbx(obj, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    _select_only([obj])
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={"MESH"},
                             global_scale=1.0, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y",
                             axis_up="Z", bake_anim=False, path_mode="STRIP",
                             use_mesh_modifiers=True, mesh_smooth_type="FACE",
                             use_tspace=True, add_leaf_bones=False)
    return hashlib.sha256(path.read_bytes()).hexdigest()


# ------------------------------------------------------------------------ preview

def _camera(name, location, target, ortho_scale=None, lens=50):
    data = bpy.data.cameras.new(name)
    if ortho_scale:
        data.type = "ORTHO"
        data.ortho_scale = ortho_scale
    else:
        data.lens = lens
    cam = _link(bpy.data.objects.new(name, data))
    cam.location = location
    direction = Vector(target) - Vector(location)
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    data.clip_start, data.clip_end = 0.01, 1000
    return cam


def render_preview(obj, path, panel=512):
    """Render a 2x2 contact sheet: 3/4 view beside a 1.63 m heroine-height
    figure, front (-Y), right side (+X) and top. Workbench only, so it works
    headless without a GPU-dependent renderer."""
    import numpy as np

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = scene.render.resolution_y = panel
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "MATERIAL"
    shading.show_shadows = True
    shading.show_cavity = True
    shading.cavity_type = "BOTH"
    scene.display.render_aa = "8"
    if scene.world is None:
        scene.world = bpy.data.worlds.new("PreviewWorld")
    scene.world.color = (0.62, 0.66, 0.70)

    hidden = [o for o in scene.objects if o is not obj and o.type == "MESH"]
    for other in hidden:
        other.hide_render = True
    gray = material("M_PreviewReference", (0.45, 0.45, 0.48))
    ground_mat = material("M_PreviewGround", (0.36, 0.40, 0.30))
    lo, hi = bounds(obj)
    size = hi - lo
    center = (lo + hi) / 2
    figure_x = hi.x + 0.25 + 0.2
    figure = cylinder("PreviewFigure", 0.2, HEROINE_HEIGHT_M, (figure_x, center.y, HEROINE_HEIGHT_M / 2),
                      material=gray, sides=24, radius_top=0.14)
    ground = box("PreviewGround", (40, 40, 0.02), (center.x, center.y, lo.z - 0.011),
                 material=ground_mat)

    union_lo = Vector((lo.x, min(lo.y, center.y - 0.2), lo.z))
    union_hi = Vector((figure_x + 0.2, max(hi.y, center.y + 0.2), max(hi.z, HEROINE_HEIGHT_M)))
    union_center = (union_lo + union_hi) / 2
    radius = max((union_hi - union_lo).length / 2, 0.05)
    distance = radius / math.sin(math.radians(19)) * 1.05
    direction = Vector((-0.55, -1.0, 0.55)).normalized()
    margin = 1.18
    views = [
        ("persp", _camera("PreviewPersp", union_center + direction * distance, union_center),
         (figure, ground)),
        ("front", _camera("PreviewFront", center + Vector((0, -50, 0)), center,
                          max(size.x, size.z, 0.05) * margin), ()),
        ("side", _camera("PreviewSide", center + Vector((50, 0, 0)), center,
                         max(size.y, size.z, 0.05) * margin), ()),
        ("top", _camera("PreviewTop", center + Vector((0, 0, 50)), center,
                        max(size.x, size.y, 0.05) * margin), ()),
    ]
    views[3][1].rotation_euler = (0, 0, 0)

    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    pixels = {}
    for label, cam, visible in views:
        figure.hide_render = figure not in visible
        ground.hide_render = ground not in visible
        scene.camera = cam
        temp = path.with_name(f".{path.stem}_{label}.png")
        scene.render.filepath = str(temp)
        bpy.ops.render.render(write_still=True)
        image = bpy.data.images.load(str(temp))
        buffer = np.empty(panel * panel * 4, dtype=np.float32)
        image.pixels.foreach_get(buffer)
        pixels[label] = buffer.reshape(panel, panel, 4)
        bpy.data.images.remove(image)
        os.remove(temp)

    # Blender image rows start at the bottom.
    sheet = np.vstack([np.hstack([pixels["side"], pixels["top"]]),
                       np.hstack([pixels["persp"], pixels["front"]])])
    out = bpy.data.images.new(path.stem, panel * 2, panel * 2, alpha=True)
    out.pixels.foreach_set(sheet.ravel())
    out.filepath_raw = str(path)
    out.file_format = "PNG"
    out.save()
    bpy.data.images.remove(out)

    for helper in [figure, ground] + [v[1] for v in views]:
        data = helper.data
        bpy.data.objects.remove(helper)
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)
        else:
            bpy.data.cameras.remove(data)
    for other in hidden:
        other.hide_render = False
    return str(path)
