#include "HomesteadSmokeTest.h"

#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "UI/SHomesteadMenu.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include <algorithm>
#include <set>

namespace
{
const Homestead::Plot* FindPlot(const Homestead::State& State, int32 Id)
{
    for (const auto& Plot : State.plots)
        if (Plot.id == Id) return &Plot;
    return nullptr;
}

const Homestead::Structure* FindPiece(const Homestead::State& State, Homestead::Piece Kind, int32 X, int32 Y)
{
    for (const auto& Piece : State.structures)
        if (Piece.kind == Kind && Piece.cellX == X && Piece.cellY == Y) return &Piece;
    return nullptr;
}

bool IsCleared(const Homestead::State& State, int32 Id)
{
    for (const auto& Node : State.resources)
        if (Node.id == Id) return Node.cleared;
    return false;
}

Homestead::ResourceKind SourceFor(Homestead::Item Item)
{
    switch (Item)
    {
    case Homestead::Item::Branch: return Homestead::ResourceKind::Branches;
    case Homestead::Item::Stone: return Homestead::ResourceKind::Stones;
    case Homestead::Item::Fiber: return Homestead::ResourceKind::Reeds;
    case Homestead::Item::Roots:
    case Homestead::Item::Seeds: return Homestead::ResourceKind::Roots;
    case Homestead::Item::Flowers: return Homestead::ResourceKind::Flowers;
    case Homestead::Item::Berries: return Homestead::ResourceKind::BerryBush;
    default: return Homestead::ResourceKind::Count;
    }
}

Homestead::Generation::EntityKind GeneratedSourceFor(Homestead::ResourceKind Kind)
{
    using EntityKind = Homestead::Generation::EntityKind;
    switch (Kind)
    {
    case Homestead::ResourceKind::Branches: return EntityKind::Branches;
    case Homestead::ResourceKind::Stones: return EntityKind::Stones;
    case Homestead::ResourceKind::BerryBush: return EntityKind::BerryBush;
    case Homestead::ResourceKind::Roots: return EntityKind::Roots;
    case Homestead::ResourceKind::Flowers: return EntityKind::Flowers;
    case Homestead::ResourceKind::Reeds: return EntityKind::Reeds;
    case Homestead::ResourceKind::Sapling: return EntityKind::Sapling;
    case Homestead::ResourceKind::ForestTree: return EntityKind::ForestTree;
    default: return EntityKind::Branches;
    }
}

Homestead::Item CraftedItem(Homestead::Recipe Recipe)
{
    switch (Recipe)
    {
    case Homestead::Recipe::Hatchet: return Homestead::Item::Hatchet;
    case Homestead::Recipe::DiggingStick: return Homestead::Item::DiggingStick;
    case Homestead::Recipe::WateringCan: return Homestead::Item::WateringCan;
    case Homestead::Recipe::RoastedRoots: return Homestead::Item::RoastedRoots;
    case Homestead::Recipe::HerbedRoots: return Homestead::Item::HerbedRoots;
    case Homestead::Recipe::SplitFirewood: return Homestead::Item::Firewood;
    default: return Homestead::Item::Count;
    }
}

int32 CraftedQuantity(Homestead::Recipe Recipe)
{
    return Recipe == Homestead::Recipe::SplitFirewood ? 4 : 1;
}

bool SameAppearance(const FHomesteadAppearance& A, const FHomesteadAppearance& B)
{
    return A.HairStyle == B.HairStyle && A.HairColor == B.HairColor
        && A.SkinTone == B.SkinTone && A.EyeColor == B.EyeColor && A.TunicColor == B.TunicColor
        && A.Outfit == B.Outfit && A.BodyPreset == B.BodyPreset;
}
}

void AHomesteadSmokeTest::QueueSelectRow(int32 Id)
{
    Add(FString::Printf(TEXT("Navigate the field book to row ID %d"), Id),
        []() {},
        [this, Id]()
        {
            if (Controller->HasNativeMenu())
            {
                const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
                return Controller->IsBookOpen() && Subject && Subject->Id == Id
                    && Subject->Subject != EHomesteadMenuSubject::GarmentRecipe
                    && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                    && Controller->NativeMenu->HasSynchronizedFocus();
            }
            const auto Rows = Controller->Rows();
            return Controller->IsBookOpen() && Rows.IsValidIndex(Controller->SelectedRow())
                && Rows[Controller->SelectedRow()].Id == Id;
        }, 8.0f);
    Steps.Last().NavigateToId = Id;
}

void AHomesteadSmokeTest::QueueGatherTo(Homestead::Item Item, int32 TargetCount)
{
    const auto Kind = SourceFor(Item);
    struct Candidate
    {
        Homestead::Generation::GeneratedEntityKey key;
        Homestead::Point position;
    };
    TArray<Candidate> Candidates;
    std::set<Homestead::Generation::GeneratedEntityKey> Seen;
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind == Kind && Seen.insert(Node.key).second)
            Candidates.Add({Node.key, Node.position});
    }
    const auto Center = Controller->State().activeChunk;
    for (int32 Y = Center.y - 2; Y <= Center.y + 2; ++Y)
        for (int32 X = Center.x - 2; X <= Center.x + 2; ++X)
        {
            Homestead::Generation::ChunkBaseline Baseline;
            if (Homestead::Generation::GenerateChunk(Controller->State().world, {X, Y}, Baseline)
                != Homestead::Generation::Status::Ok) continue;
            for (const auto& Entity : Baseline.entities)
                if (Entity.kind == GeneratedSourceFor(Kind) && Seen.insert(Entity.key).second)
                    Candidates.Add({Entity.key, {static_cast<double>(Entity.xCm), static_cast<double>(Entity.yCm)}});
        }
    for (const auto& Candidate : Candidates)
    {
        const auto Position = Candidate.position;
        const auto Key = Candidate.key;
        const auto CurrentId = MakeShared<int32>(0);
        const auto Before = MakeShared<int32>(0);
        auto Skip = [this, Item, TargetCount, Key]()
        {
            if (Controller->Simulation().Count(Item) >= TargetCount) return true;
            Homestead::ResourceNode Current;
            const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Key, Current);
            return Resolved.code == Homestead::ResultCode::Unavailable
                || (Resolved && Current.id > 0 && !Controller->Simulation().CanHarvest(Current.id));
        };
        Add(FString::Printf(TEXT("Approach full-loop forage key %d:%d:%u"), Key.chunk.x, Key.chunk.y, Key.localId),
            [this, Position, Key, CurrentId, Before, Item]()
            {
                *Before = Controller->Simulation().Count(Item);
                Teleport(Position);
                Homestead::ResourceNode Current;
                const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Key, Current);
                if (!Resolved || Current.id <= 0)
                {
                    Finish(false, TEXT("Stable generated forage key did not resolve to a current active handle."));
                    return;
                }
                *CurrentId = Current.id;
                if (FMath::Abs(Current.position.x - Position.x) > 0.01
                    || FMath::Abs(Current.position.y - Position.y) > 0.01)
                    Teleport(Current.position);
            },
            [this, CurrentId]()
            {
                return !Controller->IsBookOpen() && !Controller->IsPlanning()
                    && Controller->IsResourceFocused(*CurrentId);
            }, 0.65f);
        Steps.Last().Skip = Skip;
        Add(FString::Printf(TEXT("Gather stable key toward %d %s"), TargetCount, UTF8_TO_TCHAR(Homestead::ItemName(Item))),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this, Before, Item, CurrentId]()
            {
                return !Controller->ToastIsError() && Controller->Simulation().Count(Item) > *Before
                    && !Controller->Simulation().CanHarvest(*CurrentId)
                    && Controller->Simulation().UsedCapacity() <= Homestead::InventoryCapacity;
            });
        Steps.Last().Skip = Skip;
    }
    Add(FString::Printf(TEXT("Foraging supplied at least %d %s"), TargetCount, UTF8_TO_TCHAR(Homestead::ItemName(Item))),
        []() {},
        [this, Item, TargetCount]() { return Controller->Simulation().Count(Item) >= TargetCount; });
}

void AHomesteadSmokeTest::QueueCraft(Homestead::Recipe Recipe)
{
    const auto Item = CraftedItem(Recipe);
    const int32 Quantity = CraftedQuantity(Recipe);
    const auto Before = MakeShared<int32>(0);
    Add(FString::Printf(TEXT("Open crafting for %s"), UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))),
        [this]() { Tap(EKeys::C); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 1; });
    QueueSelectRow(static_cast<int32>(Recipe));
    Add(TEXT("Enter the selected recipe's native actions"),
        [this]() { if (Controller->HasNativeMenu()) Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return !Controller->HasNativeMenu()
            || Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
    Add(FString::Printf(TEXT("Craft %s using gamepad A"), UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))),
        [this, Before, Item]()
        {
            *Before = Controller->Simulation().Count(Item);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before, Item, Quantity]()
        {
            return !Controller->ToastIsError() && Controller->Simulation().Count(Item) == *Before + Quantity;
        });
    Add(TEXT("Close the crafting book"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
}

void AHomesteadSmokeTest::QueuePlace(Homestead::Piece Kind, int32 CellX, int32 CellY, int32 Rotation)
{
    const auto Center = Homestead::CellCenter(CellX, CellY);
    const auto Before = MakeShared<int32>(0);
    Add(FString::Printf(TEXT("Approach %s site (%d,%d)"), UTF8_TO_TCHAR(Homestead::PieceName(Kind)), CellX, CellY),
        [this, Center]()
        {
            Teleport({Center.x - 350, Center.y});
            Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-15, 0, 0));
        },
        [this]() { return !Controller->IsBookOpen() && !Controller->IsPlanning(); }, 0.65f);
    Add(TEXT("Open the build page for the next cabin piece"),
        [this]() { Tap(EKeys::B); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 2; });
    QueueSelectRow(static_cast<int32>(Kind));
    Add(TEXT("Enter the selected building piece's native actions"),
        [this]() { if (Controller->HasNativeMenu()) Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return !Controller->HasNativeMenu()
            || Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
    Add(TEXT("Enter the selected building preview"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsPlanning() && !Controller->IsBookOpen(); });
    for (int32 Turn = 0; Turn < Rotation; ++Turn)
        Add(TEXT("Rotate preview one cardinal edge with the right bumper"),
            [this]() { Tap(EKeys::Gamepad_RightShoulder); },
            [this]() { return Controller->IsPlanning(); }, 0.2f);
    Add(FString::Printf(TEXT("Commit %s at (%d,%d), rotation %d"), UTF8_TO_TCHAR(Homestead::PieceName(Kind)), CellX, CellY, Rotation),
        [this, Before]()
        {
            *Before = static_cast<int32>(Controller->State().structures.size());
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before, Kind, CellX, CellY, Rotation]()
        {
            if (Controller->ToastIsError() || Controller->State().structures.size() != static_cast<size_t>(*Before + 1))
                return false;
            for (const auto& Piece : Controller->State().structures)
                if (Piece.kind == Kind && Piece.cellX == CellX && Piece.cellY == CellY && Piece.rotation == Rotation)
                    return true;
            return false;
        });
    Add(TEXT("Finish placing the cabin piece"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning() && !Controller->IsBookOpen(); });
}

void AHomesteadSmokeTest::QueueClearCell(int32 CellX, int32 CellY)
{
    for (const auto& Node : Controller->State().resources)
    {
        const double Left = static_cast<double>(CellX) * Homestead::CellSize;
        const double Bottom = static_cast<double>(CellY) * Homestead::CellSize;
        const bool Blocks = Node.kind == Homestead::ResourceKind::Sapling
            ? FMath::FloorToInt(Node.position.x / Homestead::CellSize) == CellX
                && FMath::FloorToInt(Node.position.y / Homestead::CellSize) == CellY
            : Node.kind == Homestead::ResourceKind::ForestTree
                && FMath::Square(Node.position.x - FMath::Clamp(Node.position.x, Left, Left + Homestead::CellSize))
                    + FMath::Square(Node.position.y - FMath::Clamp(Node.position.y, Bottom, Bottom + Homestead::CellSize))
                    <= 50.0 * 50.0;
        if (!Blocks) continue;
        const auto Key = Node.key;
        const auto Position = Node.position;
        const auto CurrentId = MakeShared<int32>(Node.id);
        auto Skip = [this, Key]()
        {
            Homestead::ResourceNode Current;
            const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Key, Current);
            return Resolved.code == Homestead::ResultCode::Unavailable || (Resolved && Current.cleared);
        };
        Add(FString::Printf(TEXT("Approach stable obstruction key %d:%d:%u for cell (%d,%d)"),
            Key.chunk.x, Key.chunk.y, Key.localId, CellX, CellY),
            [this, Position, Key, CurrentId]()
            {
                Teleport(Position);
                Homestead::ResourceNode Current;
                const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Key, Current);
                if (!Resolved || Current.id <= 0)
                { Finish(false, TEXT("Stable building obstruction key has no current active handle.")); return; }
                *CurrentId = Current.id;
            },
            [this, CurrentId]() { return !Controller->IsBookOpen() && !Controller->IsPlanning()
                && Controller->IsResourceFocused(*CurrentId); }, 0.65f);
        Steps.Last().Skip = Skip;
        Add(TEXT("Permanently clear stable site obstruction with gamepad X"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
            [this, Key]()
            {
                Homestead::ResourceNode Current;
                const auto Resolved = Controller->Simulation().ResolveGeneratedResource(Key, Current);
                return Resolved && Current.cleared && !Controller->ToastIsError();
            });
        Steps.Last().Skip = Skip;
    }
    Add(FString::Printf(TEXT("Cell (%d,%d) is persistently clear"), CellX, CellY),
        []() {},
        [this, CellX, CellY]()
        {
            for (const auto& Node : Controller->State().resources)
            {
                const double Left = static_cast<double>(CellX) * Homestead::CellSize;
                const double Bottom = static_cast<double>(CellY) * Homestead::CellSize;
                const bool Blocks = Node.kind == Homestead::ResourceKind::Sapling
                    ? FMath::FloorToInt(Node.position.x / Homestead::CellSize) == CellX
                        && FMath::FloorToInt(Node.position.y / Homestead::CellSize) == CellY
                    : Node.kind == Homestead::ResourceKind::ForestTree
                        && FMath::Square(Node.position.x - FMath::Clamp(Node.position.x, Left, Left + Homestead::CellSize))
                            + FMath::Square(Node.position.y - FMath::Clamp(Node.position.y, Bottom, Bottom + Homestead::CellSize))
                            <= 50.0 * 50.0;
                if (!Node.cleared && Blocks) return false;
            }
            return true;
        });
}

void AHomesteadSmokeTest::QueueEat(Homestead::Item Item)
{
    const auto Before = MakeShared<int32>(0);
    const auto Hunger = MakeShared<double>(0);
    Add(FString::Printf(TEXT("Open pack to eat %s"), UTF8_TO_TCHAR(Homestead::ItemName(Item))),
        [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Use the carried inventory view for the selected meal"),
        [this]() { Controller->MenuInventoryView(0); Controller->OpenBook(0); },
        [this]() { return Controller->InventoryView() == 0 && Controller->IsBookOpen(); });
    QueueSelectRow(static_cast<int32>(Item));
    Add(TEXT("Enter the selected food's native actions"),
        [this]() { if (Controller->HasNativeMenu()) Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return !Controller->HasNativeMenu()
            || Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
    Add(FString::Printf(TEXT("Eat %s through the pack menu"), UTF8_TO_TCHAR(Homestead::ItemName(Item))),
        [this, Before, Hunger, Item]()
        {
            *Before = Controller->Simulation().Count(Item);
            *Hunger = Controller->State().hunger;
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before, Hunger, Item]()
        {
            return !Controller->ToastIsError() && Controller->Simulation().Count(Item) == *Before - 1
                && Controller->State().hunger > *Hunger;
        });
    Add(TEXT("Close the pack after the meal"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
}

void AHomesteadSmokeTest::PrepareFullLoop()
{
    const Homestead::Point Home = Homestead::CellCenter(-4, -2);
    const Homestead::Point Fire = Homestead::CellCenter(-3, -2);
    const Homestead::Point Chest = Homestead::CellCenter(-4, -3);
    const Homestead::Point Garden = Homestead::CellCenter(-5, 0);
    const Homestead::Point BerryGarden = Homestead::CellCenter(-3, 0);
    const auto BerryPlotId = MakeShared<int32>(-1);
    const auto Stream = MakeShared<Homestead::Point>(Homestead::Point{Homestead::StreamX(2700) - 40, 2700});
    const auto RevalidateStreamBank = [this, Stream]()
    {
        const double OriginY = Stream->y;
        for (int32 Index = 0; Index < 32; ++Index)
        {
            const double Y = OriginY + (Index % 2 ? -1.0 : 1.0) * ((Index + 1) / 2) * 180.0;
            for (const double Side : {-120.0, 120.0})
            {
                const Homestead::Point Candidate{Homestead::StreamX(Y) + Side, Y};
                Teleport(Candidate);
                bool Occupied = false;
                for (const auto& Node : Controller->State().resources)
                    if (!Node.cleared && Controller->Simulation().CanHarvest(Node.id)
                        && FMath::Square(Node.position.x - Candidate.x)
                            + FMath::Square(Node.position.y - Candidate.y) < 280.0 * 280.0)
                    { Occupied = true; break; }
                if (Occupied) continue;
                *Stream = Candidate;
                Controller->UpdateFocus();
                if (Controller->FocusTitle() == TEXT("Fresh stream water")) return;
            }
        }
        Finish(false, TEXT("No currently unobstructed generated stream-bank point was available."));
    };
    const auto ProtectedRecovery = MakeShared<std::string>();
    const auto RecoveryPosition = MakeShared<Homestead::Point>();

    Add(TEXT("Full MVP starts from the real base-smoke foundation and hatchet"),
        []() {},
        [this]()
        {
            return FindPiece(Controller->State(), Homestead::Piece::Foundation, -4, -2)
                && Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
                && !Controller->IsFailed();
        });
    QueueClearCell(-4, -2);
    QueueClearCell(-3, -2);
    QueueClearCell(-4, -3);
    QueueClearCell(-5, 0);
    QueueClearCell(-3, 0);
    QueueCraft(Homestead::Recipe::DiggingStick);
    QueueCraft(Homestead::Recipe::WateringCan);
    QueuePlace(Homestead::Piece::Chest, -4, -3);
    const auto StoredTimber = MakeShared<int32>(0);
    Add(TEXT("Approach the early storage chest for surplus timber"),
        [this, Chest]() { Teleport(Chest); },
        [this]() { return Controller->FocusTitle() == TEXT("Storage chest"); }, 0.65f);
    Add(TEXT("Store all blocker-clearing Timber through production transfer authority"),
        [this, StoredTimber]()
        {
            const auto* Storage = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            const int32 Carried = Controller->Simulation().Count(Homestead::Item::Timber);
            *StoredTimber = Storage ? Storage->storage[static_cast<int>(Homestead::Item::Timber)] + Carried : -1;
            if (!Storage || Carried <= 0
                || !Controller->Sim.Transfer(Storage->id, Homestead::Item::Timber, Carried, Controller->PlayerPoint()))
                Finish(false, TEXT("Surplus Timber could not be stored through production transfer authority."));
        },
        [this, StoredTimber]()
        {
            const auto* Storage = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            return Storage && Controller->Simulation().Count(Homestead::Item::Timber) == 0
                && Storage->storage[static_cast<int>(Homestead::Item::Timber)] == *StoredTimber;
        });
    QueueGatherTo(Homestead::Item::Branch, 48);
    QueueGatherTo(Homestead::Item::Stone, 8);
    QueueGatherTo(Homestead::Item::Fiber, 20);
    QueueGatherTo(Homestead::Item::Roots, 6);
    QueueGatherTo(Homestead::Item::Flowers, 2);
    QueueGatherTo(Homestead::Item::Berries, 1);

    QueuePlace(Homestead::Piece::Wall, -4, -2, 0);
    Add(TEXT("One wall does not count as shelter"),
        []() {},
        [this, Home]() { return !Controller->Simulation().IsSheltered(Home); });
    QueuePlace(Homestead::Piece::Wall, -4, -2, 1);
    QueuePlace(Homestead::Piece::Wall, -4, -2, 2);
    QueuePlace(Homestead::Piece::Doorway, -4, -2, 3);
    QueuePlace(Homestead::Piece::Roof, -4, -2);
    QueuePlace(Homestead::Piece::Bed, -4, -2);
    QueuePlace(Homestead::Piece::Fire, -3, -2);
    Add(TEXT("Position outside the west doorway"),
        [this, Home]()
        {
            Teleport({Home.x - 310, Home.y});
            Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-12, 0, 0));
        },
        [this]() { return !Controller->Simulation().IsSheltered(Controller->PlayerPoint()); }, 0.65f);
    Add(TEXT("Walk through the cabin doorway"),
        [this]() { MovementStart = Controller->GetPawn()->GetActorLocation(); },
        [this]()
        {
            return FVector::Dist2D(MovementStart, Controller->GetPawn()->GetActorLocation()) > 180
                && Controller->Simulation().IsSheltered(Controller->PlayerPoint());
        }, 1.5f);
    Add(TEXT("Stop moving inside the enclosed cabin"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->Simulation().IsSheltered(Controller->PlayerPoint()); });

    auto PickingStarts = [this]()
    {
        const auto* Avatar = CastChecked<AHomesteadCharacter>(Controller->GetPawn());
        const auto* Animation = CastChecked<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
        return Animation->GatherStarts() + Animation->ClearStarts();
    };
    const auto SecondaryStarts = MakeShared<uint32>(0);
    Add(TEXT("Approach the outdoor cookfire"),
        [this, Fire]() { Teleport(Fire); },
        [this]() { return Controller->FocusTitle() == TEXT("Cookfire"); }, 0.65f);
    for (int32 Branch = 0; Branch < 12; ++Branch)
    {
        const auto FuelBefore = MakeShared<double>(0);
        const auto BranchesBefore = MakeShared<int32>(0);
        Add(FString::Printf(TEXT("Fuel the individual cookfire with branch %d"), Branch + 1),
            [this, FuelBefore, BranchesBefore, SecondaryStarts, PickingStarts]()
            {
                const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
                *FuelBefore = Piece ? Piece->fuelHours : -100;
                *BranchesBefore = Controller->Simulation().Count(Homestead::Item::Branch);
                *SecondaryStarts = PickingStarts();
                Tap(EKeys::Gamepad_FaceButton_Left);
            },
            [this, FuelBefore, BranchesBefore, Fire, SecondaryStarts, PickingStarts]()
            {
                const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
                return Piece && Piece->fuelHours > *FuelBefore + 3.9
                    && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchesBefore - 1
                    && Controller->Simulation().IsNearFire(Fire) && !Controller->ToastIsError()
                    && PickingStarts() == *SecondaryStarts;
            }, 0.2f);
    }
    QueueCraft(Homestead::Recipe::RoastedRoots);
    QueueCraft(Homestead::Recipe::RoastedRoots);
    QueueCraft(Homestead::Recipe::HerbedRoots);
    QueueEat(Homestead::Item::RoastedRoots);

    const auto StoneBefore = MakeShared<int32>(0);
    Add(TEXT("Approach the outdoor storage chest"),
        [this, Chest]() { Teleport(Chest); },
        [this]() { return Controller->FocusTitle() == TEXT("Storage chest"); }, 0.65f);
    Add(TEXT("Open nearby chest using gamepad A"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Deposit two stones through production transfer authority"),
        [this, StoneBefore]()
        {
            *StoneBefore = Controller->Simulation().Count(Homestead::Item::Stone);
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            if (!Piece || !Controller->Sim.Transfer(Piece->id, Homestead::Item::Stone, 2, Controller->PlayerPoint()))
                Finish(false, TEXT("The full-loop stone deposit was rejected by production transfer authority."));
        },
        [this, StoneBefore]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            return Piece && Piece->storage[static_cast<int32>(Homestead::Item::Stone)] == 2
                && Controller->Simulation().Count(Homestead::Item::Stone) == *StoneBefore - 2
                && !Controller->ToastIsError();
        });
    Add(TEXT("Withdraw one stone through production transfer authority"),
        [this]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            if (!Piece || !Controller->Sim.Transfer(Piece->id, Homestead::Item::Stone, -1, Controller->PlayerPoint()))
                Finish(false, TEXT("The full-loop stone withdrawal was rejected by production transfer authority."));
        },
        [this, StoneBefore]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            return Piece && Piece->storage[static_cast<int32>(Homestead::Item::Stone)] == 1
                && Controller->Simulation().Count(Homestead::Item::Stone) == *StoneBefore - 1
                && !Controller->ToastIsError();
        });
    Add(TEXT("Close storage before tending the garden"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });

    Add(TEXT("Stand east of clear soil and face west"),
        [this]()
        {
            Teleport({-1160, 150});
            Controller->GetPawn()->SetActorRotation(FRotator(0, 180, 0));
            Controller->SetControlRotation(FRotator(-20, 180, 0));
        },
        [this]() { return Controller->FocusTitle() == TEXT("Woodland"); }, 0.65f);
    Add(TEXT("Till the garden using gamepad X and the crafted digging stick"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]()
        {
            for (const auto& Plot : Controller->State().plots)
                if (Plot.cellX == -5 && Plot.cellY == 0 && !Plot.planted)
                {
                    GardenPlotId = Plot.id;
                    return !Controller->ToastIsError();
                }
            return false;
        });
    Add(TEXT("Approach the new garden plot"),
        [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusTitle() == TEXT("A little patch of earth"); }, 0.65f);
    const auto SeedsBefore = MakeShared<int32>(0);
    Add(TEXT("Plant the wild-root seeds through gamepad A"),
        [this, SeedsBefore]()
        {
            *SeedsBefore = Controller->Simulation().Count(Homestead::Item::Seeds);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, SeedsBefore]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Roots
                && Controller->Simulation().Count(Homestead::Item::Seeds) == *SeedsBefore - 1
                && !Controller->ToastIsError();
        });
    Add(TEXT("Stand west of the separate berry garden and face east"),
        [this]()
        {
            Teleport({-940, 150});
            Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-20, 0, 0));
        },
        [this]() { return Controller->FocusTitle() == TEXT("Woodland"); }, 0.65f);
    Add(TEXT("Till the second food plot through gamepad X"),
        [this, SecondaryStarts, PickingStarts]() { *SecondaryStarts = PickingStarts(); Tap(EKeys::Gamepad_FaceButton_Left); },
        [this, BerryPlotId, SecondaryStarts, PickingStarts]()
        {
            for (const auto& Plot : Controller->State().plots)
                if (Plot.cellX == -3 && Plot.cellY == 0 && !Plot.planted)
                {
                    *BerryPlotId = Plot.id;
                    return Plot.id != GardenPlotId && !Controller->ToastIsError() && PickingStarts() == *SecondaryStarts;
                }
            return false;
        });
    Add(TEXT("Approach the bare berry garden"),
        [this, BerryGarden]() { Teleport(BerryGarden); },
        [this]() { return Controller->FocusTitle() == TEXT("A little patch of earth"); }, 0.65f);
    const auto FruitBefore = MakeShared<int32>(0);
    const auto BerrySeedStock = MakeShared<int32>(0);
    Add(TEXT("Plant seeds from a foraged berry with gamepad X rather than the root action"),
        [this, FruitBefore, BerrySeedStock, SecondaryStarts, PickingStarts]()
        {
            *FruitBefore = Controller->Simulation().Count(Homestead::Item::Berries);
            *BerrySeedStock = Controller->Simulation().Count(Homestead::Item::Seeds);
            *SecondaryStarts = PickingStarts();
            Tap(EKeys::Gamepad_FaceButton_Left);
        },
        [this, BerryPlotId, FruitBefore, BerrySeedStock, SecondaryStarts, PickingStarts]()
        {
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries
                && Controller->Simulation().Count(Homestead::Item::Berries) == *FruitBefore - 1
                && Controller->Simulation().Count(Homestead::Item::Seeds) == *BerrySeedStock
                && Controller->FocusTitle().StartsWith(TEXT("Berries")) && !Controller->ToastIsError()
                && PickingStarts() == *SecondaryStarts;
        });
    Add(TEXT("Walk to an unobstructed stream bank"),
        [this, Stream]()
        {
            const auto Player = Controller->PlayerPoint();
            double Best = TNumericLimits<double>::Max();
            bool Found = false;
            for (int32 Step = -8; Step <= 8; ++Step)
            {
                const double Y = Player.y + Step * 180.0;
                for (const double Side : {-120.0, 120.0})
                {
                    const Homestead::Point Candidate{Homestead::StreamX(Y) + Side, Y};
                    bool Occupied = false;
                    for (const auto& Node : Controller->State().resources)
                        if (!Node.cleared
                            && FMath::Square(Node.position.x - Candidate.x)
                                + FMath::Square(Node.position.y - Candidate.y) < 300.0 * 300.0)
                        { Occupied = true; break; }
                    if (Occupied || !Homestead::IsNearWater(Candidate)) continue;
                    const double Distance = FMath::Square(Candidate.x - Player.x)
                        + FMath::Square(Candidate.y - Player.y);
                    if (Distance < Best) { Best = Distance; *Stream = Candidate; Found = true; }
                }
            }
            if (!Found) { Finish(false, TEXT("No unobstructed loaded stream-bank focus point was available.")); return; }
            const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Stream->y - Player.y, Stream->x - Player.x));
            Controller->GetPawn()->SetActorRotation(FRotator(0, Yaw, 0));
            Controller->SetControlRotation(FRotator(-12, Yaw, 0));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            return Homestead::IsNearWater(Controller->PlayerPoint())
                && Controller->FocusTitle() == TEXT("Fresh stream water");
        }, 18.0f);
    Steps.Last().Repeat = [this, Stream]()
    {
        const auto Player = Controller->PlayerPoint();
        const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Stream->y - Player.y, Stream->x - Player.x));
        Controller->SetControlRotation(FRotator(-12, Yaw, 0));
        Axis(EKeys::Gamepad_LeftY, 1);
    };
    Add(TEXT("Release mapped stream movement before water interaction"),
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            Controller->FlushPressedKeys();
            if (auto* Avatar = Cast<ACharacter>(Controller->GetPawn()))
                Avatar->GetCharacterMovement()->StopMovementImmediately();
        },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() < 1.0; });
    Add(TEXT("Fill the crafted watering can from the actual stream"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Water) == 6 && !Controller->ToastIsError(); });
    Add(TEXT("Return to the planted garden"),
        [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusTitle().StartsWith(TEXT("Roots")); }, 0.65f);
    Add(TEXT("Water the planted root crop using gamepad A"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->moisture > 0.99 && Controller->Simulation().Count(Homestead::Item::Water) == 5
                && !Controller->ToastIsError();
        });
    Add(TEXT("Approach the planted berry bush"),
        [this, BerryGarden]() { Teleport(BerryGarden); },
        [this]() { return Controller->FocusTitle().StartsWith(TEXT("Berries")); }, 0.65f);
    Add(TEXT("Water the second crop through the same gamepad A action"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, BerryPlotId]()
        {
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            return Plot && Plot->kind == Homestead::CropKind::Berries && Plot->moisture > 0.99
                && Controller->Simulation().Count(Homestead::Item::Water) == 4 && !Controller->ToastIsError();
        });
    Add(TEXT("Return to the stream for a partial-can refill"),
        [RevalidateStreamBank]() { RevalidateStreamBank(); },
        [this]() { return Controller->FocusTitle() == TEXT("Fresh stream water"); }, 0.65f);
    Add(TEXT("Refilling tops the carried water back up to six"),
        [this, RevalidateStreamBank]() { RevalidateStreamBank(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Water) == 6 && !Controller->ToastIsError(); });

    for (int32 Rest = 0; Rest < 6; ++Rest)
    {
        if (Rest == 2) QueueEat(Homestead::Item::RoastedRoots);
        if (Rest == 4) QueueEat(Homestead::Item::HerbedRoots);
        const auto BeforeHour = MakeShared<double>(0);
        const auto BeforeGrowth = MakeShared<double>(0);
        const auto BeforeBerryGrowth = MakeShared<double>(0);
        const auto BeforeFuel = MakeShared<double>(0);
        Add(FString::Printf(TEXT("Return to the sheltered bed for rest %d"), Rest + 1),
            [this, Home]() { Teleport(Home); },
            [this]()
            {
                return Controller->FocusTitle() == TEXT("Bedroll")
                    && Controller->Simulation().IsSheltered(Controller->PlayerPoint());
            }, 0.65f);
        Add(FString::Printf(TEXT("Sleep eight game hours in the cabin, rest %d"), Rest + 1),
            [this, BeforeHour, BeforeGrowth, BeforeBerryGrowth, BeforeFuel, BerryPlotId]()
            {
                *BeforeHour = Controller->State().hour;
                const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
                *BeforeGrowth = Plot ? Plot->growth : -1;
                const auto* BerryPlot = FindPlot(Controller->State(), *BerryPlotId);
                *BeforeBerryGrowth = BerryPlot ? BerryPlot->growth : -1;
                const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
                *BeforeFuel = Piece ? Piece->fuelHours : -1;
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this, BeforeHour, BeforeGrowth, BeforeBerryGrowth, BeforeFuel, BerryPlotId]()
            {
                const auto& State = Controller->State();
                const auto* Plot = FindPlot(State, GardenPlotId);
                const auto* BerryPlot = FindPlot(State, *BerryPlotId);
                const auto* Piece = FindPiece(State, Homestead::Piece::Fire, -3, -2);
                return !Controller->IsFailed() && !Controller->ToastIsError() && Plot && Piece && BerryPlot
                    && State.hour >= *BeforeHour + 7.99 && State.hour < *BeforeHour + 8.1
                    && State.energy > 99 && State.hunger > 40 && State.warmth >= 80
                    && Plot->planted && Plot->growth >= *BeforeGrowth && Plot->weeds > 0.05
                    && BerryPlot->planted && BerryPlot->kind == Homestead::CropKind::Berries
                    && BerryPlot->growth >= *BeforeBerryGrowth && BerryPlot->weeds > 0.05
                    && FMath::Abs(Piece->fuelHours - FMath::Max(0.0, *BeforeFuel - 8.0)) < 0.05;
            }, 0.6f);
        if (Rest == 3)
            Add(TEXT("Day-two rain replenished soil during the overnight growth interval"),
                []() {},
                [this, BeforeHour, BerryPlotId]()
                {
                    const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
                    const auto* BerryPlot = FindPlot(Controller->State(), *BerryPlotId);
                    return Plot && BerryPlot && *BeforeHour < 39 && Controller->State().hour > 39
                        && Plot->moisture > 0.90 && BerryPlot->moisture > 0.90;
                });
        if (Rest == 1)
        {
            Add(TEXT("Cold-night shelter and fire protect the rested heroine"),
                []() {},
                [this]()
                {
                    return Controller->Simulation().IsNight()
                        && Controller->Simulation().IsSheltered(Controller->PlayerPoint())
                        && Controller->Simulation().IsNearFire(Controller->PlayerPoint())
                        && Controller->State().warmth >= 80;
                });
            Add(TEXT("Settle night exposure at the cabin entrance"),
                [this, Home]()
                {
                    Teleport({Home.x - 260, Home.y - 260});
                    Controller->GetPawn()->SetActorRotation(FRotator(0, 35, 0));
                    Controller->SetControlRotation(FRotator(-18, 35, 0));
                },
                [this]() { return Controller->Simulation().IsNight() && !Controller->IsFailed(); }, 15.0f);
            Add(TEXT("Capture the enclosed shelter and fueled fire at night"),
                [this]() { Screenshot(TEXT("shelter-night")); },
                []() { return true; }, 1.0f);
        }
        Add(TEXT("Approach the stream when two garden plots need more carried water"),
            [RevalidateStreamBank]() { RevalidateStreamBank(); },
            [this]() { return Controller->FocusTitle() == TEXT("Fresh stream water"); }, 0.65f);
        Steps.Last().Skip = [this]() { return Controller->Simulation().Count(Homestead::Item::Water) >= 2; };
        Add(TEXT("Refill the watering can while tending both food crops"),
            [this, RevalidateStreamBank]() { RevalidateStreamBank(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this]() { return Controller->Simulation().Count(Homestead::Item::Water) == 6 && !Controller->ToastIsError(); });
        Steps.Last().Skip = [this]() { return Controller->Simulation().Count(Homestead::Item::Water) >= 2; };
        Add(FString::Printf(TEXT("Inspect the living crop after rest %d"), Rest + 1),
            [this, Garden]() { Teleport(Garden); },
            [this]()
            {
                const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
                return Plot && Plot->planted && Plot->growth > 0 && Plot->weeds > 0.05;
            }, 0.65f);
        Add(FString::Printf(TEXT("Remove gradual weeds through gamepad X after rest %d"), Rest + 1),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
            [this]()
            {
                const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
                return Plot && Plot->planted && Plot->weeds < 0.001 && !Controller->ToastIsError();
            });
        const auto WaterBefore = MakeShared<int32>(0);
        Add(FString::Printf(TEXT("Water the still-growing crop after rest %d"), Rest + 1),
            [this, WaterBefore]()
            {
                *WaterBefore = Controller->Simulation().Count(Homestead::Item::Water);
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this, WaterBefore]()
            {
                const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
                return Plot && Plot->planted && Plot->moisture > 0.99
                    && Controller->Simulation().Count(Homestead::Item::Water) == *WaterBefore - 1
                    && !Controller->ToastIsError();
            });
        Steps.Last().Skip = [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && (Plot->growth >= 1 || Plot->moisture >= 1);
        };
        Add(FString::Printf(TEXT("Inspect berry growth and gradual weeds after rest %d"), Rest + 1),
            [this, BerryGarden]() { Teleport(BerryGarden); },
            [this, BerryPlotId]()
            {
                const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
                return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries
                    && Plot->growth > 0 && Plot->weeds > 0.05
                    && Controller->FocusTitle().StartsWith(TEXT("Berries"));
            }, 0.65f);
        Add(FString::Printf(TEXT("Gamepad X weeds the planted berry bush after rest %d"), Rest + 1),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
            [this, BerryPlotId]()
            {
                const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
                return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries
                    && Plot->weeds < 0.001 && !Controller->ToastIsError();
            });
        Add(FString::Printf(TEXT("Water the slower-growing berry bush after rest %d"), Rest + 1),
            [this, WaterBefore]()
            {
                *WaterBefore = Controller->Simulation().Count(Homestead::Item::Water);
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this, BerryPlotId, WaterBefore]()
            {
                const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
                return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries
                    && Plot->moisture > 0.99
                    && Controller->Simulation().Count(Homestead::Item::Water) == *WaterBefore - 1
                    && !Controller->ToastIsError();
            });
        Steps.Last().Skip = [this, BerryPlotId]()
        {
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            return Plot && (Plot->growth >= 1 || Plot->moisture >= 1);
        };
    }

    Add(TEXT("The second planted food matures through the shared tending and weather systems"),
        []() {},
        [this, BerryPlotId]()
        {
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries
                && Plot->growth == 1 && Controller->FocusActions().Contains(TEXT("Harvest"))
                && !Controller->IsFailed();
        });
    Add(TEXT("Frame the mature red-berry garden"),
        [this, BerryGarden]()
        {
            Teleport({BerryGarden.x + 150, BerryGarden.y - 100});
            Controller->GetPawn()->SetActorRotation(FRotator(0, 150, 0));
            Controller->SetControlRotation(FRotator(-30, 150, 0));
        },
        []() { return true; }, 2.0f);
    Add(TEXT("Capture the mature second food crop"),
        [this]() { Screenshot(TEXT("berry-garden")); },
        []() { return true; }, 1.0f);
    Add(TEXT("Approach the mature bush for its first berry harvest"),
        [this, BerryGarden]() { Teleport(BerryGarden); },
        [this]() { return Controller->FocusTitle().StartsWith(TEXT("Berries")) && Controller->FocusActions().Contains(TEXT("Harvest")); }, 0.65f);
    const auto BerryHarvestRoots = MakeShared<int32>(0);
    const auto WaterBefore = MakeShared<int32>(0);
    const auto CropWaterStarts = MakeShared<uint32>(0);
    auto WaterStarts = [this]()
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        return Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance())->WaterStarts();
    };
    Add(TEXT("Harvest six berries through gamepad A while retaining the planted bush"),
        [this, FruitBefore, BerrySeedStock, BerryHarvestRoots, WaterBefore, CropWaterStarts, WaterStarts]()
        {
            *FruitBefore = Controller->Simulation().Count(Homestead::Item::Berries);
            *BerrySeedStock = Controller->Simulation().Count(Homestead::Item::Seeds);
            *BerryHarvestRoots = Controller->Simulation().Count(Homestead::Item::Roots);
            *WaterBefore = Controller->Simulation().Count(Homestead::Item::Water);
            *CropWaterStarts = WaterStarts();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, BerryPlotId, FruitBefore, BerrySeedStock, BerryHarvestRoots, WaterBefore, CropWaterStarts, WaterStarts]()
        {
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            return Plot && Plot->planted && Plot->kind == Homestead::CropKind::Berries && Plot->growth < 0.001
                && Controller->Simulation().Count(Homestead::Item::Berries) == *FruitBefore + 6
                && Controller->Simulation().Count(Homestead::Item::Seeds) == *BerrySeedStock
                && Controller->Simulation().Count(Homestead::Item::Roots) == *BerryHarvestRoots
                && Controller->Simulation().Count(Homestead::Item::Water) == *WaterBefore
                && WaterStarts() == *CropWaterStarts
                && !Controller->ToastIsError();
        });
    Add(TEXT("Return to the cabin to checkpoint actual berry regrowth"),
        [this, Home]() { Teleport(Home); },
        [this]()
        {
            return Controller->FocusTitle() == TEXT("Bedroll")
                && Controller->Simulation().IsSheltered(Controller->PlayerPoint());
        }, 0.65f);
    const auto RegrowthHour = MakeShared<double>(0);
    const auto RegrowthBefore = MakeShared<double>(0);
    Add(TEXT("Sheltered sleep regrows the harvested bush and protects a mixed-crop checkpoint"),
        [this, RegrowthHour, RegrowthBefore, BerryPlotId]()
        {
            *RegrowthHour = Controller->State().hour;
            const auto* Plot = FindPlot(Controller->State(), *BerryPlotId);
            *RegrowthBefore = Plot ? Plot->growth : -1;
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_Special_Right);
        },
        [this, RegrowthHour, RegrowthBefore, BerryPlotId, ProtectedRecovery, RecoveryPosition]()
        {
            const auto& State = Controller->State();
            const auto* Plot = FindPlot(State, *BerryPlotId);
            if (Controller->IsFailed() || Controller->ToastIsError() || !Controller->IsBookOpen()
                || !Plot || !Plot->planted || Plot->kind != Homestead::CropKind::Berries
                || Plot->growth <= *RegrowthBefore || Plot->growth >= 1
                || FMath::Abs(State.hour - *RegrowthHour - 8.0) > 0.01
                || State.hunger <= 40 || State.warmth < 80 || State.energy <= 99) return false;
            // Input dispatch has completed and the pack pauses time at the saved sleep checkpoint.
            *ProtectedRecovery = Controller->Simulation().Serialize();
            *RecoveryPosition = Controller->PlayerPoint();
            return true;
        }, 0.6f);
    Add(TEXT("Close the protected mixed-crop checkpoint before continuing"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Naturally grown roots are mature without changing simulation rules"),
        []() {},
        [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->planted && Plot->growth == 1 && !Controller->IsFailed();
        });
    Add(TEXT("Frame the mature garden after the full tending cycle"),
        [this, Garden]()
        {
            Teleport({Garden.x + 150, Garden.y - 100});
            Controller->GetPawn()->SetActorRotation(FRotator(0, 150, 0));
            Controller->SetControlRotation(FRotator(-30, 150, 0));
        },
        []() { return true; }, 3.0f);
    Add(TEXT("Capture the playable mature garden"),
        [this]() { Screenshot(TEXT("garden")); },
        []() { return true; }, 1.0f);
    Add(TEXT("Stand beside the mature roots for harvest"),
        [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusActions().Contains(TEXT("Harvest")); }, 0.65f);
    const auto RootsBefore = MakeShared<int32>(0);
    Add(TEXT("Harvest mature roots and renewable seeds through gamepad A"),
        [this, RootsBefore, SeedsBefore, WaterBefore, CropWaterStarts, WaterStarts]()
        {
            *RootsBefore = Controller->Simulation().Count(Homestead::Item::Roots);
            *SeedsBefore = Controller->Simulation().Count(Homestead::Item::Seeds);
            *WaterBefore = Controller->Simulation().Count(Homestead::Item::Water);
            *CropWaterStarts = WaterStarts();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, RootsBefore, SeedsBefore, WaterBefore, CropWaterStarts, WaterStarts]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && !Plot->planted && Plot->growth == 0
                && Controller->Simulation().Count(Homestead::Item::Roots) == *RootsBefore + 4
                && Controller->Simulation().Count(Homestead::Item::Seeds) == *SeedsBefore + 2
                && Controller->Simulation().Count(Homestead::Item::Water) == *WaterBefore
                && WaterStarts() == *CropWaterStarts
                && !Controller->ToastIsError();
        });

    Add(TEXT("Return to the spent fire before saving the completed homestead"),
        [this, Fire]() { Teleport(Fire); },
        [this]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
            return Piece && Piece->fuelHours == 0 && Controller->FocusTitle() == TEXT("Cookfire");
        }, 0.65f);
    Add(TEXT("Relight the exhausted fire so persistence includes positive fuel"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
            return Piece && Piece->fuelHours > 3.9 && !Controller->ToastIsError();
        });
    Add(TEXT("Return to the harvested plot for the persistence checkpoint"),
        [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->FocusTitle() == TEXT("A little patch of earth"); }, 0.65f);
    const auto SavedState = MakeShared<std::string>();
    const auto SavedLook = MakeShared<FHomesteadAppearance>();
    Add(TEXT("Save the complete harvested homestead from the paused pack"),
        [this, SavedState, SavedLook]()
        {
            Tap(EKeys::Gamepad_Special_Right);
            *SavedState = Controller->Simulation().Serialize();
            *SavedLook = Controller->GetAppearance();
            Tap(EKeys::F5);
        },
        [this, BerryPlotId]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            const auto* BerryPlot = FindPlot(Controller->State(), *BerryPlotId);
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            const auto* Hearth = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
            const int32 PersistedClears = static_cast<int32>(std::count_if(
                Controller->State().resourceEdits.begin(), Controller->State().resourceEdits.end(),
                [](const Homestead::ResourceEdit& Edit) { return Edit.cleared; }));
            return Controller->IsBookOpen() && !Controller->ToastIsError() && Plot && !Plot->planted
                && Plot->kind == Homestead::CropKind::Roots
                && BerryPlot && BerryPlot->planted && BerryPlot->kind == Homestead::CropKind::Berries
                && BerryPlot->growth > 0 && BerryPlot->growth < 1
                && Piece && Piece->storage[static_cast<int32>(Homestead::Item::Stone)] == 1
                && Hearth && Hearth->fuelHours > 3.8
                && PersistedClears >= 5;
        });
    Add(TEXT("Close the saved pack"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Replant after saving to create a real garden-state difference"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->planted && !Controller->ToastIsError();
        });
    Add(TEXT("Approach the chest to change saved storage"),
        [this, Chest]() { Teleport(Chest); },
        [this]() { return Controller->FocusTitle() == TEXT("Storage chest"); }, 0.65f);
    Add(TEXT("Open the saved storage"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    QueueSelectRow(static_cast<int32>(Homestead::Item::Stone));
    Add(TEXT("Enter the saved Stone's native actions"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu.IsValid()
            && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
    Add(TEXT("Focus the semantic Take action for saved Stone"),
        [this]()
        {
            if (!Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Transfer))
                Finish(false, TEXT("The semantic saved-Stone Take action is unavailable."));
        },
        [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
    Add(TEXT("Open the saved Stone amount dialog through the real native action"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu->HasActiveDialog()
            && Controller->NativeMenu->GetDraftQuantity() == 1; });
    Add(TEXT("Confirm one saved Stone through the real native amount dialog"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Chest, -4, -3);
            return Piece && Piece->storage[static_cast<int32>(Homestead::Item::Stone)] == 0 && !Controller->ToastIsError();
        });
    Add(TEXT("Close storage and approach the saved fire"),
        [this, Fire]() { Tap(EKeys::Gamepad_FaceButton_Right); Teleport(Fire); },
        [this]() { return Controller->FocusTitle() == TEXT("Cookfire"); }, 0.65f);
    Add(TEXT("Change the saved fire fuel through an actual added branch"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]()
        {
            const auto* Piece = FindPiece(Controller->State(), Homestead::Piece::Fire, -3, -2);
            return Piece && Piece->fuelHours > 7.8 && !Controller->ToastIsError();
        });
    Add(TEXT("Restore harvested plot, depleted forage, buildings, fuel, chest, inventory and appearance"),
        [this]() { Tap(EKeys::F9); Tap(EKeys::Gamepad_Special_Right); },
        [this, SavedState, SavedLook, Home, BerryPlotId]()
        {
            const auto* BerryPlot = FindPlot(Controller->State(), *BerryPlotId);
            return Controller->IsBookOpen() && !Controller->ToastIsError()
                && Controller->Simulation().Serialize() == *SavedState
                && BerryPlot && BerryPlot->kind == Homestead::CropKind::Berries
                && BerryPlot->planted && BerryPlot->growth > 0 && BerryPlot->growth < 1
                && SameAppearance(Controller->GetAppearance(), *SavedLook)
                && Controller->HasHeroine() && Controller->Simulation().IsSheltered(Home);
        }, 0.8f);
    Add(TEXT("Return from the restored pack to the garden"),
        [this, Garden]() { Tap(EKeys::Gamepad_FaceButton_Right); Teleport(Garden); },
        [this]() { return Controller->FocusTitle() == TEXT("A little patch of earth"); }, 0.65f);
    Add(TEXT("Plant the next generation using the harvested seeds"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->planted && !Controller->ToastIsError();
        });
    Add(TEXT("Save a living second-generation garden"),
        [this, SavedState]()
        {
            Tap(EKeys::Gamepad_Special_Right);
            *SavedState = Controller->Simulation().Serialize();
            Tap(EKeys::F5);
        },
        [this]() { return Controller->IsBookOpen() && !Controller->ToastIsError(); });
    Add(TEXT("Close the pack and change the saved soil moisture"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            return Plot && Plot->planted && Plot->moisture > 0.99 && !Controller->ToastIsError();
        });
    Add(TEXT("Reload retains the exact living crop and no offline progression"),
        [this]() { Tap(EKeys::F9); Tap(EKeys::Gamepad_Special_Right); },
        [this, SavedState, SavedLook, BerryPlotId]()
        {
            const auto* Plot = FindPlot(Controller->State(), GardenPlotId);
            const auto* BerryPlot = FindPlot(Controller->State(), *BerryPlotId);
            return Controller->IsBookOpen() && !Controller->ToastIsError() && Plot && Plot->planted
                && Plot->kind == Homestead::CropKind::Roots
                && BerryPlot && BerryPlot->planted && BerryPlot->kind == Homestead::CropKind::Berries
                && BerryPlot->growth > 0 && BerryPlot->growth < 1
                && Controller->Simulation().Serialize() == *SavedState
                && SameAppearance(Controller->GetAppearance(), *SavedLook)
                && !Controller->IsFailed();
        }, 0.8f);

    Add(TEXT("Leave the saved garden checkpoint to test genuine survival failure"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, ProtectedRecovery]()
        {
            return !Controller->IsBookOpen() && !Controller->IsFailed() && !ProtectedRecovery->empty();
        });
    QueueGatherTo(Homestead::Item::Branch, 4);
    QueueGatherTo(Homestead::Item::Fiber, 4);
    QueueClearCell(0, 8);
    QueuePlace(Homestead::Piece::Bed, 0, 8);
    const Homestead::Point OutdoorBed = Homestead::CellCenter(0, 8);
    Add(TEXT("Approach the distant outdoor bed without shelter or a nearby fire"),
        [this, OutdoorBed]() { Teleport(OutdoorBed); },
        [this]()
        {
            return Controller->FocusTitle() == TEXT("Bedroll")
                && !Controller->Simulation().IsSheltered(Controller->PlayerPoint())
                && !Controller->Simulation().IsNearFire(Controller->PlayerPoint())
                && !Controller->IsFailed();
        }, 0.65f);
    for (int32 Rest = 0; Rest < 12; ++Rest)
    {
        const auto BeforeHour = MakeShared<double>(0);
        const auto BeforeHunger = MakeShared<double>(0);
        Add(FString::Printf(TEXT("Outdoor sleep %d advances real survival needs until failure"), Rest + 1),
            [this, BeforeHour, BeforeHunger]()
            {
                *BeforeHour = Controller->State().hour;
                *BeforeHunger = Controller->State().hunger;
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this, BeforeHour, BeforeHunger]()
            {
                const auto& State = Controller->State();
                const double Advanced = State.hour - *BeforeHour;
                if (Controller->IsFailed())
                    return Advanced > 0 && Advanced <= 8.1 && (State.hunger == 0 || State.warmth == 0);
                return Advanced >= 7.99 && Advanced < 8.1 && State.hunger < *BeforeHunger - 10
                    && !Controller->ToastIsError()
                    && !Controller->Simulation().IsSheltered(Controller->PlayerPoint());
            }, 0.6f);
        // A is also retry while failed; never let a remaining queued sleep dismiss the failure modal.
        Steps.Last().Skip = [this]() { return Controller->IsFailed(); };
    }
    const auto FailedState = MakeShared<std::string>();
    Add(TEXT("Genuine failed vitals expose the gamepad recovery modal"),
        [this, FailedState]() { *FailedState = Controller->Simulation().Serialize(); },
        [this]()
        {
            const auto& State = Controller->State();
            return Controller->IsFailed() && (State.hunger == 0 || State.warmth == 0)
                && Controller->UsesGamepad() && !Controller->IsBookOpen() && !Controller->IsPlanning()
                && Controller->GetHUD() && Controller->ToastIsError()
                && Controller->Toast().Contains(TEXT("recovery checkpoint"));
        });
    Add(TEXT("Failure holds all simulation still while awaiting the explicit retry"),
        [this]() { Screenshot(TEXT("failure-retry")); },
        [this, FailedState]()
        {
            return Controller->IsFailed() && Controller->Simulation().Serialize() == *FailedState;
        }, 1.0f);
    Add(TEXT("Gamepad A retries the protected same-world cabin checkpoint rather than a new game"),
        [this]()
        {
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_Special_Right);
        },
        [this, ProtectedRecovery, RecoveryPosition, SavedLook, Home, BerryPlotId]()
        {
            const auto& State = Controller->State();
            const auto Position = Controller->PlayerPoint();
            const auto* Plot = FindPlot(State, GardenPlotId);
            const auto* BerryPlot = FindPlot(State, *BerryPlotId);
            const auto* Piece = FindPiece(State, Homestead::Piece::Chest, -4, -3);
            const int32 PersistedClears = static_cast<int32>(std::count_if(
                State.resourceEdits.begin(), State.resourceEdits.end(),
                [](const Homestead::ResourceEdit& Edit) { return Edit.cleared; }));
            const bool Restored = !Controller->IsFailed() && !Controller->ToastIsError() && Controller->IsBookOpen()
                && Controller->Toast().Contains(TEXT("sheltered recovery checkpoint"))
                && Controller->Simulation().Serialize() == *ProtectedRecovery
                && State.hunger > 40 && State.warmth >= 80 && State.energy > 99
                && FMath::Abs(Position.x - RecoveryPosition->x) < 5
                && FMath::Abs(Position.y - RecoveryPosition->y) < 5
                && Controller->Simulation().IsSheltered(Position)
                && Controller->Simulation().IsSheltered(Home)
                && Plot && Plot->planted && Plot->growth == 1
                && Plot->kind == Homestead::CropKind::Roots
                && BerryPlot && BerryPlot->planted && BerryPlot->kind == Homestead::CropKind::Berries
                && BerryPlot->growth > 0 && BerryPlot->growth < 1
                && Piece && Piece->storage[static_cast<int32>(Homestead::Item::Stone)] == 1
                && !FindPiece(State, Homestead::Piece::Bed, 0, 8)
                && PersistedClears >= 5
                && SameAppearance(Controller->GetAppearance(), *SavedLook) && Controller->HasHeroine();
            if (!Restored)
            {
                UE_LOG(LogTemp, Error, TEXT("Recovery evidence: failed=%d error=%d book=%d position=(%.3f,%.3f) expected=(%.3f,%.3f) sheltered=%d crop=%d/%.6f chest=%d heroine=%d appearance=%d"),
                    Controller->IsFailed(), Controller->ToastIsError(), Controller->IsBookOpen(),
                    Position.x, Position.y, RecoveryPosition->x, RecoveryPosition->y,
                    Controller->Simulation().IsSheltered(Position), Plot ? Plot->planted : false,
                    Plot ? Plot->growth : -1.0, Piece ? Piece->storage[static_cast<int32>(Homestead::Item::Stone)] : -1,
                    Controller->HasHeroine(), SameAppearance(Controller->GetAppearance(), *SavedLook));
                UE_LOG(LogTemp, Error, TEXT("Expected recovery prefix: %s"), *FString(UTF8_TO_TCHAR(ProtectedRecovery->c_str())).Left(250));
                UE_LOG(LogTemp, Error, TEXT("Actual recovery prefix: %s"), *FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())).Left(250));
            }
            return Restored;
        }, 0.8f);
}
