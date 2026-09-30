#include "HomesteadHUD.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadMapComponent.h"
#include "UI/SHomesteadVitals.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FLinearColor Ink(0.93f, 0.93f, 0.84f, 1);
const FLinearColor Muted(0.71f, 0.77f, 0.69f, 1);
const FLinearColor HudGold(0.92f, 0.74f, 0.43f, 1);
const FLinearColor Pine(0.055f, 0.09f, 0.075f, 0.96f);
const FLinearColor HudWarning(1.0f, 0.67f, 0.48f, 1);
}

void AHomesteadHUD::Write(const FString& Text, float X, float Y, float Size, FLinearColor Color)
{
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Font) return;
    const float Height = FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
    if (bMeasureFeedback)
    {
        float W = 0, H = 0;
        Canvas->StrLen(Font, Text, W, H);
        const FBox2D Bounds(FVector2D(X, Y) * UiScale, FVector2D(X + W * Size / Height, Y + H * Size / Height) * UiScale);
        if (bDrawingToast) { ToastLines.Add(Text); ToastTextBounds.Add(Bounds); ToastColor = Color; }
        else if (!Text.IsEmpty()) FeedbackProtected.Emplace(Text, Bounds);
    }
    DrawText(Text, Color, X * UiScale, Y * UiScale, Font, Size / Height * UiScale, false);
}

void AHomesteadHUD::Panel(float X, float Y, float Width, float Height, FLinearColor Color)
{
    DrawRect(Color, X * UiScale, Y * UiScale, Width * UiScale, Height * UiScale);
}

namespace
{
FCanvasUVTri Tri(FVector2D A, FVector2D B, FVector2D C, FLinearColor Color)
{
    FCanvasUVTri T;
    T.V0_Pos = A; T.V1_Pos = B; T.V2_Pos = C;
    T.V0_Color = T.V1_Color = T.V2_Color = Color;
    return T;
}

// Sky colour of the dial by hour: night, rose dawn, soft day, amber dusk.
FLinearColor DialSky(double Hour)
{
    struct FKey { double Hour; FLinearColor Color; };
    static const FKey Keys[] = {
        {0.0, FLinearColor(0.035f, 0.06f, 0.10f, 1)}, {4.8, FLinearColor(0.035f, 0.06f, 0.10f, 1)},
        {6.0, FLinearColor(0.44f, 0.25f, 0.20f, 1)}, {8.0, FLinearColor(0.18f, 0.31f, 0.33f, 1)},
        {16.5, FLinearColor(0.18f, 0.31f, 0.33f, 1)}, {18.6, FLinearColor(0.50f, 0.26f, 0.11f, 1)},
        {20.0, FLinearColor(0.035f, 0.06f, 0.10f, 1)}, {24.0, FLinearColor(0.035f, 0.06f, 0.10f, 1)}};
    for (int32 Index = 1; Index < UE_ARRAY_COUNT(Keys); ++Index)
        if (Hour <= Keys[Index].Hour)
        {
            const double Alpha = (Hour - Keys[Index - 1].Hour) / FMath::Max(0.001, Keys[Index].Hour - Keys[Index - 1].Hour);
            return FMath::Lerp(Keys[Index - 1].Color, Keys[Index].Color, static_cast<float>(Alpha));
        }
    return Keys[0].Color;
}
}

void AHomesteadHUD::Disc(float CX, float CY, float Radius, FLinearColor Color, int32 Segments)
{
    Band(CX, CY, 0, Radius, 0, 2 * PI, Color, Segments);
}

void AHomesteadHUD::Band(float CX, float CY, float InnerRadius, float OuterRadius, float FromRadians, float ToRadians, FLinearColor Color, int32 Segments)
{
    // Angles run clockwise on screen from +X (y points down), so PI..2*PI is the upper half.
    TArray<FCanvasUVTri> Tris;
    const FVector2D C(CX * UiScale, CY * UiScale);
    const float R0 = InnerRadius * UiScale, R1 = OuterRadius * UiScale;
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float A0 = FMath::Lerp(FromRadians, ToRadians, static_cast<float>(Index) / Segments);
        const float A1 = FMath::Lerp(FromRadians, ToRadians, static_cast<float>(Index + 1) / Segments);
        const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
        if (R0 <= 0) Tris.Add(Tri(C, C + D0 * R1, C + D1 * R1, Color));
        else
        {
            Tris.Add(Tri(C + D0 * R0, C + D0 * R1, C + D1 * R1, Color));
            Tris.Add(Tri(C + D0 * R0, C + D1 * R1, C + D1 * R0, Color));
        }
    }
    FCanvasTriangleItem Item(Tris, GWhiteTexture);
    Item.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Item);
}

void AHomesteadHUD::Stroke(float X0, float Y0, float X1, float Y1, float Thickness, FLinearColor Color)
{
    const FVector2D A(X0 * UiScale, Y0 * UiScale), B(X1 * UiScale, Y1 * UiScale);
    FVector2D Dir = B - A;
    if (!Dir.Normalize()) return;
    const FVector2D Side = FVector2D(-Dir.Y, Dir.X) * (Thickness * UiScale * 0.5f);
    TArray<FCanvasUVTri> Tris;
    Tris.Add(Tri(A - Side, A + Side, B + Side, Color));
    Tris.Add(Tri(A - Side, B + Side, B - Side, Color));
    FCanvasTriangleItem Item(Tris, GWhiteTexture);
    Item.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Item);
}

void AHomesteadHUD::SunIcon(float CX, float CY, float Radius, FLinearColor Color)
{
    for (int32 Ray = 0; Ray < 8; ++Ray)
    {
        const float A = Ray * PI / 4;
        const FVector2D D(FMath::Cos(A), FMath::Sin(A));
        Stroke(CX + D.X * Radius * 1.35f, CY + D.Y * Radius * 1.35f, CX + D.X * Radius * 1.85f, CY + D.Y * Radius * 1.85f,
            FMath::Max(2.0f, Radius * 0.22f), Color);
    }
    Disc(CX, CY, Radius, Color);
}

void AHomesteadHUD::MoonIcon(float CX, float CY, float Radius, FLinearColor Color, FLinearColor Behind)
{
    Disc(CX, CY, Radius, Color);
    Disc(CX + Radius * 0.48f, CY - Radius * 0.28f, Radius * 0.86f, Behind);
}

void AHomesteadHUD::RainIcon(float CX, float CY, float Size)
{
    const float S = Size / 30.0f;
    const FLinearColor Cloud(0.86f, 0.87f, 0.82f, 1);
    const FLinearColor Drop(0.56f, 0.77f, 0.95f, 1);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const float X = CX + (-7.0f + Index * 8.0f) * S;
        Stroke(X, CY + 8 * S, X - 3 * S, CY + 16 * S, 2.6f * S, Drop);
    }
    Disc(CX - 8 * S, CY - 1 * S, 7 * S, Cloud);
    Disc(CX + 1 * S, CY - 6 * S, 9.5f * S, Cloud);
    Disc(CX + 10 * S, CY, 6 * S, Cloud);
    Panel(CX - 8 * S, CY - 1 * S, 18 * S, 7 * S, Cloud);
}

void AHomesteadHUD::DrawCalendar(const AHomesteadController& PC, float X, float Y)
{
    const auto& Sim = PC.Simulation();
    const double Hour = FMath::Fmod(PC.State().hour, 24.0);
    const bool bNight = Sim.IsNight();
    const bool bRain = Sim.IsRaining();
    const float Width = 460, Height = HomesteadHudLayout::CalendarHeight;
    Panel(X, Y, Width, Height, Pine);
    ProtectFeedback(TEXT("calendar-panel"), X, Y, Width, Height);

    // Time-of-day dial: the sun climbs a half-arc from dawn (6 AM) to dusk (7 PM), the moon by night.
    const float CX = X + 70, CY = Y + 78, R = 48;
    const FLinearColor Sky = DialSky(Hour);
    const FLinearColor MoonCream(0.95f, 0.92f, 0.80f, 1);
    Band(CX, CY, 0, R + 10, PI, 2 * PI, Sky, 36);
    Band(CX, CY, R - 1.0f, R + 1.0f, PI, 2 * PI, FLinearColor(Muted.R, Muted.G, Muted.B, 0.45f), 36);
    Stroke(CX, CY - R - 5, CX, CY - R + 5, 2, FLinearColor(Muted.R, Muted.G, Muted.B, 0.7f));
    const double Progress = bNight ? FMath::Fmod(Hour - 19.0 + 24.0, 24.0) / 11.0 : (Hour - 6.0) / 13.0;
    const float Angle = PI * (1.0f + static_cast<float>(FMath::Clamp(Progress, 0.0, 1.0)));
    const float BodyX = CX + FMath::Cos(Angle) * R, BodyY = CY + FMath::Sin(Angle) * R;
    if (bNight) MoonIcon(BodyX, BodyY, 10, MoonCream, Sky);
    else SunIcon(BodyX, BodyY, 9, bRain ? FLinearColor(0.80f, 0.70f, 0.52f, 1) : HudGold);
    // The ground hides the sun or moon as it sets, then the horizon line.
    const FLinearColor Solid(Pine.R, Pine.G, Pine.B, 1);
    Panel(CX - R - 20, CY, (R + 20) * 2, Y + Height - CY, Solid);
    Stroke(CX - R - 16, CY, CX + R + 16, CY, 2.5f, Muted);

    const float TextX = X + 140;
    // "Mon, Spring 12", with the days left in the season's last three days (lane A calendar).
    const Homestead::Calendar::Date Today = Sim.Today();
    Write(UTF8_TO_TCHAR(Homestead::Calendar::ShortDate(Today).c_str()), TextX, Y + 10, 24, Ink);
    if (PC.IsPlanning() || PC.IsBookOpen() || PC.IsShopScreenOpen())
    {
        const FString Paused = TEXT("time paused");
        Write(Paused, X + Width - 16 - TextWidth(Paused, 16), Y + 16, 16, Muted);
    }
    else if (const std::string Warning = Homestead::Calendar::SeasonWarning(Today); !Warning.empty())
    {
        const FString Left = UTF8_TO_TCHAR(Warning.c_str());
        Write(Left, X + Width - 16 - TextWidth(Left, 16), Y + 16, 16, HudGold);
    }
    const int32 H = FMath::FloorToInt(Hour);
    const int32 M = FMath::FloorToInt((Hour - H) * 60);
    const FString Clock = FString::Printf(TEXT("%d:%02d"), H % 12 == 0 ? 12 : H % 12, M);
    Write(Clock, TextX, Y + 42, 42, Ink);
    Write(H < 12 ? TEXT("AM") : TEXT("PM"), TextX + TextWidth(Clock, 42) + 8, Y + 58, 24, HudGold);

    // Weather: sun, rain cloud, or a clear night's moon.
    const float IconX = X + 350, IconY = Y + 68;
    if (bRain) RainIcon(IconX, IconY, 34);
    else if (bNight) MoonIcon(IconX, IconY, 11, MoonCream, Solid);
    else SunIcon(IconX, IconY, 9, HudGold);
    Write(bRain ? TEXT("Rain") : bNight ? TEXT("Clear") : TEXT("Sunny"), IconX + 28, Y + 56, 21, Muted);
}

TArray<FString> AHomesteadHUD::WrappedLines(const FString& Text, float Width, float Size)
{
    TArray<FString> Lines;
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Font) return Lines;
    TArray<FString> Words;
    Text.ParseIntoArrayWS(Words);
    FString Line;
    const float FontScale = Size / FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
    for (const FString& Word : Words)
    {
        FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float W = 0, H = 0;
        Canvas->StrLen(Font, Candidate, W, H);
        if (!Line.IsEmpty() && W * FontScale > Width)
        {
            Lines.Add(Line);
            Line = Word;
        }
        else Line = Candidate;
    }
    if (!Line.IsEmpty()) Lines.Add(Line);
    return Lines;
}

void AHomesteadHUD::Wrap(const FString& Text, float X, float Y, float Width, float Size, FLinearColor Color, int MaxLines)
{
    const auto Lines = WrappedLines(Text, Width, Size);
    for (int32 Index = 0; Index < FMath::Min(Lines.Num(), MaxLines); ++Index)
        Write(Lines[Index], X, Y + Index * (Size + 7), Size, Color);
}

void AHomesteadHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    const AHomesteadController* PC = Cast<AHomesteadController>(PlayerOwner);
    if (!PC) return;
    if (PC->HasNativeMenu()) return;
    UiScale = FMath::Clamp(Canvas->ClipY / 1080.0f, 0.4f, 1.5f);
    ViewWidth = Canvas->ClipX / UiScale;
    ViewHeight = Canvas->ClipY / UiScale;
    static const bool MeasureFeedback = FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"))
        && FParse::Param(FCommandLine::Get(), TEXT("HomesteadFeedbackTest"));
    bMeasureFeedback = MeasureFeedback;
    if (bMeasureFeedback)
    {
        FeedbackViewport = FVector2D(Canvas->ClipX, Canvas->ClipY);
        ToastSource.Reset(); ToastLines.Reset(); ToastTextBounds.Reset(); FeedbackProtected.Reset();
        ToastBounds = FBox2D(ForceInit);
        }
        // The calendar sits top-right; the key hints take the top-left. The vitals stack sits under it
    // (SHomesteadVitals::Top = CalendarTop + CalendarHeight + 8).
    const float CalendarX = FMath::Max(30.0f, ViewWidth - 30 - 460);
    DrawCalendar(*PC, CalendarX, HomesteadHudLayout::CalendarTop);

    if (PC->IsFailed())
    {
        Panel(0, 0, ViewWidth, ViewHeight, FLinearColor(0.025f, 0.04f, 0.035f, 0.85f));
        const float X = (ViewWidth - 760) * 0.5f;
        Write(TEXT("Time to try again"), X, ViewHeight * 0.39f, 43, Ink);
        Wrap(TEXT("You ran out of food or energy. Return to a recovery checkpoint and try a different preparation."),
            X, ViewHeight * 0.39f + 76, 740, 25, Muted);
        Write(PC->UsesGamepad() ? TEXT("[A] Retry checkpoint") : TEXT("[E / Enter] Retry checkpoint"),
            X, ViewHeight * 0.39f + 180, 27, HudGold);
    }
    else if (PC->IsBookOpen())
    {
        DrawBook(*PC);
    }
    else
    {
        if (PC->IsShopScreenOpen()) return;
        // Energy (and the woodland's food), the purse and Well fed are the Slate vitals stack (UI/SHomesteadVitals) under the calendar.
        const FBox2D Vitals = HomesteadMenus::SHomesteadVitals::LogicalBox(ViewWidth, *PC);
        ProtectFeedback(TEXT("vitals"), Vitals.Min.X, Vitals.Min.Y, Vitals.GetSize().X, Vitals.GetSize().Y);
        const float Width = FMath::Min(880.0f, ViewWidth - 80);
        const float X = (ViewWidth - Width) * 0.5f;
        if (PC->IsPlanning())
        {
            Panel(X, ViewHeight - 262, Width, 132, Pine);
            ProtectFeedback(TEXT("planning-panel"), X, ViewHeight - 262, Width, 132);
            Wrap(PC->PlacementLabel(), X + 22, ViewHeight - 249, Width - 44, 24, Ink, 1);
            Wrap(PC->PlacementStatus(), X + 22, ViewHeight - 217, Width - 44, 21, PC->IsPlacementValid() ? Muted : HudWarning, 2);
            if (PC->IsDeconstructing())
                Write(PC->UsesGamepad() ? TEXT("Sticks: walk and aim   A: take down   Y / X: build instead   B: done")
                    : TEXT("WASD + mouse: walk and aim   E / click: take down   X: build instead   Esc: done"), X + 22, ViewHeight - 157, 20, HudGold);
            else
                Write(PC->UsesGamepad() ? TEXT("Sticks: walk and aim   LB / RB: rotate   A: place   Y / X: take down   B: done")
                    : TEXT("WASD + mouse: walk and aim   R / wheel: rotate   E / click: place   X: take down   Esc: done"), X + 22, ViewHeight - 157, 20, HudGold);
        }
        else DrawInteractCue(*PC);
        if (const UHomesteadMapComponent* Map = PC->MapPresenter(); Map && Map->IsMinimapVisible())
        {
            const FBox2D Minimap = UHomesteadMapComponent::MinimapBox(ViewWidth, ViewHeight);
            ProtectFeedback(TEXT("minimap"), Minimap.Min.X, Minimap.Min.Y, Minimap.GetSize().X, Minimap.GetSize().Y);
        }
        // The wheel picks the hotbar tool; Ctrl+wheel zooms the camera (AHomesteadController::InputKey).
        // Sprint is a toggle, so the hint says which way it's set.
        const auto* Heroine = Cast<AHomesteadCharacter>(PC->GetPawn());
        const TCHAR* Sprint = Heroine && Heroine->IsSprintOn() ? TEXT("Sprint: on") : TEXT("Sprint: off");
        const FString Hints = PC->UsesGamepad()
            ? FString::Printf(TEXT("[Menu] Field book   [L3] %s   [R3] Camera distance"), Sprint)
            : FString::Printf(TEXT("[I] Field book   [C] Craft   [B] Build   [Shift] %s   Wheel: tool   Ctrl+wheel: zoom"), Sprint);
        const float HintsWidth = FMath::Max(120.0f, FMath::Min(TextWidth(Hints, 19) + 26, CalendarX - 16 - 30));
        Panel(30, 26, HintsWidth, 46, Pine);
        Write(Hints, 42, 38, 19, Ink);
    }
    const FString Toast = PC->Toast();
    if (!Toast.IsEmpty())
    {
        const bool InBook = PC->IsBookOpen();
        float Width = FMath::Min(900.0f, FMath::Max(80.0f, ViewWidth - (InBook ? 540 : 80)));
        // Outside the book the toast is centred, but never runs under the vitals stack at the
        // top-right (narrow windows): it gives up width, then slides left.
        const float VitalsLeft = HomesteadMenus::SHomesteadVitals::LogicalBox(ViewWidth, *PC).Min.X - 16;
        float X = InBook ? 30 : (ViewWidth - Width) * 0.5f;
        if (!InBook && X + Width > VitalsLeft)
        {
            Width = FMath::Clamp(VitalsLeft - 30, 300.0f, Width);
            X = FMath::Max(30.0f, FMath::Min((ViewWidth - Width) * 0.5f, VitalsLeft - Width));
        }
        const auto Lines = WrappedLines(Toast, Width - 44, 23);
        const float Height = FMath::Max(92.0f, 53.0f + (Lines.Num() - 1) * 30);
        // In the book the toast takes the top-left, clear of the calendar at the top-right.
        const float Y = InBook ? 26 : 113;
        bDrawingToast = true;
        if (bMeasureFeedback)
        {
            ToastSource = Toast;
            ToastBounds = FBox2D(FVector2D(X, Y) * UiScale, FVector2D(X + Width, Y + Height) * UiScale);
        }
        Panel(X, Y, Width, Height, Pine);
        for (int32 Index = 0; Index < Lines.Num(); ++Index)
            Write(Lines[Index], X + 22, Y + 15 + Index * 30, 23, PC->ToastIsError() ? HudWarning : Ink);
        bDrawingToast = false;
    }
}

float AHomesteadHUD::TextWidth(const FString& Text, float Size) const
{
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Font || !Canvas) return 0;
    float W = 0, H = 0;
    Canvas->StrLen(Font, Text, W, H);
    return W * Size / FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
}

void AHomesteadHUD::DrawInteractCue(const AHomesteadController& PC)
{
    const APawn* Pawn = PC.GetPawn();
    const FString Actions = PC.FocusActions();
    // Nothing to do here: the open woodland's "[I] Field book" hint already sits in the top bar.
    if (!Pawn || Actions.IsEmpty() || Actions.Contains(TEXT("Field book"))) return;
    const FVector Screen = Project(Pawn->GetActorLocation() + FVector(0, 0, 112), false);
    if (Screen.Z <= 0) return;
    // "[E] Gather   [LMB] Clear with Knife" -> (key, verb) pairs; unkeyed hints stay plain text.
    TArray<FString> Parts;
    Actions.ParseIntoArray(Parts, TEXT("   "));
    struct FCue { FString Key, Verb; };
    TArray<FCue> Cues;
    for (FString Part : Parts)
    {
        Part.TrimStartAndEndInline();
        FCue Cue;
        if (Part.StartsWith(TEXT("[")) && Part.Contains(TEXT("]")))
        {
            const int32 Close = Part.Find(TEXT("]"));
            Cue.Key = Part.Mid(1, Close - 1);
            Cue.Verb = Part.Mid(Close + 1).TrimStart();
        }
        else Cue.Verb = Part;
        // A keyed hint retires once she's done that action a few times; guidance text stays.
        if (!Cue.Key.IsEmpty() && PC.IsHintRetired(Cue.Verb)) continue;
        if (!Cue.Verb.IsEmpty() || !Cue.Key.IsEmpty()) Cues.Add(Cue);
    }
    if (Cues.IsEmpty()) return;
    constexpr float Size = 21, KeySize = 17, BadgeH = 28, Gap = 22, KeyPad = 8, Space = 9;
    float Width = 0;
    for (int32 Index = 0; Index < Cues.Num(); ++Index)
    {
        if (Index) Width += Gap;
        if (!Cues[Index].Key.IsEmpty()) Width += FMath::Max(BadgeH, TextWidth(Cues[Index].Key, KeySize) + KeyPad * 2) + Space;
        Width += TextWidth(Cues[Index].Verb, Size);
    }
    FString Title = PC.FocusTitle();
    const float TitleSize = 16;
    const float TitleWidth = FMath::Min(TextWidth(Title, TitleSize), 520.0f);
    const float BoxWidth = FMath::Max(Width, TitleWidth) + 30;
    const float BoxHeight = Title.IsEmpty() ? BadgeH + 16 : BadgeH + 40;
    float CenterX = FMath::Clamp(static_cast<float>(Screen.X) / UiScale, BoxWidth * 0.5f + 12, ViewWidth - BoxWidth * 0.5f - 12);
    const float Top = FMath::Clamp(static_cast<float>(Screen.Y) / UiScale - BoxHeight, 110.0f, ViewHeight - 260.0f);
    if (const UHomesteadMapComponent* Map = PC.MapPresenter(); Map && Map->IsMinimapVisible())
    {
        // Keep clear of the bottom-right minimap, including its bezel and the N marker on the rim.
        constexpr float RimClearance = 16, Margin = 10;
        const FBox2D Minimap = UHomesteadMapComponent::MinimapBox(ViewWidth, ViewHeight).ExpandBy(RimClearance);
        if (Top + BoxHeight + Margin > Minimap.Min.Y && CenterX + BoxWidth * 0.5f + Margin > Minimap.Min.X)
            CenterX = FMath::Max(BoxWidth * 0.5f + 12, Minimap.Min.X - Margin - BoxWidth * 0.5f);
    }
    if (!PC.IsShopScreenOpen())
    {
        // And clear of the vitals stack under the calendar at the top-right.
        constexpr float Margin = 10;
        const FBox2D Vitals = HomesteadMenus::SHomesteadVitals::LogicalBox(ViewWidth, PC);
        if (Top < Vitals.Max.Y + Margin && CenterX + BoxWidth * 0.5f + Margin > Vitals.Min.X)
            CenterX = FMath::Max(BoxWidth * 0.5f + 12, Vitals.Min.X - Margin - BoxWidth * 0.5f);
    }
    Panel(CenterX - BoxWidth * 0.5f, Top, BoxWidth, BoxHeight, FLinearColor(0.02f, 0.035f, 0.028f, 0.58f));
    ProtectFeedback(TEXT("interact-cue"), CenterX - BoxWidth * 0.5f, Top, BoxWidth, BoxHeight);
    float Y = Top + 8;
    if (!Title.IsEmpty())
    {
        Write(Title, CenterX - TitleWidth * 0.5f, Y, TitleSize, Muted);
        Y += 24;
    }
    float X = CenterX - Width * 0.5f;
    for (int32 Index = 0; Index < Cues.Num(); ++Index)
    {
        if (Index) X += Gap;
        const FCue& Cue = Cues[Index];
        if (!Cue.Key.IsEmpty())
        {
            const float BadgeW = FMath::Max(BadgeH, TextWidth(Cue.Key, KeySize) + KeyPad * 2);
            Panel(X, Y, BadgeW, BadgeH, HudGold);
            Write(Cue.Key, X + (BadgeW - TextWidth(Cue.Key, KeySize)) * 0.5f, Y + (BadgeH - KeySize) * 0.5f - 1, KeySize, Pine);
            X += BadgeW + Space;
        }
        Write(Cue.Verb, X + 1, Y + (BadgeH - Size) * 0.5f, Size, FLinearColor(0, 0, 0, 0.7f));
        Write(Cue.Verb, X, Y + (BadgeH - Size) * 0.5f - 1, Size, Ink);
        X += TextWidth(Cue.Verb, Size);
    }
}

void AHomesteadHUD::MeasureBookLine(const FString& Text, float Width, float Size, const TCHAR* TextRole)
{
    static const bool Enabled = FParse::Param(FCommandLine::Get(), TEXT("HomesteadBookClarityTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("HomesteadVideoSyncTest"));
    if (!Enabled) return;
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Canvas || !Font) { BookTextOverflow += TEXT("Missing native font or canvas; "); return; }
    float W = 0, H = 0;
    Canvas->StrLen(Font, Text, W, H);
    const float Measured = W * Size / FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
    if (Measured > Width) BookTextOverflow += FString::Printf(TEXT("%s overflows; "), TextRole);
    BookMeasurements += FString::Printf(TEXT("%s: pixels=%.2f available=%.2f text=%s\n"),
        TextRole, Measured * UiScale, Width * UiScale, *Text);
}

void AHomesteadHUD::DrawBook(const AHomesteadController& PC)
{
    if (PC.BookPage() == 6)
    {
        DrawAppearanceBook(PC);
        return;
    }
    Panel(0, 0, ViewWidth, ViewHeight, FLinearColor(0.02f, 0.035f, 0.028f, 0.62f));
    const float Width = FMath::Min(1180.0f, ViewWidth - 72);
    const float Height = FMath::Min(810.0f, ViewHeight - 120);
    const float X = (ViewWidth - Width) * 0.5f;
    const float Y = (ViewHeight - Height) * 0.5f;
    RenderedBookPage = PC.BookPage();
    BookTextOverflow.Reset();
    BookMeasurements.Reset();
    MeasureBookLine(PC.BookTitle(), Width - 423, 38, TEXT("title"));
    Panel(X, Y, Width, Height, Pine);
    ProtectFeedback(TEXT("book-panel"), X, Y, Width, Height);
    Write(PC.BookTitle(), X + 34, Y + 24, 38, Ink);
    const FString Capacity = FString::Printf(TEXT("Carried %d / %d  |  World paused"), PC.Simulation().UsedCapacity(), Homestead::InventoryCapacity);
    MeasureBookLine(Capacity, 331, 19, TEXT("capacity"));
    Write(Capacity,
        X + Width - 365, Y + 37, 19, Muted);
    const TCHAR* Tabs[] = { TEXT("Pack"), TEXT("Craft"), TEXT("Build"), TEXT("Notes"), TEXT("Settings"), TEXT("Credits"), TEXT("Look") };
    const float TabWidth = (Width - 68) / 7;
    for (int Index = 0; Index < 7; ++Index)
    {
        if (Index == 3) continue; // The Guidebook page is retired.
        const float TabX = X + 34 + Index * TabWidth;
        Write(Tabs[Index], TabX + 4, Y + 88, 23, Index == PC.BookPage() ? HudGold : Muted);
        if (Index == PC.BookPage()) Panel(TabX, Y + 123, TabWidth - 20, 3, HudGold);
    }

    const auto Rows = PC.Rows();
    const FString Summary = PC.BookSummary();
    const float RowTop = Summary.IsEmpty() ? 148 : 180;
    MeasureBookLine(Summary, Width - 80, 18, TEXT("summary"));
    if (!Summary.IsEmpty()) Wrap(Summary, X + 40, Y + 144, Width - 80, 18, Muted, 1);
    const float RowHeight = 74;
    const int Visible = FMath::Max(1, FMath::FloorToInt((Height - RowTop - 72) / RowHeight));
    const int First = FMath::Clamp(PC.SelectedRow() - Visible + 1, 0, FMath::Max(0, Rows.Num() - Visible));
    if (Rows.Num() == 0)
    {
        const FString EmptyTitle = TEXT("Your pack is empty.");
        const FString EmptyDetail = TEXT("Gather supplies in the woodland, or take items from a nearby chest.");
        MeasureBookLine(EmptyTitle, Width - 80, 25, TEXT("empty-title"));
        MeasureBookLine(EmptyDetail, Width - 80, 22, TEXT("empty-detail"));
        Write(EmptyTitle, X + 40, Y + RowTop + 19, 25, Ink);
        Wrap(EmptyDetail,
            X + 40, Y + RowTop + 62, Width - 80, 22, Muted);
    }
    for (int Index = First; Index < FMath::Min(Rows.Num(), First + Visible); ++Index)
    {
        const float RowY = Y + RowTop + (Index - First) * RowHeight;
        const bool Selected = Index == PC.SelectedRow();
        MeasureBookLine(Rows[Index].Label, Width - 80, 24, TEXT("row-label"));
        MeasureBookLine(Rows[Index].Detail, Width - 80, 18, TEXT("row-detail"));
        if (RowY + 54 > Y + Height - 91) BookTextOverflow += TEXT("Row overlaps footer area; ");
        if (Selected)
        {
            Panel(X + 23, RowY - 5, Width - 46, RowHeight - 5, FLinearColor(0.09f, 0.14f, 0.105f, 1));
            ProtectFeedback(TEXT("selected-row"), X + 23, RowY - 5, Width - 46, RowHeight - 5);
        }
        Write(Rows[Index].Label, X + 40, RowY + 3, 24, Selected ? HudGold : Ink);
        Wrap(Rows[Index].Detail, X + 40, RowY + 36, Width - 80, 18, Muted, 1);
    }
    if (Rows.Num() > Visible)
        Write(FString::Printf(TEXT("%d / %d"), PC.SelectedRow() + 1, Rows.Num()), X + Width - 108, Y + Height - 91, 18, Muted);
    Panel(X + 30, Y + Height - 64, Width - 60, 1, FLinearColor(0.28f, 0.35f, 0.29f, 1));
    MeasureBookLine(PC.BookFooter(), Width - 72, 19, TEXT("footer"));
    Write(PC.BookFooter(),
        X + 36, Y + Height - 43, 19, Muted);
}

void AHomesteadHUD::DrawAppearanceBook(const AHomesteadController& PC)
{
    const float X = 32;
    const float Y = 156;
    const float Width = FMath::Min(500.0f, ViewWidth * 0.35f);
    const float Height = FMath::Min(790.0f, ViewHeight - Y - 35);
    Panel(X, Y, Width, Height, Pine);
    ProtectFeedback(TEXT("look-panel"), X, Y, Width, Height);
    Write(TEXT("Your look"), X + 28, Y + 24, 35, Ink);
    Write(PC.HasHeroine() ? TEXT("An early, editable heroine") : TEXT("Character assets unavailable"),
        X + 28, Y + 74, 18, PC.HasHeroine() ? Muted : HudWarning);
    const auto Options = PC.Rows();
    const float RowHeight = 88;
    const int Visible = FMath::Max(1, FMath::FloorToInt((Height - 240) / RowHeight));
    const int First = FMath::Clamp(PC.SelectedRow() - Visible + 1, 0, FMath::Max(0, Options.Num() - Visible));
    for (int Index = First; Index < FMath::Min(Options.Num(), First + Visible); ++Index)
    {
        const float RowY = Y + 122 + (Index - First) * RowHeight;
        if (Index == PC.SelectedRow())
        {
            Panel(X + 15, RowY - 7, Width - 30, RowHeight - 6, FLinearColor(0.09f, 0.14f, 0.105f, 1));
            ProtectFeedback(TEXT("selected-look-row"), X + 15, RowY - 7, Width - 30, RowHeight - 6);
        }
        Write(Options[Index].Label, X + 28, RowY, 22, Index == PC.SelectedRow() ? HudGold : Ink);
        Wrap(Options[Index].Detail, X + 28, RowY + 31, Width - 56, 16, Muted, 2);
    }
    if (Options.Num() > Visible)
        Write(FString::Printf(TEXT("%d / %d"), PC.SelectedRow() + 1, Options.Num()), X + Width - 86, Y + 77, 17, Muted);
    Panel(X + 25, Y + Height - 109, Width - 50, 1, FLinearColor(0.28f, 0.35f, 0.29f, 1));
    Wrap(PC.UsesGamepad() ? TEXT("D-pad: select   A: change   LB/RB: pages   B: close")
        : TEXT("Arrows: select/pages   Enter: change   Esc: close"),
        X + 28, Y + Height - 91, Width - 56, 17, Muted, 2);
    Wrap(PC.UsesGamepad() ? TEXT("Right stick: orbit   R3: view distance")
        : TEXT("Mouse: orbit   Mouse wheel: view distance"),
        X + 28, Y + Height - 39, Width - 56, 17, HudGold, 1);
}
