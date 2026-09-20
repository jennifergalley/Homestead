#include "HomesteadAppearance.h"

namespace
{
template <typename T, SIZE_T N>
const T& Choice(const T (&Values)[N], int32 Index)
{
    checkf(Index >= 0 && Index < static_cast<int32>(N), TEXT("Invalid validated appearance index."));
    return Values[Index];
}
}

namespace HomesteadLook
{
const TCHAR* HairStyleName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Long waves"), TEXT("Straight bob"), TEXT("Ponytail")};
    return Choice(Values, Index);
}
const TCHAR* HairColorName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Chestnut"), TEXT("Dark brown"), TEXT("Black"), TEXT("Copper")};
    return Choice(Values, Index);
}
const TCHAR* SkinToneName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Natural"), TEXT("Warm"), TEXT("Deep"), TEXT("Light")};
    return Choice(Values, Index);
}
const TCHAR* EyeColorName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Blue"), TEXT("Green"), TEXT("Hazel"), TEXT("Grey")};
    return Choice(Values, Index);
}
const TCHAR* TunicColorName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Moss"), TEXT("Wine"), TEXT("Slate"), TEXT("Flax")};
    return Choice(Values, Index);
}
const TCHAR* OutfitName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Linen tunic"), TEXT("Tunic and apron")};
    return Choice(Values, Index);
}
const TCHAR* BodyPresetName(int32 Index)
{
    static const TCHAR* Values[] = {TEXT("Preferred"), TEXT("Willow"), TEXT("Hazel")};
    return Choice(Values, Index);
}
FLinearColor HairTint(int32 Index)
{
    static const FLinearColor Values[] = {
        FLinearColor::White, FLinearColor(0.45f, 0.36f, 0.32f),
        FLinearColor(0.15f, 0.14f, 0.13f), FLinearColor(1.15f, 0.68f, 0.43f)};
    return Choice(Values, Index);
}
FLinearColor SkinTint(int32 Index)
{
    static const FLinearColor Values[] = {
        FLinearColor::White, FLinearColor(0.9f, 0.76f, 0.65f),
        FLinearColor(0.48f, 0.32f, 0.23f), FLinearColor(1.08f, 1.06f, 1.04f)};
    return Choice(Values, Index);
}
FLinearColor IrisColor(int32 Index)
{
    static const FLinearColor Values[] = {
        FLinearColor(0.25f, 0.47f, 0.53f), FLinearColor(0.15f, 0.43f, 0.2f),
        FLinearColor(0.42f, 0.28f, 0.11f), FLinearColor(0.38f, 0.4f, 0.42f)};
    return Choice(Values, Index);
}
FLinearColor TunicTint(int32 Index)
{
    static const FLinearColor Values[] = {
        FLinearColor::White, FLinearColor(1.1f, 0.17f, 0.56f),
        FLinearColor(0.5f, 0.6f, 2.4f), FLinearColor(2.75f, 1.78f, 2.61f)};
    return Choice(Values, Index);
}
}
