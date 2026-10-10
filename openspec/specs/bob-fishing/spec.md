# bob-fishing Specification

## Purpose
Fishing played on a float on the water: random bites and strikes, a closing ring for the click, and most fish getting away.

## Requirements

### Requirement: Fishing is unpredictable and usually fails
A hooked fish SHALL get away about 60% of the time without reward, independent of player timing. The bite wait, the number of strikes and the wait before each strike SHALL vary randomly per cast. Clicking while the float is on the surface SHALL lose the fish.

#### Scenario: Many casts
- **WHEN** she plays many casts with perfect timing
- **THEN** roughly four in ten land a fish, and no two casts share a rhythm.

### Requirement: The float on the water is the cue
Fishing SHALL show no pop-up panel. A visible float SHALL ride the water; it SHALL sink when a click is needed, with rings collapsing onto it across the reaction window, and SHALL only shiver on false nibbles.

#### Scenario: Bite
- **WHEN** a fish bites
- **THEN** the float vanishes under the surface while rings close on its spot, and clicking before the rings close hooks the fish.
