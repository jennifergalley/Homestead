#include "HomesteadHUD.h"
#include "Engine/Canvas.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString FeedbackCharacters(const FString& Text)
{
    TArray<FString> Words; Text.ParseIntoArrayWS(Words);
    // Hard wrapping a long token adds a line break, not a missing source character.
    return FString::Join(Words, TEXT(""));
}
bool IntersectsFeedback(const FBox2D& A, const FBox2D& B)
{
    return A.Min.X < B.Max.X && A.Max.X > B.Min.X && A.Min.Y < B.Max.Y && A.Max.Y > B.Min.Y;
}
TSharedRef<FJsonObject> FeedbackBox(const FBox2D& Box)
{
    auto Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("left"), Box.Min.X); Result->SetNumberField(TEXT("top"), Box.Min.Y);
    Result->SetNumberField(TEXT("right"), Box.Max.X); Result->SetNumberField(TEXT("bottom"), Box.Max.Y);
    return Result;
}
}

void AHomesteadHUD::ProtectFeedback(const FString& RegionName, float X, float Y, float Width, float Height)
{
    if (bMeasureFeedback)
        FeedbackProtected.Emplace(RegionName, FBox2D(FVector2D(X, Y) * UiScale, FVector2D(X + Width, Y + Height) * UiScale));
}
bool AHomesteadHUD::FeedbackFullText() const
{
    return bMeasureFeedback && !ToastSource.IsEmpty() && !ToastLines.IsEmpty()
        && FeedbackCharacters(ToastSource) == FeedbackCharacters(FString::Join(ToastLines, TEXT(" ")));
}
bool AHomesteadHUD::FeedbackOverlaps() const
{
    if (ToastSource.IsEmpty()) return false;
    for (const auto& Box : FeedbackProtected) if (IntersectsFeedback(ToastBounds, Box.Value)) return true;
    return false;
}
bool AHomesteadHUD::FeedbackInsideViewport() const
{
    if (!bMeasureFeedback || FeedbackViewport.X <= 0 || FeedbackViewport.Y <= 0 || ToastSource.IsEmpty()) return false;
    const auto Inside = [this](const FBox2D& B)
    { return B.Min.X >= 0 && B.Min.Y >= 0 && B.Max.X <= FeedbackViewport.X && B.Max.Y <= FeedbackViewport.Y; };
    if (!Inside(ToastBounds)) return false;
    for (const auto& Box : ToastTextBounds)
        if (!Inside(Box) || Box.Min.X < ToastBounds.Min.X || Box.Max.X > ToastBounds.Max.X
            || Box.Min.Y < ToastBounds.Min.Y || Box.Max.Y > ToastBounds.Max.Y) return false;
    return true;
}
FString AHomesteadHUD::FeedbackCriticalGeometry() const
{
    FString Result;
    for (const auto& Box : FeedbackProtected)
        Result += FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f;"), Box.Value.Min.X, Box.Value.Min.Y, Box.Value.Max.X, Box.Value.Max.Y);
    return Result;
}
FString AHomesteadHUD::FeedbackMeasurements() const
{
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("source"), ToastSource);
    Object->SetBoolField(TEXT("fullTextRendered"), FeedbackFullText());
    Object->SetBoolField(TEXT("overlap"), FeedbackOverlaps());
    Object->SetBoolField(TEXT("insideViewport"), FeedbackInsideViewport());
    Object->SetNumberField(TEXT("viewportWidth"), FeedbackViewport.X);
    Object->SetNumberField(TEXT("viewportHeight"), FeedbackViewport.Y);
    Object->SetObjectField(TEXT("toastPanel"), FeedbackBox(ToastBounds));
    Object->SetStringField(TEXT("textColor"), ToastColor.ToString());
    TArray<TSharedPtr<FJsonValue>> Lines, Protected;
    for (int32 I = 0; I < ToastLines.Num(); ++I)
    {
        auto Line = FeedbackBox(ToastTextBounds[I]); Line->SetStringField(TEXT("text"), ToastLines[I]);
        Lines.Add(MakeShared<FJsonValueObject>(Line));
    }
    for (const auto& Box : FeedbackProtected)
    {
        auto Entry = FeedbackBox(Box.Value); Entry->SetStringField(TEXT("role"), Box.Key);
        Entry->SetBoolField(TEXT("overlap"), !ToastSource.IsEmpty() && IntersectsFeedback(ToastBounds, Box.Value));
        Protected.Add(MakeShared<FJsonValueObject>(Entry));
    }
    Object->SetArrayField(TEXT("drawnToastLines"), Lines); Object->SetArrayField(TEXT("protected"), Protected);
    FString Text; FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text)); return Text;
}
