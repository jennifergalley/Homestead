# Spec Delta

## ADDED Requirements

### Requirement: Every cue is balanced against the ambience
Every sound the game plays SHALL have a category and a gain in the audio catalogue, and its level at that gain
and the default sliders SHALL sit inside its category's band, measured in LU over the forest ambience bed.

#### Scenario: A new cue out of band
- **WHEN** a cue is added at a gain that puts it outside its category's band
- **THEN** HomesteadAudioLevelTests fails and names the cue, its level and the band

#### Scenario: A new sound with no category
- **WHEN** a sound file is added under Assets/Audio with no cue row
- **THEN** HomesteadAudioLevelTests fails and names the file

#### Scenario: The scythe against the birds
- **WHEN** she mows with the scythe in the woods
- **THEN** the swish sits a few LU under the chops, within the Swish band
