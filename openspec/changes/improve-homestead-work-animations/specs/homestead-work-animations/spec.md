# Spec Delta

## Purpose

Make repeated homestead work actions feel grounded, responsive, and visually
connected to their tools and targets without moving gameplay authority into
animation.

## ADDED Requirements

### Requirement: Chopping reads as grounded tool work
Successful sapling clearing and mature-tree felling SHALL present a readable
hatchet action with anticipation, a target-directed swing, impact follow-through,
and stable recovery. The heroine's feet SHALL remain grounded, the hatchet SHALL
stay attached to the correct hand, and the arc SHALL face the selected trunk
without changing the authoritative interaction range or player position.

#### Scenario: Fell a mature tree
- **WHEN** the player successfully fells a focused mature tree with a carried
  hatchet
- **THEN** one responsive hatchet action faces and approaches the trunk
  visually, the permanent clearing/reward transaction occurs exactly once, and
  control returns without foot sliding, camera rotation, or a queued second swing

#### Scenario: Chopping is rejected
- **WHEN** the player lacks a hatchet, capacity, range, or a valid uncleared target
- **THEN** the action fails with existing gameplay feedback and no successful
  chop presentation, reward, clearance, or delayed swing occurs

### Requirement: Tilling has dedicated ground-directed motion
Successful tilling SHALL present a digging-stick action aimed toward the selected
cell with a planted stance, downward soil contact, and clear recover. It SHALL
remain visually distinct from gathering, watering, and chopping.

#### Scenario: Till a valid cell
- **WHEN** the player tills a valid focused soil cell with a carried digging stick
- **THEN** one dedicated tilling gesture aims at that cell, the plot transaction
  commits exactly once, and the heroine returns to locomotion without sliding
  or leaving the tool visible

#### Scenario: Tilling is blocked
- **WHEN** the cell is occupied, obstructed, out of range, or the digging stick
  is missing
- **THEN** no successful tilling gesture or plot mutation occurs and the existing
  readable rejection remains authoritative

### Requirement: Watering visibly addresses the selected plot
Successful watering SHALL present the existing can through a deliberate lift,
aim, pour, and recover directed toward the selected plot. The can SHALL remain
correctly scaled and hand-bound, and MUST hide after completion or cancellation.

#### Scenario: Water a growing plot
- **WHEN** the player waters a focused eligible plot with carried water and can
- **THEN** one plot-directed pour is visible, one water portion is consumed,
  moisture changes exactly once, and the can hides after a clean recovery

#### Scenario: Watering is interrupted
- **WHEN** movement, menu opening, planning, loading, recovery, failure, or
  appearance application interrupts the gesture
- **THEN** presentation blends out without later replay, duplicate water use,
  orphaned can, camera movement, or stale action state

### Requirement: Simulation remains the only transaction authority
Animations, props, sound cues, particles, and timing markers SHALL be
presentation-only. Rewards, inventory costs, resource/plot state, time, needs,
and persistence MUST remain owned by the existing successful simulation
transaction and MUST NOT depend on an animation notify, prop collision, or
visual completion.

#### Scenario: Presentation asset is unavailable
- **WHEN** an action transaction succeeds but its required animation or prop
  cannot be loaded
- **THEN** the gameplay result remains committed once, the error is reported
  explicitly, and no fabricated fallback is claimed as completed animation work

#### Scenario: Rapid actions compete
- **WHEN** another work request arrives while a hand action is active
- **THEN** gameplay transactions still obey their existing authority, while
  presentation coalesces or cancels without stacking, replaying, or granting
  additional outcomes

### Requirement: Work animations preserve character and world behavior
The work set SHALL support all admitted adult body and equipped garment
presentations and SHALL retain existing hair, tool, collision, regional terrain,
camera, save/reload, controller, and menu behavior. Ordinary action views MUST
remain readable at the gameplay camera, not only in an isolated preview.

#### Scenario: Perform work across character variants
- **WHEN** representative body, hairstyle, and garment combinations chop, till,
  and water in ordinary gameplay
- **THEN** the same skeleton and equipment remain intact, tools do not grossly
  clip or detach, the camera remains player-controlled, and the work action
  reads clearly from close and normal gameplay views

### Requirement: Playable acceptance is measured and honest
Promotion SHALL require source/interchange checks, native lifecycle checks,
ordinary controller actions, close and gameplay-camera motion evidence,
save/replay regression, and measured cadence. Technical success MUST NOT be
described as human aesthetic approval or exact physical contact.

#### Scenario: Candidate is considered for selection
- **WHEN** the improved work-animation build is reviewed
- **THEN** the exact executable, source checkpoint, profiles, rollback, tested
  character/action matrix, timing methodology, contact limits, and any deferred
  sound/VFX or IK scope are recorded
