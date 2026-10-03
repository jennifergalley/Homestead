# Design

## Context
See proposal.md. Main's 80ec5928 implements sparse pack squares and the optional
trailing packslots save section. Review and repair that implementation, not replace it.

## Decisions
Reuse PackRowRules::Grid and Simulation::MoveToPackSlot for pointer/controller
placement. Inventory counts remain authoritative; slot metadata names existing
carried stacks/garments and never creates stock. Keep gaps and exact indices through
reload. Preserve legacy ordering when packslots is absent, and do not bump the
simulation save version.
An occupied pack square swaps even matching stacks; the hotbar and chest retain
their existing merge actions. Reconcile slot metadata on every inventory adjustment
and validate references after all optional save sections have been read.

## Risks / Verification
Validate malformed/duplicated save metadata before committing a load, including
slot identities and duplicate references. Native tests cover exact-square swaps,
unchanged hotbar/chest merges, consumption reconciliation,
gaps, rejected operations and saves with/without the section. Inspect UI drop paths
for empty squares and run one editor compile for the coherent UI batch.
Town owns economy/general-store files; coordinate only a necessary shared interface.
Jenny performs the final drag-and-reload playtest.
