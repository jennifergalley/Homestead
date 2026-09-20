#include "HomesteadHUD.h"
#include "HomesteadController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
const FLinearColor Ink(0.93f, 0.93f, 0.84f, 1);
const FLinearColor Muted(0.71f, 0.77f, 0.69f, 1);
const FLinearColor Gold(0.92f, 0.74f, 0.43f, 1);
const FLinearColor Pine(0.055f, 0.09f, 0.075f, 0.96f);
const FLinearColor Warning(1.0f, 0.67f, 0.48f, 1);
}

void AHomesteadHUD::Write(const FString& Text, float X, float Y, float Size, FLinearColor Color)
{
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Font) return;
    const float Height = FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
    DrawText(Text, Color, X * UiScale, Y * UiScale, Font, Size / Height * UiScale, false);
}

void AHomesteadHUD::Panel(float X, float Y, float Width, float Height, FLinearColor Color)
{
    DrawRect(Color, X * UiScale, Y * UiScale, Width * UiScale, Height * UiScale);
}

void AHomesteadHUD::Wrap(const FString& Text, float X, float Y, float Width, float Size, FLinearColor Color, int MaxLines)
{
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    if (!Font) return;
    TArray<FString> Words;
    Text.ParseIntoArrayWS(Words);
    FString Line;
    int Lines = 0;
    const float FontScale = Size / FMath::Max(1.0f, static_cast<float>(Font->GetMaxCharHeight()));
    for (const FString& Word : Words)
    {
        FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float W = 0, H = 0;
        Canvas->StrLen(Font, Candidate, W, H);
        if (!Line.IsEmpty() && W * FontScale > Width)
        {
            Write(Line, X, Y + Lines * (Size + 7), Size, Color);
            if (++Lines >= MaxLines) return;
            Line = Word;
        }
        else Line = Candidate;
    }
    if (!Line.IsEmpty() && Lines < MaxLines) Write(Line, X, Y + Lines * (Size + 7), Size, Color);
}

void AHomesteadHUD::Meter(const FString& Label, double Value, float X, float Y, FLinearColor Color)
{
    Panel(X - 12, Y - 8, 200, 55, FLinearColor(0.025f, 0.045f, 0.035f, 0.83f));
    Write(Label, X, Y, 19, Ink);
    Write(FString::Printf(TEXT("%.0f"), Value), X + 143, Y, 19, Value < 25 ? Warning : Muted);
    Panel(X, Y + 29, 174, 5, FLinearColor(0.2f, 0.25f, 0.2f, 1));
    Panel(X, Y + 29, 174 * FMath::Clamp(static_cast<float>(Value / 100), 0.0f, 1.0f), 5, Color);
}

void AHomesteadHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    const AHomesteadController* PC = Cast<AHomesteadController>(PlayerOwner);
    if (!PC) return;
    UiScale = FMath::Clamp(Canvas->ClipY / 1080.0f, 0.4f, 3.0f);
    ViewWidth = Canvas->ClipX / UiScale;
    ViewHeight = Canvas->ClipY / UiScale;
    const auto& State = PC->State();
    const double Hour = FMath::Fmod(State.hour, 24.0);
    const int H = FMath::FloorToInt(Hour);
    const int M = FMath::FloorToInt((Hour - H) * 60);
    Panel(30, 26, 460, 73, Pine);
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
            X, ViewHeight * 0.39f + 180, 27, Gold);
    }
    else if (PC->IsBookOpen())
    {
        DrawBook(*PC);
    }
    else
    {
        const float Bottom = ViewHeight - 90;
        Meter(TEXT("Food"), State.hunger, 46, Bottom, FLinearColor(0.77f, 0.66f, 0.37f, 1));
        Meter(TEXT("Energy"), State.energy, 257, Bottom, FLinearColor(0.66f, 0.76f, 0.52f, 1));
        Meter(TEXT("Warmth"), State.warmth, 468, Bottom, FLinearColor(0.83f, 0.56f, 0.37f, 1));
        const float Width = FMath::Min(880.0f, ViewWidth - 80);
        const float X = (ViewWidth - Width) * 0.5f;
        if (PC->IsPlanning())
        {
            Panel(X, ViewHeight - 230, Width, 100, Pine);
            Wrap(PC->PlacementLabel(), X + 22, ViewHeight - 217, Width - 44, 24, Ink, 2);
            Write(PC->UsesGamepad() ? TEXT("Left stick: position   RB: rotate   A: place   B: done")
                : TEXT("WASD: position   R: rotate   E: place   Esc: done"), X + 22, ViewHeight - 157, 20, Gold);
        }
        else
        {
            const float ContextWidth = FMath::Min(650.0f, ViewWidth * 0.38f);
            const float ContextX = ViewWidth - ContextWidth - 32;
            Panel(ContextX, ViewHeight - 225, ContextWidth, 105, Pine);
            Wrap(PC->FocusTitle(), ContextX + 22, ViewHeight - 213, ContextWidth - 44, 21, Ink, 2);
            Write(PC->FocusActions(), ContextX + 22, ViewHeight - 153, 20, Gold);
        }
        Panel(FMath::Max(18.0f, ViewWidth - 704), 26, FMath::Min(686.0f, ViewWidth - 36), 46, Pine);
        Write(PC->UsesGamepad() ? TEXT("[Menu] Field book   [R3] Camera distance") : TEXT("[I] Field book   [C] Craft   [B] Build   Mouse wheel: zoom"),
            FMath::Max(30.0f, ViewWidth - 690), 38, 19, Ink);
        Panel(FMath::Max(18.0f, ViewWidth - 632), ViewHeight - 44, FMath::Min(614.0f, ViewWidth - 36), 29, Pine);
        Write(PC->HasHeroine() ? TEXT("Character prototype - more faces, clothes, and detail to come")
            : TEXT("Technical stand-in - character assets unavailable"),
            FMath::Max(30.0f, ViewWidth - 610), ViewHeight - 34, 16, Muted);
    }
    const FString Toast = PC->Toast();
    if (!Toast.IsEmpty())
    {
        const float Width = FMath::Min(900.0f, ViewWidth - 80);
        const float X = (ViewWidth - Width) * 0.5f;
        Panel(X, 113, Width, 92, Pine);
        Wrap(Toast, X + 22, 128, Width - 44, 23, PC->ToastIsError() ? Warning : Ink, 2);
    }
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
    Panel(X, Y, Width, Height, Pine);
    Write(TEXT("Field book"), X + 34, Y + 24, 38, Ink);
    Write(FString::Printf(TEXT("Pack %d / %d  |  World paused"), PC.Simulation().UsedCapacity(), Homestead::InventoryCapacity),
        X + Width - 365, Y + 37, 19, Muted);
    const TCHAR* Tabs[] = { TEXT("Pack"), TEXT("Craft"), TEXT("Build"), TEXT("Notes"), TEXT("Settings"), TEXT("Credits"), TEXT("Look") };
    const float TabWidth = (Width - 68) / 7;
    for (int Index = 0; Index < 7; ++Index)
    {
        const float TabX = X + 34 + Index * TabWidth;
        Write(Tabs[Index], TabX + 4, Y + 88, 23, Index == PC.BookPage() ? Gold : Muted);
        if (Index == PC.BookPage()) Panel(TabX, Y + 123, TabWidth - 20, 3, Gold);
    }

    const auto Rows = PC.Rows();
    const float RowHeight = 74;
    const int Visible = FMath::Max(1, FMath::FloorToInt((Height - 220) / RowHeight));
    const int First = FMath::Clamp(PC.SelectedRow() - Visible + 1, 0, FMath::Max(0, Rows.Num() - Visible));
    if (Rows.Num() == 0)
    {
        Write(TEXT("Nothing here yet."), X + 40, Y + 167, 25, Ink);
        Wrap(TEXT("A walk through the clearing will give you a useful beginning."), X + 40, Y + 210, Width - 80, 22, Muted);
    }
    for (int Index = First; Index < FMath::Min(Rows.Num(), First + Visible); ++Index)
    {
        const float RowY = Y + 148 + (Index - First) * RowHeight;
        const bool Selected = Index == PC.SelectedRow();
        if (Selected) Panel(X + 23, RowY - 5, Width - 46, RowHeight - 5, FLinearColor(0.09f, 0.14f, 0.105f, 1));
        Write(Rows[Index].Label, X + 40, RowY + 3, 24, Selected ? Gold : Ink);
        Wrap(Rows[Index].Detail, X + 40, RowY + 36, Width - 80, 18, Muted, 1);
    }
    if (Rows.Num() > Visible)
        Write(FString::Printf(TEXT("%d / %d"), PC.SelectedRow() + 1, Rows.Num()), X + Width - 108, Y + Height - 77, 18, Muted);
    Panel(X + 30, Y + Height - 64, Width - 60, 1, FLinearColor(0.28f, 0.35f, 0.29f, 1));
    const FString Footer = PC.BookPage() == 3 || PC.BookPage() == 5
        ? (PC.UsesGamepad() ? TEXT("D-pad: scroll   LB / RB: pages   B: close")
            : TEXT("Up / Down: scroll   Left / Right: pages   Esc: close"))
        : PC.BookPage() == 0
        ? (PC.UsesGamepad() ? TEXT("D-pad: select   LB/RB: pages   A: use   X: store   Y: take   B: close")
            : TEXT("Arrows: select/pages   Enter: use   F: store   G: take   Esc: close"))
        : (PC.UsesGamepad() ? TEXT("D-pad: select   LB / RB: pages   A: use   B: close")
            : TEXT("Up / Down: select   Left / Right: pages   Enter: use   Esc: close"));
    Write(Footer,
        X + 36, Y + Height - 43, 19, Muted);
}

void AHomesteadHUD::DrawAppearanceBook(const AHomesteadController& PC)
{
    const float X = 32;
    const float Y = 156;
    const float Width = FMath::Min(500.0f, ViewWidth * 0.35f);
    const float Height = FMath::Min(790.0f, ViewHeight - Y - 35);
    Panel(X, Y, Width, Height, Pine);
    Write(TEXT("Your look"), X + 28, Y + 24, 35, Ink);
    Write(PC.HasHeroine() ? TEXT("An early, editable heroine") : TEXT("Character assets unavailable"),
        X + 28, Y + 74, 18, PC.HasHeroine() ? Muted : Warning);
    const auto Options = PC.Rows();
    const float RowHeight = 88;
    const int Visible = FMath::Max(1, FMath::FloorToInt((Height - 240) / RowHeight));
    const int First = FMath::Clamp(PC.SelectedRow() - Visible + 1, 0, FMath::Max(0, Options.Num() - Visible));
    for (int Index = First; Index < FMath::Min(Options.Num(), First + Visible); ++Index)
    {
        const float RowY = Y + 122 + (Index - First) * RowHeight;
        if (Index == PC.SelectedRow())
            Panel(X + 15, RowY - 7, Width - 30, RowHeight - 6, FLinearColor(0.09f, 0.14f, 0.105f, 1));
        Write(Options[Index].Label, X + 28, RowY, 22, Index == PC.SelectedRow() ? Gold : Ink);
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
        X + 28, Y + Height - 39, Width - 56, 17, Gold, 1);
}
