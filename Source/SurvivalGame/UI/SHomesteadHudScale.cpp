#include "SHomesteadHudScale.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/UserInterfaceSettings.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SScaleBox.h"

namespace HomesteadMenus
{
namespace
{
FIntPoint HudScaleViewportSize()
{
    const FViewport* Viewport = GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport : nullptr;
    return Viewport ? Viewport->GetSizeXY() : FIntPoint(1920, 1080);
}
}

FVector2D FullScreenLogicalSize()
{
    const FIntPoint Size = HudScaleViewportSize();
    if (Size.X <= 0 || Size.Y <= 0) return FVector2D(1280, 720);
    const float Scale = 1.5f * HudCanvasScale(static_cast<float>(Size.Y));
    return FVector2D(FMath::Max(1280.0f, Size.X / Scale), FMath::Max(720.0f, Size.Y / Scale));
}

void SHomesteadHudScale::Construct(const FArguments& Args)
{
    // A first guess from the DPI curve until the first tick reads the real ambient scale.
    const FIntPoint Size = HudScaleViewportSize();
    const float Dpi = FMath::Max(0.01f, GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(Size));
    Scale = HudCanvasScale(static_cast<float>(FMath::Max(1, Size.Y))) / Dpi;
    ChildSlot
    [
        SNew(SScaleBox).Stretch(EStretch::UserSpecified)
        .UserSpecifiedScale_Lambda([this]() { return Scale; })
        [ Args._Content.Widget ]
    ];
}

void SHomesteadHudScale::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);
    // AllottedGeometry.Scale is the scale Slate already applies here (window DPI and the UI DPI
    // curve); the scale box inside makes up the rest.
    const float Ambient = FMath::Max(0.01f, AllottedGeometry.Scale);
    Scale = HudCanvasScale(static_cast<float>(FMath::Max(1, HudScaleViewportSize().Y))) / Ambient;
}
}
