#include "HomesteadOriginalIcons.h"
#include "../HomesteadOriginalItemArt.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadOriginalIcons, Log, All);

TSharedPtr<FSlateDynamicImageBrush> HomesteadOriginalIcons::Load(FName Kind)
{
    const FString Key = Kind.ToString();
    if (!HomesteadOriginalItemArt::FindIcon(TCHAR_TO_UTF8(*Key))) return nullptr;
    static TMap<FName, TSharedPtr<FSlateDynamicImageBrush>> Brushes;
    if (const auto* Cached = Brushes.Find(Kind)) return *Cached;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("SurvivalGame/UI/ItemIcons"), Key + TEXT(".png"));
    if (!IFileManager::Get().FileExists(*Path))
    {
        UE_LOG(LogHomesteadOriginalIcons, Error, TEXT("Missing original item image: %s"), *Path);
        return nullptr;
    }
    auto Brush = MakeShared<FSlateDynamicImageBrush>(FName(*Path), FVector2D(256, 256));
    Brushes.Add(Kind, Brush);
    return Brush;
}
