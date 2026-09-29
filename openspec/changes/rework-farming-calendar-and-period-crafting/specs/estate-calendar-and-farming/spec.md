# Spec Delta

## Purpose

A readable farming year on the estate: seasons and weekdays, gentle hunger, seasonal crops and
forage, a seedsman in town, and a seasonal look.

## ADDED Requirements

### Requirement: The calendar has 28-day seasons, weekdays and years
The game SHALL count four 28-day seasons (Spring, Summer, Autumn, Winter), seven named weekdays and years from Spring 1, 1851, a Monday, with each day beginning at the 06:00 rollover. The HUD SHALL show the weekday, season and day, and in a season's last three days it SHALL show how many days are left.

#### Scenario: New game date
- **WHEN** a new game begins
- **THEN** the HUD reads "Mon, Spring 1"

#### Scenario: End of season warning
- **WHEN** it is Spring 26
- **THEN** the HUD shows that 3 days are left in Spring

#### Scenario: Year turns
- **WHEN** Winter 28, 1851 rolls over at 06:00
- **THEN** the date becomes Spring 1, 1852

### Requirement: Default day length is about 30 real minutes
A new game SHALL default to a 30-real-minute day. The Settings choice of 30, 60 or 120 minutes SHALL remain available and SHALL persist with the save.

#### Scenario: Default length
- **WHEN** a new game runs for 15 real minutes unpaused
- **THEN** about 12 game hours have passed

### Requirement: Hunger slows her but never fails the game
Empty hunger SHALL NOT fail the game, block actions or send the heroine to a checkpoint. Below 25 hunger, energy SHALL recover more slowly and work SHALL cost more energy. At 0 both effects SHALL be stronger. Eating SHALL remove the penalty at once.

#### Scenario: Famished but working
- **WHEN** hunger is 0 and the heroine clears bramble
- **THEN** the clear succeeds at a higher energy cost and no failure screen appears

#### Scenario: Eating restores normal pace
- **WHEN** a famished heroine eats a meal
- **THEN** hunger rises and her work costs return to normal immediately

### Requirement: Crops grow only in their seasons
Every crop SHALL declare the seasons it grows in. Planting out of season SHALL be refused and SHALL name the crop's seasons. Planting a crop that cannot ripen before its last in-season day SHALL be allowed with a warning.

#### Scenario: Out of season
- **WHEN** the heroine plants turnip seed in Spring
- **THEN** planting is refused with a message naming Autumn and Winter

#### Scenario: Too late to ripen
- **WHEN** she plants cabbage on Winter 25
- **THEN** the cabbage is planted and she is warned that it won't ripen before Winter ends

### Requirement: Out-of-season crops wither at the season change
At the season rollover, every planted crop that does not grow in the new season SHALL become withered. A withered plot SHALL yield nothing and SHALL be clearable back to tilled soil with the hoe. Crops that grow in both seasons SHALL continue unaffected.

#### Scenario: Potatoes left into Summer
- **WHEN** unharvested potatoes are still planted when Summer 1 begins
- **THEN** the plot shows a withered plant and yields nothing

#### Scenario: Carrots cross into Summer
- **WHEN** carrots planted in late Spring are growing on Summer 1
- **THEN** they keep growing normally

### Requirement: Each season has its own crops
The crop catalogue SHALL provide at least three crops in each season, including peas, wheat, barley, leeks and winter broccoli in addition to the existing period crops.

#### Scenario: Winter planting
- **WHEN** it is Winter and the heroine visits the seedsman
- **THEN** at least three kinds of seed are for sale, including winter broccoli

### Requirement: The seedsman sells seasonal seed and buys grain
A seedsman shop SHALL open in town. It SHALL sell only seed for crops that grow in the current season, together with a tin watering can, and SHALL buy grain. The general store SHALL no longer sell seed. Both shops SHALL be closed on Sundays.

#### Scenario: Sunday
- **WHEN** the heroine tries either shop's door on a Sunday
- **THEN** it is closed with a notice saying it is closed on Sundays

#### Scenario: Sell wheat
- **WHEN** she sells wheat to the seedsman
- **THEN** her money rises and the wheat appears in his "From {Estate}" stock

### Requirement: Foraging follows the seasons
Each forage kind SHALL be available only in its seasons. Blackberries SHALL be pickable from the estate brambles from Summer 15 to Autumn 28, and field mushrooms SHALL appear in autumn.

#### Scenario: Blackberries in late summer
- **WHEN** it is Summer 20 and the heroine interacts with a fruiting bramble
- **THEN** she picks blackberries, and the bramble renews later in the window

#### Scenario: Blackberries in spring
- **WHEN** it is Spring 10
- **THEN** the brambles show no fruit and offer no pick prompt

### Requirement: The estate looks different each season
Grass and deciduous foliage SHALL change colour through the seasons. Deciduous trees SHALL be bare in winter while evergreens stay green, and winter mornings SHALL show frost. Each change SHALL blend in over the first days of the season.

#### Scenario: Winter walk
- **WHEN** the heroine walks through the estate woods on a winter morning
- **THEN** the oaks and beeches are bare, the holly is green, and the grass is frosted
