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

- Use new scripts and `Assets\Characters\HairstyleRefinement` source/output
  namespace. Existing usable FBXs remain rollback inputs, not overwritten.
- Mid-back waves retain the upper crown/part and existing natural wave cadence.
  Cut/re-author lower card topology and tapered ends near the shoulder/waist
  midpoint; do not squeeze all existing waves into a shorter vertical interval.
  Prove a real lower-bound change plus absence of the rejected ridged shelf.
- Bob uses the admitted hair/rig basis or original authored cards for a straighter
  chin-to-nape silhouette. Neutral light strands permit a blonde presentation
  while retaining explicit brunette/black/copper choices. Any color-catalog
  addition or style-specific default is coordinated with UI before coding.
- Early delivery replaces only hair surfaces inside the original joined FBXs,
  preserves all non-hair mesh/material/weight data and maps to their current
  Unreal object paths. Ponytail exports need not change.
- Apply the same source hair to corresponding modular bases without touching
  tunic/apron/shoes/wraps. Main owns import/build and ordinary gameplay capture.

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
