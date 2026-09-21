# Spec Delta

## Purpose

Let the player actually wear the requested shorter waves and original blonde
bob in the current game while retaining her chosen body, face and animations.

## ADDED Requirements

### Requirement: Waves end naturally at mid back
The brunette wave choice SHALL end near the middle of the supported adult
character's back rather than the waist. It SHALL preserve believable wave
spacing and tapered ends without the rejected compressed ridges or blunt shelf.

#### Scenario: Player wears waves
- **WHEN** the player selects waves on any supported body and walks in the game
- **THEN** back and three-quarter views show mid-back length and coherent waves
  rather than the old waist-length style or the rejected compressed trial

### Requirement: Straight blonde bob is an original usable choice
The bob choice SHALL offer the requested straight blonde presentation with an
original silhouette, without copied third-party game art. Existing hairstyle
selection identity and supported hair-color choices SHALL remain available.

#### Scenario: Player selects the bob
- **WHEN** the player chooses the bob and blonde presentation
- **THEN** the playable character shows a straight short silhouette with readable
  strand detail and unchanged face, eyes, body and equipment

### Requirement: Hair changes preserve existing character behavior
All three existing body fits SHALL retain their original rig, animations,
non-hair surfaces and supported appearance/equipment state. Ponytail SHALL
remain unchanged. Source/interchange checks SHALL detect missing materials,
invalid weights, scale or incompatible binds.

#### Scenario: Player changes body or performs an action
- **WHEN** a supported body is selected or the player walks, gathers or uses a tool
- **THEN** the chosen new hair follows the same head pose and does not replace
  the body, reset clothing or lose the original animation/tool behavior

### Requirement: Completion requires actual playable integration
The delivered source mapping SHALL correspond to assets used by the current
Appearance selector. Completion SHALL include coordinator-owned cooked import
and ordinary gameplay views, not only successful Blender exports or screenshots.

#### Scenario: Only source export passes
- **WHEN** offline geometry checks and source previews pass but cooked integration
  has not been exercised
- **THEN** the delivery is labeled source-ready and gameplay validation pending,
  not presented as Jenny's integrated or aesthetically approved hairstyle
