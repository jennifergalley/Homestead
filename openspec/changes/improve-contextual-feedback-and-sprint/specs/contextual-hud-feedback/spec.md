# Spec Delta

## Purpose

Defines a quiet gameplay HUD that appears only when useful and reports exact inventory gains without repetitive explanatory prose.

## ADDED Requirements

### Requirement: Interaction guidance is target-gated
The gameplay HUD SHALL hide the context panel when the player has no nearby focused interaction. When a resource, plot, work structure, or water interaction is focused, the HUD SHALL show only the relevant current-device interaction guidance.

#### Scenario: Walking through empty woodland
- **WHEN** the player is not close enough to interact with a resource, plot, work structure, or water
- **THEN** no `Woodland` context panel, till instruction, or other interaction tooltip is displayed

#### Scenario: Approach a usable resource
- **WHEN** the player moves within interaction range of a usable resource
- **THEN** the resource context and correct controller or keyboard interaction hint become visible

#### Scenario: Leave interaction range
- **WHEN** the player moves away from the focused target
- **THEN** the contextual guidance disappears without leaving stale resource text

### Requirement: Permanent shortcut guide is removed
The gameplay HUD MUST NOT display the persistent top-right field-book, crafting, building, or camera-distance controller/keyboard guide. Menu and camera bindings remain functional and may be taught through onboarding or settings rather than permanent gameplay chrome.

#### Scenario: Ordinary gameplay with or without focus
- **WHEN** the player walks, focuses an interaction, gathers, builds, or changes input device
- **THEN** the top-right permanent shortcut guide remains absent

### Requirement: Gathering reports exact carried gains
After a successful resource gather, the HUD SHALL show a transient pickup toast that lists each exact item quantity added to the carried inventory. The toast MUST NOT say that the patch will regrow.

#### Scenario: Gather one-item yield
- **WHEN** gathering succeeds and adds one item kind
- **THEN** the toast names that item and its exact positive quantity

#### Scenario: Gather multi-item yield
- **WHEN** gathering succeeds and adds multiple item kinds
- **THEN** the toast lists every added item kind and exact quantity once

#### Scenario: Gather is rejected
- **WHEN** gathering fails because of distance, capacity, missing tools, depletion, or renewal
- **THEN** the existing clear error feedback remains visible and no success-shaped pickup toast appears

### Requirement: Feedback remains readable and bounded
Context prompts and pickup/error toasts SHALL preserve the existing responsive HUD layout, current-device prompt switching, and non-overlap protections at supported 720p and 4K views.

#### Scenario: Pickup while gameplay HUD is visible
- **WHEN** a pickup toast appears during ordinary gameplay
- **THEN** it remains inside the viewport and does not overlap the calendar, survival meters, contextual guidance, or protected menu geometry

#### Scenario: Device changes near a target
- **WHEN** the player switches between controller and keyboard/mouse while a target remains focused
- **THEN** the visible interaction hint updates to the active device without duplicating panels
