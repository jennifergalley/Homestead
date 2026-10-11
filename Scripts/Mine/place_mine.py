"""Place the mine house and the shallow level in the Estate level (run inside the editor, not PIE).

    . .\\Scripts\\McpHelpers.ps1 -Port <p>; pyfile Scripts\\Mine\\place_mine.py

Idempotent: removes every actor tagged ``HomesteadMine`` and places them again from
``Scripts/Blender/Recipes/mine/layout.py``, preferring the detailed meshes and falling back to the
blockouts while those don't exist. Actors go in the Outliner folder ``Mine`` and are saved as
external actors (commit only those, plus the folder's ``__ExternalObjects__`` package). The mine's
nodes are not placed here: Mine Gameplay spawns them from ``HomesteadMineLayout.inc``.
"""
import sys
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
import importlib  # noqa: E402

from mine import layout as L  # noqa: E402

importlib.reload(L)

TAG = "HomesteadMine"
FOLDER = "Mine"
PROPS = "/Game/SurvivalGame/Environment/Props"
# (label, [preferred mesh, fallback], frame) - frame "house" or "tunnel".
PIECES = [
    ("MineHouse", [f"{PROPS}/MineHouse/SM_MineHouse", f"{PROPS}/MineHouseBlockout/SM_MineHouseBlockout"], "house"),
    ("MineTunnels", [f"{PROPS}/MineTunnels/SM_MineTunnels", f"{PROPS}/MineTunnelsBlockout/SM_MineTunnelsBlockout"], "tunnel"),
]
DESK_LAMP_LIGHT = {"intensity_cd": 18.0, "radius_cm": 650.0, "temperature_k": 2100.0}


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def remove_old(actors):
    removed = 0
    for actor in actors.get_all_level_actors():
        if actor.actor_has_tag(TAG):
            actors.destroy_actor(actor)
            removed += 1
    return removed


def spawn_mesh(actors, label, mesh, location_m, yaw):
    loc = unreal.Vector(*(v * 100.0 for v in location_m))
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    actor.set_actor_label(label)
    actor.tags = [TAG]
    actor.set_folder_path(FOLDER)
    return actor


def main():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    print("removed", remove_old(actors))
    placed = []
    for label, paths, frame in PIECES:
        mesh = next((m for m in (unreal.load_asset(p) for p in paths if unreal.EditorAssetLibrary.does_asset_exist(p)) if m), None)
        if not mesh:
            print("skip", label, "(not imported)")
            continue
        origin = L.house_to_world(0, 0, 0) if frame == "house" else L.tunnel_world()
        placed.append(spawn_mesh(actors, label, mesh, origin, L.HOUSE_YAW))
        print("placed", label, mesh.get_path_name(), origin)
    # The desk lamp's light: it burns forever (the lamp mesh itself is part of the house).
    lx, ly, lz = L.house_to_world(*L.LAMP_ON_DESK)
    light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(lx * 100, ly * 100, lz * 100 + 22.0))
    comp = light.point_light_component
    comp.set_intensity_units(unreal.LightUnits.CANDELAS)
    comp.set_intensity(DESK_LAMP_LIGHT["intensity_cd"])
    comp.set_attenuation_radius(DESK_LAMP_LIGHT["radius_cm"])
    comp.set_use_temperature(True)
    comp.set_temperature(DESK_LAMP_LIGHT["temperature_k"])
    comp.set_source_radius(2.5)
    light.set_actor_label("MineDeskLampLight")
    light.tags = [TAG]
    light.set_folder_path(FOLDER)
    placed.append(light)
    packages = [a.get_outermost() for a in placed]
    unreal.EditorLoadingAndSavingUtils.save_packages(packages, False)
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    folders = [p for p in dirty if "__ExternalObjects__" in p.get_name()]
    if folders:
        unreal.EditorLoadingAndSavingUtils.save_packages(folders, False)
    print("saved", len(packages), "actors;", "folder packages:", [p.get_name() for p in folders])


main()
