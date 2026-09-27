#include "HomesteadHUD.h"
#include "HomesteadController.h"
#include "Engine/Canvas.h"
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
constexpr float MeterWidth = 168;
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

void AHomesteadHUD::Meter(const FString& Label, double Value, float X, float Y, FLinearColor Color)
{
    // Compact enough that all three sit left of the centred hotbar at 1080p.
    Panel(X - 12, Y - 8, MeterWidth, 55, FLinearColor(0.025f, 0.045f, 0.035f, 0.83f));
    ProtectFeedback(TEXT("need-meter"), X - 12, Y - 8, MeterWidth, 55);
    Write(Label, X, Y, 19, Ink);
    Write(FString::Printf(TEXT("%.0f"), Value), X + MeterWidth - 50, Y, 19, Value < 25 ? HudWarning : Muted);
    Panel(X, Y + 29, MeterWidth - 24, 5, FLinearColor(0.2f, 0.25f, 0.2f, 1));
    Panel(X, Y + 29, (MeterWidth - 24) * FMath::Clamp(static_cast<float>(Value / 100), 0.0f, 1.0f), 5, Color);
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
    const auto& State = PC->State();
    const double Hour = FMath::Fmod(State.hour, 24.0);
    const int H = FMath::FloorToInt(Hour);
    const int M = FMath::FloorToInt((Hour - H) * 60);
    Panel(30, 26, 460, 73, Pine);
    ProtectFeedback(TEXT("calendar-panel"), 30, 26, 460, 73);
    Write(FString::Printf(TEXT("%s  /  Day %d"), UTF8_TO_TCHAR(PC->Simulation().SeasonName()), PC->Simulation().DayNumber()), 48, 38, 24, Ink);
    Write(FString::Printf(TEXT("%02d:%02d   %s%s"), H, M, PC->Simulation().IsRaining() ? TEXT("Rain") : TEXT("Clear"),
        PC->IsPlanning() || PC->IsBookOpen() ? TEXT("   -   time paused") : TEXT("")), 48, 72, 18, Muted);

    if (PC->IsFailed())
    {
        Panel(0, 0, ViewWidth, ViewHeight, FLinearColor(0.025f, 0.04f, 0.035f, 0.85f));
        const float X = (ViewWidth - 760) * 0.5f;
        Write(TEXT("Time to try again"), X, ViewHeight * 0.39f, 43, Ink);
        Wrap(TEXT("You ran out of warmth, food, or energy. Return to a recovery checkpoint and try a different preparation."),
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
        const float Bottom = ViewHeight - 90;
        Meter(TEXT("Food"), State.hunger, 46, Bottom, FLinearColor(0.77f, 0.66f, 0.37f, 1));
        Meter(TEXT("Energy"), State.energy, 46 + MeterWidth + 11, Bottom, FLinearColor(0.66f, 0.76f, 0.52f, 1));
        Meter(TEXT("Warmth"), State.warmth, 46 + (MeterWidth + 11) * 2, Bottom, FLinearColor(0.83f, 0.56f, 0.37f, 1));
        const float Width = FMath::Min(880.0f, ViewWidth - 80);
        const float X = (ViewWidth - Width) * 0.5f;
        if (PC->IsPlanning())
        {
            Panel(X, ViewHeight - 262, Width, 132, Pine);
            ProtectFeedback(TEXT("planning-panel"), X, ViewHeight - 262, Width, 132);
            Wrap(PC->PlacementLabel(), X + 22, ViewHeight - 249, Width - 44, 24, Ink, 1);
            Wrap(PC->PlacementStatus(), X + 22, ViewHeight - 217, Width - 44, 21, PC->IsPlacementValid() ? Muted : HudWarning, 2);
            Write(PC->UsesGamepad() ? TEXT("Sticks: walk and aim   LB / RB: rotate   A: place   B: done")
                : TEXT("WASD + mouse: walk and aim   R / wheel: rotate   E / click: place   Esc: done"), X + 22, ViewHeight - 157, 20, HudGold);
        }
        else DrawInteractCue(*PC);
        Panel(FMath::Max(18.0f, ViewWidth - 704), 26, FMath::Min(686.0f, ViewWidth - 36), 46, Pine);
        Write(PC->UsesGamepad() ? TEXT("[Menu] Field book   [L3] Sprint   [R3] Camera distance")
                : TEXT("[I] Field book   [C] Craft   [B] Build   [Shift] Sprint   Ctrl+wheel: zoom"),
            FMath::Max(30.0f, ViewWidth - 690), 38, 19, Ink);
    }
    const FString Toast = PC->Toast();
    if (!Toast.IsEmpty())
    {
        const bool InBook = PC->IsBookOpen();
        const float Width = FMath::Min(900.0f, FMath::Max(80.0f, ViewWidth - (InBook ? 540 : 80)));
        const auto Lines = WrappedLines(Toast, Width - 44, 23);
        const float Height = FMath::Max(92.0f, 53.0f + (Lines.Num() - 1) * 30);
        const float X = InBook ? ViewWidth - Width - 30 : (ViewWidth - Width) * 0.5f;
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
    const float CenterX = FMath::Clamp(static_cast<float>(Screen.X) / UiScale, BoxWidth * 0.5f + 12, ViewWidth - BoxWidth * 0.5f - 12);
    const float Top = FMath::Clamp(static_cast<float>(Screen.Y) / UiScale - BoxHeight, 110.0f, ViewHeight - 260.0f);
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
