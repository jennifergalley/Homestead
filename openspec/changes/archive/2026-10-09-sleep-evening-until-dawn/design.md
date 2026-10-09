# Design

## Context

`BedSleepOption` currently caps tired nighttime sleep at the time energy fills. `Simulation::Sleep` steps ordinary world time and has a fixed twelve-hour limit. Lighting currently has fixed 06:00/18:00 horizon crossings. See proposal.md for feedback.

## Goals / Non-Goals

Goals: one authoritative bed offer; retain daytime policy, native callers, sleep autosaves and short pre-dawn intervals.
Non-goals: seasonal astronomy, animation edits, save changes or editor playtesting.

## Decisions

- Put sleep policy in a small simulation-owned helper; share the existing solar timing with lighting instead of duplicating a different dawn rule. Earlier sunrise/sunset timing parameters can be tested without inventing a seasonal model.
- Evening offers always choose until-morning; leave explicit daytime rest and existing generic sleep API behavior intact where possible. Allow longer-than-twelve-hour sleep only when validated against the offered night interval.
- Travel Rest owns sleep helpers and bed presentation. Coordinate minimal Simulation header/core hunks with Farming Fishing. Integration owns machine-wide compile scheduling; Jenny owns player acceptance.

## Risks / Trade-offs

- Earlier sunset can require more than twelve hours of sleep -> validate the dawn interval rather than broadly lifting all sleep limits.
- Existing tests encode waking when rested at night -> change policy assertions, retain direct time-stepping regressions.
- Fixed current sunlight -> do not claim seasonal solar behavior that the game does not have.
