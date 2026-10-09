# Proposal

## Why

Jenny's Oct 4 feedback (`jenny-mutdjl7e-nzl7lh`) shows floating fence rails and visible mortise fragments where upright post bodies should be. Supporting posts must remain rendered while the fence remains, including after nearby clearing or cultivation.

## What Changes

- Diagnose and repair the actual post-rendering failure, preserving the existing ruined fence layout and supports.
- Keep supports independent of subordinate clearing and view-distance changes; only whole-fence removal may remove them.
- Hand any required existing mesh/material repair or import to the assigned Opus Fishing Art lane.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `estate-disrepair`: require visible persistent supporting fence posts.

## Impact

Reuse research: `AHomesteadDerelictFarm::Fence` already places original upright, leaning, snapped and fallen posts in separate ISM batches. At baseline it is scenery only, with no Simulation clear/disassembly link; nearby clearing cannot intentionally remove those instances. The handed-off screenshot shows surviving internal mortise pieces, making missing body geometry/material rendering a specific unresolved lead, not a confirmed cause.

Reuse the original `FarmFence` assets and actor; add no speculative disassembly system or replacement geometry. Environment owns code diagnosis; Fishing Art owns any asset correction/import. First playable result is the existing fence with visible supports from ordinary views and after nearby clearing. Branch evidence and Jenny's integrated acceptance stay separate.
