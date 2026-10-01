# Spec Delta

## ADDED Requirements

### Requirement: She can name a chest and see its name before opening it
An owned chest SHALL accept a custom name of up to 24 characters that persists through saving and loading, SHALL show that name in its world prompt and storage view, and SHALL return to "Storage chest" when the name is cleared. Saves without names SHALL load unchanged.

#### Scenario: Naming the linen chest
- **WHEN** she names a chest "Linen press", saves and loads
- **THEN** walking up to it shows "Linen press" before she opens it

#### Scenario: An older save
- **WHEN** she loads a save from before chest names
- **THEN** every chest is a "Storage chest" and nothing else changes

