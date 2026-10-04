# Spec Delta

## MODIFIED Requirements

### Requirement: Period crops are sold as seed at the general store
Trethewey's general store SHALL sell seed for turnips, carrots, potatoes, broad beans, strawberries and cabbage. Each crop SHALL yield its own produce, which she can eat or sell at the store. The legacy root and berry crops SHALL keep working, and the General Store SHALL also buy their produce at its existing listed price.

#### Scenario: Buying and sowing seed
- **WHEN** she buys carrot seed at the store and sows it
- **THEN** the plot grows carrots and yields carrots when harvested

#### Scenario: Picking a regrowing crop
- **WHEN** she picks a ripe broad bean plot
- **THEN** she gets bean pods, the plant stays in the ground, and it ripens again in about 3 days

#### Scenario: Selling harvested produce
- **WHEN** she harvests any supported crop and sells a chosen quantity at the open General Store counter
- **THEN** only that quantity leaves her pack and she receives quantity times the listed unit price in coins
- **AND** saving and reloading retains the remaining produce, coin balance, and sold shop stock

#### Scenario: A refused crop sale
- **WHEN** she tries to sell more produce than she carries or trades while the store is closed
- **THEN** the trade refuses without changing her produce, shop stock, or coins
