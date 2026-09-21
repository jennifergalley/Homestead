# Tree source preparation lane

## Scope and authorization

Reactivated 2026-09-21 by Homestead Orchestrator for independent source-only
preparation of the already admitted Poly Haven Tree Small 02. The current
game-plan policy snapshot supplied by the coordinator supersedes this branch's
historical planning-only and arbitrary iteration/deadline restrictions.
This lane implements only the tree-source portion of task 2.3 and supports
later LOD/material work; it does not claim whole central tasks complete.

Owned files: `Scripts\Environment\TreePreparation.*`,
`Assets\Environment\TreeSmall02Prepared\`, and this addendum.
Raw copied originals remain ignored and unchanged at
`Assets\Source\woodland-preparation-20260920-182217-d1f84e39\tree_small_02`.
The coordinator supplied original download receipts and source inventory in
this session's `files\tree-source-context` directory.

Reuse official Blender 4.5.14 FBX IO and standard mesh reduction, with factory
startup, disabled automatic asset script execution, CPU-only operations, and
lane-owned profile/temp/output. Inspect actual geometry first. Prevent imported
legacy absolute texture references from resolving; materials must use only
the admitted texture whitelist. No custom decimation engine or handcrafted tree.

Do not modify central proposal/design/tasks, game code, native import commandlet,
common guard/build scripts, existing imported content, launcher or main checkout.
No Unreal/UBT/UAT/cook/native helpers, GUI/GPU captures, external research,
downloads, account/security changes, schedules or Notion writes.

## Deliverable and acceptance

Produce versioned source-derived conventional LOD FBXs (separate files are
acceptable), retaining a valid source-derived LOD0 and explicitly identifying
practical reduced candidates. Record actual triangle counts per material,
bounds, root/pivot, units, both UV layers and all three material slots; bind
outputs to source/license/tool hashes. Reimport exports through Blender to
check the handoff contract, and inspect a concise CPU/source-only silhouette
comparison when useful. This is not an in-game image or runtime performance pass.

Main implementer owns Unreal import, material reconstruction, instance placement,
collision, runtime LOD selection and playable validation. Freeze a version before
handoff; later experiments must use a new version. Commit and push this private
branch only after verification; no force/merge/PR.

## Progress

- [x] Confirmed isolated worktree, scoped authorization and existing OpenSpec context.
- [x] Verified copied originals against acquisition receipts and inspected geometry.
- [x] Prepared conventional reduced variants and exact material/texture contract.
- [x] Checked exports, provenance and source-only visual comparison.
- [x] Frozen v3 and reverified it without changing any prepared-file hash.

The source commit and remote ref are reported to the coordinator/main at
handoff after the private push succeeds; they are not self-referential fields
inside the frozen asset directory.

## Source results

Selected handoff is `Assets\Environment\TreeSmall02Prepared\v3`, with exact
integration contract in that namespace's README. Actual source triangles:
2,062,487 (branches 94,814, leaves 1,939,380, trunk 28,293).
Selected reduced levels: 378,926 / 231,785 / 170,289 triangles. All retain the
three ordered material regions and both UV layers; four export/reimport checks
passed. Root remains (0,0,0); source height is about 4.5567 m with approximately
2.4 cm below-root geometry. FBX unit metadata is explicit, not guessed scaling.

Aggressive leaf reduction produced visible canopy loss and was rejected.
Planar reduction retained excessive source geometry. The final conservative
standard-collapse chain preserves approximately 99% front/side pixel coverage
at its nearest reduced level; farther levels lose more detail (LOD3 side about
12% thinner), documented rather than called an invisible optimization.
Two internal CPU/source views were reviewed; they are not game screenshots.
Raw trials remain in this session's files, not in the published source namespace.

No main game code, central planning artifacts, existing content, launcher,
Unreal processes or shared authoring/build tools were modified/launched.
Main still owns in-game appearance, material response, collisions, instancing,
runtime LOD selection and performance. This lane does not mark those tasks done.
