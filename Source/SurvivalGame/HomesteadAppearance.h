#pragma once

#include "CoreMinimal.h"

struct FHomesteadAppearance
{
    int32 HairStyle = 0;
    int32 HairColor = 0;
    int32 SkinTone = 0;
    int32 EyeColor = 0;
    int32 TunicColor = 0;
    int32 Outfit = 0;
    int32 BodyPreset = 0;

    bool IsValid() const
    {
        return HairStyle >= 0 && HairStyle < 3
            && HairColor >= 0 && HairColor < 4
            && SkinTone >= 0 && SkinTone < 4
            && EyeColor >= 0 && EyeColor < 4
            && TunicColor >= 0 && TunicColor < 4
            && Outfit >= 0 && Outfit < 2
            && BodyPreset >= 0 && BodyPreset < 3;
    }
};

namespace HomesteadLook
{
    const TCHAR* HairStyleName(int32 Index);
    const TCHAR* HairColorName(int32 Index);
    const TCHAR* SkinToneName(int32 Index);
    const TCHAR* EyeColorName(int32 Index);
    const TCHAR* TunicColorName(int32 Index);
    const TCHAR* OutfitName(int32 Index);
    const TCHAR* BodyPresetName(int32 Index);
    FLinearColor HairTint(int32 Index);
    FLinearColor SkinTint(int32 Index);
    FLinearColor IrisColor(int32 Index);
    FLinearColor TunicTint(int32 Index);
}
