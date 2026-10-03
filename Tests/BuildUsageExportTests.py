"""Metadata-export regression tests. Usage: python Tests\\BuildUsageExportTests.py"""

import importlib.util
import sqlite3
import tempfile
import unittest
from contextlib import closing
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "Scripts" / "Export-BuildUsage.py"
SPEC = importlib.util.spec_from_file_location("export_usage", MODULE_PATH)
EXPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EXPORTER)


class ExportTests(unittest.TestCase):
    def test_explicit_session_time_scope_and_readonly_database(self) -> None:
        with tempfile.TemporaryDirectory(prefix="homestead-usage-") as directory:
            path = Path(directory) / "usage.db"
            with closing(sqlite3.connect(path)) as connection, connection:
                columns = EXPORTER.FIELDS.split(",")
                connection.execute(
                    "CREATE TABLE assistant_usage_events (" +
                    ",".join(f"{column} " + ("INTEGER" if column == "id" else "TEXT")
                             for column in columns) + ")")
                connection.executemany(
                    "INSERT INTO assistant_usage_events (id,session_id,created_at) VALUES (?,?,?)",
                    [(1, "wanted", "2026-10-01T00:00:00Z"),
                     (2, "wanted", "2026-10-03T04:33:00Z"),
                     (3, "unrelated", "2026-10-03T04:34:00Z")])
            allocation = {"buildId": "test", "authorizedAt": "2026-10-03T04:32:10.763Z",
                          "segments": [{"sessionId": "wanted"}]}
            result = EXPORTER.export_usage(path, allocation)
            self.assertEqual([row["id"] for row in result["events"]], [2])
            self.assertEqual(result["throughEventId"], 3)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(connection.execute("SELECT COUNT(*) FROM assistant_usage_events").fetchone()[0], 3)

    def test_archived_rows_survive_repeat_export_and_conflicts_fail(self) -> None:
        base = {"sourceId": "local-assistant-usage-v1", "buildId": "test", "since": "today",
                "sessionIds": ["a"], "throughEventId": 2, "events": [{"id": 1, "value": 10}]}
        latest = {**base, "events": [{"id": 2, "value": 20}], "sessionIds": ["b"], "throughEventId": 3}
        merged = EXPORTER.merge_snapshot(base, latest)
        self.assertEqual([row["id"] for row in merged["events"]], [1, 2])
        self.assertEqual(merged["sessionIds"], ["a", "b"])
        with self.assertRaisesRegex(ValueError, "Conflicting"):
            EXPORTER.merge_snapshot(base, {**base, "events": [{"id": 1, "value": 11}]})
        with self.assertRaisesRegex(ValueError, "different"):
            EXPORTER.merge_snapshot(base, {**base, "buildId": "other"})


if __name__ == "__main__":
    unittest.main()
