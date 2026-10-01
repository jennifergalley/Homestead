"""Anatomical joint-limit checker for the MetaHuman heroine's clips (realistic-animation skill).

    from homestead_agent import joint_limits as jl
    print('\\n'.join(jl.report(anim, events=FRAMES)))   # editor: a baked AnimSequence, every frame
    result = jl.check_frames(frames, neutral)          # anywhere: poses as {bone: (location, quaternion)}

Poses are component space (rig_authoring's frame: forward +Y, her left +X, up +Z); locations in cm,
quaternions (x, y, z, w). Joint angles are measured two ways, never from raw local Euler channels
(MetaHuman's local axes differ per bone and per side):
- absolute, from positions in body frames: shoulder and hip elevation (flexion/extension and
  abduction/adduction in the chest and pelvis frames) and elbow and knee flexion. These read 0 at
  anatomical neutral whatever the bind pose is (MetaHuman binds in an A-pose, arms lowered about 45
  degrees);
- relative to a neutral pose (the skeleton's reference pose in the editor): the child bone's rotation
  relative to its parent, minus the neutral relation, split into swing along anatomical directions
  (flexion toward the front, the palm, upward...) and twist about the segment. That covers the spine,
  neck, head, clavicle, wrist, fingers, thumb, ankle, toes and the rotations (shoulder and hip
  internal/external rotation, forearm pronation/supination, tibial rotation, subtalar inversion).

Bands (comfortable, extreme) come from clinical range-of-motion norms for adult women; the sources and
the reasoning are in .github/skills/realistic-animation/SKILL.md. Between the bands is a warning (the
pose reads strained); beyond the extreme band is an error. Coupling checks catch combinations that are
each legal but not together (an arm overhead on a still clavicle, a fist on a flexed wrist, a DIP bent
with its PIP straight...). Speeds are checked against per-joint pop ceilings at 30 fps, and contacts,
foot slides and the centre of mass over the support as before.

The core imports nothing from unreal, so Tests/JointLimitsTests.py checks it with synthetic poses. The
editor adapter at the bottom (pose_at, neutral_pose, report) imports unreal lazily.
"""
import math

# ---------------------------------------------------------------------------------------------- constants

FPS = 30.0
FORWARD = (0.0, 1.0, 0.0)
BACK = (0.0, -1.0, 0.0)
LEFT = (1.0, 0.0, 0.0)
UP = (0.0, 0.0, 1.0)
DOWN = (0.0, 0.0, -1.0)
# A bone this close to or below the floor plane (cm) counts as touching / penetrating it.
CONTACT_CM = 3.0
PENETRATION_CM = 1.5
# A planted contact moving faster than this along the floor slides (cm/s): 0.5 cm a frame at 30 fps.
SLIDE_CM_PER_S = 15.0
# The centre of mass should stay this far inside the support polygon in a held pose (cm).
COM_MARGIN_CM = 2.0
# A limb's elevation is only split into flexion and abduction when its projection on that plane is at
# least this long (unit vector), so an arm straight out to the side has no flexion reading.
PLANE_MIN = 0.35
# Speed issues within this many frames of a listed contact (a strike's bite) are only warnings.
CONTACT_GRACE_FRAMES = 2


# ------------------------------------------------------------------------------------------- vector math

def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(dot(a, a))


def norm(a):
    n = length(a)
    return (0.0, 0.0, 0.0) if n < 1e-12 else (a[0] / n, a[1] / n, a[2] / n)


def reject(a, axis):
    """``a`` with its component along the unit ``axis`` removed."""
    return sub(a, scale(axis, dot(a, axis)))


def angle_between(a, b):
    return math.degrees(math.acos(max(-1.0, min(1.0, dot(norm(a), norm(b))))))


def signed_angle(a, b, axis):
    """Angle (degrees) from ``a`` to ``b`` about ``axis``, both projected on the plane normal to it."""
    ax = norm(axis)
    pa, pb = norm(reject(a, ax)), norm(reject(b, ax))
    return math.degrees(math.atan2(dot(ax, cross(pa, pb)), dot(pa, pb)))


def wrap(degrees):
    return (degrees + 180.0) % 360.0 - 180.0


# Quaternions are (x, y, z, w), the same order as unreal.Quat, and rotate vectors as q * v * q^-1.

def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def q_inv(q):
    x, y, z, w = q
    n = x * x + y * y + z * z + w * w
    return (-x / n, -y / n, -z / n, w / n)


def q_norm(q):
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def q_rotate(q, v):
    p = q_mul(q_mul(q, (v[0], v[1], v[2], 0.0)), q_inv(q))
    return (p[0], p[1], p[2])


def q_axis_angle(axis, degrees):
    a = norm(axis)
    h = math.radians(degrees) * 0.5
    s = math.sin(h)
    return (a[0] * s, a[1] * s, a[2] * s, math.cos(h))


IDENTITY = (0.0, 0.0, 0.0, 1.0)


def twist_angle(q, axis):
    """Signed twist (degrees) of ``q`` about the unit ``axis`` (swing-twist decomposition)."""
    x, y, z, w = q
    p = dot((x, y, z), axis)
    if abs(p) < 1e-12 and abs(w) < 1e-12:
        return 180.0
    return wrap(2.0 * math.degrees(math.atan2(p, w)))


# ------------------------------------------------------------------------------------------ joint tables

def _band(comfortable, extreme):
    return {"comfortable": comfortable, "extreme": extreme}


# Joints measured relative to the neutral pose. Each: the parent and child bones; the segment whose
# direction the child's rotation swings (from, to) at neutral; the anatomical direction flexion swings
# toward and the side direction; and bands per axis (None: not measured here). Directions: 'forward',
# 'up', 'left', 'lateral' (away from her midline on that side), 'palm' (out of the palm, into a grip),
# 'thumb' (toward the thumb side). Twist is positive toward internal rotation / pronation / turning to her
# right / inversion; twist_left_sign fixes the left side's sign where the anatomical name isn't mirrored
# the default way. Names ending in _l/_r get the side; 'pelvis', 'head', spine_* and neck_* don't.
FINGERS = ("index", "middle", "ring", "pinky")
RIG_JOINTS = [
    # --- spine: the thoracolumbar range spread over five bones; lumbar twist is tiny.
    dict(name="spine_01", parent="pelvis", child="spine_01", seg=("spine_01", "spine_02"), flex="forward", side="left",
         bands=dict(flex=_band((-10, 14), (-15, 20)), side=_band((-6, 6), (-12, 12)), twist=_band((-2, 2), (-5, 5)))),
    dict(name="spine_02", parent="spine_01", child="spine_02", seg=("spine_02", "spine_03"), flex="forward", side="left",
         bands=dict(flex=_band((-10, 14), (-15, 20)), side=_band((-7, 7), (-12, 12)), twist=_band((-3, 3), (-5, 5)))),
    dict(name="spine_03", parent="spine_02", child="spine_03", seg=("spine_03", "spine_04"), flex="forward", side="left",
         bands=dict(flex=_band((-8, 12), (-15, 20)), side=_band((-7, 7), (-12, 12)), twist=_band((-8, 8), (-15, 15)))),
    dict(name="spine_04", parent="spine_03", child="spine_04", seg=("spine_04", "spine_05"), flex="forward", side="left",
         bands=dict(flex=_band((-8, 10), (-15, 20)), side=_band((-7, 7), (-12, 12)), twist=_band((-10, 10), (-15, 15)))),
    dict(name="spine_05", parent="spine_04", child="spine_05", seg=("spine_05", "neck_01"), flex="forward", side="left",
         bands=dict(flex=_band((-8, 8), (-15, 20)), side=_band((-6, 6), (-12, 12)), twist=_band((-12, 12), (-15, 15)))),
    # --- neck and head: the cervical range over three bones; no one bone turns more than 35-40 degrees.
    dict(name="neck_01", parent="spine_05", child="neck_01", seg=("neck_01", "neck_02"), flex="forward", side="left",
         bands=dict(flex=_band((-25, 20), (-30, 30)), side=_band((-18, 18), (-22, 22)), twist=_band((-20, 20), (-35, 35)))),
    dict(name="neck_02", parent="neck_01", child="neck_02", seg=("neck_02", "head"), flex="forward", side="left",
         bands=dict(flex=_band((-20, 15), (-25, 25)), side=_band((-15, 15), (-20, 20)), twist=_band((-35, 35), (-40, 40)))),
    dict(name="head", parent="neck_02", child="head", seg=("neck_02", "head"), flex="forward", side="left",
         bands=dict(flex=_band((-8, 10), (-12, 20)), side=_band((-7, 7), (-12, 12)), twist=_band((-20, 20), (-35, 35)))),
    # --- shoulder girdle: elevation (up) and protraction (forward) of the clavicle.
    dict(name="clavicle", sides="lr", parent="spine_05", child="clavicle", seg=("clavicle", "upperarm"), flex="up",
         side="forward", bands=dict(flex=_band((-5, 20), (-10, 40)), side=_band((-15, 15), (-30, 35)), twist=None)),
    # --- rotations of the long bones about themselves.
    dict(name="shoulder", sides="lr", parent="clavicle", child="upperarm", seg=("upperarm", "lowerarm"),
         bands=dict(flex=None, side=None, twist=_band((-90, 70), (-110, 100)))),
    # Forearm roll: the hand's twist about the forearm relative to the upper arm, wherever the rig keys it.
    dict(name="forearm", sides="lr", parent="upperarm", child="hand", seg=("lowerarm", "hand"),
         bands=dict(flex=None, side=None, twist=_band((-80, 80), (-90, 85)))),
    dict(name="wrist", sides="lr", parent="lowerarm", child="hand", seg=("hand", "middle_01"), flex="palm", side="thumb",
         bands=dict(flex=_band((-40, 40), (-80, 85)), side=_band((-25, 15), (-40, 25)), twist=None)),
    *[dict(name=f"{f}_mcp", sides="lr", parent=f"{f}_metacarpal", child=f"{f}_01", seg=(f"{f}_01", f"{f}_02"),
           flex="palm", side="thumb",
           bands=dict(flex=_band((-15, 75), (-30, 100)), side=_band((-15, 15), (-25, 30)), twist=None)) for f in FINGERS],
    *[dict(name=f"{f}_pip", sides="lr", parent=f"{f}_01", child=f"{f}_02", seg=(f"{f}_02", f"{f}_03"),
           flex="palm", side="thumb", bands=dict(flex=_band((-5, 90), (-10, 115)), side=None, twist=None)) for f in FINGERS],
    *[dict(name=f"{f}_dip", sides="lr", parent=f"{f}_02", child=f"{f}_03", seg=(f"{f}_02", f"{f}_03"),
           flex="palm", side="thumb", bands=dict(flex=_band((-5, 55), (-10, 90)), side=None, twist=None)) for f in FINGERS],
    dict(name="thumb_mcp", sides="lr", parent="thumb_01", child="thumb_02", seg=("thumb_02", "thumb_03"), flex="palm",
         side="thumb", bands=dict(flex=_band((-10, 35), (-15, 55)), side=None, twist=None)),
    dict(name="thumb_ip", sides="lr", parent="thumb_02", child="thumb_03", seg=("thumb_02", "thumb_03"), flex="palm",
         side="thumb", bands=dict(flex=_band((-10, 60), (-15, 90)), side=None, twist=None)),
    dict(name="hip", sides="lr", parent="pelvis", child="thigh", seg=("thigh", "calf"),
         bands=dict(flex=None, side=None, twist=_band((-60, 40), (-75, 55)))),
    dict(name="knee", sides="lr", parent="thigh", child="calf", seg=("calf", "foot"),
         bands=dict(flex=None, side=None, twist=_band((-35, 25), (-40, 30)))),
    # Dorsiflexion lifts the foot (positive); the twist about the foot is subtalar inversion.
    dict(name="ankle", sides="lr", parent="calf", child="foot", seg=("foot", "ball"), flex="up", side=None,
         twist_left_sign=1.0, bands=dict(flex=_band((-50, 20), (-60, 30)), side=None, twist=_band((-8, 15), (-15, 25)))),
    # The MTP joints as one ball bone: extension lifts the toes (positive).
    dict(name="toes", sides="lr", parent="foot", child="ball", seg=("foot", "ball"), flex="up", side=None,
         bands=dict(flex=_band((-30, 70), (-45, 90)), side=None, twist=None)),
]

# Absolute measures: (key, family, band). Computed in absolute_angles().
ABSOLUTE = {
    "shoulder.flex": _band((-45, 150), (-60, 180)),
    "shoulder.abd": _band((-40, 150), (-75, 180)),
    "elbow.flex": _band((-5, 130), (-10, 152)),
    "hip.flex": _band((-15, 120), (-25, 135)),
    "hip.abd": _band((-25, 40), (-35, 55)),
    "knee.flex": _band((-5, 140), (-10, 156)),
}

# Regional totals of the per-bone readings.
TOTALS = {
    "lumbar.twist": (("spine_01", "spine_02"), "twist", _band((-5, 5), (-12, 12))),
    "spine.flex": (("spine_01", "spine_02", "spine_03", "spine_04", "spine_05"), "flex", _band((-25, 55), (-30, 70))),
    "spine.side": (("spine_01", "spine_02", "spine_03", "spine_04", "spine_05"), "side", _band((-30, 30), (-35, 35))),
    "spine.twist": (("spine_01", "spine_02", "spine_03", "spine_04", "spine_05"), "twist", _band((-35, 35), (-60, 60))),
    "neck.twist": (("neck_01", "neck_02", "head"), "twist", _band((-75, 75), (-85, 85))),
    "neck.flex": (("neck_01", "neck_02", "head"), "flex", _band((-55, 45), (-70, 60))),
}

# Readable names for each axis's positive and negative direction.
AXIS_NAMES = {
    "spine": {"flex": ("flexion", "extension"), "side": ("left bend", "right bend"), "twist": ("turn right", "turn left")},
    "clavicle": {"flex": ("elevation", "depression"), "side": ("protraction", "retraction")},
    "shoulder": {"flex": ("flexion", "extension"), "abd": ("abduction", "adduction"),
                 "twist": ("internal rotation", "external rotation")},
    "forearm": {"twist": ("pronation", "supination")},
    "elbow": {"flex": ("flexion", "hyperextension")},
    "wrist": {"flex": ("flexion", "extension"), "side": ("radial deviation", "ulnar deviation")},
    "finger": {"flex": ("flexion", "hyperextension"), "side": ("spread", "spread")},
    "hip": {"flex": ("flexion", "extension"), "abd": ("abduction", "adduction"),
            "twist": ("internal rotation", "external rotation")},
    "knee": {"flex": ("flexion", "hyperextension"), "twist": ("internal rotation", "external rotation")},
    "ankle": {"flex": ("dorsiflexion", "plantarflexion"), "twist": ("inversion", "eversion")},
    "toes": {"flex": ("extension", "flexion")},
}

# Angular-speed ceilings (deg/s) per family: (warning, error). Warnings at a tool swing's fast end,
# errors at the hard pop (degrees a frame at 30 fps x 30); realistic-animation skill, joint speed.
SPEED = {
    "head": (180, 240), "spine": (300, 360), "clavicle": (600, 1200), "shoulder": (900, 1200),
    "elbow": (1000, 1500), "forearm": (1200, 1800), "wrist": (1200, 1800), "finger": (1200, 2400),
    "hip": (700, 1000), "knee": (900, 1300), "ankle": (900, 1200), "toes": (900, 1200),
}


def family(key):
    """'wrist_l.flex' -> 'wrist'; spine and neck bones -> 'spine' / 'head'; finger joints -> 'finger'."""
    joint = key.split(".")[0]
    if joint.endswith(("_l", "_r")):
        joint = joint[:-2]
    if joint.startswith("spine") or joint == "lumbar":
        return "spine"
    if joint.startswith("neck") or joint == "head":
        return "head"
    if joint.endswith(("_mcp", "_pip", "_dip", "_ip")):
        return "finger"
    return joint


def describe(key, value):
    axis = key.split(".")[1]
    f = family(key)
    names = AXIS_NAMES.get("spine" if f == "head" else f, {}).get(axis)
    if not names:
        return f"{value:+.0f}"
    return f"{names[0] if value >= 0 else names[1]} {abs(value):.0f}"


def _side_bone(name, side):
    if not side or name in ("pelvis", "head") or name.startswith(("spine_", "neck_")):
        return name
    return f"{name}_{side}"


def hand_frame(pose, side):
    """The hand's knuckle direction, thumb-side direction and palm normal (out of the palm, into a grip)
    in component space, from bone positions."""
    hand = pose[f"hand_{side}"][0]
    along = norm(sub(pose[f"middle_01_{side}"][0], hand))
    across = sub(pose[f"index_01_{side}"][0], pose[f"pinky_01_{side}"][0])
    thumb = norm(reject(across, along))
    palm = norm(cross(along, thumb) if side == "r" else cross(thumb, along))
    return {"along": along, "thumb": thumb, "palm": palm}


def _direction(word, side, hf):
    lateral = (-1.0, 0.0, 0.0) if side == "r" else (1.0, 0.0, 0.0)
    return {"forward": FORWARD, "up": UP, "left": LEFT, "lateral": lateral,
            "palm": hf["palm"] if hf else FORWARD, "thumb": hf["thumb"] if hf else FORWARD}[word]


# ----------------------------------------------------------------------------------------- calibration

class Calibration:
    """What the measurements need from the neutral pose, computed once: per relative joint the segment
    and anatomical directions in the parent's frame and the neutral relation; the body frames; the
    elbow and knee hinge axes in the upper arm and thigh."""

    def __init__(self, neutral):
        self.neutral = neutral
        self.joints = {}
        frames = {s: hand_frame(neutral, s) for s in "lr"
                  if all(f"{b}_{s}" in neutral for b in ("hand", "middle_01", "index_01", "pinky_01"))}
        for j in RIG_JOINTS:
            for side in (j.get("sides") or [""]):
                name = _side_bone(j["name"], side) if side else j["name"]
                parent, child = _side_bone(j["parent"], side), _side_bone(j["child"], side)
                if parent not in neutral and j["parent"].endswith("_metacarpal"):
                    parent = _side_bone("hand", side)
                a, b = (_side_bone(x, side) for x in j["seg"])
                if parent not in neutral or child not in neutral or a not in neutral or b not in neutral:
                    continue
                d = norm(sub(neutral[b][0], neutral[a][0]))
                if length(d) < 0.5:
                    continue
                hf = frames.get(side)
                to_local = q_inv(neutral[parent][1])
                g = dict(def_=j, side=side, parent=parent, child=child,
                         rel0=q_mul(q_inv(neutral[parent][1]), neutral[child][1]), d=q_rotate(to_local, d))
                if j.get("flex"):
                    flex = norm(reject(_direction(j["flex"], side, hf), d))
                    g["e_flex"] = q_rotate(to_local, flex)
                    if j.get("side"):
                        sd = norm(reject(reject(_direction(j["side"], side, hf), d), flex))
                        if length(sd) < 0.5:
                            sd = norm(cross(d, flex))
                        g["e_side"] = q_rotate(to_local, sd)
                left_sign = j.get("twist_left_sign", -1.0)
                g["mirror"] = left_sign if side == "l" else -left_sign if side == "r" else 1.0
                self.joints[name] = g
        # Hinge axes: elbow flexion carries the forearm forward, knee flexion the shank back.
        self.hinges = {}
        for side in "lr":
            for key, parent, mid, flex_dir in (("elbow", "upperarm", "lowerarm", FORWARD), ("knee", "thigh", "calf", BACK)):
                p, m = f"{parent}_{side}", f"{mid}_{side}"
                if p in neutral and m in neutral:
                    seg = norm(sub(neutral[m][0], neutral[p][0]))
                    axis = norm(cross(seg, flex_dir))
                    if length(axis) < 0.5:
                        axis = LEFT
                    self.hinges[f"{key}_{side}"] = (p, q_rotate(q_inv(neutral[p][1]), axis))
        self.frame_bones = {"chest": "spine_05", "pelvis": "pelvis"}

    def body_frame(self, pose, which):
        """(forward, left, up) of the chest or pelvis: the neutral component axes carried by that bone."""
        bone = self.frame_bones[which]
        if bone not in pose or bone not in self.neutral:
            return None
        delta = q_mul(pose[bone][1], q_inv(self.neutral[bone][1]))
        return tuple(q_rotate(delta, v) for v in (FORWARD, LEFT, UP))

    def relative(self, pose, name):
        """{'flex', 'side', 'twist'} degrees for one relative joint (only its measured axes), or None."""
        g = self.joints.get(name)
        if g is None or g["parent"] not in pose or g["child"] not in pose:
            return None
        rel = q_mul(q_inv(pose[g["parent"]][1]), pose[g["child"]][1])
        delta = q_norm(q_mul(rel, q_inv(g["rel0"])))
        d = q_rotate(delta, g["d"])
        bands = g["def_"]["bands"]
        out = {}
        if bands.get("flex") and "e_flex" in g:
            out["flex"] = math.degrees(math.atan2(dot(d, g["e_flex"]), dot(d, g["d"])))
        if bands.get("side") and "e_side" in g:
            out["side"] = math.degrees(math.atan2(dot(d, g["e_side"]), math.hypot(dot(d, g["d"]), dot(d, g.get("e_flex", d)))))
        if bands.get("twist"):
            out["twist"] = twist_angle(delta, g["d"]) * g["mirror"]
        return out


def absolute_angles(cal, pose):
    """Shoulder and hip elevation in the chest and pelvis frames, and elbow and knee flexion."""
    out = {}
    for side, sign in (("l", 1.0), ("r", -1.0)):
        for limb, frame, a, b in (("shoulder", "chest", "upperarm", "lowerarm"), ("hip", "pelvis", "thigh", "calf")):
            fr = cal.body_frame(pose, frame)
            if fr is None or f"{a}_{side}" not in pose or f"{b}_{side}" not in pose:
                continue
            f, l, u = fr
            d = norm(sub(pose[f"{b}_{side}"][0], pose[f"{a}_{side}"][0]))
            df, dl, du = dot(d, f), dot(d, l) * sign, dot(d, u)
            if math.hypot(df, du) >= PLANE_MIN:
                out[f"{limb}_{side}.flex"] = math.degrees(math.atan2(df, -du))
            if math.hypot(dl, du) >= PLANE_MIN:
                out[f"{limb}_{side}.abd"] = math.degrees(math.atan2(dl, -du))
            out[f"{limb}_{side}.elevation"] = angle_between(d, scale(u, -1.0))
        for key, (top, mid, end) in (("elbow", ("upperarm", "lowerarm", "hand")), ("knee", ("thigh", "calf", "foot"))):
            name = f"{key}_{side}"
            bones = [f"{x}_{side}" for x in (top, mid, end)]
            if name not in cal.hinges or any(x not in pose for x in bones):
                continue
            parent, axis_local = cal.hinges[name]
            axis = q_rotate(pose[parent][1], axis_local)
            upper = sub(pose[bones[1]][0], pose[bones[0]][0])
            lower = sub(pose[bones[2]][0], pose[bones[1]][0])
            out[f"{name}.flex"] = signed_angle(upper, lower, axis)
    return out


def measure(cal, pose):
    """Every angle for one pose: {'wrist_l.flex': degrees, ...} (absent when it can't be measured)."""
    out = absolute_angles(cal, pose)
    for name in cal.joints:
        for axis, value in (cal.relative(pose, name) or {}).items():
            out[f"{name}.{axis}"] = value
    for key, (bones, axis, _) in TOTALS.items():
        values = [out.get(f"{b}.{axis}") for b in bones]
        if all(v is not None for v in values):
            out[key] = sum(values)
    return out


def band_for(key):
    if key in TOTALS:
        return TOTALS[key][2]
    joint, axis = key.split(".")
    base = joint[:-2] if joint.endswith(("_l", "_r")) else joint
    if f"{base}.{axis}" in ABSOLUTE:
        return ABSOLUTE[f"{base}.{axis}"]
    for j in RIG_JOINTS:
        if j["name"] == base:
            return j["bands"].get(axis)
    return None


def classify(value, band):
    lo, hi = band["comfortable"]
    elo, ehi = band["extreme"]
    if elo <= value <= ehi:
        return "ok" if lo <= value <= hi else "strained"
    return "beyond"


# --------------------------------------------------------------------------------------------- coupling

def coupling(values):
    """Combinations that are each legal but not together: [(key, severity, value, text)]."""
    out = []
    v = values.get
    for s in "lr":
        elevation, clav = v(f"shoulder_{s}.elevation"), v(f"clavicle_{s}.flex")
        if elevation is not None and clav is not None and elevation > 100 and clav < 5:
            out.append((f"shoulder_{s}.rhythm", "warn", elevation,
                        f"arm raised {elevation:.0f} with the clavicle still ({clav:+.0f}): add scapular upward rotation"))
        wflex = v(f"wrist_{s}.flex")
        pips = [v(f"{f}_pip_{s}.flex") for f in FINGERS]
        pips = [p for p in pips if p is not None]
        if wflex is not None and pips and wflex > 20 and sum(pips) / len(pips) > 70:
            out.append((f"wrist_{s}.tenodesis", "warn", wflex,
                        f"fist on a flexed wrist ({wflex:.0f}): a power grip wants 15-35 extension"))
        wdev = v(f"wrist_{s}.side")
        if wflex is not None and wdev is not None and abs(wflex) > 45 and (wdev > 15 or wdev < -20):
            out.append((f"wrist_{s}.deviation", "warn", wdev,
                        f"wrist deviation {describe(f'wrist_{s}.side', wdev)} at flexion {wflex:+.0f}: deviation shrinks off neutral"))
        for f in FINGERS:
            pip, dip = v(f"{f}_pip_{s}.flex"), v(f"{f}_dip_{s}.flex")
            if pip is not None and dip is not None and dip > 30 and pip < 20:
                out.append((f"{f}_dip_{s}.coupling", "warn", dip, f"{f} DIP {dip:.0f} with its PIP straight ({pip:.0f})"))
            if pip is not None and dip is not None and pip > 95 and dip < -5:
                out.append((f"{f}_dip_{s}.coupling", "warn", dip, f"{f} DIP extended with its PIP at {pip:.0f}"))
            mcp, spread = v(f"{f}_mcp_{s}.flex"), v(f"{f}_mcp_{s}.side")
            if mcp is not None and spread is not None and mcp > 45 and abs(spread) > 15:
                out.append((f"{f}_mcp_{s}.coupling", "warn", spread, f"{f} spread {abs(spread):.0f} in a fist (MCP {mcp:.0f})"))
        knee, hip = v(f"knee_{s}.flex"), v(f"hip_{s}.flex")
        if knee is not None and hip is not None and knee < 20 and hip > 90:
            out.append((f"hip_{s}.hamstring", "error" if hip > 110 else "warn", hip,
                        f"hip flexed {hip:.0f} with the knee straight ({knee:.0f}): hamstring-limited"))
        rot = v(f"knee_{s}.twist")
        if knee is not None and rot is not None and knee < 30 and abs(rot) > 10:
            out.append((f"knee_{s}.screwhome", "error" if abs(rot) > 15 else "warn", rot,
                        f"tibial rotation {rot:+.0f} at knee flexion {knee:.0f}: locked near extension"))
        ankle = v(f"ankle_{s}.flex")
        if knee is not None and ankle is not None and knee < 15 and ankle > 15:
            out.append((f"ankle_{s}.gastrocnemius", "warn", ankle,
                        f"dorsiflexion {ankle:.0f} with a straight knee: lift the heel or shift the pelvis"))
    neck, torso = v("neck.twist"), v("spine.twist")
    if neck is not None and torso is not None and abs(neck) > 75 and abs(torso) < 15:
        out.append(("neck.trunk", "warn", neck, f"head turned {neck:+.0f} with the torso still ({torso:+.0f})"))
    bend = v("spine.flex")
    hips = [h for h in (v("hip_l.flex"), v("hip_r.flex")) if h is not None]
    if bend is not None and hips and bend > 45 and max(hips) < 30:
        out.append(("spine.lumbopelvic", "warn", bend, f"spine bent {bend:.0f} with the hips nearly straight"))
    return out


# --------------------------------------------------------------------------------------------- checking

def check_frames(frames, neutral, fps=FPS, events=None, contacts=None, floor_z=0.0):
    """Check a clip. ``frames``: poses ({bone: (location, quaternion)}, component space); ``neutral``: the
    neutral (reference) pose; ``events``: {name: frame} key frames (balance is checked at them);
    ``contacts``: frames of strikes or impacts, where a fast joint is only a warning.
    Returns {'issues': [...], 'angles': [per-frame measure()]}. Each issue: frame, kind ('rom', 'coupling',
    'speed', 'ground', 'slide', 'balance'), severity ('warn' or 'error'), joint, value, text."""
    cal = Calibration(neutral)
    issues, history = [], []
    contacts = list(contacts or [])
    for i, pose in enumerate(frames):
        values = measure(cal, pose)
        history.append(values)
        for key, value in values.items():
            band = band_for(key)
            if band is None:
                continue
            status = classify(value, band)
            if status != "ok":
                issues.append(dict(frame=i, kind="rom", severity="error" if status == "beyond" else "warn",
                                   joint=key, value=value,
                                   text=f"{key} {describe(key, value)} deg (comfortable {band['comfortable']}, "
                                        f"extreme {band['extreme']})"))
        for key, severity, value, text in coupling(values):
            issues.append(dict(frame=i, kind="coupling", severity=severity, joint=key, value=value, text=text))
        if i:
            prev = history[i - 1]
            near_contact = any(abs(i - c) <= CONTACT_GRACE_FRAMES for c in contacts)
            for key, value in values.items():
                if key not in prev or key.endswith(".elevation"):
                    continue
                warn, error = SPEED.get(family(key), (900, 1800))
                speed = abs(wrap(value - prev[key])) * fps
                if speed > warn:
                    severity = "error" if speed > error and not near_contact else "warn"
                    issues.append(dict(frame=i, kind="speed", severity=severity, joint=key, value=speed,
                                       text=f"{key} {speed:.0f} deg/s ({warn} warns, {error} pops)"))
        issues.extend(_ground(pose, i, floor_z))
        if i:
            issues.extend(_slides(frames[i - 1], pose, i, fps, floor_z))
    issues.extend(_balance(frames, events or {}, floor_z, fps))
    return {"issues": issues, "angles": history}


# Bones that may touch the floor, and how far below the bone origin its surface lies (cm, approx.).
CONTACT_BONES = {"ball_l": 2.0, "ball_r": 2.0, "foot_l": 7.5, "foot_r": 7.5, "calf_l": 5.5, "calf_r": 5.5,
                 "middle_01_l": 1.5, "middle_01_r": 1.5, "hand_l": 2.5, "hand_r": 2.5}


def _ground(pose, i, floor_z):
    out = []
    for bone, skin in CONTACT_BONES.items():
        if bone not in pose:
            continue
        depth = floor_z - (pose[bone][0][2] - skin)
        if depth > PENETRATION_CM:
            out.append(dict(frame=i, kind="ground", severity="error" if depth > 4 else "warn", joint=bone,
                            value=depth, text=f"{bone} {depth:.1f} cm below the floor"))
    return out


def _slides(prev, pose, i, fps, floor_z):
    out = []
    for bone in ("ball_l", "ball_r", "foot_l", "foot_r", "calf_l", "calf_r"):
        if bone not in pose or bone not in prev:
            continue
        a, b = prev[bone][0], pose[bone][0]
        skin = CONTACT_BONES[bone]
        planted = a[2] - skin - floor_z < CONTACT_CM and b[2] - skin - floor_z < CONTACT_CM
        speed = math.hypot(b[0] - a[0], b[1] - a[1]) * fps
        if planted and speed > SLIDE_CM_PER_S:
            out.append(dict(frame=i, kind="slide", severity="error" if speed > 3 * SLIDE_CM_PER_S else "warn",
                            joint=bone, value=speed, text=f"{bone} slides {speed:.0f} cm/s while planted"))
    return out


# Segment mass fractions for women and each segment's centre of mass from its first bone (de Leva 1996;
# realistic-animation skill). The trunk and head spans are rig landmarks, so their centres are approximate.
SEGMENTS = [
    ("head", "neck_02", "head", 0.0668, 0.5),
    ("trunk", "pelvis", "neck_01", 0.4258, 0.5),
    ("upperarm_l", "upperarm_l", "lowerarm_l", 0.0255, 0.5754), ("upperarm_r", "upperarm_r", "lowerarm_r", 0.0255, 0.5754),
    ("forearm_l", "lowerarm_l", "hand_l", 0.0138, 0.4559), ("forearm_r", "lowerarm_r", "hand_r", 0.0138, 0.4559),
    ("hand_l", "hand_l", "middle_01_l", 0.0056, 0.7474), ("hand_r", "hand_r", "middle_01_r", 0.0056, 0.7474),
    ("thigh_l", "thigh_l", "calf_l", 0.1478, 0.3612), ("thigh_r", "thigh_r", "calf_r", 0.1478, 0.3612),
    ("shank_l", "calf_l", "foot_l", 0.0481, 0.4352), ("shank_r", "calf_r", "foot_r", 0.0481, 0.4352),
    ("foot_l", "foot_l", "ball_l", 0.0129, 0.4014), ("foot_r", "foot_r", "ball_r", 0.0129, 0.4014),
]


def centre_of_mass(pose):
    total, acc = 0.0, (0.0, 0.0, 0.0)
    for _, a, b, mass, at in SEGMENTS:
        if a in pose and b in pose:
            p = add(pose[a][0], scale(sub(pose[b][0], pose[a][0]), at))
            acc = add(acc, scale(p, mass))
            total += mass
    return scale(acc, 1.0 / total) if total else None


def support_points(pose, floor_z=0.0):
    """Floor-contact points (x, y) of the bones within CONTACT_CM of the floor."""
    return [(pose[b][0][0], pose[b][0][1]) for b, skin in CONTACT_BONES.items()
            if b in pose and pose[b][0][2] - skin - floor_z < CONTACT_CM]


def convex_hull(points):
    pts = sorted(set(points))
    if len(pts) <= 2:
        return pts

    def turn(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and turn(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and turn(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def distance_outside(point, hull):
    """How far (cm) ``point`` lies outside the convex ``hull``; 0 inside. One or two contacts measure to
    the point or segment."""
    if not hull:
        return float("inf")
    if len(hull) == 1:
        return math.hypot(point[0] - hull[0][0], point[1] - hull[0][1])

    def seg_dist(p, a, b):
        ab = (b[0] - a[0], b[1] - a[1])
        t = max(0.0, min(1.0, ((p[0] - a[0]) * ab[0] + (p[1] - a[1]) * ab[1]) / max(1e-9, ab[0] ** 2 + ab[1] ** 2)))
        return math.hypot(p[0] - a[0] - t * ab[0], p[1] - a[1] - t * ab[1])
    if len(hull) == 2:
        return seg_dist(point, hull[0], hull[1])
    inside = all((b[0] - a[0]) * (point[1] - a[1]) - (b[1] - a[1]) * (point[0] - a[0]) >= 0
                 for a, b in zip(hull, hull[1:] + hull[:1]))
    if inside:
        return 0.0
    return min(seg_dist(point, a, b) for a, b in zip(hull, hull[1:] + hull[:1]))


def _balance(frames, events, floor_z, fps=FPS):
    """Static balance where the pose holds (the centre of mass barely moves) or at a key frame: its
    ground projection must sit inside the support polygon."""
    out = []
    keys = set(events.values())
    for i in range(1, len(frames) - 1):
        com = centre_of_mass(frames[i])
        prev, nxt = centre_of_mass(frames[i - 1]), centre_of_mass(frames[i + 1])
        if com is None or prev is None or nxt is None:
            continue
        still = length(sub(nxt, prev)) * fps * 0.5 < 5.0
        if not still and i not in keys:
            continue
        hull = convex_hull(support_points(frames[i], floor_z))
        if not hull:
            continue
        outside = distance_outside((com[0], com[1]), hull)
        if outside > 0.0:
            out.append(dict(frame=i, kind="balance", severity="error" if outside > 8 else "warn", joint="com",
                            value=outside, text=f"centre of mass {outside:.1f} cm outside the support polygon"))
    return out


def summarize(result, keys=None, limit=40):
    """Report lines: counts by kind and severity, then the worst issue of each joint and kind."""
    issues = result["issues"]
    counts = {}
    for it in issues:
        counts[(it["kind"], it["severity"])] = counts.get((it["kind"], it["severity"]), 0) + 1
    lines = ["joint check: " + (", ".join(f"{k[0]} {k[1]} {v}" for k, v in sorted(counts.items())) or "clean")]
    groups = {}
    for it in issues:
        g = groups.setdefault((it["kind"], it["joint"]), {"first": it["frame"], "last": it["frame"], "worst": it, "n": 0})
        g["n"] += 1
        g["last"] = it["frame"]
        worse = (it["severity"] == "error" and g["worst"]["severity"] != "error") or (
            it["severity"] == g["worst"]["severity"] and abs(it["value"]) > abs(g["worst"]["value"]))
        if worse:
            g["worst"] = it
    order = sorted(groups.values(), key=lambda g: (g["worst"]["severity"] != "error", -abs(g["worst"]["value"])))
    for g in order[:limit]:
        w = g["worst"]
        key = f" [{keys[w['frame']]}]" if keys and w["frame"] in keys else ""
        lines.append(f"  {w['severity']:5s} {w['kind']:8s} frames {g['first']}-{g['last']} ({g['n']}): "
                     f"worst at {w['frame']}{key}: {w['text']}")
    if len(order) > limit:
        lines.append(f"  ... and {len(order) - limit} more")
    return lines


# ------------------------------------------------------------------------------- editor adapter (unreal)

def bones_needed():
    names = {"root", "pelvis", "head"} | set(CONTACT_BONES)
    for _, a, b, _, _ in SEGMENTS:
        names.update([a, b])
    for j in RIG_JOINTS:
        for side in (j.get("sides") or [""]):
            names.update(_side_bone(x, side) for x in (j["parent"], j["child"], *j["seg"]))
            if j["parent"].endswith("_metacarpal"):
                names.add(_side_bone("hand", side))
    for side in "lr":
        names.update(f"{b}_{side}" for b in ("upperarm", "lowerarm", "hand", "thigh", "calf", "foot", "ball",
                                             "middle_01", "index_01", "pinky_01"))
    names.update(["spine_05", "neck_01"])
    return sorted(names)


def _pose_from_transforms(transforms):
    out = {}
    for name, t in transforms.items():
        loc, q = t.translation, t.rotation
        out[name] = ((loc.x, loc.y, loc.z), (q.x, q.y, q.z, q.w))
    return out


def pose_at(anim, time, bones=None):
    """Component-space pose of a baked clip at ``time`` seconds (editor Python). Each bone's local pose
    is read once per call and composed down its path from the root."""
    import unreal
    lib = unreal.AnimationLibrary
    local, out = {}, {}
    for bone in bones or bones_needed():
        try:
            path = list(lib.find_bone_path_to_root(anim, bone))
        except Exception:
            continue
        if not path:
            continue
        t = unreal.Transform()
        for name in path:
            key = str(name)
            if key not in local:
                local[key] = lib.get_bone_pose_for_time(anim, name, time, False)
            t = unreal.MathLibrary.compose_transforms(t, local[key])
        out[bone] = t
    return _pose_from_transforms(out)


def neutral_pose(bones=None):
    """The heroine skeleton's reference pose in component space (editor Python): the zero for every
    angle measured relative to neutral."""
    import unreal
    from homestead_agent import rig_authoring as ra
    mesh = unreal.load_asset(ra.BODY)
    skeleton = mesh.get_editor_property("skeleton")
    pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    out = {}
    for bone in bones or bones_needed():
        try:
            out[bone] = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)
        except Exception:
            continue
    return _pose_from_transforms(out)


def report(anim, events=None, contacts=None, every=1, neutral=None, limit=40):
    """Joint check of a baked clip every ``every`` frames: report lines for a recipe's report().
    ``events``: the recipe's FRAMES dict (named in the report and balance-checked); ``contacts``: frames
    of strikes or impacts (named FRAMES keys or numbers)."""
    frames_n = int(round(anim.get_play_length() * FPS))
    indices = list(range(0, frames_n + 1, every))
    frames = [pose_at(anim, i / FPS) for i in indices]
    neutral = neutral or neutral_pose()
    events = {k: v // every for k, v in (events or {}).items()}
    contacts = [(events.get(c) if isinstance(c, str) else c // every) for c in (contacts or [])]
    result = check_frames(frames, neutral, fps=FPS / every, events=events,
                          contacts=[c for c in contacts if c is not None])
    keys = {}
    for name, i in events.items():
        keys.setdefault(i, name)
    return summarize(result, keys, limit)
