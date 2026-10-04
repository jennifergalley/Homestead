"""Read-only PIE evidence: from homestead_agent import estate_environment_audit as audit; audit.snapshot(x, y).

Run on an owned Estate fixture only. Reports flower transforms near a test square and persistent fence
instance/material state; it neither mutates gameplay nor saves assets. Not a performance benchmark.
"""
import hashlib
import json
import math

import unreal

FLOWER_MESHES = {
    "SM_BluebellClump", "SM_PrimroseClump", "SM_WildGarlic", "SM_WoodAnemoneClump",
    "SM_RedCampionClump", "SM_Foxglove", "SM_CowParsley",
}
FENCE_PREFIX = "SM_FarmFence"
DEFAULT_REACH_CM = 500.0


def _materials(component: unreal.InstancedStaticMeshComponent) -> list:
    rows = []
    for index in range(component.get_num_materials()):
        material = component.get_material(index)
        if not material:
            raise RuntimeError(f"{component.get_path_name()}: material {index} is missing")
        base = material.get_base_material()
        if not base:
            raise RuntimeError(f"{component.get_path_name()}: material {index} has no base")
        rows.append({
            "material": material.get_path_name(),
            "base": base.get_path_name(),
            "instanced": base.get_editor_property("used_with_instanced_static_meshes"),
            "nanite": base.get_editor_property("used_with_nanite"),
            "blend": str(base.get_editor_property("blend_mode")),
        })
    return rows


def snapshot(x: float, y: float, reach_cm: float = DEFAULT_REACH_CM) -> dict:
    """One bounded inspection, for before/after till, return and save/load comparisons."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError("Owned PIE world is not running")
    flowers, fences, beds, flower_materials = [], [], [], {}
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    for actor in actors:
        for component in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
            if not mesh:
                continue
            name = mesh.get_name()
            if name not in FLOWER_MESHES and not name.startswith(FENCE_PREFIX):
                continue
            flower = name in FLOWER_MESHES
            if flower and name not in flower_materials:
                flower_materials[name] = _materials(component)
            bounds = mesh.get_bounds()
            radius = math.hypot(abs(bounds.origin.x) + bounds.box_extent.x,
                                abs(bounds.origin.y) + bounds.box_extent.y)
            transforms = []
            for index in range(component.get_instance_count()):
                transform = component.get_instance_transform(index, True)
                position = transform.translation
                scale = transform.scale3d
                if flower and math.hypot(position.x - x, position.y - y) > reach_cm:
                    continue
                row = {"mesh": name, "component": component.get_name(), "instance": index,
                       "x": round(position.x, 3), "y": round(position.y, 3),
                       "z": round(position.z, 3), "scale": round(scale.x, 6),
                       "radius_cm": round(radius * scale.x + 5.0, 3)}
                if flower:
                    flowers.append(row)
                else:
                    rotation = transform.rotation
                    transforms.append({
                        "position": [round(v, 3) for v in (position.x, position.y, position.z)],
                        "rotation": [round(v, 6) for v in (rotation.x, rotation.y, rotation.z, rotation.w)],
                        "scale": [round(v, 6) for v in (scale.x, scale.y, scale.z)],
                    })
            if not flower:
                fences.append({
                    "mesh": name, "instances": len(transforms),
                    "visible": component.is_visible(),
                    "hidden": component.get_editor_property("hidden_in_game"),
                    "start_cull": component.get_editor_property("instance_start_cull_distance"),
                    "end_cull": component.get_editor_property("instance_end_cull_distance"),
                    "materials": _materials(component),
                    "transform_hash": hashlib.sha256(json.dumps(transforms, sort_keys=True).encode()).hexdigest(),
                })
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
            if mesh and "TilledBed" in mesh.get_name():
                position = component.get_world_location()
                if math.hypot(position.x - x, position.y - y) <= reach_cm:
                    beds.append({"x": round(position.x, 3), "y": round(position.y, 3)})
    result = {"flowers": flowers, "flower_materials": flower_materials, "fences": fences, "beds": beds}
    print(json.dumps({"flowers": len(flowers), "visible_flowers": sum(f["scale"] > 0.001 for f in flowers),
                      "fence_batches": len(fences), "beds": beds}, sort_keys=True))
    return result
