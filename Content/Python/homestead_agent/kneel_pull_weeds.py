"""Weed pulling on both knees for the MetaHuman heroine, one hand at a time, tool-free.

    from homestead_agent import kneel_pull_weeds as kw
    anim = kw.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelPullWeeds
    print(kw.report(anim))  # grip against each weed, the bracing hand, and forearm/torso clearance

Jenny's playtest asked for weeding that looks like weeding: not the one-knee pouch forage, not a
standing stoop. She lowers herself in a controlled way onto both knees, right knee first, then the
left, and sits back a little over her heels. Then each weed is one hand's own beat (Jenny, 09-30: "one
hand at a time, and the hand that dug is the one that throws it"):
- Her left hand reaches in to the first weed (front-left), digs its fingers in round the root, grips
  it low on the stem, rocks back to draw it out and tosses it back over her left shoulder. All the
  while her right palm is planted on the ground beside her right knee, taking her weight as she leans
  in, and it comes back up onto her right thigh as she straightens for the toss.
- Then the right hand does the same with the second weed (front-right), tossing it over her right
  shoulder, while her left palm braces on the ground and then slides out along her left thigh as her
  chest turns right (the forearm-clearance fix: it stays clear of the turning torso).
She brings her left foot forward into a half-kneel and rises. Nothing stays in her hands: there is no
hip pouch, no stones, no tool. The two-handed toss is unchanged (Jenny: "10/10, keep it").

Timings (30 fps) are in FRAMES. The game commits the pull once, at EVENTS['pulled2'] (the second
root coming out of the ground); if she's interrupted before then nothing happens in the simulation
(AHomesteadController::UpdatePendingWeedPull, AHomesteadCharacter::PullWeedsCommit). The weed
clump's centre lands at WEED_CENTRE (AHomesteadCharacter::PullWeedsForward / PullWeedsRight), and
the two fistfuls come from either side of it. The game shows each fistful in the hand that pulled
it (the first in her left, the second in her right) from EVENTS['pulled<n>'] to EVENTS['toss<n>'],
then throws it back from there (AHomesteadCharacter::UpdatePulledWeeds, PullWeedsTiming), and
shrinks the clump when the first comes out.

Anatomy it follows: kneeling, the thighs stand near vertical under the pelvis, so the pelvis sits
only ~55 cm up; sitting back toward the heels drops it to ~50 cm and moves it back. Leaning in to
reach the ground with one arm, the other takes the weight on a flat palm, as people weeding do; the
pull comes from rocking the pelvis back, not from yanking with the arm. A one-handed pull is quicker
than the two-fisted one it replaces (FRAMES). A backhand toss swings the arm out to the side and back
at shoulder height, with the chest turning with it, so the forearm never passes through the torso.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_KneelPullWeeds'
ANIM = 'AN_HeroineMH_KneelPullWeeds'

FRAMES = {
    'stand': 0, 'step': 8, 'knee_r': 16, 'knee_l': 24, 'settle': 30,
    'reach1': 38, 'grab1': 44, 'tug1': 49, 'pulled1': 54, 'swing1': 62, 'toss1': 66, 'back1': 74,
    'reach2': 86, 'grab2': 92, 'tug2': 97, 'pulled2': 102, 'swing2': 110, 'toss2': 114, 'back2': 122,
    'half': 130, 'rise': 140, 'end': 150,
}
# The toss keeps the two-fisted clip's timing exactly (Jenny: "10/10, keep it"): the swing 8 frames
# after the root comes free, the release 4 after that, back 8 after the release. Only the dig is
# one-handed and quicker.
# Which hand pulls (and tosses) each weed; the other braces.
PULL_HAND = {1: 'l', 2: 'r'}
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
# Grip on the stem: one fist low, just above the soil, its fingers dug in round the root at the reach.
WRIST_FROM_STEM_LOW = (0.0, -3.0, 9.0)
DIG_FROM_STEM = (0.0, -2.0, 6.0)
# The toss: out to her side at shoulder height, then back past the shoulder, where she lets go.
SWING_L = (38.0, 4.0, 88.0)
TOSS_L = (32.0, -26.0, 98.0)
SWING_R = (-38.0, 4.0, 88.0)
TOSS_R = (-32.0, -26.0, 98.0)
# A hand not working rests on top of its own thigh, just behind the knee...
THIGH_REST_L = (14.0, 12.0, 46.0)
THIGH_REST_R = (-14.0, 12.0, 46.0)
# ...and while the chest turns toward the other side for a toss, it slides out along that thigh so the
# forearm stays clear of the turning torso.
THIGH_OUT_L = (22.0, 14.0, 48.0)
THIGH_OUT_R = (-22.0, 14.0, 48.0)
# While the other hand leans in to dig, this one takes her weight on a flat palm on the ground, out
# beside its knee and a little ahead of it (the wrist a palm's thickness up).
GROUND_BRACE_L = (25.0, 30.0, 6.0)
GROUND_BRACE_R = (-25.0, 30.0, 6.0)
# Elbow poles: out round the stem for the pulling arm; back and out for the bracing arm, so the elbow
# stays soft and points behind her rather than locking or folding into the thigh.
POLE_PULL_GRAB = (70.0, 10.0, 70.0)
POLE_PULL_DRAW = (70.0, -10.0, 80.0)
POLE_BRACE = (60.0, -30.0, 70.0)


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
    # Chest turn (+ turns her toward her left): a little toward the working hand as it leans in, then
    # round toward each toss.
    turn = {F['reach1']: 6, F['grab1']: 8, F['tug1']: 6, F['pulled1']: 6, F['swing1']: 22, F['toss1']: 28, F['back1']: 8,
            F['reach2']: -6, F['grab2']: -8, F['tug2']: -6, F['pulled2']: -6, F['swing2']: -22, F['toss2']: -28, F['back2']: -8}

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

    # Bracing: palm flat on the ground, fingers forward and a little out.
    brace_l = s.hand_turn('l', (0.15, 1.0, -0.05), (0, 0, -1))
    brace_r = s.hand_turn('r', (-0.15, 1.0, -0.05), (0, 0, -1))

    def pull(n, stem):
        """One hand's dig, grip and draw on its own weed (PULL_HAND[n]); the other is left to the caller."""
        side = PULL_HAND[n]
        spread = (2.5 if side == 'l' else -2.5, 0, 0)
        for beat, offset, turns in (('reach', kg._add(DIG_FROM_STEM, (0, -2, 12)), 'grip'),
                                    ('grab', kg._add(DIG_FROM_STEM, (0, 0, 0)), 'grip'),
                                    ('tug', kg._add(WRIST_FROM_STEM_LOW, (0, -3, 6)), 'draw'),
                                    ('pulled', kg._add(WRIST_FROM_STEM_LOW, (0, -12, 26)), 'draw')):
            rot = (grip_l if side == 'l' else grip_r) if turns == 'grip' else (draw_l if side == 'l' else draw_r)
            s.key_world(F[f'{beat}{n}'], f'hand_{side}_ik_ctrl', kg._add(kg._add(stem, offset), spread), rot)

    def brace(n, rest_point, out_point):
        """The hand that isn't pulling weed n: from its thigh down onto the ground as she leans in, back up
        onto the thigh as she rocks back, and out along it while her chest turns away for the toss."""
        side = 'r' if PULL_HAND[n] == 'l' else 'l'
        control = f'hand_{side}_ik_ctrl'
        ground, flat, rest = ((GROUND_BRACE_L, brace_l, rest_l) if side == 'l' else (GROUND_BRACE_R, brace_r, rest_r))
        s.key_world(F[f'reach{n}'] - 4, control, kg._add(ground, (0, -6, 12)), flat)
        for beat in ('reach', 'grab', 'tug'):
            s.key_world(F[f'{beat}{n}'], control, ground, flat)
        s.key_world(F[f'pulled{n}'], control, kg._add(rest_point, (0, 4, 4)), rest)
        s.key_world(F[f'swing{n}'], control, out_point, rest)
        s.key_world(F[f'back{n}'], control, out_point, rest)

    side_r = (-24.0, 5.0, 86.0)
    side_l = (24.0, 5.0, 86.0)
    for side, point, rot in (('r', side_r, hang_r), ('l', side_l, hang_l)):
        s.key_world(F['stand'], f'hand_{side}_ik_ctrl', point, rot)
    # Going down, both hands come forward for balance, the right touching toward its thigh first.
    s.key_world(F['knee_r'], 'hand_r_ik_ctrl', (-18.0, 22.0, 60.0), rest_r)
    s.key_world(F['knee_r'], 'hand_l_ik_ctrl', (20.0, 24.0, 66.0), hang_l)
    s.key_world(F['settle'], 'hand_r_ik_ctrl', THIGH_REST_R, rest_r)
    s.key_world(F['settle'], 'hand_l_ik_ctrl', THIGH_REST_L, rest_l)

    # First weed: the left hand digs it out and tosses it back over her left shoulder; the right braces.
    pull(1, WEED_1)
    brace(1, THIGH_REST_R, THIGH_OUT_R)
    s.key_world(F['swing1'], 'hand_l_ik_ctrl', on_chest(F['swing1'], SWING_L), chest_turn(F['swing1'], toss_l))
    s.key_world(F['toss1'], 'hand_l_ik_ctrl', on_chest(F['toss1'], TOSS_L), chest_turn(F['toss1'], toss_l))
    s.key_world(F['back1'], 'hand_l_ik_ctrl', on_chest(F['back1'], (26.0, 14.0, 70.0)), chest_turn(F['back1'], hang_l))

    # Second weed: the right hand digs it out and tosses it back over her right shoulder; the left braces.
    pull(2, WEED_2)
    brace(2, THIGH_REST_L, THIGH_OUT_L)
    s.key_world(F['swing2'], 'hand_r_ik_ctrl', on_chest(F['swing2'], SWING_R), chest_turn(F['swing2'], toss_r))
    s.key_world(F['toss2'], 'hand_r_ik_ctrl', on_chest(F['toss2'], TOSS_R), chest_turn(F['toss2'], toss_r))
    s.key_world(F['back2'], 'hand_r_ik_ctrl', on_chest(F['back2'], (-26.0, 14.0, 70.0)), chest_turn(F['back2'], hang_r))
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

    # Elbows: the pulling arm's out round the stem (clear of the thigh), the bracing arm's back and out,
    # each tossing arm's up and back; the resting arm keeps its elbow out while the chest turns away.
    def mirror(p, side):
        return p if side == 'l' else (-p[0], p[1], p[2])

    for side in ('l', 'r'):
        s.key_world(F['stand'], f'arm_{side}_pv_ik_ctrl', mirror((45.0, -30.0, 100.0), side))
        s.key_world(F['end'], f'arm_{side}_pv_ik_ctrl', mirror((45.0, -30.0, 100.0), side))
    for n in (1, 2):
        side = PULL_HAND[n]
        other = 'r' if side == 'l' else 'l'
        s.key_world(F[f'grab{n}'], f'arm_{side}_pv_ik_ctrl', mirror(POLE_PULL_GRAB, side))
        s.key_world(F[f'pulled{n}'], f'arm_{side}_pv_ik_ctrl', mirror(POLE_PULL_DRAW, side))
        s.key_world(F[f'toss{n}'], f'arm_{side}_pv_ik_ctrl', on_chest(F[f'toss{n}'], mirror((60.0, -10.0, 60.0), side)))
        for beat in ('reach', 'tug'):
            s.key_world(F[f'{beat}{n}'], f'arm_{other}_pv_ik_ctrl', mirror(POLE_BRACE, other))
        s.key_world(F[f'swing{n}'], f'arm_{other}_pv_ik_ctrl', mirror(POLE_PULL_DRAW, other))
        s.key_world(F[f'back{n}'], f'arm_{other}_pv_ik_ctrl', mirror(POLE_PULL_DRAW, other))
    return s.bake(ANIM)


def report(anim):
    """The pulling fist against its weed at each grab and the other palm on the ground, both knees on the
    ground while kneeling, each toss hand behind the shoulder, and the worst forearm-to-torso clearance
    (under ~16 cm is inside her)."""
    lines = []
    for n, stem in ((1, WEED_1), (2, WEED_2)):
        side = PULL_HAND[n]
        other = 'r' if side == 'l' else 'l'
        bones = ra.bone_positions(anim, (f'middle_01_{side}', f'hand_{other}'), FRAMES[f'grab{n}'] / 30)
        grip = bones[f'middle_01_{side}'].translation
        palm = bones[f'hand_{other}'].translation
        lines.append(f"grab{n}  {side} fist=({grip.x:6.1f},{grip.y:6.1f},{grip.z:6.1f}) weed={stem}  "
                     f"{other} brace wrist z={palm.z:5.1f}")
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
