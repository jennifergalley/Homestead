// The dye chooser's live preview (UI/SHomesteadMenu OpenDyeChooser): she wears the garment in the
// hovered dye through the same wardrobe preparation a real recolour uses, on a copy of the
// simulation, so nothing is saved or spent until Apply.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadDye, Log, All);

bool AHomesteadController::MenuPreviewDye(int32 WearableId, int32 Dye)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return false;
    Homestead::Simulation Candidate = Sim;
    const auto* Item = Candidate.GetWearable(WearableId);
    if (!Item) return false;
    // She tries it on: a garment in her pack or the open chest is put on in the copy first, since the
    // wardrobe only dresses her in what she wears.
    const auto Refuse = [WearableId, Dye](const FString& Why)
    {
        UE_LOG(LogHomesteadDye, Warning, TEXT("Dye preview %d on wearable %d refused: %s"), Dye, WearableId, *Why);
        return false;
    };
    if (Item->owner == Homestead::WearableOwner::Chest)
        if (const auto Moved = Candidate.MoveWearable(WearableId, 0, PlayerPoint(), Candidate.GetRevision()); !Moved)
            return Refuse(UTF8_TO_TCHAR(Moved.message.c_str()));
    if (Candidate.GetWearable(WearableId)->owner == Homestead::WearableOwner::Carried)
        if (const auto Worn = Candidate.EquipWearable(WearableId, Candidate.GetRevision()); !Worn)
            return Refuse(UTF8_TO_TCHAR(Worn.message.c_str()));
    Item = Candidate.GetWearable(WearableId);
    if (!Item || Item->owner != Homestead::WearableOwner::Equipped) return Refuse(TEXT("not worn after equipping"));
    if (Item->dye != Dye)
        if (const auto Dyed = Candidate.RecolorWearable(WearableId, Dye, PlayerPoint(), Candidate.GetRevision()); !Dyed)
            return Refuse(UTF8_TO_TCHAR(Dyed.message.c_str()));
    FString Error;
    if (Avatar->PrepareEquipment(Candidate.GetState(), Appearance, Error) && Avatar->ApplyPreparedEquipment(Error)) return true;
    Avatar->ClearPreparedEquipment();
    return Refuse(Error);
}

void AHomesteadController::MenuEndDyePreview()
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    FString Error;
    if (!Avatar->PrepareEquipment(Sim.GetState(), Appearance, Error) || !Avatar->ApplyPreparedEquipment(Error))
    {
        Avatar->ClearPreparedEquipment();
        UE_LOG(LogHomesteadDye, Warning, TEXT("Restoring her clothes after the dye preview failed: %s"), *Error);
    }
}
