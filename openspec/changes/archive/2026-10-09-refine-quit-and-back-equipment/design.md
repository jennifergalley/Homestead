# Design

## Context

See proposal.md. SetBackpackShown, backpack persistence and character visibility already exist; Settings already uses compact 13-16 point theme sizes.

## Goals / Non-Goals

Use existing visibility state rather than inventing a wearable or changing equipment enum semantics. No strap/anatomy edits, capacity changes or exit behavior changes.

## Decisions

Add a Back UI selection alongside existing equipment focus/popup handling; route it through a small controller feature method that calls the existing simulation command and refreshes gameplay/portrait presentation. Reuse saved backpackShown instead of a new preference.

Use quit-specific compact sizing, preserving other modal controls.

## Lanes and ownership

Gameplay UI owns menu/controller/backpack tests. Fishing Art owns fishing character header additions; existing backpack support avoids a shared-header collision. Integration alone merges/packages.

## Risks / Trade-offs

Portrait refresh while paused -> follow existing equipment refresh path. New Back focus index -> include pointer, keyboard and controller navigation in the compact control path.

## Migration Plan

No new serialization. Existing saves retain their visibility and upgrade flags; revert the UI change without touching saved ownership.
