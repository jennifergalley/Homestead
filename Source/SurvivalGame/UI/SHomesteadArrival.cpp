#include "SHomesteadArrival.h"

#include "Engine/FontFace.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/CompositeFont.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
FSlateFontInfo DisplayFont(float Size, bool bItalic)
{
    static TSharedPtr<const FCompositeFont> Faces[2];
    static bool bTried[2] = {false, false};
    const int32 Style = bItalic ? 1 : 0;
    if (!bTried[Style])
    {
        bTried[Style] = true;
        const TCHAR* Path = bItalic ? TEXT("/Game/SurvivalGame/UI/Fonts/EBGaramond-Italic.EBGaramond-Italic")
                                    : TEXT("/Game/SurvivalGame/UI/Fonts/EBGaramond.EBGaramond");
        if (UFontFace* Face = LoadObject<UFontFace>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
        {
            // The composite font refers to the face by pointer; keep it loaded for the session.
            Face->AddToRoot();
            TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
            FTypefaceEntry& Entry = Composite->DefaultTypeface.Fonts.Emplace_GetRef(TEXT("Regular"));
            Entry.Font = FFontData(Face);
            Faces[Style] = Composite;
        }
    }
    if (Faces[Style].IsValid()) return FSlateFontInfo(Faces[Style], Size);
    return FCoreStyle::GetDefaultFontStyle(bItalic ? "Italic" : "Light", FMath::RoundToInt(Size));
}

float SHomesteadArrival::OpacityAt(double Elapsed)
{
    if (Elapsed <= 0.0) return 0.0f;
    if (Elapsed < FadeIn) return static_cast<float>(FMath::SmoothStep(0.0, 1.0, Elapsed / FadeIn));
    if (Elapsed < FadeIn + Hold) return 1.0f;
    const double Out = (Elapsed - FadeIn - Hold) / FadeOut;
    return Out >= 1.0 ? 0.0f : static_cast<float>(1.0 - FMath::SmoothStep(0.0, 1.0, Out));
}

float SHomesteadArrival::CurrentOpacity() const
{
    return OpacityAt(FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() - Started : 0.0);
}

bool SHomesteadArrival::IsFinished() const
{
    return FSlateApplication::IsInitialized()
        && FSlateApplication::Get().GetCurrentTime() - Started >= FadeIn + Hold + FadeOut;
}

void SHomesteadArrival::Construct(const FArguments& Args)
{
    Started = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : 0.0;
    SetVisibility(EVisibility::HitTestInvisible);
    SetCanTick(false);
    const auto Ink = [this](FLinearColor Color)
    {
        return TAttribute<FSlateColor>::CreateLambda([this, Color]()
        {
            FLinearColor Faded = Color;
            Faded.A *= CurrentOpacity();
            return FSlateColor(Faded);
        });
    };
    const auto Shadow = [this]()
    {
        return TAttribute<FLinearColor>::CreateLambda([this]() { return FLinearColor(0, 0, 0, 0.6f * CurrentOpacity()); });
    };
    ChildSlot
    .HAlign(HAlign_Center)
    .VAlign(VAlign_Center)
    [
        SNew(SBox)
        .Padding(FMargin(0, 0, 0, 180))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Args._Title))
                .Font(DisplayFont(64))
                .ColorAndOpacity(Ink(FLinearColor(0.96f, 0.91f, 0.8f)))
                .ShadowOffset(FVector2D(2, 2))
                .ShadowColorAndOpacity(Shadow())
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Args._Subtitle))
                .Font(DisplayFont(30, true))
                .ColorAndOpacity(Ink(FLinearColor(0.9f, 0.84f, 0.72f)))
                .ShadowOffset(FVector2D(1.5f, 1.5f))
                .ShadowColorAndOpacity(Shadow())
            ]
        ]
    ];
}
}
