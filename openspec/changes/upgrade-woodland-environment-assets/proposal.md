# Proposal

## Why

Homestead's warm, peaceful clearing already supports the intended play loop, but
cone trees, spherical crowns and pointed grass still dominate its appearance.
A tightly curated free woodland asset set can substantially improve that view
without replacing the landscape or expanding gameplay.

## What Changes

- Replace dominant decorative tree and understory primitives with a cohesive
  licensed set, preserving open foreground, resource legibility, stream access,
  construction clearance and deterministic placement.
- Preserve imported bark/leaf/PBR material slots and authored proportions rather
  than routing natural assets through flat prototype tint overrides.
- Complement the existing CC0 forest-floor and moss-rock assets with restrained
  ground layering; leave analytic height and collision topology unchanged.
- Improve sky depth only within the existing dynamic sun/moon/atmosphere/
  skylight/fog ownership; evaluate bounded engine-native clouds rather than a
  second lighting rig or static daytime HDRI.
- Establish acquisition receipts, representative import gates, matched visual
  reviews and measurable regression checks before any candidate promotion.

The exact recommended bundle, fallback, current publisher evidence and unknowns
are maintained in `docs\research\environment-assets\2026-09-20-decision.md`.
This request creates planning artifacts only. Starting implementation requires a
new explicit request and an approved offline-safe editor/import workflow.

## Capabilities

### New Capabilities

- `woodland-environment-presentation`: Coherent natural vegetation, ground and
  dynamic sky presentation that preserves playable-world semantics.
- `environment-asset-provenance`: Zero-cost asset admission, reproducible
  acquisition/import records, truthful validation and reversible promotion.

### Modified Capabilities

None. The initialized OpenSpec root has no existing capability specifications.
Existing product and gameplay behavior remains authoritative and unchanged.

## Impact

Future implementation affects `HomesteadWorld.cpp/.h` asset loading, decoration
and material helpers; `Scripts\bootstrap_unreal.py`; environment assets and
materials; the asset manifest/receipts/credits; and relevant visual/renewal tests.
It must integrate the parent's accepted offline-startup/fullscreen work before
any engine activity and must not alter personal saves or the accepted preview.

No terrain replacement, new biome/actor framework, character/animation/tools,
building-piece packs, audio changes, gameplay rules, recipe progression,
firewall changes, purchases, automation or PR is included.
