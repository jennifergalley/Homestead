"""Export metadata-only local usage. Usage: python Scripts\\Export-BuildUsage.py --help"""

import argparse
import json
import sqlite3
from contextlib import closing
from datetime import datetime, timezone
from pathlib import Path

FIELDS = (
    "id,session_id,turn_index,agent_id,parent_tool_call_id,model,reasoning_effort,"
    "input_tokens,output_tokens,cache_read_tokens,cache_write_tokens,reasoning_tokens,"
    "total_nano_aiu,token_details_json,created_at"
)


def export_usage(database: Path, allocation: dict) -> dict:
    """Read only usage rows for explicitly allocated runtime sessions."""
    sessions = sorted({segment["sessionId"] for segment in allocation["segments"]})
    if not sessions:
        raise ValueError("Allocation must identify runtime sessions")
    with closing(sqlite3.connect(database.resolve().as_uri() + "?mode=ro", uri=True)) as connection:
        connection.row_factory = sqlite3.Row
        connection.execute("BEGIN")
        cursor = connection.execute(
            "SELECT COALESCE(MAX(id),0) FROM assistant_usage_events"
        ).fetchone()[0]
        rows = connection.execute(
            f"SELECT {FIELDS} FROM assistant_usage_events "
            f"WHERE session_id IN ({','.join('?' for _ in sessions)}) "
            "AND created_at >= ? AND id <= ? ORDER BY id",
            [*sessions, allocation["authorizedAt"], cursor],
        ).fetchall()
    return {
        "schemaVersion": 1,
        "buildId": allocation["buildId"],
        "sourceId": "local-assistant-usage-v1",
        "capturedAt": datetime.now(timezone.utc).isoformat(),
        "throughEventId": cursor,
        "sessionIds": sessions,
        "since": allocation["authorizedAt"],
        "events": [dict(row) for row in rows],
    }


def merge_snapshot(previous: dict, current: dict) -> dict:
    """Retain captured events even after local session archival."""
    if (previous["sourceId"] != current["sourceId"]
            or previous["since"] != current["since"]
            or previous.get("buildId", current["buildId"]) != current["buildId"]):
        raise ValueError("Output already belongs to a different source/build boundary")
    events = {event["id"]: event for event in previous["events"]}
    for event in current["events"]:
        if event["id"] in events and events[event["id"]] != event:
            raise ValueError(f"Conflicting captured usage record {event['id']}")
        events[event["id"]] = event
    current["events"] = sorted(events.values(), key=lambda event: event["id"])
    current["sessionIds"] = sorted(set(previous["sessionIds"]) | set(current["sessionIds"]))
    current["throughEventId"] = max(previous["throughEventId"], current["throughEventId"])
    return current


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--allocation", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    allocation = json.loads(args.allocation.read_text(encoding="utf-8-sig"))
    snapshot = export_usage(args.database, allocation)
    if args.out.exists():
        snapshot = merge_snapshot(json.loads(args.out.read_text(encoding="utf-8-sig")), snapshot)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.out.with_suffix(args.out.suffix + ".tmp")
    temporary.write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
    temporary.replace(args.out)
    print(f"Exported {len(snapshot['events'])} usage records through {snapshot['throughEventId']}")


if __name__ == "__main__":
    main()
