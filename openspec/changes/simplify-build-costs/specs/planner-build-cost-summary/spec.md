# Planner build cost summary

## Purpose

Help Jenny understand shipped Homestead improvements and their observed AI cost without navigating detailed session and token accounting.

## ADDED Requirements

### Requirement: Concise shipped build summaries

The Build cost tab SHALL list shipped features, actual shipment date/time, recorded cost and a coarse feature/category breakdown, without session/token tables. Both planner tabs MUST label shipments `YYYY-MM-DD — h:mm AM/PM`, order them by parsed shipment date/time newest first and visibly flag invalid shipment headings rather than guessing. Deferred/historical/unshipped records MUST NOT appear as delivered cards or stale upcoming releases. It MUST preserve detailed evidence, keep missing costs unknown and distinguish observed AIU from billing-reconciled credits. Reading it MUST NOT notify or invoke agents.

#### Scenario: Measured build shipment

- **WHEN** Jenny opens Build cost
- **THEN** measured02 appears on its October 4 shipment date despite its October 3 ID, with shipped improvements and grouped costs; unshipped work is excluded

### Requirement: Current billing-period daily chart

The tab SHALL show a daily shipped-build cost bar chart first, restricted to Jenny's confirmed September 30 through October 30 period, resetting October 31. Days without shipments MUST show zero; days with unavailable shipment costs MUST indicate unknown. An expired or missing cycle MUST NOT be presented as current.

#### Scenario: Zero-shipment days

- **WHEN** the confirmed cycle is current
- **THEN** every date in the period is represented, multiple shipments on a day aggregate once, and days with no shipment display zero
