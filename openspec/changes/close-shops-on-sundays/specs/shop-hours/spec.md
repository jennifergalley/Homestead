# Spec Delta

## ADDED Requirements

### Requirement: Shops close all day Sunday
Every shop SHALL be closed for the whole of the calendar day named Sunday, from 06:00 Sunday to 06:00 Monday, and keep its ordinary hours on the other six days. The rule SHALL come from the game clock alone, so no save data changes.

#### Scenario: Sunday at noon
- **WHEN** she tries to trade at the general store's counter at 12:00 on a Sunday
- **THEN** the trade is refused with "Closed today (Sunday) - opens Monday at 8 AM"
- **AND** the shopkeeper is off duty and the door's board reads "CLOSED / on Sundays"

#### Scenario: Saturday and Monday hours
- **WHEN** it is 17:59 on a Saturday, or 08:00 on a Monday
- **THEN** the store is open
- **AND** at 18:00 on a Saturday it is closed, with "Closed - opens Monday at 8 AM"

### Requirement: No waiting at the door through a closed day
Waiting by a shop's door SHALL pass only an ordinary night's closure, from closing to opening. Through a closed day the door SHALL offer no wait, and a request to wait SHALL be refused without passing time, naming the day and hour it opens.

#### Scenario: Wait on a Sunday
- **WHEN** she asks to wait at the general store's door at 12:00 on a Sunday
- **THEN** she is told "The general store is closed on Sundays. It opens Monday at 8 AM."
- **AND** no time passes

### Requirement: The walk to town warns of a Sunday
The walk-to-town summary SHALL say when she'd arrive on a Sunday, and that the general store is closed all day.

#### Scenario: Set out late on Saturday
- **WHEN** she plans the walk to town at 1 AM, arriving about 8 AM on a Sunday
- **THEN** the summary says "You'd arrive on a Sunday, when the general store is closed all day (it opens Monday at 8 AM)."
