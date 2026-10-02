## ADDED Requirements

### Requirement: Heroine clips are checked against human joint limits
Every baked heroine clip SHALL be checkable against per-joint range-of-motion bands for an adult woman. Each joint SHALL have a comfortable band and an extreme band. A frame outside the extreme band SHALL be an error, and a frame between the bands a warning. The check SHALL also flag coupling violations, joint speeds above the pop ceilings, ground penetration, planted contacts that slide, and the centre of mass outside the support in held frames. Angles SHALL be measured from component-space bone transforms, never from raw local Euler channels.

#### Scenario: A hyperextended elbow is an error
- **WHEN** a clip bends her elbow 15 degrees backward
- **THEN** the check reports an error on that elbow at that frame

#### Scenario: A neutral standing pose is clean
- **WHEN** the check runs on a pose in anatomical neutral
- **THEN** it reports no issues

#### Scenario: Forearm roll is measured wherever it is keyed
- **WHEN** a clip rolls the hand 100 degrees about the forearm
- **THEN** the check reports a pronation error, whether the roll is keyed on the hand or on the forearm

### Requirement: Every bake reports its anatomy
Baking a clip from a recipe SHALL run the joint-limit check and log a summary. The summary SHALL name the recipe's key frames, and fast joints within two frames of a listed contact SHALL count only as warnings. A failed check SHALL never block the bake.

#### Scenario: Baking a tool swing
- **WHEN** a recipe bakes its clip with its FRAMES and strike contacts
- **THEN** the log shows the issue counts and the worst issue per joint with its frame and key name

### Requirement: Animators have a realistic-animation skill
The repository SHALL have a skill covering design rules, a per-joint ROM table for the MetaHuman bones with sources, coupling rules, grips per tool, failure patterns with fixes, and a review checklist with front, side and top views. The animation and Blender asset skills SHALL reference it.

#### Scenario: Designing a new action
- **WHEN** an agent authors a new heroine action
- **THEN** the animation-layer skill sends it to the realistic-animation skill before keying, and to its checklist before committing
