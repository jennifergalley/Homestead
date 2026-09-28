#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SCompoundWidget.h"

namespace HomesteadMenus
{
// The period display face (EB Garamond, SIL OFL; /Game/SurvivalGame/UI/Fonts) for titles and the
// journal, falling back to the engine's Roboto Light when it isn't imported.
FSlateFontInfo DisplayFont(float Size, bool bItalic = false);

// "{Estate}" over "Spring, 1851": fades in for a second, holds three, fades out. It never takes
// input or focus, so she can move from the first frame.
class SHomesteadArrival : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadArrival) {}
        SLATE_ARGUMENT(FString, Title)
        SLATE_ARGUMENT(FString, Subtitle)
    SLATE_END_ARGS()

    static constexpr double FadeIn = 1.0, Hold = 3.0, FadeOut = 1.0;
    void Construct(const FArguments& Args);
    bool IsFinished() const;
    // Opacity at `Elapsed` seconds into the card.
    static float OpacityAt(double Elapsed);

private:
    double Started = 0.0;
    float CurrentOpacity() const;
};
}
