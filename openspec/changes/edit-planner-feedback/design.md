# Design

## Context

See proposal.md. The current inbox uses one optional screenshot per entry and
`backlog:<id>` for priority state. Existing writers directly overwrite JSON.

## Goals / Non-Goals

Keep the existing themed form and board; do not introduce a modal or a new gallery.
Do not touch live inbox/priority data in validation, gameplay or build tools.

## Decisions

- Reuse the form in edit mode, with pencil, current screenshot, Remove screenshot,
  Save changes and Cancel. Disable conflicting form actions during submission.
- PATCH the stable ID; omitted image keeps it, null removes it, supplied image replaces it.
  An entry-content revision rejects stale edits instead of silently overwriting them.
- Serialize add/update writers with an exclusive, repository-local lock (fail promptly,
  no waiting loop). Read the latest validated document inside the lock, preserving
  other entries and document metadata. Atomically rename a unique temporary file,
  following accounting-data.mjs. Use immutable new screenshot filenames, preserving
  earlier images rather than overwriting or deleting them.
- Inbox is authoritative. Markdown sync follows the commit; expose any mirror failure
  explicitly as a saved-with-warning result, not an ordinary successful mirror write.
- Keep edits and scheduling entirely local; no session send/model calls.

## Risks / Trade-offs

- A crash can leave the lock: report its path for recovery rather than guessing ownership.
- Older screenshots remain on disk for safety; broader cleanup is not this lane's task.
- Individual file commits are atomic, not a multi-file transaction; canonical feedback
  survives a markdown failure and the UI states that partial outcome.

## Lanes and ownership

Planner Editing owns extension JS/UI and its focused Node tests plus this change and
feature handoff. Integration alone merges; coordinator refreshes its canvas after merge.
No Unreal build or cache work is necessary.
