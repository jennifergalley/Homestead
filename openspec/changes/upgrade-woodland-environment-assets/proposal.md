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
- Following Jenny's September21 rejection of the sparse visual result, compose
  a lush surrounding woodland and replace primitive interactive resource
  silhouettes as one coherent visible increment. Denser decoration alone is
  insufficient while gatherable bushes, sticks, flowers and saplings remain proxies.
- Preserve imported bark/leaf/PBR material slots and authored proportions rather
  than routing natural assets through flat prototype tint overrides.
- Complement the existing CC0 forest-floor and moss-rock assets with restrained
  ground layering; leave analytic height and collision topology unchanged.
- Improve sky depth only within the existing dynamic sun/moon/atmosphere/
  skylight/fog ownership; evaluate bounded engine-native clouds rather than a
  second lighting rig or static daytime HDRI.
- Establish acquisition receipts, representative import gates, matched visual
  reviews and measurable regression checks before any candidate promotion.
- Diagnose the player-observed multi-second terrain-generation hitch with
  main-thread and preparation-phase measurements, then materially reduce it
  through bounded asynchronous preparation and current caches when supported;
  never move full regional generation onto the game thread or hide an
  unresolved architectural limit.

The exact recommended bundle, fallback, current publisher evidence and unknowns
are maintained in `docs\research\environment-assets\2026-09-20-decision.md`.
Implementation is authorized and the representative fern import/render workflow
has passed. Put the existing fern into the actual clearing first, then add
tree/grass/ground improvements. Full palette completion and exhaustive
performance work do not block the first usable candidate. Carry proven tool
controls forward; new helper/network/security requirements remain boundaries.

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
The final polish round also affects generated-world preparation timing and
instrumentation directly adjacent to `AHomesteadWorld::Refresh`, while
preserving regional descriptor authority and stable generated identities.
It preserves the accepted offline-startup/fullscreen behavior. Old saves are
disposable test data under current user policy: report necessary resets rather
than delaying features for migration. The latest basically verified usable
candidate may become the normal preview, retaining cheap rollback.

Deliberate palette, ground coverage and lighting refinement is now in scope,
judged in actual daylight rather than inferred from warm dawn. The existing
time/weather owner remains authoritative. No terrain replacement, new biome/
actor framework, character/animation/tools, building-piece packs, audio changes,
gameplay rules, recipe progression, firewall changes, purchases, automation or PR
is included.
