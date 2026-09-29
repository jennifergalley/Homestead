"""Test grove for Blender-built trees on the Estate, driven from a running editor over MCP.

    sys.path.insert(0, r'<repo>\\Scripts\\Blender'); import tree_grove_test as G
    G.editor_setup(['/Game/SurvivalGame/Environment/Trees/Oak/SM_Oak'])   # before PIE (editor world)
    # ...start PIE, close the book...
    G.populate(x, y, count=2000, spacing=900)        # in PIE: HISM instances traced onto the landscape
    G.look(x, y, z, pitch, yaw)                      # in PIE: free camera view target
    G.show(False)                                    # hide the grove for a stat unit baseline

The grove actor and camera are not spatially loaded, so PIE copies them from the unsaved editor
world. Nothing is saved: don't save the Estate map afterwards (discard the edit or restart).
"""
import math
import random

import unreal

TAG = "TreeGroveTest"
CAM_TAG = "TreeGroveCam"


def _editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def _game_world():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError("PIE is not running")
    return world


def editor_setup(meshes):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if actor.tags and (TAG in [str(t) for t in actor.tags] or CAM_TAG in [str(t) for t in actor.tags]):
            actors.destroy_actor(actor)
    grove = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    grove.set_actor_label("TreeGroveTest")
    grove.tags = [TAG]
    grove.set_editor_property("is_spatially_loaded", False)
    subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = subobjects.k2_gather_subobject_data_for_instance(grove)[0]
    for path in meshes:
        mesh = unreal.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Missing mesh " + path)
        params = unreal.AddNewSubobjectParams(parent_handle=root,
                                              new_class=unreal.HierarchicalInstancedStaticMeshComponent,
                                              blueprint_context=None)
        handle, reason = subobjects.add_new_subobject(params)
        if not unreal.SubobjectDataBlueprintFunctionLibrary.is_handle_valid(handle):
            raise RuntimeError(f"Could not add HISM for {path}: {reason}")
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        component.set_static_mesh(mesh)
        component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0, 0, 20000))
    camera.set_actor_label("TreeGroveCam")
    camera.tags = [CAM_TAG]
    camera.set_editor_property("is_spatially_loaded", False)
    return [c.get_name() for c in grove.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)]


def _tagged(world, tag):
    found = unreal.GameplayStatics.get_all_actors_with_tag(world, tag)
    if not found:
        raise RuntimeError(f"No actor tagged {tag} in PIE; run editor_setup before starting PIE")
    return found[0]


def ground(world, x, y):
    """Landscape height at (x, y), ignoring trees, rocks and buildings."""
    hits = unreal.SystemLibrary.line_trace_multi(
        world, unreal.Vector(x, y, 60000), unreal.Vector(x, y, -20000),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    for hit in hits or []:
        values = hit.to_tuple()
        actor = values[9]
        if actor and "Landscape" in actor.get_class().get_name():
            return values[4].z
    return None


def populate(cx, cy, count=2000, spacing=900.0, seed=7, scale=(0.85, 1.15), sink=45.0):
    """Jittered grid of ``count`` trees round (cx, cy), shared across the grove's HISMs.
    ``sink``: the pivot is 30 cm under the ground line; a little extra buries the root flare."""
    world = _game_world()
    grove = _tagged(world, TAG)
    components = list(grove.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent))
    for component in components:
        component.clear_instances()
    rng = random.Random(seed)
    side = int(math.ceil(math.sqrt(count)))
    placed = [[] for _ in components]
    missed = 0
    for i in range(side * side):
        if sum(len(p) for p in placed) >= count:
            break
        gx, gy = i % side - side / 2.0, i // side - side / 2.0
        x = cx + (gx + rng.uniform(-0.35, 0.35)) * spacing
        y = cy + (gy + rng.uniform(-0.35, 0.35)) * spacing
        z = ground(world, x, y)
        if z is None:
            missed += 1
            continue
        s = rng.uniform(*scale)
        xf = unreal.Transform(unreal.Vector(x, y, z - sink * s), unreal.Rotator(0, 0, rng.uniform(0, 360)),
                              unreal.Vector(s, s, s))
        placed[i % len(components)].append(xf)
    for component, transforms in zip(components, placed):
        if transforms:
            component.add_instances(transforms, False, True)
    return {"instances": [c.get_instance_count() for c in components], "missed": missed}


def look(x, y, z, pitch, yaw, fov=70.0):
    world = _game_world()
    camera = _tagged(world, CAM_TAG)
    camera.set_actor_location_and_rotation(unreal.Vector(x, y, z), unreal.Rotator(0, pitch, yaw), False, True)
    camera.camera_component.set_editor_property("field_of_view", fov)
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    controller.set_view_target_with_blend(camera, 0.0)
    return "ok"


def show(visible=True):
    grove = _tagged(_game_world(), TAG)
    grove.set_actor_hidden_in_game(not visible)
    return "shown" if visible else "hidden"
