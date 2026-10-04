# Proposal

## Why

Jenny wants to understand what each shipped build bought, rather than inspect a session/token ledger. The current Build cost tab buries delivery and feature information in detailed accounting rows.

## What Changes

- Replace the detailed table with shipped features, actual shipment date, recorded cost and a short feature/category breakdown.
- Put a daily shipped-build cost chart first, for Jenny's confirmed September 30 through October 30 billing period, resetting October 31. Days without shipments show zero; missing costs stay unknown.
- Keep recorded AIU explicitly distinct from billing-reconciled credits and all detailed source reports intact.

## Capabilities

### New Capabilities

- `planner-build-cost-summary`: agent-free shipment summaries and a billing-period daily chart.

### Modified Capabilities

None.

## Impact

Reuse the existing local accounting reports, delivery manifests, changelist, theme and Node test runner. No new runtime dependency or external service is needed; custom code only joins shipment metadata and renders the simpler view. The first demonstration is the Build cost tab, not an in-game change. Integration publishes it; Jenny's visual acceptance is separate. Inbox, priority, attachments, game builds and saves remain untouched.
