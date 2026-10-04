## ADDED Requirements

### Requirement: Seed packets have crop-specific identities and individual slots
Each roots, turnip, carrot, potato, cabbage, broad bean and strawberry planting packet SHALL show its crop-specific name and end-product packet icon. Each packet SHALL occupy its own inventory, chest or hotbar slot and SHALL NOT merge with another packet, including packets of the same crop. Buying, harvesting, planting, transferring and dropping SHALL preserve each crop identity and count. Existing saved seed quantities SHALL load without loss or conversion into unrelated crops.

#### Scenario: Buy packets for two crops
- **WHEN** the player buys two carrot packets and one turnip packet
- **THEN** three individual slots show the appropriate crop identities and distinct packet icons, and selecting one plants only that crop

#### Scenario: Load an old multi-seed group
- **WHEN** a valid existing save contains a group of several seeds for one crop
- **THEN** all seeds become individual packets of that same crop, preserving the original referenced packet and total quantity

## MODIFIED Requirements

### Requirement: Ripe crops are easy to spot
A growing crop SHALL show its produce (roots pushing out of the soil, heads, pods or fruit) growing and colouring up from pale green as it ripens. A ripe plot SHALL read as ripe at the gameplay camera distance, in rain and at dusk, by the produce's full size and colour alone, with no glint or other effect. When focused, a weed-free ripe crop SHALL always show [E] Harvest followed by its crop name, or the equivalent controller interact binding, regardless of retired tutorial hints. Visible weeds SHALL first offer their removal instead of Harvest.

#### Scenario: A ripening plot
- **WHEN** a strawberry plot is three-quarters grown
- **THEN** it shows small, pale berries that are visibly larger and redder each morning

#### Scenario: A ripe plot
- **WHEN** a plot is ripe and needs no weeding
- **THEN** its produce is full size and full colour, and focusing it always offers Harvest with the crop name, even after repeated successful harvests

#### Scenario: A picked plant
- **WHEN** she picks a ripe broad bean plot
- **THEN** the big pods are gone and small green ones grow back over the next days
- **AND** the plot reads "Broad beans: ripening again, day 1 of 3", matching the harvest message, while a first-growth plot keeps counting "day N of 7"

#### Scenario: Weeds don't look like produce
- **WHEN** a plot grows weedy
- **THEN** the weeds are plain green young nettles, with no white or coloured flower heads that could read as ripe produce
- **AND** visible weeds on a ripe square offer weeding first and Harvest returns immediately after they are removed
