# Spec Delta

## Purpose

Let resource visuals and inventory changes carry routine foraging feedback without exposing the internal renewal schedule in world prompts or toasts.

## ADDED Requirements

### Requirement: Renewal state is not narrated to the player
Routine resource focus labels, successful harvest/clear notices and post-harvest messages MUST NOT mention renewing, regrowing, permanent removal, cooldown durations or other simulation scheduling. The underlying renewal and persistence rules SHALL remain unchanged.

#### Scenario: Gather a berry bush or reeds
- **WHEN** the player harvests a ready renewable patch
- **THEN** inventory, plant presentation and appropriate action/audio respond, with no toast or focus label explaining that the patch will regrow or is "renewing"

#### Scenario: Clear low growth
- **WHEN** the player clears an eligible patch with the correct tool
- **THEN** removal and rewards happen once without a tutorial toast about permanent clearing or regrowth

### Requirement: Failures remain understandable without oversharing
Genuine inability to gather SHALL remain apparent through unavailable action/produce cues and a concise response when attempted. Input errors MUST NOT silently grant resources or hide a failed transaction.

#### Scenario: Try an exhausted patch
- **WHEN** the player attempts to gather a depleted resource
- **THEN** nothing is awarded and any failure message is short and nontechnical, without explaining timers or renewal mechanics
