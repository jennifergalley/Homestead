# Spec Delta

## Purpose

Make repeated homestead work actions feel grounded, responsive, and visually connected to their tools and targets without moving gameplay authority into animation.

## ADDED Requirements

### Requirement: Felled trees leave the world visibly
After a successful mature-tree fell, the tree SHALL remain visible until the chop impact and SHALL
then leave the world through a readable presentation, such as a fall, topple, or a remaining stump
with debris, instead of disappearing between frames. The simulation transaction, rewards, and
cleared-key persistence SHALL remain immediate and authoritative; the presentation MUST NOT block
movement, grant items, or reappear after save and reload.

#### Scenario: Fell a mature tree in ordinary play
- **WHEN** the player fells a focused mature tree with the selected hatchet
- **THEN** captures of the next few seconds show the trunk present at impact and then visibly falling or leaving a stump, not a tree that is simply gone

#### Scenario: Reload after felling
- **WHEN** the player saves and reloads after felling a tree
- **THEN** the tree does not rematerialize and any stump or debris shown is consistent with the cleared state
