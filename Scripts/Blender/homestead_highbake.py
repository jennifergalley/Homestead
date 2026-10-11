"""High-to-low texture baking for Homestead props built from scratch.

A recipe models a dense, displaced ``high`` mesh with a procedural material (vertex
attributes and shader noise), decimates a game ``low`` mesh from the same surface, unwraps
it, and bakes the high mesh's shading onto it with a Cycles selected-to-active bake:

    import homestead_highbake as hb
    info = hb.bake(kit, high, low, folder, "DriftwoodLog", size=4096, samples=64,
                   extrusion=0.02, distance=0.10)

This writes ``T_<stem>_{basecolor,roughness,normal,ao}.png`` (normal: tangent space,
OpenGL +Y, like ``kit.bake``) to ``folder`` and gives ``low`` one image material
``M_<stem>`` with the same wiring as ``kit.bake``, so ``import_props.py`` treats it like
any baked prop. Use it with ``BAKE = None`` in the recipe (the builder must not re-bake),
assign the returned material to the LODs yourself and delete ``high`` before returning.

The low mesh is made invisible to every ray type during the bake so it can't shadow the
AO it is receiving; every other mesh in the scene is hidden from render too.
"""
from pathlib import Path

import bpy

MAPS = ("basecolor", "roughness", "normal", "ao")


def bake(kit, high, low, folder, stem, size=2048, samples=64, maps=MAPS, extrusion=0.02, distance=0.1,
         margin=16):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    device = kit.use_gpu()
    scene.cycles.samples = samples
    settings = scene.render.bake
    settings.margin = margin
    settings.margin_type = "EXTEND"
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    highs = high if isinstance(high, (list, tuple)) else [high]

    hidden = []
    for obj in scene.objects:
        if obj is not low and obj not in highs and not obj.hide_render:
            obj.hide_render = True
            hidden.append(obj)
    rays = ("visible_camera", "visible_diffuse", "visible_glossy", "visible_transmission",
            "visible_volume_scatter", "visible_shadow")
    saved_rays = {r: getattr(low, r) for r in rays}
    for r in rays:
        setattr(low, r, False)

    target = bpy.data.materials.new("M_" + stem + "_BakeTarget")
    target.use_nodes = True
    low.data.materials.clear()
    low.data.materials.append(target)
    for obj in scene.objects:
        obj.select_set(False)
    for obj in highs:
        obj.select_set(True)
    low.select_set(True)
    bpy.context.view_layer.objects.active = low

    paths = {}
    try:
        for role in maps:
            image = bpy.data.images.new(f"T_{stem}_{role}", size, size, alpha=False)
            if role != "basecolor":
                image.colorspace_settings.name = "Non-Color"
            node = target.node_tree.nodes.new("ShaderNodeTexImage")
            node.image = image
            target.node_tree.nodes.active = node
            common = dict(use_selected_to_active=True, cage_extrusion=extrusion, max_ray_distance=distance,
                          use_clear=True, margin=margin)
            print(f"HOMESTEAD_HIGHBAKE {stem} {role} {size}px", flush=True)
            if role == "basecolor":
                bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, **common)
            elif role == "roughness":
                bpy.ops.object.bake(type="ROUGHNESS", **common)
            elif role == "normal":
                bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT", **common)
            elif role == "ao":
                bpy.ops.object.bake(type="AO", **common)
            else:
                raise ValueError("Unsupported high-to-low bake role " + role)
            path = folder / f"T_{stem}_{role}.png"
            image.filepath_raw = str(path)
            image.file_format = "PNG"
            image.save()
            paths[role] = path
            target.node_tree.nodes.remove(node)
            bpy.data.images.remove(image)
    finally:
        for obj in hidden:
            obj.hide_render = False
        for r, value in saved_rays.items():
            setattr(low, r, value)

    baked = bpy.data.materials.get("M_" + stem) or bpy.data.materials.new("M_" + stem)
    baked.use_nodes = True
    tree = baked.node_tree
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputMaterial")
    bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    tree.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

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
    if "ao" in paths:
        # Reference only: Unreal reads the AO map into its own slot; Blender's review keeps it unwired.
        texture("ao")
    bsdf.inputs["Subsurface Weight"].default_value = 0.0
    low.data.materials.clear()
    low.data.materials.append(baked)
    bpy.data.materials.remove(target)
    return baked, {"size": size, "samples": samples, "device": device[0], "normal": "OpenGL",
                   "method": "high-to-low selected-to-active", "maps": {r: p.name for r, p in paths.items()}}
