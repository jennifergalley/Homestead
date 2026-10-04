# Design

## Context

See proposal.md. Seven planting identities already exist; Item::Seeds maps only to CropKind::Roots. Inventory stock counts capacity per unit, while layout reconciliation currently merges gains into a group. Saves validate stock/layout partitions and hotbar/pack-square references.

## Goals / Non-Goals

Make seed packets truly non-stackable through all inventory paths without new crop/item enums. No seed-count loss, capacity redesign, version bump or placement changes.

## Decisions

Centralize seed-packet classification beside the item catalogue. Reconciliation allocates one group per packet, preserves referenced first group ids and appends additional groups deterministically; merge/swap/drop paths respect nonstackability. Normalize valid old layouts only after reading and validating their original references, then revalidate before commit. Existing unit-based capacity means expansion fits the same capacity rather than increasing it.

Use existing CropInfo names and weed predicates for a native crop action cue. Harvest is a permanent affordance, not tutorial guidance; its named verb bypasses retirement. E/A remains interaction only, and a visible weed requirement blocks harvest consistently.

## Lanes and ownership

Gameplay UI owns catalogue rules, simulation/layout/save normalization, naming and focus/input. Fishing Art owns icon assets/mapping for roots-seeds, turnip-seeds, carrot-seeds, potato-seeds, cabbage-seeds, broad-bean-seeds and strawberry-seeds. Environment's plot queries stay unchanged.

## Risks / Trade-offs

Old seed stacks referenced by hotbar/pack squares -> preserve the existing id for one packet and place remaining packets in unclaimed cells. Malformed saves -> validate before normalization and leave the live simulation untouched. Store/harvest multiplicity -> allocate distinct packets through shared reconciliation rather than caller-specific logic.

## Migration Plan

Same save version and fields, more quantity-one layout groups. Load normalization does not write actual player save files; report the compatibility effect before ready. Focused native roundtrip/old-layout/merge/stock tests precede admission.
