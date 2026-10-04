## Purpose

Provide a readable animation-led bank fishing sequence with fair bounded reactions, safe cancellation and catches awarded only on successful authored contact.

## ADDED Requirements

### Requirement: Fishing timing is variable, bounded and tool-driven
Fishing SHALL vary the wait to bite and strike timing between casts within bounded readable windows. Click or the controller tool input SHALL cast and respond to cues; E or controller interact SHALL NOT cast, hook or strike. Early, late, interrupted or cancelled attempts SHALL award no fish and allow another cast.

#### Scenario: Follow fishing cues
- **WHEN** the player casts and responds inside the bite and strike windows using tool input
- **THEN** the original fishing animations visibly lead the sequence toward a successful catch

#### Scenario: Miss or cancel
- **WHEN** the player misses a timing window, walks away, changes tools or opens a menu
- **THEN** the sequence cancels or fails without yielding fish and returns safely to ordinary play

### Requirement: A successful catch is awarded on authored contact
Successfully timed input SHALL authorize the catch animation, not grant a fish immediately. The fish SHALL enter the pack only at that animation's successful authored contact, after capacity, habitat and player validity are checked again. Timers alone SHALL never grant a catch.

#### Scenario: Reach the catch contact
- **WHEN** a successful sequence reaches its authored catch contact with a valid player and free capacity
- **THEN** exactly one habitat-appropriate fish is awarded and repeated contact cannot duplicate it

#### Scenario: Contact never arrives
- **WHEN** the presentation is interrupted or the successful contact is never reported
- **THEN** no fish is awarded and the bounded sequence expires or cancels safely
