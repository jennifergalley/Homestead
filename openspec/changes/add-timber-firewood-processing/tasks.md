# Tasks

## 1. Playable authority increment

- [x] 1.1 Append Timber, Firewood and SplitFirewood identifiers, bump the simulation save version, and wire names/costs/requirements; verify all existing numeric IDs remain unchanged and an old-version save rejects before state mutation.
- [x] 1.2 Change only mature generated trees to yield six Timber and four Branch while retaining sapling Branch/Fiber yields and stable cleared keys; verify successful, full-pack and missing-Hatchet clear transactions with portable simulation tests.
- [x] 1.3 Implement atomic Hatchet-gated Timber splitting and Firewood-first/Branch-fallback cookfire fueling under the existing cap; verify success, missing prerequisite, net-capacity overflow, fuel priority, fallback and no-fuel diagnostics with portable simulation tests.
- [x] 1.4 Extend pack/chest transfer and serialization coverage for both new items; verify exact quantities, layout/capacity invariants and current-version save round-trip in the portable suite.

## 2. Integrated gameplay path

- [x] 2.1 Expose the Split Firewood recipe and dual-fuel guidance through the existing Field Book, focus prompt and result messages without changing mapped controls; verify focused menu/source tests cover the new row, Hatchet requirement and accepted fuels.
- [ ] 2.2 Extend the generated-world producer fixture to fell one actual stable-key mature tree, assert the exact Timber/Branch reward, craft Firewood through mapped recipe input, fuel its existing cookfire, and store a new material; verify all prior regional terrain, water, batching, building, garden and rollback assertions remain active.
- [ ] 2.3 Extend the separate consumer fixture to verify exact pack/chest Timber and Firewood, positive cookfire fuel and the same cleared tree key after reload; verify the felled tree remains absent and no test recovery occurs.

## 3. Bounded acceptance and delivery

- [x] 3.1 Run the focused portable simulation tests, related source tests and strict OpenSpec validation; verify all new behavior and every pre-existing selected assertion in those targets passes before native work.
- [x] 3.2 Build the Unreal editor target once under the guarded run slot and fix only integration defects caused by this change; verify compile/link success without overlapping Unreal operations.
- [ ] 3.3 Package a fresh immutable Shipping candidate using compatible caches, then run the focused producer and separate consumer with a fresh test profile; verify exact executable identity, zero recoveries, the mapped timber-to-firewood loop and retained regional-water acceptance facts.
- [ ] 3.4 Capture and inspect ordinary gameplay evidence when the changed recipe/interaction is naturally visible, update development status and the change artifacts with exact evidence and disclosed limitations, and promote only if the candidate is more useful than `regional-water-17`; verify `Start-Preview -ValidateOnly` on any selected candidate while retaining `regional-water-17` as rollback.
