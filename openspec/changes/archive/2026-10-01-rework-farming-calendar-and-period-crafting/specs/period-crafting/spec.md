# Spec Delta

## Purpose

Period crafting on the estate: a workbench, planks, fences and gates, first furniture, and
hearth dishes from her own crops.

## ADDED Requirements

### Requirement: The workbench is the station for period recipes
The heroine SHALL be able to build a workbench from hand-carried materials. Workbench recipes SHALL require a workbench within reach. Hafting tools SHALL remain possible by hand.

#### Scenario: Away from the workbench
- **WHEN** she tries to craft a fence rail with no workbench nearby
- **THEN** the recipe shows that a workbench is required and nothing is consumed

### Requirement: Timber is sawn into planks
A sawhorse SHALL turn one timber into four planks, spending energy once per saw.

#### Scenario: Saw planks
- **WHEN** she saws at the sawhorse carrying 2 timber
- **THEN** one timber is consumed and four planks are added

### Requirement: Fences and gates can be built on owned land
Post-and-rail fence runs SHALL snap end to end, and gates SHALL open and close. Both SHALL block the heroine's movement when closed, SHALL persist across save and load, and SHALL be placeable only on owned land.

#### Scenario: Gate state persists
- **WHEN** she opens a gate, saves and reloads
- **THEN** the gate is still open

#### Scenario: Fence outside the estate
- **WHEN** she previews a fence run beyond the estate boundary
- **THEN** placement is refused with the reason "Outside your estate"

### Requirement: First furniture can be crafted and placed
The workbench SHALL craft a stool, table, chair and shelf, each placeable on foundations, including in the standing room.

#### Scenario: Furnish the standing room
- **WHEN** she crafts a table and places it in the standing room
- **THEN** the table stands on the floor and stays there after reload

### Requirement: Hearth dishes from her own crops
The hearth SHALL cook period dishes from estate crops. Each dish SHALL be a meal that restores its
defined Energy amount and grants the same flat 3 game hours of Well fed.

#### Scenario: Vegetable stew
- **WHEN** she cooks vegetable stew with a potato, carrot, turnip and leek and eats it
- **THEN** her energy rises more than with roast potatoes, and both meals grant 3 game hours of Well fed

### Requirement: Craft recipes are grouped
The Craft page SHALL group recipes into Tools, Stations, Farm, Furniture and Cooking categories, reachable by mouse and by controller directional focus.

#### Scenario: Controller browsing
- **WHEN** the player moves through Craft categories using only a controller
- **THEN** each category's recipes are reachable, and LB/RB still switch the field book's main tabs
