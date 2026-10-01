# Flexible sleep

## ADDED Requirements

### Requirement: No bedtime by the clock

The game SHALL NOT force or shorten sleep by the time of day. Only running out of Energy SHALL force
rest: she dozes off where she stands and wakes part rested. Running out of Energy SHALL NOT fail the
game.

#### Scenario: Night owl

- **WHEN** she stays up until 05:00 and sleeps "until rested" with little Energy left
- **THEN** she sleeps into the afternoon and wakes with full Energy

### Requirement: One-press sleep at the bed

Within the bed's focused interaction range, E/A SHALL immediately sleep without a confirmation
dialog, choice picker or nap-hours control. When Energy is not full, it sleeps until full but stops
at 06:00 when sleeping overnight. When already rested at night, it sleeps until 06:00. When already
rested during the day, it SHALL expose no sleep verb.

#### Scenario: Tired early night

- **WHEN** she presses E/A at 21:00 with Energy below full
- **THEN** she sleeps until Energy is full or 06:00, whichever comes first

### Requirement: Recovery by hours slept

Sleep SHALL restore Energy by hours slept at a fixed rate, capped at full, whatever the hour.

#### Scenario: Tired daytime recovery

- **WHEN** she presses E/A at a focused bed during the day with Energy below full
- **THEN** she sleeps until Energy is full and never exceeds full Energy
