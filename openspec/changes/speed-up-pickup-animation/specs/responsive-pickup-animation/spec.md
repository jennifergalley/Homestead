# Spec Delta

## Purpose

Makes frequent ground pickup and weeding presentation quick and readable while keeping all rewards, rules, and interruption behavior simulation-owned.

## ADDED Requirements

### Requirement: Pickup presentation completes promptly
A successful action using the shared pickup gesture SHALL complete its visible reach/pick/recover in no more than one second under normal playback. The motion SHALL retain a readable anticipation, contact moment, and return to locomotion without a prolonged crouch or low hold.

#### Scenario: Pick up a stone
- **WHEN** the player successfully gathers a focused stone resource
- **THEN** one complete readable pickup gesture starts and recovers within one second

### Requirement: Every current pickup caller uses the responsive timing
Successful non-tree/non-sapling resource gathering and successful weeding SHALL use the same responsive pickup timing. Rejected actions SHALL start no successful pickup presentation.

#### Scenario: Gather representative resources
- **WHEN** the player gathers branches, stones, berries, roots, flowers, or reeds
- **THEN** each successful action uses the responsive gesture once

#### Scenario: Weed a plot
- **WHEN** the player successfully weeds a planted plot
- **THEN** the same responsive gesture plays once without changing weed authority

### Requirement: Faster timing preserves character quality
The retimed gesture SHALL preserve planted feet, stable idle endpoints, compatible adult body/hair/garment presentations, disabled root motion, and zero gameplay notifies. It MUST NOT visibly pop, teleport, stretch, or snap through representative terrain at ordinary gameplay speed.

#### Scenario: Review appearance variants
- **WHEN** representative appearance/equipment combinations perform the pickup at close and gameplay cameras
- **THEN** the faster motion remains readable without gross body, clothing, hair, or ground intersection

### Requirement: Simulation remains authoritative
Reward, capacity, range, tools, renewal, inventory, sound, time, and persistence SHALL remain committed by the existing transaction independently of animation completion. Retiming MUST NOT delay, repeat, or cancel an already valid result.

#### Scenario: Animation is interrupted
- **WHEN** movement, menu, planning, failure, load/retry, or appearance application interrupts the gesture
- **THEN** presentation blends out promptly without queued replay or duplicate/missing transaction

#### Scenario: Rapid gathers compete
- **WHEN** another valid gather occurs while pickup presentation is active
- **THEN** gameplay transactions retain current authority while presentation coalesces without stacking delayed gestures

