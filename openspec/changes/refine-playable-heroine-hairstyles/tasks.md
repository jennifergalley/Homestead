# Tasks

## 1. Earliest usable waves

- [ ] 1.1 Author retained-cadence mid-back waves from verified source; verify tip height against shoulder/waist midpoint and back/three-quarter CPU views without the rejected compressed shelf.
- [x] 1.2 Export the six existing joined LongWave body/outfit variants with new hair only; verify unchanged non-hair surfaces, rig, weights and materials through FBX round trips and publish exact current-selector import paths.
- [ ] 1.3 Main imports and exercises the early wave set in a cooked candidate; verify actual Appearance selection plus ordinary back/side walking/action views before claiming playable completion.

## 2. Straight blonde bob

- [x] 2.1 Author original straight bob and strand material, coordinating explicit blonde selection with UI; verify supported color choices and unchanged face/body/rig in source previews.
- [x] 2.2 Export six existing joined Bob body/outfit variants; verify non-hair preservation, rig/weight/material round trips and publish current-selector import paths.
- [ ] 2.3 Main imports and exercises the bob in game; verify actual blonde silhouette, selection, saved appearance and ordinary movement for all body fits.

## 3. Modular parity and handoff

- [x] 3.1 Apply identical new hair to the corresponding six modular base variants; verify complete permanent coverage, rig and material contract while ponytail/garment hashes remain unchanged.
- [ ] 3.2 Commit source/provenance/hash/import records and report exact source versus cooked acceptance status; verify private branch push and main-owned integration handoff without automatic promotion.

Source checkpoint: twelve joined FBXs and source snapshots pass geometry,
material, rig and interchange checks. 1.1 remains open: the new waves are truly
mid-back and retain their old cadence, but CPU three-quarter review still reads
as broad, chunky locks with a conspicuous cut edge. Feathered alpha improves the
tips without making this a confidently accepted natural end silhouette. Do not
equate the valid export receipt with visual acceptance. No source-lane commit,
push, Unreal import or gameplay acceptance has occurred.

Stable modular handoff received 2026-09-20 23:53 Arizona. Six additional bases
in `HairstyleRefinement\Modular` now carry the same joined candidate hair.
All 54 stable modular input files remain hash-identical, including ponytails
and garments. Separate permanent bra/briefs coverage, all nine material slots,
nonhair surfaces/UVs/weights/normals and shader/texture data pass round trips.
This closes source parity only; the wave visual-finish caveat remains open.

Reuse-first refinement (Jenny's explicit 23:28 preference, relayed after this
checkpoint): the bounded admitted-source review and choices are in design.md.
For remaining 1.1 work, retain fitted long01 and author only its essential new
ends. Before further substantial bob shaping, prefer targeted adaptation of
fitted bob01's unapproved diagonal fringe/panels while reusing its cap and
strand layout. The existing custom bob remains a validated source fallback,
not evidence that wholesale custom construction is necessary. Existing exports,
completed source checks and garment work are not discarded or redone by this
planning refinement; new affected hair revisions must be revalidated.

Bob reuse result after waves-only freeze: `HairstyleRefinement\BobReuse` now
contains six joined and three modular Bob FBXs plus snapshots and receipts.
The bounded bob01 fringe/chin adaptation retains its stock cap, layered UV
strands and original alpha. Source/interchange and permanent-coverage checks
pass; all 24 frozen wave/Appearance files remain hash-identical. The earlier
custom bob remains a separate fallback. This does not mark 2.3 gameplay/art
acceptance complete or change the four completed source-task checkboxes.

Final technical selection: `final-bundle.json` chooses BobReuse's six joined
and three modular Bob outputs plus `PublishedWaveParity`'s three modular waves
copied from exact published a3a6dee inputs. No sculpting was performed for
published parity. The later root wave files are not the published checkpoint
and are excluded; `publication-audit.json` records the distinction. Frozen
assets/palette and parent handoff files remain untouched. 1.1, 1.3 and 2.3
remain open; main owns normal-game review, rollback if worse, and final publish.

Final parent-selected freeze: all 12 canonical Joined + 6 canonical Modular
outputs now select latest feather-tip waves plus stock-based Bob. Bob-only
technical consolidation supersedes the old canonical parametric Bob, without
new shaping or rewriting waves/palette. `final-bundle.json` and full hashes are
the final handoff; a3a6dee stays immutable and explicitly different. Parent will
make the one final commit after source completion; no source-lane commit or
runtime/gameplay/visual acceptance is claimed.
