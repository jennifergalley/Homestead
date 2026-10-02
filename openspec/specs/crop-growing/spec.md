# crop-growing Specification

## Purpose
Make growing crops readable: how long they take, how they're doing, when they're ripe, and a proper harvest.

## Requirements

### Requirement: Crops state their growing time in days
Every crop SHALL have a growing time in whole days at full care. The seed's description, the planting message and the plot's focus line SHALL show that time in days, not hours.

#### Scenario: Sowing a crop
- **WHEN** she sows turnip seed in tilled soil
- **THEN** the message reads "Planted turnips. Ready in about 4 days if watered."

#### Scenario: Checking a growing plot
- **WHEN** she focuses a turnip plot sown yesterday and watered today
- **THEN** the focus line reads "Turnips: day 2 of 4"

#### Scenario: A watered crop ripens on time
- **WHEN** a 4-day crop is sown and watered each morning with no weeds
- **THEN** it is ripe 4 days after it was sown

### Requirement: Dry and weedy plots grow slowly and say so
Soil watered within the last day and plots less than half weedy SHALL grow at full speed. Drier or weedier plots SHALL grow more slowly without dying, and the focus line SHALL name the reason.

#### Scenario: A dry plot
- **WHEN** a growing plot's soil has dried below the watered threshold
- **THEN** the focus line adds "needs water, growing slowly", and the bed shows the dry soil

#### Scenario: A weedy plot
- **WHEN** weeds cover more than half of a growing plot
- **THEN** the focus line adds "weedy, growing slowly"

### Requirement: Period crops are sold as seed at the general store
Pascoe's general store SHALL sell seed for turnips, carrots, potatoes, broad beans, strawberries and cabbage. Each crop SHALL yield its own produce, which she can eat or sell at the store. The legacy root and berry crops SHALL keep working.

#### Scenario: Buying and sowing seed
- **WHEN** she buys carrot seed at the store and sows it
- **THEN** the plot grows carrots and yields carrots when harvested

#### Scenario: Picking a regrowing crop
- **WHEN** she picks a ripe broad bean plot
- **THEN** she gets bean pods, the plant stays in the ground, and it ripens again in about 3 days

### Requirement: Plants visibly grow through stages
A sown plot SHALL show a plant for its crop that changes through Sprout, Young, Growing, Mature and Ripe stages as it grows, with the produce visible on ripe plants where it grows above ground or breaks the soil.

#### Scenario: Watching a crop grow
- **WHEN** she sleeps through the nights after sowing a crop and watering it each morning
- **THEN** the plant is visibly larger each morning, and the ripe plant shows its produce

### Requirement: Ripe crops are easy to spot
A growing crop SHALL show its produce (roots pushing out of the soil, heads, pods or fruit) growing and colouring up from pale green as it ripens. A ripe plot SHALL read as ripe at the gameplay camera distance, in rain and at dusk, by the produce's full size and colour alone, with no glint or other effect. When focused, it SHALL read "ready to harvest" with a Harvest action.

#### Scenario: A ripening plot
- **WHEN** a strawberry plot is three-quarters grown
- **THEN** it shows small, pale berries that are visibly larger and redder each morning

#### Scenario: A ripe plot
- **WHEN** a plot is ripe
- **THEN** its produce is full size and full colour, and focusing it offers Harvest

#### Scenario: A picked plant
- **WHEN** she picks a ripe broad bean plot
- **THEN** the big pods are gone and small green ones grow back over the next days
- **AND** the plot reads "Broad beans: ripening again, day 1 of 3", matching the harvest message, while a first-growth plot keeps counting "day N of 7"

#### Scenario: Weeds don't look like produce
- **WHEN** a plot grows weedy
- **THEN** the weeds are plain green young nettles, with no white or coloured flower heads that could read as ripe produce

### Requirement: Harvesting has its own animation
Harvesting SHALL play a kneel-and-pull animation for root crops and cabbage, and a pick animation for beans and berries. The produce SHALL show briefly in her hand and then be hidden.

#### Scenario: Pulling a turnip
- **WHEN** she harvests a ripe turnip plot
- **THEN** she kneels, pulls the turnip up, holds it briefly, and her hand is empty afterwards

### Requirement: Weeds come up once a day, on some plots
Plot weeds SHALL grow only in one daily pass: at 6 AM when she is awake, or when she wakes from a sleep
or doze that crossed 6 AM. In each pass, each cultivated plot SHALL sprout weeds with a fixed chance
(35%), decided deterministically from the plot and the day so a reload never rerolls it. Weeds SHALL
NOT grow between passes.

#### Scenario: A night's sleep
- **WHEN** she sleeps from 22:00 until 06:00
- **THEN** one pass has run when she wakes: some plots have new weeds and others don't

#### Scenario: A nap
- **WHEN** she naps for two hours in the afternoon
- **THEN** no weeds appear

#### Scenario: Several days pass
- **WHEN** she stays awake or sleeps through several mornings
- **THEN** each morning runs exactly one pass, and the same plots sprout as if each day had passed separately

#### Scenario: Reload
- **WHEN** she saves and loads either side of 6 AM
- **THEN** which plots sprout, and the timing of the next pass, are unchanged
