# Flexible sleep

## ADDED Requirements

### Requirement: No bedtime by the clock

The game SHALL NOT force or shorten sleep by the time of day. Only running out of Energy SHALL force
rest: she dozes off where she stands and wakes part rested. Running out of Energy SHALL NOT fail the
game.

#### Scenario: Night owl

- **WHEN** she stays up until 05:00 and sleeps "until rested" with little Energy left
- **THEN** she sleeps into the afternoon and wakes with full Energy

### Requirement: Sleep choices at the bed

The bed SHALL offer "until morning" (wake 06:45) in the evening and at night, "until rested" at any
hour, and a one-hour nap, each showing its wake time, chosen with Up/Down and confirmed with A.

#### Scenario: Early night

- **WHEN** she goes to bed at 21:00 with some Energy left and chooses "until morning"
- **THEN** she wakes at 06:45

### Requirement: Recovery by hours slept

Sleep SHALL restore Energy by hours slept at a fixed rate, capped at full, whatever the hour.
