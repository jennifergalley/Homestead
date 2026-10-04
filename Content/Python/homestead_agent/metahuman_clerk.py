"""Scripted MetaHuman Creator look for the general-store clerk, Mr. Josiah Trethewey.

Run inside the editor (MCP ``run_python``), one step per call, for example::

    from homestead_agent import metahuman_clerk as mc
    mc.create()          # duplicate the stock Walter preset (Unreal Engine EULA) once
    mc.set_grooms()      # hair, whiskers, no moustache
    mc.apply_colours()   # a later call: groom parameters appear only after the editor ticks

Then rig (``request_auto_rigging``; needs a cached Epic login, else Creator's "Create Full Rig"),
request 2K textures and assemble with ``assemble()``. Close the clerk's Creator window first;
scripted edits don't show in an open one.
"""

import unreal

from homestead_agent import metahuman_look as ml

PRESET = "/MetaHumanCharacter/Optional/Presets/Walter"
CLERK = "/Game/Characters/Clerk_MH/MHC_Clerk"
ASSEMBLED = "/Game/Characters/Clerk_MH/Assembled"
COMMON = "/Game/Characters/Clerk_MH/Common"
BINDINGS = "/MetaHumanCharacter/Optional/Grooms/Bindings"

# Balance (2026-10-04): greying brown hair, mutton-chop whiskers greyer than the hair, no moustache.
GROOMS = {
    # RecedeMessy read nearer seventy and untidy; the swept-back cut reads mid-fifties and proud.
    "Hair": "Hair/WI_Hair_S_SlickBack",
    "Beard": "Beards/WI_Beard_M_MuttonChops",
    "Eyebrows": "Eyebrows/WI_Eyebrows_M_Dense",
    "Mustache": None,
    # Walter's stock outfit is replaced by original Blender garments; peach fuzz isn't worth a
    # strand groom on one NPC seen at counter distance.
    "Outfits": None,
    "Peachfuzz": None,
}
HAIR_COLOUR = {"Melanin": 0.6, "Redness": 0.3, "Whiteness": 0.6}
WHISKER_COLOUR = {"Melanin": 0.45, "Redness": 0.2, "Whiteness": 0.8}
BROW_COLOUR = {"Melanin": 0.6, "Redness": 0.3, "Whiteness": 0.35}
COLOURS = {"Hair": HAIR_COLOUR, "Beard": WHISKER_COLOUR, "Eyebrows": BROW_COLOUR, "Eyelashes": BROW_COLOUR}
TEXTURE_RESOLUTION = "RES2K"


def create():
    if not unreal.EditorAssetLibrary.does_asset_exist(CLERK):
        unreal.EditorAssetLibrary.duplicate_asset(PRESET, CLERK)
    character = unreal.load_asset(CLERK)
    unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character


def set_grooms(grooms=GROOMS):
    character = unreal.load_asset(CLERK)
    ml.close_editors(character)
    sub = ml._edit(character)
    try:
        collection = sub.get_preview_collection(character)
        for slot, item_path in grooms.items():
            if item_path is None:
                collection.default_instance.set_single_slot_selection(slot, unreal.MetaHumanPaletteItemKey())
                continue
            item = unreal.load_asset("%s/%s" % (BINDINGS, item_path))
            keys = list(collection.get_item_keys_for_wardrobe_item(item))
            key = keys[0] if keys else collection.try_add_item_from_wardrobe_item(slot_name=slot, wardrobe_item=item)
            collection.default_instance.set_single_slot_selection(slot, key)
        sub.on_edit_preview_collection(character)
    finally:
        ml._end_edit(character)
    unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character


def groom_parameters():
    """{slot: [parameter names]} for the current selections, to check what apply_colours can set."""
    character = unreal.load_asset(CLERK)
    sub = ml._edit(character)
    try:
        collection = sub.get_preview_collection(character)
        sub.assemble_for_preview(character=character)
        result = {}
        for slot, key in ml._selected_items(collection).items():
            params = collection.default_instance.get_instance_parameters(
                item_path=unreal.MetaHumanPaletteItemPath(item_key=key))
            result[slot] = [str(p.name) for p in params]
        return result
    finally:
        ml._end_edit(character)


def apply_colours(colours=COLOURS):
    character = unreal.load_asset(CLERK)
    ml.close_editors(character)
    sub = ml._edit(character)
    try:
        collection = sub.get_preview_collection(character)
        sub.assemble_for_preview(character=character)
        selected = ml._selected_items(collection)
        for slot, values in colours.items():
            if slot in selected:
                ml._set_params(collection.default_instance, selected[slot], values)
        sub.on_edit_preview_collection(character)
    finally:
        ml._end_edit(character)
    unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character


def set_texture_resolution(resolution=TEXTURE_RESOLUTION):
    """Check the desired texture source resolutions (stock default is 2K, the one-NPC budget).

    `skin_settings` is read-only from Python; change it in Creator's Materials tab if needed.
    """
    character = unreal.load_asset(CLERK)
    resolutions = character.skin_settings.desired_texture_sources_resolutions
    fields = ("face_albedo", "face_normal", "face_cavity", "face_animated_maps",
              "body_albedo", "body_normal", "body_cavity", "body_masks")
    wrong = [f for f in fields
             if str(resolutions.get_editor_property(f)).upper().split(".")[-1] != resolution.upper()]
    if wrong:
        unreal.log_warning(f"Clerk texture sources not at {resolution}: {wrong}")
    return resolutions


def rig():
    """Auto-rig (joints and blend shapes) and fetch the 2K texture sources; both are async."""
    character = unreal.load_asset(CLERK)
    ml.close_editors(character)
    sub = ml._edit(character)
    params = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    params.set_editor_property("rig_type", unreal.MetaHumanRigType.JOINTS_AND_BLEND_SHAPES)
    sub.request_auto_rigging(character, params)
    # Call textures() once the log says "Auto-Rigging finished".
    return character


def textures():
    character = unreal.load_asset(CLERK)
    sub = ml._edit(character)
    sub.request_texture_sources(character, unreal.MetaHumanCharacterTextureRequestParams())
    return character


def assemble(quality="HIGH"):
    """UE Optimized assembly into ASSEMBLED/Clerk with its own Common, then save (build doesn't)."""
    character = unreal.load_asset(CLERK)
    sub = ml._edit(character)
    params = unreal.MetaHumanCharacterEditorBuildParameters()
    params.pipeline_type = type(params.pipeline_type).OPTIMIZED
    params.pipeline_quality = getattr(type(params.pipeline_quality), quality)
    params.absolute_build_path = ASSEMBLED
    params.common_folder_path = COMMON
    params.name_override = "Clerk"
    try:
        sub.build_meta_human(character, params)
    except RuntimeError as error:
        # Control Rig "Cannot break link" is raised even when the log says the build succeeded.
        unreal.log_warning("build_meta_human raised %s; check the log" % error)
    finally:
        ml._end_edit(character)
    for folder in (ASSEMBLED, COMMON):
        unreal.EditorAssetLibrary.save_directory(folder, False, True)
    unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return unreal.EditorAssetLibrary.list_assets(ASSEMBLED, recursive=True)
