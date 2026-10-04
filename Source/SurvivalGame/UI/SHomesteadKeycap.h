#pragma once

#include "CoreMinimal.h"
#include "HomesteadPalette.h"
#include "HomesteadUITheme.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
// A key glyph as an ink keycap: paper fill, ink rim and a slightly deeper lower edge (the bevel), in the
// key font. The same cap in every theme (HomesteadPalette::Keycap*), so a hint never reads as a stray
// blue or pine button. Read at build time; the palette's colours update in place.
namespace Keycap
{
constexpr float Rim = 1.0f, Bevel = 2.0f, PadX = 5.0f, PadY = 0.0f, MinWidth = 20.0f;

inline TSharedRef<SWidget> Make(const FText& Label, int32 FontSize)
{
    const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    return SNew(SBorder)
        .BorderImage(White)
        .BorderBackgroundColor(HomesteadPalette::KeycapInk)
        .Padding(Rim, Rim, Rim, Rim)
        [
            SNew(SBorder)
            .BorderImage(White)
            .BorderBackgroundColor(HomesteadPalette::KeycapBevel)
            .Padding(0, 0, 0, Bevel)
            [
                SNew(SBorder)
                .BorderImage(White)
                .BorderBackgroundColor(HomesteadPalette::KeycapPaper)
                .Padding(PadX, PadY)
                .HAlign(HAlign_Center)
                [
                    SNew(SBox).MinDesiredWidth(MinWidth - 2 * PadX)
                    [
                        SNew(STextBlock).Text(Label)
                        .Font(HomesteadUITheme::KeyFont(TEXT("Bold"), FontSize))
                        .ColorAndOpacity(HomesteadPalette::KeycapInk)
                        .Justification(ETextJustify::Center)
                    ]
                ]
            ]
        ];
}
}
}
