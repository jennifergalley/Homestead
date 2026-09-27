"""MetaHuman hairstyles for the heroine: copies stock MetaHuman grooms into the project and binds
them to her face.

    from homestead_agent import metahuman_hair as mh
    mh.prepare()                 # every style in STYLES (skips ones already prepared)
    mh.prepare(['Hair_S_Pixie']) # or just some

Each groom is copied from the MetaHuman Creator plugin's library
(``/MetaHumanCharacter/Optional/Grooms/GroomAssets/Hair/<Name>``, authored on the plugin's
``SKM_Groom_Head_Legacy01``) into ``GROOMS/<Name>``, its material slots pointed at the heroine's
shared hair materials, and a binding ``<Name>_Binding`` built for her face mesh with the plugin
head as the transfer source, so the roots follow her own head shape. The runtime list lives in
``HomesteadLook::MetaHairStyles`` (HomesteadAppearance.cpp); keep the two in the same order.
"""
import unreal

PLUGIN = '/MetaHumanCharacter/Optional/Grooms/GroomAssets/Hair'
SOURCE_HEAD = '/MetaHumanCharacter/Optional/Grooms/GroomMesh/SKM_Groom_Head_Legacy01'
GROOMS = '/Game/Characters/Heroine_MH/Common/Optional/Grooms/GroomAssets/Hair'
FACE = '/Game/Characters/Heroine_MH/Assembled/Heroine/Face/SKM_MHC_Heroine_FaceMesh'
MATERIALS = '/Game/Characters/Heroine_MH/Common/Materials'

# Same order as HomesteadLook::MetaHairStyles. Hair_L_Straight is the assembled groom already in
# Assembled/Heroine/Grooms, so it is not copied here.
STYLES = [
    'Hair_L_Straight', 'Hair_M_BobStraight', 'Hair_S_LowPonytail', 'Hair_S_UpdoBraids',
    'Hair_S_UpdoBuns', 'Hair_L_MessyClumps', 'Hair_M_BobCurly', 'Hair_L_StraightBangs', 'Hair_S_Pixie',
]
EAL = unreal.EditorAssetLibrary


def _local(asset, name):
    """The project copy of a plugin sub-asset of groom ``name`` (or the asset itself)."""
    if not asset:
        return asset
    path = asset.get_path_name().split('.')[0]
    src = f'{PLUGIN}/{name}'
    if path.startswith(src):
        copy = unreal.load_asset(f'{GROOMS}/{name}' + path[len(src):])
        if copy:
            return copy
    return asset


def _copy(name):
    src, dst = f'{PLUGIN}/{name}', f'{GROOMS}/{name}'
    if not EAL.does_directory_exist(dst) or not EAL.does_asset_exist(f'{dst}/{name}'):
        if not EAL.duplicate_directory(src, dst):
            raise RuntimeError(f'Could not copy {src}')
    groom = unreal.load_asset(f'{dst}/{name}')
    # The copied groom still points at the plugin's cards meshes, atlases and helmet meshes, and
    # at the plugin's hair materials: point them at the copies and the heroine's materials.
    # Struct arrays come back as copies: edit each element, write it back, then set the array.
    cards = groom.get_editor_property('hair_groups_cards')
    for i in range(len(cards)):
        card = cards[i]
        card.set_editor_property('imported_mesh', _local(card.get_editor_property('imported_mesh'), name))
        textures = card.get_editor_property('textures')
        textures.set_editor_property('textures', [_local(x, name) for x in textures.get_editor_property('textures')])
        card.set_editor_property('textures', textures)
        cards[i] = card
    groom.set_editor_property('hair_groups_cards', cards)
    meshes = groom.get_editor_property('hair_groups_meshes')
    for i in range(len(meshes)):
        mesh = meshes[i]
        mesh.set_editor_property('imported_mesh', _local(mesh.get_editor_property('imported_mesh'), name))
        meshes[i] = mesh
    groom.set_editor_property('hair_groups_meshes', meshes)
    materials = groom.get_editor_property('hair_groups_materials')
    for i in range(len(materials)):
        slot = materials[i]
        local = unreal.load_asset(f'{MATERIALS}/{slot.get_editor_property("slot_name")}')
        if local:
            slot.set_editor_property('material', local)
        materials[i] = slot
    groom.set_editor_property('hair_groups_materials', materials)
    return groom

def _binding(name, groom):
    path = f'{GROOMS}/{name}/{name}_Binding'
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    face = unreal.load_asset(FACE)
    head = unreal.load_asset(SOURCE_HEAD)
    binding = unreal.GroomLibrary.create_new_groom_binding_asset_with_path(path, groom, face, 100, head, 0)
    if not binding:
        raise RuntimeError(f'Binding failed for {name}')
    return binding


def prepare(names=None):
    done = []
    for name in names or STYLES[1:]:
        groom = _copy(name)
        binding = _binding(name, groom)
        for asset in (groom, binding):
            EAL.save_loaded_asset(asset, False)
        done.append((name, groom.get_path_name(), binding.get_path_name()))
    return done
