# Rain weather

## ADDED Requirements

### Requirement: Visible rain

Rain SHALL fall visibly round the camera while `Homestead::IsRainingAt` is true, by day or night, with
its strength following `Homestead::RainAmount`, and SHALL NOT fall inside roofed buildings. Spells SHALL occur at random times through the full day/night cycle, weighted by season, on a
reload-stable schedule.

#### Scenario: A spell of rain

- **WHEN** a spell of rain is falling and she stands outdoors
- **THEN** rain streaks fall round her and the rain loop plays

#### Scenario: Rain at night

- **WHEN** a spell of rain falls at 23:00
- **THEN** it rains, the rain loop plays, her crops are watered, and the night stays readable by the lamp

### Requirement: Rain at any hour

Rain SHALL come in deterministic spells of 1 to 8 hours that can start at any hour of the day or night
and run past midnight and the 06:00 day boundary, more often in autumn and winter than in spring and
summer. The weather SHALL depend on the game clock alone, so a reload reproduces it with no save data.

#### Scenario: Reload during rain

- **WHEN** she saves during a night's rain and loads the save
- **THEN** it is still raining at the same moment

### Requirement: Overcast sky

The sky SHALL cloud over, the light SHALL soften and the scene SHALL read darker and greyer with
`Homestead::Overcast`, building over half an hour to an hour and a half before each spell and clearing as
long after it.

#### Scenario: Cloud before a spell

- **WHEN** a spell of rain is a few minutes away
- **THEN** the sky is part clouded and no rain falls yet

#### Scenario: Night rain

- **WHEN** a scheduled rain event occurs at night
- **THEN** rain, overcast and night lighting remain visually coherent without turning night into day
