# Spec Delta

## Purpose

Present a coherent natural woodland clearing while preserving the readable,
controller-first homesteading world and its existing resource and building rules.

## ADDED Requirements

### Requirement: Coherent woodland presentation

The environment SHALL replace dominant decorative primitive tree and grass
silhouettes with the admitted natural asset bundle while preserving the warm
clearing, open foreground, surrounding woodland, meadow and stream composition.
Imported vegetation SHALL retain distinct bark and leaf surface appearance,
credible proportions and grounded roots rather than uniform prototype tinting.

#### Scenario: Ordinary clearing view
- **WHEN** the player enters the clearing in a normally lit daytime scene
- **THEN** the enclosing trees and nearby understory use the admitted natural
  assets, their bark and leaves remain visibly distinct, and the home site and
  approach to the stream remain readable

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
