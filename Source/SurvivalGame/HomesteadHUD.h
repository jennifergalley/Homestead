#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HomesteadHUD.generated.h"

class AHomesteadController;

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
    void Meter(const FString& Label, double Value, float X, float Y, FLinearColor Color);
    void DrawBook(const AHomesteadController& PC);
    void DrawAppearanceBook(const AHomesteadController& PC);
};
