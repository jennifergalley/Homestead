"""Retarget Game Animation Sample (GASP) locomotion loops onto the MetaHuman heroine.

The GASP source clips live at their original paths under /Game/Characters/UEFN_Mannequin
(migrated from the Game Animation Sample project, see docs/asset-credits.md). Run inside the
editor, for example through the MCP ``run_python`` tool:

    from homestead_agent import gasp_locomotion as gl
    gl.build_retargeter()
    gl.retarget(gl.CLIPS)
    print(gl.measure('/Game/Characters/Heroine_MH/Animations/GASP/AN_HeroineMH_GASP_Sprint'))
"""
import unreal

SOURCE_RIG = '/Game/Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin'
SOURCE_MESH = '/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin'
TARGET_RIG = '/MetaHumanCharacter/Animation/Retargeting/IK_MH_IKRig'
TARGET_MESH = '/Game/Characters/Heroine_MH/Assembled/Heroine/Body/SKM_MHC_Heroine_BodyMesh'
RETARGETER = '/Game/Characters/Heroine_MH/Retarget/RTG_UEFN_To_HeroineMH'
OUTPUT = '/Game/Characters/Heroine_MH/Animations/GASP'

GASP = '/Game/Characters/UEFN_Mannequin/Animations/'
# source clip -> output name
CLIPS = {
    GASP + 'Sprint/M_Neutral_Sprint_Loop_F': 'AN_HeroineMH_GASP_Sprint',
    GASP + 'Sprint/M_Relaxed_Sprint_Loop_F': 'AN_HeroineMH_GASP_SprintRelaxed',
    GASP + 'Run/M_Neutral_Run_Loop_F': 'AN_HeroineMH_GASP_Run',
    GASP + 'Walk/M_Neutral_Walk_Loop_F': 'AN_HeroineMH_GASP_Walk',
    GASP + 'Walk/M_Relaxed_Walk_Loop_F': 'AN_HeroineMH_GASP_WalkRelaxed',
}


def _controller():
    return unreal.IKRetargeterController.get_controller(unreal.load_asset(RETARGETER))


def build_retargeter(disabled_ops=()):
    """Create (or reset) the retargeter with default ops and fuzzy chain mapping.

    ``disabled_ops`` holds op names to switch off (for example 'Run IK Rig').
    """
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(RETARGETER):
        rtg = unreal.load_asset(RETARGETER)
    else:
        folder, name = RETARGETER.rsplit('/', 1)
        rtg = tools.create_asset(name, folder, unreal.IKRetargeter, unreal.IKRetargetFactory())
    c = unreal.IKRetargeterController.get_controller(rtg)
    c.remove_all_ops()
    c.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, unreal.load_asset(SOURCE_RIG))
    c.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, unreal.load_asset(TARGET_RIG))
    c.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, unreal.load_asset(SOURCE_MESH))
    c.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, unreal.load_asset(TARGET_MESH))
    c.add_default_ops()
    c.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, unreal.load_asset(SOURCE_RIG))
    c.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, unreal.load_asset(TARGET_RIG))
    c.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    # IK_MH_IKRig's retarget root is the pelvis, so the Root Motion op would write the source's
    # forward travel onto the pelvis. Send it to the real root bone; the clips are then locked
    # in place on import (the capsule drives movement).
    root_op = c.get_index_of_op_by_name('Root Motion')
    if root_op >= 0:
        root = c.get_op_controller(root_op)
        root.set_target_root_bone('root')
        settings = root.get_settings()
        settings.root_motion_source = unreal.RootMotionSource.COPY_FROM_SOURCE_ROOT
        settings.root_height_source = unreal.RootMotionHeightSource.SNAP_TO_GROUND
        root.set_settings(settings)
    ops = []
    for i in range(c.get_num_retarget_ops()):
        name = str(c.get_op_name(i))
        enabled = name not in disabled_ops
        c.set_retarget_op_enabled(i, enabled)
        ops.append((name, enabled))
    unreal.EditorAssetLibrary.save_asset(RETARGETER, False)
    return ops


def retarget(clips=None):
    clips = clips or CLIPS
    if not unreal.EditorAssetLibrary.does_directory_exist(OUTPUT):
        unreal.EditorAssetLibrary.make_directory(OUTPUT)
    made = []
    for source, name in clips.items():
        target = f'{OUTPUT}/{name}'
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            unreal.EditorAssetLibrary.delete_asset(target)
        inputs = unreal.IKRetargetBatchOperationInputs()
        inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(source)]
        inputs.source_mesh = unreal.load_asset(SOURCE_MESH)
        inputs.target_mesh = unreal.load_asset(TARGET_MESH)
        inputs.ik_retarget_asset = unreal.load_asset(RETARGETER)
        inputs.target_path = OUTPUT
        inputs.use_source_path = False
        inputs.include_referenced_assets = False
        inputs.overwrite_existing_files = True
        inputs.prefix = ''
        inputs.suffix = '_HMH'
        results = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        produced = f"{OUTPUT}/{source.rsplit('/', 1)[1]}_HMH"
        if not unreal.EditorAssetLibrary.does_asset_exist(produced):
            raise RuntimeError(f'Retarget produced nothing for {source}: {results}')
        unreal.EditorAssetLibrary.rename_asset(produced, target)
        anim = unreal.load_asset(target)
        # GASP foley notifies reference sample-only audio and blueprints; the game plays its own steps.
        unreal.AnimationLibrary.remove_all_animation_notify_tracks(anim)
        # In place: the capsule drives movement, so lock the root and discard sequence root motion.
        unreal.AnimationLibrary.set_root_motion_enabled(anim, True)
        # GASP clips use a sample-only curve compression asset; use the engine default.
        anim.set_editor_property('curve_compression_settings',
                                 unreal.load_asset('/Engine/Animation/DefaultAnimCurveCompressionSettings'))
        unreal.EditorAssetLibrary.save_asset(target, False)
        made.append(target)
    return made


LEGACY_MESH = '/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave'
LEGACY_RETARGETER = '/Game/Characters/Heroine_MH/Retarget/RTG_HeroineLegacy_To_MH'
LEGACY_ANIMS = '/Game/SurvivalGame/Characters/Heroine/Animations/'
# Work actions and idle still come from the legacy heroine (task 6.1/6.2 replaces them).
LEGACY_CLIPS = {
    'AN_Heroine_LivingIdle02': 'AN_HeroineMH_LivingIdle02',
    'AN_Heroine_GroundedWalk': 'AN_HeroineMH_GroundedWalk',
    'AN_Heroine_Sprint': 'AN_HeroineMH_Sprint',
    'AN_Heroine_Gather': 'AN_HeroineMH_Gather',
    'AN_Heroine_WaterRefined': 'AN_HeroineMH_WaterRefined',
    'AN_Heroine_Chop': 'AN_HeroineMH_Chop',
    'AN_Heroine_KnifeCut': 'AN_HeroineMH_KnifeCut',
    'AN_Heroine_Till': 'AN_HeroineMH_Till',
}


def retarget_legacy(clips=None, anim_root=LEGACY_ANIMS):
    """Re-run the legacy-clip retarget (for example after the MetaHuman body changes).

    RTG_HeroineLegacy_To_MH must keep Run IK Rig and Root Motion disabled; see the editor skill.
    Source clips are looked up by name under ``anim_root`` and its subfolders.
    """
    clips = clips or LEGACY_CLIPS
    folder = '/Game/Characters/Heroine_MH/Animations'
    found = {str(unreal.EditorAssetLibrary.find_asset_data(p).asset_name): p.split('.')[0]
             for p in unreal.EditorAssetLibrary.list_assets('/Game/SurvivalGame/Characters/Heroine', True, False)}
    found.update({str(unreal.EditorAssetLibrary.find_asset_data(p).asset_name): p.split('.')[0]
                  for p in unreal.EditorAssetLibrary.list_assets('/Game/Trials', True, False)
                  if 'AN_Heroine' in p})
    made = []
    for source_name, name in clips.items():
        source = found.get(source_name)
        if not source:
            raise RuntimeError(f'Legacy clip {source_name} not found')
        target = f'{folder}/{name}'
        inputs = unreal.IKRetargetBatchOperationInputs()
        inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(source)]
        inputs.source_mesh = unreal.load_asset(LEGACY_MESH)
        inputs.target_mesh = unreal.load_asset(TARGET_MESH)
        inputs.ik_retarget_asset = unreal.load_asset(LEGACY_RETARGETER)
        inputs.target_path = folder
        inputs.use_source_path = False
        inputs.overwrite_existing_files = True
        inputs.suffix = '_HMH'
        unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        produced = f'{folder}/{source_name}_HMH'
        if not unreal.EditorAssetLibrary.does_asset_exist(produced):
            raise RuntimeError(f'Retarget produced nothing for {source}')
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            unreal.EditorAssetLibrary.delete_asset(target)
        unreal.EditorAssetLibrary.rename_asset(produced, target)
        unreal.EditorAssetLibrary.save_asset(target, False)
        made.append(target)
    return made


def _component_positions(anim, bones, time):
    skeleton = anim.get_editor_property('skeleton')
    ref = unreal.load_asset(TARGET_MESH)
    positions = {}
    for bone in bones:
        path = unreal.AnimationLibrary.find_bone_path_to_root(anim, bone)
        transform = unreal.Transform()
        for name in path:
            local = unreal.AnimationLibrary.get_bone_pose_for_time(anim, name, time, False)
            transform = unreal.MathLibrary.compose_transforms(transform, local)
        positions[bone] = transform.translation
    return positions


def measure(asset, samples=6, bones=('root', 'pelvis', 'foot_l', 'foot_r', 'head')):
    """Component-space bone positions (cm) over the clip, to catch broken or drifting poses."""
    anim = unreal.load_asset(asset)
    length = anim.get_play_length()
    rows = []
    for k in range(samples):
        t = length * k / samples
        p = _component_positions(anim, bones, t)
        rows.append((round(t, 3), {b: (round(v.x, 1), round(v.y, 1), round(v.z, 1)) for b, v in p.items()}))
    return {'length': round(length, 3), 'samples': rows}
