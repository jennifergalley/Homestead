#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "HomesteadMapComponent.h"
#include "Simulation/HomesteadManor.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadCrops.h"
#include "UI/SHomesteadMenu.h"
#include "UI/SHomesteadShop.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

using HomesteadControllerText::Text;
using HomesteadControllerText::SleepClockText;

namespace HomesteadControllerBookDetail
{
constexpr int32 FieldBookPages[] = {0, 1, 2, 7, 6}; // As SHomesteadMenuPrivate.h: page 3 (Guidebook) is retired.
constexpr int32 RetiredGuidebookPage = 3;

int32 ShiftFieldBookPage(int32 Page, int32 Direction)
{
    int32 Index = 0;
    for (int32 I = 0; I < UE_ARRAY_COUNT(FieldBookPages); ++I)
        if (FieldBookPages[I] == Page) { Index = I; break; }
    return FieldBookPages[(Index + UE_ARRAY_COUNT(FieldBookPages) + Direction) % UE_ARRAY_COUNT(FieldBookPages)];
}

bool Edible(Homestead::Item Item) { return Homestead::IsEdible(Item); }
const TCHAR* RecipeDescription(Homestead::Recipe Recipe)
{
    switch (Recipe)
    {
    case Homestead::Recipe::HaftAxe: return TEXT("Fit a salvaged axe head to a new handle. Fells trees and clears stumps and fallen timber.");
    case Homestead::Recipe::HaftHoe: return TEXT("Fit a salvaged hoe blade to a new handle, to break and tend garden soil.");
    case Homestead::Recipe::HaftScythe: return TEXT("Fit a salvaged scythe blade to a snath. Mows tall grass and weeds in a wide sweep.");
    case Homestead::Recipe::HaftBillhook: return TEXT("Fit a salvaged billhook head to a handle. Hacks through bramble and saplings.");
    case Homestead::Recipe::HaftPickaxe: return TEXT("Fit a salvaged pick head to a new handle. Breaks rubble and rocks into stone and scrap.");
    case Homestead::Recipe::RoastedRoots: return TEXT("Wild roots softened and warmed over a fueled cookfire.");
    case Homestead::Recipe::HerbedRoots: return TEXT("Roasted roots brightened with meadow herbs.");
    case Homestead::Recipe::SplitFirewood: return TEXT("Prepared fuel split from timber with a carried axe.");
    default: return TEXT("");
    }
}
}

using HomesteadControllerBookDetail::Edible;
using HomesteadControllerBookDetail::RecipeDescription;
using HomesteadControllerBookDetail::ShiftFieldBookPage;
using HomesteadControllerBookDetail::RetiredGuidebookPage;

void AHomesteadController::OpenBook(int32 TargetPage)
{
    EndPlacement();
    HoveredHotbarSlot = INDEX_NONE;
    bBookOpen = true;
    // The Guidebook (page 3) is retired: anything still asking for it gets the pack.
    Page = TargetPage == RetiredGuidebookPage ? 0 : FMath::Clamp(TargetPage, 0, 7);
    Selection = 0;
    bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->CancelSprint();
        Avatar->GetCharacterMovement()->StopMovementImmediately();
        // Appearance turns the camera to face her where she stands, beside its column of choices.
        Avatar->SetAppearancePreview(Page == 6);
    }
    ShowNativeMenu();
}

void AHomesteadController::Withdraw()
{
    if (IsFailed()) return;
    if (bPlanning) { ToggleDeconstruct(); return; }
    if (!bBookOpen) { OpenBook(0); return; }
    if (Page != 0) return;
    const auto Items = Rows();
    if (!Items.IsValidIndex(Selection)) return;
    const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
    if (ChestId < 0) { Notify(TEXT("Stand near a storage chest to take an item."), true); return; }
    Notify(Sim.Transfer(ChestId, static_cast<Homestead::Item>(Items[Selection].Id), -1, PlayerPoint()));
    Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
}

void AHomesteadController::CloseBook()
{
    if (bBookOpen) PlayEffect(UIClick, 0.08f);
    bBookOpen = false;
    ActiveChestId.Reset();
    MenuInventoryViewIndex = 0;
    bConfirmRestart = false;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(false);
    if (IsFailed()) ShowNativeMenu();
    else HideNativeMenu();
    // New-game setup: leaving Appearance moves on to the Names step.
    if (bNewGameSetup && !IsFailed() && !NamesWidget.IsValid()) ShowNames();
}

void AHomesteadController::ToggleBook() { if (IsFailed()) return; if (bBookOpen) CloseBook(); else OpenBook(0); }

void AHomesteadController::OpenSettings() { if (bBookOpen && Page == 4) CloseBook(); else OpenBook(4); }

void AHomesteadController::OpenCraft() { if (!IsFailed()) OpenBook(1); }

void AHomesteadController::OpenBuild() { if (!IsFailed()) OpenBook(2); }

void AHomesteadController::OpenMap() { if (!IsFailed()) OpenBook(7); }

void AHomesteadController::Back()
{
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) CloseBook();
    else if (bPlanning) EndPlacement();
    else if (CancelShopWait()) PlayEffect(UIClick, 0.05f);
    else OpenBook(4);
}

void AHomesteadController::PreviousPage()
{
    if (bPlanning) { RotatePlacementBy(-1); return; }
    if (!bBookOpen) { CycleHotbar(-1); return; }
    Page = ShiftFieldBookPage(Page, -1); Selection = 0; bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(Page == 6);
}

void AHomesteadController::NextPage()
{
    if (bPlanning) { RotatePlacement(); return; }
    if (!bBookOpen) { CycleHotbar(1); return; }
    Page = ShiftFieldBookPage(Page, 1); Selection = 0; bConfirmRestart = false;
    PlayEffect(UIClick, 0.08f);
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetAppearancePreview(Page == 6);
}

void AHomesteadController::PreviousRow()
{
    if (!bBookOpen) { if (!CycleBedChoice(-1)) CycleSeedPouch(-1); return; }
    const int Count = Rows().Num();
    if (Count) Selection = (Selection + Count - 1) % Count;
    PlayEffect(UIClick, 0.06f);
    bConfirmRestart = false;
}

void AHomesteadController::NextRow()
{
    if (!bBookOpen) { if (!CycleBedChoice(1)) CycleSeedPouch(1); return; }
    const int Count = Rows().Num();
    if (Count) Selection = (Selection + 1) % Count;
    PlayEffect(UIClick, 0.06f);
    bConfirmRestart = false;
}

TArray<FHomesteadRow> AHomesteadController::Rows() const
{
    TArray<FHomesteadRow> Result;
    if (Page == 0)
    {
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        const Homestead::Structure* Chest = nullptr;
        for (const auto& Structure : State().structures) if (Structure.id == ChestId) Chest = &Structure;
        for (int Index = 0; Index < Homestead::ItemCount; ++Index)
        {
            const auto Item = static_cast<Homestead::Item>(Index);
            const int InPack = Sim.Count(Item);
            const int InChest = Chest ? Chest->storage[Index] : 0;
            if (!InPack && !InChest) continue;
            const FString Action = InPack && Edible(Item) ? TEXT("eat 1")
                : InChest ? TEXT("take 1") : TEXT("");
            const FString Detail = !InPack ? TEXT("Stored nearby, not carried. Take one into your pack.")
                : Edible(Item) ? TEXT("Food - eat one from your pack.")
                : TEXT("Used in the world or in recipes.");
            const FString Label = Chest
                ? FString::Printf(TEXT("%s  |  Carried: %d  |  Chest: %d"), *Text(Homestead::ItemName(Item)), InPack, InChest)
                : FString::Printf(TEXT("%s  |  Carried: %d"), *Text(Homestead::ItemName(Item)), InPack);
            Result.Add({ Index, Label, Detail, Action, Chest && InPack > 0, InChest > 0 });
        }
    }
    else if (Page == 1)
    {
        for (int Index = 0; Index < static_cast<int>(Homestead::Recipe::Count); ++Index)
        {
            const auto Recipe = static_cast<Homestead::Recipe>(Index);
            const auto Assessment = Sim.AssessRecipe(Recipe, PlayerPoint());
            FHomesteadRow Row;
            Row.Id = Index;
            Row.SubjectId = Index;
            Row.Subject = EHomesteadMenuSubject::Recipe;
            Row.Name = Row.Label = Text(Homestead::RecipeName(Recipe));
            Row.Location = FString::Printf(TEXT("Makes %d %s"), Assessment.outputCount,
                *Text(Homestead::ItemName(Assessment.output)));
            Row.Detail = RecipeDescription(Recipe);
            Row.IconTint = Assessment.craftable
                ? FLinearColor(0.92f, 0.74f, 0.43f)
                : FLinearColor(0.34f, 0.36f, 0.34f);
            Row.RecipeState = Assessment;
            Row.HasRecipeState = true;
            Result.Add(MoveTemp(Row));
        }
    }
    else if (Page == 2)
    {
        for (int Index = 0; Index < static_cast<int>(Homestead::Piece::Count); ++Index)
        {
            const auto Piece = static_cast<Homestead::Piece>(Index);
            if (!Homestead::IsBuildable(Piece)) continue;
            // Activating a plan closes the book and starts a placement preview (BeginPlacement): she
            // aims it in the world and confirms there, and only then are the materials spent.
            Result.Add({ Index, Text(Homestead::PieceName(Piece)),
                FString::Printf(TEXT("Needs: %s\nChoose a spot in the world, then place it. Nothing is spent until you place it."),
                    *Text(Homestead::PieceRequirements(Piece))), TEXT("Choose a spot to build") });
        }
        FHomesteadRow TakeDown{ static_cast<int>(Homestead::Piece::Count), TEXT("Take down"),
            TEXT("Aim at anything you built and take it apart for its full cost. A chest's contents come with it; "
                 "a floor must be bare first. In build mode Y / X switches between building and taking down."),
            TEXT("Choose what to take down") };
        TakeDown.Icon = FName(TEXT("hatchet"));
        Result.Add(MoveTemp(TakeDown));
    }

    else if (Page == 4)
    {
        Result.Add({0, TEXT("Save"), TEXT("Write a manual save and remain in Settings.")});
        Result.Add({1, TEXT("Load latest save"), LatestSaveLabel.IsEmpty()
            ? FString(TEXT("Resume the newest valid manual or automatic save."))
            : FString::Printf(TEXT("Resume the newest valid manual or automatic save: %s."), *LatestSaveLabel)});
        const FString Speed = State().dayMinutes >= 119 ? TEXT("Leisurely") : State().dayMinutes <= 31 ? TEXT("Fast") : TEXT("Balanced");
        Result.Add({2, TEXT("Game speed: ") + Speed, TEXT("Leisurely, Balanced, or Fast.")});
        Result.Add({3, FString::Printf(TEXT("Camera sensitivity: %.1f"), Sensitivity), TEXT("Cycle a comfortable turn speed.")});
        Result.Add({4, FString::Printf(TEXT("Invert camera Y: %s"), bInvertY ? TEXT("On") : TEXT("Off")), TEXT("Change vertical look direction.")});
        Result.Add({16, FString::Printf(TEXT("Overall volume: %d%%"), FMath::RoundToInt(MasterVolume * 100)), TEXT("Scales every sound in the game.")});
        Result.Add({5, FString::Printf(TEXT("Music volume: %d%%"), FMath::RoundToInt(MusicVolume * 100)), TEXT("Music playback level.")});
        Result.Add({6, FString::Printf(TEXT("Ambience volume: %d%%"), FMath::RoundToInt(AmbienceVolume * 100)), TEXT("Wind, woodland and creek ambience.")});
        Result.Add({7, FString::Printf(TEXT("Effects volume: %d%%"), FMath::RoundToInt(EffectsVolume * 100)), TEXT("Footsteps, gathering, crafting, and interface sounds.")});
        Result.Add({8, TEXT("Start a new woodland"), TEXT("Create a new seed after confirmation. Cancel keeps your current woodland. This build uses a new test-save version.")});
        Result.Add({9, TEXT("Quit game"), TEXT("Choose Save & Quit or Quit without Saving.")});
        if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
        {
            float Normalized = 0, Scale = 100, Minimum = 0, Maximum = 100;
            Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
            Result.Add({10, FString::Printf(TEXT("3D resolution scale: %.0f%%"), Scale),
                TEXT("Cycle 100 / 85 / 70 percent. UI stays sharp; TSR upscales the scene.")});
            const auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
            const bool Requested = Settings->IsVSyncEnabled();
            FString Label = FString::Printf(TEXT("Vertical sync: %s"), Requested ? TEXT("On") : TEXT("Off"));
            if (!VSync) Label += TEXT(" (unavailable)");
            else if ((VSync->GetInt() != 0) != Requested)
                Label += FString::Printf(TEXT(" | active %s (override)"), VSync->GetInt() ? TEXT("On") : TEXT("Off"));
            else if ((VSync->GetFlags() & ECVF_SetByMask) > ECVF_SetByGameSetting)
                Label += TEXT(" (engine override)");
            Result.Add({11, Label, TEXT("May reduce tearing, but can add input delay. Does not fix every flicker.")});
        }
        Result.Add({12, FString::Printf(TEXT("Autosave: %s"), bAutosaveEnabled ? TEXT("On") : TEXT("Off")),
            TEXT("Periodic rotating saves. Recovery checkpoints remain separate.")});
        Result.Add({13, FString::Printf(TEXT("Autosave interval: %d minutes"), AutosaveMinutes),
            bAutosaveEnabled ? TEXT("Counts only unpaused gameplay time.") : TEXT("Stored interval; Autosave is Off.")});
        if (Map)
            Result.Add({17, FString::Printf(TEXT("Minimap: %s"), Map->RotatesWithCamera() ? TEXT("turns with your view") : TEXT("north up")),
                TEXT("North up keeps the map still; turning with your view keeps ahead at the top, and the N marker shows north.")});
        Result.Add({15, TEXT("Show action hints again"),
            FString::Printf(TEXT("Each floating action hint retires after you've done that action %d times. This brings them all back."), HintRetireUses)});
        if (!PreviewLabel().IsEmpty())
            Result.Add({14, PreviewLabel(), TEXT("This preview uses isolated saves.")});
    }
    else if (Page == 7)
    {
        Result.Add({0, TEXT("Map"), TEXT("The estate and the country around it.")});
    }
    else if (Page == 6)
    {
        Result.Add({0, FString::Printf(TEXT("Hair: %s"), HomesteadLook::MetaHairName(Appearance.MetaHair)), TEXT("MetaHuman hairstyles: long, bobbed, tied back, braided or cropped.")});
        Result.Add({1, FString::Printf(TEXT("Hair color: %s"), HomesteadLook::HairColorName(Appearance.HairColor)), TEXT("Chestnut, dark brown, black, copper, or blonde. Hair color is independent of hairstyle.")});
        Result.Add({2, FString::Printf(TEXT("Skin: %s"), HomesteadLook::SkinToneName(Appearance.SkinTone)),         TEXT("Natural, warm, deep or light. Her face and body change together.")});
                Result.Add({3, FString::Printf(TEXT("Eyes: %s"), HomesteadLook::EyeColorName(Appearance.EyeColor)), TEXT("Blue, green, hazel or grey. The view moves close to her face while you choose.")});
        Result.Add({4, FString::Printf(TEXT("Tunic dye: %s"), HomesteadLook::TunicColorName(Appearance.TunicColor)), TEXT("A color choice for the current original outfit.")});
        Result.Add({5, FString::Printf(TEXT("Outfit: %s"), HomesteadLook::OutfitName(Appearance.Outfit)), TEXT("Cosmetic linen choices.")});
    }
    else
    {
        Result.Add({0, TEXT("Evening Fall (Harp) by Kevin MacLeod (incompetech.com)"), TEXT("Creative Commons Attribution 4.0: https://creativecommons.org/licenses/by/4.0/")});
        Result.Add({1, TEXT("Public-domain (CC0) music, with thanks"), TEXT("Whispers of the Glen and Medieval Theme by Maarten Schellekens; A New Town by cynicmusic (pixelsphere.org)")});
        Result.Add({2, TEXT("Music playback"), TEXT("Converted for game playback; playback fades and level matching applied.")});
        Result.Add({3, TEXT("Forest and creek ambience"), TEXT("TinyWorlds - OpenGameArt - CC0; creek: SamsterBirdies - Freesound - CC0; hearth: PagDev - OpenGameArt - CC0")});
        Result.Add({4, TEXT("Brown Mud Leaves 01"), TEXT("Rob Tuytel - Poly Haven - CC0")});
        Result.Add({5, TEXT("Rock Moss Set 02"), TEXT("Kless Gyzen - Poly Haven - CC0")});
        Result.Add({6, TEXT("Complete credits"), TEXT("See docs/asset-credits.md in the project or packaged build.")});
        Result.Add({7, TEXT("Interaction and footstep sounds"), TEXT("Kenney - Impact Sounds and Interface Sounds - CC0")});
        Result.Add({8, TEXT("Character foundation"), TEXT("MakeHuman Community / MPFB graphical assets - CC0; original outfit and motion.")});
        Result.Add({9, TEXT("Estate terrain"), TEXT("Reshaped from Environment Agency LIDAR. Contains Environment Agency information \u00A9 Environment Agency and/or database right 2022, licensed under the Open Government Licence v3.0.")});
    }
    return Result;
}

FString AHomesteadController::BookTitle() const
{
    switch (Page)
    {
    case 0: return ActiveChestId.IsSet() ? TEXT("Storage") : TEXT("Your pack");
    case 1: return TEXT("Crafting recipes");
    case 2: return TEXT("Building plans");
    default: return TEXT("Field book");
    }
}

FString AHomesteadController::BookSummary() const
{
    switch (Page)
    {
    case 0: return ActiveChestId.IsSet()
        ? TEXT("Move whole stacks between this chest and your pack.")
        : TEXT("Carried items and equipped clothing.");
    case 1: return FString();
    case 2: return TEXT("Choose a plan to start placing it. Materials are spent when you place it.");
    case 6: return bGamepad ? TEXT("D-pad Left / Right: change the highlighted choice. She changes as you choose.")
        : TEXT("Click a swatch or style to wear it. She changes as you choose.");

    default: return {};
    }
}

FString AHomesteadController::BookFooter() const
{
    if (Page == 5)
        return bGamepad ? TEXT("D-pad: scroll   LB / RB: pages   B: close")
            : TEXT("Up / Down: scroll   Left / Right: pages   Esc: close");
    if (Page == 4 && Rows().IsValidIndex(Selection) && Rows()[Selection].Id == 11)
        return bGamepad ? TEXT("D-pad: select   LB / RB: pages   A: toggle   B: close")
            : TEXT("Up / Down: select   Left / Right: pages   Enter: toggle   Esc: close");
    if (Page > 2)
        return bGamepad ? TEXT("D-pad: select   LB / RB: pages   A: use   B: close")
            : TEXT("Up / Down: select   Left / Right: pages   Enter: use   Esc: close");
    const auto Items = Rows();
    FString Footer = Items.IsEmpty()
        ? (bGamepad ? TEXT("LB/RB: pages") : TEXT("Left / Right: pages"))
        : (bGamepad ? TEXT("D-pad: select   LB/RB: pages") : TEXT("Arrows: select/pages"));
    if (Items.IsValidIndex(Selection))
    {
        const auto& Row = Items[Selection];
        if (!Row.Action.IsEmpty())
            Footer += FString::Printf(TEXT("   %s: %s"), bGamepad ? TEXT("A") : TEXT("Enter"), *Row.Action);
        if (Row.CanStore) Footer += bGamepad ? TEXT("   X: store 1") : TEXT("   F: store 1");
        if (Row.CanTake)
            Footer += bGamepad ? TEXT("   Y: take 1") : TEXT("   G: take 1");
    }
    Footer += bGamepad ? TEXT("   B: close") : TEXT("   Esc: close");
    return Footer;
}

void AHomesteadController::MenuStepAppearance(int32 Id, int32 Direction)
{
    if (!bBookOpen || Page != 6 || bMenuSaveInProgress) return;
    if (IsFailed()) { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    const int32 Count = AppearanceChoiceCount(Id);
    if (Count <= 0) return;
    const int32 Current = AppearanceChoice(Id);
    MenuSetAppearance(Id, ((Current + (Direction < 0 ? -1 : 1)) % Count + Count) % Count);
}

int32 AHomesteadController::AppearanceChoiceCount(int32 Id)
{
    switch (Id)
    {
    case 0: return HomesteadLook::MetaHairCount;
    case 1: return HomesteadLook::HairColorCount;
    case 2: case 3: case 4: return 4;
    case 5: return 2;
    default: return 0;
    }
}

int32 AHomesteadController::AppearanceChoice(int32 Id) const
{
    switch (Id)
    {
    case 0: return Appearance.MetaHair;
    case 1: return Appearance.HairColor;
    case 2: return Appearance.SkinTone;
    case 3: return Appearance.EyeColor;
    case 4: return Appearance.TunicColor;
    case 5: return Appearance.Outfit;
    default: return 0;
    }
}

void AHomesteadController::MenuFocusAppearance(int32 Id)
{
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        Avatar->SetAppearanceFaceFocus(bBookOpen && Page == 6 && Id == 3);
}

void AHomesteadController::MenuSetAppearance(int32 Id, int32 Value)
{
    if (!bBookOpen || Page != 6 || bMenuSaveInProgress) return;
    if (IsFailed()) { Notify(TEXT("Retry a checkpoint before changing possessions or appearance."), true); return; }
    const int32 Count = AppearanceChoiceCount(Id);
    if (Count <= 0 || Value < 0 || Value >= Count) return;
    MenuFocusAppearance(Id);
    if (AppearanceChoice(Id) == Value) return;
    FHomesteadAppearance Next = Appearance;
    switch (Id)
    {
    case 0: Next.MetaHair = Value; Next.HairStyle = HomesteadLook::LegacyHairStyle(Value); break;
    case 1: Next.HairColor = Value; break;
    case 2: Next.SkinTone = Value; break;
    case 3: Next.EyeColor = Value; break;
    case 4: Next.TunicColor = Value; break;
    case 5: Next.Outfit = Value; break;
    default: return;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    FString AppearanceError;
    const bool Applied = Avatar && (Avatar->IsEquipmentPresentationReady()
        ? Avatar->PrepareEquipment(State(), Next, AppearanceError) && Avatar->ApplyPreparedEquipment(AppearanceError)
        : Avatar->ApplyAppearance(Next));
    if (!Applied)
    {
        Notify(TEXT("That appearance could not be applied. Your saved selection has not changed. ") + AppearanceError, true);
        return;
    }
    Appearance = Next;
    PlayEffect(UIClick, 0.08f);
}

std::vector<Homestead::SleepOption> AHomesteadController::BedSleepOptions() const
{
    return Homestead::SleepOptions(State().hour, State().energy);
}

int32 AHomesteadController::BedSleepIndex() const
{
    if (Focus != EFocus::Bed || FocusId != BedChoiceBed) return 0;
    const auto Options = BedSleepOptions();
    for (int32 Index = 0; Index < static_cast<int32>(Options.size()); ++Index)
        if (Options[Index].choice == BedChoice) return Index;
    return 0;
}

double AHomesteadController::BedSleepHours() const
{
    const auto Options = BedSleepOptions();
    return Options.empty() ? 0.0 : Options[BedSleepIndex()].hours;
}

FString AHomesteadController::SleepOptionLabel(const Homestead::SleepOption& Option)
{
    switch (Option.choice)
    {
    case Homestead::SleepChoice::UntilMorning: return TEXT("Sleep until morning (wake ") + SleepClockText(Option.wakeHour) + TEXT(")");
    case Homestead::SleepChoice::UntilRested: return TEXT("Sleep until rested (wake ~") + SleepClockText(Option.wakeHour) + TEXT(")");
    default: return FString::Printf(TEXT("Nap %g h (wake "), Option.hours) + SleepClockText(Option.wakeHour) + TEXT(")");
    }
}

bool AHomesteadController::CycleBedChoice(int32 Delta)
{
    if (bBookOpen || bPlanning || IsFailed() || Focus != EFocus::Bed) return false;
    const auto Options = BedSleepOptions();
    if (Options.size() < 2) return true;
    const int32 Count = static_cast<int32>(Options.size());
    const int32 Index = (BedSleepIndex() + Delta % Count + Count) % Count;
    BedChoice = Options[Index].choice;
    BedChoiceBed = FocusId;
    PlayEffect(UIClick, 0.06f);
    return true;
}

void AHomesteadController::SleepAtBed(Homestead::Point Position)
{
    Notify(SleepInBed(Position));
    if (!IsFailed())
    {
        if (bAutosaveEnabled && SaveSlot(FString::Printf(TEXT("Homestead_Auto_%d"), AutoSaveIndex), true))
            AutoSaveIndex = (AutoSaveIndex + 1) % 3;
        // The woodland's recovery checkpoint wants her fed; the estate has no hunger, so no gate there.
        if (Sim.IsSheltered(Position) && (State().fixedEstate || State().hunger >= 35))
            SaveSlot(TEXT("Homestead_Recovery"), true);
    }
}

void AHomesteadController::HomesteadSleep(int32 Option)
{
    if (bBookOpen || bPlanning || IsFailed()) return;
    UpdateFocus();
    if (Focus != EFocus::Bed) { Notify(TEXT("Stand beside a bed to sleep."), true); return; }
    const auto Options = BedSleepOptions();
    if (Option >= 0 && Option < static_cast<int32>(Options.size())) { BedChoice = Options[Option].choice; BedChoiceBed = FocusId; }
    SleepAtBed(PlayerPoint());
}

void AHomesteadController::HomesteadBedChoice(int32 Delta)
{
    UpdateFocus();
    CycleBedChoice(Delta);
}

Homestead::Result AHomesteadController::SleepInBed(Homestead::Point Position)
{
    const auto Options = BedSleepOptions();
    if (Options.empty()) return {false, "There's nothing to sleep on here.", Homestead::ResultCode::Unavailable, Sim.GetRevision()};
    const Homestead::SleepOption Option = Options[BedSleepIndex()];
    const double Hours = Option.hours;
    // Sleep takes at most twelve hours at a time; an early night from 18:00 is two halves.
    Homestead::Result Slept = Sim.Sleep(Hours > 12.0 ? Hours * 0.5 : Hours, Position);
    if (Slept && Hours > 12.0) Slept = Sim.Sleep(Hours * 0.5, Position);
    BedChoiceBed = INDEX_NONE;
    if (!Slept) return Slept;
    const FString Now = SleepClockText(State().hour);
    FString Message;
    if (Option.choice == Homestead::SleepChoice::Nap)
        Message = TEXT("You nap for an hour and get up at ") + Now + TEXT(".");
    else if (Option.choice == Homestead::SleepChoice::UntilMorning)
        Message = State().energy >= 99.0
            ? TEXT("You wake at first light, rested. Your garden and fires carried on through the night.")
            : TEXT("You wake at first light, though still a little tired. An earlier night would leave you fully rested.");
    else
        Message = TEXT("You wake rested at ") + Now + TEXT(". Your garden and fires carried on while you slept.");
    Slept.message = TCHAR_TO_UTF8(*Message);
    return Slept;
}

void AHomesteadController::ActivateRow()
{
    const auto Items = Rows();
    if (!Items.IsValidIndex(Selection)) return;
    const int Id = Items[Selection].Id;
    if (Page == 0)
    {
        const auto Item = static_cast<Homestead::Item>(Id);
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        if (Sim.Count(Item) == 0 && ChestId >= 0) Notify(Sim.Transfer(ChestId, Item, -1, PlayerPoint()));
        else if (Edible(Item)) Notify(Sim.Eat(Item));
        else if (ChestId >= 0)
        {
            bool Stored = false;
            for (const auto& Structure : State().structures)
                if (Structure.id == ChestId && Structure.storage[Id] > 0) Stored = true;
            if (Stored) Notify(Sim.Transfer(ChestId, Item, -1, PlayerPoint()));
            else Notify(TEXT("This tool or material is used in the world or in recipes."));
        }
        else Notify(TEXT("This tool or material is used in the world or in recipes."));
        Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
    }
    else if (Page == 1)
    {
        const auto Result = Sim.Craft(static_cast<Homestead::Recipe>(Id), PlayerPoint());
        NotifyResourceAction(Result, WoodTapB);
        if (Result.ok)
        {
            Sim.AdvanceGameHours(0.05, PlayerPoint());
            SlotHaftedTool(static_cast<Homestead::Recipe>(Id));
        }
    }
    else if (Page == 2) BeginPlacement(static_cast<Homestead::Piece>(Id));
    else if (Page == 6) MenuStepAppearance(Id, 1);
    else if (Page == 4)
    {
        switch (Id)
        {
        case 0: MenuSave(); break;
        case 1: QuickLoad(); break;
        case 2: Notify(Sim.SetDayMinutes(State().dayMinutes < 60 ? 60 : State().dayMinutes < 120 ? 120 : 30)); break;
        case 3: PersistCameraSensitivity(Sensitivity >= 1.8f ? 0.6f : Sensitivity + 0.2f); break;
        case 4: PersistCameraInversion(!bInvertY); break;
        case 5:
            PersistAudioVolume(5, MusicVolume >= 0.99f ? 0 : MusicVolume + 0.2f, MusicVolume);
            break;
        case 6:
            PersistAudioVolume(6, AmbienceVolume >= 0.99f ? 0 : AmbienceVolume + 0.2f, AmbienceVolume);
            break;
        case 7: PersistAudioVolume(7, EffectsVolume >= 0.99f ? 0 : EffectsVolume + 0.2f, EffectsVolume); break;
        case 16: PersistAudioVolume(16, MasterVolume >= 0.99f ? 0 : MasterVolume + 0.2f, MasterVolume); break;
        case 8: if (bConfirmRestart) NewGame(); else bConfirmRestart = true; break;
        case 9:
            MenuRequestExit();
            break;
        case 10:
            if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
            {
                float Normalized = 0, Scale = 100, Minimum = 0, Maximum = 100;
                Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
                PersistResolutionScale(Scale > 99 ? 85 : Scale > 84 ? 70 : 100);
            }
            else Notify(TEXT("Video settings are unavailable in this session."), true);
            break;
        case 11: ToggleVerticalSync(); break;
        case 12: MenuSetAutosaveEnabled(!bAutosaveEnabled); break;
        case 13: MenuSetAutosaveInterval(AutosaveMinutes == 5 ? 10 : AutosaveMinutes == 10 ? 20 : AutosaveMinutes == 20 ? 30 : 5); break;
        case 15: ResetActionHints(); break;
        case 17: if (Map) Map->SetRotatesWithCamera(!Map->RotatesWithCamera()); break;
        default: break;
        }
    }
}
