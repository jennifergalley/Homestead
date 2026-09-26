"""Small, deterministic Blender helpers for authoring Homestead static props.

Recipes receive this module as ``kit``. Conventions match the rest of the
project's Blender exports: meters, Z up, -Y forward, pivot at bottom-center,
one material slot per flat-tinted part (imported into Unreal as M_Field
instances with the same Tint and Roughness).
"""
import hashlib
import math
import os
import shutil
from pathlib import Path

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector, noise

HEROINE_HEIGHT_M = 1.63
ROOT = Path(__file__).resolve().parents[2]
SOURCE_CACHE = ROOT / "Assets" / "Source" / "Blender" / "polyhaven"
DEFAULT_HDRI = SOURCE_CACHE / "kloofendal_48d_partly_cloudy_puresky_2k.exr"


# ------------------------------------------------------------------- sources

def polyhaven(asset_id, resolution="4k"):
    """Path to a fetched CC0 Poly Haven .blend (see Get-PolyHavenAsset.ps1)."""
    path = SOURCE_CACHE / f"{asset_id}_{resolution}" / f"{asset_id}_{resolution}.blend"
    if not path.exists():
        raise FileNotFoundError(
            f"Missing {path}. Run: .\\Scripts\\Blender\\Get-PolyHavenAsset.ps1 {asset_id} -Resolution {resolution}")
    return path


def append(blend_path, names):
    """Append objects (with their materials/images) as hidden templates."""
    with bpy.data.libraries.load(str(blend_path), link=False) as (source, target):
        missing = sorted(set(names) - set(source.objects))
        if missing:
            raise KeyError(f"{blend_path} has no objects {missing}")
        target.objects = list(names)
    templates = {}
    for obj in target.objects:
        _link(obj)
        obj.hide_set(True)
        obj.hide_render = True
        templates[obj.name] = obj
    return templates


def instance(template, name, location=(0, 0, 0), rotation=(0, 0, 0), scale=1.0, matrix=None):
    """Independent copy of ``template`` keeping its own rotation/scale but not its
    location. ``matrix`` (a 4x4 placement) overrides location/rotation/scale."""
    obj = template.copy()
    obj.data = template.data.copy()
    obj.name = name
    _link(obj)
    obj.hide_set(False)
    obj.hide_render = False
    loc, rot, size = template.matrix_basis.decompose()
    own = rot.to_matrix().to_4x4() @ Matrix.Diagonal((*size, 1.0))
    if matrix is None:
        factor = scale if isinstance(scale, (tuple, list)) else (scale, scale, scale)
        matrix = (Matrix.Translation(location)
                  @ Euler([math.radians(a) for a in rotation]).to_matrix().to_4x4()
                  @ Matrix.Diagonal((*factor, 1.0)))
    obj.matrix_world = matrix @ own
    return obj


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
    textures = texture_maps(mat)
    if textures:
        return {"name": mat.name, "textures": {role: Path(p).name for role, p in textures.items()},
                "blend_method": getattr(mat, "surface_render_method", "")}
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


def texture_maps(mat):
    """Image files used by a material, keyed by a role guessed from the file name."""
    maps = {}
    if not mat or not mat.node_tree:
        return maps
    roles = (("basecolor", "basecolor"), ("alpha", "opacity"), ("nor", "normal"), ("rough", "roughness"),
             ("diff", "basecolor"), ("col", "basecolor"), ("mask", "mask"), ("ao", "ao"), ("arm", "arm"),
             ("disp", "height"))
    for node in mat.node_tree.nodes:
        if node.type != "TEX_IMAGE" or not node.image or node.image.source != "FILE":
            continue
        path = bpy.path.abspath(node.image.filepath)
        stem = Path(path).stem.lower()
        role = next((r for key, r in roles if f"_{key}" in stem), stem)
        maps[role] = path
    return maps


def copy_textures(objects, folder):
    """Copy every texture used by ``objects`` into ``folder`` and repoint the images.
    Textures that already live in another asset set under ``Assets/Props`` (shared
    tiling maps such as GraniteDetail) stay where they are and are reported by their
    path relative to ``Assets/Props``."""
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    props = (ROOT / "Assets" / "Props").resolve()
    copied = {}
    for obj in objects:
        for slot in obj.material_slots:
            if not slot.material or not slot.material.node_tree:
                continue
            for node in slot.material.node_tree.nodes:
                image = getattr(node, "image", None)
                if node.type != "TEX_IMAGE" or not image or image.source != "FILE":
                    continue
                source = Path(bpy.path.abspath(image.filepath))
                resolved = source.resolve()
                if resolved.is_relative_to(props) and not resolved.is_relative_to(folder.parent.resolve()):
                    copied[resolved.relative_to(props).as_posix()] = \
                        hashlib.sha256(resolved.read_bytes()).hexdigest()
                    continue
                target = folder / source.name
                if source.resolve() != target.resolve():
                    shutil.copy2(source, target)
                image.filepath = str(target)
                copied[source.name] = hashlib.sha256(target.read_bytes()).hexdigest()
    return copied


# ----------------------------------------------------------------------- primitives

def _from_bmesh(name, bm, material_, location, rotation, scale):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    if not mesh.uv_layers:
        mesh.uv_layers.new(name="UVMap")
    tag_coords(mesh)
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
    tag_coords(data)
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

def join(parts, name, pivot="base", smooth_angle=35.0, unwrap=True, reshade=True):
    """Apply modifiers, merge ``parts`` into one ``SM_`` mesh and finalize it.
    Use ``unwrap=False, reshade=False`` for scanned sources so their atlas UVs and
    authored normals survive."""
    parts = [p for p in parts if p is not None]
    _select_only(parts)
    bpy.ops.object.convert(target="MESH")
    if len(parts) > 1:
        _select_only(parts, parts[0])
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name
    return finalize(obj, pivot=pivot, smooth_angle=smooth_angle, unwrap=unwrap, reshade=reshade)


def finalize(obj, pivot="base", smooth_angle=35.0, unwrap=True, reshade=True):
    """Apply transforms and modifiers, set the pivot, and (optionally) UVs/shading."""
    if not obj.name.startswith("SM_"):
        raise ValueError("Exported static meshes must be named SM_*: " + obj.name)
    _select_only([obj])
    if obj.modifiers:
        bpy.ops.object.convert(target="MESH")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    shift = Vector((0, 0, 0))
    if pivot == "base":
        lo, hi = bounds(obj)
        shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    elif pivot == "center":
        lo, hi = bounds(obj)
        shift = (lo + hi) / 2
    obj.data.transform(Matrix.Translation(-shift))
    obj["homestead_shift"] = list(shift)
    obj.location = (0, 0, 0)
    if unwrap:
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.remove_doubles(threshold=0.0001)
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
        bpy.ops.object.mode_set(mode="OBJECT")
    if reshade:
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
    textured = any(texture_maps(s.material) for s in obj.material_slots)
    shading.color_type = "TEXTURE" if textured else "MATERIAL"
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


# -------------------------------------------------------------------- beauty

def use_gpu():
    """Enable Cycles on the best available GPU backend (OptiX, then CUDA/HIP)."""
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for backend in ("OPTIX", "CUDA", "HIP", "ONEAPI", "METAL"):
        try:
            prefs.compute_device_type = backend
        except TypeError:
            continue
        prefs.get_devices()
        gpus = [d for d in prefs.devices if d.type == backend]
        if gpus:
            for device in prefs.devices:
                device.use = device.type == backend
            bpy.context.scene.cycles.device = "GPU"
            return backend, [d.name for d in gpus]
    bpy.context.scene.cycles.device = "CPU"
    return "CPU", []


def _sky(hdri, strength=1.0, rotation=0.0):
    world = bpy.data.worlds.new("BeautySky")
    bpy.context.scene.world = world
    tree = world.node_tree
    env = tree.nodes.new("ShaderNodeTexEnvironment")
    env.image = bpy.data.images.load(str(hdri), check_existing=True)
    mapping = tree.nodes.new("ShaderNodeMapping")
    coords = tree.nodes.new("ShaderNodeTexCoord")
    mapping.inputs["Rotation"].default_value[2] = math.radians(rotation)
    tree.links.new(coords.outputs["Generated"], mapping.inputs["Vector"])
    tree.links.new(mapping.outputs["Vector"], env.inputs["Vector"])
    background = tree.nodes["Background"]
    background.inputs["Strength"].default_value = strength
    tree.links.new(env.outputs["Color"], background.inputs["Color"])
    return world


def _fit_distance(corners, target, direction, lens, aspect, margin=1.12, sensor=36.0):
    """Distance along ``direction`` from ``target`` that keeps every corner in frame."""
    forward = -direction.normalized()
    right = forward.cross(Vector((0, 0, 1))).normalized()
    up = right.cross(forward).normalized()
    tan_h = (sensor / 2) / lens
    tan_v = tan_h / aspect
    need = 0.0
    for corner in corners:
        rel = corner - target
        depth = rel.dot(-forward)
        need = max(need, depth + abs(rel.dot(right)) / tan_h, depth + abs(rel.dot(up)) / tan_v)
    return need * margin


def render_beauty(obj, folder, stem, hdri=DEFAULT_HDRI, resolution=(3840, 2160), samples=256,
                  views=("hero", "detail"), pose=(0, 0, 0), focus=None, ground="lowest",
                  eye_distance=None):
    """Photoreal Cycles review renders of ``obj`` on a soil ground under an HDRI sky.
    ``pose`` (XYZ degrees) temporarily re-orients the asset for review (e.g. lay a
    tool on the ground); ``focus`` is an object-space point for the close detail
    view (default: upper third). ``ground="origin"`` puts the soil at the object's
    origin instead of under its lowest point, so props authored to sink into the
    terrain (rocks) are reviewed half-buried as placed. The optional ``"eye"`` view
    looks at the asset from a standing player's eye height (1.6 m) at
    ``eye_distance`` meters. Returns render metadata."""
    scene = bpy.context.scene
    rest = obj.matrix_world.copy()
    obj.matrix_world = Euler([math.radians(a) for a in pose]).to_matrix().to_4x4() @ rest
    bpy.context.view_layer.update()
    lo, _ = bounds(obj)
    ground_z = 0.0
    ground_mode = ground
    if ground_mode != "origin":
        obj.matrix_world = Matrix.Translation((0, 0, -lo.z)) @ obj.matrix_world
    bpy.context.view_layer.update()
    focus_world = obj.matrix_world @ Vector(focus) if focus is not None else None
    scene.render.engine = "CYCLES"
    backend = use_gpu()
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.cycles.use_adaptive_sampling = True
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.35
    _sky(hdri, rotation=40)

    soil = bpy.data.materials.new("M_BeautySoil")
    if soil.node_tree is None:
        soil.use_nodes = True
    soil_bsdf = soil.node_tree.nodes["Principled BSDF"]
    noise_tex = soil.node_tree.nodes.new("ShaderNodeTexNoise")
    noise_tex.inputs["Scale"].default_value = 18.0
    ramp = soil.node_tree.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0.035, 0.026, 0.017, 1)
    ramp.color_ramp.elements[1].color = (0.085, 0.066, 0.042, 1)
    soil.node_tree.links.new(noise_tex.outputs["Fac"], ramp.inputs["Fac"])
    soil.node_tree.links.new(ramp.outputs["Color"], soil_bsdf.inputs["Base Color"])
    soil_bsdf.inputs["Roughness"].default_value = 0.95
    lo, hi = bounds(obj)
    if ground_mode == "origin":
        lo = Vector((lo.x, lo.y, max(lo.z, ground_z)))
    size = hi - lo
    center = (lo + hi) / 2
    ground = box("BeautyGround", (60, 60, 0.02), (center.x, center.y, lo.z - 0.012), material=soil)

    corners = [Vector((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]
    aspect = resolution[0] / resolution[1]
    radius = max(size.length / 2, 0.1)
    out = {}
    folder = Path(folder)
    for view in views:
        data = bpy.data.cameras.new("BeautyCam")
        data.lens = {"hero": 50, "eye": 28}.get(view, 85)
        data.dof.use_dof = view == "detail"
        data.clip_start, data.clip_end = 0.05, 2000
        cam = _link(bpy.data.objects.new("BeautyCam", data))
        if view == "hero":
            target = center
            elevation = 0.42 if size.z > max(size.x, size.y) * 0.5 else 0.75
            direction = Vector((-0.62, -1.0, elevation)).normalized()
            distance = _fit_distance(corners, target, direction, data.lens, aspect)
        elif view == "eye":
            flat = Vector((-0.62, -1.0, 0.0)).normalized()
            reach = eye_distance or max(size.x, size.y) * 1.3 + 2.0
            cam.location = Vector((center.x, center.y, lo.z + 1.6)) + flat * reach
            target = Vector((center.x, center.y, lo.z + min(size.z * 0.4, 1.6)))
            direction = (cam.location - target).normalized()
            distance = (cam.location - target).length
        else:
            target = focus_world or Vector((center.x - size.x * 0.12, center.y - size.y * 0.25,
                                            lo.z + size.z * 0.72))
            direction = Vector((-0.5, -1.0, 0.45 if focus_world else 0.25)).normalized()
            distance = max(radius * (0.9 if focus_world else 1.35), 0.35)
            data.dof.focus_distance = distance
            data.dof.aperture_fstop = 22.0
        cam.location = target + direction * distance
        cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
        scene.camera = cam
        path = folder / f"{stem}_{view}.png"
        scene.render.filepath = str(path)
        bpy.ops.render.render(write_still=True)
        out[view] = str(path)
        bpy.data.objects.remove(cam)
        bpy.data.cameras.remove(data)
    bpy.data.objects.remove(ground)
    obj.matrix_world = rest
    return {"views": out, "device": backend[0], "gpus": backend[1], "samples": samples,
            "resolution": list(resolution), "hdri": Path(hdri).name, "pose": list(pose),
            "ground": ground_mode}


# ------------------------------------------------------------ modeling tools

def tag_coords(mesh, coords=None):
    """Store part-local rest coordinates as the ``pcoord`` point attribute that
    procedural materials read (so shading follows each part after joins)."""
    if coords is None:
        coords = [v.co.copy() for v in mesh.vertices]
    attr = mesh.attributes.get("pcoord") or mesh.attributes.new("pcoord", "FLOAT_VECTOR", "POINT")
    attr.data.foreach_set("vector", [c for co in coords for c in co])
    return mesh


def tube(name, points, radius=0.01, sides=12, material=None, cap=True, radii=None, roll=0.0):
    """Sweep a circle along ``points`` (list of 3-tuples) with parallel-transport
    frames. ``radii`` gives a per-point radius (else constant ``radius``); a
    callable ``radius(t)`` with t in 0..1 also works. pcoord = (x, y, arclength)
    in the tube's own frame, so wood/cord/stem materials run along it."""
    pts = [Vector(p) for p in points]
    count = len(pts)
    if count < 2:
        raise ValueError("tube needs at least two points")
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    total = lengths[-1] or 1.0
    if radii is None:
        radii = [radius(l / total) if callable(radius) else radius for l in lengths]
    tangents = []
    for i in range(count):
        prev_pt = pts[max(i - 1, 0)]
        next_pt = pts[min(i + 1, count - 1)]
        tangents.append((next_pt - prev_pt).normalized())
    seed = Vector((0, 0, 1)) if abs(tangents[0].z) < 0.9 else Vector((1, 0, 0))
    normal = tangents[0].cross(seed).normalized()
    normals = []
    for i in range(count):
        if i:
            rotation = tangents[i - 1].rotation_difference(tangents[i])
            normal = (rotation @ normal).normalized()
        normals.append(normal.copy())
    verts, coords, faces, uvs = [], [], [], []
    for i in range(count):
        binormal = tangents[i].cross(normals[i]).normalized()
        for j in range(sides):
            angle = 2 * math.pi * j / sides + roll
            offset = normals[i] * math.cos(angle) + binormal * math.sin(angle)
            verts.append(pts[i] + offset * radii[i])
            coords.append((math.cos(angle) * radii[i], math.sin(angle) * radii[i], lengths[i]))
    for i in range(count - 1):
        for j in range(sides):
            a, b = i * sides + j, i * sides + (j + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    if cap:
        for index, ring in ((0, 0), (count - 1, (count - 1) * sides)):
            verts.append(pts[index])
            coords.append((0.0, 0.0, lengths[index]))
            center = len(verts) - 1
            for j in range(sides):
                a, b = ring + j, ring + (j + 1) % sides
                faces.append((center, b, a) if index == 0 else (center, a, b))
    data = bpy.data.meshes.new(name)
    data.from_pydata([tuple(v) for v in verts], [], faces)
    data.update()
    data.uv_layers.new(name="UVMap")
    tag_coords(data, coords)
    obj = _link(bpy.data.objects.new(name, data))
    if material is not None:
        data.materials.append(material)
    recalc_normals(obj)
    return obj


def assign_tube_uvs(obj, rect, sides, rings):
    """Map an un-joined ``kit.tube`` (``rings`` points x ``sides``) into a fixed
    atlas rect (u0, u1, v0, v1): u runs around, v along. Use with
    ``BAKE = {"repack": False}`` for foliage/cord atlases."""
    u0, u1, v0, v1 = rect
    layer = obj.data.uv_layers["UVMap"].data
    body_faces = (rings - 1) * sides
    for poly_index, poly in enumerate(obj.data.polygons):
        seam = poly_index < body_faces and poly_index % sides == sides - 1
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            ring, side = divmod(min(vertex_index, rings * sides - 1), sides)
            u = 1.0 if seam and side == 0 else side / sides
            v = ring / max(1, rings - 1)
            layer[loop_index].uv = (u0 + (u1 - u0) * u, v0 + (v1 - v0) * v)
    return obj


def warp(obj, fn):
    """Move every vertex: ``fn(co) -> new co`` in object space. pcoord is kept."""
    for vert in obj.data.vertices:
        vert.co = Vector(fn(vert.co.copy()))
    obj.data.update()
    return obj


def displace(obj, fn):
    """Offset vertices along their normals by ``fn(co, pcoord) -> meters``."""
    mesh = obj.data
    coords = [0.0] * (len(mesh.vertices) * 3)
    if "pcoord" in mesh.attributes:
        mesh.attributes["pcoord"].data.foreach_get("vector", coords)
    else:
        mesh.vertices.foreach_get("co", coords)
    for index, vert in enumerate(mesh.vertices):
        pco = Vector(coords[index * 3:index * 3 + 3])
        vert.co += vert.normal * fn(vert.co.copy(), pco)
    mesh.update()
    return obj


def apply_modifiers(obj):
    """Bake the modifier stack into the mesh data, keeping attributes."""
    if not obj.modifiers:
        return obj
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = obj.evaluated_get(depsgraph)
    data = bpy.data.meshes.new_from_object(evaluated)
    old = obj.data
    obj.modifiers.clear()
    obj.data = data
    if old.users == 0:
        bpy.data.meshes.remove(old)
    return obj


def subdivide(obj, levels=2, smooth=True):
    """Apply subdivision now (so later warps/displacements see the density)."""
    mod = obj.modifiers.new("Subdivision", "SUBSURF")
    mod.levels = mod.render_levels = levels
    mod.subdivision_type = "CATMULL_CLARK" if smooth else "SIMPLE"
    mod.uv_smooth = "PRESERVE_BOUNDARIES"
    return apply_modifiers(obj)


def pack_uvs(obj, margin=0.004):
    """Fresh non-overlapping UVs for baking: smart project + tight packing."""
    _select_only([obj])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=margin, scale_to_bounds=False)
    bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


# -------------------------------------------------------------------- baking

BAKE_MAPS = ("basecolor", "roughness", "normal", "ao")


def _bake_labelled(materials, role, margin):
    """Bake a custom scalar/color map: each material exposes it on a node labelled
    ``HOMESTEAD_<ROLE>`` (e.g. HOMESTEAD_HEIGHT), which is routed through an emission
    shader for the bake and then disconnected again."""
    label = "HOMESTEAD_" + role.upper()
    restore = []
    for mat in materials:
        tree = mat.node_tree
        source = next((n for n in tree.nodes if n.label == label), None)
        output = next((n for n in tree.nodes if n.type == "OUTPUT_MATERIAL" and n.is_active_output), None)
        if source is None or output is None:
            raise RuntimeError(f"{mat.name} has no node labelled {label} for the '{role}' bake")
        previous = [link.from_socket for link in output.inputs["Surface"].links]
        emission = tree.nodes.new("ShaderNodeEmission")
        tree.links.new(source.outputs[0], emission.inputs["Color"])
        tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
        restore.append((tree, output, emission, previous))
    bpy.ops.object.bake(type="EMIT", use_clear=True, margin=margin)
    for tree, output, emission, previous in restore:
        tree.nodes.remove(emission)
        for socket in previous:
            tree.links.new(socket, output.inputs["Surface"])


def bake(obj, folder, stem, size=2048, samples=96, maps=BAKE_MAPS, margin=16):
    """Bake the object's procedural materials into game textures on its UVMap
    and replace them with one image-based material ``M_<stem>``.
    Normal maps are tangent-space OpenGL (+Y); flip green for Unreal."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    device = use_gpu()
    scene.cycles.samples = samples
    scene.render.bake.margin = margin
    scene.render.bake.margin_type = "EXTEND"
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    _select_only([obj])
    materials = [s.material for s in obj.material_slots if s.material]
    images, paths = {}, {}
    for role in maps:
        image = bpy.data.images.new(f"T_{stem}_{role}", size, size, alpha=False)
        if role != "basecolor":
            image.colorspace_settings.name = "Non-Color"
        for mat in materials:
            node = mat.node_tree.nodes.new("ShaderNodeTexImage")
            node.image = image
            node.name = node.label = "HomesteadBakeTarget"
            mat.node_tree.nodes.active = node
        if role == "basecolor":
            bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, use_clear=True, margin=margin)
        elif role == "roughness":
            bpy.ops.object.bake(type="ROUGHNESS", use_clear=True, margin=margin)
        elif role == "normal":
            bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT", use_clear=True, margin=margin)
        elif role == "ao":
            bpy.ops.object.bake(type="AO", use_clear=True, margin=margin)
        else:
            _bake_labelled(materials, role, margin)
        for mat in materials:
            for node in [n for n in mat.node_tree.nodes if n.name.startswith("HomesteadBakeTarget")]:
                mat.node_tree.nodes.remove(node)
        path = folder / f"T_{stem}_{role}.png"
        image.filepath_raw = str(path)
        image.file_format = "PNG"
        image.save()
        images[role], paths[role] = image, path

    clash = bpy.data.materials.get("M_" + stem)
    if clash is not None:
        clash.name = "M_" + stem + "_Procedural"
    baked = bpy.data.materials.new("M_" + stem)
    if baked.node_tree is None:
        baked.use_nodes = True
    tree = baked.node_tree
    bsdf = tree.nodes["Principled BSDF"]
    def texture(role):
        node = tree.nodes.new("ShaderNodeTexImage")
        node.image = bpy.data.images.load(str(paths[role]), check_existing=False)
        if role != "basecolor":
            node.image.colorspace_settings.name = "Non-Color"
        return node
    tree.links.new(texture("basecolor").outputs["Color"], bsdf.inputs["Base Color"])
    if "roughness" in paths:
        tree.links.new(texture("roughness").outputs["Color"], bsdf.inputs["Roughness"])
    if "normal" in paths:
        normal_map = tree.nodes.new("ShaderNodeNormalMap")
        tree.links.new(texture("normal").outputs["Color"], normal_map.inputs["Color"])
        tree.links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])
    subsurface = max((m.node_tree.nodes.get("Principled BSDF") and
                      m.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value or 0.0)
                     for m in materials) if materials else 0.0
    bsdf.inputs["Subsurface Weight"].default_value = subsurface
    obj.data.materials.clear()
    obj.data.materials.append(baked)
    for image in images.values():
        bpy.data.images.remove(image)
    return {"size": size, "samples": samples, "device": device[0], "normal": "OpenGL",
            "maps": {role: path.name for role, path in paths.items()}}


# ---------------------------------------------------------------- references

def layer_detail(obj, folder, stem, tile=1.0, strength=0.8, bump=0.6, box_blend=0.25):
    """Layer shared tiling detail maps (``<folder>/T_<stem>_{basecolor,roughness,height}.png``)
    over an object's baked material, box-projected in object space at ``tile`` m per
    repeat: base colour x (2 x detail) at ``strength``, roughness nudged per mineral,
    and grain height as bump on top of the baked normal map. This is what the Unreal
    material should reproduce with world-aligned textures (see the recipe NOTES)."""
    folder = Path(folder)
    mat = obj.material_slots[0].material
    tree = mat.node_tree
    bsdf = next(n for n in tree.nodes if n.type == "BSDF_PRINCIPLED")
    coords = tree.nodes.new("ShaderNodeTexCoord")
    mapping = tree.nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (1.0 / tile,) * 3
    tree.links.new(coords.outputs["Object"], mapping.inputs["Vector"])

    def image(role, color):
        node = tree.nodes.new("ShaderNodeTexImage")
        node.image = bpy.data.images.load(str(folder / f"T_{stem}_{role}.png"), check_existing=True)
        if not color:
            node.image.colorspace_settings.name = "Non-Color"
        node.projection = "BOX"
        node.projection_blend = box_blend
        tree.links.new(mapping.outputs["Vector"], node.inputs["Vector"])
        return node.outputs["Color"]

    base_link = bsdf.inputs["Base Color"].links[0]
    doubled = tree.nodes.new("ShaderNodeVectorMath")
    doubled.operation = "SCALE"
    doubled.inputs["Scale"].default_value = 2.0
    tree.links.new(image("basecolor", True), doubled.inputs[0])
    mix = tree.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = strength
    colors = [s for s in mix.inputs if s.type == "RGBA"]
    tree.links.new(base_link.from_socket, colors[0])
    tree.links.new(doubled.outputs["Vector"], colors[1])
    tree.links.new(next(s for s in mix.outputs if s.type == "RGBA"), bsdf.inputs["Base Color"])

    rough_link = bsdf.inputs["Roughness"].links[0]
    rough_mix = tree.nodes.new("ShaderNodeMix")
    rough_mix.data_type = "FLOAT"
    rough_mix.inputs["Factor"].default_value = strength * 0.5
    values = [s for s in rough_mix.inputs if s.type == "VALUE"]
    tree.links.new(rough_link.from_socket, values[1])
    rough_detail = tree.nodes.new("ShaderNodeSeparateColor")
    tree.links.new(image("roughness", False), rough_detail.inputs["Color"])
    tree.links.new(rough_detail.outputs[0], values[2])
    tree.links.new(next(s for s in rough_mix.outputs if s.type == "VALUE"), bsdf.inputs["Roughness"])

    normal_link = bsdf.inputs["Normal"].links[0]
    height = tree.nodes.new("ShaderNodeSeparateColor")
    tree.links.new(image("height", False), height.inputs["Color"])
    bump_node = tree.nodes.new("ShaderNodeBump")
    bump_node.inputs["Strength"].default_value = bump
    bump_node.inputs["Distance"].default_value = 0.0007
    tree.links.new(height.outputs[0], bump_node.inputs["Height"])
    tree.links.new(normal_link.from_socket, bump_node.inputs["Normal"])
    tree.links.new(bump_node.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def reference_image(path, view="FRONT", height=1.0, opacity=0.5):
    """Show a reference photo/drawing as an image empty in the live viewport,
    standing on the ground behind the model (FRONT faces -Y, SIDE faces +X)."""
    image = bpy.data.images.load(str(path), check_existing=True)
    empty = bpy.data.objects.new("Reference_" + Path(path).stem, None)
    empty.empty_display_type = "IMAGE"
    empty.data = image
    empty.empty_display_size = height
    empty.empty_image_offset = (-0.5, 0.0)
    empty.color[3] = opacity
    empty.use_empty_image_alpha = True
    empty.rotation_euler = (math.radians(90), 0, math.radians(90) if view == "SIDE" else 0)
    empty.location = (0.6, 0, 0) if view == "SIDE" else (0, 0.6, 0)
    empty.hide_render = True
    _link(empty)
    return empty
