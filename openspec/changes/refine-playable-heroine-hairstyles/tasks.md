# Tasks

## 1. Earliest usable waves

- [ ] 1.1 Author retained-cadence mid-back waves from verified source; verify tip height against shoulder/waist midpoint and back/three-quarter CPU views without the rejected compressed shelf.
- [x] 1.2 Export the six existing joined LongWave body/outfit variants with new hair only; verify unchanged non-hair surfaces, rig, weights and materials through FBX round trips and publish exact current-selector import paths.
- [ ] 1.3 Main imports and exercises the early wave set in a cooked candidate; verify actual Appearance selection plus ordinary back/side walking/action views before claiming playable completion.

## 2. Straight blonde bob

- [x] 2.1 Author original straight bob and strand material, coordinating explicit blonde selection with UI; verify supported color choices and unchanged face/body/rig in source previews.
- [x] 2.2 Export six existing joined Bob body/outfit variants; verify non-hair preservation, rig/weight/material round trips and publish current-selector import paths.
- [x] 2.3 Main imports and exercises the bob in game; verify actual blonde silhouette, selection, saved appearance and ordinary movement for all body fits.

## 3. Modular parity and handoff

- [x] 3.1 Apply identical new hair to the corresponding six modular base variants; verify complete permanent coverage, rig and material contract while ponytail/garment hashes remain unchanged.
- [x] 3.2 Commit source/provenance/hash/import records and report exact source versus cooked acceptance status; verify private branch push and main-owned integration handoff without automatic promotion.

## Main integration
Main integration evidence: `docs/research/character-assets/ready-waves-01.md`.
The exact frozen six-wave set passed actual native import and a separate
fresh-process persisted-package verification. This justifies source/export
task1.2, not the still-pending cooked visual acceptance in1.3 or finish in1.1.
The default Preferred/tunic selection now also has a real308-frame ordinary
Shipping route with actual runtime mesh/material identities. The five other
variants lack gameplay selector coverage and the sawtoothed wave end remains
unaccepted;1.3/1.1 stay open. The coordinator selected the rough wave candidate
as an accessible playtest increment, not final art approval; prior preview is
retained for rollback (selection checkpoint4ed4050).

## Source-lane history (superseded by final published handoff)
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

Coordinator-lane publication is now persistent: updated waves `79c3e79`,
remaining canonical stock-bob/modular bundle `a37e24d`, and archived-reference
Windows line-ending protection `3a75556` are privately pushed. The source
author's earlier "no commit/push" statements describe that subagent's boundary,
not the final published handoff. All 18 canonical source FBXs and receipts pass
the final checker; original modular clothing remains unchanged. This completes
3.2, not the open wave-quality or in-game acceptance tasks.

## September22 selected-content runtime review

Current Content and the selected `wardrobe-complete-10` cooked containers already
use the final canonical bundle: `final-bundle.json` SHA256
`68E70D4F6C39495756321BDE18FA0C6F9B22C10CC777E540E3760DA90FBCAFA5`.
Its exact three modular wave and three modular Bob FBX hashes match the persisted
wardrobe import receipt and current uasset identities. No reimport was needed.

Task1.1 remains open. Actual close production-path back/three-quarter images for
Preferred, Willow and Hazel show the latest wave lower edge as a conspicuous
repeated triangular/sawtooth shelf around the shoulder-blade/mid-back region.
The ordinary all-body routes pass movement and gather, but foliage-obscured
frames do not improve that visual result. This is not a natural feather-tip
acceptance. Task1.3 therefore also remains open and no hairstyle candidate is
promoted over `wardrobe-complete-10`.

Task2.3 is complete from the combined exact evidence:

- `hair-review-03-native` passes the real production presentation for Bob with
  Blonde color index4 on all three modular bodies, close back/side/three-quarter
  captures, actual F5 save, Ponytail mutation and F9 restoration with equipment,
  IDs and dye retained.
- `hair-review-01-bob-blonde-body0/1/2` each pass the ordinary mapped
  walk/turn/orbit/portrait/gather route using the exact Bob base path and two
  starter garments.
- The final Shipping full loop exercises the same Bob geometry through real
  gathering, watering, weeding, clearing, save/reload and recovery, but uses
  its saved non-Blonde tint. That distinction is disclosed rather than called
  all-action Blonde evidence.
- The dedicated Bob/Blonde watering attempt is preserved as incomplete:
  it gathered real setup stock but stopped during setup, with
  `Watered real planted plot=0`. It is not counted as a pass.

The Bob reads as a short stock-based straight silhouette with layered strand
contrast and a dark-golden Blonde tint. This is technical/runtime completion,
not Jenny's aesthetic approval. OpenSpec progress is6/8.
