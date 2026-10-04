# Design

## Context
See proposal.md. The planting cue exists in FocusActions, but the HUD retires lone keyed cues after three uses. Only cultivated legacy roots currently grant separate bonus seeds; all six purchased crop varieties already grant none.

## Goals / Non-Goals
Keep planting actionable and visible using simulation eligibility. No growth retuning, wild-forage nerf, new save data, tool-input changes or animation work.

## Decisions
- Use CheckSow/DescribeSow rather than duplicate eligibility in the HUD; exempt the Plant Seeds verb from retirement. Other action hints retain their existing policy.
- Limit the cultivated root bonus to one seed at 25%. Use a stable hash of plot id and calendar day, like existing weed rolls; repeated loads of the same state match. Unlike a stored harvest counter, this adds no save section. Delaying harvest to another day can change the roll, an acceptable tuning trade-off for this small slice.
- Test the actual retirement predicate and sow eligibility, not only the generated focus string; add bounded distribution and replay checks for seed returns.

## Lanes and ownership
Farming Fishing owns garden cues, HUD planting retirement and crop harvesting. Travel Rest edits only bed/sleep/travel members in shared controller/simulation files; keep changes localized. Integration receives the exact checkpoint and owns builds/package admission.

## Risks / Trade-offs
Seed drops reported by Jenny may also include wild forage or replantable fruit; those are deliberately unchanged. Bought crop seed returns are already zero, so do not fabricate a nerf to absent behavior.

## Migration Plan
No enum, placement or save format changes in this slice. Reuse current caches; run focused native tests and request the coordinator's compile slot. Jenny verifies the integrated cue after repeated planting.
