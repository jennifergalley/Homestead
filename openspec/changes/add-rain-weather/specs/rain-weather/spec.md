# Rain weather

## ADDED Requirements

### Requirement: Visible rain

On a rain day, rain SHALL fall visibly round the camera while `Homestead::IsRainingAt` is true, with
its strength following `Homestead::RainAmount`, and SHALL NOT fall inside roofed buildings.

#### Scenario: Rain day

- **WHEN** it's 11:00 on a rain day and she stands outdoors
- **THEN** rain streaks fall round her and the rain loop plays

### Requirement: Overcast sky

The sky SHALL cloud over, the light SHALL soften and the scene SHALL read darker and greyer with
`Homestead::Overcast`, blending over half an hour before and after the rain.

#### Scenario: Before the rain

- **WHEN** it's 08:45 on a rain day
- **THEN** the sky is part clouded and no rain falls yet
