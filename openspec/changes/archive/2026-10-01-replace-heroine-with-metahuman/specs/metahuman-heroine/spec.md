# Spec Delta

## Purpose

Defines the playable heroine as a photorealistic MetaHuman whose appearance options, clothing, runtime behavior and provenance meet Homestead's quality and shipping requirements.

## ADDED Requirements

### Requirement: The heroine is a photorealistic MetaHuman
The playable heroine SHALL be a MetaHuman authored in MetaHuman Creator and assembled for real-time
use, with a full face rig and high-resolution skin, eye and hair materials. Her default look SHALL
follow the recorded visual direction: a softly sculpted face with slightly sharp cheekbones, a
slim nose and moderately full lips, defined brows, light (green) eyes, and long dark-brown hair, on
a petite, curvy young-adult body of about 1.60 m. Until a long wavy groom is sourced, stock
MetaHuman grooms are acceptable.

#### Scenario: Close view in daylight
- **WHEN** the player views the heroine up close in the Appearance preview and in the woodland at midday
- **THEN** skin, eyes, brows and hair read as photorealistic MetaHuman quality, with no visible seams, card gaps, sheet-like hair or neck/torso coverage defects

#### Scenario: Firelight and night
- **WHEN** the heroine stands by a lit cookfire at night
- **THEN** skin and hair respond believably to the warm light without blown-out, flat or pale patches

### Requirement: Appearance options map to the MetaHuman
The Appearance page SHALL offer hairstyles (long, bob, ponytail), hair colours, eye colours and
skin tones that change the MetaHuman's grooms and materials in play and in the preview. Hair, eye
and skin colour SHALL be player-configurable in game; the authored look is only the default
(dark brown hair, green eyes, the heroine's authored skin). Options that cannot yet be offered
SHALL be hidden rather than shown as broken or placeholder choices. Saved appearance choices SHALL
restore on load.

#### Scenario: Change hairstyle and colour
- **WHEN** the player selects Bob and then Blonde on the Appearance page
- **THEN** the heroine's hair changes to a straight bob in a blonde shade in both the preview and the world, and the choice survives save and reload

#### Scenario: Change eye and skin colour
- **WHEN** the player selects a different eye colour and skin tone on the Appearance page
- **THEN** the heroine's irises and skin change in both the preview and the world without seams between face and body, and the choice survives save and reload

#### Scenario: Unsupported option
- **WHEN** a legacy option has no MetaHuman equivalent yet, such as an additional body preset
- **THEN** it is not offered, and loading a save that selected it falls back to the default without error

### Requirement: Clothing fits the MetaHuman body
The owned tunic, apron and footwear SHALL each have a MetaHuman-fitted garment. Existing dye
choices SHALL tint them, and they SHALL layer and animate without the body poking through at
gameplay camera distance during walking, sprinting and work actions.

#### Scenario: Wear the apron over the tunic
- **WHEN** the player equips the apron over the tunic and chops a tree
- **THEN** both garments stay attached and layered with no visible body poke-through at the gameplay camera

### Requirement: Runtime is offline and provenance is recorded
The packaged game SHALL run the MetaHuman heroine with no network, account or Epic service access.
MetaHuman, animation and garment sources SHALL be recorded with their licence terms in the
project's asset manifests and credits. MetaHuman cloud steps (texture download, auto-rigging)
SHALL be used only during authoring.

#### Scenario: Offline packaged launch
- **WHEN** the packaged Shipping game starts with networking disabled
- **THEN** the heroine renders and animates fully and no Epic login or service request occurs

#### Scenario: Licence audit
- **WHEN** a reviewer inspects `Assets/` manifests and `docs/asset-credits.md`
- **THEN** each MetaHuman, GASP and garment source lists its origin, licence and any usage limits
