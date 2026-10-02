# Spec Delta

## Purpose

Provides a visibly natural resting loop so the heroine feels alive while standing in gameplay and while being reviewed in character menus.

## ADDED Requirements

### Requirement: Standing idle has readable natural motion
When grounded and not moving or performing an action, the heroine SHALL show a subtle looping rest with visible breathing, minute weight shift, and restrained head, shoulder, arm, hand, and finger movement. The motion MUST remain calm rather than swaying, posing, or fidgeting continuously.

#### Scenario: Stand still in ordinary gameplay
- **WHEN** the heroine remains grounded and idle for one complete loop
- **THEN** breathing and small resting motion are perceptible at the normal gameplay camera while both feet remain planted

### Requirement: Idle remains physically stable
The living idle SHALL preserve the accepted skeleton, scale, loop seam, grounded feet, stable world position, compatible body/hair/garment presentation, disabled root motion, and zero gameplay notifies.

#### Scenario: Review appearance combinations
- **WHEN** representative body, hairstyle, tunic, apron, and footwear combinations idle
- **THEN** the motion does not create gross clipping, foot sliding, root drift, camera movement, or garment detachment

### Requirement: Idle integrates with locomotion and work actions
Movement and successful hand actions SHALL blend out of idle without a start snap. Stopping and action recovery SHALL return to the current idle phase cleanly without replaying or affecting authoritative gameplay state.

#### Scenario: Walk then stop
- **WHEN** the heroine transitions from walking to stationary
- **THEN** she settles into natural idle without teleporting feet, restarting a work action, or moving the pawn

### Requirement: Character preview uses the same living rest
The fixed inventory/Appearance preview SHALL play the same admitted idle motion continuously while menus pause simulation. Preview playback SHALL be presentation-only and MUST NOT advance world time, needs, save state, or gameplay animation authority.

#### Scenario: Leave inventory open
- **WHEN** Inventory remains open for multiple idle loops
- **THEN** the preview continues breathing/resting smoothly while gameplay remains paused

