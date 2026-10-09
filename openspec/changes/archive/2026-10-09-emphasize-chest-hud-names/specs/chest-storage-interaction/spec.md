# Spec Delta

## ADDED Requirements

### Requirement: Focused chest names lead the interaction hint
The focused chest's name SHALL be larger and more prominent than its keyed Open verb. Both SHALL remain readable without clipping or overlapping other HUD elements at 720p and 4K, including long custom names. Existing input and hint retirement SHALL remain unchanged.

#### Scenario: Read a named chest
- **WHEN** she faces a chest with a long custom name while an interaction hint is offered
- **THEN** its full name leads the focus card and the current device's Open verb remains legible below it
