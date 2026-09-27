# Spec Delta

## Purpose

Confirmed outcome-level rules for this round. Expand them when the round is fully specified.

## ADDED Requirements

### Requirement: The calendar has four 28-day seasons
The game SHALL run a calendar of four 28-day seasons starting on Spring day 1, with days from 6 AM to 2 AM lasting about 30 real minutes. Crops SHALL grow only in their seasons.

#### Scenario: Out-of-season planting
- **WHEN** the heroine tries to plant a summer-only crop in spring
- **THEN** planting is rejected with the reason that it is out of season

### Requirement: Late nights end gently
Staying awake past 2 AM or running out of energy SHALL send the heroine home asleep with at most a small, disclosed penalty and no other loss.

#### Scenario: Past 2 AM
- **WHEN** the clock reaches 2 AM while she is in the fields
- **THEN** she wakes at home the next morning with the disclosed penalty applied
