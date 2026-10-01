"""Author MetaHuman heroine animations by keying her body Control Rig in Sequencer, then baking.

Run inside the editor (MCP ``run_python``):

    from homestead_agent import rig_authoring as ra
    s = ra.Session('LS_KneelGatherSticks')          # sequence + authoring actor + Control Rig track
    s.key_bool(0, 'arm_r_fk_ik_switch', True)          # IK arm
    s.key_world(0, 'hand_r_ik_ctrl', (-20, 40, 60))    # component-space target (cm)
    s.bake('AN_HeroineMH_KneelGatherSticks')
    ra.bone_positions(anim, ('hand_r', 'pelvis'), 0.5)

Heroine component space: forward is +Y, her left is +X, up is +Z; the floor is z = 0.
"""
import unreal

AUTHORING = '/Game/Characters/Heroine_MH/Animations/Authoring'
OUTPUT = '/Game/Characters/Heroine_MH/Animations'
BODY = '/Game/Characters/Heroine_MH/Assembled/Heroine/Body/SKM_MHC_Heroine_BodyMesh'
RIG = '/Game/Characters/Heroine_MH/Common/Common/MetaHuman_ControlRig'
ACTOR_LABEL = 'HeroineRigAuthoring'
FPS = 30
L = unreal.ControlRigSequencerLibrary


def T(x, y, z, pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), unreal.Vector(1, 1, 1))


def _editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


class Session:
    """A level sequence with the heroine body and a MetaHuman Control Rig track, reset on open."""

    def __init__(self, name, frames=120, reset=True):
        if not unreal.EditorAssetLibrary.does_directory_exist(AUTHORING):
            unreal.EditorAssetLibrary.make_directory(AUTHORING)
        path = f'{AUTHORING}/{name}'
        if reset and unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.LevelSequenceEditorBlueprintLibrary.close_level_sequence()
            unreal.EditorAssetLibrary.delete_asset(path)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        self.sequence = (unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path)
                         else tools.create_asset(name, AUTHORING, unreal.LevelSequence, unreal.LevelSequenceFactoryNew()))
        self.sequence.set_display_rate(unreal.FrameRate(FPS, 1))
        self.sequence.set_playback_start(0)
        self.sequence.set_playback_end(frames)
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actor = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == ACTOR_LABEL), None)
        if not actor:
            actor = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0))
            actor.set_actor_label(ACTOR_LABEL)
        actor.skeletal_mesh_component.set_skinned_asset_and_update(unreal.load_asset(BODY))
        self.actor = actor
        bindings = [b for b in self.sequence.get_bindings() if str(b.get_display_name()) == ACTOR_LABEL]
        self.binding = bindings[0] if bindings else self.sequence.add_possessable(actor)
        unreal.LevelSequenceEditorBlueprintLibrary.open_level_sequence(self.sequence)
        rig_class = unreal.load_asset(RIG).generated_class()
        L.find_or_create_control_rig_track(_editor_world(), self.sequence, rig_class, self.binding)
        self.rig = L.get_control_rigs(self.sequence)[0].control_rig
        self.frames = frames
        self._rest = {}
        # Evaluate once so the rig's construction runs on this body; offsets then match her
        # proportions rather than the rig's default skeleton.
        editor = unreal.LevelSequenceEditorBlueprintLibrary
        editor.set_current_time(1)
        editor.set_current_time(0)
        editor.refresh_current_level_sequence()

    def rest(self, control):
        """Control's component-space offset (its transform when its local value is zero)."""
        if control not in self._rest:
            key = unreal.RigElementKey(unreal.RigElementType.CONTROL, control)
            self._rest[control] = self.rig.get_hierarchy().get_global_control_offset_transform(key, False)
        return self._rest[control]

    def bone(self, name):
        """Bone's component-space transform in the evaluated rest pose."""
        key = unreal.RigElementKey(unreal.RigElementType.BONE, name)
        return self.rig.get_hierarchy().get_global_transform(key, False)

    def hand_turn(self, side, fingers, palm):
        """Extra world rotation for ``key_world`` that turns a hand so its fingers point along
        ``fingers`` and its palm faces ``palm`` (component-space direction vectors)."""
        hand = self.bone(f'hand_{side}').translation
        middle = self.bone(f'middle_01_{side}').translation
        index = self.bone(f'index_01_{side}').translation
        pinky = self.bone(f'pinky_01_{side}').translation
        rest_fingers = (middle - hand).normal()
        across = (index - pinky).normal()
        # Palm normal: for the right hand index->pinky x fingers points out of the palm; mirrored on the left.
        rest_palm = rest_fingers.cross(across) if side == 'r' else across.cross(rest_fingers)
        rest_frame = unreal.MathLibrary.make_rot_from_xz(rest_fingers, rest_palm).quaternion()
        target = unreal.MathLibrary.make_rot_from_xz(unreal.Vector(*fingers).normal(), unreal.Vector(*palm).normal()).quaternion()
        return (target * rest_frame.inversed()).rotator()

    def key_world(self, frame, control, location, rotation=None):
        """Key a control to a component-space location. ``rotation`` is an extra world rotation
        (Rotator) applied on top of the control's rest orientation, or None to keep it.
        Valid for controls whose parent space doesn't move (IK hands/feet, pole vectors, body)."""
        rest = self.rest(control)
        rot = rest.rotation if rotation is None else (rotation.quaternion() * rest.rotation)
        world = unreal.Transform(unreal.Vector(*location), rot.rotator(), unreal.Vector(1, 1, 1))
        local = unreal.MathLibrary.make_relative_transform(world, rest)
        self.key_euler(frame, control, local.translation, local.rotation.rotator())

    def _channels(self):
        if not hasattr(self, '_channel_map'):
            section = self.binding.get_tracks()[0].get_sections()[0]
            self._channel_map = {str(c.channel_name): c for c in section.get_all_channels()}
        return self._channel_map

    def _add(self, name, frame, value):
        # The library's set_local_control_rig_* calls don't key from Python here, so write channels.
        channel = self._channels().get(name)
        if channel is None:
            raise KeyError(f'No channel {name}')
        channel.add_key(unreal.FrameNumber(int(frame)), value, 0.0, unreal.MovieSceneTimeUnit.DISPLAY_RATE,
                        unreal.MovieSceneKeyInterpolation.AUTO)

    def key_euler(self, frame, control, location=(0, 0, 0), rotation=None):
        """Key an Euler-transform control's local value (relative to its rest/offset)."""
        loc = location if isinstance(location, unreal.Vector) else unreal.Vector(*location)
        rot = rotation if isinstance(rotation, unreal.Rotator) else unreal.Rotator(*(rotation or (0, 0, 0)))
        for axis, value in zip('XYZ', (loc.x, loc.y, loc.z)):
            self._add(f'{control}.Location.{axis}', frame, float(value))
        for axis, value in zip('XYZ', (rot.roll, rot.pitch, rot.yaw)):
            self._add(f'{control}.Rotation.{axis}', frame, float(value))

    def key_rotation(self, frame, control, roll=0.0, pitch=0.0, yaw=0.0):
        """Local rotation of an FK-style control relative to its rest (degrees)."""
        self.key_euler(frame, control, (0, 0, 0), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))

    def key_bool(self, frame, control, value):
        self._channels()[control].add_key(unreal.FrameNumber(int(frame)), bool(value), 0.0,
                                          unreal.MovieSceneTimeUnit.DISPLAY_RATE)

    def key_float(self, frame, control, value):
        self._add(control, frame, float(value))

    def bake(self, anim_name, folder=OUTPUT, events=None, contacts=None):
        """Bake the sequence onto a new AnimSequence for the heroine skeleton; returns it. The baked clip is
        then checked against human joint limits (joint_limits, realistic-animation skill) and the summary
        logged: ``events`` is the recipe's FRAMES dict, ``contacts`` its strike or impact frames."""
        path = f'{folder}/{anim_name}'
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
        factory = unreal.AnimSequenceFactory()
        factory.set_editor_property('target_skeleton', unreal.load_asset(BODY).skeleton)
        anim = unreal.AssetToolsHelpers.get_asset_tools().create_asset(anim_name, folder, unreal.AnimSequence, factory)
        options = unreal.AnimSeqExportOption()
        if not unreal.SequencerTools.export_anim_sequence(_editor_world(), self.sequence, anim, options,
                                                          self.binding, False):
            raise RuntimeError(f'Bake failed for {path}')
        anim.set_editor_property('curve_compression_settings',
                                 unreal.load_asset('/Engine/Animation/DefaultAnimCurveCompressionSettings'))
        unreal.EditorAssetLibrary.save_loaded_asset(anim, False)
        unreal.EditorAssetLibrary.save_loaded_asset(self.sequence, False)
        try:
            from homestead_agent import joint_limits
            for line in joint_limits.report(anim, events=events, contacts=contacts):
                unreal.log(f'[anatomy {anim_name}] {line}')
        except Exception as error:  # the check never blocks a bake
            unreal.log_warning(f'[anatomy {anim_name}] check skipped: {error}')
        return anim


def bone_positions(anim, bones, time):
    """Component-space bone locations (cm) of a baked clip at ``time`` seconds."""
    lib = unreal.AnimationLibrary
    out = {}
    for bone in bones:
        transform = unreal.Transform()
        for name in lib.find_bone_path_to_root(anim, bone):
            transform = unreal.MathLibrary.compose_transforms(transform, lib.get_bone_pose_for_time(anim, name, time, False))
        out[bone] = transform
    return out
