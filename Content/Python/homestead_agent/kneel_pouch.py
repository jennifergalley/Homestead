"""Kneel-and-forage animation for the MetaHuman heroine: roots and berries go into a hip pouch.

    from homestead_agent import kneel_pouch as kp
    anim = kp.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelGatherPouch

She steps her left foot forward and drops onto her right knee like the stick gather (same body,
feet and pickup spots, so the game's alignment is shared), braces her left hand on her forward knee,
and twice reaches down with her right hand, plucks or tugs something up, and slips it into the
forage pouch hanging at her right hip, then rises. Nothing is carried in her arms. Timings (30 fps)
are in FRAMES; the game shows the item in her hand and hides it in the pouch at POUCH_EVENTS.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_KneelGatherPouch'
ANIM = 'AN_HeroineMH_KneelGatherPouch'

FRAMES = {
    'stand': 0, 'step': 8, 'kneel': 20,
    'reach1': 30, 'grab1': 36, 'tug1': 42, 'lift1': 48, 'stow1': 56, 'out1': 62,
    'reach2': 68, 'grab2': 74, 'tug2': 80, 'lift2': 86, 'stow2': 94, 'out2': 100,
    'rise': 108, 'end': 118,
}
# Seconds at which the game shows an item in her right hand / hides it in the pouch.
POUCH_EVENTS = {
    'pick1': FRAMES['grab1'] / 30, 'stow1': FRAMES['stow1'] / 30,
    'pick2': FRAMES['grab2'] / 30, 'stow2': FRAMES['stow2'] / 30,
}
# Pickup spots: the stick gather's, so AHomesteadCharacter aligns her the same way.
PICK_1 = (-24.0, 34.0, 4.0)
PICK_2 = (-28.0, 30.0, 4.0)
# Pouch opening at her right hip in the standing pose (it hangs from a belt at the hip, pelvis
# frame). The wrist sits above it so the fingers reach in.
POUCH_OPENING = (-19.0, 7.0, 93.0)
STOW_WRIST = (-20.0, 9.0, 103.0)


def build():
    """Bake twice: the second pass keys the pouch reach in the pelvis's baked frame."""
    return _author(_author(None))


def _author(pelvis_anim):
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    down = (0, 0, -1)
    fwd = (0, 1, 0)
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 30,
            F['reach1']: 62, F['grab1']: 70, F['tug1']: 64, F['lift1']: 40, F['stow1']: 22, F['out1']: 30,
            F['reach2']: 62, F['grab2']: 70, F['tug2']: 64, F['lift2']: 40, F['stow2']: 22, F['out2']: 24,
            F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    for name in ('kneel', 'reach1', 'grab1', 'tug1', 'lift1', 'stow1', 'out1',
                 'reach2', 'grab2', 'tug2', 'lift2', 'stow2', 'out2'):
        sink = (0, 0, -4.0) if name.startswith('grab') else (0, 0, -2.0) if name.startswith(('reach', 'tug')) else (0, 0, 0)
        s.key_world(F[name], 'body_ctrl', kg._add(kg.BODY_KNEEL, sink), tilt(F[name]))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet exactly as the stick gather: left steps forward, right slides back onto tucked toes.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['out2'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['out2'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 2, 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Side bend toward her right as she reaches down beside the forward thigh, and a small turn
    # toward the pouch as she stows.
    bend = {F['reach1']: 14, F['grab1']: 16, F['tug1']: 12, F['lift1']: 6,
            F['reach2']: 14, F['grab2']: 16, F['tug2']: 12, F['lift2']: 6}
    twist = {F['stow1']: -8, F['stow2']: -8}
    for frame, deg in lean.items():
        side = bend.get(frame, 0) * kg.BEND_SIGN
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, pitch=side * 0.3, yaw=twist.get(frame, 0) * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15)
        # She glances down at the pouch as she stows.
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.12 + (8 if frame in twist else 0), yaw=twist.get(frame, 0))

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

    # Right hand: side -> reach -> pinch/grip -> tug up -> lift -> slip into the pouch -> out, twice -> side.
    side_r = (-24.0, 5.0, 86.0)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    grab_r = s.hand_turn('r', (0.2, 0.6, -0.8), (0, 0, -1))
    lift_r = s.hand_turn('r', (0.1, 0.5, -0.85), (0.2, 0, -1))
    # Fingers point down into the pouch, palm toward her hip.
    stow_r = s.hand_turn('r', (-0.1, 0.15, -1), (1, 0, 0))
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['step'], 'hand_r_ik_ctrl', (-22.0, 12.0, 80.0), hang_r)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-24.0, 22.0, 40.0), hang_r)
    for n, pick in ((1, PICK_1), (2, PICK_2)):
        s.key_world(F[f'reach{n}'], 'hand_r_ik_ctrl', kg._add(pick, (0, -4, 14)), grab_r)
        s.key_world(F[f'grab{n}'], 'hand_r_ik_ctrl', kg._add(pick, (0, -6, 8)), grab_r)
        # A short, firm tug (roots) or pluck (berries) straight up before lifting away.
        s.key_world(F[f'tug{n}'], 'hand_r_ik_ctrl', kg._add(pick, (0, -6, 16)), grab_r)
        s.key_world(F[f'lift{n}'], 'hand_r_ik_ctrl', kg._add(pick, (4, -12, 36)), lift_r)
        stow = F[f'stow{n}']
        s.key_world(stow - 3, 'hand_r_ik_ctrl', on_pelvis(stow - 3, kg._add(STOW_WRIST, (-2, 4, 8))), pelvis_turn(stow - 3, stow_r))
        s.key_world(stow, 'hand_r_ik_ctrl', on_pelvis(stow, STOW_WRIST), pelvis_turn(stow, stow_r))
        out = F[f'out{n}']
        s.key_world(out, 'hand_r_ik_ctrl', on_pelvis(out, kg._add(STOW_WRIST, (-4, 8, 10))), pelvis_turn(out, hang_r))
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-24.0, 8.0, 84.0), hang_r)
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)

    # Left hand: braced on the forward knee for the whole kneel, then back to her side.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    knee_l = s.hand_turn('l', fwd, (0, 0, -1))
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', (16.0, 38.0, 54.0), knee_l)
    for name in ('grab1', 'stow1', 'grab2', 'stow2', 'out2'):
        s.key_world(F[name], 'hand_l_ik_ctrl', (16.0, 38.0, 55.0 if name.startswith('stow') else 54.0), knee_l)
    s.key_world(F['rise'] - 2, 'hand_l_ik_ctrl', (18.0, 34.0, 66.0), knee_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    # Elbows: right elbow out and back; left elbow out to the side while bracing.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    for n in (1, 2):
        s.key_world(F[f'stow{n}'], 'arm_r_pv_ik_ctrl', on_pelvis(F[f'stow{n}'], (-60.0, -35.0, 110.0)))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['out2'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))

    return s.bake(ANIM)


def pouch_point(anim, frame):
    """Pouch opening at ``frame`` of a baked clip (component space), carried by the pelvis."""
    pelvis = ra.bone_positions(anim, ('pelvis',), frame / 30)['pelvis']
    rest = ra.bone_positions(anim, ('pelvis',), 0)['pelvis']
    return pelvis.transform_location(rest.inverse_transform_location(unreal.Vector(*POUCH_OPENING)))
