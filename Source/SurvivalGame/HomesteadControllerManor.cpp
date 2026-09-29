// add-ruined-manor-and-arrival: the new-game Names step, the arrival title card and save labels.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadManor.h"
#include "UI/SHomesteadArrival.h"
#include "UI/SHomesteadNames.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/Layout/SBox.h"

namespace
{
FString FromUtf8(const std::string& Text) { return UTF8_TO_TCHAR(Text.c_str()); }

// Agent editors (Start-EditorMcp.ps1 passes -HomesteadSkipNewGameSetup) start new Estate games with the
// default names instead of stopping on Appearance and "Who comes home?". Set it to 0 in the console to
// test the setup steps themselves.
TAutoConsoleVariable<int32> CVarSkipNewGameSetup(TEXT("homestead.SkipNewGameSetup"),
    FParse::Param(FCommandLine::Get(), TEXT("HomesteadSkipNewGameSetup")) ? 1 : 0,
    TEXT("1: a new Estate game skips the Appearance and Names steps and uses the default names."));
}

FString AHomesteadController::EstateName() const
{
    return FromUtf8(Sim.EstateName());
}

FString AHomesteadController::CurrentSaveLabel() const
{
    return FromUtf8(Homestead::Manor::SaveLabel(State(), Sim.SeasonName(), Sim.DayNumber()));
}

void AHomesteadController::BeginNewGameSetup()
{
    if (IsFailed()) return;
    if (CVarSkipNewGameSetup.GetValueOnGameThread() != 0)
    {
        const auto Result = Sim.SetNames(Homestead::Manor::DefaultHeroineName, Homestead::Manor::DefaultFamilyName,
            Homestead::Manor::DefaultEstateName);
        UE_LOG(LogTemp, Display, TEXT("New-game setup skipped (homestead.SkipNewGameSetup): %s"),
            Result ? TEXT("default names") : *FromUtf8(Result.message));
        bNewGameSetup = false;
        HideNames();
        if (bBookOpen) CloseBook();
        return;
    }
    bNewGameSetup = true;
    HideNames();
    OpenBook(6);
    Notify(TEXT("Choose her look, then close the book to name her."));
}

void AHomesteadController::HomesteadNewGameSetup() { BeginNewGameSetup(); }

void AHomesteadController::ShowNames()
{
    if (NamesWidget.IsValid() || !GEngine || !GEngine->GameViewport) return;
    const auto& Now = State();
    NamesWidget = SNew(HomesteadMenus::SHomesteadNames)
        .Heroine(Now.heroineName.empty() ? FString(Homestead::Manor::DefaultHeroineName) : FromUtf8(Now.heroineName))
        .Family(Now.familyName.empty() ? FString(Homestead::Manor::DefaultFamilyName) : FromUtf8(Now.familyName))
        .Estate(Now.estateName.empty() ? FString(Homestead::Manor::DefaultEstateName) : FromUtf8(Now.estateName))
        .OnBegin_Lambda([this](const FString& Heroine, const FString& Family, const FString& Estate)
        { FinishNames(Heroine, Family, Estate); })
        .OnBack_Lambda([this]()
        {
            // Back to Appearance; closing it returns here.
            HideNames();
            OpenBook(6);
        });
    // Viewport content is already DPI-scaled by the engine's resolution curve.
    NamesRoot = SNew(SBox)
        [
            NamesWidget.ToSharedRef()
        ];
    GEngine->GameViewport->AddViewportWidgetContent(NamesRoot.ToSharedRef(), 110);
    FlushPressedKeys();
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(NamesWidget);
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    if (FSlateApplication::IsInitialized())
        FSlateApplication::Get().SetKeyboardFocus(NamesWidget, EFocusCause::SetDirectly);
}

void AHomesteadController::HideNames()
{
    if (NamesRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(NamesRoot.ToSharedRef());
    const bool bWasShown = NamesWidget.IsValid();
    NamesWidget.Reset();
    NamesRoot.Reset();
    if (bWasShown && !bBookOpen)
    {
        FlushPressedKeys();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
}

void AHomesteadController::FinishNames(const FString& Heroine, const FString& Family, const FString& Estate)
{
    const auto Result = Sim.SetNames(TCHAR_TO_UTF8(*Heroine), TCHAR_TO_UTF8(*Family), TCHAR_TO_UTF8(*Estate));
    if (!Result) { Notify(FromUtf8(Result.message), true); return; }
    bNewGameSetup = false;
    HideNames();
    ShowArrival();
    SaveSlot(TEXT("Homestead_Manual"), true);
}

void AHomesteadController::ShowArrival()
{
    if (!GEngine || !GEngine->GameViewport) return;
    if (ArrivalCard.IsValid())
        GEngine->GameViewport->RemoveViewportWidgetContent(StaticCastSharedPtr<SWidget>(ArrivalCard).ToSharedRef());
    ArrivalCard = SNew(HomesteadMenus::SHomesteadArrival)
        .Title(FromUtf8(Sim.EstateName()))
        .Subtitle(FString::Printf(TEXT("%s, %d"), UTF8_TO_TCHAR(Sim.SeasonName()), Homestead::Manor::ArrivalYear));
    GEngine->GameViewport->AddViewportWidgetContent(StaticCastSharedPtr<SWidget>(ArrivalCard).ToSharedRef(), 40);
}

void AHomesteadController::UpdateArrival()
{
    if (!ArrivalCard.IsValid() || !ArrivalCard->IsFinished()) return;
    if (GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(StaticCastSharedPtr<SWidget>(ArrivalCard).ToSharedRef());
    ArrivalCard.Reset();
}
