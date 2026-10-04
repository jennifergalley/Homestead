# OpenSpec task planner canvas

A local Copilot app side panel: OpenSpec progress, screenshot feedback with
pencil editing, reorderable build scheduling, and shipped features/times/costs
with broad cost groups and a daily chart. Refresh, feedback and scheduling make
no model calls or agent notifications. Quote only attaches a card to the chat
composer.

## Install in another repository

1. Copy this **whole folder** to `.github\extensions\openspec-task-planner\` in
   your repo, including sibling `.mjs` files and `billing-cycle.json`, but not
   Homestead's live backlog/screenshots/accounting data.
2. Or ask your Copilot app agent to call `install_extension` with:
   ```json
   {
     "url": "https://github.com/jennifergalley/Homestead/tree/main/.github/extensions/openspec-task-planner",
     "scope": "project"
   }
   ```
   **Use project scope:** the repo root is derived from the extension's location.
3. Adapt settings below, call `extensions_reload`, then open with
   `open_canvas({canvasId: "openspec-task-planner", instanceId: "planner-1"})`.
   Diagnose loading with `extensions_manage`. The CLI supplies
   `@github/copilot-sdk`; viewing needs no npm install, engine or OpenSpec CLI.

## Data contract

Paths below are relative to the destination repo, except the billing config.
Opening/refreshing only reads; user actions create missing writable files.

| File | Access and contract | When missing |
| --- | --- | --- |
| `openspec\changes\*\tasks.md` | Read recursively, excluding `archive`/hidden directories. `##` sections, numbered `- [ ] 1.1 Task` / `- [x] 1.2 Task` lines. IDs/titles come from change-directory paths/names. | No OpenSpec cards; feedback works. |
| `docs\handoff\priority.json` | Read/write board state: `order` and `removed` ID arrays, `slots` mapping IDs to local `YYYY-MM-DD HH:mm` release keys; legacy `nextBuild` is supported. Removal hides a card, not its source. | Defaults to empty state; also currently falls back on unreadable/invalid JSON. Created on a board action. |
| `docs\handoff\backlog-inbox.json` | Read/write authoritative `{ "entries": [] }`: stable `id`, `title`, `description`, `createdUtc`, optional `updatedUtc`, `imageFile`, `imageMime`. Board ID: `backlog:<id>`. Revision-checked edits preserve IDs/schedules. | Empty inbox; created on save. Invalid data errors, never silently replaced. |
| `docs\handoff\backlog.md` | Read/write Markdown mirror under `## Later`, replacing only `<!-- jenny-inbox:start -->` through `<!-- jenny-inbox:end -->`. Other prose survives. JSON remains authoritative if mirroring fails; the form shows a warning. | Created on feedback save. It is not a board input. |
| `docs\handoff\attachments\backlog\` | Write/read screenshots by inbox filename: one PNG/JPEG/WebP/GIF per entry, maximum 8 MB. Replacement writes a new filename; removal clears the reference, retaining old files. | Created for an upload; a missing referenced image returns 404. |
| `docs\handoff\builds.md` | Read ledger: `## YYYY-MM-DD — h:mm AM/PM`, `- Status: delivered`, `- Build ID: <id>`, `- Ships:` and feature bullets. A `Planner/accounting:` report link can supply the ID. Actual shipment time determines order, not build ID. Historical/deferred entries are excluded from shipped cards; `## Later` holds deferrals. | No recorded builds; upcoming slots still generated. Invalid delivered date/time is flagged, never guessed. |
| `docs\handoff\measured-build-*.json` | Read optional delivery metadata, keyed by `buildId` or `plannerDelivery.buildId`; optional ISO `deliveredAt` is interpreted in the configured timezone. Separately, the hard-coded `measured-build-02.json` registry annotates selected cards with shipped/pending-acceptance badges: `plannerDelivery` needs `status: "shipped"`, `playerAcceptance: "pending"`, `buildId`, `selectedIds`. | No manifest metadata/badges. Present malformed manifests error. A manifest alone does not declare a shipment: the ledger does. |
| `docs\handoff\accounting\reports\*.json` | Read externally produced schema-version-1 `buildId`, `totals`, `segments`, coverage/legacy metadata; integer-string `recordedNanoAiu` / `estimatedNanoAiu` and call counts. Producer shape: `accounting-data.mjs`'s `buildUsageReport`. | Shipped costs unknown; no-shipment days zero. Invalid reports/group totals error. |
| Extension-local `billing-cycle.json` | **Required by the current loader:** inclusive `start`/`end`, next-day `reset` (`YYYY-MM-DD`) and IANA `timeZone`. Replace the bundled Sep 30-Oct 30 2026 dates with your own. | Missing/invalid file errors. An expired period shows an update notice, not an old chart labelled current. |
| `Automation\run.json` and `Automation\status.json` | Optional read-only active-work hint: matching run IDs, `run.state: "running"`, `run.openSpecChange`, `status.phase: "implementing"` and `updatedUtc` within 15 minutes. | No active badge; task completion still determines proposed/paused/complete. |

Feedback saves create `backlog-inbox.json.lock` and unique `.tmp` siblings for
atomic inbox/Markdown replacement. Preserve IDs/screenshots; test on fixtures.

## Homestead-specific settings to change

- `extension.mjs`: canvas ID if needed, display name, description and panel title.
- `planner-html.mjs`: Homestead/Jenny branding, copy and cost labels.
- `planner-data.mjs`: `project`, `jenny-` feedback ID prefix, Jenny Markdown copy/
  markers, `measuredBuildManifestSegments` (`measured-build-02.json`), and
  `slotMinutes` (currently 7:30 AM, 4 PM, 9 PM in machine-local time).
- `cost-view.mjs`: feature/category label mappings and default timezone;
  `billing-cycle.json`: your confirmed dates/timezone.
- AIU wording in `planner-html.mjs` / `accounting-data.mjs`: observed nano-AIU is
  divided by `1e9`, **not billing-reconciled AI credits**. The chart attributes
  build cost to shipment day, not call day or account-wide usage. Change that
  claim only with actual billing evidence; keep estimates/unknown coverage
  distinct, and keep automatic notifications off.
