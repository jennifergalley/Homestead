"""Mr. Trethewey's idle behind the General Store counter: leaning on it with both palms flat on the top.

    from homestead_agent import clerk_counter_idle as cci
    anim = cci.build()       # bakes /Game/Characters/Clerk_MH/Animations/AN_ClerkMH_CounterIdle
    print(cci.report(anim))  # palm height over the counter top, elbow bend, feet spacing

He stands with his feet a little behind his hips and shoulder width apart, his weight leaning
forward onto his hands, palms flat on the counter top and fingers relaxed and turned a touch
inward, elbows softly bent and out. Over the 8 s loop he breathes slowly, eases his weight from one
hand to the other and back, and glances along the counter and back to the door. The hands stay
planted (IK) while the body moves over them. First and last frames match so it loops.

Counter geometry (HomesteadGeneralStore.cpp): he stands at DoorToCounter facing the door; the
SM_Store_Counter centre is 70 cm in front of him and it is 88.5 cm deep and 98.9 cm high
(Assets/Props/StoreCounter/report.json), so its back edge is 26 cm in front of him.

Component space (as the heroine): forward +Y, his left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra

SEQUENCE = 'LS_ClerkCounterIdle'
ANIM = 'AN_ClerkMH_CounterIdle'
AUTHORING = '/Game/Characters/Clerk_MH/Animations/Authoring'
OUTPUT = '/Game/Characters/Clerk_MH/Animations'
BODY = '/Game/Characters/Clerk_MH/Assembled/Clerk/Body/SKM_MHC_Clerk_BodyMesh'
RIG = '/Game/Characters/Clerk_MH/Common/Common/MetaHuman_ControlRig'
ACTOR_LABEL = 'ClerkRigAuthoring'
LENGTH = 240  # 8 s at 30 fps

COUNTER_BACK_Y = 26.0
COUNTER_TOP_Z = 98.9
# Hand bone (wrist) above the counter with the palm flat on it; checked by report().
WRIST_ABOVE_TOP = 7.0
HAND_X = 21.0
HAND_Y = COUNTER_BACK_Y + 11.0
FEET_HALF_WIDTH = 15.0
FEET_BACK = (-7.0, -10.0)  # left, right: a little behind the hips as he leans
TOE_OUT = 9.0
# Hips settle a touch (soft knees) and stay back while the chest leans over the hands.
HIP_DROP = 1.5
SPINE_LEAN = (('spine_01_ctrl', 3.0), ('spine_02_ctrl', 4.0), ('spine_03_ctrl', 4.0))
NECK_LIFT = -4.0   # neck and head counter the lean so his gaze stays level on the door
HEAD_LIFT = -5.0
ELBOW_POLE = (55.0, -35.0, 115.0)
# Fingers lie nearly flat on the wood, slightly curled at the tips and drawn together.
CURL = {'index': (-3.0, -5.0, -3.0), 'middle': (-4.0, -6.0, -3.0),
        'ring': (-5.0, -7.0, -4.0), 'pinky': (-6.0, -8.0, -4.0)}
SPREAD = {'index': 5.0, 'middle': 1.5, 'ring': -2.0, 'pinky': -5.0}


class _ClerkRig:
    """Points rig_authoring at the clerk's body, rig and folders for the duration of a bake."""
    NAMES = ('BODY', 'RIG', 'AUTHORING', 'ACTOR_LABEL')

    def __enter__(self):
        self.saved = {n: getattr(ra, n) for n in self.NAMES}
        ra.BODY, ra.RIG, ra.AUTHORING, ra.ACTOR_LABEL = BODY, RIG, AUTHORING, ACTOR_LABEL
        return self

    def __exit__(self, *exc):
        for name, value in self.saved.items():
            setattr(ra, name, value)
        return False


def _wave(frame, cycles=1.0, phase=0.0):
    return math.sin(2 * math.pi * (cycles * frame / LENGTH + phase))


def build(anim=ANIM, wrist_above=WRIST_ABOVE_TOP, hand_y=HAND_Y, lean=1.0):
    with _ClerkRig():
        s = ra.Session(SEQUENCE, frames=LENGTH)
        if not unreal.EditorAssetLibrary.does_directory_exist(OUTPUT):
            unreal.EditorAssetLibrary.make_directory(OUTPUT)
        body_rest = s.rest('body_ctrl').translation
        foot_rest = s.rest('foot_l_ik_ctrl').translation
        foot_z = foot_rest.z
        for side in ('l', 'r'):
            s.key_bool(0, f'arm_{side}_fk_ik_switch', True)
            s.key_bool(LENGTH, f'arm_{side}_fk_ik_switch', True)
        foot_l = (FEET_HALF_WIDTH, FEET_BACK[0], foot_z)
        foot_r = (-FEET_HALF_WIDTH, FEET_BACK[1], foot_z)
        for frame in (0, LENGTH):
            s.key_world(frame, 'foot_l_ik_ctrl', foot_l, unreal.Rotator(roll=0, pitch=0, yaw=-TOE_OUT))
            s.key_world(frame, 'foot_r_ik_ctrl', foot_r, unreal.Rotator(roll=0, pitch=0, yaw=TOE_OUT))
            s.key_world(frame, 'leg_l_pv_ik_ctrl', (foot_l[0] + 4.0, 60.0, 50.0))
            s.key_world(frame, 'leg_r_pv_ik_ctrl', (foot_r[0] - 4.0, 60.0, 50.0))

        hand_z = COUNTER_TOP_Z + wrist_above
        flat_l = s.hand_turn('l', (-0.3, 1.0, -0.04), (0, 0, -1))
        flat_r = s.hand_turn('r', (0.3, 1.0, -0.04), (0, 0, -1))
        for frame in range(0, LENGTH + 1, 20):
            breath = _wave(frame, 2)  # two slow breaths per loop
            shift = _wave(frame, 1)   # weight eases toward his left hand, then his right
            body = (body_rest.x + 1.6 * shift, body_rest.y + 0.4 * breath,
                    body_rest.z - HIP_DROP + 0.3 * breath)
            s.key_world(frame, 'body_ctrl', body, unreal.Rotator(roll=0, pitch=1.5 * shift, yaw=1.5 * shift))
            for control, degrees in SPINE_LEAN:
                share = degrees / sum(d for _, d in SPINE_LEAN)
                s.key_rotation(frame, control, roll=lean * degrees - 1.0 * breath * share,
                               pitch=-1.5 * shift * share, yaw=-1.5 * shift * share)
            glance = _wave(frame, 1, 0.15)
            s.key_rotation(frame, 'neck_01_ctrl', roll=lean * NECK_LIFT + 0.4 * breath, yaw=5.0 * glance * 0.4)
            s.key_rotation(frame, 'head_ctrl', roll=lean * HEAD_LIFT, yaw=5.0 * glance * 0.6, pitch=1.0 * shift)
            # Planted palms: the hands only take the tiny give of the weight moving over them.
            s.key_world(frame, 'hand_l_ik_ctrl', (HAND_X, hand_y, hand_z), flat_l)
            s.key_world(frame, 'hand_r_ik_ctrl', (-HAND_X, hand_y, hand_z), flat_r)
            s.key_world(frame, 'arm_l_pv_ik_ctrl', ELBOW_POLE)
            s.key_world(frame, 'arm_r_pv_ik_ctrl', (-ELBOW_POLE[0], ELBOW_POLE[1], ELBOW_POLE[2]))
        for frame in (0, LENGTH):
            for side in ('l', 'r'):
                for finger, degrees in CURL.items():
                    for joint, deg in zip(('01', '02', '03'), degrees):
                        pitch = SPREAD.get(finger, 0.0) if joint == '01' else 0.0
                        s.key_rotation(frame, f'{finger}_{joint}_{side}_ctrl', yaw=deg, pitch=pitch)
        baked = s.bake(anim, folder=OUTPUT)
    # The shared metahuman_base_skel's reference pose is the female base; without his own mesh as the
    # retarget source, AnimationScaled bones (pelvis) stretch his whole pose ~7 cm up in game.
    clip = unreal.load_asset(f'{OUTPUT}/{anim}')
    clip.set_editor_property('retarget_source_asset', unreal.load_asset(BODY))
    unreal.EditorAssetLibrary.save_loaded_asset(clip)
    return baked


def _angle(a, b, c):
    u, v = a - b, c - b
    cos = u.dot(v) / max(u.length() * v.length(), 1e-6)
    return math.degrees(math.acos(max(-1.0, min(1.0, cos))))


def report(anim):
    """Per sample: lowest point of each hand (finger tips/knuckles/wrist) relative to the counter top,
    hand forward distance past the counter's back edge, elbow angle (180 = straight), feet spacing."""
    bones = ['hand_l', 'hand_r', 'lowerarm_l', 'lowerarm_r', 'upperarm_l', 'upperarm_r', 'foot_l', 'foot_r', 'head']
    for side in ('l', 'r'):
        for finger in ('index', 'middle', 'ring', 'pinky', 'thumb'):
            bones += [f'{finger}_01_{side}', f'{finger}_03_{side}']
    lines = []
    for frame in (0, LENGTH // 4, LENGTH // 2, 3 * LENGTH // 4):
        p = {k: v.translation for k, v in ra.bone_positions(anim, bones, frame / 30).items()}
        parts = [f'f{frame:3d}']
        for side in ('l', 'r'):
            hand = [p[b] for b in p if b.endswith(f'_{side}') and b.split('_')[0] in
                    ('hand', 'index', 'middle', 'ring', 'pinky', 'thumb')]
            low = min(v.z for v in hand) - COUNTER_TOP_Z
            fwd = p[f'hand_{side}'].y - COUNTER_BACK_Y
            elbow = _angle(p[f'upperarm_{side}'], p[f'lowerarm_{side}'], p[f'hand_{side}'])
            parts.append(f'{side}: low {low:+5.1f} wrist {p[f"hand_{side}"].z - COUNTER_TOP_Z:+5.1f} '
                         f'past edge {fwd:5.1f} elbow {elbow:5.1f}')
        parts.append(f"feet {(p['foot_l'] - p['foot_r']).length():5.1f} head ({p['head'].y:5.1f},{p['head'].z:6.1f})")
        lines.append('  '.join(parts))
    return '\n'.join(lines)
