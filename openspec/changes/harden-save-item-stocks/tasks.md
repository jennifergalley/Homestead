# Tasks

## 1. Implementation

- [ ] 1.1 `WriteStock` writes the width first; `ReadStock` reads a prefixed width for version 13+, pads with zeros, rejects 0 and widths above `ItemCount` (newer build) with `UnsupportedVersion`
- [ ] 1.2 Bump `SimulationSaveVersion` to 13 (orchestrator's call) and accept 12 in `Deserialize` with the fixed-width path
- [ ] 1.3 Native tests: round-trip at 13; load a 13 save written with a shorter width (new items at zero, pack and chest); reject a wider one with `UnsupportedVersion`; version 12 fixture still loads

## 2. Verification

- [ ] 2.1 `Scripts\Test-Native.ps1 -Configuration Release` 7/7, editor module and game target build
- [ ] 2.2 PIE: save, reload, pack and chest contents unchanged
- [ ] 2.3 Update `docs/architecture.md` section 5 and the item skill (no version bump needed for appended items)
