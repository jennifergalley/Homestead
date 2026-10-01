"""Synthetic-pose tests for the anatomical joint-limit checker (Content/Python/homestead_agent/joint_limits.py).

    python -m unittest Tests/JointLimitsTests.py

A small forward-kinematics skeleton with MetaHuman bone names, in rig_authoring's component frame (forward
+Y, her left +X, up +Z), standing in anatomical neutral (arms at her sides, palms in, thumbs forward) with
every bone's rotation the identity. Each test bends joints by known amounts and checks what is read.
"""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Content" / "Python"))
from homestead_agent import joint_limits as jl  # noqa: E402

# bone -> (parent, offset from the parent in its frame, cm). Right-side bones mirror X.
_LEFT = [
    ("clavicle", "spine_05", (3.0, 2.0, 5.0)), ("upperarm", "clavicle", (15.0, -2.0, 0.0)),
    ("lowerarm", "upperarm", (0.0, 0.0, -28.0)), ("hand", "lowerarm", (0.0, 0.0, -25.0)),
    ("thumb_01", "hand", (0.0, 2.5, -2.0)), ("thumb_02", "thumb_01", (0.0, 1.5, -3.0)), ("thumb_03", "thumb_02", (0.0, 0.5, -3.0)),
    ("thigh", "pelvis", (9.0, 0.0, -5.0)), ("calf", "thigh", (0.0, 0.0, -42.0)), ("foot", "calf", (0.0, 0.0, -42.0)),
    ("ball", "foot", (0.0, 14.0, -4.0)), ("bigtoe_01", "ball", (0.0, 3.0, 0.0)),
]
for _f, _y in (("index", 2.0), ("middle", 0.7), ("ring", -0.7), ("pinky", -2.0)):
    _LEFT += [(f"{_f}_metacarpal", "hand", (0.0, _y, -2.0)), (f"{_f}_01", f"{_f}_metacarpal", (0.0, 0.0, -7.0)),
              (f"{_f}_02", f"{_f}_01", (0.0, 0.0, -4.0)), (f"{_f}_03", f"{_f}_02", (0.0, 0.0, -2.5))]
SKELETON = [("root", None, (0.0, 0.0, 0.0)), ("pelvis", "root", (0.0, 0.0, 95.0)),
            ("spine_01", "pelvis", (0.0, 0.0, 5.0)), ("spine_02", "spine_01", (0.0, 0.0, 10.0)),
            ("spine_03", "spine_02", (0.0, 0.0, 10.0)), ("spine_04", "spine_03", (0.0, 0.0, 10.0)),
            ("spine_05", "spine_04", (0.0, 0.0, 10.0)), ("neck_01", "spine_05", (0.0, 0.0, 8.0)),
            ("neck_02", "neck_01", (0.0, 0.0, 5.0)), ("head", "neck_02", (0.0, 0.0, 6.0))]
for _side, _sx in (("l", 1.0), ("r", -1.0)):
    for _name, _parent, (_x, _y, _z) in _LEFT:
        _p = _parent if _parent in ("spine_05", "pelvis") else f"{_parent}_{_side}"
        SKELETON.append((f"{_name}_{_side}", _p, (_x * _sx, _y, _z)))

X, Y, Z = (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)
NX, NY = (-1.0, 0.0, 0.0), (0.0, -1.0, 0.0)


def pose(rotations=None, shift=None):
    """Forward kinematics: rotations {bone: [(axis, degrees), ...]} in the bone's local (neutral-aligned)
    frame, applied in order; shift {bone: (dx, dy, dz)} moves a bone and everything below it."""
    rotations, shift = rotations or {}, shift or {}
    out = {}
    for bone, parent, offset in SKELETON:
        local = jl.IDENTITY
        for axis, degrees in rotations.get(bone, []):
            local = jl.q_mul(local, jl.q_axis_angle(axis, degrees))
        if parent is None:
            loc, q = offset, local
        else:
            ploc, pq = out[parent]
            loc, q = jl.add(ploc, jl.q_rotate(pq, offset)), jl.q_mul(pq, local)
        loc = jl.add(loc, shift.get(bone, (0.0, 0.0, 0.0)))
        out[bone] = (loc, q)
    return out


NEUTRAL = pose()


def measured(rotations=None):
    return jl.measure(jl.Calibration(NEUTRAL), pose(rotations))


def issues(frames, **kw):
    return jl.check_frames(frames, NEUTRAL, **kw)["issues"]


def keys(found, kind=None, severity=None):
    return {i["joint"] for i in found if (kind is None or i["kind"] == kind) and (severity is None or i["severity"] == severity)}


def palm_axis(side):
    """The axis that flexes a finger or the hand toward the palm (palms face her midline)."""
    return Y if side == "l" else (0.0, -1.0, 0.0)


class NeutralPose(unittest.TestCase):
    def test_neutral_reads_zero_and_is_clean(self):
        values = measured()
        for key, value in values.items():
            if key.endswith(".elevation"):
                continue
            self.assertAlmostEqual(value, 0.0, delta=0.5, msg=key)
        self.assertIn("elbow_l.flex", values)
        self.assertIn("wrist_r.flex", values)
        self.assertIn("pinky_dip_l.flex", values)
        self.assertIn("lumbar.twist", values)
        found = issues([NEUTRAL] * 5)
        self.assertEqual([i for i in found if i["kind"] in ("rom", "coupling", "speed", "ground", "slide", "balance")], [])


class Arms(unittest.TestCase):
    def test_elbow_flexion_and_hyperextension(self):
        for side in "lr":
            self.assertAlmostEqual(measured({f"lowerarm_{side}": [(X, 90)]})[f"elbow_{side}.flex"], 90, delta=0.5)
            found = issues([pose({f"lowerarm_{side}": [(X, -15)]})])
            self.assertIn(f"elbow_{side}.flex", keys(found, "rom", "error"))
        self.assertEqual(keys(issues([pose({"lowerarm_l": [(X, 120)]})]), "rom"), set())

    def test_shoulder_flexion_abduction_and_rhythm(self):
        self.assertAlmostEqual(measured({"upperarm_l": [(X, 90)]})["shoulder_l.flex"], 90, delta=0.5)
        self.assertAlmostEqual(measured({"upperarm_l": [(NY, 90)]})["shoulder_l.abd"], 90, delta=0.5)
        self.assertAlmostEqual(measured({"upperarm_r": [(Y, 90)]})["shoulder_r.abd"], 90, delta=0.5)
        self.assertNotIn("shoulder_l.flex", measured({"upperarm_l": [(NY, 90)]}))
        # Overhead on a still clavicle: a rhythm warning; with the clavicle elevated it reads right.
        still = issues([pose({"upperarm_l": [(X, 170)]})])
        self.assertIn("shoulder_l.rhythm", keys(still, "coupling"))
        self.assertIn("shoulder_l.flex", keys(still, "rom", "warn"))
        lifted = issues([pose({"clavicle_l": [(NY, 25)], "upperarm_l": [(X, 150)]})])
        self.assertNotIn("shoulder_l.rhythm", keys(lifted, "coupling"))
        self.assertAlmostEqual(measured({"clavicle_l": [(NY, 25)]})["clavicle_l.flex"], 25, delta=0.5)

    def test_internal_rotation(self):
        self.assertAlmostEqual(measured({"upperarm_l": [(Z, 40)]})["shoulder_l.twist"], 40, delta=0.5)
        self.assertAlmostEqual(measured({"upperarm_r": [(Z, -40)]})["shoulder_r.twist"], 40, delta=0.5)

    def test_forearm_pronation_on_either_bone(self):
        # Palm-in to palm-back is pronation on both sides, keyed on the hand or on the forearm.
        self.assertAlmostEqual(measured({"hand_l": [(Z, 70)]})["forearm_l.twist"], 70, delta=0.5)
        self.assertAlmostEqual(measured({"hand_r": [(Z, -70)]})["forearm_r.twist"], 70, delta=0.5)
        self.assertAlmostEqual(measured({"lowerarm_l": [(Z, 70)]})["forearm_l.twist"], 70, delta=0.5)
        self.assertAlmostEqual(measured({"hand_l": [(Z, -60)]})["forearm_l.twist"], -60, delta=0.5)
        # Elbow flexion doesn't read as roll.
        self.assertAlmostEqual(measured({"lowerarm_l": [(X, 90)], "hand_l": [(Z, 30)]})["forearm_l.twist"], 30, delta=1.0)
        found = issues([pose({"hand_l": [(Z, 100)]})])
        self.assertIn("forearm_l.twist", keys(found, "rom", "error"))


class Hands(unittest.TestCase):
    def test_wrist_flexion_extension_deviation(self):
        for side in "lr":
            v = measured({f"hand_{side}": [(palm_axis(side), 30)]})
            self.assertAlmostEqual(v[f"wrist_{side}.flex"], 30, delta=0.5)
            self.assertAlmostEqual(v[f"forearm_{side}.twist"], 0, delta=0.5)
            self.assertAlmostEqual(measured({f"hand_{side}": [(palm_axis(side), -30)]})[f"wrist_{side}.flex"], -30, delta=0.5)
            # Toward the thumb (forward) is radial deviation.
            self.assertAlmostEqual(measured({f"hand_{side}": [(X, 12)]})[f"wrist_{side}.side"], 12, delta=0.5)
        self.assertIn("wrist_l.flex", keys(issues([pose({"hand_l": [(Y, 60)]})]), "rom", "warn"))
        self.assertIn("wrist_l.flex", keys(issues([pose({"hand_l": [(Y, 95)]})]), "rom", "error"))
        self.assertIn("wrist_l.flex", keys(issues([pose({"hand_l": [(Y, -95)]})]), "rom", "error"))

    def test_fingers_curl_and_bend_backward(self):
        a = palm_axis("l")
        curl = {}
        for f in jl.FINGERS:
            curl[f"{f}_01_l"] = [(a, 60)]
            curl[f"{f}_02_l"] = [(a, 80)]
            curl[f"{f}_03_l"] = [(a, 50)]
        v = measured(curl)
        self.assertAlmostEqual(v["index_mcp_l.flex"], 60, delta=0.5)
        self.assertAlmostEqual(v["index_pip_l.flex"], 80, delta=0.5)
        self.assertAlmostEqual(v["index_dip_l.flex"], 50, delta=0.5)
        self.assertEqual(keys(issues([pose(curl)]), "rom"), set())
        back = issues([pose({"middle_02_l": [(a, -20)]})])
        self.assertIn("middle_pip_l.flex", keys(back, "rom", "error"))

    def test_hand_couplings(self):
        a = palm_axis("l")
        isolated = issues([pose({"ring_03_l": [(a, 45)]})])
        self.assertIn("ring_dip_l.coupling", keys(isolated, "coupling"))
        fist = {f"{f}_02_l": [(a, 85)] for f in jl.FINGERS}
        fist.update({f"{f}_01_l": [(a, 70)] for f in jl.FINGERS})
        fist.update({f"{f}_03_l": [(a, 55)] for f in jl.FINGERS})
        self.assertNotIn("wrist_l.tenodesis", keys(issues([pose(fist)]), "coupling"))
        fist["hand_l"] = [(a, 30)]
        self.assertIn("wrist_l.tenodesis", keys(issues([pose(fist)]), "coupling"))


class SpineAndNeck(unittest.TestCase):
    def test_lumbar_twist_is_tiny(self):
        v = measured({"spine_01": [(Z, 4)], "spine_02": [(Z, 4)]})
        self.assertAlmostEqual(v["lumbar.twist"], 8, delta=0.5)
        found = issues([pose({"spine_01": [(Z, 4)], "spine_02": [(Z, 4)]})])
        self.assertIn("lumbar.twist", keys(found, "rom", "warn"))
        # The same turn from the thorax is fine.
        self.assertEqual(keys(issues([pose({"spine_03": [(Z, 6)], "spine_04": [(Z, 8)], "spine_05": [(Z, 8)]})]), "rom"), set())

    def test_one_bone_hinge_and_distributed_bend(self):
        self.assertIn("spine_03.flex", keys(issues([pose({"spine_03": [(NX, 30)]})]), "rom", "error"))
        # The skill's split of a 45-degree bend: lumbar-heavy, tapering up the thorax.
        bend = {b: [(NX, d)] for b, d in (("spine_01", 10), ("spine_02", 12), ("spine_03", 10), ("spine_04", 8), ("spine_05", 5))}
        v = measured(bend)
        self.assertAlmostEqual(v["spine.flex"], 45, delta=1.0)
        self.assertEqual(keys(issues([pose(bend)]), "rom"), set())

    def test_neck_turn(self):
        turn = {"neck_01": [(Z, 15)], "neck_02": [(Z, 30)], "head": [(Z, 15)]}
        self.assertAlmostEqual(measured(turn)["neck.twist"], 60, delta=1.0)
        self.assertEqual(keys(issues([pose(turn)]), "rom"), set())
        self.assertIn("neck_02.twist", keys(issues([pose({"neck_02": [(Z, 50)]})]), "rom", "error"))


class Legs(unittest.TestCase):
    def test_knee_and_hip(self):
        for side in "lr":
            self.assertAlmostEqual(measured({f"calf_{side}": [(NX, 90)]})[f"knee_{side}.flex"], 90, delta=0.5)
            self.assertAlmostEqual(measured({f"thigh_{side}": [(X, 100)]})[f"hip_{side}.flex"], 100, delta=0.5)
            self.assertIn(f"knee_{side}.flex", keys(issues([pose({f"calf_{side}": [(X, 12)]})]), "rom", "error"))
        self.assertAlmostEqual(measured({"thigh_l": [(NY, 30)]})["hip_l.abd"], 30, delta=0.5)
        self.assertAlmostEqual(measured({"thigh_l": [(Z, 25)]})["hip_l.twist"], 25, delta=0.5)
        # Hip flexed past 90 with the knee straight is hamstring-limited; with the knee bent it's fine.
        straight = issues([pose({"thigh_l": [(X, 100)]})])
        self.assertIn("hip_l.hamstring", keys(straight, "coupling"))
        bent = issues([pose({"thigh_l": [(X, 100)], "calf_l": [(NX, 100)]})])
        self.assertNotIn("hip_l.hamstring", keys(bent, "coupling"))

    def test_tibial_rotation_locks_near_extension(self):
        self.assertIn("knee_l.screwhome", keys(issues([pose({"calf_l": [(Z, 14)]})]), "coupling"))
        self.assertNotIn("knee_l.screwhome", keys(issues([pose({"calf_l": [(NX, 90), (Z, 14)]})]), "coupling"))

    def test_ankle_and_toes(self):
        v = measured({"foot_l": [(X, 15)], "ball_l": [(X, 40)]})
        self.assertAlmostEqual(v["ankle_l.flex"], 15, delta=0.5)
        self.assertAlmostEqual(v["toes_l.flex"], 40, delta=0.5)
        self.assertAlmostEqual(measured({"foot_l": [(Y, 10)]})["ankle_l.twist"], 10, delta=1.5)
        self.assertAlmostEqual(measured({"foot_r": [(Y, -10)]})["ankle_r.twist"], 10, delta=1.5)
        self.assertIn("toes_l.flex", keys(issues([pose({"ball_l": [(X, 100)]})]), "rom", "error"))


class MotionAndContact(unittest.TestCase):
    def test_speed_pops_and_contact_grace(self):
        frames = [pose(), pose({"lowerarm_l": [(X, 60)]})]
        found = issues(frames)
        self.assertIn("elbow_l.flex", keys(found, "speed", "error"))
        graced = issues(frames, contacts=[1])
        self.assertIn("elbow_l.flex", keys(graced, "speed", "warn"))
        self.assertNotIn("elbow_l.flex", keys(graced, "speed", "error"))
        smooth = [pose({"lowerarm_l": [(X, 4 * i)]}) for i in range(10)]
        self.assertEqual(keys(issues(smooth), "speed"), set())

    def test_ground_penetration_and_slide(self):
        sunk = pose(shift={"root": (0.0, 0.0, -6.0)})
        self.assertTrue(keys(issues([sunk]), "ground"))
        slid = pose(shift={"root": (0.0, 2.0, 0.0)})
        found = issues([NEUTRAL, slid])
        self.assertIn("foot_l", keys(found, "slide"))

    def test_balance_outside_support(self):
        lean = pose(shift={"spine_01": (0.0, 40.0, 0.0)})
        found = issues([lean, lean, lean])
        self.assertIn("com", keys(found, "balance"))
        self.assertNotIn("com", keys(issues([NEUTRAL] * 3), "balance"))

    def test_summary_names_key_frames(self):
        result = jl.check_frames([NEUTRAL, pose({"lowerarm_l": [(X, -15)]})], NEUTRAL)
        lines = jl.summarize(result, {1: "strike"})
        self.assertTrue(lines[0].startswith("joint check:"))
        self.assertTrue(any("[strike]" in line and "elbow_l.flex" in line for line in lines))
        self.assertEqual(jl.summarize(jl.check_frames([NEUTRAL], NEUTRAL)), ["joint check: clean"])


if __name__ == "__main__":
    unittest.main()
