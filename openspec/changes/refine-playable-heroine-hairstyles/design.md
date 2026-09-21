# Design

## Context

See proposal.md. Inspection of `docs\character-pipeline.md` confirms
`hair-length-01` was rejected despite reaching 111.05 cm: vertical compression
bunched waves into ridges. The source retains the original long01/bob01/ponytail01
CC0 meshes on three adult bodies, with all hair bound to the head. Current
Character loads eighteen joined meshes at stable existing paths. New modular
bases are not yet imported, so they cannot be the only delivery vehicle.

The prior supplied character reference is private and not opened in this lane.
No new bob image has been available here. Written direction governs an original
straight blonde bob, not a claimed reconstruction or approval of the current
angled eye-covering bob.

## Goals / Non-Goals

**Goals:** Make the requested hairstyles selectable in the earliest playable
candidate without waiting for inventory/wardrobe; preserve face/body/rig and
non-hair geometry exactly in that early joined set.

**Non-Goals:** New character pipeline, purchased assets, hair physics, face
redesign, additional hairstyle IDs, copying Hades II art, or a screenshot-only
showcase.

## Decisions

- Reuse admitted assets first; custom-author only essential gaps. The bounded
  source review below governs remaining shaping, without restarting a broad
  asset survey or redoing completed garment work.
- Use new scripts and `Assets\Characters\HairstyleRefinement` source/output
  namespace. Existing usable FBXs remain rollback inputs, not overwritten.
- Mid-back waves retain the upper crown/part and existing natural wave cadence.
  Cut/re-author lower card topology and tapered ends near the shoulder/waist
  midpoint; do not squeeze all existing waves into a shorter vertical interval.
  Prove a real lower-bound change plus absence of the rejected ridged shelf.
- For the next substantial bob shape pass, prefer adapting the fitted admitted
  bob01 cap, layered cards and UVs: trim/redirect the diagonal face-covering
  fringe and straighten only the panels that require it. Whole custom cards
  are a fallback for a demonstrated local quality gap, not the default.
  Neutral light strands permit a blonde presentation
  while retaining explicit brunette/black/copper choices. Any color-catalog
  addition or style-specific default is coordinated with UI before coding.
- Early delivery replaces only hair surfaces inside the original joined FBXs,
  preserves all non-hair mesh/material/weight data and maps to their current
  Unreal object paths. Ponytail exports need not change.
- Apply the same source hair to corresponding modular bases without touching
  tunic/apron/shoes/wraps. Main owns import/build and ordinary gameplay capture.

## Bounded admitted-source review and actionable reuse choice

Jenny's explicit reuse-first preference was relayed after the source candidates
and modular parity had been delivered. This review is limited to the assets
already admitted in this worktree, not a new acquisition or market survey.

Evidence: `Assets\Characters\provenance.json` admits MakeHuman system hair
long01/bob01 and MPFB graphical output as CC0-1.0; `Variants\provenance.json`
adds ponytail01. Existing license evidence is retained under
`Assets\Characters\Source\Licenses`. The raw `SystemAssets` pack/catalog is
not present in this isolated checkout, so no additional stock styles are
represented as inspected or available. The actual fitted meshes remain usable
in `Variants\Heroine_Variants.blend`, both body-preset blends and their joined
FBXs; no download, other checkout or private reference is needed.

The existing offline source inventory confirms the same 53-bone rig and
head-only hair groups on all three body fits:

| Admitted option | Existing reusable basis | Concrete gap and next action |
|---|---|---|
| long01 | Fitted crown, existing wave cadence, card topology and strand/alpha atlas; 3,239 source vertices / 2,054 faces per body | Stock/project-fitted tips are around 89 cm, not mid-back. Keep the verified upper source; refine only new lower topology/feathered ends. Vertical compression is rejected. |
| bob01 | Already fitted short cap, layered cards, existing UV/alpha strands and head weights; 5,203 source vertices / 4,237 faces per body, roughly 138–162 cm high | The diagonal face-covering fringe is explicitly not approved. Prefer adapting those offending front/lower panels while retaining the usable cap and strand layout. Reuse its admitted strand atlas for neutral/blonde treatment where suitable. |
| ponytail01 | Previously admitted and fitted playable choice with the same rig | Wrong requested silhouette and explicitly protected. Leave unchanged; not a donor that needs rebuilding. |

**Custom-work justification:** long01 lacks a natural mid-back termination, so
new lower-tip topology/UV ending is an essential length/quality gap. There is
no license, fit or rig gap that justifies replacing its crown. For bob01, the
concrete gap is the diagonal fringe/straight silhouette, not legal availability
or compatibility. The already-exported parametric blunt-fringe bob is a usable
interchange fallback, but the review has **not** established that stock bob01's
cap or all of its cards are unusable; further whole-bob construction is therefore
not the preferred route. Local replacement cards/strand texture are justified
only if the adapted stock panels cannot resolve a specific remaining silhouette,
alpha or blonde-readability defect, with that defect recorded.

**Next bounded shaping sequence:** continue the open long01 end-finish task
first; for any further bob shaping, test a targeted bob01 adaptation against the
existing candidate in CPU front/back/three-quarter views. Do not promote either
on export success alone. Reuse existing joined export, neutral palette, rig and
modular-transfer machinery; refresh only affected hair outputs and their
receipts. Keep the current 12 joined + 6 modular candidate FBXs available while
doing so. No garment redo, purchase/account, private upload, broad research,
new planning round or source-lane Unreal execution is required.

## Risks / Trade-offs

### Native integration of the frozen wave set

Main adopted the exact approved `013c70b/a3a6dee/79c3e79` chain. The final
manifest is `cb6e61cd7624cc5535053ba2d503d3655a064bfa48d936aa61fed6bc8880cac8`;
the existing independent six-FBX/texture/protected-input checker passed.
No source sculpting, Blender execution or new acquisition is needed.

Import into fresh `/Game/Trials/HeroineWave_20260921_01` packages rather than
blindly reimporting accepted meshes: six joined skeletal meshes, one neutral
texture and one skeletal masked/two-sided material. Reuse explicit legacy
UFbxFactory options from the existing character importer, the original shared
skeleton and unchanged nonhair material interfaces. Disable automatic material,
texture, animation and physics-asset creation. Require exact slots/counts,
measured units/bounds and unchanged reference bone names/parents/transforms;
save only the eight named packages, then verify them in a fresh process.
Keep all incumbent packages and already-selected Shipping builds intact.

After the actual import and fresh-process reload passed, the next bounded
feature slice is a separate `hair-cook-01` / `hair-waves-01` Shipping candidate.
Use the same installed standard Windows Cook with `SkipZenStore`, verify
all eight new cooked packages in addition to the accepted fern outputs, and
stage fresh loose content with the genuine new Shipping executable/manifest.
Exercise the existing ordinary-control route before changing selection.
UI integration waits until its modular dependencies exist: its menu-only
smoke can pass without proving normal current-save resume, which is required
for the later coherent wardrobe/menu candidate.

The normal candidate's existing LongWave ID/body/outfit mapping will resolve
the new trial meshes directly; no QA-only hairstyle override or new preset.
Record actual runtime mesh/material identities with ordinary back/side/action
views. Existing Bob, Ponytail, skeleton, motion and nonhair surfaces remain
unchanged. The source's chunky cut-edge caveat is still open.

Reuse the proven native commandlet/supervisor, disabled Python/workers, exact
local cache, inherited marker lifetime, root-only job and endpoint policy.
The coordinator explicitly rejected inheriting the fern's arbitrary short
import timer for six unmeasured skeletal meshes. Import/verification reuse
the existing completion-driven guard primitive (its legacy profile token is
`CookCompletionDriven`; this does not invoke or authorize Cook). Live run
pause/stop, native stop polling and owned policy-failure cancellation remain.
There is no synthetic expired deadline. Synchronous mesh
compilation/serialization are cancellable only at actual polling boundaries.
Retain failed evidence/partials, never modify the previous playable candidate.

- Cut alpha cards expose a blunt edge -> taper strand ends and inspect back and
  three-quarter views at gameplay distance, not length numbers alone.
- Blonde tint washes out detail -> retain textured strand contrast and test
  existing hair-color selections without recoloring skin/eyes/clothing.
- Import-ready is mistaken for playable -> label every source receipt pending
  until the current selector in a cooked candidate actually displays it.

## Migration Plan

Keep hairstyle indices 0 LongWave, 1 Bob, 2 Ponytail. Hand off a manifest from
new source FBXs to existing cooked object paths, with content hashes. Main
imports the earliest verified wave set, then bob, checks supported bodies and
ordinary walking/actions, and promotes only a usable candidate. Existing source
files remain available for rollback; no source-lane UE processes are allowed.

## Applied source contract

The early drop lives in `Assets\Characters\HairstyleRefinement\Joined`, with
per-style manifest mappings to all twelve existing selector object paths.
Sources are the actual joined FBXs: replacing only their hair surfaces avoids
re-running the legacy exporter body-deletion step. Original source trees,
ponytails and animations are hash-protected. Upper wave coordinates at/above
120 cm remain exact; clipping removes old lower waves and produces staggered
pointed ends around 111–112 cm rather than squeezing their cadence.
The newly cut lower band remaps hair UVs to the admitted atlas's feathered
alpha tips; nonhair UVs and the original alpha image are unchanged.

Palette addition was explicitly approved: `HomesteadLook::HairColorCount=5`,
indices 0–3 unchanged and Blonde=4; the default remains Chestnut. Selecting Bob
does not change color. Hair slot 7 uses new `_Neutral` material names and
`NeutralHairTint`; legacy ponytail/brows use the retained `HairTint` semantics.
Neutral wave texture preserves admitted alpha byte-for-byte. Bob geometry and
strand texture are original, using written direction only. No private image or
game-art source was accessed.

Source receipts cover exact pre-export nonhair surfaces/UVs/weights, unchanged
nonhair corner normals, 53-bone bind topology, and float-tolerant FBX round trips.
CPU 8-sample front/back/three-quarter previews are diagnostic, not gameplay.
The bob is a simple stylized straight silhouette with rounded crown and short
fringe, not a claim of reference approval. Main still owns 1.3/2.3, aesthetic
acceptance and source commit/push in 3.2. Modular parity 3.1 requires main's
explicit stable-base handoff and must not touch main's modular outputs.

The stable-base read-only handoff arrived at 23:53 Arizona on 2026-09-20.
The modular parity recipe now transfers the actual joined hair to six separate
outputs in `HairstyleRefinement\Modular`; it never overwrites the stable input.
Hair occupies modular slot 3 (joined slot 7), with nine slots retaining separate
permanent BaseBra/BaseBriefs. Full source hashes protect ponytails and garments;
nonhair surfaces, coverage counts, weights, normals, shader values/links and
texture hashes are compared before/after export. This parity does not close
the unresolved wave visual-finish or coordinator-owned gameplay requirements.

## Frozen wave handoff and isolated bob reuse result

The parent requested waves-only publication and a freeze of wave source assets
and Appearance.h/.cpp. The read-only `check_hairstyle_waves.py` verifies that
standalone drop without requiring Bob or modular outputs. Those files are not
mutated by subsequent bob work.

The bounded reuse-first bob pass is now separately exported under
`Assets\Characters\HairstyleRefinement\BobReuse`: six joined Bob FBXs and three
matching modular Bob bases, with `.blend` snapshots, materials and provenance.
It adapts admitted fitted bob01 instead of rebuilding its cap. Only the
diagonal central face-covering fringe and the lowest chin/nape ends are clipped;
original crown coordinates at/above 155.5 cm remain exact (5,815 Preferred,
5,911 Willow, 6,057 Hazel vertices). Existing layered cards/UVs and admitted
strand alpha are retained. Preferred CPU front review shows an uncovered face
and the original layered side-part appearance rather than the earlier custom
cap. This is a source-candidate assessment, not Jenny's approval.

All nine real FBXs passed nonhair/material/rig round trips and modular coverage
checks. The separate manifest confirms 24 frozen wave/Appearance files remained
hash-identical. New material `M_Heroine_Hair_bob01_Neutral` keeps the same exact
runtime contract but uses the BobReuse neutralized stock texture; do not import
both candidate material records indiscriminately. The earlier parametric bob is
retained as fallback, while BobReuse is the reuse-first candidate for the next
coordinator-owned Bob import. No garment or frozen wave re-authoring occurred.

Publication clarification: parent published wave checkpoint a3a6dee and palette
checkpoint 013c70b. The later frozen working-copy wave FBXs/manifest differ from
a3a6dee; local modular-wave parity follows those later donor hashes. It is not
published-checkpoint parity and must not be promoted as such. Published neutral
texture bytes and palette source text remain unchanged. The checker now offers
`--published-commit a3a6dee` to reject accidental substitution, and the general
checker is read-only unless metadata/preview refresh is explicitly requested.
Parent-owned EARLY-WAVES.md and incumbent-neutral-tint.patch are preserved.

The final technical bundle resolves published-wave modular parity separately:
`PublishedWaveParity` derives three modular wave bases from SHA-verified local
Git LFS copies of a3a6dee. It performs no shaping and changes no frozen input.
`final-bundle.json` selects these three bases plus BobReuse's six joined and
three modular Bob outputs. Earlier root late-wave/parametric-bob candidates
are explicitly not selected. `publication-audit.json` records the six root
wave FBX differences, source-blend differences and manifest difference against
a3a6dee; shared material/texture calibration and palette source remain intact.
Main should review at the normal game camera and retain the original baseline
if worse, rather than treat technical parity as visual completion.

Final parent instruction supersedes the intermediate selection: include the
latest local feather-tip waves in one explicitly new source commit, plus stock
Bob and matching modular parity. The finalizer transfers validated BobReuse
hair to the six canonical Joined Bob and three canonical Modular Bob paths
without further shaping; canonical wave files and wave donor rows are retained.
`final-bundle.json` therefore now selects all 12 canonical Joined + 6 canonical
Modular outputs. PublishedWaveParity remains an immutable-a3a6dee reference
alternative only. The publication audit preserves the exact old/new wave
distinction; no runtime or natural-end acceptance is implied.
