# Design

## Context

See `proposal.md` for motivation. The local creek is generated as part of each 24 m procedural terrain chunk. Its colliding terrain already contains a shallow channel and already blends the credited Brown Mud Leaves and Grass Ground materials through vertex red weights. Separate noncolliding water and bank mesh sections are then laid over that terrain; the two bank sections use one flat tan material from the water edge to 175 cm on either side, creating the canal appearance.

The simulation independently defines stream proximity from the legacy stream center and a 180 cm interaction radius. Generated resources, saves, and water transactions do not depend on the decorative bank ribbons. Cover is already deterministic and HISM-batched, but ordinary cover is currently excluded within 215 cm of the stream.

Focused reuse research therefore selects the existing terrain blend, procedural mesh sections, Brown Mud Leaves and Grass Ground CC0 textures, existing grass/fern batches, admitted moss-rock geometry, reeds, stable chunk seeding, and current reservation checks. No new asset, plugin, shader framework, or network dependency is justified. The only custom gap is deterministic creek-specific shaping and placement logic.

## Goals / Non-Goals

**Goals:**

- Replace the tan overlay with a damp forest-floor transition visible in the real generated world.
- Add bounded, globally deterministic edge variation that is continuous across chunk seams.
- Reuse existing batched cover to soften the banks without creating collision or hiding water access.
- Preserve simulation authority, terrain collision, current saves, resource identity, and the selected world's broader material language.

**Non-Goals:**

- Replacing the legacy stream center or interaction radius.
- Expanding regional hydrology, rendering every regional reach/lake, adding erosion, flow simulation, swimming, waterfalls, or a new water shader.
- Acquiring a sand, mud, shoreline, or vegetation pack.
- Changing terrain collision to follow decorative edge noise.

## Decisions

### 1. Remove the dedicated tan bank sections and use the existing terrain blend

The first playable slice will stop creating the two flat-color bank overlay sections. The colliding terrain beneath them already has a stream depression and a Brown Mud Leaves-dominant vertex blend near the water, so removing the overlays immediately exposes a darker, authored PBR creek bed without changing collision.

The terrain's existing global material-weight function will be refined with bounded global-position variation so the mud-to-grass transition is less uniform. Variation will be evaluated from world coordinates and stream-relative distance, not local chunk indices, which keeps shared vertices identical.

**Alternative considered:** author a new creek-bank material or download a sand/mud asset. Rejected because the admitted forest-floor materials already fit the requested woodland creek, and the observed problem is the flat tan override rather than missing source texture quality.

### 2. Vary only presentation water edges, not stream authority or terrain collision

The noncolliding water ribbon will retain its existing center and height sampling. Its left and right half-widths will receive separately bounded, low-frequency deterministic offsets derived from global longitudinal coordinates. Width remains inside conservative limits around the current 52 cm half-width, and both chunk sides evaluate the same shared boundary row.

This breaks parallel edges without affecting `IsNearWater`, refill positions, terrain collision, saves, or resource keys.

**Alternative considered:** move the stream center or deform the colliding channel to match every visual variation. Rejected because it would expand the change into simulation compatibility and terrain-authority work without being needed to solve the canal-like material presentation.

### 3. Reuse existing cover batches for restrained bank dressing

The active-chunk cover rebuild will admit a small deterministic subset of existing noncolliding grass and fern instances in a creek-relative transition band while retaining a clear inner water margin. Optional small moss-rock instances may be reused only when their existing batch and no-collision policy are available. Existing resource/plot/structure/home reservation checks remain mandatory, and generated reeds retain their authoritative lifecycle.

The bank subset will use the current per-chunk random stream plus global-distance acceptance, never per-instance actors. Density and scale will be tuned from ordinary gameplay views rather than maximizing coverage.

**Alternative considered:** create a dedicated shoreline actor/system. Rejected because it duplicates active-window lifecycle and batching already present in `AHomesteadWorld`.

### 4. Separate the first playable correction from full acceptance

The first playable demonstration consists of removing the tan bank sections, refining the material-weight transition, and confirming unchanged refill/collision in one ordinary creek approach. Full acceptance adds edge variation, restrained dressing, seam traversal, representative lighting views, source/runtime checks, and regression evidence in an immutable Shipping candidate.

The active work-animation round owns current shared UBT, Editor, package, and promotion resources. Creek source work can begin only after that milestone relinquishes shared engine execution, or in an independent lane with explicit ownership; integrated packaging and promotion remain serialized.

## Risks / Trade-offs

- **[Underlying terrain blend still looks too uniform after removing the overlay]** → Tune only the existing vertex-weight function first; add a small custom material input only if ordinary in-game evidence proves the existing blend cannot achieve the requirement.
- **[Visual water edge no longer perfectly matches the colliding channel depression]** → Keep width offsets modest and entirely inside the broad shallow channel; inspect from bank level and reject offsets that expose implausible shelves.
- **[Cross-chunk seams appear from local random input]** → Calculate edge and material variation from global coordinates and add an exact shared-boundary test.
- **[Bank cover blocks the view or reads as a repeated border]** → Keep it noncolliding, sparse, multi-scale, and seeded through existing batches; compare upstream/downstream and perpendicular views.
- **[Additional cover worsens transition hitch or frame cadence]** → Reuse existing HISM components, cap attempts per active chunk, and compare against the selected woodland cadence/transition evidence before promotion.
- **[Regional reach presentation remains pale or path-like]** → Disclose it as unchanged; this change targets the local playable creek and does not silently claim full regional-water naturalization.

## Migration Plan

1. Record ordinary baseline views and current refill/collision behavior at a stable creek location.
2. Land the smallest playable material correction and targeted deterministic/source tests.
3. Add edge variation and batched bank dressing, then verify seams, reservations, refill, collision, and active-window cleanup.
4. Build one immutable Shipping candidate and compare representative ordinary views and runtime evidence to the selected build.
5. Promote only if the creek clearly loses the sandy-canal read without gameplay or material performance regressions. Rollback is source reversion plus retention of the currently selected packaged build; no save migration is required.
