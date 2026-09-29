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

### Requirement: Energy is the only meter, and meals make her Well fed
On the estate the heroine SHALL have no hunger meter, hunger drain, hunger penalty or hunger failure, and the HUD SHALL show no hunger meter. Food SHALL restore energy. Snacks (raw food, bread, cheese) SHALL restore a little and SHALL NOT grant Well fed. Meals (cooked dishes) SHALL restore more and SHALL grant Well fed for 3 game hours, shown as a clock time. While Well fed, every piece of work SHALL cost 15% less energy. Eating a meal SHALL set the Well fed expiry to 3 game hours from now, never stacking. Below full energy, any food SHALL restore its energy. At full energy, a snack SHALL be refused without being consumed. At full energy, a meal SHALL be eaten only if it starts Well fed or extends it by at least one game hour, and the game SHALL say that her energy was already full; otherwise the meal SHALL be refused without being consumed.

#### Scenario: Meal
- **WHEN** she eats a Cornish pasty below full energy
- **THEN** her energy rises by 40, she is Well fed for 3 game hours shown as a clock time, and clearing bramble costs 15% less energy until it expires

#### Scenario: Snack
- **WHEN** she eats a loaf of bread below full energy
- **THEN** her energy rises by 12 and no Well fed state begins

#### Scenario: Second meal below full energy
- **WHEN** she is Well fed for 2 more hours, is below full energy, and eats roast potatoes
- **THEN** her energy rises by 25 and she is Well fed for 3 game hours from now

#### Scenario: Full energy
- **WHEN** her energy is full and she is not Well fed, and she tries to eat bread and then a Cornish pasty
- **THEN** the bread is refused and kept, and the pasty is eaten with a message that her energy was already full and when Well fed ends

#### Scenario: Already well fed at full energy
- **WHEN** her energy is full, she ate a pasty 30 game minutes ago, and she tries to eat another pasty
- **THEN** the pasty is refused and kept, because it would extend Well fed by less than one hour

#### Scenario: Well fed across midnight
- **WHEN** she eats a pasty at 11 PM
- **THEN** she is still Well fed at 1:30 AM the next day, and it has ended by 2 AM

#### Scenario: Tampered Well fed save
- **WHEN** a save's Well fed expiry is not a finite number, or lies more than 3 game hours after the save's time
- **THEN** loading fails with the corrupt-save message and the current game is unchanged

#### Scenario: Expired Well fed save
- **WHEN** a save's Well fed expiry is at or before the save's time
- **THEN** the save loads normally and she is not Well fed

#### Scenario: No hunger
- **WHEN** she plays ten full days without eating
- **THEN** no hunger toast, penalty or failure appears, and the HUD shows only energy among her vitals

### Requirement: A new game starts with some food and a findable hoe
A new game SHALL seed the standing-room chest with 3 Cornish pasties, 2 loaves of bread and one of every finished outfit piece she doesn't already wear. Loading a game SHALL NOT seed them again. Salvage piles SHALL yield the rusted hoe blade second, after the billhook blade. Trying to till without a hoe SHALL suggest searching the old manor's salvage. The arrival journal note SHALL hint where the garden tools were kept.

#### Scenario: Starter wardrobe
- **WHEN** a new game begins, and later she saves and reloads
- **THEN** the standing-room chest holds one of each finished outfit piece except the tunic she wears, and reloading adds no second copy

#### Scenario: Second salvage pile
- **WHEN** she has hafted the billhook and searches any other salvage pile
- **THEN** that pile yields the rusted hoe blade

#### Scenario: Tilling without a hoe
- **WHEN** she tries to till with no hoe
- **THEN** the refusal suggests searching the salvage in the old manor for a hoe blade

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
