# Rain weather

## ADDED Requirements

### Requirement: Visible rain

During a scheduled rain event, rain SHALL fall visibly round the camera while
`Homestead::IsRainingAt` is true, with its strength following `Homestead::RainAmount`, and SHALL
NOT fall inside roofed buildings. Rain events SHALL occur at random times through the full
day/night cycle with seasonal weighting and a reload-stable schedule.

#### Scenario: Rain day

- **WHEN** a scheduled rain event occurs and she stands outdoors
- **THEN** rain streaks fall round her and the rain loop plays

### Requirement: Overcast sky

The sky SHALL cloud over, the light SHALL soften and the scene SHALL read darker and greyer with
`Homestead::Overcast`, blending before and after the rain, including during night events.

#### Scenario: Night rain

- **WHEN** a scheduled rain event occurs at night
- **THEN** rain, overcast and night lighting remain visually coherent without turning night into day
