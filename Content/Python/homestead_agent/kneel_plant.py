"""Kneel-and-plant work animation for the MetaHuman heroine: one seed pressed into tilled soil.

    from homestead_agent import kneel_plant as kp2
    anim = kp2.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelPlant
    print(kp2.report(anim)) # fingertip against the planting spot at each key

She steps her left foot forward and drops onto her right knee (as in the other kneeling gathers),
takes a seed from the pouch at her right hip in a pinch, reaches down in front of her and pushes
it into the soil with her forefinger, then cups her hand and draws loose soil over the hole with
two short sweeps toward her, pats it down, and rises. The game shows the seed in her pinch between
EVENTS['pick'] and EVENTS['press'] and keeps the square looking bare until EVENTS['covered'].

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import kneel_pouch as kp

SEQUENCE = 'LS_KneelPlant'
ANIM = 'AN_HeroineMH_KneelPlant'

FRAMES = {
    'stand': 0, 'step': 8, 'kneel': 20, 'pouch': 30, 'pick': 36, 'reach': 46, 'poke': 54,
    'press': 58, 'lift': 64, 'sweep1': 72, 'draw1': 78, 'sweep2': 84, 'draw2': 90, 'pat': 96,
    'patup': 100, 'covered': 102, 'rise': 116, 'end': 126,
}
EVENTS = {'pick': FRAMES['pick'] / 30, 'press': FRAMES['press'] / 30, 'covered': FRAMES['covered'] / 30}
# The planting spot relative to her standing pose (forward, a little to her right of centre);
# the game settles her so the square's middle lands here.
SPOT = (-10.0, 36.0, 0.0)
# Fingertip = wrist + fingers * this far (index tip reach from the wrist).
TIP_ALONG = 17.0


def _norm(v):
    length = sum(c * c for c in v) ** 0.5
    return tuple(c / length for c in v)


def _wrist_for_tip(tip, fingers):
    f = _norm(fingers)
    return tuple(tip[i] - f[i] * TIP_ALONG for i in range(3))


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    down = (0, 0, -1)
    fwd = (0, 1, 0)
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 30, F['pouch']: 26, F['pick']: 26, F['reach']: 66,
            F['poke']: 76, F['press']: 78, F['lift']: 72, F['sweep1']: 76, F['draw1']: 74, F['sweep2']: 76,
            F['draw2']: 74, F['pat']: 76, F['patup']: 72, F['covered']: 70, F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    for name in ('kneel', 'pouch', 'pick', 'reach', 'poke', 'press', 'lift', 'sweep1', 'draw1', 'sweep2',
                 'draw2', 'pat', 'patup', 'covered'):
        sink = (0, 3.0, -7.0) if name in ('poke', 'press', 'pat') else (0, 2.0, -5.0) if lean[F[name]] > 60 else (0, 0, 0)
        s.key_world(F[name], 'body_ctrl', kg._add(kg.BODY_KNEEL, sink), tilt(F[name]))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet as in the kneeling gathers.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['covered'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    kg.key_step_back(s, F['step'], F['kneel'], toes)
    s.key_world(F['covered'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    kg.key_rise_steps(s, F['covered'], F['rise'], toes)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Spine: lean, a little bend to her right over the spot, a glance down at the pouch.
    for frame, deg in lean.items():
        side = (8 if F['reach'] <= frame <= F['covered'] else 0) * kg.BEND_SIGN
        twist = -8 if F['pouch'] <= frame <= F['pick'] else 0
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, pitch=side * 0.3, yaw=twist * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.12 + (8 if twist else 4 if side else 0), yaw=twist)

    side_r = (-24.0, 5.0, 86.0)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    stow_r = s.hand_turn('r', (-0.1, 0.15, -1), (1, 0, 0))
    # Forefinger straight down into the soil, palm toward her.
    poke_fingers = (0.0, 0.25, -1.0)
    poke_r = s.hand_turn('r', poke_fingers, (0, -1, 0.2))
    # Cupped hand, fingers forward and down, palm down and back, drawing soil toward her.
    scoop_fingers = (-0.1, 0.8, -0.6)
    scoop_r = s.hand_turn('r', scoop_fingers, (0, -0.5, -0.85))
    pat_r = s.hand_turn('r', (0, 1, -0.2), (0, 0, -1))
    spot = SPOT
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['step'], 'hand_r_ik_ctrl', (-22.0, 10.0, 80.0), hang_r)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-22.0, 12.0, 60.0), hang_r)
    # Into the pouch (kneeling drops the pouch with her hips; kneel_pouch keys it the same way).
    pouch_kneel = kg._add(kp.STOW_WRIST, (0, -6, -44))
    s.key_world(F['pouch'], 'hand_r_ik_ctrl', kg._add(pouch_kneel, (0, 0, 4)), stow_r)
    s.key_world(F['pick'], 'hand_r_ik_ctrl', pouch_kneel, stow_r)
    s.key_world(F['reach'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (0, -4, 10)), poke_fingers), poke_r)
    s.key_world(F['poke'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (0, 0, 1.5)), poke_fingers), poke_r)
    s.key_world(F['press'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (0, 0, -2.0)), poke_fingers), poke_r)
    s.key_world(F['lift'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (0, -2, 9)), poke_fingers), poke_r)
    # Two sweeps: fingertips start past the hole and drag soil back over it.
    for n, x in ((1, 3.0), (2, -3.0)):
        s.key_world(F[f'sweep{n}'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (x, 10, 2)), scoop_fingers), scoop_r)
        s.key_world(F[f'draw{n}'], 'hand_r_ik_ctrl', _wrist_for_tip(kg._add(spot, (x * 0.3, -3, 2)), scoop_fingers), scoop_r)
    pat_wrist = kg._add(spot, (0, -8, 3.5))
    s.key_world(F['pat'], 'hand_r_ik_ctrl', pat_wrist, pat_r)
    s.key_world(F['patup'], 'hand_r_ik_ctrl', kg._add(pat_wrist, (0, 0, 5)), pat_r)
    s.key_world(F['covered'], 'hand_r_ik_ctrl', kg._add(pat_wrist, (0, 0, 3.5)), pat_r)
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-24.0, 8.0, 84.0), hang_r)
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)

    # Left hand braced on the forward knee throughout the kneel.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    knee_l = kg.knee_turn(s)
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, knee_l)
    for name in ('pick', 'press', 'draw2', 'covered'):
        s.key_world(F[name], 'hand_l_ik_ctrl', kg._add(kg.KNEE_WRIST_L, (0, 0.6, 0.3) if name == 'press' else (0, 0, 0)), knee_l)
    s.key_world(F['rise'] - 2, 'hand_l_ik_ctrl', (18.0, 28.0, 66.0), knee_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    kg.key_knee_fingers(s, F['stand'], 0)
    for f in (F['kneel'], F['covered']):
        kg.key_knee_fingers(s, f)
    kg.key_knee_fingers(s, F['rise'] + 2, 0)

    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    s.key_world(F['pick'], 'arm_r_pv_ik_ctrl', (-60.0, -35.0, 80.0))
    s.key_world(F['reach'], 'arm_r_pv_ik_ctrl', (-60.0, 10.0, 70.0))
    s.key_world(F['covered'], 'arm_r_pv_ik_ctrl', (-60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['covered'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    return s.bake(ANIM)


def report(anim):
    lines = []
    spot = unreal.Vector(*SPOT)
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'index_03_r', 'middle_03_r', 'pelvis'), frame / 30)
        tip = b['index_03_r'].translation
        lines.append(f"{name:8s} index tip ({tip.x:6.1f},{tip.y:6.1f},{tip.z:6.1f}) to spot {(tip - spot).length():5.1f}  "
                     f"pelvis z {b['pelvis'].translation.z:5.1f}")
    return '\n'.join(lines)
