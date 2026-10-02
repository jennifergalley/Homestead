# Spec Delta

## MODIFIED Requirements

### Requirement: Weeds come up once a day
Plot weeds SHALL grow only in one daily pass: at 6 AM when she is awake, or when she wakes from a sleep
or doze that crossed 6 AM. Each calendar day SHALL get exactly one pass, and weeds SHALL NOT grow
between passes.

#### Scenario: Staying up
- **WHEN** she is awake from noon until 6 AM the next morning
- **THEN** no weeds appear until 6 AM, and one pass of weeds appears at 6 AM

#### Scenario: A night's sleep
- **WHEN** she sleeps from 22:00 until 06:00
- **THEN** one pass of weeds is up when she wakes, and none more come that day

#### Scenario: A nap
- **WHEN** she naps for two hours in the afternoon
- **THEN** no weeds appear

#### Scenario: Three days without sleep
- **WHEN** she stays awake for three days
- **THEN** three passes of weeds appear, one at each 6 AM

#### Scenario: Reload
- **WHEN** she saves and loads either side of 6 AM
- **THEN** the weeds and the timing of the next pass are unchanged
