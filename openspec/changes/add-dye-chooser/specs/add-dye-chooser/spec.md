# Spec Delta

## ADDED Requirements

### Requirement: She chooses a garment's dye with a live preview
"Change dye..." SHALL open a chooser listing each dye with a colour swatch. Moving over a dye SHALL show her wearing it without changing the save. Apply SHALL commit the chosen dye, and Cancel SHALL restore her garment as it was.

#### Scenario: Try on and apply
- **WHEN** she opens the chooser on her tunic, moves over Wine and applies it
- **THEN** she wears the tunic in Wine, and it's still Wine after closing the book and after a save and load

#### Scenario: Try on and cancel
- **WHEN** she moves over Slate and then cancels
- **THEN** she's back in her own colour and nothing was saved

### Requirement: The dye shows on the MetaHuman heroine
On the MetaHuman heroine, the equipped linen tunic's dye SHALL tint the homespun tank top and shorts, with dye 0 leaving the cloth as woven.

#### Scenario: A dyed tunic
- **WHEN** she applies Wine to the equipped tunic
- **THEN** her tank top and shorts take the Wine tint
