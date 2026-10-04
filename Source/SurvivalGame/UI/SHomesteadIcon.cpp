#include "SHomesteadIcon.h"
#include "HomesteadPalette.h"
#include "HomesteadOriginalIcons.h"
#include "../HomesteadOriginalItemArt.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#include <initializer_list>

namespace HomesteadIcons
{
namespace
{
    const FLinearColor& Cream = HomesteadPalette::Cream;
    const FLinearColor& Pine = HomesteadPalette::Pine;
    const FLinearColor Charcoal(0.045f, 0.035f, 0.025f);
    const FLinearColor Wood(0.49f, 0.29f, 0.13f);
    const FLinearColor LeafGreen(0.32f, 0.52f, 0.19f);
    const FLinearColor Berry(0.67f, 0.16f, 0.25f);
    const FLinearColor RootOrange(0.86f, 0.46f, 0.21f);
    const FLinearColor WaterBlue(0.36f, 0.65f, 0.68f);
    const FLinearColor StoneGray(0.53f, 0.59f, 0.55f);
const FLinearColor Rust(0.55f, 0.25f, 0.11f);
const FLinearColor Iron(0.34f, 0.35f, 0.36f);
const FLinearColor HayGold(0.86f, 0.72f, 0.36f);

    class FIconPainter
    {
    public:
        FIconPainter(const FGeometry& InGeometry, FSlateWindowElementList& InElements,
            int32 InLayer, const FLinearColor& InStyleTint, const FLinearColor& InAccent,
            float InDesaturation, ESlateDrawEffect InEffects)
            : Accent(Desaturate(InAccent, InDesaturation)), Geometry(InGeometry),
              Elements(InElements), Layer(InLayer), StyleTint(InStyleTint), Effects(InEffects)
        {
            Desaturation = FMath::Clamp(InDesaturation, 0.0f, 1.0f);
            const FVector2D Size = Geometry.GetLocalSize();
            Scale = FMath::Max(0.0f, static_cast<float>(FMath::Min(Size.X, Size.Y) / 56.0));
            Origin = (Size - FVector2D(56.0f, 56.0f) * Scale) * 0.5f;
            StyleTint.A *= InAccent.A;
        }

        void Line(std::initializer_list<FVector2D> Points, FLinearColor Color, float Width = 2.0f)
        {
            TArray<FVector2D> Path;
            Path.Reserve(static_cast<int32>(Points.size()));
            for (const FVector2D& Point : Points)
            {
                Path.Add(Point);
            }
            Stroke(Path, Color, Width);
        }

        void Rect(float X, float Y, float Width, float Height, FLinearColor Color)
        {
            FSlateDrawElement::MakeBox(Elements, ++Layer,
                Geometry.ToPaintGeometry(FVector2D(Width, Height) * Scale,
                    FSlateLayoutTransform(Origin + FVector2D(X, Y) * Scale)),
                FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
                Effects | ESlateDrawEffect::NoPixelSnapping, Styled(Color));
        }

        void Shape(std::initializer_list<FVector2D> Points, FLinearColor Color)
        {
            TArray<FVector2D> Path;
            Path.Reserve(static_cast<int32>(Points.size()) + 1);
            for (const FVector2D& Point : Points)
            {
                Path.Add(Point);
            }
            Fill(Path, Color);
        }

        void Disc(float X, float Y, float Radius, FLinearColor Color)
        {
            TArray<FVector2D> Path;
            Path.Reserve(17);
            for (int32 Index = 0; Index < 16; ++Index)
            {
                const float Angle = Index * (2.0f * PI / 16.0f);
                Path.Add(FVector2D(X + FMath::Cos(Angle) * Radius, Y + FMath::Sin(Angle) * Radius));
            }
            Fill(Path, Color);
        }

        void Leaf(FVector2D Base, FVector2D Tip, float Width = 4.0f)
        {
            const FVector2D Direction = (Tip - Base).GetSafeNormal();
            const FVector2D Side(-Direction.Y * Width, Direction.X * Width);
            const FVector2D Middle = (Base + Tip) * 0.5;
            Shape({Base, Middle + Side, Tip, Middle - Side}, LeafGreen);
            Line({Base, Tip}, Cream, 1.0f);
        }

        void Root(float X, float Y, FLinearColor Color)
        {
            Shape({{X - 4, Y}, {X + 3, Y - 2}, {X + 6, Y + 3},
                {X + 3, Y + 10}, {X - 4, Y + 17}, {X - 2, Y + 7}}, Color);
            Line({{X - 1, Y + 4}, {X + 3, Y + 5}}, Pine, 1.5f);
            Line({{X - 2, Y + 10}, {X + 1, Y + 11}}, Pine, 1.5f);
        }

        void Unknown()
        {
            Rect(10, 6, 36, 44, Accent);
            FSlateDrawElement::MakeText(Elements, ++Layer,
                Geometry.ToPaintGeometry(FVector2D(24, 40),
                    FSlateLayoutTransform(Scale, Origin + FVector2D(17, 7) * Scale)),
                FText::FromString(TEXT("?")), FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 28),
                Effects, Styled(Charcoal));
        }

        int32 GetLayer() const { return Layer; }
        float GetScale() const { return Scale; }
        FLinearColor Accent;

    private:
        void Stroke(const TArray<FVector2D>& Points, FLinearColor Color, float Width)
        {
            TArray<FVector2D> Scaled;
            Scaled.Reserve(Points.Num());
            for (const FVector2D& Point : Points)
            {
                Scaled.Add(Origin + Point * Scale);
            }
            FSlateDrawElement::MakeLines(Elements, ++Layer, Geometry.ToPaintGeometry(),
                Scaled, Effects, Styled(Color), true, Width * Scale);
        }

        void Fill(TArray<FVector2D>& Path, FLinearColor Color)
        {
            // Local scan bands fill authored polygons using only Slate's standard box brush.
            // The antialiased contour covers the band edges; no engine custom-shape API is used.
            float MinY = 56.0f;
            float MaxY = 0.0f;
            for (const FVector2D& Point : Path)
            {
                MinY = FMath::Min(MinY, static_cast<float>(Point.Y));
                MaxY = FMath::Max(MaxY, static_cast<float>(Point.Y));
            }
            const int32 FillLayer = ++Layer;
            for (float Y = MinY; Y < MaxY; Y += 1.0f)
            {
                const float Height = FMath::Min(1.0f, MaxY - Y);
                const float SampleY = Y + Height * 0.5f;
                TArray<float, TInlineAllocator<16>> Crossings;
                for (int32 Index = 0; Index < Path.Num(); ++Index)
                {
                    const FVector2D A = Path[Index];
                    const FVector2D B = Path[(Index + 1) % Path.Num()];
                    if ((A.Y <= SampleY && B.Y > SampleY) || (B.Y <= SampleY && A.Y > SampleY))
                    {
                        Crossings.Add(static_cast<float>(A.X + (SampleY - A.Y) * (B.X - A.X) / (B.Y - A.Y)));
                    }
                }
                Crossings.Sort();
                for (int32 Index = 0; Index + 1 < Crossings.Num(); Index += 2)
                {
                    FSlateDrawElement::MakeBox(Elements, FillLayer,
                        Geometry.ToPaintGeometry(FVector2D(Crossings[Index + 1] - Crossings[Index], Height) * Scale,
                            FSlateLayoutTransform(Origin + FVector2D(Crossings[Index], Y) * Scale)),
                        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), Effects, Styled(Color));
                }
            }
            const FVector2D FirstPoint = Path[0];
            Path.Add(FirstPoint);
            Stroke(Path, Color, 1.5f);
        }

        const FGeometry& Geometry;
        FSlateWindowElementList& Elements;
        int32 Layer;
        FLinearColor StyleTint;
        ESlateDrawEffect Effects;
        float Scale = 1.0f;
        FVector2D Origin;
        float Desaturation = 0.0f;

        static FLinearColor Desaturate(const FLinearColor& Color, float Amount)
        {
            const float Luminance = Color.R * 0.2126f + Color.G * 0.7152f + Color.B * 0.0722f;
            return FLinearColor(
                FMath::Lerp(Color.R, Luminance, Amount),
                FMath::Lerp(Color.G, Luminance, Amount),
                FMath::Lerp(Color.B, Luminance, Amount),
                Color.A);
        }

        FLinearColor Styled(const FLinearColor& Color) const
        {
            return Desaturate(Color, Desaturation) * StyleTint;
        }
    };

    void PaintSeedPacket(FIconPainter& P, FLinearColor Band)
    {
        const FLinearColor Paper = Cream * 0.9f + HayGold * 0.1f;
        P.Shape({{11, 7}, {45, 7}, {47, 51}, {9, 51}}, Paper);
        P.Rect(11, 7, 34, 6, Band);
        P.Line({{11, 13}, {45, 13}}, Wood, 1.0f);
        P.Shape({{14, 16}, {42, 16}, {43, 43}, {13, 43}}, FLinearColor(0.62f, 0.78f, 0.84f));
        P.Shape({{13.5f, 37}, {42.6f, 35}, {43, 43}, {13, 43}}, FLinearColor(0.47f, 0.33f, 0.19f));
        P.Line({{16, 47}, {40, 47}}, Wood, 1.5f);
    }
}

void SHomesteadIcon::Construct(const FArguments& InArgs)
{
    Kind = InArgs._Kind;
    Tint = InArgs._Tint;
    Desaturation = InArgs._Desaturation;
}

bool SHomesteadIcon::ComputeVolatility() const
{
    return Kind.IsBound() || Tint.IsBound() || Desaturation.IsBound()
        || SLeafWidget::ComputeVolatility();
}

FVector2D SHomesteadIcon::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D(56.0f, 56.0f);
}

int32 SHomesteadIcon::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    static const TPair<FName, EKind> Kinds[] = {
        {FName(TEXT("pack")), EKind::Pack},
        {FName(TEXT("sort")), EKind::Sort},
        {FName(TEXT("craft")), EKind::Craft},
        {FName(TEXT("build")), EKind::Build},
        {FName(TEXT("guide")), EKind::Guide},
        {FName(TEXT("settings")), EKind::Settings},
        {FName(TEXT("appearance")), EKind::Appearance},
        {FName(TEXT("map")), EKind::Map},
        {FName(TEXT("knife")), EKind::Knife},
        {FName(TEXT("branch")), EKind::Branch},
        {FName(TEXT("stone")), EKind::Stone},
        {FName(TEXT("fiber")), EKind::Fiber},
        {FName(TEXT("berries")), EKind::Berries},
        {FName(TEXT("roots")), EKind::Roots},
        {FName(TEXT("flowers")), EKind::Flowers},
        {FName(TEXT("seeds")), EKind::SeedRoots},
        {FName(TEXT("hatchet")), EKind::Hatchet},
        {FName(TEXT("digging-stick")), EKind::DiggingStick},
        {FName(TEXT("watering-can")), EKind::WateringCan},
        {FName(TEXT("water")), EKind::Water},
        {FName(TEXT("roasted-roots")), EKind::RoastedRoots},
        {FName(TEXT("herbed-roots")), EKind::HerbedRoots},
        {FName(TEXT("timber")), EKind::Timber},
        {FName(TEXT("firewood")), EKind::Firewood},
        {FName(TEXT("machete")), EKind::Machete},
        {FName(TEXT("foundation")), EKind::Foundation},
        {FName(TEXT("wall")), EKind::Wall},
        {FName(TEXT("doorway")), EKind::Doorway},
        {FName(TEXT("roof")), EKind::Roof},
        {FName(TEXT("fire")), EKind::Fire},
        {FName(TEXT("bed")), EKind::Bed},
        {FName(TEXT("chest")), EKind::Chest},
        {FName(TEXT("linen-tunic")), EKind::LinenTunic},
        {FName(TEXT("linen-apron")), EKind::LinenApron},
        {FName(TEXT("leather-shoes")), EKind::LeatherShoes},
        {FName(TEXT("woven-footwraps")), EKind::WovenFootwraps},
        {FName(TEXT("fur")), EKind::Fur},
        {FName(TEXT("linen-shirt")), EKind::LinenShirt},
        {FName(TEXT("long-linen-shirt")), EKind::LongLinenShirt},
        {FName(TEXT("trousers")), EKind::Trousers},
        {FName(TEXT("fur-coat")), EKind::FurCoat},
        {FName(TEXT("fur-boots")), EKind::FurBoots},
        {FName(TEXT("woven-sandals")), EKind::WovenSandals},
        {FName(TEXT("turnshoes")), EKind::TurnShoes},
        {FName(TEXT("slot-torso")), EKind::SlotTorso},
        {FName(TEXT("slot-apron")), EKind::SlotApron},
        {FName(TEXT("slot-feet")), EKind::SlotFeet},
        {FName(TEXT("pasty")), EKind::Pasty},
        {FName(TEXT("bread")), EKind::Bread},
        {FName(TEXT("cheese")), EKind::Cheese},
        {FName(TEXT("twine")), EKind::Twine},
        {FName(TEXT("coin")), EKind::Coin},
        {FName(TEXT("shop")), EKind::Shop},
        {FName(TEXT("scythe")), EKind::Scythe},
        {FName(TEXT("billhook")), EKind::Billhook},
        {FName(TEXT("pickaxe")), EKind::Pickaxe},
        {FName(TEXT("rusted-axe-head")), EKind::RustedAxeHead},
        {FName(TEXT("rusted-hoe-blade")), EKind::RustedHoeBlade},
        {FName(TEXT("rusted-scythe-blade")), EKind::RustedScytheBlade},
        {FName(TEXT("rusted-billhook-head")), EKind::RustedBillhookHead},
        {FName(TEXT("rusted-pick-head")), EKind::RustedPickHead},
        {FName(TEXT("hay")), EKind::Hay},
        {FName(TEXT("weeds")), EKind::Weeds},
        {FName(TEXT("bramble-canes")), EKind::BrambleCanes},
        {FName(TEXT("kindling")), EKind::Kindling},
        {FName(TEXT("scrap-iron")), EKind::ScrapIron},
        {FName(TEXT("scrap-lead")), EKind::ScrapLead},
        {FName(TEXT("primroses")), EKind::Primroses},
        {FName(TEXT("bluebells")), EKind::Bluebells},
        {FName(TEXT("wild-daffodils")), EKind::WildDaffodils},
        {FName(TEXT("wild-garlic")), EKind::WildGarlic},
        {FName(TEXT("oil-lamp")), EKind::OilLamp},
        {FName(TEXT("oil-flask")), EKind::OilFlask},
        {FName(TEXT("pouch-arrows")), EKind::PouchArrows},
        {FName(TEXT("fishing-pole")), EKind::FishingPole},
        {FName(TEXT("fish")), EKind::Fish},
        {FName(TEXT("roots-seeds")), EKind::SeedRoots},
        {FName(TEXT("turnip-seeds")), EKind::SeedTurnip},
        {FName(TEXT("turnip-seed")), EKind::SeedTurnip},
        {FName(TEXT("carrot-seeds")), EKind::SeedCarrot},
        {FName(TEXT("carrot-seed")), EKind::SeedCarrot},
        {FName(TEXT("potato-seeds")), EKind::SeedPotato},
        {FName(TEXT("seed-potato")), EKind::SeedPotato},
        {FName(TEXT("cabbage-seeds")), EKind::SeedCabbage},
        {FName(TEXT("cabbage-seed")), EKind::SeedCabbage},
        {FName(TEXT("broad-bean-seeds")), EKind::SeedBroadBean},
        {FName(TEXT("broad-bean-seed")), EKind::SeedBroadBean},
        {FName(TEXT("strawberry-seeds")), EKind::SeedStrawberry},
        {FName(TEXT("strawberry-runner")), EKind::SeedStrawberry}
    };

    const FName CurrentKind = Kind.Get();
    EKind IconKind = EKind::Unknown;
    for (const auto& Entry : Kinds)
    {
        if (CurrentKind == Entry.Key)
        {
            IconKind = Entry.Value;
            break;
        }
    }
    const bool bGlyphFirst = IconKind >= EKind::SeedRoots;
    if (!bImageResolved || ImageKind != CurrentKind)
    {
        ImageKind = CurrentKind;
        bOriginalImageExpected = !bGlyphFirst
            && HomesteadOriginalItemArt::FindIcon(TCHAR_TO_UTF8(*CurrentKind.ToString())) != nullptr;
        OriginalImage = bGlyphFirst ? nullptr : HomesteadOriginalIcons::Load(CurrentKind);
        bImageResolved = true;
    }
    if (OriginalImage)
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const double Side = FMath::Min(Size.X, Size.Y);
        FLinearColor ImageTint = InWidgetStyle.GetColorAndOpacityTint();
        ImageTint.A *= Tint.Get().A;
        const ESlateDrawEffect ImageEffects = ShouldBeEnabled(bParentEnabled) && Desaturation.Get() <= 0.0f
            ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(Side, Side), FSlateLayoutTransform((Size - FVector2D(Side, Side)) * 0.5)),
            OriginalImage.Get(), ImageEffects, ImageTint);
        return LayerId;
    }
    if (CurrentKind == FName(TEXT("deferred-meal")) || bOriginalImageExpected)
        return LayerId;
    // Shop seed without a glyph of its own yet draws as a seed packet.
    if (IconKind == EKind::Unknown && !CurrentKind.IsNone())
    {
        const FString Id = CurrentKind.ToString();
        if (Id.EndsWith(TEXT("-seed")) || Id == TEXT("seed-potato") || Id == TEXT("strawberry-runner"))
            IconKind = EKind::Seeds;
    }

    const ESlateDrawEffect Effects = ShouldBeEnabled(bParentEnabled)
        ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
    FIconPainter P(AllottedGeometry, OutDrawElements, LayerId,
        InWidgetStyle.GetColorAndOpacityTint(), Tint.Get(), Desaturation.Get(), Effects);
    if (P.GetScale() <= 0.0f)
    {
        return LayerId;
    }

    const FLinearColor Gold = P.Accent;
    switch (IconKind)
    {
    case EKind::FishingPole:
        P.Line({{12, 47}, {33, 10}, {39, 8}}, Wood, 4);
        P.Line({{39, 8}, {44, 17}, {44, 35}, {39, 41}, {35, 39}, {35, 35}}, Cream, 1.5f);
        P.Line({{11, 47}, {17, 36}}, Gold, 6);
        break;
    case EKind::Fish:
        P.Shape({{11, 28}, {19, 18}, {34, 17}, {44, 27}, {35, 38}, {20, 38}}, WaterBlue);
        P.Shape({{11, 28}, {5, 18}, {5, 38}}, Gold);
        P.Shape({{23, 19}, {29, 10}, {35, 19}}, Cream);
        P.Line({{19, 28}, {32, 30}}, Cream, 1.5f);
        P.Disc(37, 25, 2, Pine);
        break;
    case EKind::SeedRoots:
        PaintSeedPacket(P, Wood);
        P.Root(23, 23, RootOrange);
        P.Root(33, 21, HayGold);
        P.Leaf({23, 23}, {17, 16}, 2.5f);
        P.Leaf({33, 21}, {38, 17}, 2.5f);
        break;
    case EKind::SeedTurnip:
    {
        PaintSeedPacket(P, FLinearColor(0.50f, 0.24f, 0.50f));
        P.Leaf({28, 26}, {20, 17}, 3.0f);
        P.Leaf({28, 26}, {36, 17}, 3.0f);
        P.Leaf({28, 26}, {28, 17}, 2.5f);
        P.Disc(28, 33, 8, FLinearColor(0.96f, 0.95f, 0.90f));
        P.Shape({{20.5f, 31}, {23, 27}, {28, 25}, {33, 27}, {35.5f, 31}, {28, 29}}, FLinearColor(0.56f, 0.24f, 0.52f));
        P.Line({{28, 41}, {28, 44}}, FLinearColor(0.96f, 0.95f, 0.90f), 1.2f);
        break;
    }
    case EKind::SeedCarrot:
        PaintSeedPacket(P, RootOrange);
        P.Leaf({28, 24}, {21, 17}, 2.5f);
        P.Leaf({28, 24}, {35, 17}, 2.5f);
        P.Leaf({28, 24}, {28, 17}, 2.0f);
        P.Shape({{23, 24}, {33, 24}, {30, 36}, {28, 44}, {26, 36}}, RootOrange);
        P.Line({{25, 28}, {28, 28.5f}}, Rust, 1.0f);
        P.Line({{29, 32}, {31, 31.5f}}, Rust, 1.0f);
        P.Line({{26.5f, 36}, {28.5f, 36.5f}}, Rust, 1.0f);
        break;
    case EKind::SeedPotato:
    {
        const FLinearColor Skin(0.76f, 0.58f, 0.34f);
        PaintSeedPacket(P, FLinearColor(0.58f, 0.42f, 0.24f));
        P.Shape({{17, 31}, {20, 25}, {28, 22}, {36, 24}, {40, 30}, {37, 37}, {28, 40}, {20, 38}}, Skin);
        P.Shape({{30, 34}, {36, 31}, {40, 34}, {38, 40}, {31, 41}}, Skin * 0.92f);
        for (const FVector2D& Eye : {FVector2D(23, 28), FVector2D(30, 26), FVector2D(25, 34), FVector2D(35, 36)})
            P.Disc(Eye.X, Eye.Y, 0.9f, Wood);
        P.Line({{30, 26}, {31, 22}}, LeafGreen, 1.5f);
        break;
    }
    case EKind::SeedCabbage:
    {
        const FLinearColor Heart(0.70f, 0.82f, 0.44f);
        PaintSeedPacket(P, LeafGreen);
        P.Leaf({28, 33}, {16, 38}, 5.0f);
        P.Leaf({28, 33}, {40, 38}, 5.0f);
        P.Disc(28, 31, 10, LeafGreen * 1.1f);
        P.Disc(28, 30, 6.5f, Heart);
        P.Line({{28, 30}, {24, 23}}, Cream, 1.0f);
        P.Line({{28, 30}, {33, 24}}, Cream, 1.0f);
        P.Line({{28, 30}, {21, 33}}, Cream, 1.0f);
        P.Line({{28, 30}, {35, 34}}, Cream, 1.0f);
        break;
    }
    case EKind::SeedBroadBean:
    {
        const FLinearColor Pod(0.40f, 0.62f, 0.24f);
        const FLinearColor Bean(0.74f, 0.86f, 0.54f);
        PaintSeedPacket(P, FLinearColor(0.24f, 0.44f, 0.20f));
        P.Shape({{15, 39}, {20, 30}, {29, 23}, {39, 19}, {42, 21}, {36, 28}, {26, 35}, {17, 41}}, Pod);
        for (const FVector2D& At : {FVector2D(21, 34), FVector2D(28, 28.5f), FVector2D(35, 23.5f)})
            P.Disc(At.X, At.Y, 2.6f, Bean);
        P.Line({{17, 40}, {37, 21}}, Pod * 0.75f, 1.0f);
        P.Line({{41, 20}, {43, 17}}, Wood, 1.5f);
        break;
    }
    case EKind::SeedStrawberry:
        PaintSeedPacket(P, Berry);
        P.Shape({{19, 27}, {28, 25}, {37, 27}, {36, 35}, {28, 43}, {20, 35}}, FLinearColor(0.82f, 0.14f, 0.18f));
        for (const FVector2D& Pip : {FVector2D(23, 30), FVector2D(28, 29), FVector2D(33, 30), FVector2D(25, 35),
                 FVector2D(31, 35), FVector2D(28, 39)})
            P.Disc(Pip.X, Pip.Y, 0.8f, HayGold);
        P.Leaf({28, 26}, {20, 21}, 2.5f);
        P.Leaf({28, 26}, {36, 21}, 2.5f);
        P.Leaf({28, 26}, {28, 19}, 2.0f);
        break;
    case EKind::Pack:
        P.Line({{21, 14}, {21, 8}, {34, 8}, {34, 14}}, Cream, 3);
        P.Shape({{16, 15}, {39, 15}, {44, 23}, {43, 47}, {12, 47}, {11, 23}}, Wood);
        P.Shape({{14, 15}, {41, 15}, {39, 28}, {28, 32}, {16, 28}}, Gold);
        P.Rect(24, 26, 7, 10, Pine);
        P.Rect(26, 28, 3, 5, Cream);
        P.Line({{17, 38}, {17, 43}, {38, 43}, {38, 38}}, Cream, 1.5f);
        break;
    case EKind::Sort:
        P.Line({{12, 16}, {42, 16}}, Cream, 4);
        P.Line({{12, 28}, {34, 28}}, Gold, 4);
        P.Line({{12, 40}, {26, 40}}, Wood, 4);
        P.Shape({{43, 25}, {50, 33}, {46, 33}, {46, 45}, {40, 45}, {40, 33}, {36, 33}}, Gold);
        break;
    case EKind::Craft:
        P.Line({{13, 44}, {39, 13}}, Wood, 6);
        P.Shape({{28, 10}, {33, 6}, {48, 19}, {43, 25}}, Gold);
        P.Line({{12, 13}, {43, 44}}, Cream, 3);
        P.Line({{10, 8}, {17, 15}, {13, 19}, {6, 12}}, Cream, 2);
        P.Disc(42, 43, 4, Gold);
        P.Disc(42, 43, 1.5f, Pine);
        break;
    case EKind::Build:
        P.Line({{9, 28}, {28, 10}, {47, 28}}, Gold, 4);
        P.Line({{15, 27}, {15, 45}, {41, 45}, {41, 27}}, Cream, 3);
        P.Rect(24, 31, 8, 14, Wood);
        P.Line({{37, 12}, {43, 12}, {43, 21}}, Gold, 3);
        break;
    case EKind::Guide:
        P.Shape({{7, 13}, {19, 12}, {28, 17}, {37, 12}, {49, 13},
            {49, 43}, {37, 42}, {28, 47}, {19, 42}, {7, 43}}, Cream);
        P.Line({{28, 18}, {28, 44}}, Wood, 2);
        P.Line({{12, 21}, {19, 20}, {23, 22}}, Wood, 1.5f);
        P.Line({{12, 28}, {22, 29}}, Wood, 1.5f);
        P.Line({{12, 35}, {22, 36}}, Wood, 1.5f);
        P.Leaf({34, 36}, {43, 22}, 4);
        P.Line({{34, 36}, {33, 40}}, Wood, 1.5f);
        break;
    case EKind::Settings:
        P.Line({{12, 11}, {12, 45}}, Cream, 2);
        P.Line({{28, 11}, {28, 45}}, Cream, 2);
        P.Line({{44, 11}, {44, 45}}, Cream, 2);
        P.Rect(7, 17, 10, 7, Gold);
        P.Rect(23, 33, 10, 7, Gold);
        P.Rect(39, 22, 10, 7, Gold);
        break;
    case EKind::Map:
        // A folded estate map: three panels, a dashed boundary and a pin.
        P.Shape({{7, 14}, {20, 9}, {20, 43}, {7, 48}}, Cream);
        P.Shape({{20, 9}, {36, 14}, {36, 48}, {20, 43}}, Gold);
        P.Shape({{36, 14}, {49, 9}, {49, 43}, {36, 48}}, Cream);
        P.Line({{11, 36}, {16, 30}}, Wood, 2);
        P.Line({{19, 26}, {24, 22}}, Wood, 2);
        P.Line({{28, 24}, {33, 28}}, Wood, 2);
        P.Line({{38, 31}, {44, 27}}, Wood, 2);
        P.Disc(28, 34, 4, Berry);
        P.Disc(28, 34, 1.5f, Cream);
        break;
    case EKind::Appearance:
        P.Line({{28, 38}, {28, 49}}, Wood, 6);
        P.Shape({{17, 9}, {28, 5}, {39, 9}, {43, 20}, {40, 32},
            {28, 39}, {16, 32}, {13, 20}}, Gold);
        P.Shape({{20, 12}, {28, 9}, {36, 12}, {39, 21}, {36, 29},
            {28, 35}, {20, 29}, {17, 21}}, Pine);
        P.Line({{21, 23}, {30, 14}}, Cream, 2);
        P.Line({{26, 29}, {34, 21}}, Cream, 2);
        P.Rect(25, 45, 6, 3, Gold);
        break;
    case EKind::Knife:
        P.Shape({{25, 31}, {39, 10}, {46, 6}, {43, 22}, {32, 36}}, Cream);
        P.Line({{29, 31}, {40, 15}}, StoneGray, 1.5f);
        P.Line({{13, 46}, {26, 33}}, Wood, 7);
        P.Line({{22, 29}, {33, 40}}, Gold, 3);
        P.Disc(17, 42, 1.5f, Gold);
        break;
    case EKind::Branch:
        P.Line({{12, 46}, {26, 29}, {31, 10}}, Wood, 6);
        P.Line({{26, 29}, {43, 22}, {48, 13}}, Wood, 4);
        P.Line({{24, 32}, {15, 21}, {14, 12}}, Wood, 4);
        P.Line({{14, 44}, {25, 30}}, Gold, 1.5f);
        P.Leaf({33, 24}, {42, 9}, 4);
        break;
    case EKind::Stone:
        P.Shape({{8, 35}, {13, 20}, {27, 11}, {43, 17}, {49, 36},
            {36, 45}, {18, 44}}, StoneGray);
        P.Shape({{13, 21}, {27, 12}, {34, 25}, {22, 32}, {9, 35}}, Cream);
        P.Line({{34, 25}, {43, 18}}, Pine, 1.5f);
        P.Line({{22, 32}, {25, 41}}, Pine, 1.5f);
        P.Line({{35, 39}, {43, 35}}, Gold, 2);
        break;
    case EKind::Fiber:
        P.Line({{14, 47}, {23, 31}, {16, 10}}, Cream, 3);
        P.Line({{24, 47}, {28, 30}, {26, 7}}, Gold, 3);
        P.Line({{35, 47}, {30, 31}, {39, 9}}, Cream, 3);
        P.Line({{42, 44}, {34, 31}, {46, 17}}, Gold, 2);
        P.Line({{19, 28}, {35, 29}, {35, 35}, {20, 34}, {19, 28}}, Wood, 3);
        P.Line({{28, 34}, {37, 40}}, Wood, 2);
        break;
    case EKind::Berries:
        P.Line({{19, 27}, {30, 18}, {39, 28}}, Wood, 2);
        P.Leaf({30, 20}, {19, 8}, 4);
        P.Leaf({30, 20}, {44, 12}, 4);
        P.Disc(19, 32, 8, Berry);
        P.Disc(37, 32, 8, Berry);
        P.Disc(28, 43, 7, Berry);
        P.Line({{15, 29}, {18, 27}}, Cream, 2);
        P.Line({{33, 29}, {36, 27}}, Cream, 2);
        P.Line({{25, 40}, {28, 39}}, Gold, 2);
        break;
    case EKind::Roots:
        P.Root(21, 29, RootOrange);
        P.Root(35, 24, Gold);
        P.Leaf({21, 29}, {11, 14}, 4);
        P.Leaf({21, 29}, {25, 9}, 4);
        P.Leaf({35, 24}, {33, 7}, 4);
        P.Leaf({35, 24}, {46, 12}, 4);
        break;
    case EKind::Flowers:
        P.Line({{27, 48}, {18, 19}}, LeafGreen, 3);
        P.Line({{27, 48}, {38, 16}}, LeafGreen, 3);
        P.Leaf({26, 43}, {11, 33}, 4);
        P.Leaf({31, 35}, {45, 29}, 4);
        P.Disc(13, 15, 4, Cream);
        P.Disc(21, 12, 4, Cream);
        P.Disc(24, 20, 4, Cream);
        P.Disc(16, 24, 4, Cream);
        P.Disc(18, 18, 3, Gold);
        P.Disc(33, 11, 4, Berry);
        P.Disc(41, 9, 4, Berry);
        P.Disc(44, 17, 4, Berry);
        P.Disc(36, 21, 4, Berry);
        P.Disc(38, 15, 3, Gold);
        break;
    case EKind::Seeds:
        P.Shape({{12, 15}, {31, 15}, {34, 43}, {9, 43}}, Cream);
        P.Rect(12, 17, 19, 4, Gold);
        P.Line({{21, 37}, {21, 29}}, Wood, 2);
        P.Leaf({21, 31}, {14, 24}, 2.5f);
        P.Leaf({21, 31}, {28, 25}, 2.5f);
        P.Shape({{39, 29}, {44, 31}, {45, 36}, {41, 38}, {38, 34}}, Gold);
        P.Shape({{39, 42}, {43, 43}, {46, 48}, {40, 49}, {37, 46}}, Wood);
        P.Shape({{46, 20}, {49, 24}, {47, 28}, {44, 24}}, Gold);
        break;
    case EKind::Hatchet:
        P.Line({{17, 48}, {33, 11}}, Wood, 6);
        P.Line({{19, 44}, {28, 23}}, Gold, 1.5f);
        P.Shape({{27, 12}, {38, 13}, {48, 23}, {44, 34}, {30, 24}, {24, 22}}, StoneGray);
        P.Shape({{44, 20}, {48, 23}, {44, 34}, {39, 30}}, Cream);
        P.Line({{26, 15}, {35, 19}, {25, 19}, {33, 23}}, Gold, 2);
        break;
    case EKind::DiggingStick:
        // Stone hoe: a long diagonal haft with a flat stone blade lashed across its head.
        P.Line({{9, 50}, {35, 10}}, Wood, 5);
        P.Shape({{31, 9}, {40, 6}, {49, 27}, {46, 34}, {38, 32}, {37, 20}}, StoneGray);
        P.Shape({{46, 24}, {49, 27}, {46, 34}, {41, 33}}, Cream);
        P.Line({{30, 12}, {39, 14}, {29, 17}, {38, 19}}, Gold, 2);
        break;
    case EKind::Machete:
        // Long leaf-tapered blade over a riveted wooden grip.
        P.Shape({{21, 34}, {42, 9}, {49, 5}, {47, 13}, {27, 38}}, StoneGray);
        P.Shape({{42, 9}, {49, 5}, {47, 13}, {44, 14}}, Cream);
        P.Line({{24, 33}, {43, 11}}, Cream, 1.5f);
        P.Line({{9, 49}, {22, 35}}, Wood, 7);
        P.Line({{18, 31}, {27, 40}}, Gold, 3);
        P.Disc(13, 45, 1.5f, Gold);
        P.Disc(17, 41, 1.5f, Gold);
        break;
    case EKind::WateringCan:
        P.Line({{18, 23}, {9, 21}, {6, 28}, {9, 36}, {18, 36}}, Gold, 3);
        P.Shape({{33, 30}, {43, 22}, {47, 24}, {36, 40}}, Gold);
        P.Line({{42, 20}, {49, 25}}, Cream, 4);
        P.Shape({{17, 22}, {33, 22}, {37, 42}, {32, 46}, {16, 46}, {13, 42}}, Wood);
        P.Line({{18, 22}, {20, 14}, {29, 14}, {32, 22}}, Gold, 3);
        P.Line({{20, 27}, {19, 40}}, Cream, 2);
        P.Line({{27, 27}, {29, 41}}, Gold, 2);
        P.Line({{46, 30}, {47, 33}}, WaterBlue, 2);
        P.Line({{49, 36}, {50, 39}}, WaterBlue, 2);
        break;
    case EKind::Water:
        P.Shape({{28, 7}, {34, 19}, {43, 32}, {43, 40}, {36, 47},
            {20, 47}, {13, 40}, {13, 32}, {22, 19}}, WaterBlue);
        P.Line({{21, 28}, {18, 34}, {19, 40}, {23, 43}}, Cream, 2);
        P.Line({{25, 49}, {32, 49}}, Gold, 2);
        break;
    case EKind::RoastedRoots:
        P.Line({{17, 19}, {14, 14}, {18, 8}}, Cream, 2);
        P.Line({{30, 17}, {27, 12}, {31, 6}}, Cream, 2);
        P.Line({{41, 21}, {38, 16}, {42, 10}}, Cream, 2);
        P.Shape({{7, 37}, {49, 37}, {44, 44}, {12, 44}}, Gold);
        P.Root(19, 23, Wood);
        P.Root(32, 22, RootOrange);
        P.Line({{14, 28}, {21, 31}}, Cream, 2);
        P.Line({{28, 28}, {34, 31}}, Gold, 2);
        P.Line({{10, 46}, {46, 46}}, Cream, 2);
        break;
    case EKind::HerbedRoots:
        P.Shape({{8, 30}, {48, 30}, {43, 44}, {35, 49}, {21, 49}, {13, 44}}, Cream);
        P.Line({{11, 34}, {45, 34}}, Gold, 3);
        P.Root(19, 17, RootOrange);
        P.Root(32, 18, Gold);
        P.Line({{35, 26}, {43, 9}}, LeafGreen, 2);
        P.Leaf({39, 18}, {32, 8}, 3);
        P.Leaf({40, 17}, {49, 10}, 3);
        P.Leaf({36, 25}, {47, 19}, 3);
        P.Line({{23, 42}, {33, 42}}, Wood, 2);
        break;
    case EKind::Timber:
        P.Rect(10, 12, 36, 11, Wood);
        P.Rect(7, 26, 38, 11, Gold);
        P.Rect(12, 40, 36, 10, Wood);
        P.Disc(10, 17, 5, Cream);
        P.Disc(45, 31, 5, Cream);
        P.Disc(12, 45, 4, Gold);
        P.Line({{20, 14}, {39, 20}}, Gold, 2);
        P.Line({{16, 29}, {36, 35}}, Cream, 2);
        P.Line({{20, 43}, {40, 48}}, Gold, 2);
        break;
    case EKind::Firewood:
        P.Line({{12, 44}, {42, 14}}, Wood, 8);
        P.Line({{14, 14}, {44, 44}}, Gold, 8);
        P.Line({{11, 43}, {40, 14}}, Cream, 2);
        P.Line({{16, 15}, {45, 43}}, Cream, 2);
        P.Shape({{22, 48}, {28, 34}, {34, 48}}, RootOrange);
        P.Shape({{29, 49}, {34, 29}, {41, 49}}, Gold);
        break;
    case EKind::Foundation:
        P.Shape({{7, 28}, {29, 16}, {49, 27}, {27, 40}}, Gold);
        P.Shape({{7, 30}, {27, 42}, {27, 49}, {7, 37}}, Wood);
        P.Shape({{29, 42}, {49, 30}, {49, 37}, {29, 49}}, Cream);
        P.Line({{16, 24}, {36, 35}}, Wood, 1.5f);
        P.Line({{25, 19}, {45, 30}}, Wood, 1.5f);
        break;
    case EKind::Wall:
        P.Rect(11, 13, 34, 32, Wood);
        P.Line({{18, 14}, {18, 44}}, Gold, 2);
        P.Line({{27, 14}, {27, 44}}, Gold, 2);
        P.Line({{37, 14}, {37, 44}}, Gold, 2);
        P.Rect(8, 9, 5, 40, Cream);
        P.Rect(43, 9, 5, 40, Cream);
        P.Rect(8, 12, 40, 4, Gold);
        P.Line({{14, 41}, {42, 20}}, Cream, 3);
        break;
    case EKind::Doorway:
        P.Rect(10, 10, 7, 39, Wood);
        P.Rect(39, 10, 7, 39, Wood);
        P.Rect(10, 8, 36, 8, Gold);
        P.Shape({{18, 17}, {25, 17}, {21, 32}, {18, 37}}, Cream);
        P.Shape({{31, 17}, {38, 17}, {38, 37}, {35, 32}}, Cream);
        P.Line({{18, 32}, {23, 32}}, Gold, 2);
        P.Line({{33, 32}, {38, 32}}, Gold, 2);
        P.Line({{8, 49}, {48, 49}}, Cream, 2);
        break;
    case EKind::Roof:
        P.Shape({{6, 34}, {23, 12}, {38, 12}, {50, 34}, {34, 34}, {23, 18}, {12, 34}}, Gold);
        P.Line({{23, 12}, {36, 34}}, Cream, 2);
        P.Line({{29, 13}, {43, 33}}, Wood, 1.5f);
        P.Line({{15, 24}, {44, 24}}, Wood, 1.5f);
        P.Line({{8, 39}, {48, 39}}, Cream, 3);
        P.Line({{16, 40}, {16, 47}}, Wood, 3);
        P.Line({{40, 40}, {40, 47}}, Wood, 3);
        break;
    case EKind::Fire:
        P.Line({{13, 48}, {43, 39}}, Wood, 5);
        P.Line({{13, 39}, {43, 48}}, Gold, 4);
        P.Shape({{28, 6}, {30, 20}, {39, 15}, {38, 25}, {45, 32},
            {40, 40}, {29, 44}, {17, 41}, {11, 33}, {18, 22}, {21, 29}}, RootOrange);
        P.Shape({{28, 23}, {29, 32}, {35, 29}, {36, 36}, {28, 41}, {21, 36}}, Gold);
        P.Line({{27, 34}, {28, 38}}, Cream, 3);
        break;
    case EKind::Bed:
        P.Rect(9, 19, 4, 28, Wood);
        P.Rect(44, 30, 4, 17, Wood);
        P.Rect(12, 27, 32, 14, Gold);
        P.Shape({{13, 23}, {24, 23}, {27, 30}, {13, 30}}, Cream);
        P.Rect(28, 27, 16, 13, LeafGreen);
        P.Line({{31, 29}, {31, 38}}, Cream, 2);
        P.Line({{10, 42}, {46, 42}}, Wood, 4);
        break;
    case EKind::Chest:
        P.Shape({{9, 25}, {13, 15}, {43, 15}, {47, 25}}, Gold);
        P.Rect(9, 27, 38, 20, Wood);
        P.Rect(14, 17, 4, 29, Cream);
        P.Rect(38, 17, 4, 29, Cream);
        P.Line({{10, 26}, {46, 26}}, Pine, 2);
        P.Rect(24, 25, 8, 10, Gold);
        P.Rect(27, 28, 2, 4, Pine);
        P.Line({{20, 41}, {36, 41}}, Gold, 1.5f);
        break;
    case EKind::LinenTunic:
        P.Shape({{19, 9}, {24, 13}, {32, 13}, {37, 9}, {49, 21}, {41, 29},
            {37, 25}, {39, 47}, {17, 47}, {19, 25}, {15, 29}, {7, 21}}, Gold);
        P.Line({{19, 9}, {24, 13}, {32, 13}, {37, 9}, {49, 21}, {41, 29},
            {37, 25}, {39, 47}, {17, 47}, {19, 25}, {15, 29}, {7, 21}, {19, 9}}, Cream, 1.5f);
        P.Line({{21, 10}, {25, 18}, {31, 18}, {35, 10}}, Wood, 2);
        P.Line({{18, 36}, {38, 36}}, Gold, 3);
        P.Line({{29, 36}, {32, 43}}, Wood, 2);
        P.Line({{19, 44}, {37, 44}}, Gold, 1.5f);
        break;
    case EKind::LinenApron:
        P.Line({{22, 17}, {22, 8}, {34, 8}, {34, 17}}, Gold, 3);
        P.Shape({{21, 17}, {35, 17}, {35, 27}, {44, 48}, {12, 48}, {21, 27}}, Gold);
        P.Line({{21, 17}, {35, 17}, {35, 27}, {44, 48}, {12, 48}, {21, 27}, {21, 17}}, Cream, 1.5f);
        P.Line({{21, 28}, {10, 24}, {9, 32}, {21, 28}, {35, 28}, {46, 24}, {47, 32}, {35, 28}}, Gold, 2);
        P.Rect(22, 33, 12, 9, Wood);
        P.Line({{23, 34}, {33, 34}}, Gold, 2);
        P.Line({{16, 45}, {40, 45}}, Gold, 1.5f);
        break;
    case EKind::LeatherShoes:
        P.Shape({{9, 17}, {21, 17}, {22, 28}, {29, 31}, {31, 37},
            {27, 40}, {8, 40}, {6, 35}}, Wood);
        P.Shape({{31, 11}, {42, 11}, {43, 23}, {49, 27}, {50, 33},
            {46, 35}, {30, 35}, {28, 29}}, Gold);
        P.Line({{8, 41}, {28, 41}}, Cream, 3);
        P.Line({{31, 36}, {48, 36}}, Cream, 3);
        P.Line({{12, 22}, {19, 24}, {13, 27}, {21, 29}}, Cream, 1.5f);
        P.Line({{34, 16}, {40, 18}, {34, 21}, {42, 24}}, Pine, 1.5f);
        break;
    case EKind::WovenFootwraps:
        P.Shape({{10, 9}, {21, 9}, {22, 33}, {28, 39}, {27, 47}, {8, 47}, {7, 39}}, Cream);
        P.Shape({{33, 6}, {44, 6}, {44, 28}, {49, 34}, {48, 41}, {31, 41}, {30, 34}}, Gold);
        P.Line({{10, 14}, {21, 20}, {10, 26}, {22, 32}, {10, 38}, {24, 44}}, Wood, 2);
        P.Line({{21, 14}, {10, 20}, {21, 26}, {10, 32}}, Wood, 2);
        P.Line({{33, 11}, {44, 17}, {33, 23}, {44, 29}, {33, 35}, {46, 38}}, Wood, 2);
        P.Line({{44, 11}, {33, 17}, {44, 23}}, Wood, 2);
        break;
    case EKind::Fur:
        // A stretched pelt: ragged outline, darker spine, pale belly edges.
        P.Shape({{18, 7}, {28, 11}, {38, 7}, {41, 16}, {48, 19}, {44, 30}, {49, 41},
            {39, 44}, {34, 50}, {22, 50}, {17, 44}, {7, 41}, {12, 30}, {8, 19}, {15, 16}}, Wood);
        P.Shape({{24, 14}, {32, 14}, {34, 44}, {22, 44}}, Gold);
        P.Line({{28, 12}, {28, 47}}, Charcoal, 2);
        P.Line({{14, 22}, {19, 25}}, Cream, 1.5f);
        P.Line({{42, 22}, {37, 25}}, Cream, 1.5f);
        P.Line({{12, 38}, {18, 36}}, Cream, 1.5f);
        P.Line({{44, 38}, {38, 36}}, Cream, 1.5f);
        break;
    case EKind::LinenShirt:
        P.Shape({{19, 9}, {24, 12}, {32, 12}, {37, 9}, {48, 17}, {43, 26},
            {38, 23}, {38, 46}, {18, 46}, {18, 23}, {13, 26}, {8, 17}}, Cream);
        P.Line({{24, 11}, {28, 18}, {32, 11}}, Gold, 2);
        P.Line({{19, 43}, {37, 43}}, Gold, 1.5f);
        P.Line({{13, 24}, {18, 21}}, Wood, 1.5f);
        P.Line({{43, 24}, {38, 21}}, Wood, 1.5f);
        break;
    case EKind::LongLinenShirt:
        P.Shape({{19, 8}, {24, 11}, {32, 11}, {37, 8}, {45, 14}, {51, 42}, {45, 44},
            {38, 22}, {38, 47}, {18, 47}, {18, 22}, {11, 44}, {5, 42}, {11, 14}}, Cream);
        P.Line({{24, 10}, {28, 17}, {32, 10}}, Gold, 2);
        P.Line({{27, 14}, {29, 14}}, Wood, 3);
        P.Line({{6, 40}, {11, 41}}, Gold, 2);
        P.Line({{50, 40}, {45, 41}}, Gold, 2);
        P.Line({{19, 44}, {37, 44}}, Gold, 1.5f);
        break;
    case EKind::Trousers:
        P.Shape({{15, 8}, {41, 8}, {45, 48}, {33, 48}, {28, 22}, {23, 48}, {11, 48}}, Gold);
        P.Rect(15, 8, 26, 5, Wood);
        P.Line({{24, 13}, {27, 19}}, Cream, 1.5f);
        P.Line({{32, 13}, {29, 19}}, Cream, 1.5f);
        P.Line({{12, 44}, {23, 44}}, Wood, 2);
        P.Line({{33, 44}, {44, 44}}, Wood, 2);
        break;
    case EKind::FurCoat:
        P.Shape({{18, 10}, {38, 10}, {47, 18}, {50, 44}, {43, 45}, {40, 26},
            {41, 50}, {15, 50}, {16, 26}, {13, 45}, {6, 44}, {9, 18}}, Wood);
        P.Shape({{16, 8}, {28, 16}, {40, 8}, {41, 14}, {28, 24}, {15, 14}}, Cream);
        P.Line({{28, 22}, {28, 49}}, Charcoal, 2);
        P.Line({{16, 47}, {40, 47}}, Cream, 3);
        P.Line({{6, 42}, {13, 43}}, Cream, 3);
        P.Line({{50, 42}, {43, 43}}, Cream, 3);
        P.Disc(31, 31, 1.5f, Gold);
        P.Disc(31, 39, 1.5f, Gold);
        break;
    case EKind::FurBoots:
        P.Shape({{9, 16}, {22, 16}, {23, 38}, {30, 42}, {30, 48}, {8, 48}}, Wood);
        P.Shape({{31, 10}, {44, 10}, {45, 32}, {52, 36}, {52, 42}, {30, 42}}, Gold);
        P.Rect(8, 11, 15, 7, Cream);
        P.Rect(30, 5, 15, 7, Cream);
        P.Line({{10, 34}, {22, 36}}, Charcoal, 1.5f);
        P.Line({{32, 28}, {44, 30}}, Pine, 1.5f);
        break;
    case EKind::WovenSandals:
        P.Shape({{9, 14}, {19, 12}, {23, 22}, {22, 46}, {14, 49}, {8, 44}}, Gold);
        P.Shape({{33, 10}, {43, 8}, {48, 18}, {47, 42}, {39, 46}, {33, 41}}, Gold);
        P.Line({{9, 22}, {22, 26}, {10, 32}, {22, 36}}, Wood, 1.5f);
        P.Line({{34, 18}, {47, 22}, {34, 28}, {47, 32}}, Wood, 1.5f);
        P.Line({{8, 30}, {24, 18}}, Cream, 2);
        P.Line({{33, 26}, {48, 14}}, Cream, 2);
        break;
    case EKind::TurnShoes:
        P.Shape({{7, 24}, {18, 20}, {23, 27}, {29, 32}, {30, 38}, {26, 40}, {6, 40}}, Wood);
        P.Shape({{29, 14}, {40, 10}, {45, 18}, {51, 24}, {51, 31}, {47, 33}, {28, 33}}, Gold);
        P.Line({{15, 22}, {19, 26}, {14, 27}}, Cream, 1.5f);
        P.Line({{37, 12}, {41, 16}, {36, 17}}, Pine, 1.5f);
        P.Line({{6, 41}, {26, 41}}, Charcoal, 2);
        P.Line({{28, 34}, {47, 34}}, Charcoal, 2);
        break;
    case EKind::SlotTorso:
        P.Shape({{16, 10}, {24, 7}, {32, 7}, {40, 10}, {48, 22}, {42, 28},
            {38, 22}, {38, 49}, {18, 49}, {18, 22}, {14, 28}, {8, 22}}, Cream);
        P.Line({{24, 8}, {28, 16}, {32, 8}}, Gold, 2);
        P.Line({{20, 34}, {36, 34}}, Wood, 3);
        break;
    case EKind::SlotApron:
        P.Line({{20, 8}, {28, 15}, {36, 8}}, Gold, 3);
        P.Shape({{20, 14}, {36, 14}, {42, 48}, {14, 48}}, Cream);
        P.Rect(23, 30, 12, 10, Gold);
        P.Line({{14, 22}, {7, 27}}, Wood, 2);
        P.Line({{42, 22}, {49, 27}}, Wood, 2);
        break;
    case EKind::SlotFeet:
        P.Shape({{10, 32}, {22, 28}, {29, 39}, {27, 47}, {9, 47}, {6, 42}}, Cream);
        P.Shape({{31, 22}, {42, 25}, {50, 39}, {47, 45}, {31, 45}, {27, 39}}, Gold);
        P.Line({{11, 39}, {25, 39}}, Wood, 2);
        P.Line({{33, 35}, {47, 35}}, Cream, 2);
        break;
    case EKind::Pasty:
        // A half-moon pasty with its crimped edge along the top.
        P.Shape({{7, 38}, {10, 27}, {18, 18}, {28, 15}, {38, 18}, {46, 27}, {49, 38}, {40, 43}, {28, 45}, {16, 43}}, RootOrange);
        P.Shape({{12, 36}, {16, 28}, {28, 22}, {40, 28}, {44, 36}, {28, 40}}, Gold);
        for (int32 Crimp = 0; Crimp < 7; ++Crimp)
        {
            const float X = 11.0f + Crimp * 5.6f;
            const float Y = 27.0f - FMath::Sin(Crimp / 6.0f * PI) * 10.0f;
            P.Disc(X, Y, 2.6f, Cream);
        }
        P.Line({{22, 33}, {26, 31}}, Wood, 1.5f);
        P.Line({{31, 31}, {35, 33}}, Wood, 1.5f);
        break;
    case EKind::Bread:
        P.Shape({{7, 40}, {8, 29}, {15, 19}, {28, 15}, {41, 19}, {48, 29}, {49, 40}, {40, 45}, {16, 45}}, Wood);
        P.Shape({{11, 36}, {14, 27}, {22, 21}, {28, 20}, {38, 23}, {44, 30}, {45, 37}, {28, 40}}, RootOrange);
        P.Line({{18, 25}, {22, 33}}, Cream, 2);
        P.Line({{27, 22}, {29, 32}}, Cream, 2);
        P.Line({{36, 24}, {35, 33}}, Cream, 2);
        break;
    case EKind::Cheese:
        P.Shape({{7, 34}, {44, 14}, {49, 22}, {49, 44}, {7, 44}}, Gold);
        P.Shape({{7, 34}, {44, 14}, {49, 22}, {12, 38}}, Cream);
        P.Disc(20, 40, 2.5f, Wood);
        P.Disc(33, 36, 3.2f, Wood);
        P.Disc(43, 40, 2.2f, Wood);
        P.Disc(40, 29, 2, Wood);
        break;
    case EKind::Twine:
        P.Disc(28, 29, 17, Wood);
        P.Disc(28, 29, 13, Gold);
        P.Line({{14, 22}, {42, 36}}, Wood, 1.5f);
        P.Line({{13, 29}, {43, 29}}, Wood, 1.5f);
        P.Line({{15, 36}, {41, 22}}, Wood, 1.5f);
        P.Line({{42, 38}, {47, 45}, {40, 49}, {44, 52}}, Gold, 2);
        break;
    case EKind::Coin:
        P.Disc(28, 28, 19, Wood);
        P.Disc(28, 28, 16, Gold);
        P.Disc(28, 28, 12, RootOrange);
        // A small stamped crown: money is whole coins, with no currency sign.
        P.Line({{21, 33}, {21, 24}, {25, 29}, {28, 21}, {31, 29}, {35, 24}, {35, 33}, {21, 33}}, Cream, 2.0f);
        break;
    case EKind::Shop:
        P.Shape({{6, 22}, {28, 8}, {50, 22}}, Gold);
        P.Rect(10, 22, 36, 26, Wood);
        P.Rect(14, 27, 12, 10, Cream);
        P.Rect(31, 27, 11, 21, Pine);
        P.Rect(8, 22, 40, 4, Cream);
        break;
    case EKind::Scythe:
        // A long bowed snath with its hand nib, the blade sweeping out from the top.
        P.Line({{12, 51}, {17, 36}, {22, 22}, {25, 8}}, Wood, 4);
        P.Line({{15, 40}, {9, 35}}, Wood, 3);
        P.Shape({{24, 7}, {36, 8}, {46, 13}, {52, 21}, {44, 17}, {34, 14}, {25, 13}}, StoneGray);
        P.Line({{27, 13}, {36, 14}, {46, 18}, {52, 21}}, Cream, 1.5f);
        break;
    case EKind::Billhook:
        // A deep hooked blade over a turned handle with its ferrule.
        P.Shape({{22, 34}, {31, 20}, {39, 10}, {47, 6}, {50, 10}, {45, 11}, {42, 15}, {41, 22}, {29, 38}}, StoneGray);
        P.Line({{25, 34}, {36, 20}}, Cream, 1.5f);
        P.Line({{10, 50}, {21, 37}}, Wood, 7);
        P.Line({{19, 35}, {25, 41}}, Iron, 3);
        break;
    case EKind::Pickaxe:
        P.Line({{12, 51}, {37, 15}}, Wood, 5);
        P.Line({{19, 7}, {28, 9}, {37, 14}, {45, 21}, {52, 31}}, Iron, 5);
        P.Line({{21, 8}, {29, 10}}, StoneGray, 1.5f);
        P.Disc(37, 15, 3.5f, Iron);
        break;
    case EKind::RustedAxeHead:
        P.Shape({{13, 21}, {30, 18}, {41, 10}, {47, 26}, {44, 44}, {32, 36}, {13, 33}}, Rust);
        P.Disc(19, 27, 4, Pine);
        P.Line({{41, 11}, {46, 26}, {44, 42}}, StoneGray, 1.5f);
        break;
    case EKind::RustedHoeBlade:
        P.Shape({{10, 31}, {46, 25}, {49, 42}, {12, 45}}, Rust);
        P.Line({{25, 28}, {27, 12}, {33, 10}}, Rust, 4);
        P.Line({{12, 45}, {49, 42}}, StoneGray, 1.5f);
        break;
    case EKind::RustedScytheBlade:
        P.Shape({{8, 16}, {24, 12}, {40, 16}, {51, 28}, {42, 23}, {26, 20}, {9, 22}}, Rust);
        P.Line({{9, 22}, {26, 20}, {42, 23}, {51, 28}}, StoneGray, 1.5f);
        P.Line({{8, 16}, {6, 26}}, Rust, 3);
        break;
    case EKind::RustedBillhookHead:
        P.Shape({{16, 46}, {24, 30}, {33, 16}, {42, 8}, {48, 11}, {43, 13}, {40, 19}, {38, 27}, {25, 48}}, Rust);
        P.Line({{25, 48}, {38, 27}, {40, 19}, {43, 13}}, StoneGray, 1.5f);
        P.Line({{16, 46}, {12, 52}}, Rust, 3);
        break;
    case EKind::RustedPickHead:
        P.Line({{8, 20}, {18, 14}, {28, 12}, {38, 14}, {48, 20}}, Rust, 7);
        P.Disc(28, 14, 5, Rust);
        P.Disc(28, 14, 2.5f, Pine);
        P.Line({{8, 20}, {5, 25}}, Rust, 3);
        P.Line({{48, 20}, {51, 25}}, Rust, 3);
        break;
    case EKind::Hay:
        // A bound sheaf of mown grass.
        for (int32 I = 0; I < 7; ++I)
        {
            const float X = 16.0f + I * 4.0f;
            P.Line({{28, 30}, {X, 8.0f + (I % 2) * 3.0f}}, I % 2 ? HayGold : Cream, 2.5f);
            P.Line({{28, 30}, {X - 2.0f, 50.0f}}, I % 2 ? Cream : HayGold, 2.5f);
        }
        P.Line({{19, 30}, {37, 30}}, Wood, 4);
        break;
    case EKind::Weeds:
        P.Line({{28, 50}, {28, 30}}, LeafGreen, 2);
        P.Leaf({28, 48}, {10, 30}, 6);
        P.Leaf({28, 48}, {46, 28}, 6);
        P.Leaf({28, 44}, {18, 14}, 5);
        P.Leaf({28, 44}, {38, 12}, 5);
        P.Disc(28, 25, 4, HayGold);
        break;
    case EKind::BrambleCanes:
        // Three cut canes, prickled, tied in a bundle.
        P.Line({{8, 46}, {22, 28}, {48, 10}}, Berry * 0.7f + LeafGreen * 0.3f, 3);
        P.Line({{10, 50}, {28, 32}, {50, 18}}, LeafGreen * 0.8f, 3);
        P.Line({{6, 40}, {20, 24}, {42, 8}}, Berry * 0.6f + Wood * 0.4f, 2.5f);
        for (const FVector2D& Prickle : {FVector2D(16, 36), FVector2D(30, 22), FVector2D(40, 18), FVector2D(24, 36)})
            P.Line({Prickle, Prickle + FVector2D(3, 3)}, Cream, 1.5f);
        P.Line({{19, 38}, {27, 28}}, HayGold, 3);
        break;
    case EKind::Kindling:
        P.Line({{8, 44}, {48, 30}}, Wood, 3);
        P.Line({{10, 34}, {46, 44}}, Wood * 0.8f, 3);
        P.Line({{12, 40}, {44, 22}}, Wood * 1.15f, 2.5f);
        P.Line({{14, 26}, {42, 38}}, Cream * 0.7f + Wood * 0.3f, 2);
        P.Line({{30, 30}, {33, 22}}, Wood, 2);
        break;
    case EKind::ScrapIron:
        P.Shape({{8, 38}, {20, 30}, {30, 36}, {26, 46}, {10, 47}}, Iron);
        P.Line({{24, 24}, {38, 14}, {48, 18}}, Rust, 5);
        P.Line({{32, 44}, {48, 34}}, Iron, 4);
        P.Line({{12, 42}, {22, 36}}, StoneGray, 1.5f);
        break;
    case EKind::ScrapLead:
        P.Shape({{10, 34}, {22, 24}, {40, 22}, {48, 32}, {42, 44}, {18, 46}}, FLinearColor(0.38f, 0.42f, 0.48f));
        P.Line({{16, 34}, {28, 28}, {40, 30}}, StoneGray, 2);
        P.Line({{20, 42}, {36, 38}}, Cream * 0.6f, 1.5f);
        break;
    case EKind::Primroses:
        P.Leaf({28, 50}, {10, 38}, 7);
        P.Leaf({28, 50}, {46, 38}, 7);
        for (const FVector2D& Centre : {FVector2D(19, 20), FVector2D(36, 16), FVector2D(29, 32)})
        {
            for (int32 Petal = 0; Petal < 5; ++Petal)
            {
                const float Angle = Petal * 2 * PI / 5;
                P.Disc(Centre.X + FMath::Cos(Angle) * 4.5f, Centre.Y + FMath::Sin(Angle) * 4.5f, 3.2f,
                    FLinearColor(0.96f, 0.9f, 0.55f));
            }
            P.Disc(Centre.X, Centre.Y, 2, RootOrange);
        }
        break;
    case EKind::Bluebells:
        P.Line({{16, 50}, {20, 30}, {28, 14}, {40, 10}}, LeafGreen, 2.5f);
        P.Leaf({18, 50}, {8, 24}, 4);
        for (const FVector2D& Bell : {FVector2D(26, 20), FVector2D(33, 15), FVector2D(40, 14), FVector2D(22, 28)})
        {
            P.Shape({Bell, Bell + FVector2D(-3, 8), Bell + FVector2D(4, 9), Bell + FVector2D(3, 1)},
                FLinearColor(0.34f, 0.40f, 0.86f));
        }
        break;
    case EKind::WildDaffodils:
        P.Line({{28, 50}, {28, 24}}, LeafGreen, 2.5f);
        P.Leaf({26, 50}, {12, 20}, 4);
        P.Leaf({30, 50}, {44, 22}, 4);
        for (int32 Petal = 0; Petal < 6; ++Petal)
        {
            const float Angle = Petal * PI / 3;
            P.Leaf({28, 20}, {28 + FMath::Cos(Angle) * 12, 20 + FMath::Sin(Angle) * 12}, 5);
        }
        P.Disc(28, 20, 5, FLinearColor(0.98f, 0.78f, 0.18f));
        P.Disc(28, 20, 2.5f, RootOrange);
        break;
    case EKind::WildGarlic:
        P.Leaf({22, 52}, {8, 18}, 8);
        P.Leaf({30, 52}, {46, 20}, 8);
        P.Line({{27, 50}, {27, 18}}, LeafGreen, 2);
        for (int32 Floret = 0; Floret < 7; ++Floret)
        {
            const float Angle = Floret * 2 * PI / 7;
            const FVector2D At(27 + FMath::Cos(Angle) * 7, 13 + FMath::Sin(Angle) * 5);
            P.Line({{27, 18}, At}, LeafGreen, 1);
            P.Disc(At.X, At.Y, 2.2f, Cream);
        }
        break;
    case EKind::OilLamp:
        // A hurricane lantern hung from its bail: tin cap and fount, a glass chimney with its flame.
        P.Line({{17, 16}, {17, 9}, {22, 4}, {34, 4}, {39, 9}, {39, 16}}, Iron, 2);
        P.Shape({{19, 18}, {23, 12}, {33, 12}, {37, 18}}, StoneGray);
        P.Shape({{18, 21}, {22, 18}, {34, 18}, {38, 21}, {40, 29}, {38, 37}, {34, 40}, {22, 40}, {18, 37}, {16, 29}}, Cream * 0.55f + Gold * 0.45f);
        P.Shape({{25, 34}, {26, 27}, {28, 22}, {30, 27}, {31, 34}, {28, 36}}, RootOrange);
        P.Shape({{27, 33}, {27.5f, 28}, {28, 25}, {28.5f, 28}, {29, 33}}, FLinearColor(1.0f, 0.93f, 0.62f));
        P.Line({{17, 22}, {17, 38}}, Iron, 1.5f);
        P.Line({{39, 22}, {39, 38}}, Iron, 1.5f);
        P.Rect(19, 39, 18, 3, Gold);
        P.Shape({{14, 42}, {42, 42}, {44, 49}, {40, 52}, {16, 52}, {12, 49}}, Iron);
        P.Line({{15, 46}, {41, 46}}, StoneGray, 1.5f);
        break;
    case EKind::OilFlask:
        // A stoppered tin flask of lamp oil with a paper label.
        P.Rect(24, 6, 8, 7, Wood);
        P.Rect(23, 12, 10, 5, StoneGray);
        P.Shape({{17, 20}, {23, 16}, {33, 16}, {39, 20}, {41, 26}, {41, 48}, {37, 52}, {19, 52}, {15, 48}, {15, 26}}, Iron);
        P.Rect(16, 29, 24, 12, Cream * 0.8f + Gold * 0.2f);
        P.Line({{20, 33}, {36, 33}}, Wood, 1.5f);
        P.Line({{22, 37}, {34, 37}}, Wood, 1);
        P.Line({{17, 24}, {17, 48}}, StoneGray, 1.5f);
        break;
    case EKind::PouchArrows:
        // Up and down chevrons: this hotbar slot switches between the seed in her pack.
        P.Shape({{28, 4}, {48, 24}, {8, 24}}, Gold);
        P.Shape({{8, 32}, {48, 32}, {28, 52}}, Gold);
        break;
    case EKind::Unknown:        P.Unknown();
        break;
    }
    return P.GetLayer();
}
}
