# Design

## Context

See proposal. Canvas `DrawInteractCue` renders every title at size 17 in muted ink; it caps measured width at 520 but does not wrap the drawn title.

## Goals / Non-Goals

**Goals:** raise chest names to size 32 in primary ink, with measured multiline fitting.

**Non-Goals:** change names, storage, hint-learning or inputs; enlarge unrelated focus titles.

## Decisions

Add a cheap focused-chest query; reuse the existing EB Garamond measurement and notice card. Bound title and action rows to the same band left of the vitals. Wrap rather than truncate or shrink the name. No new widget/assets/dependencies.

## Lanes and ownership

HUD owns the Canvas focus layout and query declaration. Farming owns seed/fishing focus behavior; preserve its pending source. Notice layout is shared with `unify-hud-notices`.

## Risks / Trade-offs

720p uses the existing serif floor: explicitly floor chest titles above action text so hierarchy survives. Integrated compile and player visual checks remain held, not assumed passed.
