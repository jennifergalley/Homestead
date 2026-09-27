#pragma once

#include "CoreMinimal.h"

namespace HomesteadLook
{
    constexpr int32 HairColorCount = 5;
    // MetaHuman hairstyles (homestead_agent.metahuman_hair STYLES, same order).
    constexpr int32 MetaHairCount = 9;
}

struct FHomesteadAppearance
{
    int32 HairStyle = 0;
    int32 HairColor = 0;
    int32 SkinTone = 0;
    int32 EyeColor = 0;
    int32 TunicColor = 0;
    int32 Outfit = 0;
    int32 BodyPreset = 0;
    // The MetaHuman heroine's hairstyle; HairStyle keeps the nearest legacy modular style.
    int32 MetaHair = 0;

    bool IsValid() const
    {
        return HairStyle >= 0 && HairStyle < 3
            && HairColor >= 0 && HairColor < HomesteadLook::HairColorCount
            && SkinTone >= 0 && SkinTone < 4
            && EyeColor >= 0 && EyeColor < 4
            && TunicColor >= 0 && TunicColor < 4
            && Outfit >= 0 && Outfit < 2
            && BodyPreset >= 0 && BodyPreset < 3
            && MetaHair >= 0 && MetaHair < HomesteadLook::MetaHairCount;
    }
};

namespace HomesteadLook
{
    const TCHAR* HairStyleName(int32 Index);
    const TCHAR* MetaHairName(int32 Index);
    // Groom asset name, e.g. Hair_S_LowPonytail.
    const TCHAR* MetaHairGroom(int32 Index);
    // The legacy modular style closest to a MetaHuman style (long, bob or ponytail).
    int32 LegacyHairStyle(int32 MetaHair);
    // The MetaHuman style a legacy save's long, bob or ponytail choice maps to.
    int32 MetaHairForLegacy(int32 HairStyle);
    // MetaHuman hair shader melanin and redness for a hair colour.
    FVector2D HairPigment(int32 Index);
    const TCHAR* HairColorName(int32 Index);
    const TCHAR* SkinToneName(int32 Index);
    const TCHAR* EyeColorName(int32 Index);
    const TCHAR* TunicColorName(int32 Index);
    const TCHAR* OutfitName(int32 Index);
    const TCHAR* BodyPresetName(int32 Index);
    FLinearColor HairTint(int32 Index);
    FLinearColor NeutralHairTint(int32 Index);
    FLinearColor SkinTint(int32 Index);
    FLinearColor IrisColor(int32 Index);
    FLinearColor TunicTint(int32 Index);
}
