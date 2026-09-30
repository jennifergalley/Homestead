#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "Simulation/HomesteadEstate.h"
#include "UI/SHomesteadMenu.h"

#include "Misc/CommandLine.h"

using HomesteadControllerText::Text;

void AHomesteadController::HomesteadPackMenu(int32 Tile, int32 Mode)
{
    if (Mode == 2)
    {
        const auto Position = PlayerPoint();
        const Homestead::Structure* Nearest = nullptr;
        float Best = TNumericLimits<float>::Max();
        for (const auto& Structure : State().structures)
        {
            if (Structure.kind != Homestead::Piece::Chest) continue;
            const auto Center = Homestead::StructureCenter(State(), Structure);
            const float Distance = FMath::Square(Center.x - Position.x) + FMath::Square(Center.y - Position.y);
            if (Distance < Best) { Best = Distance; Nearest = &Structure; }
        }
        if (!Nearest) { Notify(TEXT("There is no storage chest nearby."), true); return; }
        const auto ChestCenter = Homestead::StructureCenter(State(), *Nearest);
        UE_LOG(LogTemp, Display, TEXT("HomesteadPackMenu: nearest chest %d at (%.0f, %.0f)"), Nearest->id, ChestCenter.x, ChestCenter.y);
        if (bBookOpen) CloseBook();
        OpenChestStorage(Nearest->id);
        return;
    }
    if (!NativeMenu.IsValid() || !bBookOpen || Page != 0) { Notify(TEXT("Open the pack first."), true); return; }
    const auto Rows = MenuRows();
    if (!Rows.IsValidIndex(Tile)) { Notify(TEXT("There is no item in that tile."), true); return; }
    if (Mode == 1) NativeMenu->OpenQuantityPrompt(Rows[Tile]);
    else NativeMenu->OpenItemContextMenu(Tile, false);
}

void AHomesteadController::HomesteadMorning(float Hour)
{
    Sim.SkipToHourOfDay(Hour);
    RefreshRemaining = 0;
}

void AHomesteadController::HomesteadGrowCrops(float Days, int32 Tend)
{
    Notify(Sim.PassDaysForPlaytest(Days, Tend != 0, PlayerPoint()));
    RefreshRemaining = 0;
}

void AHomesteadController::HomesteadCropGrowth(float Growth)
{
    Notify(Sim.SetCropGrowthForPlaytest(Growth));
    RefreshRemaining = 0;
}

void AHomesteadController::HomesteadEnergy(float Energy)
{
    Notify(Sim.SetEnergy(Energy));
    RefreshRemaining = 0;
}

void AHomesteadController::HomesteadStandingRoom()
{
    const APawn* Avatar = GetPawn();
    if (!Avatar) return;
    // She wakes facing the doorway on the room's west side, so the grid heading is her yaw + 90.
    const double RoomYaw = FMath::Fmod(Avatar->GetActorRotation().Yaw + 90.0 + 720.0, 360.0);
    const Homestead::Point Offset = Homestead::RotateYaw({-150.0, 0.0}, RoomYaw);
    const auto Position = PlayerPoint();
    const auto Result = Sim.SeedStandingRoomAt({Position.x - Offset.x, Position.y - Offset.y}, RoomYaw);
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result);
    RefreshRemaining = 0;
    if (Result) ShowArrival();
}

void AHomesteadController::HomesteadEmptyPail()
{
    Notify(Sim.EmptyPail());
}

void AHomesteadController::HomesteadGive(const FString& ItemName, int32 Amount)
{
    const FString Wanted = ItemName.Replace(TEXT(" "), TEXT(""));
    for (int32 Index = 0; Index < static_cast<int32>(Homestead::Item::Count); ++Index)
    {
        const auto Item = static_cast<Homestead::Item>(Index);
        if (!FString(UTF8_TO_TCHAR(Homestead::ItemName(Item))).Replace(TEXT(" "), TEXT("")).Equals(Wanted, ESearchCase::IgnoreCase))
            continue;
        const auto Result = Sim.GrantItems(Item, Amount);
        Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result);
        return;
    }
    Notify(FString::Printf(TEXT("No item called %s."), *ItemName), true);
}

void AHomesteadController::HomesteadWear(const FString& Garment)
{
    const auto Key = [](FString Text) { return Text.Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT("")); };
    for (int32 Index = 0; Index < static_cast<int32>(Homestead::WearableDefinition::Count); ++Index)
    {
        const auto Definition = static_cast<Homestead::WearableDefinition>(Index);
        const auto* Info = Homestead::GetWearableDefinition(Definition);
        if (!Info || (!Key(UTF8_TO_TCHAR(Info->key)).Equals(Key(Garment), ESearchCase::IgnoreCase)
            && !Key(UTF8_TO_TCHAR(Info->name)).Equals(Key(Garment), ESearchCase::IgnoreCase)))
            continue;
        if (Info->fiberCost <= 0 && Info->furCost <= 0) { Notify(TEXT("That garment cannot be made."), true); return; }
        if (Sim.Count(Homestead::Item::Knife) == 0) Sim.GrantItems(Homestead::Item::Knife, 1);
        if (Info->fiberCost > 0) Sim.GrantItems(Homestead::Item::Fiber, Info->fiberCost);
        if (Info->furCost > 0) Sim.GrantItems(Homestead::Item::Fur, Info->furCost);
        FHomesteadRow Recipe;
        Recipe.Subject = EHomesteadMenuSubject::GarmentRecipe;
        Recipe.Id = Recipe.SubjectId = Index;
        if (!MenuItemAction(Recipe, EHomesteadItemAction::Equip, 1, Sim.GetRevision())) return;
        int32 Made = 0;
        for (const auto& Item : State().wearables)
            if (Item.definition == Definition && Item.owner == Homestead::WearableOwner::Carried) Made = FMath::Max(Made, Item.id);
        FHomesteadRow Worn;
        if (Made && MenuWearableRow(Made, Worn)) MenuItemAction(Worn, EHomesteadItemAction::Equip, 1, Sim.GetRevision());
        return;
    }
    Notify(FString::Printf(TEXT("No garment called %s."), *Garment), true);
}
