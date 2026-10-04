# Design

## Context

See proposal.md. Existing reports already retain disjoint per-call allocations. Build IDs can name an earlier target day: measured02 actually shipped October 4. The delivery changelist and manifests, not report generation time or IDs, establish shipment.

## Goals / Non-Goals

**Goals:** a read-only projection with a chart followed by scan-friendly build entries, using the incumbent theme.

**Non-Goals:** collecting account-wide billing, changing allocations, editing live planner data, guessing delivery times, or game/build work.

## Decisions

- Keep detailed reports and their API representation; add a compact view joining delivered changelist entries, explicit build IDs/report references, delivery manifests and feature titles. Exclude planned/historical/deferred entries.
- Sum integer nano-AIU before display. Combine model/session segments into feature bundles and broad overhead/integration/review categories; do not divide shared work arbitrarily between features.
- Persist Jenny's confirmed cycle as extension configuration. Do not assume later cycles: once it expires, request updated dates rather than display the old cycle as current.
- Chart costs on the shipment's local date in America/Los_Angeles. Zero means no shipment; shipments without cost evidence show unknown. A concise qualifier explains observed AIU and missing historical costs.
- Planner owns extension files, focused Node/browser fixtures and feature handoff. Integration owns publication; all reads and tests remain agent-free.

## Risks / Trade-offs

- Missing historical costs -> show unknown, never fabricate zero.
- Incomplete accounting -> show recorded subtotal and concise coverage note; retain raw reports.
- Missing exact delivery time -> show the actual recorded date only.
- Metadata ambiguity -> require explicit linkage, not date-shaped ID matching.

## Migration Plan

No data migration. Integration admits the source commit and the coordinator reloads its existing canvas. Rollback restores the renderer/projection without modifying inbox or accounting evidence.
