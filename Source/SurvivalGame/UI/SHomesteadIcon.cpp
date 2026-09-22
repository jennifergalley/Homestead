#include "SHomesteadIcon.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#include <initializer_list>

namespace HomesteadIcons
{
namespace
{
    const FLinearColor Cream(0.93f, 0.93f, 0.84f);
    const FLinearColor Pine(0.055f, 0.09f, 0.075f);
    const FLinearColor Charcoal(0.045f, 0.035f, 0.025f);
    const FLinearColor Wood(0.49f, 0.29f, 0.13f);
    const FLinearColor LeafGreen(0.32f, 0.52f, 0.19f);
    const FLinearColor Berry(0.67f, 0.16f, 0.25f);
    const FLinearColor RootOrange(0.86f, 0.46f, 0.21f);
    const FLinearColor WaterBlue(0.36f, 0.65f, 0.68f);
    const FLinearColor StoneGray(0.53f, 0.59f, 0.55f);

    class FIconPainter
    {
    public:
        FIconPainter(const FGeometry& InGeometry, FSlateWindowElementList& InElements,
            int32 InLayer, const FLinearColor& InStyleTint, const FLinearColor& InAccent,
            ESlateDrawEffect InEffects)
            : Accent(InAccent.R, InAccent.G, InAccent.B, 1.0f), Geometry(InGeometry),
              Elements(InElements), Layer(InLayer), StyleTint(InStyleTint), Effects(InEffects)
        {
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
                Effects | ESlateDrawEffect::NoPixelSnapping, Color * StyleTint);
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
                Effects, Charcoal * StyleTint);
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
                Scaled, Effects, Color * StyleTint, true, Width * Scale);
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
                        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), Effects, Color * StyleTint);
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
    };
}

void SHomesteadIcon::Construct(const FArguments& InArgs)
{
    Kind = InArgs._Kind;
    Tint = InArgs._Tint;
}

bool SHomesteadIcon::ComputeVolatility() const
{
    return Kind.IsBound() || Tint.IsBound() || SLeafWidget::ComputeVolatility();
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
        {FName(TEXT("craft")), EKind::Craft},
        {FName(TEXT("build")), EKind::Build},
        {FName(TEXT("guide")), EKind::Guide},
        {FName(TEXT("settings")), EKind::Settings},
        {FName(TEXT("credits")), EKind::Credits},
        {FName(TEXT("appearance")), EKind::Appearance},
        {FName(TEXT("knife")), EKind::Knife},
        {FName(TEXT("branch")), EKind::Branch},
        {FName(TEXT("stone")), EKind::Stone},
        {FName(TEXT("fiber")), EKind::Fiber},
        {FName(TEXT("berries")), EKind::Berries},
        {FName(TEXT("roots")), EKind::Roots},
        {FName(TEXT("flowers")), EKind::Flowers},
        {FName(TEXT("seeds")), EKind::Seeds},
        {FName(TEXT("hatchet")), EKind::Hatchet},
        {FName(TEXT("digging-stick")), EKind::DiggingStick},
        {FName(TEXT("watering-can")), EKind::WateringCan},
        {FName(TEXT("water")), EKind::Water},
        {FName(TEXT("roasted-roots")), EKind::RoastedRoots},
        {FName(TEXT("herbed-roots")), EKind::HerbedRoots},
        {FName(TEXT("timber")), EKind::Timber},
        {FName(TEXT("firewood")), EKind::Firewood},
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
        {FName(TEXT("woven-footwraps")), EKind::WovenFootwraps}
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

    const ESlateDrawEffect Effects = ShouldBeEnabled(bParentEnabled)
        ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
    FIconPainter P(AllottedGeometry, OutDrawElements, LayerId,
        InWidgetStyle.GetColorAndOpacityTint(), Tint.Get(), Effects);
    if (P.GetScale() <= 0.0f)
    {
        return LayerId;
    }

    const FLinearColor Gold = P.Accent;
    switch (IconKind)
    {
    case EKind::Pack:
        P.Line({{21, 14}, {21, 8}, {34, 8}, {34, 14}}, Cream, 3);
        P.Shape({{16, 15}, {39, 15}, {44, 23}, {43, 47}, {12, 47}, {11, 23}}, Wood);
        P.Shape({{14, 15}, {41, 15}, {39, 28}, {28, 32}, {16, 28}}, Gold);
        P.Rect(24, 26, 7, 10, Pine);
        P.Rect(26, 28, 3, 5, Cream);
        P.Line({{17, 38}, {17, 43}, {38, 43}, {38, 38}}, Cream, 1.5f);
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
    case EKind::Credits:
        P.Line({{19, 43}, {11, 34}, {10, 22}, {17, 11}}, Gold, 2);
        P.Line({{37, 43}, {45, 34}, {46, 22}, {39, 11}}, Gold, 2);
        P.Leaf({12, 29}, {6, 20}, 3);
        P.Leaf({13, 36}, {7, 30}, 3);
        P.Leaf({44, 29}, {50, 20}, 3);
        P.Leaf({43, 36}, {49, 30}, 3);
        P.Disc(28, 22, 6, Cream);
        P.Shape({{18, 39}, {19, 33}, {24, 29}, {32, 29}, {37, 33}, {38, 39}}, Gold);
        P.Line({{21, 45}, {28, 49}, {35, 45}}, Cream, 2);
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
        P.Line({{13, 13}, {20, 7}, {28, 12}, {23, 19}, {13, 13}}, Gold, 3);
        P.Line({{22, 17}, {37, 39}}, Wood, 6);
        P.Shape({{32, 37}, {38, 35}, {45, 48}, {36, 46}}, Gold);
        P.Line({{37, 39}, {41, 45}}, Pine, 1.5f);
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
    case EKind::Unknown:
        P.Unknown();
        break;
    }
    return P.GetLayer();
}
}
