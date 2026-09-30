#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "UI/HomesteadMenuPortrait.h"
#include "UI/SHomesteadMenu.h"
#include "UI/SHomesteadNames.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

using HomesteadControllerText::Text;

void AHomesteadController::RefreshMenuPortrait()
{
    // Appearance shows her in the world itself; only the pack page has a portrait.
    if (!bBookOpen || Page != 0)
    {
        if (MenuPortrait) MenuPortrait->Destroy();
        MenuPortrait = nullptr;
        PortraitBrush.SetResourceObject(nullptr);
        return;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    if (!MenuPortrait)
    {
        FActorSpawnParameters Parameters;
        Parameters.ObjectFlags |= RF_Transient;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MenuPortrait = GetWorld()->SpawnActor<AHomesteadMenuPortrait>(FVector(0, 0, -20000), FRotator::ZeroRotator, Parameters);
    }
    if (MenuPortrait && MenuPortrait->Refresh(*Avatar))
    {
        PortraitBrush.SetResourceObject(MenuPortrait->BrushResource());
        PortraitBrush.ImageSize = FVector2D(384, 768);
        PortraitBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    else
    {
        if (MenuPortrait) MenuPortrait->Destroy();
        MenuPortrait = nullptr;
        PortraitBrush.SetResourceObject(nullptr);
        UE_LOG(LogTemp, Warning, TEXT("Menu character preview is unavailable."));
    }
}

void AHomesteadController::OrbitMenuPortrait(float Degrees)
{
    if (MenuPortrait) MenuPortrait->Orbit(Degrees);
}

void AHomesteadController::ZoomMenuPortrait()
{
    if (MenuPortrait) MenuPortrait->ToggleCloseup();
}

FString AHomesteadController::MenuPortraitStatus() const
{
    return TEXT("As you look now");
}

void AHomesteadController::MenuPage(int32 TargetPage)
{
    if (!bMenuSaveInProgress) OpenBook(TargetPage);
}

void AHomesteadController::MenuSelect(int32 Row)
{
    Selection = FMath::Clamp(Row, 0, FMath::Max(0, Rows().Num() - 1));
    if (bBookOpen && Page == 6)
    {
        const auto Items = Rows();
        MenuFocusAppearance(Items.IsValidIndex(Selection) ? Items[Selection].Id : -1);
    }
}

void AHomesteadController::MenuActivate()
{
    if (bMenuSaveInProgress || !bBookOpen) return;
    if (bTestResetRequired && (Page != 4 || !Rows().IsValidIndex(Selection)
        || (Rows()[Selection].Id != 1 && Rows()[Selection].Id != 8 && Rows()[Selection].Id != 9)))
    { Notify(LoadProblem, true); return; }
    if (IsFailed() && Page != 4 && Page != 3 && Page != 5)
    { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    if (IsFailed() && Page == 4 && Rows().IsValidIndex(Selection) && Rows()[Selection].Id == 0)
    { Notify(TEXT("A failed state cannot replace your checkpoint. Retry or quit without saving."), true); return; }
    ActivateRow();
}

bool AHomesteadController::MenuCraftRecipe(Homestead::Recipe Recipe)
{
    if (bMenuSaveInProgress || !bBookOpen || Page != 1 || IsFailed() || bTestResetRequired)
        return false;
    const auto Assessment = Sim.AssessRecipe(Recipe, PlayerPoint());
    if (!Assessment.craftable)
    {
        Notify(Text(Assessment.blocker.c_str()), true);
        return false;
    }
    const auto Result = Sim.Craft(Recipe, PlayerPoint());
    // What she made shows as the "+1 Hatchet" pickup beside her; only a refusal needs words.
    NotifyResourceAction(Result, nullptr);
    if (Result.ok)
    {
        Sim.AdvanceGameHours(0.05, PlayerPoint());
        SlotHaftedTool(Recipe);
    }
    return Result.ok;
}

void AHomesteadController::SlotHaftedTool(Homestead::Recipe Recipe)
{
    Homestead::Item Tool = Homestead::Item::Count;
    switch (Recipe)
    {
    case Homestead::Recipe::HaftAxe: Tool = Homestead::Item::Hatchet; break;
    case Homestead::Recipe::HaftHoe: Tool = Homestead::Item::DiggingStick; break;
    case Homestead::Recipe::HaftScythe: Tool = Homestead::Item::Scythe; break;
    case Homestead::Recipe::HaftBillhook: Tool = Homestead::Item::Billhook; break;
    case Homestead::Recipe::HaftPickaxe: Tool = Homestead::Item::Pickaxe; break;
    default: return;
    }
    // A newly hafted tool goes straight to hand: it arrived in the first empty hotbar cell (the row
    // is the first row of her pack), so select that cell; with the row full it waits below.
    const int32 Slot = HotbarCellOf(Tool);
    if (Slot == INDEX_NONE) return;
    SelectedHotbarSlot = Slot;
}

float AHomesteadController::CraftProgress() const
{
    return bBookOpen && NativeMenu.IsValid() ? NativeMenu->GetCraftProgress() : 0.0f;
}

void AHomesteadController::MenuCraftBeat(int32 Beat)
{
    USoundBase* Strikes[] = {CraftStrikeA.Get(), CraftStrikeB.Get(), CraftStrikeC.Get()};
    USoundBase* Strike = Strikes[FMath::Abs(Beat) % UE_ARRAY_COUNT(Strikes)];
    ++TestCraftBeatRequests;
    if (Strike && bAudioEnabled && EffectsVolume > 0) ++TestAudibleCraftBeats;
    PlayEffect(Strike, 0.16f);
}

void AHomesteadController::MenuStore() { if (!bMenuSaveInProgress) Secondary(); }

void AHomesteadController::MenuTake() { if (!bMenuSaveInProgress) Withdraw(); }

void AHomesteadController::MenuBack()
{
    if (bTestResetRequired) { Notify(LoadProblem, true); return; }
    if (!bMenuSaveInProgress) CloseBook();
}

void AHomesteadController::MenuRetry()
{
    if (bMenuSaveInProgress) return;
    RetryCheckpoint();
    if (!IsFailed()) CloseBook();
}

void AHomesteadController::MenuRequestExit() { if (NativeMenu.IsValid()) NativeMenu->RequestExit(); }

void AHomesteadController::MenuSave() { if (!bMenuSaveInProgress) QuickSave(); }

FString AHomesteadController::MenuSaveStatus() const
{
    const FString When = LastSuccessfulSave.GetTicks() > 0
        ? LastSuccessfulSave.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")) : TEXT("not known in this session");
    const FString Label = CurrentSaveLabel();
    return FString::Printf(TEXT("%s\nLast successful save: %s"),
        !Label.IsEmpty() ? *Label : PreviewLabel().IsEmpty() ? TEXT("Current homestead") : *PreviewLabel(), *When);
}

void AHomesteadController::MenuSaveAndQuit()
{
    if (bMenuSaveInProgress) return;
    if (IsFailed())
    {
        if (NativeMenu.IsValid()) NativeMenu->ShowSaveFailure(TEXT("A failed state cannot replace your checkpoint. Retry or quit without saving."));
        return;
    }
    bMenuSaveInProgress = true;
    const bool Saved = SaveSlot(TEXT("Homestead_Manual"));
    bMenuSaveInProgress = false;
    if (Saved)
    {
        if (PendingResolutionScale.IsSet() && !PersistResolutionScale(PendingResolutionScale.GetValue()))
        {
            if (NativeMenu.IsValid()) NativeMenu->ShowGraphicsSaveFailure(GraphicsSaveError);
            return;
        }
        UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
    }
    else if (NativeMenu.IsValid()) NativeMenu->ShowSaveFailure(ToastText);
}

void AHomesteadController::MenuQuitWithoutSaving()
{
    if (!bMenuSaveInProgress) UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AHomesteadController::MenuRestart() { if (!bMenuSaveInProgress) NewGame(); }
