# Spec Delta

## ADDED Requirements

### Requirement: Stepped sound levels are heard live and saved once
Stepping a sound slider with the d-pad or keyboard SHALL change the level she hears immediately without saving, SHALL save it once when she confirms, moves off the slider or closes the book, and SHALL restore the previous level unsaved when she presses Back before that.

#### Scenario: Three steps then moving on
- **WHEN** she presses Right three times on Music and then moves up a row
- **THEN** the music gets louder with each press, and the new level is written to her settings once

#### Scenario: Changing her mind
- **WHEN** she presses Left on Music and then B
- **THEN** the music returns to its previous level, nothing is saved, and Settings stays open
