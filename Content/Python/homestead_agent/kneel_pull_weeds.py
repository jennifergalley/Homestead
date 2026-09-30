"""Two-handed weed pulling on both knees for the MetaHuman heroine, tool-free.

    from homestead_agent import kneel_pull_weeds as kw
    anim = kw.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelPullWeeds
    print(kw.report(anim))  # grip against each weed, and forearm/torso clearance

Jenny's playtest asked for weeding that looks like weeding: not the one-knee pouch forage, not a
standing stoop. She lowers herself in a controlled way onto both knees, right knee first, then the
left, and sits back a little over her heels. She leans in, takes the first weed (front-left) low on
its stem in both fists, rocks back to draw the root out, and tosses it over her left shoulder behind
her. Then she takes the second (front-right) the same way and tosses it back over her right
shoulder. She brings her left foot forward into a half-kneel and rises. Nothing stays in her hands:
there is no hip pouch, no stones, no tool.

Timings (30 fps) are in FRAMES. The game commits the pull once, at EVENTS['pulled2'] (the second
root coming out of the ground); if she's interrupted before then nothing happens in the simulation
(AHomesteadController::UpdatePendingWeedPull, AHomesteadCharacter::PullWeedsCommit). The weed
clump's centre lands at WEED_CENTRE (AHomesteadCharacter::PullWeedsForward / PullWeedsRight), and
the two fistfuls come from either side of it. The game shows each fistful in the pulling hand from
EVENTS['pulled<n>'] to EVENTS['toss<n>'], then throws it back from there (AHomesteadCharacter::UpdatePulledWeeds),
and shrinks the clump when the first comes out.

Anatomy it follows: kneeling, the thighs stand near vertical under the pelvis, so the pelvis sits
only ~55 cm up; sitting back toward the heels drops it to ~50 cm and moves it back. A two-handed
pull is a hip-hinge: the back stays long and the pull comes from rocking the pelvis back, not from
yanking with the arms. A backhand toss swings the arm out to the side and back at shoulder height,
with the chest turning with it, so the forearm never passes through the torso.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_KneelPullWeeds'
ANIM = 'AN_HeroineMH_KneelPullWeeds'

FRAMES = {
    'stand': 0, 'step': 8, 'knee_r': 16, 'knee_l': 24, 'settle': 30,
    'reach1': 38, 'grab1': 44, 'tug1': 50, 'pulled1': 56, 'swing1': 64, 'toss1': 68, 'back1': 76,
    'reach2': 84, 'grab2': 90, 'tug2': 96, 'pulled2': 102, 'swing2': 110, 'toss2': 114, 'back2': 122,
    'half': 130, 'rise': 140, 'end': 150,
}
# Seconds of the gameplay beats: fists close, each root comes free, each weed leaves her hand.
EVENTS = {
    'grab1': FRAMES['grab1'] / 30, 'pulled1': FRAMES['pulled1'] / 30, 'toss1': FRAMES['toss1'] / 30,
    'grab2': FRAMES['grab2'] / 30, 'pulled2': FRAMES['pulled2'] / 30, 'toss2': FRAMES['toss2'] / 30,
}
# The clump she weeds, relative to her standing pose (C++ settles her so the node lands here), and the
# two fistfuls: front-left, then front-right.
WEED_CENTRE = (0.0, 39.0, 3.0)
WEED_1 = kg._add(WEED_CENTRE, (9.0, -1.0, 0.0))
WEED_2 = kg._add(WEED_CENTRE, (-9.0, 1.0, 0.0))
# Both knees down, sitting back a little over the heels; the pull rocks her further back.
BODY_KNEEL_BOTH = (0.0, -6.0, 55.0)
BODY_SIT_BACK = (0.0, -11.0, 50.0)
# The left foot joins the right behind her: shin on the ground, heel up, toes tucked.
FOOT_L_KNEEL = (13.4, -30.0, 16.0)
# Grip on the stem: the lower fist just above the soil, the upper one a fist's height above it.
WRIST_FROM_STEM_LOW = (0.0, -3.0, 9.0)
WRIST_FROM_STEM_HIGH = (0.0, -2.0, 18.0)
# The toss: out to her side at shoulder height, then back past the shoulder, where she lets go.
SWING_L = (38.0, 4.0, 88.0)
TOSS_L = (32.0, -26.0, 98.0)
SWING_R = (-38.0, 4.0, 88.0)
TOSS_R = (-32.0, -26.0, 98.0)
# A hand not pulling or tossing rests on top of its own thigh, just behind the knee.
THIGH_REST_L = (14.0, 12.0, 46.0)
THIGH_REST_R = (-14.0, 12.0, 46.0)


def build():
    """Bake twice: the second pass keys the tosses in the chest's baked frame, so the turning torso
    carries the throwing arm with it."""
    return _author(_author(None))


def _author(chest_anim):
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    # Forward lean: in over each weed, back as the root comes, upright for the toss.
    lean = {F['stand']: 0, F['step']: 8, F['knee_r']: 16, F['knee_l']: 18, F['settle']: 14,
            F['reach1']: 58, F['grab1']: 66, F['tug1']: 60, F['pulled1']: 30, F['swing1']: 12, F['toss1']: 8, F['back1']: 20,
            F['reach2']: 58, F['grab2']: 66, F['tug2']: 60, F['pulled2']: 30, F['swing2']: 12, F['toss2']: 8, F['back2']: 18,
            F['half']: 16, F['rise']: 8, F['end']: 0}
    # Chest turn toward each toss (+ turns her toward her left).
    turn = {F['pulled1']: 6, F['swing1']: 22, F['toss1']: 28, F['back1']: 8,
            F['pulled2']: -6, F['swing2']: -22, F['toss2']: -28, F['back2']: -8}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=turn.get(frame, 0) * 0.3)

    # Body: a controlled drop, right knee then left, weight back over the heels; each pull rocks back.
    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 5.0, 94.0), tilt(F['step']))
    s.key_world(F['knee_r'], 'body_ctrl', (0.0, 0.0, 70.0), tilt(F['knee_r']))
    s.key_world(F['knee_l'], 'body_ctrl', kg._add(BODY_KNEEL_BOTH, (0, 2.0, 4.0)), tilt(F['knee_l']))
    shift = {'reach': (0, 3.0, 0.0), 'grab': (0, 4.0, -2.0), 'tug': (0, 1.0, -1.0), 'pulled': None,
             'swing': (0, 0, 1.0), 'toss': (0, 0, 1.5), 'back': (0, 0, 0)}
    s.key_world(F['settle'], 'body_ctrl', BODY_KNEEL_BOTH, tilt(F['settle']))
    for n in (1, 2):
        for beat, offset in shift.items():
            frame = F[f'{beat}{n}']
            point = BODY_SIT_BACK if offset is None else kg._add(BODY_KNEEL_BOTH, offset)
            s.key_world(frame, 'body_ctrl', point, tilt(frame))
    # Half-kneel on the right knee with the left foot planted forward, then up.
    s.key_world(F['half'], 'body_ctrl', kg._add(kg.BODY_KNEEL, (0, 2.0, 2.0)), tilt(F['half']))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet: the right slides back onto tucked toes as its knee goes down, then the left joins it.
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg._add(kg.FOOT_R, (0, -6, 2)))
    s.key_world(F['knee_r'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 4, 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['knee_r'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['knee_r'] + 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, -14, 8)))
    s.key_world(F['knee_l'], 'foot_l_ik_ctrl', FOOT_L_KNEEL, toes)
    s.key_world(F['back2'], 'foot_l_ik_ctrl', FOOT_L_KNEEL, toes)
    # Bring the left foot through and plant it forward for the half-kneel, then stand on it.
    s.key_world(F['back2'] + 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 6, 12)))
    s.key_world(F['half'], 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    # Knees point forward while kneeling; the left swings up and forward for the half-kneel.
    s.key_world(F['knee_r'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['knee_l'], 'leg_l_pv_ik_ctrl', (18.0, 60.0, 20.0))
    s.key_world(F['back2'], 'leg_l_pv_ik_ctrl', (18.0, 60.0, 20.0))
    s.key_world(F['half'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Spine curls with the lean and turns with each toss; her eyes stay on the weed, then follow
    # the toss a little over her shoulder.
    for frame, deg in lean.items():
        yaw = turn.get(frame, 0)
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, yaw=yaw * 0.25)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15, yaw=yaw * 0.2)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.12, yaw=yaw * 0.5)

    chest_cache = {}

    def chest(frame):
        if chest_anim is None:
            return s.bone('spine_05')
        if frame not in chest_cache:
            chest_cache[frame] = ra.bone_positions(chest_anim, ('spine_05',), frame / 30)['spine_05']
        return chest_cache[frame]

    chest_rest = chest(F['stand'])

    def on_chest(frame, standing_point):
        """A point given for the standing pose, carried by the chest's baked motion at ``frame``."""
        local = chest_rest.inverse_transform_location(unreal.Vector(*standing_point))
        v = chest(frame).transform_location(local)
        return (v.x, v.y, v.z)

    def chest_turn(frame, rotator):
        delta = chest(frame).rotation * chest_rest.rotation.inversed()
        return (delta * rotator.quaternion()).rotator()

    down = (0, 0, -1)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    # Fists round the stem: fingers down and a little forward, palms toward each other.
    grip_r = s.hand_turn('r', (0.15, 0.35, -0.9), (1, 0, 0))
    grip_l = s.hand_turn('l', (-0.15, 0.35, -0.9), (-1, 0, 0))
    # Drawing the root out: forearms steeper, knuckles forward.
    draw_r = s.hand_turn('r', (0.1, 0.55, -0.8), (1, 0, 0))
    draw_l = s.hand_turn('l', (-0.1, 0.55, -0.8), (-1, 0, 0))
    # Tossing back over the shoulder: fingers trail backward, palm toward her.
    toss_l = s.hand_turn('l', (0.2, -1.0, 0.2), (-1, 0, 0))
    toss_r = s.hand_turn('r', (-0.2, -1.0, 0.2), (1, 0, 0))
    rest_l = s.hand_turn('l', (0.0, 1.0, -0.5), (0, 0, -1))
    rest_r = s.hand_turn('r', (0.0, 1.0, -0.5), (0, 0, -1))

    def pull(n, stem, low_side):
        """Both fists on the stem (``low_side`` 'l' or 'r' holds nearer the soil), then the draw."""
        high_side = 'r' if low_side == 'l' else 'l'
        for beat, lift, turns in (('reach', (0, -2, 12), ('grip',)), ('grab', (0, 0, 0), ('grip',)),
                                  ('tug', (0, -3, 6), ('draw',)), ('pulled', (0, -12, 26), ('draw',))):
            frame = F[f'{beat}{n}']
            for side, wrist in ((low_side, WRIST_FROM_STEM_LOW), (high_side, WRIST_FROM_STEM_HIGH)):
                spread = (2.5 if side == 'l' else -2.5, 0, 0)
                point = kg._add(kg._add(stem, lift), kg._add(wrist, spread))
                rot = (grip_l if side == 'l' else grip_r) if turns[0] == 'grip' else (draw_l if side == 'l' else draw_r)
                s.key_world(frame, f'hand_{side}_ik_ctrl', point, rot)

    side_r = (-24.0, 5.0, 86.0)
    side_l = (24.0, 5.0, 86.0)
    for side, point, rot in (('r', side_r, hang_r), ('l', side_l, hang_l)):
        s.key_world(F['stand'], f'hand_{side}_ik_ctrl', point, rot)
    # Going down, both hands come forward for balance, the right touching toward its thigh first.
    s.key_world(F['knee_r'], 'hand_r_ik_ctrl', (-18.0, 22.0, 60.0), rest_r)
    s.key_world(F['knee_r'], 'hand_l_ik_ctrl', (20.0, 24.0, 66.0), hang_l)
    s.key_world(F['settle'], 'hand_r_ik_ctrl', THIGH_REST_R, rest_r)
    s.key_world(F['settle'], 'hand_l_ik_ctrl', THIGH_REST_L, rest_l)

    # First weed: left hand low, right hand high; the left hand carries it out and tosses it back.
    pull(1, WEED_1, 'l')
    s.key_world(F['swing1'], 'hand_l_ik_ctrl', on_chest(F['swing1'], SWING_L), chest_turn(F['swing1'], toss_l))
    s.key_world(F['toss1'], 'hand_l_ik_ctrl', on_chest(F['toss1'], TOSS_L), chest_turn(F['toss1'], toss_l))
    s.key_world(F['back1'], 'hand_l_ik_ctrl', on_chest(F['back1'], (26.0, 14.0, 70.0)), chest_turn(F['back1'], hang_l))
    # Meanwhile the right hand lets go and drops toward the second weed.
    s.key_world(F['swing1'], 'hand_r_ik_ctrl', kg._add(WEED_2, (-4.0, -6.0, 30.0)), grip_r)
    s.key_world(F['back1'], 'hand_r_ik_ctrl', kg._add(WEED_2, (-3.0, -4.0, 20.0)), grip_r)

    # Second weed: right hand low, left hand high; the right hand tosses it back over her shoulder.
    pull(2, WEED_2, 'r')
    s.key_world(F['swing2'], 'hand_r_ik_ctrl', on_chest(F['swing2'], SWING_R), chest_turn(F['swing2'], toss_r))
    s.key_world(F['toss2'], 'hand_r_ik_ctrl', on_chest(F['toss2'], TOSS_R), chest_turn(F['toss2'], toss_r))
    s.key_world(F['back2'], 'hand_r_ik_ctrl', on_chest(F['back2'], (-26.0, 14.0, 70.0)), chest_turn(F['back2'], hang_r))
    s.key_world(F['swing2'], 'hand_l_ik_ctrl', THIGH_REST_L, rest_l)
    s.key_world(F['back2'], 'hand_l_ik_ctrl', THIGH_REST_L, rest_l)

    # Rising: the left hand braces on the forward knee as she comes up, the right swings free.
    s.key_world(F['half'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, kg.knee_turn(s))
    s.key_world(F['half'], 'hand_r_ik_ctrl', (-22.0, 16.0, 70.0), hang_r)
    kg.key_knee_fingers(s, F['back2'], 0)
    kg.key_knee_fingers(s, F['half'])
    kg.key_knee_fingers(s, F['rise'], 0)
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-24.0, 8.0, 84.0), hang_r)
    s.key_world(F['rise'], 'hand_l_ik_ctrl', (22.0, 10.0, 82.0), hang_l)
    for side, point, rot in (('r', side_r, hang_r), ('l', side_l, hang_l)):
        s.key_world(F['end'], f'hand_{side}_ik_ctrl', point, rot)

    # Elbows out round the stem while pulling (clear of the thighs), and up and back for each toss.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    for n in (1, 2):
        s.key_world(F[f'grab{n}'], 'arm_r_pv_ik_ctrl', (-70.0, 10.0, 70.0))
        s.key_world(F[f'grab{n}'], 'arm_l_pv_ik_ctrl', (70.0, 10.0, 70.0))
        s.key_world(F[f'pulled{n}'], 'arm_r_pv_ik_ctrl', (-70.0, -10.0, 80.0))
        s.key_world(F[f'pulled{n}'], 'arm_l_pv_ik_ctrl', (70.0, -10.0, 80.0))
    s.key_world(F['toss1'], 'arm_l_pv_ik_ctrl', on_chest(F['toss1'], (60.0, -10.0, 60.0)))
    s.key_world(F['toss2'], 'arm_r_pv_ik_ctrl', on_chest(F['toss2'], (-60.0, -10.0, 60.0)))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))

    return s.bake(ANIM)


def report(anim):
    """Fists against each weed at the grab, both knees on the ground while kneeling, each toss hand
    behind the shoulder, and the worst forearm-to-torso clearance (under ~16 cm is inside her)."""
    lines = []
    for n, stem in ((1, WEED_1), (2, WEED_2)):
        bones = ra.bone_positions(anim, ('middle_01_r', 'middle_01_l'), FRAMES[f'grab{n}'] / 30)
        grip = (bones['middle_01_r'].translation + bones['middle_01_l'].translation) * 0.5
        lines.append(f"grab{n}  grip=({grip.x:6.1f},{grip.y:6.1f},{grip.z:6.1f}) weed={stem}")
    for name in ('settle', 'grab1', 'pulled2', 'back2'):
        bones = ra.bone_positions(anim, ('calf_l', 'calf_r', 'pelvis'), FRAMES[name] / 30)
        knees = ' '.join(f"{b}.z={bones[b].translation.z:5.1f}" for b in ('calf_l', 'calf_r'))
        lines.append(f"{name:7s} {knees} pelvis.z={bones['pelvis'].translation.z:5.1f}")
    for n, side in ((1, 'l'), (2, 'r')):
        bones = ra.bone_positions(anim, (f'hand_{side}', f'clavicle_{side}'), FRAMES[f'toss{n}'] / 30)
        behind = bones[f'clavicle_{side}'].translation.y - bones[f'hand_{side}'].translation.y
        lines.append(f"toss{n}  hand_{side} {behind:5.1f} cm behind the shoulder")
    worst = min(min(l, r) for _, l, r, _ in _clearance(anim))
    lines.append(f"worst forearm-to-spine clearance {worst:.1f} cm")
    return '\n'.join(lines)


def _clearance(anim, step=2):
    """kneel_gather.clearance over this clip's length."""
    saved = kg.FRAMES['end']
    kg.FRAMES['end'] = FRAMES['end']
    try:
        return kg.clearance(anim, step)
    finally:
        kg.FRAMES['end'] = saved
