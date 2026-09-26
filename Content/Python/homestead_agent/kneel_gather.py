"""Kneel-and-gather-sticks work animation for the MetaHuman heroine, keyed on her Control Rig.

    from homestead_agent import kneel_gather as kg
    anim = kg.build()          # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelGatherSticks
    print(kg.check(anim))      # contact and reach measurements

She steps her left foot forward and drops onto her right knee, leans in, picks up a stick with her
right hand and lays it across her left forearm (cradled against her belly), picks up a second, then
rises holding the bundle and lowers her arms. Timings (30 fps) are in FRAMES; the game shows and
hides the stick props at those moments (see STICK_EVENTS).

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra

SEQUENCE = 'LS_KneelGatherSticks'
ANIM = 'AN_HeroineMH_KneelGatherSticks'

FRAMES = {
    'stand': 0, 'step': 8, 'kneel': 20,
    'reach1': 32, 'grab1': 38, 'lift1': 46, 'place1': 54,
    'reach2': 64, 'grab2': 70, 'lift2': 78, 'place2': 86,
    'rise': 104, 'settle': 112, 'end': 118,
}
# Seconds at which the game shows a stick in the right hand / moves it to the bundle / hides all.
STICK_EVENTS = {
    'pick1': FRAMES['grab1'] / 30, 'stack1': FRAMES['place1'] / 30,
    'pick2': FRAMES['grab2'] / 30, 'stack2': FRAMES['place2'] / 30,
    'stow': FRAMES['settle'] / 30,
}

BODY_STAND = (0.0, 2.8, 103.7)
BODY_KNEEL = (0.0, -4.0, 57.0)
FOOT_L = (13.4, -0.2, 8.6)
FOOT_R = (-13.4, -0.2, 8.6)
FOOT_L_FORWARD = (15.0, 34.0, 8.6)
# Right foot behind the kneeling knee, heel up and toes tucked on the ground.
FOOT_R_KNEEL = (-13.4, -30.0, 16.0)
# Ground pickup points (wrist targets sit a hand's length above/behind the grasp).
STICK_1 = (-24.0, 34.0, 3.0)
STICK_2 = (-28.0, 30.0, 3.0)
# Sign of spine-control pitch that bends her toward her right side.
BEND_SIGN = 1
# Bundle cradle, given for the standing pose and carried in the chest's frame at every lean: the
# left wrist sits in front of her belly with the elbow out and forward, so the forearm clears her
# torso, and the right hand lays each stick on top.
CRADLE_WRIST_L = (4.0, 28.0, 120.0)
CRADLE_ELBOW_POLE_L = (75.0, 10.0, 110.0)
LAY_FROM_CRADLE_R = (-6.0, 8.0, 6.0)
HUG_FROM_CRADLE_L = (0.0, 0.0, 12.0)
# While kneeling the cradling wrist stays at least this high: the forward thigh's top is ~58 cm.
THIGH_CLEAR_Z = 66.0


def _add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def build():
    """Bake twice: the first pass gives the torso's motion (hand keys don't move the spine), the
    second keys the cradle in the chest's frame from it."""
    return _author(_author(None))


def _author(chest_anim):
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    down = (0, 0, -1)
    fwd = (0, 1, 0)
    inward_l = (-1, 0, 0)

    # Arms in IK for the whole clip; legs are IK by default.
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    # Spine fold: the body control tilts the whole upper body forward about the hips (roll bends
    # forward); the spine, neck and head curl on top of it.
    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 30, F['reach1']: 74, F['grab1']: 82, F['lift1']: 48,
            F['place1']: 24, F['reach2']: 66, F['grab2']: 74, F['lift2']: 44, F['place2']: 22,
            F['place2'] + 8: 30, F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    # Pelvis / body. At each grasp the body sinks a little onto the kneeling leg.
    s.key_world(F['stand'], 'body_ctrl', BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    for name in ('kneel', 'reach1', 'grab1', 'lift1', 'place1', 'reach2', 'grab2', 'lift2', 'place2'):
        sink = (0, 0, -4.0) if name.startswith('grab') else (0, 0, -2.0) if name.startswith('reach') else (0, 0, 0)
        s.key_world(F[name], 'body_ctrl', _add(BODY_KNEEL, sink), tilt(F[name]))
    s.key_world(F['place2'] + 8, 'body_ctrl', (0.0, 4.0, 70.0), tilt(F['place2'] + 8))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', BODY_STAND, tilt(F['end']))

    # Feet: left steps forward and plants; right slides back onto tucked toes as the knee drops.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', _add(FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['place2'] + 6, 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', _add(FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', FOOT_R_KNEEL, toes)
    s.key_world(F['place2'] + 4, 'foot_r_ik_ctrl', FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 2, 'foot_r_ik_ctrl', FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', FOOT_R)
    # Knees point forward (pole vectors ahead of the knees).
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Side bend toward her right at each pickup carries her chest into the gap beside the forward
    # thigh instead of folding it onto that thigh.
    bend = {F['reach1']: 14, F['grab1']: 16, F['lift1']: 6, F['reach2']: 14, F['grab2']: 16, F['lift2']: 6}
    for frame, deg in lean.items():
        side = bend.get(frame, 0) * BEND_SIGN
        for control, share in (('spine_01_ctrl', 0.15), ('spine_02_ctrl', 0.15), ('spine_03_ctrl', 0.15)):
            s.key_rotation(frame, control, roll=deg * share, pitch=side * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.12)

    # The bundle rides against her chest: cradle targets (left wrist, left elbow pole, and the right
    # hand laying a stick on the bundle) are fixed in the chest's frame (spine_05) and follow it
    # as she leans, so her torso never folds through the cradling arm.
    chest_cache = {}

    def chest(frame):
        if chest_anim is None:
            return s.bone('spine_05')
        if frame not in chest_cache:
            chest_cache[frame] = ra.bone_positions(chest_anim, ('spine_05',), frame / 30)['spine_05']
        return chest_cache[frame]

    chest_rest = chest(F['stand'])

    def local(world):
        return chest_rest.inverse_transform_location(unreal.Vector(*world))

    def on_chest(frame, local_point):
        v = chest(frame).transform_location(local_point)
        return (v.x, v.y, v.z)

    def chest_turn(frame, rotator):
        delta = chest(frame).rotation * chest_rest.rotation.inversed()
        return (delta * rotator.quaternion()).rotator()

    cradle_wrist = local(CRADLE_WRIST_L)
    cradle_elbow = local(CRADLE_ELBOW_POLE_L)
    # Folded deep over the forward thigh there's no room for the bundle at the belly, so she hugs it
    # up against her chest; blended in by how far she leans.
    cradle_hug = local(_add(CRADLE_WRIST_L, HUG_FROM_CRADLE_L))
    elbow_hug = local(_add(CRADLE_ELBOW_POLE_L, (0.0, 0.0, 20.0)))

    def lean_at(frame):
        keys = sorted(lean)
        for a, b in zip(keys, keys[1:]):
            if a <= frame <= b:
                return lean[a] + (lean[b] - lean[a]) * (frame - a) / (b - a)
        return 0.0

    def hug(frame):
        return min(1.0, max(0.0, (lean_at(frame) - 30.0) / 40.0))

    cradle_ahead = local(_add(CRADLE_WRIST_L, (8.0, 10.0, 0.0)))
    lay_point = local(tuple(a + b for a, b in zip(CRADLE_WRIST_L, LAY_FROM_CRADLE_R)))

    # Right hand: side -> reach -> grasp -> lay the stick on the bundle, twice -> support it -> side.
    side_r = (-24.0, 5.0, 86.0)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    grab_r = s.hand_turn('r', (0.2, 0.6, -0.8), (0, 0, -1))
    lay_r = s.hand_turn('r', (1, 0.3, 0), (0, 0, -1))
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['step'], 'hand_r_ik_ctrl', (-22.0, 12.0, 80.0), hang_r)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-24.0, 22.0, 40.0), hang_r)
    for n, stick in ((1, STICK_1), (2, STICK_2)):
        s.key_world(F[f'reach{n}'], 'hand_r_ik_ctrl', _add(stick, (0, -4, 14)), grab_r)
        s.key_world(F[f'grab{n}'], 'hand_r_ik_ctrl', _add(stick, (0, -6, 8)), grab_r)
        s.key_world(F[f'lift{n}'], 'hand_r_ik_ctrl', _add(stick, (6, -8, 30)), grab_r)
        p = F[f'place{n}']
        s.key_world(p, 'hand_r_ik_ctrl', on_chest(p, lay_point), chest_turn(p, lay_r))
    # Dense keys while she rises: the chest follows a curve that sparse component-space keys cut through.
    for f in list(range(F['place2'] + 3, F['settle'], 3)) + [F['settle']]:
        s.key_world(f, 'hand_r_ik_ctrl', on_chest(f, lay_point - unreal.Vector(0, 0, 6)), chest_turn(f, lay_r))
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)

    # Left hand: side -> brace on the forward knee while kneeling -> cradle the bundle -> side.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    cradle_l = s.hand_turn('l', (-1, 0.2, 0), (0, 0, 1))
    knee_l = s.hand_turn('l', fwd, (0, 0, -1))
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', (16.0, 38.0, 54.0), knee_l)
    s.key_world(F['grab1'], 'hand_l_ik_ctrl', (16.0, 38.0, 54.0), knee_l)
    # Keep bracing on the knee while the torso comes up, so the arm doesn't sweep through it.
    s.key_world(F['lift1'] - 3, 'hand_l_ik_ctrl', (16.0, 38.0, 56.0), knee_l)
    s.key_world(F['lift1'] + 1, 'hand_l_ik_ctrl', on_chest(F['lift1'] + 1, cradle_ahead), chest_turn(F['lift1'] + 1, cradle_l))
    cradle_frames = sorted(set(range(F['place1'] - 4, F['settle'], 3)) | {F['reach2'], F['grab2'], F['lift2'], F['place2'], F['settle']})
    for f in cradle_frames:
        w = hug(f)
        x, y, z = on_chest(f, cradle_wrist + (cradle_hug - cradle_wrist) * w)
        px, py, pz = on_chest(f, cradle_elbow + (elbow_hug - cradle_elbow) * w)
        if f < F['rise']:
            # Keep the bundle above the forward thigh and out in front of her chest.
            y = max(y, chest(f).translation.y + 18.0)
            z = max(z, THIGH_CLEAR_Z)
            pz = max(pz, THIGH_CLEAR_Z + 6.0)
        s.key_world(f, 'hand_l_ik_ctrl', (x, y, z), chest_turn(f, cradle_l))
        s.key_world(f, 'arm_l_pv_ik_ctrl', (px, py, pz))
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    # Elbows: right elbow out and back; left elbow out to the side while bracing on the knee.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['grab1'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))

    return s.bake(ANIM)


def _segment_distance(p1, q1, p2, q2):
    """Closest distance between segments p1-q1 and p2-q2 (sampled; plenty for a clearance check)."""
    best = 1e9
    for i in range(11):
        a = [p + (q - p) * i / 10 for p, q in zip(p1, q1)]
        for j in range(11):
            b = [p + (q - p) * j / 10 for p, q in zip(p2, q2)]
            best = min(best, sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5)
    return best


def clearance(anim, step=2):
    """Per-frame distance from each forearm (elbow to wrist) to the spine line (pelvis to neck).
    The chest is roughly 15 cm deep from the spine joints and a forearm about 4 cm thick, so values
    under about 16 cm mean the arm is inside her torso."""
    bones = ('pelvis', 'spine_03', 'spine_05', 'neck_01', 'lowerarm_l', 'hand_l', 'lowerarm_r', 'hand_r',
             'thigh_l', 'calf_l')
    rows = []
    for frame in range(0, FRAMES['end'] + 1, step):
        p = ra.bone_positions(anim, bones, frame / 30)
        v = {b: (x.translation.x, x.translation.y, x.translation.z) for b, x in p.items()}
        spine = (('pelvis', 'spine_03'), ('spine_03', 'spine_05'), ('spine_05', 'neck_01'))
        side = {}
        for s in 'lr':
            side[s] = round(min(_segment_distance(v[f'lowerarm_{s}'], v[f'hand_{s}'], v[a], v[b]) for a, b in spine), 1)
        # Left forearm to the raised left thigh (thigh about 8 cm in radius).
        thigh = round(_segment_distance(v['lowerarm_l'], v['hand_l'], v['thigh_l'], v['calf_l']), 1)
        rows.append((frame, side['l'], side['r'], thigh))
    return rows


def check(anim):
    """Contacts at key moments: knee/toes on the ground, hands at the sticks, feet planted."""
    rows = {}
    for name in ('kneel', 'grab1', 'grab2', 'rise', 'end'):
        t = FRAMES[name] / 30
        p = ra.bone_positions(anim, ('calf_r', 'ball_r', 'foot_r', 'foot_l', 'upperarm_r', 'hand_r', 'middle_03_r', 'hand_l', 'pelvis', 'head'), t)
        rows[name] = {b: tuple(round(v, 1) for v in (x.translation.x, x.translation.y, x.translation.z)) for b, x in p.items()}
    return rows
