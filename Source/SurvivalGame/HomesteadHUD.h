#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HomesteadHUD.generated.h"

class AHomesteadController;

namespace HomesteadHudLayout
{
// The calendar panel's place in Canvas HUD units (1080-line layout); the vitals stack sits under it.
inline constexpr float CalendarTop = 26, CalendarHeight = 100;
}

UCLASS()
class SURVIVALGAME_API AHomesteadHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool BookTextFits(int32 Page) const { return RenderedBookPage == Page && !BookMeasurements.IsEmpty() && BookTextOverflow.IsEmpty(); }
    FString BookTextMeasurements() const { return BookMeasurements; }
    bool FeedbackFullText() const;
    bool FeedbackOverlaps() const;
    bool FeedbackInsideViewport() const;
    FString FeedbackMeasurements() const;
    FString FeedbackCriticalGeometry() const;
    FString FeedbackSource() const { return ToastSource; }
    FLinearColor FeedbackColor() const { return ToastColor; }

private:
    int32 RenderedBookPage = -1;
    FString BookTextOverflow;
    FString BookMeasurements;
    bool bMeasureFeedback = false, bDrawingToast = false;
    // While set, Write / TextWidth / WrappedLines set words in the field book's display serif (the toast).
    bool bNoticeText = false;
    FString ToastSource;
    TArray<FString> ToastLines;
    TArray<FBox2D> ToastTextBounds;
    TArray<TPair<FString, FBox2D>> FeedbackProtected;
    FBox2D ToastBounds;
    FVector2D FeedbackViewport = FVector2D::ZeroVector;
    FLinearColor ToastColor;
    void ProtectFeedback(const FString& RegionName, float X, float Y, float Width, float Height);
    void MeasureBookLine(const FString& Text, float Width, float Size, const TCHAR* TextRole);
    float UiScale = 1;
    float ViewWidth = 1920;
    float ViewHeight = 1080;
    void Write(const FString& Text, float X, float Y, float Size, FLinearColor Color);
    TArray<FString> WrappedLines(const FString& Text, float Width, float Size);
    void Wrap(const FString& Text, float X, float Y, float Width, float Size, FLinearColor Color, int MaxLines = 3);
    void Panel(float X, float Y, float Width, float Height, FLinearColor Color);
    // The calendar: a sun/moon dial for the time of day, "Spring / Day 2", the 12-hour time and a weather icon.
    void DrawCalendar(const AHomesteadController& PC, float X, float Y);
    // Procedural icon primitives in HUD units (scaled by UiScale).
    void Disc(float CX, float CY, float Radius, FLinearColor Color, int32 Segments = 28);
    void Band(float CX, float CY, float InnerRadius, float OuterRadius, float FromRadians, float ToRadians, FLinearColor Color, int32 Segments = 32);
    void Stroke(float X0, float Y0, float X1, float Y1, float Thickness, FLinearColor Color);
    void SunIcon(float CX, float CY, float Radius, FLinearColor Color);
    void MoonIcon(float CX, float CY, float Radius, FLinearColor Color, FLinearColor Behind);
    void RainIcon(float CX, float CY, float Size);
    void DrawBook(const AHomesteadController& PC);
    // The focus prompt at the top centre, under the compass: key badges and verbs ("E  Gather") with
    // the target's name small above, on the shared parchment notice. Returns its bottom edge (HUD
    // units), or 0 when nothing is focused, so the toast can stack under it.
    float DrawInteractCue(const AHomesteadController& PC);
    // The parchment notice card (UI/HomesteadNoticeStyle.h) in HUD units; a rust frame for errors.
    void NoticeCard(float X, float Y, float Width, float Height, bool bError);
    float TextWidth(const FString& Text, float Size) const;
    void DrawAppearanceBook(const AHomesteadController& PC);
};
