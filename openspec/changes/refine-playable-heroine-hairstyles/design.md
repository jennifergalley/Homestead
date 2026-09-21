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
