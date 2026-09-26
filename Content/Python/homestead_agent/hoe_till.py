"""Hoeing a garden square for the MetaHuman heroine with the stone hoe (SM_StoneHoe).

    from homestead_agent import hoe_till as ht
    anim = ht.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_HoeTill
    print(ht.report(anim)) # blade edge against the soil at each key

She sets her feet (left foot forward, knees bent), bends from the hips over the square, raises the
hoe with the right hand down the haft and the left on its end, and chops the blade down into the
soil in front of her, then draws it back toward her to turn the earth. A second chop lands a hand's
width to the side, then she straightens and lets the hoe down to the carry. The game shows the
square's turned soil from EVENTS['chop1'].

SM_StoneHoe (Assets/Props/StoneHoe/report.json "attach"): pivot at the right hand's grip centre,
haft along +Z toward its top end (the left hand at LEFT_ALONG), blade at the far -Z end pointing
-Y (the direction the right hand's knuckles face in its closed grip, as for the hatchet).

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import axe_fell as af

SEQUENCE = 'LS_HoeTill'
ANIM = 'AN_HeroineMH_HoeTill'

FRAMES = {
    'stand': 0, 'set': 12, 'raise1': 22, 'chop1': 30, 'bite1': 33, 'draw1': 42,
    'raise2': 52, 'chop2': 60, 'bite2': 63, 'draw2': 72, 'recover': 86, 'end': 100,
}
EVENTS = {'chop1': FRAMES['chop1'] / 30, 'chop2': FRAMES['chop2'] / 30}
# From the grip pivot (report.json attach), cm.
LEFT_ALONG = 32.0
EDGE_ALONG = -75.45
EDGE_OUT = 19.31


def _v(t):
    return unreal.Vector(*t)


def _dirs(toward_crook):
    """Haft (+Z, toward the top) and blade (-Y) directions for a haft whose crook end points along
    ``toward_crook`` in her sagittal plane (plus a little sideways)."""
    c = _v(toward_crook).normal()
    haft = c * -1.0
    # The blade stands perpendicular to the haft in the swing plane, facing back toward her feet.
    side = unreal.Vector(1, 0, 0)
    blade = side.cross(haft).normal()
    if blade.z > 0 and c.z < 0:
        blade = blade * -1.0
    return haft, blade


# Right grip centre and the crook-ward direction of the haft at each key.
KEYS = {
    'set': ((-9.0, 30.0, 88.0), (0.02, 0.8, -0.6)),
    'raise1': ((-9.0, 30.0, 104.0), (0.02, 0.66, 0.75)),
    'chop1': ((-9.0, 40.0, 70.0), (0.02, 0.62, -0.78)),
    'bite1': ((-9.0, 39.0, 67.0), (0.02, 0.6, -0.8)),
    'draw1': ((-9.0, 27.0, 72.0), (0.02, 0.72, -0.69)),
    'raise2': ((-13.0, 30.0, 104.0), (-0.05, 0.66, 0.75)),
    'chop2': ((-13.0, 40.0, 70.0), (-0.05, 0.62, -0.78)),
    'bite2': ((-13.0, 39.0, 67.0), (-0.05, 0.6, -0.8)),
    'draw2': ((-13.0, 27.0, 72.0), (-0.05, 0.72, -0.69)),
    'recover': ((-18.0, 20.0, 86.0), (0.0, 0.55, -0.83)),
}
# Pelvis drop/forward (cm) and forward bend (deg).
BODY = {
    'stand': ((0, 0, 0), 0), 'set': ((0, 2, -8), 18), 'raise1': ((0, 0, -6), 12),
    'chop1': ((0, 5, -14), 38), 'bite1': ((0, 5, -15), 40), 'draw1': ((0, 2, -12), 32),
    'raise2': ((0, 0, -6), 12), 'chop2': ((0, 5, -14), 38), 'bite2': ((0, 5, -15), 40),
    'draw2': ((0, 2, -12), 32), 'recover': ((0, 1, -5), 10), 'end': ((0, 0, 0), 0),
}
POLE_R = (-60.0, -20.0, 80.0)
POLE_L = (60.0, -10.0, 90.0)
FOOT_L_FORWARD = (14.0, 18.0, 8.6)
FOOT_R_BACK = (-15.0, -12.0, 8.6)


def edge_point(key):
    centre, crook = KEYS[key]
    haft, blade = _dirs(crook)
    return _v(centre) + haft * EDGE_ALONG + blade * EDGE_OUT


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    right, left = af.Hand(s, 'r'), af.Hand(s, 'l')
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        drop, bend = BODY[name]
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop), unreal.Rotator(roll=bend * 0.45, pitch=0, yaw=0))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=bend * 0.18)
        s.key_rotation(frame, 'neck_01_ctrl', roll=bend * 0.05)
        s.key_rotation(frame, 'head_ctrl', roll=(6 + bend * 0.1) if frame else 0)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R if name not in ('stand', 'end') else (-45.0, -30.0, 100.0))
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L if name not in ('stand', 'end') else (45.0, -30.0, 100.0))
        if name in ('stand', 'end'):
            position, blade, edge = af.WRIST_R_STAND
            b, e = af._norm(blade), af._norm(edge)
            s.key_world(frame, 'hand_r_ik_ctrl', position, right.turn(b, (e - b * e.dot(b)).normal()))
            s.key_world(frame, 'hand_l_ik_ctrl', af.WRIST_L_STAND, s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
            continue
        centre, crook = KEYS[name]
        haft, blade = _dirs(crook)
        s.key_world(frame, 'hand_r_ik_ctrl', right.wrist(centre, haft, blade), right.turn(haft, blade))
        top = _v(centre) + haft * LEFT_ALONG
        s.key_world(frame, 'hand_l_ik_ctrl', left.wrist((top.x, top.y, top.z), haft, blade), left.turn(haft, blade))
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(5, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 9, 6)))
    s.key_world(F['set'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 6, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 9, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['set'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['set'], 'leg_l_pv_ik_ctrl', (25.0, 80.0, 50.0))
    s.key_world(F['set'], 'leg_r_pv_ik_ctrl', (-25.0, 80.0, 50.0))
    return s.bake(ANIM)


def report(anim):
    """Baked right grip centre vs keyed, the left grip's spacing up the haft, and the edge point."""
    bones = [f'{b}_{side}' for side in 'lr' for b in ('hand', 'middle_01', 'index_01', 'pinky_01')]
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, bones, frame / 30)
        centre_r, across_r = af._grip_frame(b, 'r')
        centre_l, _ = af._grip_frame(b, 'l')
        want = KEYS.get(name, (None,))[0]
        edge = edge_point(name) if name in KEYS else None
        e = f"edge ({edge.x:5.1f},{edge.y:5.1f},{edge.z:5.1f})" if edge else ''
        lines.append(f"{name:8s} grip ({centre_r.x:6.1f},{centre_r.y:6.1f},{centre_r.z:6.1f}) want {want}  "
                     f"hands {(centre_l - centre_r).length():5.1f} cm  {e}")
    return '\n'.join(lines)
