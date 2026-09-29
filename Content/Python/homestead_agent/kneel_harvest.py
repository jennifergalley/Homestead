"""Kneel-and-pull crop harvest for the MetaHuman heroine: a root crop or cabbage lifted two-handed.

    from homestead_agent import kneel_harvest as kh
    anim = kh.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelHarvest

She steps her left foot forward and drops onto her right knee like the other kneeling gathers,
leans in and takes the crop's leafy crown in both fists, works it loose with two short tugs, then
leans back and draws it up out of the ground. She lifts it to her chest to look at it, and her right
hand carries it down to the pouch at her hip while her left comes back to her knee. Then she rises.
The game shows the produce between both fists from EVENTS['pulled'] and hides it at EVENTS['stowed']
(AHomesteadCharacter::HarvestPulled / HarvestStowed). The ripe plant stays in the ground until
'pulled'. Beans and berries are picked with the pouch clip (kneel_pouch) instead.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import kneel_pouch as kp

SEQUENCE = 'LS_KneelHarvest'
ANIM = 'AN_HeroineMH_KneelHarvest'

FRAMES = {
    'stand': 0, 'step': 8, 'kneel': 20, 'reach': 28, 'grip': 36, 'tug1': 42, 'tug2': 48,
    'pulled': 56, 'lift': 66, 'look': 74, 'hold': 82, 'stow': 96, 'out': 102, 'rise': 112, 'end': 122,
}
EVENTS = {'grip': FRAMES['grip'] / 30, 'pulled': FRAMES['pulled'] / 30, 'stowed': FRAMES['stow'] / 30}
# The crop's crown (where both fists close) relative to her standing pose; the game settles her so
# the plot's middle lands here (AHomesteadCharacter::HarvestCrownForward / HarvestCrownRight).
CROWN = (-6.0, 36.0, 4.0)
# Each wrist sits this far from the grip point: above it and to its own side, fingers down round it.
WRIST_FROM_GRIP_R = (-5.0, -2.0, 10.0)
WRIST_FROM_GRIP_L = (5.0, -2.0, 10.0)
# The crown's path: out of the ground, up to her chest, held there a moment.
PULLED = (-5.0, 32.0, 26.0)
LIFTED = (-3.0, 28.0, 74.0)
LOOK = (-2.0, 30.0, 80.0)


def build():
    """Bake twice: the second pass keys the pouch reach in the pelvis's baked frame."""
    return _author(_author(None))


def _author(pelvis_anim):
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    # Forward lean: in over the crop, back as she pulls, upright to look, a little in to stow.
    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 30, F['reach']: 66, F['grip']: 76, F['tug1']: 70,
            F['tug2']: 74, F['pulled']: 30, F['lift']: 16, F['look']: 12, F['hold']: 14, F['stow']: 22,
            F['out']: 24, F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    # The pull: her weight rocks back onto the heel of the kneeling leg.
    shift = {'reach': (0, 2.0, -4.0), 'grip': (0, 3.0, -7.0), 'tug1': (0, 0, -5.0), 'tug2': (0, 2.0, -7.0), 'pulled': (0, -6.0, 2.0),
             'lift': (0, -3.0, 2.0)}
    for name in ('kneel', 'reach', 'grip', 'tug1', 'tug2', 'pulled', 'lift', 'look', 'hold', 'stow', 'out'):
        s.key_world(F[name], 'body_ctrl', kg._add(kg.BODY_KNEEL, shift.get(name, (0, 0, 0))), tilt(F[name]))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet as the stick gather: left steps forward, right slides back onto tucked toes.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['out'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['out'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 2, 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Spine follows the lean; her head stays on the crop, then drops to the pouch as she stows.
    twist = {F['stow']: -8}
    for frame, deg in lean.items():
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, yaw=twist.get(frame, 0) * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15)
        look_down = 14 if frame in (F['lift'], F['look'], F['hold']) else 8 if frame in twist else 0
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.12 + look_down, yaw=twist.get(frame, 0))

    pelvis_cache = {}

    def pelvis(frame):
        if pelvis_anim is None:
            return s.bone('pelvis')
        if frame not in pelvis_cache:
            pelvis_cache[frame] = ra.bone_positions(pelvis_anim, ('pelvis',), frame / 30)['pelvis']
        return pelvis_cache[frame]

    pelvis_rest = pelvis(F['stand'])

    def on_pelvis(frame, standing_point):
        local = pelvis_rest.inverse_transform_location(unreal.Vector(*standing_point))
        v = pelvis(frame).transform_location(local)
        return (v.x, v.y, v.z)

    def pelvis_turn(frame, rotator):
        delta = pelvis(frame).rotation * pelvis_rest.rotation.inversed()
        return (delta * rotator.quaternion()).rotator()

    down = (0, 0, -1)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    # Fists round the crown: fingers down and a little forward, palms facing each other.
    grip_r = s.hand_turn('r', (0.15, 0.35, -0.9), (1, 0, 0))
    grip_l = s.hand_turn('l', (-0.15, 0.35, -0.9), (-1, 0, 0))
    # Lifted: forearms more level, fingers wrapping forward round the crown.
    hold_r = s.hand_turn('r', (0.3, 0.7, -0.6), (1, 0, 0))
    hold_l = s.hand_turn('l', (-0.3, 0.7, -0.6), (-1, 0, 0))
    stow_r = s.hand_turn('r', (-0.1, 0.15, -1), (1, 0, 0))

    crown = {'reach': kg._add(CROWN, (0, -2, 12)), 'grip': CROWN, 'tug1': kg._add(CROWN, (0, -1, 5)),
             'tug2': kg._add(CROWN, (0, 0, 1)), 'pulled': PULLED, 'lift': LIFTED, 'look': LOOK,
             'hold': kg._add(LOOK, (0, -2, -3))}
    turn_r = {'reach': grip_r, 'grip': grip_r, 'tug1': grip_r, 'tug2': grip_r, 'pulled': grip_r,
              'lift': hold_r, 'look': hold_r, 'hold': hold_r}
    turn_l = {'reach': grip_l, 'grip': grip_l, 'tug1': grip_l, 'tug2': grip_l, 'pulled': grip_l,
              'lift': hold_l, 'look': hold_l, 'hold': hold_l}

    side_r = (-24.0, 5.0, 86.0)
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['step'], 'hand_r_ik_ctrl', (-22.0, 12.0, 80.0), hang_r)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-20.0, 24.0, 44.0), hang_r)
    for name, point in crown.items():
        s.key_world(F[name], 'hand_r_ik_ctrl', kg._add(point, WRIST_FROM_GRIP_R), turn_r[name])
    stow = F['stow']
    s.key_world(stow - 4, 'hand_r_ik_ctrl', on_pelvis(stow - 4, kg._add(kp.STOW_WRIST, (-2, 6, 10))), pelvis_turn(stow - 4, stow_r))
    s.key_world(stow, 'hand_r_ik_ctrl', on_pelvis(stow, kp.STOW_WRIST), pelvis_turn(stow, stow_r))
    s.key_world(F['out'], 'hand_r_ik_ctrl', on_pelvis(F['out'], kg._add(kp.STOW_WRIST, (-4, 8, 10))), pelvis_turn(F['out'], hang_r))
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-24.0, 8.0, 84.0), hang_r)
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)

    # Left hand: shares the grip until she has looked at the crop, then braces on the forward knee.
    side_l = (24.0, 5.0, 86.0)
    knee_l = kg.knee_turn(s)
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['step'], 'hand_l_ik_ctrl', (22.0, 12.0, 80.0), hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', (18.0, 26.0, 50.0), hang_l)
    for name, point in crown.items():
        s.key_world(F[name], 'hand_l_ik_ctrl', kg._add(point, WRIST_FROM_GRIP_L), turn_l[name])
    s.key_world(F['stow'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, knee_l)
    s.key_world(F['out'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, knee_l)
    s.key_world(F['rise'] - 2, 'hand_l_ik_ctrl', (18.0, 28.0, 66.0), knee_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    kg.key_knee_fingers(s, F['hold'], 0)
    kg.key_knee_fingers(s, F['stow'])
    kg.key_knee_fingers(s, F['out'])
    kg.key_knee_fingers(s, F['rise'] + 2, 0)

    # Elbows out to the sides round the crop, so the forearms clear her thighs.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['grip'], 'arm_r_pv_ik_ctrl', (-70.0, 10.0, 70.0))
    s.key_world(F['look'], 'arm_r_pv_ik_ctrl', (-65.0, -10.0, 80.0))
    s.key_world(stow, 'arm_r_pv_ik_ctrl', on_pelvis(stow, (-60.0, -35.0, 110.0)))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['grip'], 'arm_l_pv_ik_ctrl', (70.0, 10.0, 70.0))
    s.key_world(F['look'], 'arm_l_pv_ik_ctrl', (65.0, -10.0, 80.0))
    s.key_world(F['stow'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['out'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))

    return s.bake(ANIM)


def report(anim):
    """Grip point between the wrists at each key, against the crown path (component space)."""
    lines = []
    for name in ('grip', 'tug2', 'pulled', 'lift', 'look'):
        bones = ra.bone_positions(anim, ('hand_r', 'hand_l', 'middle_01_r', 'middle_01_l'), FRAMES[name] / 30)
        grip = (bones['middle_01_r'].translation + bones['middle_01_l'].translation) * 0.5
        lines.append(f"{name:7s} grip=({grip.x:6.1f},{grip.y:6.1f},{grip.z:6.1f})")
    return '\n'.join(lines)
