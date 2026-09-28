#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadHUD.h"
#include "InputCoreTypes.h"

namespace
{
int32 Stored(const AHomesteadController& PC, Homestead::Item Item)
{
    const int32 Id = PC.Simulation().FindNearestStructure(PC.PlayerPoint(), Homestead::Piece::Chest, 280);
    for (const auto& Structure : PC.State().structures)
        if (Structure.id == Id) return Structure.storage[static_cast<int32>(Item)];
    return 0;
}

bool PackCountsMatch(const AHomesteadController& PC)
{
    const bool Chest = PC.Simulation().FindNearestStructure(PC.PlayerPoint(), Homestead::Piece::Chest, 280) >= 0;
    const auto Rows = PC.Rows();
    int32 RowIndex = 0;
    for (int32 Id = 0; Id < Homestead::ItemCount; ++Id)
    {
        const auto Item = static_cast<Homestead::Item>(Id);
        const int32 Count = PC.Simulation().Count(Item), InChest = Stored(PC, Item);
        if (!Count && !InChest) continue;
        const FString Label = Chest
            ? FString::Printf(TEXT("%s  |  Carried: %d  |  Chest: %d"), UTF8_TO_TCHAR(Homestead::ItemName(Item)), Count, InChest)
            : FString::Printf(TEXT("%s  |  Carried: %d"), UTF8_TO_TCHAR(Homestead::ItemName(Item)), Count);
        if (!Rows.IsValidIndex(RowIndex) || Rows[RowIndex].Id != Id || Rows[RowIndex].Label != Label
            || Rows[RowIndex].CanStore != (Chest && Count > 0) || Rows[RowIndex].CanTake != (InChest > 0)) return false;
        ++RowIndex;
    }
    return PC.BookPage() == 0 && Rows.Num() == RowIndex;
}
}

void AHomesteadSmokeTest::QueueBookCapture(const FString& Name)
{
    Add(TEXT("Let existing transient toast clear without advancing the paused world"),
        []() {}, [this]() { return Controller->Toast().IsEmpty(); }, 8.2f);
    Steps.Last().Skip = [this]() { return Controller->Toast().IsEmpty(); };
    Add(TEXT("Capture native book and measured text bounds: ") + Name,
        [this, Name]() { Screenshot(Name); },
        [this]()
        {
            const auto* HUD = Controller->GetHUD<AHomesteadHUD>();
            return HUD && HUD->BookTextFits(Controller->BookPage());
        });
}

void AHomesteadSmokeTest::PrepareBookClarityChecks()
{
    const auto Before = MakeShared<std::string>();
    const auto Expected = MakeShared<Homestead::Simulation>();
    Add(TEXT("Close initial Notes"), [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    QueueGrant(Homestead::Item::Billhook, 1);
    Add(TEXT("Open actual carried inventory with controller"), [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]()
        {
            return PackCountsMatch(*Controller) && Controller->Simulation().Count(Homestead::Item::Billhook) == 1
                && Controller->BookTitle() == TEXT("Your pack") && Controller->Rows()[0].Action.IsEmpty()
                && !Controller->BookFooter().Contains(TEXT("A:")) && !Controller->BookFooter().Contains(TEXT("X:"));
        });
    QueueBookCapture(TEXT("book-pack-tool"));
    Add(TEXT("Carried billhook advertises no pack action and pressing A changes no simulation state"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Open recipes with no crafting supplies"), [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]()
        {
            const auto Rows = Controller->Rows();
            if (Controller->BookPage() != 1 || Rows.Num() != static_cast<int32>(Homestead::Recipe::Count)
                || Controller->BookTitle() != TEXT("Crafting recipes") || !Controller->BookFooter().Contains(TEXT("A: craft"))) return false;
            for (int32 Id = 0; Id < Rows.Num(); ++Id)
            {
                const auto Recipe = static_cast<Homestead::Recipe>(Id);
                if (Rows[Id].Id != Id || Rows[Id].Label != UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))
                    || Rows[Id].Detail != FString(TEXT("Needs: ")) + UTF8_TO_TCHAR(Homestead::RecipeRequirements(Recipe))) return false;
            }
            return true;
        });
    Add(TEXT("Unaffordable controller craft rejects without hiding requirements or changing state"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->ToastIsError() && Controller->Simulation().Serialize() == *Before
            && Controller->Rows()[0].Detail.Contains(UTF8_TO_TCHAR(Homestead::RecipeRequirements(Homestead::Recipe::HaftAxe))); });
    QueueBookCapture(TEXT("book-recipes-missing"));
    Add(TEXT("Keyboard craft has the same rejection and a truthful Enter: craft hint"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Enter); },
        [this, Before]() { return Controller->ToastIsError() && Controller->Simulation().Serialize() == *Before
            && !Controller->UsesGamepad() && Controller->BookFooter().Contains(TEXT("Enter: craft")); });
    QueueBookCapture(TEXT("book-recipes-keyboard"));
    Add(TEXT("Open building plans"), [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]()
        {
            const auto Rows = Controller->Rows();
            // Every buildable piece in enum order; the hearth belongs to the old house.
            if (Controller->BookPage() != 2 || Rows.Num() != static_cast<int32>(Homestead::Piece::Hearth)
                || Controller->BookTitle() != TEXT("Building plans") || !Controller->BookFooter().Contains(TEXT("A: plan"))) return false;
            for (int32 Id = 0; Id < Rows.Num(); ++Id)
            {
                const auto Piece = static_cast<Homestead::Piece>(Id);
                if (Rows[Id].Id != Id || Rows[Id].Label != UTF8_TO_TCHAR(Homestead::PieceName(Piece))
                    || Rows[Id].Detail != FString(TEXT("Needs: ")) + UTF8_TO_TCHAR(Homestead::PieceRequirements(Piece))) return false;
            }
            return true;
        });
    QueueBookCapture(TEXT("book-plans"));
    Add(TEXT("Planning selects a preview, not a possession or material purchase"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->IsPlanning() && Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Cancel planning"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning(); });
    Add(TEXT("Keyboard reopens the same building plan page"), [this]() { Tap(EKeys::B); },
        [this]() { return Controller->BookPage() == 2 && Controller->SelectedRow() == 0
            && Controller->BookFooter().Contains(TEXT("Enter: plan")); });
    QueueBookCapture(TEXT("book-plans-keyboard"));
    Add(TEXT("Menus pause the simulation and preserve the camera"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); CameraStart = Controller->GetControlRotation().Yaw; },
        [this, Before]() { return Controller->Simulation().Serialize() == *Before
            && Controller->GetControlRotation().Yaw == CameraStart; }, 1.0f);
    Add(TEXT("Close plans"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    QueueGatherTo(Homestead::Item::Berries, 5);
    QueueGatherTo(Homestead::Item::Branch, 5);
    Add(TEXT("Open partial inventory"), [this]() { Tap(EKeys::I); },
        [this]() { return Controller->BookPage() == 0 && Controller->Simulation().Count(Homestead::Item::Berries) >= 5; });
    QueueSelectRow(static_cast<int32>(Homestead::Item::Berries));
    Add(TEXT("Partial inventory counts reflect possessions and selected food offers eating"),
        []() {}, [this]() { return PackCountsMatch(*Controller) && Controller->BookFooter().Contains(TEXT("A: eat 1")); });
    QueueBookCapture(TEXT("book-pack-food"));
    for (const FKey Key : {EKeys::Gamepad_FaceButton_Bottom, EKeys::Enter})
        Add(TEXT("Eat exactly one carried berry through ") + Key.ToString(),
            [this, Key]() { BerriesBeforeFood = Controller->Simulation().Count(Homestead::Item::Berries); Tap(Key); },
            [this]() { return Controller->Simulation().Count(Homestead::Item::Berries) == BerriesBeforeFood - 1
                && PackCountsMatch(*Controller); });
    Add(TEXT("Keyboard changes row without changing page"), [this]() { Tap(EKeys::Up); },
        [this]() { return Controller->BookPage() == 0 && Controller->Rows()[Controller->SelectedRow()].Id == static_cast<int32>(Homestead::Item::Branch); });
    Add(TEXT("Controller selects the same next food row"), [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this]() { return Controller->BookPage() == 0 && Controller->Rows()[Controller->SelectedRow()].Id == static_cast<int32>(Homestead::Item::Berries); });
    Add(TEXT("Close inventory for recipe supplies"), [this]() { Tap(EKeys::Escape); }, []() { return true; });
    QueueGatherTo(Homestead::Item::Stone, 4);
    QueueGrant(Homestead::Item::RustedAxeHead, 1);
    Add(TEXT("Open affordable axe hafting recipe"), [this]() { Tap(EKeys::C); },
        [this]() { return Controller->BookPage() == 1 && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0; });
    QueueBookCapture(TEXT("book-recipes-supplied"));
    Add(TEXT("Affordable recipe performs exactly the existing transaction and time cost"),
        [this, Expected]()
        {
            *Expected = Controller->Simulation();
            Expected->Craft(Homestead::Recipe::HaftAxe, Controller->PlayerPoint());
            Expected->AdvanceGameHours(0.05, Controller->PlayerPoint());
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected]() { return !Controller->ToastIsError() && Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
            && Controller->Simulation().Serialize() == Expected->Serialize(); });
    Add(TEXT("Close recipes for disclosed functional storage setup"), [this]() { Tap(EKeys::Escape); },
        [this]() { return !Controller->IsBookOpen(); });
    QueueGatherTo(Homestead::Item::Branch, 10);
    QueueGrant(Homestead::Item::BrambleCanes, 4);
    QueueClearCell(-4, -3);
    QueuePlace(Homestead::Piece::Chest, -4, -3);
    Add(TEXT("Approach the newly crafted test chest; fixture teleport, not ordinary footage"),
        [this]() { const auto P = Homestead::CellCenter(-4, -3); Teleport({P.x, P.y - 140}); },
        [this]() { return Controller->Simulation().FindNearestStructure(Controller->PlayerPoint(), Homestead::Piece::Chest, 280) >= 0; }, 0.65f);
    Add(TEXT("Open inventory beside chest"), [this]() { Tap(EKeys::I); }, [this]() { return PackCountsMatch(*Controller); });
}

void AHomesteadSmokeTest::PrepareBookStorageChecks()
{
    for (int32 Id = 0; Id < Homestead::ItemCount; ++Id)
    {
        const auto Item = static_cast<Homestead::Item>(Id);
        const int32 Count = Controller->Simulation().Count(Item);
        if (!Count) continue;
        QueueSelectRow(Id);
        for (int32 Index = 0; Index < Count; ++Index)
            Add(FString::Printf(TEXT("Store %s %d/%d through keyboard F"), UTF8_TO_TCHAR(Homestead::ItemName(Item)), Index + 1, Count),
                [this]() { Tap(EKeys::F); },
                [this, Item, Count, Index]() { return Controller->Simulation().Count(Item) == Count - Index - 1
                    && Stored(*Controller, Item) == Index + 1 && PackCountsMatch(*Controller); });
    }
    QueueSelectRow(static_cast<int32>(Homestead::Item::Berries));
    Add(TEXT("Zero carried count is not confused with nearby chest ownership"), []() {},
        [this]() { return Controller->Simulation().UsedCapacity() == 0 && PackCountsMatch(*Controller)
            && Controller->Rows()[Controller->SelectedRow()].Action == TEXT("take 1")
            && !Controller->Rows()[Controller->SelectedRow()].CanStore; });
    QueueBookCapture(TEXT("book-storage-only"));
    for (const FKey Key : {EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Top})
        Add(TEXT("Take exactly one stored berry through ") + Key.ToString(),
            [this, Key]() { BerriesBeforeFood = Controller->Simulation().Count(Homestead::Item::Berries); SavedBranches = Stored(*Controller, Homestead::Item::Berries); Tap(Key); },
            [this]() { return Controller->Simulation().Count(Homestead::Item::Berries) == BerriesBeforeFood + 1
                && Stored(*Controller, Homestead::Item::Berries) == SavedBranches - 1 && PackCountsMatch(*Controller)
                && Controller->BookFooter().Contains(TEXT("A: eat 1")) && Controller->BookFooter().Contains(TEXT("X: store 1")); });
    QueueBookCapture(TEXT("book-storage-carried"));
    for (int32 Index = 0; Index < 2; ++Index)
        Add(TEXT("Store carried berries with controller X"), [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
            [this, Index]() { return Controller->Simulation().Count(Homestead::Item::Berries) == 1 - Index && PackCountsMatch(*Controller); });
    for (const FKey Key : {EKeys::Enter, EKeys::G})
        Add(TEXT("Keyboard take preserves separate storage counts: ") + Key.ToString(),
            [this, Key]() { BerriesBeforeFood = Controller->Simulation().Count(Homestead::Item::Berries); SavedBranches = Stored(*Controller, Homestead::Item::Berries); Tap(Key); },
            [this]() { return Controller->Simulation().Count(Homestead::Item::Berries) == BerriesBeforeFood + 1
                && Stored(*Controller, Homestead::Item::Berries) == SavedBranches - 1 && PackCountsMatch(*Controller); });
    for (int32 Index = 0; Index < 2; ++Index)
        Add(TEXT("Return berries to chest through keyboard F"), [this]() { Tap(EKeys::F); },
            [this, Index]() { return Controller->Simulation().Count(Homestead::Item::Berries) == 1 - Index && PackCountsMatch(*Controller); });
    Add(TEXT("Close pack before moving away from storage"), [this]() { Tap(EKeys::Escape); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Move functional fixture outside storage reach"), [this]() { Teleport({-2200, -1500}); },
        [this]() { return Controller->Simulation().FindNearestStructure(Controller->PlayerPoint(), Homestead::Piece::Chest, 280) < 0; }, 0.65f);
    Add(TEXT("Actually empty pack has no owned rows or misleading primary action"), [this]() { Tap(EKeys::I); },
        [this]() { return Controller->IsBookOpen() && Controller->Rows().IsEmpty() && Controller->Simulation().UsedCapacity() == 0
            && !Controller->BookFooter().Contains(TEXT("Enter:")) && PackCountsMatch(*Controller); });
    QueueBookCapture(TEXT("book-pack-empty"));
}
