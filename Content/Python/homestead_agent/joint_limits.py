"""Anatomical joint-limit checker for the MetaHuman heroine's clips (realistic-animation skill).

    from homestead_agent import joint_limits as jl
    print(jl.report(anim))                     # in the editor: a baked AnimSequence, every frame
    issues = jl.check_frames(frames, bind)     # anywhere: poses as {bone: (location, quaternion)}

Every joint angle is measured from the bones' component-space transforms against the skeleton's own
bind (reference) pose. No assumption is made about Unreal's per-bone local axes:
- the child bone's rotation relative to its parent, minus the bind relation, is split into a swing
  (where the segment points) and a twist (roll about the segment);
- the swing is read along anatomical directions fixed in the parent at bind time: flexion toward the
  body's front, the palm, the back of the knee and so on, from the component frame (forward +Y,
  her left +X, up +Z) and the hand's own knuckle and palm directions;
- so a joint reads 0 in the bind pose, and its signs and names follow anatomy (flexion,
  extension, abduction, ulnar deviation, pronation...).

The limits come from clinical range-of-motion norms (AAOS, AMA Guides, the CDC normative ROM study
Soucie et al. 2011, Norkin & White, Kapandji/Neumann). The sources and per-joint citations are in
.github/skills/realistic-animation/SKILL.md. Each axis has a comfortable band (functional, everyday
work) and an extreme band (normal active end-range). Outside the extreme band is a hard error; between
the bands is a warning that the pose reads strained. MetaHuman's bind pose isn't anatomical zero
everywhere (the arms are lowered about 45 degrees and the fingers slightly curled), so BIND_OFFSET adds
each joint's bind angle back before the bands are applied.

The pure-Python core (vectors, quaternions, decomposition, checks) imports nothing from unreal, so
Tests/JointLimitsTests.py exercises it with synthetic poses. The editor adapter at the bottom
(pose_at, bind_pose, report) imports unreal lazily.
"""
import math

# ---------------------------------------------------------------------------------------------- constants

FPS = 30.0
FORWARD = (0.0, 1.0, 0.0)
LEFT = (1.0, 0.0, 0.0)
UP = (0.0, 0.0, 1.0)
# A bone this close to or below the floor plane (cm) counts as touching / penetrating it.
CONTACT_CM = 3.0
PENETRATION_CM = 1.5
# A planted contact moving faster than this along the floor slides (cm/s).
SLIDE_CM_PER_S = 8.0
# The centre of mass may sit this far outside the support polygon (cm) before a static frame is flagged.
COM_MARGIN_CM = 2.0


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


def twist_angle(q, axis):
    """Signed twist (degrees) of ``q`` about the unit ``axis`` (swing-twist decomposition)."""
    x, y, z, w = q
    p = dot((x, y, z), axis)
    if abs(p) < 1e-12 and abs(w) < 1e-12:
        return 180.0
    angle = 2.0 * math.degrees(math.atan2(p, w))
    return (angle + 180.0) % 360.0 - 180.0


# ------------------------------------------------------------------------------------------ joint table

def _band(comfortable, extreme):
    return {"comfortable": comfortable, "extreme": extreme}


# Each joint: the proximal (parent) and distal (child) bones; the bone whose position gives the distal
# segment's direction (None: the child's own direction from the parent, for end bones); and three
# anatomical axes:
#   flex  - swing toward `flex_toward` (positive) and away (negative: extension / hyperextension);
#   side  - swing toward `side_toward` (positive) and away;
#   twist - roll about the segment, positive toward `twist_positive` (named; sign mirrored on the left).
# Directions: 'forward', 'back', 'up', 'down', 'lateral' (away from her midline on that side),
# 'medial', 'palm' (out of the palm), 'thumb' (toward the thumb side), 'left'.
# Bands are degrees from anatomical zero; see the skill for the sources. 'provisional' marks values still
# to be confirmed against the research findings.
JOINTS = [
    # --- spine: per-segment shares of the regional ranges (thoracolumbar spread over five bones).
    dict(name="spine_01", parent="pelvis", child="spine_01", end="spine_02", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-10, 20), (-20, 30)), side=_band((-8, 8), (-12, 12)), twist=_band((-3, 3), (-6, 6)))),
    dict(name="spine_02", parent="spine_01", child="spine_02", end="spine_03", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-8, 15), (-15, 25)), side=_band((-8, 8), (-12, 12)), twist=_band((-5, 5), (-8, 8)))),
    dict(name="spine_03", parent="spine_02", child="spine_03", end="spine_04", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-6, 12), (-10, 18)), side=_band((-6, 6), (-10, 10)), twist=_band((-8, 8), (-12, 12)))),
    dict(name="spine_04", parent="spine_03", child="spine_04", end="spine_05", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-5, 10), (-8, 15)), side=_band((-6, 6), (-9, 9)), twist=_band((-8, 8), (-12, 12)))),
    dict(name="spine_05", parent="spine_04", child="spine_05", end="neck_01", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-5, 8), (-8, 12)), side=_band((-5, 5), (-8, 8)), twist=_band((-8, 8), (-12, 12)))),
    # --- neck and head: the cervical range split over neck_01, neck_02 and head (C1-C2 does most rotation).
    dict(name="neck_01", parent="spine_05", child="neck_01", end="neck_02", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-15, 15), (-25, 22)), side=_band((-10, 10), (-15, 15)), twist=_band((-10, 10), (-15, 15)))),
    dict(name="neck_02", parent="neck_01", child="neck_02", end="head", flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-15, 15), (-25, 20)), side=_band((-10, 10), (-15, 15)), twist=_band((-15, 15), (-25, 25)))),
    dict(name="head", parent="neck_02", child="head", end=None, flex_toward="forward",
         side_toward="left", axes=dict(
             flex=_band((-12, 12), (-20, 18)), side=_band((-8, 8), (-12, 12)), twist=_band((-25, 25), (-40, 40)))),
    # --- shoulder girdle: the clavicle elevates/depresses and protracts/retracts.
    dict(name="clavicle", sides="lr", parent="spine_05", child="clavicle", end="upperarm", flex_toward="up",
         side_toward="forward", axes=dict(
             flex=_band((-5, 20), (-10, 35)), side=_band((-15, 15), (-25, 25)), twist=_band((-20, 20), (-40, 40)))),
    # --- glenohumeral: the upper arm relative to the clavicle (scapula), so elevation the girdle takes
    # is not counted twice. Flexion forward, abduction outward, internal rotation positive.
    dict(name="shoulder", sides="lr", parent="clavicle", child="upperarm", end="lowerarm", flex_toward="forward",
         side_toward="lateral", twist_positive="internal", axes=dict(
             flex=_band((-30, 120), (-50, 160)), side=_band((-20, 120), (-30, 150)), twist=_band((-60, 60), (-90, 70)))),
    # --- elbow: a hinge; side and twist should stay near zero (the carrying angle is in the bind pose).
    dict(name="elbow", sides="lr", parent="upperarm", child="lowerarm", end="hand", flex_toward="forward",
         side_toward="lateral", axes=dict(
             flex=_band((0, 140), (-8, 150)), side=_band((-5, 5), (-10, 10)), twist=_band((-10, 10), (-20, 20)))),
    # --- wrist: hand relative to the forearm. Twist here is forearm roll (pronation positive): it
    # belongs to the radioulnar joints, and on the rig to the forearm twist bones.
    dict(name="wrist", sides="lr", parent="lowerarm", child="hand", end="middle_01", flex_toward="palm",
         side_toward="thumb", twist_positive="pronation", axes=dict(
             flex=_band((-40, 40), (-70, 75)), side=_band((-25, 10), (-35, 20)), twist=_band((-60, 60), (-85, 75)))),
    # --- fingers: MCP (proximal phalanx on the hand), PIP and DIP. Flexion into the palm; small
    # hyperextension only. Abduction (side) at the MCP only.
    *[dict(name=f"{f}_mcp", sides="lr", parent="hand", child=f"{f}_01", end=f"{f}_02", flex_toward="palm",
           side_toward="thumb", axes=dict(
               flex=_band((-10, 80), (-30, 95)), side=_band((-15, 15), (-25, 25)), twist=_band((-10, 10), (-20, 20))))
      for f in ("index", "middle", "ring", "pinky")],
    *[dict(name=f"{f}_pip", sides="lr", parent=f"{f}_01", child=f"{f}_02", end=f"{f}_03", flex_toward="palm",
           side_toward="thumb", axes=dict(
               flex=_band((0, 95), (-5, 110)), side=_band((-5, 5), (-10, 10)), twist=_band((-5, 5), (-10, 10))))
      for f in ("index", "middle", "ring", "pinky")],
    *[dict(name=f"{f}_dip", sides="lr", parent=f"{f}_02", child=f"{f}_03", end=None, flex_toward="palm",
           side_toward="thumb", axes=dict(
               flex=_band((0, 70), (-15, 90)), side=_band((-5, 5), (-10, 10)), twist=_band((-5, 5), (-10, 10))))
      for f in ("index", "middle", "ring", "pinky")],
    dict(name="thumb_mcp", sides="lr", parent="thumb_01", child="thumb_02", end="thumb_03", flex_toward="palm",
         side_toward="thumb", axes=dict(
             flex=_band((-5, 45), (-10, 60)), side=_band((-10, 10), (-15, 15)), twist=_band((-10, 10), (-20, 20)))),
    dict(name="thumb_ip", sides="lr", parent="thumb_02", child="thumb_03", end=None, flex_toward="palm",
         side_toward="thumb", axes=dict(
             flex=_band((-10, 70), (-25, 80)), side=_band((-5, 5), (-10, 10)), twist=_band((-5, 5), (-10, 10)))),
    # --- hip: thigh relative to the pelvis. Flexion forward, abduction outward, internal rotation positive.
    dict(name="hip", sides="lr", parent="pelvis", child="thigh", end="calf", flex_toward="forward",
         side_toward="lateral", twist_positive="internal", axes=dict(
             flex=_band((-15, 110), (-25, 125)), side=_band((-15, 35), (-25, 45)), twist=_band((-35, 30), (-45, 40)))),
    # --- knee: a hinge with flexion-coupled tibial rotation; flexion moves the shin back.
    dict(name="knee", sides="lr", parent="thigh", child="calf", end="foot", flex_toward="back",
         side_toward="lateral", twist_positive="internal", axes=dict(
             flex=_band((0, 135), (-5, 155)), side=_band((-5, 5), (-8, 8)), twist=_band((-15, 15), (-30, 30)))),
    # --- ankle: dorsiflexion lifts the foot (positive); the twist about the foot's long axis is the
    # subtalar inversion (positive) / eversion.
    dict(name="ankle", sides="lr", parent="calf", child="foot", end="ball", flex_toward="up",
         side_toward="lateral", twist_positive="inversion", axes=dict(
             flex=_band((-35, 15), (-50, 25)), side=_band((-10, 10), (-20, 20)), twist=_band((-10, 20), (-15, 30)))),
    # --- toes (the MTP joints as one ball bone): extension lifts the toes (positive here).
    dict(name="toes", sides="lr", parent="foot", child="ball", end=None, flex_toward="up",
         side_toward="lateral", axes=dict(
             flex=_band((-20, 60), (-35, 90)), side=_band((-10, 10), (-15, 15)), twist=_band((-10, 10), (-15, 15)))),
]

# Names of the positive and negative directions of each axis, for readable reports.
AXIS_NAMES = {
    "spine": {"flex": ("flexion", "extension"), "side": ("left bend", "right bend"), "twist": ("twist", "twist")},
    "clavicle": {"flex": ("elevation", "depression"), "side": ("protraction", "retraction"), "twist": ("roll", "roll")},
    "shoulder": {"flex": ("flexion", "extension"), "side": ("abduction", "adduction"),
                 "twist": ("internal rotation", "external rotation")},
    "elbow": {"flex": ("flexion", "hyperextension"), "side": ("valgus", "varus"), "twist": ("twist", "twist")},
    "wrist": {"flex": ("flexion", "extension"), "side": ("radial deviation", "ulnar deviation"),
              "twist": ("pronation", "supination")},
    "finger": {"flex": ("flexion", "hyperextension"), "side": ("abduction", "adduction"), "twist": ("roll", "roll")},
    "hip": {"flex": ("flexion", "extension"), "side": ("abduction", "adduction"),
            "twist": ("internal rotation", "external rotation")},
    "knee": {"flex": ("flexion", "hyperextension"), "side": ("varus", "valgus"),
             "twist": ("internal rotation", "external rotation")},
    "ankle": {"flex": ("dorsiflexion", "plantarflexion"), "side": ("abduction", "adduction"),
              "twist": ("inversion", "eversion")},
    "toes": {"flex": ("extension", "flexion"), "side": ("abduction", "adduction"), "twist": ("roll", "roll")},
}

# Angular-speed ceilings (deg/s) per joint family for hand-keyed work clips. Above `warn` a frame-to-frame
# change reads as a snap unless it's the strike of a swing; above `error` it is a pop at any time.
SPEED = {
    "spine": (250, 500), "neck": (300, 600), "head": (350, 700), "clavicle": (300, 600),
    "shoulder": (600, 1200), "elbow": (700, 1400), "wrist": (700, 1400), "finger": (900, 1800),
    "hip": (500, 1000), "knee": (700, 1400), "ankle": (600, 1200), "toes": (600, 1200),
}

# Per-joint bind-pose angles (degrees, anatomical zero = 0) added to the measured delta. MetaHuman binds
# with the arms lowered ~45 deg from horizontal, i.e. shoulder abduction ~45 deg from the trunk's
# side... measured in-engine with bind_offsets() and pasted here. Missing joints read 0.
BIND_OFFSET = {}


def family(name):
    if name.startswith("spine"):
        return "spine"
    if name in ("neck_01", "neck_02"):
        return "neck"
    if name.endswith(("_mcp", "_pip", "_dip", "_ip")):
        return "finger"
    return name


def axis_names(name):
    f = family(name)
    if f in ("neck", "head"):
        f = "spine"
    return AXIS_NAMES.get(f, AXIS_NAMES["spine"])


def joint_list():
    """Every joint instance: (instance name, side or '', joint dict)."""
    out = []
    for j in JOINTS:
        for side in (j.get("sides") or ""):
            out.append((f"{j['name']}_{side}", side, j))
        if not j.get("sides"):
            out.append((j["name"], "", j))
    return out


def _bone(name, side):
    return f"{name}_{side}" if side else name


# -------------------------------------------------------------------------------------- the measurement

def _direction(word, side, hand_frame):
    lat = (-1.0, 0.0, 0.0) if side == "r" else (1.0, 0.0, 0.0)
    if word == "forward":
        return FORWARD
    if word == "back":
        return scale(FORWARD, -1.0)
    if word == "up":
        return UP
    if word == "down":
        return scale(UP, -1.0)
    if word == "left":
        return LEFT
    if word == "lateral":
        return lat
    if word == "medial":
        return scale(lat, -1.0)
    if word == "palm":
        return hand_frame["palm"]
    if word == "thumb":
        return hand_frame["thumb"]
    raise ValueError(word)


def hand_frame(pose, side):
    """The hand's knuckle direction, thumb-side direction and palm normal (out of the palm, into a grip)
    in component space, from bone positions (works on any pose)."""
    hand = pose[f"hand_{side}"][0]
    along = norm(sub(pose[f"middle_01_{side}"][0], hand))
    across = sub(pose[f"index_01_{side}"][0], pose[f"pinky_01_{side}"][0])
    thumb = norm(reject(across, along))
    palm = norm(cross(along, thumb) if side == "r" else cross(thumb, along))
    return {"along": along, "thumb": thumb, "palm": palm}


def _segment(pose, joint, side):
    child = _bone(joint["child"], side)
    if joint.get("end"):
        end = _bone(joint["end"], side) if joint["end"] not in ("head", "neck_01", "neck_02", "spine_02",
                                                               "spine_03", "spine_04", "spine_05") else joint["end"]
        if end in pose:
            return norm(sub(pose[end][0], pose[child][0]))
    parent = _bone(joint["parent"], side) if joint["parent"] not in ("pelvis", "spine_01", "spine_02", "spine_03",
                                                                      "spine_04", "spine_05", "neck_01", "neck_02") \
        else joint["parent"]
    return norm(sub(pose[child][0], pose[parent][0]))


def _bones(joint, side):
    def resolve(b):
        return b if b in ("pelvis", "head") or b.startswith(("spine_", "neck_")) else _bone(b, side)
    return resolve(joint["parent"]), resolve(joint["child"])


class Rig:
    """Bind-pose geometry for every joint, computed once: the distal segment direction and the
    anatomical flex and side directions, all expressed in the parent bone's local frame."""

    def __init__(self, bind):
        self.bind = bind
        self.joints = {}
        frames = {s: hand_frame(bind, s) for s in "lr" if f"hand_{s}" in bind and f"index_01_{s}" in bind}
        for name, side, j in joint_list():
            parent, child = _bones(j, side)
            if parent not in bind or child not in bind:
                continue
            hf = frames.get(side) or {"palm": FORWARD, "thumb": FORWARD, "along": FORWARD}
            d = _segment(bind, j, side)
            flex = norm(reject(_direction(j["flex_toward"], side, hf), d))
            side_dir = reject(_direction(j["side_toward"], side, hf), d)
            side_dir = norm(reject(side_dir, flex))
            if length(side_dir) < 0.5:
                side_dir = norm(cross(d, flex))
            qp = bind[parent][1]
            to_local = q_inv(qp)
            self.joints[name] = dict(
                joint=j, side=side, parent=parent, child=child,
                rel0=q_mul(q_inv(qp), bind[child][1]),
                d=q_rotate(to_local, d), e_flex=q_rotate(to_local, flex), e_side=q_rotate(to_local, side_dir),
                mirror=-1.0 if side == "l" else 1.0)

    def angles(self, pose, name):
        """(flex, side, twist) degrees for one joint instance in ``pose``, or None if bones are missing."""
        g = self.joints.get(name)
        if g is None or g["parent"] not in pose or g["child"] not in pose:
            return None
        rel = q_mul(q_inv(pose[g["parent"]][1]), pose[g["child"]][1])
        delta = q_norm(q_mul(rel, q_inv(g["rel0"])))
        d = q_rotate(delta, g["d"])
        flex = math.degrees(math.atan2(dot(d, g["e_flex"]), dot(d, g["d"])))
        side = math.degrees(math.atan2(dot(d, g["e_side"]), math.hypot(dot(d, g["d"]), dot(d, g["e_flex"]))))
        twist = twist_angle(delta, g["d"]) * g["mirror"]
        off = BIND_OFFSET.get(name.rsplit("_", 1)[0] if g["side"] else name, (0.0, 0.0, 0.0))
        return (flex + off[0], side + off[1], twist + off[2])


# ------------------------------------------------------------------------------------------- the checks

def classify(value, band):
    lo, hi = band["comfortable"]
    elo, ehi = band["extreme"]
    if elo <= value <= ehi:
        return "ok" if lo <= value <= hi else "strained"
    return "beyond"


def check_pose(rig, pose):
    """Joint-angle status for one pose: {joint: {axis: (degrees, status)}}."""
    out = {}
    for name, side, j in joint_list():
        a = rig.angles(pose, name)
        if a is None:
            continue
        out[name] = {axis: (value, classify(value, j["axes"][axis])) for axis, value in zip(("flex", "side", "twist"), a)}
    return out


def describe(name, axis, value):
    pos, neg = axis_names(name.rsplit("_", 1)[0] if name.endswith(("_l", "_r")) else name)[axis]
    return f"{pos if value >= 0 else neg} {abs(value):.0f}"


def check_frames(frames, bind, fps=FPS, events=None, floor_z=0.0):
    """Check a clip: ``frames`` is a list of poses ({bone: (location, quaternion)}), component space.
    Returns {'issues': [...], 'angles': [per-frame check_pose]} where each issue is a dict with frame,
    kind ('rom', 'speed', 'ground', 'slide', 'balance'), severity ('warn' or 'error'), joint, text."""
    rig = Rig(bind)
    issues, history = [], []
    events = events or {}
    for i, pose in enumerate(frames):
        state = check_pose(rig, pose)
        history.append(state)
        for name, axes in state.items():
            j = rig.joints[name]["joint"]
            for axis, (value, status) in axes.items():
                if status == "ok":
                    continue
                band = j["axes"][axis]
                issues.append(dict(frame=i, kind="rom", severity="error" if status == "beyond" else "warn",
                                   joint=name, axis=axis, value=value,
                                   text=f"{name} {describe(name, axis, value)} deg "
                                        f"(comfortable {band['comfortable']}, extreme {band['extreme']})"))
        if i:
            prev = history[i - 1]
            for name, axes in state.items():
                if name not in prev:
                    continue
                warn, error = SPEED.get(family(name), (700, 1400))
                for axis in axes:
                    speed = abs(axes[axis][0] - prev[name][axis][0]) * fps
                    if axis == "twist" and abs(axes[axis][0] - prev[name][axis][0]) > 180:
                        continue
                    if speed > warn:
                        issues.append(dict(frame=i, kind="speed", severity="error" if speed > error else "warn",
                                           joint=name, axis=axis, value=speed,
                                           text=f"{name} {axis} {speed:.0f} deg/s (snap above {warn}, pop above {error})"))
        issues.extend(_ground(pose, i, floor_z))
        if i:
            issues.extend(_slides(frames[i - 1], pose, i, fps, floor_z))
    issues.extend(_balance(frames, events, floor_z))
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
    for bone in ("ball_l", "ball_r", "foot_l", "foot_r"):
        if bone not in pose or bone not in prev:
            continue
        a, b = prev[bone][0], pose[bone][0]
        planted = a[2] - CONTACT_BONES[bone] - floor_z < CONTACT_CM and b[2] - CONTACT_BONES[bone] - floor_z < CONTACT_CM
        speed = math.hypot(b[0] - a[0], b[1] - a[1]) * fps
        if planted and speed > SLIDE_CM_PER_S:
            out.append(dict(frame=i, kind="slide", severity="error" if speed > 3 * SLIDE_CM_PER_S else "warn",
                            joint=bone, value=speed, text=f"{bone} slides {speed:.0f} cm/s while planted"))
    return out


# Segment mass fractions for women (de Leva 1996, after Zatsiorsky-Seluyanov) and where each segment's
# centre of mass sits along it (fraction from the proximal end). Provisional until checked against findings.
SEGMENTS = [
    ("head", "neck_02", "head", 0.0668, 0.5),
    ("trunk", "pelvis", "neck_01", 0.4257, 0.5),
    ("upperarm_l", "upperarm_l", "lowerarm_l", 0.0255, 0.575), ("upperarm_r", "upperarm_r", "lowerarm_r", 0.0255, 0.575),
    ("forearm_l", "lowerarm_l", "hand_l", 0.0138, 0.456), ("forearm_r", "lowerarm_r", "hand_r", 0.0138, 0.456),
    ("hand_l", "hand_l", "middle_01_l", 0.0056, 0.75), ("hand_r", "hand_r", "middle_01_r", 0.0056, 0.75),
    ("thigh_l", "thigh_l", "calf_l", 0.1478, 0.39), ("thigh_r", "thigh_r", "calf_r", 0.1478, 0.39),
    ("shank_l", "calf_l", "foot_l", 0.0481, 0.44), ("shank_r", "calf_r", "foot_r", 0.0481, 0.44),
    ("foot_l", "foot_l", "ball_l", 0.0129, 0.5), ("foot_r", "foot_r", "ball_r", 0.0129, 0.5),
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
    pts = []
    for bone, skin in CONTACT_BONES.items():
        if bone in pose and pose[bone][0][2] - skin - floor_z < CONTACT_CM:
            pts.append((pose[bone][0][0], pose[bone][0][1]))
    return pts


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
    """How far (cm) ``point`` lies outside the convex ``hull`` polygon; 0 inside. Degenerate hulls
    (one or two contacts) measure to the point or segment."""
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


def _balance(frames, events, floor_z):
    """Static balance at held frames: where the pose barely moves (a hold or an authored key), the
    centre of mass must project inside the support polygon."""
    out = []
    for i in range(1, len(frames) - 1):
        com = centre_of_mass(frames[i])
        prev, nxt = centre_of_mass(frames[i - 1]), centre_of_mass(frames[i + 1])
        if com is None or prev is None or nxt is None:
            continue
        still = length(sub(nxt, prev)) * FPS * 0.5 < 5.0
        if not still and i not in events.values():
            continue
        hull = convex_hull(support_points(frames[i], floor_z))
        outside = distance_outside((com[0], com[1]), hull)
        if outside > COM_MARGIN_CM:
            out.append(dict(frame=i, kind="balance", severity="error" if outside > 8 else "warn", joint="com",
                            value=outside, text=f"centre of mass {outside:.1f} cm outside the support polygon"))
    return out


def summarize(result, keys=None, limit=40):
    """Report lines: counts by kind and severity, then the worst issues grouped by joint and axis."""
    issues = result["issues"]
    lines = []
    counts = {}
    for it in issues:
        counts[(it["kind"], it["severity"])] = counts.get((it["kind"], it["severity"]), 0) + 1
    lines.append("joint check: " + (", ".join(f"{k[0]} {k[1]} {v}" for k, v in sorted(counts.items())) or "clean"))
    groups = {}
    for it in issues:
        key = (it["kind"], it["joint"], it.get("axis", ""))
        g = groups.setdefault(key, {"first": it["frame"], "last": it["frame"], "worst": it, "n": 0})
        g["n"] += 1
        g["last"] = it["frame"]
        if abs(it["value"]) > abs(g["worst"]["value"]) or it["severity"] == "error" and g["worst"]["severity"] != "error":
            g["worst"] = it
    order = sorted(groups.items(), key=lambda kv: (kv[1]["worst"]["severity"] != "error", -abs(kv[1]["worst"]["value"])))
    for (kind, joint, axis), g in order[:limit]:
        w = g["worst"]
        key = f" [{keys[w['frame']]}]" if keys and w["frame"] in keys else ""
        lines.append(f"  {w['severity']:5s} frames {g['first']}-{g['last']} ({g['n']}): worst at {w['frame']}{key}: {w['text']}")
    return lines


# ------------------------------------------------------------------------------- editor adapter (unreal)

def _pose_from_transforms(transforms):
    out = {}
    for name, t in transforms.items():
        loc, q = t.translation, t.rotation
        out[name] = ((loc.x, loc.y, loc.z), (q.x, q.y, q.z, q.w))
    return out


def bones_needed():
    names = set(["pelvis", "head"] + [b for _, a, b, _, _ in SEGMENTS] + [a for _, a, _, _, _ in SEGMENTS])
    for name, side, j in joint_list():
        p, c = _bones(j, side)
        names.update([p, c])
        if j.get("end"):
            e = j["end"]
            names.add(e if e in ("head",) or e.startswith(("spine_", "neck_")) else _bone(e, side))
    for side in "lr":
        names.update([f"hand_{side}", f"middle_01_{side}", f"index_01_{side}", f"pinky_01_{side}"])
    names.update(CONTACT_BONES)
    return sorted(names)


def pose_at(anim, time):
    """Component-space pose of a baked clip at ``time`` seconds (editor Python)."""
    from homestead_agent import rig_authoring as ra
    return _pose_from_transforms(ra.bone_positions(anim, bones_needed(), time))


def bind_pose():
    """The heroine skeleton's reference pose in component space (editor Python)."""
    import unreal
    from homestead_agent import rig_authoring as ra
    skeleton = unreal.load_asset(ra.BODY).skeleton
    out = {}
    for name in bones_needed():
        try:
            t = unreal.AnimationLibrary.get_bone_pose_for_frame  # noqa: F841 (presence check only)
        except AttributeError:
            pass
        t = _ref_component(skeleton, name)
        if t is not None:
            out[name] = t
    return out


def _ref_component(skeleton, bone):
    import unreal
    # Accumulate the reference local transforms up to the root.
    chain = []
    name = bone
    ref = unreal.AnimationLibrary
    while name and str(name) != "None":
        try:
            local = skeleton.get_reference_pose().get_ref_bone_pose(name, unreal.AnimPoseSpaces.LOCAL)
        except Exception:
            return None
        chain.append(local)
        name = skeleton.get_reference_pose().get_parent_bone_name(name) if hasattr(
            skeleton.get_reference_pose(), "get_parent_bone_name") else None
    t = unreal.Transform()
    for local in reversed(chain):
        t = local * t
    loc, q = t.translation, t.rotation
    return ((loc.x, loc.y, loc.z), (q.x, q.y, q.z, q.w))


def report(anim, events=None, every=1, bind=None):
    """Joint check of a baked clip, every ``every`` frames: report lines for a recipe's report()."""
    frames_n = int(round(anim.get_play_length() * FPS))
    times = [i / FPS for i in range(0, frames_n + 1, every)]
    frames = [pose_at(anim, t) for t in times]
    bind = bind or bind_pose()
    result = check_frames(frames, bind, fps=FPS / every)
    keys = {i // every: name for name, i in (events or {}).items()}
    return summarize(result, keys)
