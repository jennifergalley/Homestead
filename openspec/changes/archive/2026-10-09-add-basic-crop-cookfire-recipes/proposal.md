# Proposal

## Why
Jenny selected `jenny-mut5gsd4-oebvov`: grown crops need useful cooked meals, not only sale value. Add eight simple cookfire dishes for build `20261003-measured-02`.

## What Changes
- Add eight basic recipes covering all six shop-seed crops, berries and roots.
- Require a fueled cookfire or the existing hearth and one kindling per cooked batch; season some dishes with Meadow herb.
- Show ingredients, output and Energy in the existing cookbook; balance Energy against foregone ingredient sale coins.
- Jenny's 20:04 clarification: author new Blender art for each of the eight cooked dishes; do not reuse existing food meshes.

## Capabilities
### New Capabilities
- `crop-cookfire-meals`: basic crop cooking and sale-cost-relative Energy.
### Modified Capabilities
None.

## Impact
Append-only Item/Recipe entries, a shared recipe table, crafting assessment/transactions, cookbook and native tests. Existing meals keep their balance. Saves use the existing counted item stocks without a version change.

## Reuse research
Reuse ItemCatalogue, Craft/AssessRecipe, fueled-fire/hearth checks, FoodClass::Meal and the field-book recipe grid. Crafting details already display structured requirements and Energy. Original prepared-dish assets and matching item images replace generic food presentation; no third-party geometry, code or dependencies.

## Smallest useful result and first playable demonstration
At the cookfire, select a new crop dish, inspect its ingredients/Energy, cook it with kindling, pin it and eat it. Eight dishes are the complete selected crop-food slice; fishing preparations remain in the fishing checkpoint. Jenny performs integrated player acceptance.
