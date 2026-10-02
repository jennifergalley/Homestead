# woodland-environment-presentation Specification

## Purpose
Present a coherent natural woodland clearing while preserving the readable,
controller-first homesteading world and its existing resource and building rules.

## Requirements

### Requirement: Coherent woodland presentation

The environment SHALL replace dominant decorative primitive tree and grass
silhouettes with the admitted natural asset bundle while preserving the warm
clearing, open foreground, surrounding woodland, meadow and stream composition.
Imported vegetation SHALL retain distinct bark and leaf surface appearance,
credible proportions and grounded roots rather than uniform prototype tinting.
The playable clearing SHALL read as a lush enclosed woodland with layered
understory, rather than isolated showcase trees on an exposed primitive field.
Existing gatherable bushes, saplings, branches and low plants SHALL receive
authored natural silhouettes; decorative density alone does not satisfy this.

#### Scenario: Ordinary clearing view
- **WHEN** the player enters the clearing in a normally lit daytime scene
- **THEN** the enclosing trees and nearby understory use the admitted natural
  assets, their bark and leaves remain visibly distinct, and the home site and
  approach to the stream remain readable

#### Scenario: Coherent woodland and resource palette
- **WHEN** actual wide, shoulder-level and ground-resource daytime views are reviewed
- **THEN** overlapping canopy groups enclose usable open space, natural ground
  layers connect them, and nearby interactive resources no longer retain the
  dominant cone, blob-bush or bare stick-figure presentation
- **AND** the visual judgment is recorded separately from asset counts and test passes

#### Scenario: Material or asset unavailable
- **WHEN** a required selected environment mesh or material cannot be loaded
- **THEN** the runtime logs the missing asset and explicitly identifies any
  provisional fallback rather than presenting it as a successful visual upgrade

### Requirement: Traversal and construction remain authoritative

Decoration SHALL preserve the existing terrain heights and collision surface,
accessible routes, resource interaction positions and construction exclusions.
Decorative understory SHALL NOT block movement. Non-removable decorative trees
SHALL NOT occupy the protected home site, cleared resource sites, placed
structures, farm plots or required access routes.

#### Scenario: Walk and build after clearing
- **WHEN** a player clears a sapling, saves, reloads and builds at a valid site
- **THEN** the sapling remains gone, no replacement decorative blocker appears,
  the same construction rules apply and the player can use the resulting doorway

#### Scenario: Decoration refresh around new structures
- **WHEN** a valid plot or structure is added and decoration refreshes
- **THEN** nearby natural assets respect that occupied space without changing
  the structure, crop state or terrain collision

### Requirement: Resource state remains visually legible

Interactive plants SHALL remain distinguishable from decoration at gameplay
camera distances. Harvestable produce SHALL visibly disappear after harvesting
and reappear only when the existing simulation permits renewal; persistent plant
bases SHALL retain their existing lifecycle. Clearing SHALL remove the complete
resource visual and associated blocking collision without changing rewards,
cooldowns or save compatibility.

#### Scenario: Harvest and renewal across reload
- **WHEN** a ready berry bush or flower is harvested, saved, reloaded and later
  becomes ready through the existing passage-of-time or sleep rules
- **THEN** its depleted state and HUD agree before renewal, its produce reappears
  after renewal, and gathering awards the existing reward exactly once

#### Scenario: Decorative lookalike beside a resource
- **WHEN** the player approaches an interactive plant among natural understory
- **THEN** the target and ready/depleted state remain readable without requiring
  a new interaction rule or disguising decorative foliage as gatherable produce

### Requirement: Ground variation does not change terrain

Ground material variation SHALL complement the existing forest-floor and moss
rock appearance, preserve readable meadow and bank transitions, and leave terrain
topology, analytic placement height, stream position and physical collision
unchanged.

#### Scenario: Traverse a layered ground transition
- **WHEN** the player walks from the home site toward woodland and stream banks
- **THEN** surface variation has no new collision steps, floating vegetation,
  displacement-induced mismatch or visible material-boundary gaps

### Requirement: Sky remains coherent with time and weather

The environment SHALL retain a single coordinated day/night/weather lighting
system. Any added cloud presentation SHALL follow that system's clear/rain/night
state and SHALL have a reversible disabled fallback. A static daytime background
SHALL NOT remain luminous through night or override the current dynamic lights.

#### Scenario: Night and rain comparisons
- **WHEN** the same camera is reviewed at settled dawn, clear noon, rainy noon
  and night
- **THEN** vegetation and sky respond coherently to those states, nighttime is
  not replaced by daylight, and essential resource and route visibility remains

#### Scenario: Clouds disabled
- **WHEN** the cloud candidate fails its visual or performance gate and is disabled
- **THEN** the existing atmosphere, sun, moon, skylight and fog remain functional
  without a second lighting owner or a change to weather gameplay

### Requirement: Streamed woodland preparation avoids multi-second game-thread hitches

The generated woodland SHALL measure player-visible chunk preparation by
separating main-thread refresh work, reusable preparation work and asynchronous
regional descriptor work. Entering a new chunk SHALL NOT synchronously execute a
full regional `GenerateRegion`. A candidate SHALL materially reduce the
reproduced multi-second player-visible hitch when a bounded supported
preparation/cache change can do so, while preserving stable entity keys,
collision, clearing persistence, regional reach identity and save behavior.

#### Scenario: Baseline chunk transition is profiled
- **WHEN** ordinary mapped movement crosses into uncached generated chunks
- **THEN** evidence records the longest player-visible frame, main-thread
  preparation duration and relevant terrain/decoration/descriptor phases
- **AND** screenshot readback, startup and controlled teleports are identified
  separately rather than attributed to terrain generation

#### Scenario: Prepared chunk transition
- **WHEN** the player approaches and crosses a chunk boundary covered by the
  bounded preparation window
- **THEN** required immutable terrain/entity data is reused from current caches
  or prepared off the game thread before commit
- **AND** the game-thread commit does not call full regional generation or
  introduce invisible blockers, stale collision, duplicate entities or changed
  stable keys

#### Scenario: Architectural limit remains
- **WHEN** measurement shows the hitch cannot be materially reduced without a
  broader streaming or rendering architecture change
- **THEN** the candidate is not described as hitch-free
- **AND** the exact dominant phase, measured spikes and required follow-up scope
  are recorded without weakening the gameplay or persistence gates

### Requirement: One directional light owns forward shading
At any time of day, exactly one directional light SHALL be the designated owner of forward
shading, translucency, water, and volumetric fog lighting, chosen deterministically (sun by day,
moonlight by night or an equivalent rule). The engine MUST NOT report that multiple directional
lights are competing for that role, and day/night sky and lighting behavior SHALL otherwise remain
as today.

#### Scenario: Play through dawn
- **WHEN** a new woodland starts at dawn and the player plays in the editor or the packaged game
- **THEN** no "Multiple directional lights are competing" warning appears on screen or in the log

#### Scenario: Day to night transition
- **WHEN** game time passes from day into night
- **THEN** the forward-shading owner switches without a visible lighting pop and without the competing-light warning

### Requirement: Sun shadows move smoothly with the time of day
As game time advances, sun and moon shadows SHALL move continuously, with no visible jumps in shadow
position, and the movement MUST NOT cause frame-time spikes. Where hardware ray tracing is
unavailable, the game MAY fall back to stepped shadow updates that keep frame pacing smooth.

#### Scenario: Watching shadows at a low sun
- **WHEN** the player stands still in the woodland near dawn or dusk for a minute of real time
- **THEN** tree and heroine shadows drift smoothly rather than jumping every few seconds

#### Scenario: Frame pacing while the sun moves
- **WHEN** the 4K presentation route runs capped at 60 FPS with the sun moving
- **THEN** its p99 frame time is no worse than the stepped-sun baseline and no shadow update produces a stall

#### Scenario: Shade stays clean while moving
- **WHEN** the heroine walks through dappled canopy shade with the camera following
- **THEN** shadows on the ground and on her clothes don't crawl, sparkle or break into visible pixel noise, and static shade is no noisier than the Virtual Shadow Map look
