"""Crafting by hand for the MetaHuman heroine: she works a piece in front of her while the craft
square fills.

    from homestead_agent import craft_hands as ch
    anim = ch.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_CraftHands
    print(ch.report(anim)) # wrist positions, hand gap and forearm clearance at each key

The game layers this clip from spine_02 up (both arms, chest and head) over her standing pose while
a Craft recipe is held in the field book, and plays it in step with the hold: one loop is one craft
cycle (``SHomesteadMenu::CraftCycleSeconds``, 1.2 s). Her left fist holds a branch upright in
front of her stomach (the CraftPiece prop); her right fist closes on it higher up and, on each of
the three craft beats (the knocks at 0.18, 0.58 and 0.98 s), presses down and twists as if seating
a head on the haft, then eases back. She leans in a
little and looks down at her hands. First and last frames match so it loops.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import active_idle as ai

SEQUENCE = 'LS_CraftHands'
ANIM = 'AN_HeroineMH_CraftHands'
LENGTH = 36  # 1.2 s at 30 fps, one craft cycle
# The craft beats (SHomesteadMenu::Tick BeatTimes) in frames, and the lifts between them.
BEATS = (5, 17, 29)
LIFTS = (11, 23)
# The work is a branch held upright in her left fist in front of her stomach (the game puts the
# CraftPiece prop through that fist); her right fist closes on it a hand's width higher, where a
# head or binding would go.
WRIST_L = (8.0, 33.0, 110.0)
WRIST_R = (-5.0, 34.0, 123.0)
PRESS = 2.4      # cm the right fist pushes down the branch on a beat
COUNTER = 0.8    # cm the left fist rises to meet it
TWIST = 22.0     # degrees the right fist turns about the branch on a beat
# Both fists thumb-up around the branch: fingers forward and in, palms facing each other's side.
LEFT_FINGERS = (-0.5, 0.86, 0.0)
LEFT_PALM = (-0.86, -0.5, 0.0)
RIGHT_FINGERS = (0.5, 0.86, 0.0)
RIGHT_PALM = (0.86, -0.5, 0.0)
# Elbows down and a little out, clear of her ribs.
POLE_L = (48.0, -35.0, 88.0)
POLE_R = (-48.0, -35.0, 88.0)
# Both hands closed around the work (yaw on {finger}_0N ctrl; negative curls toward the palm).
GRIP_L = {'index': (-28.0, -40.0, -22.0), 'middle': (-32.0, -44.0, -24.0),
          'ring': (-36.0, -46.0, -26.0), 'pinky': (-40.0, -48.0, -28.0)}
GRIP_R = dict(GRIP_L)
# Forward lean (spine roll bends forward) and her gaze down at the work.
LEAN = {'spine_01_ctrl': 1.5, 'spine_02_ctrl': 2.5, 'spine_03_ctrl': 2.5}
NECK_ROLL = 18.0
HEAD_ROLL = 32.0


def _press(frame):
    """0 at rest, 1 at a beat: a quick push down and a slower lift back up."""
    for beat in BEATS:
        d = frame - beat
        if -3 <= d <= 0:
            return (d + 3) / 3.0
        if 0 < d <= 5:
            return 1.0 - d / 5.0
    return 0.0


def build(anim=ANIM):
    s = ra.Session(SEQUENCE, frames=LENGTH)
    for frame in (0, LENGTH):
        s.key_bool(frame, 'arm_l_fk_ik_switch', True)
        s.key_bool(frame, 'arm_r_fk_ik_switch', True)
        s.key_world(frame, 'foot_l_ik_ctrl', ai.FOOT_L, unreal.Rotator(roll=0, pitch=0, yaw=-ai.TOE_OUT))
        s.key_world(frame, 'foot_r_ik_ctrl', ai.FOOT_R, unreal.Rotator(roll=0, pitch=0, yaw=ai.TOE_OUT))
        s.key_world(frame, 'leg_l_pv_ik_ctrl', (ai.FOOT_L[0] + 3.0, 60.0, 50.0))
        s.key_world(frame, 'leg_r_pv_ik_ctrl', (ai.FOOT_R[0] - 3.0, 60.0, 50.0))
        s.key_world(frame, 'body_ctrl', ai.BODY)
        for control, roll in LEAN.items():
            s.key_rotation(frame, control, roll=roll)
        for side, grip in (('l', GRIP_L), ('r', GRIP_R)):
            for finger, degrees in grip.items():
                for joint, deg in zip(('01', '02', '03'), degrees):
                    s.key_rotation(frame, f'{finger}_{joint}_{side}_ctrl', yaw=deg,
                                   pitch=ai.SPREAD_L.get(finger, 0.0) * 0.5 if joint == '01' else 0.0)
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R)

    left = s.hand_turn('l', LEFT_FINGERS, LEFT_PALM)
    keys = sorted(set((0, LENGTH) + BEATS + LIFTS + tuple(b - 3 for b in BEATS)))
    for frame in keys:
        p = _press(frame)
        # The right fist turns about the upright branch as it presses, like seating a head on a haft.
        up = unreal.Vector(0, 0, 1)
        fingers = unreal.MathLibrary.rotate_angle_axis(unreal.Vector(*RIGHT_FINGERS), TWIST * p, up)
        palm = unreal.MathLibrary.rotate_angle_axis(unreal.Vector(*RIGHT_PALM), TWIST * p, up)
        right = s.hand_turn('r', (fingers.x, fingers.y, fingers.z), (palm.x, palm.y, palm.z))
        s.key_world(frame, 'hand_l_ik_ctrl', kg._add(WRIST_L, (0.0, 0.0, COUNTER * p)), left)
        s.key_world(frame, 'hand_r_ik_ctrl', kg._add(WRIST_R, (0.4 * p, 0.6 * p, -PRESS * p)), right)
        nod = 1.5 * p
        s.key_rotation(frame, 'neck_01_ctrl', roll=NECK_ROLL + 0.3 * nod)
        s.key_rotation(frame, 'head_ctrl', roll=HEAD_ROLL + 0.7 * nod, yaw=-3.0)
    return s.bake(anim)


def report(anim):
    lines = []
    for frame in (0,) + BEATS + LIFTS:
        b = ra.bone_positions(anim, ('hand_l', 'hand_r', 'lowerarm_l', 'lowerarm_r', 'spine_03', 'head'), frame / 30)
        p = {k: v.translation for k, v in b.items()}
        spine = p['spine_03']
        lines.append(
            f"f{frame:2d} hand_l ({p['hand_l'].x:5.1f},{p['hand_l'].y:5.1f},{p['hand_l'].z:6.1f})  "
            f"hand_r ({p['hand_r'].x:5.1f},{p['hand_r'].y:5.1f},{p['hand_r'].z:6.1f})  "
            f"gap {(p['hand_l'] - p['hand_r']).length():5.1f}  "
            f"elbows to spine {(p['lowerarm_l'] - spine).length():5.1f} / {(p['lowerarm_r'] - spine).length():5.1f}  "
            f"head z {p['head'].z:6.1f}")
    return '\n'.join(lines)
