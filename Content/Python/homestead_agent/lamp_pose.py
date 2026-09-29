"""The oil lamp's poses for the MetaHuman heroine (add-oil-lamp).

    from homestead_agent import lamp_pose as lp
    raised = lp.build_raised()      # /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_LampRaised
    down = lp.build_set_down()      # /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_LampSetDown
    print(lp.report(raised, down))  # wrist heights and the lamp's base at the contact

**Raised** (a held pose): the game layers it over the right arm and the head (from clavicle_r and
neck_01) while the lamp is her selected tool, over walking and idling. She holds the lamp up and out
ahead of her at head height by its bail, the arm raised and a little bent, and peers past it, like
Mr. Filch searching the library. The lamp hangs from her fist (the game's hanger sits at her grip,
about 8 cm past the wrist along the fingers), so its glass is near her eye line, ahead and to the
right of her face.

**Set down** (a one-shot, full body): she steps and kneels on her right knee, lowers the lamp by its
bail to the floor 45 cm straight ahead of her (the game's spot), and at CONTACT its base meets the
floor; her hand opens and she rises. Taking the lamp up plays the same clip: at CONTACT it arrives
in her hand, and she rises holding it. CONTACT must match AHomesteadCharacter::LampSetDownContact.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import active_idle as ai

RAISED_SEQUENCE = 'LS_LampRaised'
RAISED_ANIM = 'AN_HeroineMH_LampRaised'
SET_DOWN_SEQUENCE = 'LS_LampSetDown'
SET_DOWN_ANIM = 'AN_HeroineMH_LampSetDown'

# HomesteadLampLook::BailTopHeight: the bail's top above the lamp's base, and the game's hanger
# offset from the wrist bone along the fingers.
BAIL_TOP = 34.1
GRIP_ALONG = 6.7

# Raised: wrist up, forward and to her right; the hand carries on up from the forearm with the
# palm toward her face, fingers curled back over the bail. The elbow points out and down.
RAISED_WRIST_R = (-14.0, 44.0, 160.0)
RAISED_FINGERS_R = (0.1, 0.35, 0.93)
RAISED_PALM_R = (0.25, -0.92, 0.3)
RAISED_POLE_R = (-70.0, 0.0, 115.0)
# She peers past the lamp into the dark: chin a touch forward and down, head turned a little left.
RAISED_NECK_ROLL = 5.0
RAISED_HEAD_ROLL = -3.0
RAISED_HEAD_YAW = 5.0

FRAMES = {'stand': 0, 'step': 8, 'kneel': 20, 'lower': 26, 'contact': 30, 'hold': 34,
          'release': 38, 'rise': 52, 'end': 60}
CONTACT = FRAMES['contact'] / 30.0
# The lamp stands 45 cm straight ahead of her (AHomesteadController::StartLampSetDown).
SPOT = (0.0, 45.0, 0.0)


def _hanging(s):
    # Fingers down with the fist closed on the bail, palm toward her side.
    return s.hand_turn('r', (0, 0, -1), (1, 0, 0))


def build_raised(anim=RAISED_ANIM):
    s = ra.Session(RAISED_SEQUENCE, frames=2)
    turn = s.hand_turn('r', RAISED_FINGERS_R, RAISED_PALM_R)
    for frame in (0, 2):
        s.key_bool(frame, 'arm_l_fk_ik_switch', True)
        s.key_bool(frame, 'arm_r_fk_ik_switch', True)
        s.key_world(frame, 'foot_l_ik_ctrl', ai.FOOT_L, unreal.Rotator(roll=0, pitch=0, yaw=-ai.TOE_OUT))
        s.key_world(frame, 'foot_r_ik_ctrl', ai.FOOT_R, unreal.Rotator(roll=0, pitch=0, yaw=ai.TOE_OUT))
        s.key_world(frame, 'body_ctrl', ai.BODY)
        s.key_world(frame, 'hand_l_ik_ctrl', ai.HAND_L, s.hand_turn('l', (0, 0, -1), (-1, 0, 0)))
        s.key_world(frame, 'arm_l_pv_ik_ctrl', (ai.ELBOW_POLE[0], ai.ELBOW_POLE[1], ai.ELBOW_POLE[2]))
        s.key_world(frame, 'hand_r_ik_ctrl', RAISED_WRIST_R, turn)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', RAISED_POLE_R)
        s.key_rotation(frame, 'neck_01_ctrl', roll=RAISED_NECK_ROLL)
        s.key_rotation(frame, 'head_ctrl', roll=RAISED_HEAD_ROLL, yaw=RAISED_HEAD_YAW)
    return s.bake(anim)


def build_set_down(anim=SET_DOWN_ANIM):
    s = ra.Session(SET_DOWN_SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 26, F['lower']: 40, F['contact']: 54, F['hold']: 54,
            F['release']: 36, F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    # Body: down onto the right knee, sinking a little as the lamp reaches the floor.
    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    for name in ('kneel', 'lower', 'contact', 'hold', 'release'):
        sink = (0, 0, -6.0) if name in ('contact', 'hold') else (0, 0, 0)
        s.key_world(F[name], 'body_ctrl', kg._add(kg.BODY_KNEEL, sink), tilt(F[name]))
    s.key_world(F['release'] + 6, 'body_ctrl', (0.0, 4.0, 70.0), tilt(F['release'] + 6))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet: the left steps forward and plants; the right slides back onto tucked toes.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['release'] + 4, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['release'] + 2, 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 2, 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    for frame, deg in lean.items():
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.2)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.15)

    # Right hand: the lamp hangs from it at her side, then is lowered by the bail to the spot and
    # set down (the lamp's base touches the floor at the contact), the hand opens and lifts away.
    hang = _hanging(s)
    wrist_at_spot = (SPOT[0], SPOT[1], SPOT[2] + BAIL_TOP + GRIP_ALONG)
    side_r = (-24.0, 6.0, 88.0)
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang)
    s.key_world(F['step'], 'hand_r_ik_ctrl', (-20.0, 16.0, 84.0), hang)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-10.0, 36.0, 62.0), hang)
    s.key_world(F['lower'], 'hand_r_ik_ctrl', kg._add(wrist_at_spot, (0.0, 0.0, 8.0)), hang)
    s.key_world(F['contact'], 'hand_r_ik_ctrl', wrist_at_spot, hang)
    s.key_world(F['hold'], 'hand_r_ik_ctrl', wrist_at_spot, hang)
    s.key_world(F['release'], 'hand_r_ik_ctrl', kg._add(wrist_at_spot, (-2.0, -3.0, 6.0)), hang)
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-22.0, 10.0, 86.0), hang)
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang)
    s.key_world(F['kneel'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    s.key_world(F['release'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))

    # Left hand: at her side, braced on the forward knee while she kneels.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', (0, 0, -1), (-1, 0, 0))
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['step'], 'hand_l_ik_ctrl', (22.0, 14.0, 80.0), hang_l)
    for name in ('kneel', 'lower', 'contact', 'hold', 'release'):
        s.key_world(F[name], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, kg.knee_turn(s))
    kg.key_knee_fingers(s, F['kneel'], 1.0)
    kg.key_knee_fingers(s, F['release'], 1.0)
    kg.key_knee_fingers(s, F['rise'], 0.0)
    s.key_world(F['rise'], 'hand_l_ik_ctrl', (22.0, 10.0, 84.0), hang_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    return s.bake(anim)


def report(raised=None, down=None):
    lines = []
    if raised:
        b = ra.bone_positions(raised, ('hand_r', 'head', 'clavicle_r', 'upperarm_r'), 0.0)
        p = {k: v.translation for k, v in b.items()}
        lines.append(f"raised: hand_r ({p['hand_r'].x:.1f},{p['hand_r'].y:.1f},{p['hand_r'].z:.1f})  "
                     f"head z {p['head'].z:.1f}  shoulder-to-wrist {(p['hand_r'] - p['upperarm_r']).length():.1f}")
    if down:
        for name in ('kneel', 'lower', 'contact', 'release'):
            b = ra.bone_positions(down, ('hand_r', 'middle_01_r', 'pelvis'), FRAMES[name] / 30.0)
            hand, knuckle = b['hand_r'].translation, b['middle_01_r'].translation
            grip = hand + (knuckle - hand) * 0.78
            lines.append(f"{name:8s} hand_r ({hand.x:.1f},{hand.y:.1f},{hand.z:.1f})  grip z {grip.z:.1f}"
                         f"  -> lamp base z {grip.z - BAIL_TOP:.1f}  pelvis z {b['pelvis'].translation.z:.1f}")
    return '\n'.join(lines)
