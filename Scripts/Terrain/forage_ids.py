"""Stable placement ids for generated forage (forage.py). Placement ids are save identity: a picked bush
is remembered by its id, so a rebake must never give an existing id to a different kind or position.

The committed .inc files are the record. A rebake keeps every committed row verbatim and in order,
recognises the generator's picks that are those same rows, and gives genuinely new picks fresh ids above
every id the range has ever used (retired ids included), inside that output's own reserved range. A pick
the generator no longer makes (the terrain changed) keeps its row; retiring one is an explicit act
(RETIRED_* in forage.py) that leaves a hole, never a renumbering.

Pure Python (no numpy), so Scripts/Terrain/test_forage_ids.py can exercise it with synthetic cases.
"""
import math
import os
import re

ROW = re.compile(r"^(\w+)\((\d+), ResourceKind::(\w+), (-?\d+(?:\.\d+)?), (-?\d+(?:\.\d+)?)\);$")
SAME_ROW_CM = 100.0     # a pick within 1 m of a committed row of the same kind is that row


class Row:
    def __init__(self, macro, pid, kind, x_cm, y_cm, line=None):
        self.macro, self.id, self.kind, self.x, self.y = macro, pid, kind, x_cm, y_cm
        self.line = line or f"{macro}({pid}, ResourceKind::{kind}, {x_cm:.1f}, {y_cm:.1f});"

    def __repr__(self):
        return self.line


def read_rows(path, macro, first_id, end_id):
    """The committed rows of one generated .inc: header comments and rows, in file order. A missing file,
    a malformed row or an id outside [first_id, end_id) is an error, never a silent regenerate."""
    if not os.path.exists(path):
        raise SystemExit(f"{path} is missing: restore it from git before regenerating (ids are save identity)")
    header, rows = [], []
    for number, raw in enumerate(open(path).read().splitlines(), 1):
        if not raw.strip():
            continue
        if raw.startswith("//"):
            if rows:
                raise SystemExit(f"{path}:{number}: comment after the rows")
            header.append(raw)
            continue
        m = ROW.match(raw)
        if not m or m.group(1) != macro:
            raise SystemExit(f"{path}:{number}: not a {macro}(...) row: {raw!r}")
        pid = int(m.group(2))
        if not first_id <= pid < end_id:
            raise SystemExit(f"{path}:{number}: id {pid} outside {first_id}-{end_id - 1}")
        rows.append(Row(macro, pid, m.group(3), float(m.group(4)), float(m.group(5)), raw))
    ids = [r.id for r in rows]
    if len(set(ids)) != len(ids):
        raise SystemExit(f"{path}: duplicate ids")
    return header, rows


def _next_id(rows, retired, first_id, end_id):
    used = [r.id for r in rows] + list(retired)
    nid = max(used) + 1 if used else first_id
    if nid >= end_id:
        raise SystemExit(f"id range {first_id}-{end_id - 1} is full")
    return nid


def _near(a_xy, rows, kind, cm):
    return any(r.kind == kind and math.hypot(r.x - a_xy[0], r.y - a_xy[1]) < cm for r in rows)


def allocate_pool(committed, picks, targets, macro, first_id, end_id, retired=(), gap_cm=0.0):
    """Estate forage. committed: rows from the .inc; picks: [(kind, x_cm, y_cm)] in the generator's order;
    targets: {kind: how many rows of that kind the pool should hold}. Committed rows stay (minus retired ones)
    in order; a pick that is a committed row is dropped; other picks are added with fresh ids only while
    their kind is short of its target and they keep gap_cm from every row of their kind."""
    rows = [r for r in committed if r.id not in retired]
    added = []
    for kind, x, y in picks:
        if _near((x, y), rows, kind, SAME_ROW_CM):
            continue
        if sum(r.kind == kind for r in rows) >= targets.get(kind, 0):
            continue
        if gap_cm and _near((x, y), rows, kind, gap_cm):
            continue
        row = Row(macro, _next_id(committed + added, retired, first_id, end_id), kind, x, y)
        rows.append(row)
        added.append(row)
    return rows, added


def allocate_slots(committed, slot_of, picks, macro, first_id, end_id, retired=()):
    """Roadside forage, one stop per chainage slot. slot_of(row) -> the slot a committed row stands in;
    picks: {slot: (kind, x_cm, y_cm)} for the slots the generator found a verge in. A slot with a committed
    row keeps it; an empty slot with a pick gets a fresh id; a slot without either stays a hole."""
    rows = [r for r in committed if r.id not in retired]
    taken = {slot_of(r) for r in rows}
    added = []
    for slot in sorted(picks):
        if slot in taken:
            continue
        kind, x, y = picks[slot]
        row = Row(macro, _next_id(committed + added, retired, first_id, end_id), kind, x, y)
        rows.append(row)
        added.append(row)
        taken.add(slot)
    return rows, added


def write_rows(path, header, rows):
    with open(path, "w", newline="\n") as fh:
        fh.write("\n".join(header + [r.line for r in rows]) + "\n")
