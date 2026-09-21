"""Synthetic validator tests; these do not substitute for a native diagnostic run."""
import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "presentation_analysis", Path(__file__).parents[1] / "Scripts" / "Analyze-PresentationDiagnostics.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PresentationDiagnosticsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.rows = []
        for label, count in (("timing-idle", 2), ("timing-walk", 6), ("timing-turn", 6), ("timing-sweep", 6)):
            self.rows.extend(dict(pass_name=label, wall_frame_ms=1000,
                                 captures_requested_before_tick=0, capture_requested_this_tick=0,
                                 speed=40, view_yaw=i * 15) for i in range(count))
        for index in range(2):
            self.rows.append(dict(pass_name="capture-walk", wall_frame_ms=1000,
                                  captures_requested_before_tick=index, capture_requested_this_tick=1,
                                  speed=40, view_yaw=0))
        (self.root / "telemetry.csv").write_text("frame,pass\n0,capture-walk\n1,capture-walk\n")
        (self.root / "presentation-settings.txt").write_text("[start]\nactual_viewport=1280,720\n[end]\n")
        (self.root / "engine.log").write_text("mode=test-sandbox automation_input=1 smoke_actor=0 visual_actor=1")

    def run_analysis(self):
        with (self.root / "presentation-timings.csv").open("w", newline="") as target:
            writer = csv.DictWriter(target, fieldnames=["pass" if key == "pass_name" else key for key in self.rows[0]])
            writer.writeheader()
            writer.writerows({("pass" if key == "pass_name" else key): value for key, value in row.items()} for row in self.rows)
        return module.analyze(self.root)

    def test_valid_separation(self):
        report = self.run_analysis()
        self.assertEqual(report["captured_frames"], 2)
        self.assertEqual(report["screenshot_free_timing"]["timing-walk"]["wall_seconds"], 6)

    def test_rejects_readback_contamination(self):
        self.rows[3]["captures_requested_before_tick"] = 1
        with self.assertRaisesRegex(ValueError, "contaminated"):
            self.run_analysis()

    def test_rejects_missing_motion(self):
        for row in self.rows:
            row["speed"] = 0
        with self.assertRaisesRegex(ValueError, "Movement"):
            self.run_analysis()

    def test_rejects_missing_camera_sweep(self):
        for row in self.rows:
            row["view_yaw"] = 0
        with self.assertRaisesRegex(ValueError, "camera sweep"):
            self.run_analysis()

    def test_rejects_short_interval(self):
        self.rows[0]["wall_frame_ms"] = 1
        with self.assertRaisesRegex(ValueError, "incomplete"):
            self.run_analysis()

    def test_rejects_wrong_capture_interval(self):
        (self.root / "telemetry.csv").write_text("frame,pass\n0,timing-walk\n1,capture-walk\n")
        with self.assertRaisesRegex(ValueError, "subsequent capture"):
            self.run_analysis()

    def test_rejects_unisolated_process(self):
        (self.root / "engine.log").write_text("mode=default automation_input=0")
        with self.assertRaisesRegex(ValueError, "isolated"):
            self.run_analysis()

    def shipping_admission(self, **overrides):
        values = dict(version="1", shipping="1", trace_compiled="0", route="visual",
                      save_directory=str(self.root / "SmokeSave"),
                      project_saved_directory=str(self.root / "EngineUser" / "Saved"))
        values.update(overrides)
        (self.root / "qa-admission.txt").write_text("".join(f"{key}={value}\n" for key, value in values.items()))

    def test_shipping_without_development_log(self):
        (self.root / "engine.log").unlink()
        self.shipping_admission()
        self.assertIn("Shipping QA", self.run_analysis()["admission"])

    def test_rejects_shipping_wrong_directory_or_route(self):
        for overrides in (dict(save_directory=str(self.root.parent / "SmokeSave")),
                          dict(project_saved_directory=str(self.root / "Saved")),
                          dict(route="smoke"), dict(trace_compiled="1"),
                          dict(shipping="0"), dict(version="2")):
            with self.subTest(overrides=overrides):
                self.shipping_admission(**overrides)
                with self.assertRaisesRegex(ValueError, "isolated"):
                    self.run_analysis()

    def test_rejects_ambiguous_shipping_admission(self):
        for suffix in ("route=visual\n", "broken\n"):
            self.shipping_admission()
            with (self.root / "qa-admission.txt").open("a") as target:
                target.write(suffix)
            with self.assertRaisesRegex(ValueError, "Malformed"):
                self.run_analysis()


if __name__ == "__main__":
    unittest.main()
