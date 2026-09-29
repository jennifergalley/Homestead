"""Retarget Game Animation Sample (GASP) locomotion loops onto the MetaHuman heroine.

The GASP source clips live at their original paths under /Game/Characters/UEFN_Mannequin
(migrated from the Game Animation Sample project, see docs/asset-credits.md). Run inside the
editor, for example through the MCP ``run_python`` tool:

    from homestead_agent import gasp_locomotion as gl
    gl.build_retargeter()
    gl.retarget(gl.CLIPS)
    print(gl.measure('/Game/Characters/Heroine_MH/Animations/GASP/AN_HeroineMH_GASP_Sprint'))
    print(gl.heel_report('/Game/Characters/Heroine_MH/Animations/GASP/AN_HeroineMH_GASP_Run'))
"""
import math

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


def build_retargeter(disabled_ops=('Root Motion',)):
    """Create (or reset) the retargeter with default ops and fuzzy chain mapping.

    ``disabled_ops`` holds op names to switch off. The Root Motion op stays off by default: with
    IK_MH_IKRig it writes a root that snakes sideways and yaws by up to 15 degrees per step, and
    because the game locks the root, that turned into a head-and-shoulder wobble in play.
    ``retarget`` rebuilds a straight root track instead (see ``straighten_root``).
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
        straighten_root(anim)
        if any(word in name for word in HEEL_EASE_CLIPS):
            ease_heel_kick(anim)
        add_footstep_notifies(anim)
        # In place: the capsule drives movement, so lock the root and discard sequence root motion.
        unreal.AnimationLibrary.set_root_motion_enabled(anim, True)
        # GASP clips use a sample-only curve compression asset; use the engine default.
        anim.set_editor_property('curve_compression_settings',
                                 unreal.load_asset('/Engine/Animation/DefaultAnimCurveCompressionSettings'))
        unreal.EditorAssetLibrary.save_asset(target, False)
        made.append(target)
    return made


def straighten_root(anim):
    """Move a loop's forward travel from the pelvis onto a straight, unrotated root track.

    Without the Root Motion op the retargeted root stays at the origin and the pelvis carries the
    travel. The root becomes a constant-velocity line through the pelvis path, so locking the
    root in game keeps only the natural pelvis sway. Only the pelvis is keyed under the root.
    """
    lib = unreal.AnimationLibrary
    frames = lib.get_num_keys(anim)
    children = [str(n) for n in lib.get_animation_track_names(anim)
                if list(lib.find_bone_path_to_root(anim, n))[1:2] == ['root']]
    if children != ['pelvis']:
        raise RuntimeError(f'Expected only the pelvis under the root, found {children}')
    root = [lib.get_bone_pose_for_frame(anim, 'root', i, False) for i in range(frames)]
    if any(abs(r.translation.x) + abs(r.translation.y) > 0.01
           or abs(r.rotation.rotator().yaw) > 0.01 for r in root):
        raise RuntimeError('straighten_root expects the root at the origin (Root Motion op off)')
    pelvis = [lib.get_bone_pose_for_frame(anim, 'pelvis', i, False) for i in range(frames)]
    first, last = pelvis[0].translation, pelvis[-1].translation
    step_x = (last.x - first.x) / max(frames - 1, 1)
    step_y = (last.y - first.y) / max(frames - 1, 1)
    offset_x = sum(p.translation.x - step_x * i for i, p in enumerate(pelvis)) / frames
    offset_y = sum(p.translation.y - step_y * i for i, p in enumerate(pelvis)) / frames
    root_keys, pelvis_keys = [], []
    for i, p in enumerate(pelvis):
        x, y = offset_x + step_x * i, offset_y + step_y * i
        root_keys.append(unreal.Vector(x, y, 0.0))
        pelvis_keys.append(unreal.Vector(p.translation.x - x, p.translation.y - y, p.translation.z))
    ctl = anim.get_editor_property('controller')
    ctl.open_bracket(unreal.Text('Straighten root'))
    ctl.set_bone_track_keys('root', root_keys, [unreal.Quat()] * frames, [unreal.Vector(1, 1, 1)] * frames)
    rotations, scales = [p.rotation for p in pelvis], [p.scale3d for p in pelvis]
    ctl.set_bone_track_keys('pelvis', pelvis_keys, rotations, scales)
    # Evaluation adds the skeleton-vs-retarget-source pelvis offset to raw keys (about -10.7 cm in
    # z for this heroine), so measure it and write keys that evaluate to the intended pose.
    shift = lib.get_bone_pose_for_frame(anim, 'pelvis', 0, False).translation - pelvis_keys[0]
    pelvis_keys = [k - shift for k in pelvis_keys]
    ctl.set_bone_track_keys('pelvis', pelvis_keys, rotations, scales)
    ctl.close_bracket()
    return {'frames': frames, 'speed_cm_s': round(
        (step_x ** 2 + step_y ** 2) ** 0.5 * (frames - 1) / max(anim.get_play_length(), 1e-3), 1)}


# MetaHuman standing reference heights (cm): ankle joint and ball of the foot above the floor.
ANKLE_HEIGHT = 8.6
BALL_HEIGHT = 1.1
FOOTSTEP_TRACK = 'Footsteps'


def footstep_contacts(anim, touch_cm=1.5, lift_cm=6.0):
    """Touchdown times of each foot in a locomotion loop: [(seconds, 'l' | 'r'), ...].

    A foot touches down when the lower of its heel (ankle minus standing height) and ball comes
    within ``touch_cm`` of the floor after having lifted more than ``lift_cm``.
    """
    lib = unreal.AnimationLibrary
    frames = lib.get_num_keys(anim)
    length = anim.get_play_length()
    heights = {'l': [], 'r': []}
    for i in range(frames - 1):  # the last key repeats the first in a loop
        t = length * i / (frames - 1)
        p = _component_positions(anim, ('foot_l', 'ball_l', 'foot_r', 'ball_r'), t)
        for side in 'lr':
            heights[side].append(min(p[f'foot_{side}'].z - ANKLE_HEIGHT, p[f'ball_{side}'].z - BALL_HEIGHT))
    contacts = []
    for side, h in heights.items():
        n = len(h)
        lifted = False
        for k in range(2 * n):  # two passes so a touchdown across the loop seam is found
            z = h[k % n]
            if z > lift_cm:
                lifted = True
            elif lifted and z <= touch_cm:
                lifted = False
                if k >= n:
                    contacts.append((round(length * (k - n) / (frames - 1), 4), side))
    return sorted(contacts)


def add_footstep_notifies(anim):
    """Replace the clip's footstep track with UHomesteadFootstepNotify events at each touchdown."""
    lib = unreal.AnimationLibrary
    if FOOTSTEP_TRACK in [str(n) for n in lib.get_animation_notify_track_names(anim)]:
        lib.remove_animation_notify_track(anim, FOOTSTEP_TRACK)
    lib.add_animation_notify_track(anim, FOOTSTEP_TRACK)
    run = any(word in anim.get_name() for word in ('Run', 'Sprint'))
    contacts = footstep_contacts(anim)
    for t, side in contacts:
        notify = lib.add_animation_notify_event(anim, FOOTSTEP_TRACK, t, unreal.HomesteadFootstepNotify)
        notify.set_editor_property('left_foot', side == 'l')
        notify.set_editor_property('run', run)
    return contacts


# --- Heel kick (Jenny's playtest: "her feet kick slightly too high up, near her butt") -----------
# GASP's neutral run folds the trailing knee to about 120 degrees in swing, so the heel curls up
# toward the glutes. ease_heel_kick opens the knee a little, only where it is folded past
# HEEL_EASE_START_DEG: stance, contact and toe-off never fold that far, so foot placement, stride,
# cadence, the pelvis and the loop seam are untouched. The thigh is kept, so the knee drives
# forward exactly as before and only the lower leg swings a little lower. The foot keeps its
# local pose on the calf.
HEEL_EASE_CLIPS = ('Run', 'Sprint')
HEEL_EASE_START_DEG = 70.0   # knee flexion (0 = straight) below which nothing changes
HEEL_EASE_KEEP = 0.80        # share of the flexion beyond the start that is kept at the peak
HEEL_EASE_SOFT_DEG = 10.0    # blend width, so the easing fades in with no kink at the start
HEEL_EASE_TAG = 'HomesteadHeelEase'


def _q(t):
    return (t.w, t.x, t.y, t.z)


def _qmul(a, b):
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return (aw * bw - ax * bx - ay * by - az * bz, aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw)


def _qinv(q):
    return (q[0], -q[1], -q[2], -q[3])


def _qrot(q, v):
    w, x, y, z = _qmul(_qmul(q, (0.0, v[0], v[1], v[2])), _qinv(q))
    return (x, y, z)


def _qaxis(axis, angle):
    s = math.sin(angle / 2)
    return (math.cos(angle / 2), axis[0] * s, axis[1] * s, axis[2] * s)


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _norm(a):
    length = sum(c * c for c in a) ** 0.5
    return tuple(c / length for c in a) if length > 1e-9 else (0.0, 0.0, 0.0)


def _angle(a, b):
    a, b = _norm(a), _norm(b)
    return math.degrees(math.acos(max(-1.0, min(1.0, sum(x * y for x, y in zip(a, b))))))


def _chain(anim, frame, leaf):
    """Component-space (rotation, position) of every bone from the root down to ``leaf``."""
    lib = unreal.AnimationLibrary
    path = list(lib.find_bone_path_to_root(anim, leaf))[::-1]   # root first
    rot, pos, out = (1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 0.0), {}
    for name in path:
        local = lib.get_bone_pose_for_frame(anim, str(name), frame, False)
        t = local.translation
        offset = _qrot(rot, (t.x, t.y, t.z))
        pos = (pos[0] + offset[0], pos[1] + offset[1], pos[2] + offset[2])
        rot = _qmul(rot, _q(local.rotation))
        out[str(name)] = (rot, pos)
    return out


def _eased_flexion(theta):
    excess = theta - HEEL_EASE_START_DEG
    if excess <= 0:
        return theta
    return theta - (1 - HEEL_EASE_KEEP) * excess * excess / (excess + HEEL_EASE_SOFT_DEG)


def ease_heel_kick(anim, force=False):
    """Open the swing knee of a run loop so the heel stays further from her seat (see above)."""
    lib = unreal.AnimationLibrary
    tag = unreal.EditorAssetLibrary.get_metadata_tag(anim, HEEL_EASE_TAG)
    if tag and not force:
        raise RuntimeError(f'{anim.get_name()} already eased ({tag}); re-retarget it or pass force=True')
    frames = lib.get_num_keys(anim)
    changed = {}
    for side in 'lr':
        calf = f'calf_{side}'
        positions, rotations, scales, peak = [], [], [], 0.0
        for i in range(frames):
            bones = _chain(anim, i, f'foot_{side}')
            thigh_rot, hip = bones[f'thigh_{side}']
            knee, ankle = bones[calf][1], bones[f'foot_{side}'][1]
            upper, lower = _sub(knee, hip), _sub(ankle, knee)
            theta = _angle(upper, lower)
            local = lib.get_bone_pose_for_frame(anim, calf, i, False)
            rotation = _q(local.rotation)
            ease = theta - _eased_flexion(theta)
            if ease > 1e-4:
                # Rotate the lower leg toward the thigh's line about the knee hinge, in thigh space.
                hinge = _qrot(_qinv(thigh_rot), _norm(_cross(upper, lower)))
                rotation = _qmul(_qaxis(hinge, -math.radians(ease)), rotation)
                peak = max(peak, ease)
            positions.append(local.translation)
            rotations.append(unreal.Quat(rotation[1], rotation[2], rotation[3], rotation[0]))
            scales.append(local.scale3d)
        changed[calf] = (positions, rotations, scales, round(peak, 2))
    ctl = anim.get_editor_property('controller')
    ctl.open_bracket(unreal.Text('Ease heel kick'))
    for calf, (positions, rotations, scales, _) in changed.items():
        ctl.set_bone_track_keys(calf, positions, rotations, scales)
    # As in straighten_root, evaluation adds the retarget offset to raw translation keys, so writing
    # back the evaluated translations lengthens the shin. Measure that shift and write keys that
    # evaluate to the original bone translations.
    for calf, (positions, rotations, scales, _) in changed.items():
        shifted = [lib.get_bone_pose_for_frame(anim, calf, i, False).translation for i in range(frames)]
        fixed = [p - (s - p) for p, s in zip(positions, shifted)]
        ctl.set_bone_track_keys(calf, fixed, rotations, scales)
    ctl.close_bracket()
    for calf, (positions, _, _, _) in changed.items():
        worst = max((lib.get_bone_pose_for_frame(anim, calf, i, False).translation - positions[i]).length()
                    for i in range(frames))
        if worst > 0.01:
            raise RuntimeError(f'{calf} translation drifted {worst:.3f} cm after easing')
    params = f'start={HEEL_EASE_START_DEG} keep={HEEL_EASE_KEEP} soft={HEEL_EASE_SOFT_DEG}'
    unreal.EditorAssetLibrary.set_metadata_tag(anim, HEEL_EASE_TAG, params)
    return {calf: value[3] for calf, value in changed.items()}


def heel_report(asset):
    """Swing-phase heel measurements of a locomotion loop (cm, component space, floor at z=0).

    peak_heel_cm: highest ankle; min_heel_below_pelvis_cm: closest the ankle comes up under the
    pelvis; min_heel_to_seat_cm: closest the ankle comes to a point 12 cm behind and 8 cm below the
    pelvis (her seat); max_knee_deg: peak knee flexion; min_toe_cm: lowest ball of the foot
    above its standing height while the ankle is up in swing (toe clearance, should stay > 0).
    """
    anim = unreal.load_asset(asset) if isinstance(asset, str) else asset
    lib = unreal.AnimationLibrary
    frames = lib.get_num_keys(anim)
    root_first = lib.get_bone_pose_for_frame(anim, 'root', 0, False).translation
    root_last = lib.get_bone_pose_for_frame(anim, 'root', frames - 1, False).translation
    forward = _norm((root_last.x - root_first.x, root_last.y - root_first.y, 0.0))
    if forward == (0.0, 0.0, 0.0):
        forward = (0.0, 1.0, 0.0)
    out = {}
    for side in 'lr':
        peak, below, seat, knee_max, toe = 0.0, 1e9, 1e9, 0.0, 1e9
        for i in range(frames):
            bones = _chain(anim, i, f'ball_{side}')
            pelvis = bones['pelvis'][1]
            hip, knee = bones[f'thigh_{side}'][1], bones[f'calf_{side}'][1]
            ankle, ball = bones[f'foot_{side}'][1], bones[f'ball_{side}'][1]
            seat_point = (pelvis[0] - forward[0] * 12, pelvis[1] - forward[1] * 12, pelvis[2] - 8)
            peak = max(peak, ankle[2])
            below = min(below, pelvis[2] - ankle[2])
            seat = min(seat, math.dist(ankle, seat_point))
            knee_max = max(knee_max, _angle(_sub(knee, hip), _sub(ankle, knee)))
            if ankle[2] - ANKLE_HEIGHT > 6.0:
                toe = min(toe, ball[2] - BALL_HEIGHT)
        out[side] = {'peak_heel_cm': round(peak, 1), 'min_heel_below_pelvis_cm': round(below, 1),
                     'min_heel_to_seat_cm': round(seat, 1), 'max_knee_deg': round(knee_max, 1),
                     'min_toe_cm': round(toe, 1)}
    return out


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
