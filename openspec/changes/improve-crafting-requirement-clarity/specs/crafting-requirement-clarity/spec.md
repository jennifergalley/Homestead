# Spec Delta

## Purpose

Makes recipe readiness and shortages understandable before activation while preserving authoritative crafting validation and current recipe balance.

## ADDED Requirements

### Requirement: Recipe cards communicate current availability
Each Craft recipe card SHALL visibly distinguish whether that recipe can be crafted at the player's current position with the carried pack. An unavailable recipe SHALL use a clearly greyed treatment but MUST remain selectable for inspection.

#### Scenario: Open Craft without enough ingredients
- **WHEN** one or more recipes cannot be crafted from the carried pack
- **THEN** each unavailable recipe is visibly greyed
- **AND** selecting it still opens its details

#### Scenario: A recipe is ready
- **WHEN** all consumed ingredients, retained requirements, nearby station requirements, and resulting pack capacity permit a recipe
- **THEN** its card uses the normal available treatment and its Craft action is enabled

### Requirement: Details show Have and Need for every requirement
The selected recipe details SHALL show each consumed ingredient as a separate requirement with its carried `Have` amount, required `Need` amount, and met/short state. An unmet ingredient row SHALL also show one concise current acquisition hint backed by authoritative resource/recipe behavior. Retained tools and nearby station requirements SHALL be shown separately and identified as retained or nearby rather than consumed.

#### Scenario: Inspect a partially supplied recipe
- **WHEN** the player selects a recipe with some but not all required ingredients
- **THEN** every ingredient row shows its current carried amount and required amount
- **AND** only insufficient rows use shortage treatment
- **AND** each insufficient row names a current primary way to obtain that item

#### Scenario: Find Fiber for the first hatchet
- **WHEN** the crude hatchet is selected without enough Fiber
- **THEN** the Fiber row identifies reeds near water as the bootstrap source
- **AND** it does not imply that the hatchet-gated sapling source is required first

#### Scenario: Inspect cooking requirements
- **WHEN** the player selects a cooking recipe away from a fueled cookfire
- **THEN** ingredient rows still show their carried Have/Need amounts
- **AND** a separate nearby cookfire row is marked unmet

#### Scenario: Inspect split firewood
- **WHEN** the player selects Split Firewood without carrying a hatchet
- **THEN** Timber appears as a consumed Have/Need row
- **AND** the hatchet appears as an unmet retained-tool row

### Requirement: Unavailable crafting is not presented as actionable
The Craft action for an unavailable recipe SHALL be visibly disabled and SHALL NOT mutate simulation state. Recipe inspection and directional/pointer navigation SHALL remain available.

#### Scenario: Activate an unavailable recipe
- **WHEN** the player presses A, Enter, or clicks Craft while the selected recipe is unavailable
- **THEN** no recipe transaction is requested or committed
- **AND** the structured shortage details remain visible

#### Scenario: State changes after selection
- **WHEN** a selected recipe changes between available and unavailable
- **THEN** its card, details, and Craft action update together without losing the selected recipe

### Requirement: Availability remains explanatory rather than authoritative
Pre-activation availability SHALL be derived from the same current recipe costs, retained requirements, station proximity, inventory capacity, and player state enforced by crafting authority. The authoritative transaction MUST revalidate all rules when an available recipe is activated.

#### Scenario: State changes between display and activation
- **WHEN** the displayed recipe was available but authoritative state changes before activation
- **THEN** crafting authority rejects the stale attempt without consuming ingredients or granting output
- **AND** the Craft page refreshes to show current requirements

#### Scenario: Supplies exist only in storage
- **WHEN** a nearby chest contains ingredients that are absent from the carried pack
- **THEN** those stored items do not satisfy the recipe Have amount
- **AND** the player is directed by the displayed shortage to move supplies into the pack

#### Scenario: A source rule changes
- **WHEN** an item no longer comes from the source named by its acquisition hint
- **THEN** source-contract validation fails rather than leaving misleading guidance in the Craft page

### Requirement: Craft clarity works across supported input and layout
Availability styling, semantic requirement rows, selection, focus, scrolling, and disabled-action behavior SHALL remain readable and synchronized for pointer, keyboard, and controller at supported 720p and 4K layouts.

#### Scenario: Navigate mixed recipe states
- **WHEN** the player moves through available and unavailable recipes using pointer, arrows/D-pad, or tab-region navigation
- **THEN** visible focus and details always correspond to the same selected recipe
- **AND** greyed styling does not hide focus indication or requirement text
