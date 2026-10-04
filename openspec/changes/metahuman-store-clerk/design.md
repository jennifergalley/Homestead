# MetaHuman Store Clerk: design

- Character source: `/Game/Characters/Clerk_MH/MHC_Clerk`, duplicated from a stock male preset,
  assembled (UE Optimized) into `/Game/Characters/Clerk_MH/Assembled` and sharing the heroine's
  `metahuman_base_skel`, so the heroine's MetaHuman clips can play on him.
- Garments: `Scripts\Blender\Recipes\clerk_outfit.py` fits them to his exported body; they are
  skinned to `metahuman_base_skel` and follow the body by leader pose.
- Runtime: `AHomesteadShopkeeper` builds body, face (ABP_Face), garments, grooms and a LOD sync,
  the way `AHomesteadCharacter::LoadMetaHumanStack` does. If an asset is missing, the legacy
  stand-in stays so the store still works.
- Provenance: `Assets\Characters\MetaHumanClerk\provenance.json` and `docs\asset-credits.md`.
