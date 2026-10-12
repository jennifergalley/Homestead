# Spec Delta

## ADDED Requirements

### Requirement: The mine house can be entered and holds the way down
The estate SHALL have a stone mine house at the mine site with a walk-in ground-floor room
containing a rock pile, a desk, a permanently lit oil lamp that cannot be picked up, and a trapdoor
that leads down to the shallow level.

#### Scenario: Going down
- **WHEN** she opens the trapdoor and climbs down
- **THEN** she arrives at the foot of a ladder in the shallow level, and climbing the ladder returns her to the mine house

### Requirement: The mine is dark without her lamp
The shallow level SHALL receive no sky or sun light apart from daylight at the ladder foot, so her
oil lamp is the only practical light. Without a lit lamp the tunnels SHALL be near-dark but walkable,
and pickaxe work SHALL be refused. Pickaxe work SHALL also be refused unless the lit lamp is in her hand or a lit
set-down lamp is within its light radius (about 4 m) of the face.

#### Scenario: Lamp runs dry
- **WHEN** her lamp runs out of oil in a side drift
- **THEN** she can still make out the tunnel and walk back to the lit ladder shaft, and swinging the pickaxe says it's too dark to work

#### Scenario: Lamp left too far behind
- **WHEN** she sets the lit lamp down and works a face beyond its light
- **THEN** the pickaxe says it's too dark to work until she holds the lamp or sets it down closer

### Requirement: Rock faces give plenty of stone every day
The shallow level SHALL have rock faces that yield stone to the pickaxe and come back every morning
at 6:00, plus coal seams and copper veins that come back on a slower cadence. Gems have no place of
their own: mining stone or ore SHALL now and then also turn up a quartz or, rarely, an amethyst
(Jenny, 2026-10-11). The general store SHALL buy all of them.

#### Scenario: A morning in the mine
- **WHEN** she mines the shallow level's rock faces with a full lamp and full energy
- **THEN** she leaves with enough stone for a visible step of the manor restoration, plus some coal and copper ore, and now and then a gem

#### Scenario: A gem in the rock
- **WHEN** a swing at a rock face, coal seam or copper vein turns up a gem
- **THEN** she gets one quartz (or, rarely, one amethyst) on top of the usual yield, and the toast says what she found

### Requirement: Exhaustion underground is harmless
Running out of energy in the mine SHALL return her to the mine house's ground floor fully rested,
with nothing lost.

#### Scenario: Worn out below
- **WHEN** her energy reaches zero in the tunnels
- **THEN** she wakes in the mine house with full energy and her pack unchanged
