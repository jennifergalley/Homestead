"""Place the mine house, the shallow level and their dressing in the Estate level (run inside the
editor, not PIE).

    . .\\Scripts\\McpHelpers.ps1 -Port <p>; pyfile Scripts\\Mine\\place_mine.py

Idempotent and stable: every actor carries the tag ``HomesteadMine`` and a fixed label; existing
actors (loaded from World Partition first, so none are missed after an editor restart) are updated
in place, new ones are spawned, and tagged actors no longer wanted are deleted. Positions come from
``Scripts/Blender/Recipes/mine/layout.py``. The detailed meshes are used when imported, the blockouts
otherwise. Actors go in the Outliner folder ``Mine`` as external actors (commit only those, plus the
folder's ``__ExternalObjects__`` package). The mine's nodes are not placed here: Mine Gameplay spawns
them from ``HomesteadMineLayout.inc``.
"""
import importlib
import sys
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
from mine import layout as L  # noqa: E402

importlib.reload(L)

TAG = "HomesteadMine"
FOLDER = "Mine"
PROPS = L.PROPS
HOUSE_SET, HOUSE_BLOCKOUT = f"{PROPS}/MineHouse", f"{PROPS}/MineHouseBlockout/SM_MineHouseBlockout"
TUNNELS, TUNNELS_BLOCKOUT = f"{PROPS}/MineTunnels/SM_MineTunnels", f"{PROPS}/MineTunnelsBlockout/SM_MineTunnelsBlockout"
FURNISHING_SET = f"{PROPS}/MineFurnishing"
GLASS_MATERIAL = f"{PROPS}/OilLamp/MI_OilLampGlass"
FLAME_MATERIAL = f"{PROPS}/OilLamp/MI_OilLampFlame"
DESK_LAMP_LIGHT = {"intensity_cd": 18.0, "radius_cm": 650.0, "temperature_k": 2100.0}


def exists(path):
    return unreal.EditorAssetLibrary.does_asset_exist(path)


def set_meshes(folder, prefix):
    """The non-LOD static meshes of an imported asset set, by name."""
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        return []
    out = []
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=False):
        name = path.rsplit("/", 1)[-1].split(".")[0]
        if name.startswith(prefix) and "_LOD" not in name:
            asset = unreal.load_asset(path)
            if isinstance(asset, unreal.StaticMesh):
                out.append((name, asset))
    return sorted(out, key=lambda item: item[0])


def wanted():
    """[(label, mesh, world location m, yaw)]."""
    out = []
    house = L.house_to_world(0, 0, 0)
    meshes = set_meshes(HOUSE_SET, "SM_MineHouse")
    if meshes:
        out += [(name[3:], mesh, house, L.HOUSE_YAW) for name, mesh in meshes]
    elif exists(HOUSE_BLOCKOUT):
        out.append(("MineHouse", unreal.load_asset(HOUSE_BLOCKOUT), house, L.HOUSE_YAW))
    tunnel_mesh = TUNNELS if exists(TUNNELS) else (TUNNELS_BLOCKOUT if exists(TUNNELS_BLOCKOUT) else None)
    if tunnel_mesh:
        out.append(("MineTunnels", unreal.load_asset(tunnel_mesh), L.tunnel_world(), L.HOUSE_YAW))
    for label, path, frame, (x, y, z), yaw in L.dressing():
        if not exists(path):
            print("skip", label, "(not imported)", path)
            continue
        loc = L.house_to_world(x, y, z) if frame == "house" else L.tunnel_to_world(x, y, z)
        out.append((label, unreal.load_asset(path), loc, yaw + L.HOUSE_YAW))
    return out


def load_mine_actors():
    hx, hy, _ = L.house_to_world(0, 0)
    box = unreal.Box(unreal.Vector(hx * 100 - 2500, hy * 100 - 8000, -5000),
                     unreal.Vector(hx * 100 + 9000, hy * 100 + 4000, 12000))
    descs = unreal.WorldPartitionBlueprintLibrary.get_intersecting_actor_descs(box)
    guids = [d.get_editor_property("guid") for d in descs if str(d.get_editor_property("label")).startswith("Mine")]
    if guids:
        unreal.WorldPartitionBlueprintLibrary.load_actors(guids)


def main():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    load_mine_actors()
    existing = {}
    for actor in actors.get_all_level_actors():
        if actor.actor_has_tag(TAG):
            label = actor.get_actor_label()
            if label in existing:          # an older duplicate
                actors.destroy_actor(existing[label])
            existing[label] = actor
    keep, touched = set(), []
    for label, mesh, (x, y, z), yaw in wanted():
        loc = unreal.Vector(x * 100.0, y * 100.0, z * 100.0)
        rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
        actor = existing.get(label)
        if actor is None or not isinstance(actor, unreal.StaticMeshActor):
            if actor is not None:
                actors.destroy_actor(actor)
            actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
            actor.set_actor_label(label)
            actor.tags = [TAG]
            actor.set_folder_path(FOLDER)
        else:
            actor.modify()
            actor.set_actor_location_and_rotation(loc, rot, False, True)
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        comp.set_mobility(unreal.ComponentMobility.STATIC)
        for i in range(comp.get_num_materials()):
            comp.set_material(i, None)
        if label == "MineHouseGlass" and exists(GLASS_MATERIAL):
            comp.set_material(0, unreal.load_asset(GLASS_MATERIAL))
        if label in ("MineDeskOilLampGlass",) and exists(GLASS_MATERIAL):
            comp.set_material(0, unreal.load_asset(GLASS_MATERIAL))
        if label == "MineDeskOilLampFlame" and exists(FLAME_MATERIAL):
            comp.set_material(0, unreal.load_asset(FLAME_MATERIAL))
        keep.add(label)
        touched.append(actor)
    # The desk lamp's light: it burns forever.
    lx, ly, lz = L.house_to_world(*L.LAMP_ON_DESK)
    loc = unreal.Vector(lx * 100, ly * 100, lz * 100 + 22.0)
    light = existing.get("MineDeskLampLight")
    if light is None:
        light = actors.spawn_actor_from_class(unreal.PointLight, loc)
        light.set_actor_label("MineDeskLampLight")
        light.tags = [TAG]
        light.set_folder_path(FOLDER)
    else:
        light.modify()
        light.set_actor_location(loc, False, True)
    comp = light.point_light_component
    comp.set_intensity_units(unreal.LightUnits.CANDELAS)
    comp.set_intensity(DESK_LAMP_LIGHT["intensity_cd"])
    comp.set_attenuation_radius(DESK_LAMP_LIGHT["radius_cm"])
    comp.set_use_temperature(True)
    comp.set_temperature(DESK_LAMP_LIGHT["temperature_k"])
    comp.set_source_radius(2.5)
    keep.add("MineDeskLampLight")
    touched.append(light)
    removed = []
    for label, actor in existing.items():
        if label not in keep:
            removed.append(actor.get_outermost().get_name())
            actors.destroy_actor(actor)
    packages = [a.get_outermost() for a in touched]
    dirty = [p for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
             if p.get_name() in removed or "__ExternalObjects__" in p.get_name()]
    unreal.EditorLoadingAndSavingUtils.save_packages(packages + dirty, False)
    print(f"mine: {len(touched)} actors placed or updated, {len(removed)} removed")


main()
