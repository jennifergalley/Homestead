# Spec Delta

## Purpose

Let Jenny correct her persisted planner feedback without losing its identity or scheduling.

## ADDED Requirements

### Requirement: Edit feedback in place

The planner SHALL offer a pencil on feedback cards that opens the existing title,
description and screenshot in the feedback form. Saving SHALL preserve the entry ID,
creation time, scheduling, ordering and removal identity, and unrelated entries.
Screenshot edits SHALL support keeping, replacing, removing and adding an image.
Edits and scheduling MUST remain agent-free.

#### Scenario: Save an edited scheduled card
- **WHEN** Jenny edits text or the screenshot and saves a scheduled feedback card
- **THEN** the same card displays the new content with its original release and priority

### Requirement: Safe cancellation and failures

Cancel SHALL discard only the form draft without writing files. Invalid, missing or
concurrently edited entries SHALL produce explicit errors without overwriting persisted
feedback; a failed save SHALL retain the form draft for recovery.

#### Scenario: Cancel a draft
- **WHEN** Jenny changes a feedback draft and chooses Cancel
- **THEN** the persisted card and screenshot remain unchanged

#### Scenario: A stale edit is rejected
- **WHEN** the entry changed after its form was opened
- **THEN** saving reports the conflict without overwriting the newer content
