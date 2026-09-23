# Spec Delta

## Purpose

Defines natural ordinary heroine locomotion beyond the current prototype walk loop, including starts, stops, turns, grounded contact, and slope response.

## ADDED Requirements

### Requirement: Ordinary walking reads as natural human motion
The heroine's walk SHALL show coordinated weight transfer, pelvis and torso counter-motion, non-mechanical arm swing, and a stride cadence matched to movement speed. It MUST remain compatible with every supported body, hairstyle, garment, and footwear combination.

#### Scenario: Straight walk at gameplay camera
- **WHEN** the heroine walks steadily on level ground
- **THEN** the gait reads as a relaxed human walk rather than a symmetrical robotic loop or accelerated idle

#### Scenario: Slow and full walk speeds
- **WHEN** movement accelerates from a small input to full walking speed
- **THEN** stride and cadence adapt without visible foot skating, leg snapping, or abrupt pose replacement

### Requirement: Starts, stops, and turns are presented explicitly
Locomotion SHALL present readable acceleration, planted stopping, and direction-change motion rather than relying only on a continuous walk loop blended to idle.

#### Scenario: Start from rest
- **WHEN** movement begins from idle
- **THEN** the heroine transfers weight into the first step without sliding both feet

#### Scenario: Stop from full walk
- **WHEN** movement input is released at walking speed
- **THEN** a planted recovery settles into idle without snapping or gliding

#### Scenario: Turn while moving
- **WHEN** the movement direction changes materially
- **THEN** body heading, pelvis, feet, and upper body produce a readable turn without moonwalking

### Requirement: Feet adapt conservatively to terrain
On supported slopes and small height differences, locomotion SHALL keep feet near the visible ground and adjust pelvis/legs without changing authoritative capsule collision or moving the pawn through animation root motion.

#### Scenario: Traverse representative slope
- **WHEN** the heroine walks across and along generated sloped terrain
- **THEN** feet do not visibly float or sink beyond the admitted tolerance and the pelvis does not pop

#### Scenario: Unsupported trace or steep surface
- **WHEN** reliable foot placement cannot be resolved
- **THEN** the system falls back to the authored pose without invalid transforms, teleporting, or gameplay mutation

### Requirement: Existing action and camera behavior remains compatible
Locomotion polish MUST preserve movement authority, collision, camera orbit, sprint/action overlays, menu/planning pause, save/load, and cancellation semantics.

#### Scenario: Gather after walking
- **WHEN** the player approaches a resource and starts an action
- **THEN** locomotion yields cleanly to the authoritative action presentation with no queued start/stop/turn pose

#### Scenario: Appearance changes
- **WHEN** body, hair, clothing, dye, or equipment changes
- **THEN** locomotion remains valid on the shared skeleton and returns without stale pose state
