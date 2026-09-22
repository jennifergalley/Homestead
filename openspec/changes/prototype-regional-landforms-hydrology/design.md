# Design

## Context

See `proposal.md` for motivation and
`specs/regional-landforms-hydrology/spec.md` for behavior. Version-three woodland
owns 24 m chunks, local rolling terrain and a deliberately limited winding
stream. It remains frozen while main integrates it.

Reuse the portable C++17 conventions already proven by that work: explicit
seed/version descriptors, integer hashes, global signed coordinates, floor
ownership, immutable results and failure-without-output-mutation. No suitable
existing project or engine facility supplies deterministic cross-region
watershed topology. No third-party code or assets are needed for the prototype,
so it has no new license, access, account or redistribution constraints.

This design corrects an early unit error: forty 24 m chunks form a **960 m**
region, not a 96 km region.

## Goals / Non-Goals

**Goals:**

- Produce immutable regional relief, drainage, lake and habitat descriptors from
  a seed, version and signed global coordinates.
- Make shared nodes and border outlets identical regardless of queried region or
  visit order.
- Prove connected, non-uphill flow and deterministic spill resolution with
  bounded work.
- Keep a later adapter possible between regional potential and the existing
  100 cm woodland terrain without coupling this prototype to that generator.

**Non-Goals:**

- Editing version-three generation, saves, runtime streaming or current assets.
- Water meshes, collision, swimming, erosion, sediment, flooding, seasonal flow,
  final terrain blending, final biome selection or a playable candidate.
- Pretending every arbitrary depression can be solved with unbounded global
  search.

## Decisions

### Separate immutable prototype API

Add only `Simulation/HomesteadRegionalGeneration.h/.cpp` and
`Tests/HomesteadRegionalGenerationTests.cpp`. The namespace has its own
`RegionalDescriptor`, `RegionCoord`, status enum and result types. No current
header includes it.

**Alternative:** extend `HomesteadWorldGeneration`. Rejected because main is
integrating and visually validating version three; changing that contract would
couple speculative regional work to the active candidate.

### 960 m regions on a 30 m global drainage lattice

A region covers 40 existing 24 m chunks and contains 32 by 32 owned drainage
nodes. Nodes use signed global lattice coordinates; regions use floor division
and half-open bounds. Every node identity is its global lattice coordinate.
Immediate downstream selection reads the eight neighboring global nodes, which
is equivalent to a one-cell halo without storing duplicated identities.

The fixed result contains at most 1,024 owned nodes plus reaches and lake
references derived from them. This is coarse future authority, not render
geometry.

**Alternative:** a much larger region or terrain-resolution drainage. Rejected
because either makes queries expensive or confuses topology with rendering.

### Global integer relief potential

Sample deterministic fixed-point value noise in global coordinates at regional
and multi-region scales. Combine broad uplift, directional ridge and valley
potential into an integer elevation potential. Expose normalized mountain,
ridge, valley, wetness and lake-shore hooks; later biome code can consume them
without changing drainage identity.

The prototype does not blend this potential into current terrain. Main later
chooses amplitudes, traversal shaping and the relationship to version-three
local relief.

### Downstream graph from potential, not per-region river noise

Each node chooses the strictly lowest of its eight neighbors. Equal elevations
use canonical global coordinate ordering. A direct lower neighbor creates one
downstream edge. This makes ordinary reaches globally reproducible and prevents
loops.

Flow accumulation is bounded: trace upstream contributors within a fixed
eight-node radius and saturate at an explicit maximum class. Width and depth
classes derive monotonically from that contribution. This is enough to validate
tributary behavior; exact whole-continent discharge is deferred.

**Alternative:** draw rivers from independent noise masks. Rejected because
neighboring chunks could disagree and paths would not establish drainage.

### Bounded deterministic depression spill search

If a node has no lower neighbor, run a priority spill search over global nodes.
The queue orders by candidate spill elevation and then canonical coordinate.
Expand the connected depression until reaching a node lower than the current
spill saddle. All members at or below the saddle share one lake ID derived from
the canonical minimum member and one water-surface elevation; the lowest ordered
spill edge is the outlet.

Search is capped at 4,096 visited nodes. Exceeding the cap returns
`BasinTooLarge` without changing output. Region generation memoizes lake results
within that call, so repeated members do not repeat the search. This is bounded
and order-independent while honestly deferring continental basins.

**Alternative:** priority-fill only the region and halo. Rejected because a
basin crossing the halo would receive a different outlet from each side.

### Canonical border and reach identities

An outlet is an ordered pair of adjacent global node identities. Both regions
therefore describe the same crossing. A reach key is its upstream node plus
downstream node; lake keys use the canonical basin minimum. No vector index,
active region or visit order enters identity.

Water-surface endpoints are base elevation for ordinary reaches and spill
elevation for lake members. A reach is emitted only when the downstream
water-surface value is lower, or when leaving a same-surface lake through its
canonical outlet.

### Staged integration

The prototype handoff contains OpenSpec artifacts, portable source and focused
tests. Main later decides whether to adopt the API, how to blend regional
potential into local terrain, and how to render and collide water. The first
playable stage is one valley river continuous through neighboring woodland
regions; lakes and mountains need their own visual and traversal acceptance.

## Risks / Trade-offs

- **[Coarse 30 m topology misses small creeks]** -> Keep current local stream
  until a later adapter derives minor channels beneath regional reaches.
- **[Bounded accumulation understates enormous watersheds]** -> Expose saturated
  classes and defer physical discharge simulation.
- **[A basin may exceed 4,096 nodes]** -> Return `BasinTooLarge` explicitly;
  later introduce hierarchical drainage domains if real seeds reproduce this.
- **[Potential-derived ridges may not look geologically eroded]** -> Treat output
  as placement authority and blend input, not finished landform geometry.
- **[Region generation may repeat work]** -> Caller may cache immutable region
  results; correctness never depends on cache presence.
- **[Future blending could violate downhill rendered water]** -> Runtime
  integration must preserve resolved water surfaces or reject the blend during
  native validation.

## Migration Plan

There is no runtime or save migration in this prototype. It lands as unused new
files. Adoption later requires a new explicit regional generation version and a
fresh test world; rollback removes the adapter while leaving version-three
woodland unchanged.
