# Flexible sleep

## ADDED Requirements

### Requirement: No bedtime by the clock

The game SHALL NOT force or shorten bed sleep by the time of day. Low Energy SHALL NOT force rest,
fainting, failure, death or checkpoint recovery on the Estate.

#### Scenario: Night owl

- **WHEN** she stays up until 05:00 with low Energy
- **THEN** she remains in play with no forced sleep or failure until she eats or uses a bed

### Requirement: One-press sleep at the bed

Within the bed's focused interaction range, E/A SHALL immediately sleep without a confirmation
dialog, choice picker or nap-hours control. When Energy is not full, it sleeps until full but stops
at 06:00 when sleeping overnight. When already rested at night, it sleeps until 06:00. When already
rested during the day, it SHALL expose no sleep verb.

#### Scenario: Tired early night

- **WHEN** she presses E/A at 21:00 with Energy below full
- **THEN** she sleeps until Energy is full or 06:00, whichever comes first

#### Scenario: Final minutes to dawn

- **WHEN** she presses E/A at 05:52 on a focused bed
- **THEN** the positive short sleep ends exactly at 06:00
- **AND** an ordinary non-bed `Simulation::Sleep` interval below 0.25 hours remains refused

### Requirement: Recovery by hours slept

Sleep SHALL restore Energy by hours slept at a fixed rate, capped at full, whatever the hour.

#### Scenario: Tired daytime recovery

- **WHEN** she presses E/A at a focused bed during the day with Energy below full
- **THEN** she sleeps until Energy is full and never exceeds full Energy

### Requirement: Low Energy constrains play without failure

Below about 25% Energy, sprint SHALL be unavailable. Below about 10%, walking SHALL slow and tool
work SHALL refuse with `Too tired`. The Energy bar SHALL change colour and pulse, with concise
`Getting tired` and `Exhausted` warnings. Eating and bed sleep SHALL recover Energy.

#### Scenario: Exhausted tool work

- **WHEN** Energy is below about 10% and she attempts tool work
- **THEN** no tool action occurs and the refusal says `Too tired`

### Requirement: Recovery loads the newest valid save

Any recovery or checkpoint load SHALL choose the newest valid candidate by timestamp and simulation
revision, rather than selecting an older Recovery save over a newer autosave.

#### Scenario: Newer autosave beats recovery

- **WHEN** a valid autosave is newer than a valid Recovery save
- **THEN** recovery loads the autosave
