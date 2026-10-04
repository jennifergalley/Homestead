## ADDED Requirements

### Requirement: First visits unlock every marked travel destination
Each location marked on the map or minimap SHALL unlock fast travel on its first physical visit, with one concise notice `Fast Travel Destination Unlocked: <Location>`. Unlocks SHALL persist through saving/loading. Map travel and signpost travel SHALL both reject undiscovered destinations without spending time. Town SHALL require a visit before either travel path is available. Initial manor presence/access and the existing manor travel arrival SHALL be preserved.

#### Scenario: Discover Town and another marked place
- **WHEN** the heroine first visits Town and a previously unvisited marked location
- **THEN** each visit shows its own unlock notice and enables that destination's map action, while repeat visits show no unlock notice

#### Scenario: Persistent gates
- **WHEN** the player saves and reloads after visiting Town but before visiting the mine
- **THEN** Town travel is available by map and signpost, mine travel remains locked, and the manor arrival is unchanged
