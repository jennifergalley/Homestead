#include "HomesteadUIGallery.h"

#if !UE_BUILD_SHIPPING
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Simulation/HomesteadBackpack.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadEstatePublicRoad.h"
#include "Simulation/HomesteadShops.h"
#include "Simulation/HomesteadTravel.h"
#include "UI/SHomesteadMenu.h"
#include "UI/SHomesteadShop.h"

#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "ShowFlags.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"

#include <cmath>

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadUIGallery, Log, All);

namespace HomesteadUIGalleryFixture
{
// The fixture every entry starts from, taken from the game the first time the gallery runs in it.
TOptional<Homestead::Simulation> Base;
TWeakObjectPtr<UWorld> BaseWorld;
FTSTicker::FDelegateHandle Pending;
int32 Cursor = INDEX_NONE;
// The live console's backdrop (homestead.UIGallery backdrop plain|world), reapplied after each entry.
bool bLivePlain = false;
// Open ground west of the manor with nothing in reach (Estate PIE, 2026-09-30), facing east.
constexpr double StageX = -1500.0, StageY = -2600.0;
// Tillable meadow by the manor garden (a plot was tilled here in Estate PIE, 2026-09-30).
constexpr double GardenX = -1242.0, GardenY = -2114.0;
// How long to wait for her to settle after a teleport, and how close counts as there (cm).
constexpr double ArriveSeconds = 45.0;
constexpr double ArrivedWithinCm = 160.0;
constexpr double SameSpotCm = 100.0;

double Distance(Homestead::Point A, Homestead::Point B) { return std::hypot(A.x - B.x, A.y - B.y); }
Homestead::Point Toward(Homestead::Point From, Homestead::Point To, double Cm)
{
    const double Length = FMath::Max(1.0, Distance(From, To));
    return {From.x + (To.x - From.x) / Length * Cm, From.y + (To.y - From.y) / Length * Cm};
}
Homestead::Point Ahead(Homestead::Point From, double YawDegrees, double Cm)
{
    const double Radians = FMath::DegreesToRadians(YawDegrees);
    return {From.x + std::cos(Radians) * Cm, From.y + std::sin(Radians) * Cm};
}
}

namespace HomesteadUIGalleryBackdrop
{
// Fog and effects off on the plain backdrop (FEngineShowFlags names; any missing are skipped). The sky
// stays on so its light and the exposure stay as in the game (with it off, auto-exposure blew her out).
const TCHAR* const HiddenFlags[] = {
    TEXT("Fog"), TEXT("VolumetricFog"), TEXT("Particles"), TEXT("Decals"),
    TEXT("Landscape"), TEXT("InstancedGrass"), TEXT("InstancedFoliage")};
// A calm warm grey, lit like any surface in the scene, so the exposure matches the game.
const FLinearColor Neutral(0.30f, 0.28f, 0.25f, 1.0f);
// The plane stands this far beyond the camera, wide enough to fill any view.
constexpr float DistanceCm = 6000.0f;
constexpr float ScaleOfBasicPlane = 400.0f; // the basic plane is 100 cm across
const TCHAR* const PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");
const TCHAR* const UnlitMaterial = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
TWeakObjectPtr<AStaticMeshActor> Plane;
TWeakObjectPtr<UWorld> PlaneWorld;
}

void FHomesteadUIGallery::SetBackdrop(AHomesteadController& PC, bool bPlain, bool bHeroine)
{
    using namespace HomesteadUIGalleryBackdrop;
    UWorld* World = PC.GetWorld();
    UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
    if (!World || !Viewport) return;
    for (const TCHAR* Name : HiddenFlags)
    {
        const int32 Index = FEngineShowFlags::FindIndexByName(Name);
        if (Index != INDEX_NONE) Viewport->EngineShowFlags.SetSingleFlag(static_cast<uint32>(Index), !bPlain);
    }
    APawn* Heroine = PC.GetPawn();
    if (Heroine) Heroine->SetActorHiddenInGame(bPlain && !bHeroine);
    PC.HiddenActors.Reset();
    if (PlaneWorld.Get() != World) { Plane.Reset(); PlaneWorld = World; }
    if (!bPlain)
    {
        if (Plane.IsValid()) Plane->SetActorHiddenInGame(true);
        return;
    }
    if (!Plane.IsValid())
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, PlaneMesh);
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, UnlitMaterial);
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Spawn);
        if (!Actor || !Mesh) return;
        UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(false);
        if (Material)
        {
            UMaterialInstanceDynamic* Colour = UMaterialInstanceDynamic::Create(Material, Actor);
            Colour->SetVectorParameterValue(TEXT("Color"), Neutral);
            Component->SetMaterial(0, Colour);
        }
        Plane = Actor;
    }
    // Square to the camera, beyond the heroine.
    FVector Eye;
    FRotator View;
    PC.GetPlayerViewPoint(Eye, View);
    const FVector Forward = View.Vector();
    Plane->SetActorHiddenInGame(false);
    Plane->SetActorLocationAndRotation(Eye + Forward * DistanceCm, FRotationMatrix::MakeFromZ(-Forward).Rotator());
    Plane->SetActorScale3D(FVector(ScaleOfBasicPlane, ScaleOfBasicPlane, 1.0f));
    // Everything else in the world is hidden from her view: the heroine, what she carries and the
    // plane stay (the controller, HUD and camera draw nothing themselves).
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == Plane.Get() || Actor == Heroine || Actor == &PC) continue;
        bool bHers = false;
        for (AActor* Parent = Actor->GetAttachParentActor(); Parent && !bHers; Parent = Parent->GetAttachParentActor())
            bHers = Parent == Heroine;
        if (Heroine && Actor->GetOwner() == Heroine) bHers = true;
        if (!bHers) PC.HiddenActors.Add(Actor);
    }
}
void FHomesteadUIGallery::ResetFixture()
{
    HomesteadUIGalleryFixture::Base.Reset();
    HomesteadUIGalleryFixture::BaseWorld.Reset();
}

void FHomesteadUIGallery::Prepare(AHomesteadController& PC, bool bPad)
{
    using namespace HomesteadUIGalleryFixture;
    if (PC.IsShopScreenOpen()) PC.CloseShopScreen();
    // Out of the new-game setup (its Names step, or its Appearance step in the book).
    if (PC.NamesWidget.IsValid()) PC.HideNames();
    PC.bNewGameSetup = false;
    if (PC.NativeMenu.IsValid() && PC.NativeMenu->HasActiveDialog()) PC.NativeMenu->Back();
    if (PC.IsBookOpen()) PC.CloseBook();
    if (auto* Avatar = Cast<AHomesteadCharacter>(PC.GetPawn())) Avatar->CancelAction(true);
    UWorld* World = PC.GetWorld();
    if (!Base.IsSet() || BaseWorld.Get() != World)
    {
        Homestead::Simulation& Sim = PC.Sim;
        Sim.SkipToHourOfDay(10.0);
        // A trading day: the shops close on Sundays (Water's close-shops-on-sundays).
        if (Sim.Today().weekday == Homestead::Weekday::Sunday) Sim.SkipToHourOfDay(10.0);
        const TPair<Homestead::Item, int32> Kit[] = {
            {Homestead::Item::Pasty, 3}, {Homestead::Item::Bread, 2}, {Homestead::Item::Berries, 5},
            {Homestead::Item::Branch, 6}, {Homestead::Item::Stone, 4}, {Homestead::Item::Fiber, 3},
            {Homestead::Item::Seeds, 4}, {Homestead::Item::DiggingStick, 1}, {Homestead::Item::WateringCan, 1}};
        for (const auto& Stock : Kit)
            if (Sim.Count(Stock.Key) < Stock.Value) Sim.GrantItems(Stock.Key, Stock.Value - Sim.Count(Stock.Key));
        Sim.GrantMoney(1000 - Sim.GetState().money);
        Sim.SetEnergy(80.0);
        Base = Sim;
        BaseWorld = World;
    }
    else PC.Sim = *Base;
    // A fresh revision, so nothing cached from the last entry is reused.
    PC.Sim.SetEnergy(PC.State().energy);
    PC.ToastRemaining = 0;
    PC.ToastText.Reset();
    PC.bToastError = false;
    PC.Pickups.Reset();
    PC.ControlsHint.Advance(1.0e6, true);
    PC.bGamepad = bPad;
    PC.bShowMouseCursor = !bPad;
    const int32 Empty = PC.FirstEmptyHotbarCell();
    if (Empty != INDEX_NONE) PC.SelectHotbarSlot(Empty);
}

void FHomesteadUIGallery::Face(AHomesteadController& PC, Homestead::Point At)
{
    const Homestead::Point From = PC.PlayerPoint();
    const float Yaw = static_cast<float>(FMath::RadiansToDegrees(std::atan2(At.y - From.y, At.x - From.x)));
    PC.SetControlRotation(FRotator(-14.0f, Yaw, 0.0f));
    if (APawn* Pawn = PC.GetPawn()) Pawn->SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
}

const TArray<FHomesteadUIGallery::FEntry>& FHomesteadUIGallery::Entries()
{
    using namespace HomesteadUIGalleryFixture;
    using Homestead::Item;
    using Homestead::Point;
    static TArray<FEntry> List;
    if (!List.IsEmpty()) return List;

    const Point Stage{StageX, StageY};
    const auto Press = [](AHomesteadController& PC, FKey Key)
    {
        PC.InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1));
        PC.InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0));
    };
    const auto Accept = [Press](AHomesteadController& PC) { Press(PC, PC.UsesGamepad() ? EKeys::Gamepad_FaceButton_Bottom : EKeys::Enter); };
    // After the book has built its rows (it builds on the frame after it opens).
    const auto Later = [](AHomesteadController& PC, TFunction<void(AHomesteadController&)> Then)
    {
        TWeakObjectPtr<AHomesteadController> Weak(&PC);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Then](float)
            { if (Weak.IsValid()) Then(*Weak.Get()); return false; }), 0.3f);
    };
    // The fixture's stacks start in the hotbar row; the pack grid's popovers need one below it.
    const auto ToPack = [](AHomesteadController& PC, Item Wanted)
    {
        for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell)
        {
            FHomesteadRow Held;
            if (PC.MenuHotbarRow(Cell, Held) && Held.Subject == EHomesteadMenuSubject::ItemGroup && Held.Id == static_cast<int32>(Wanted))
            { PC.MenuMoveHotbarToPack(Cell, nullptr); return; }
        }
    };
    const auto Notify = [](const TCHAR* Text, bool bError)
    { return [Text, bError](AHomesteadController& PC) { PC.Notify(Text, bError); }; };
    const auto Book = [](int32 Page) { return [Page](AHomesteadController& PC) { PC.OpenBook(Page); }; };
    // Stand `Cm` from the nearest uncleared resource of a kind, coming from the stage.
    const auto NearResource = [Stage](Homestead::ResourceKind Kind, double Cm)
    {
        return [Stage, Kind, Cm](AHomesteadController& PC, FStage& Out, FString& Why)
        {
            const Homestead::ResourceNode* Best = nullptr;
            for (const auto& Node : PC.State().resources)
                if (Node.kind == Kind && !Node.cleared && (!Best || Distance(Node.position, Stage) < Distance(Best->position, Stage)))
                    Best = &Node;
            if (!Best) { Why = TEXT("No such resource on this estate."); return false; }
            Out.bMove = true;
            Out.Stand = Toward(Best->position, Stage, Cm);
            Out.Face = Best->position;
            return true;
        };
    };
    const auto Garden = [](AHomesteadController&, FStage& Out, FString&)
    {
        Out.bMove = true;
        Out.Stand = {GardenX, GardenY};
        Out.Face = {GardenX + 500.0, GardenY};
        return true;
    };
    // A tilled square just ahead of her in the garden (returns its plot id, or -1).
    const auto TillAhead = [](AHomesteadController& PC)
    {
        const Point At = Ahead(PC.PlayerPoint(), 0.0, 150.0);
        PC.Sim.Till(Homestead::GardenCell(At.x), Homestead::GardenCell(At.y), PC.PlayerPoint());
        int32 Plot = -1;
        double Closest = 1.0e9;
        for (const auto& Each : PC.State().plots)
            if (const double D = Distance(Homestead::PlotCenter(Each), At); D < Closest) { Closest = D; Plot = Each.id; }
        return Closest < 200.0 ? Plot : -1;
    };
    const auto Chest = [](AHomesteadController& PC, FStage& Out, FString& Why)
    {
        for (const auto& Structure : PC.State().structures)
            if (Structure.kind == Homestead::Piece::Chest)
            {
                const Point At = PC.Sim.StructureCenter(Structure);
                const auto* Spawn = PC.Sim.Layout().FindLandmark(Homestead::Anchor::StandingRoomSpawn);
                Out.bMove = true;
                Out.Stand = Toward(At, Spawn ? Spawn->position : Point{At.x + 200.0, At.y}, 150.0);
                Out.Face = At;
                return true;
            }
        Why = TEXT("No storage chest in this game.");
        return false;
    };
    const auto ChestId = [](const AHomesteadController& PC)
    {
        for (const auto& Structure : PC.State().structures)
            if (Structure.kind == Homestead::Piece::Chest) return Structure.id;
        return -1;
    };
    const auto Counter = [](AHomesteadController& PC, FStage& Out, FString& Why)
    {
        if (PC.State().shops.empty()) { Why = TEXT("No general store in this game."); return false; }
        const auto& Shop = PC.State().shops.front();
        Out.bMove = true;
        Out.Stand = Ahead({Shop.counterX, Shop.counterY}, Shop.counterYaw, 150.0);
        Out.Face = {Shop.counterX, Shop.counterY};
        return true;
    };
    const auto Shop = [](int32 Tab, int64 Coins, const TCHAR* Choose)
    {
        return [Tab, Coins, Choose](AHomesteadController& PC)
        {
            if (Coins >= 0) PC.Sim.GrantMoney(Coins - PC.State().money);
            if (PC.State().shops.empty()) return;
            PC.OpenShopScreen(PC.State().shops.front().id, false);
            const auto Screen = PC.GetShopScreen();
            if (!Screen.IsValid()) return;
            Screen->SetTab(Tab);
            if (!Choose) return;
            for (int32 Index = 0; Index < Screen->RowCount(); ++Index)
                if (FString(Choose) == TEXT("upgrade") ? Screen->IsUpgradeRow(Index) : Screen->RowLabel(Index).Contains(Choose))
                { Screen->Choose(Index); return; }
        };
    };
    const auto Sign = [](const TCHAR* Name, double Cm)
    {
        return [Name, Cm](AHomesteadController&, FStage& Out, FString& Why)
        {
            const auto* Found = Homestead::EstatePublicRoad().FindSign(TCHAR_TO_UTF8(Name));
            if (!Found) { Why = TEXT("No such road sign."); return false; }
            Out.bMove = true;
            Out.Stand = Ahead(Found->position, Found->yaw, Cm);
            Out.Face = Found->position;
            return true;
        };
    };
    const auto SignIndex = [](const TCHAR* Name) -> int32
    {
        const auto& Signs = Homestead::EstatePublicRoad().signs;
        for (int32 Index = 0; Index < static_cast<int32>(Signs.size()); ++Index)
            if (Signs[Index].name == TCHAR_TO_UTF8(Name)) return Index;
        return INDEX_NONE;
    };
    const auto Eat = [](double Energy)
    {
        return [Energy](AHomesteadController& PC)
        {
            PC.Sim.SetEnergy(Energy);
            if (PC.ChooseOnHotbar(Item::Pasty)) PC.EatFromHotbar(Item::Pasty);
        };
    };
    // The next spell of rain (Simulation/HomesteadRain.h: any hour, day or night), half an hour in; or tonight.
    const auto Rain = [](AHomesteadController& PC)
    {
        Homestead::RainSpell Spell;
        if (!Homestead::NextRainSpell(PC.State().hour, Spell) || Homestead::IsRainingAt(PC.State().hour)) return;
        const double At = Spell.start + FMath::Min(0.5, (Spell.end - Spell.start) * 0.5);
        for (int32 Day = 0; Day < 40 && PC.State().hour < At - 1e-6; ++Day) PC.Sim.SkipToHourOfDay(FMath::Fmod(At, 24.0));
    };
    const auto Night = [](AHomesteadController& PC) { PC.Sim.SkipToHourOfDay(22.5); };
    const auto Add = [](const TCHAR* Id, const TCHAR* Description, ECover Cover, int32 Key,
        TFunction<bool(AHomesteadController&, FStage&, FString&)> StageAt, TFunction<void(AHomesteadController&)> Apply,
        float Settle = 0.9f)
    {
        FEntry& Entry = List.AddDefaulted_GetRef();
        Entry.Id = Id;
        Entry.Description = Description;
        Entry.Cover = Cover;
        Entry.Key = Key;
        Entry.Stage = MoveTemp(StageAt);
        Entry.Apply = MoveTemp(Apply);
        Entry.Settle = Settle;
    };
    const int32 Toast = static_cast<int32>(ENotice::WorldNotice);
    const int32 ToastError = static_cast<int32>(ENotice::WorldNoticeError);

    // The field book.
    Add(TEXT("book-pack"), TEXT("Field book, Inventory tab: the hotbar row over the pack grid, her portrait and equipped slots."),
        ECover::BookPage, 0, nullptr, Book(0));
    Add(TEXT("book-pack-rows"), TEXT("Pack page: the hotbar row as the grid's first row, its cells the same size and spacing as the rows beneath."),
        ECover::BookPage, 0, nullptr, Book(0));
    Add(TEXT("book-back-equipment"), TEXT("Back equipment: Leather Rucksack or None; the appearance-only capacity hint shows only while None is highlighted."),
        ECover::Dialog, 10, nullptr, [Later](AHomesteadController& Opened)
        {
            const auto* Store = Opened.Sim.FindShop(Homestead::ShopKind::GeneralStore);
            if (!Store) { Opened.Notify(TEXT("The gallery store is unavailable."), true); return; }
            const int32 ShopId = Store->id;
            const Homestead::Point Counter{Store->counterX, Store->counterY};
            const auto Money = Opened.Sim.GrantMoney(Homestead::Backpack::Price - Opened.State().money);
            if (!Money) { Opened.Notify(Money); return; }
            const auto Bought = Opened.Sim.BuyBackpack(ShopId, Counter);
            if (!Bought) { Opened.Notify(Bought); return; }
            Opened.OpenBook(0);
            Later(Opened, [](AHomesteadController& PC)
            {
                PC.MenuSetBackEquipment(true);
                if (PC.NativeMenu.IsValid()) PC.NativeMenu->OpenBackEquipment();
            });
        });
    Add(TEXT("book-seed-packets"), TEXT("Individual crop-specific seed packets: two of each crop, never stacked."),
        ECover::BookPage, 0, nullptr, [](AHomesteadController& PC)
        {
            PC.Sim.SetPackRowAutoFill(false);
            for (Item Seed : {Item::Seeds, Item::TurnipSeed, Item::CarrotSeed, Item::SeedPotato,
                Item::CabbageSeed, Item::BroadBeanSeed, Item::StrawberryRunner})
            {
                const auto Granted = PC.Sim.GrantItems(Seed, 2);
                if (!Granted) PC.Notify(Granted);
            }
            PC.Sim.SetPackRowAutoFill(true);
            PC.OpenBook(0);
        });
    Add(TEXT("book-pack-dragging"), TEXT("Pack page, holding a stack of branches over the hotbar row: its icon and count ride with the pointer (on the pad, on the focused cell's corner)."),
        ECover::Dialog, 9, nullptr,
        [Later, ToPack](AHomesteadController& Opened)
        {
            ToPack(Opened, Item::Branch);
            Opened.OpenBook(0);
            Later(Opened, [Later](AHomesteadController& PC)
            {
                if (!PC.NativeMenu.IsValid()) return;
                for (const auto& Row : PC.MenuRows())
                    if (Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.Id == static_cast<int32>(Item::Branch))
                    { PC.NativeMenu->BeginPlacingOnHotbar(Row); break; }
                // The pointer over the cell she's aiming at (a capture shows no cursor of its own).
                Later(PC, [](AHomesteadController& Aimed)
                {
                    if (!Aimed.NativeMenu.IsValid() || Aimed.UsesGamepad() || !FSlateApplication::IsInitialized()) return;
                    if (const auto Cell = Aimed.NativeMenu->GetBookHotbarSlot(FMath::Max(0, Aimed.NativeMenu->GetFocusedHotbarSlot())))
                        FSlateApplication::Get().SetCursorPos(Cell->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.55f, 0.6f)));
                });
            });
        }, 1.6f);
    Add(TEXT("book-craft"), TEXT("Craft tab: recipes with what each needs, the ones she can make now first."),
        ECover::BookPage, 1, nullptr, Book(1));
    Add(TEXT("book-craft-unaffordable"), TEXT("Craft tab, pressing a recipe she can't make yet: a rust-edged parchment notice saying what to gather."),
        ECover::Dialog, 0, nullptr, [Accept](AHomesteadController& PC)
        {
            PC.OpenBook(1);
            for (const auto& Row : PC.MenuRows())
                if (Row.Subject == EHomesteadMenuSubject::Recipe && Row.HasRecipeState && !Row.RecipeState.craftable)
                {
                    if (PC.NativeMenu.IsValid() && PC.NativeMenu->FocusSubject(Row.Subject, Row.SubjectId, Row.ContainerId)) Accept(PC);
                    return;
                }
        });
    Add(TEXT("book-build"), TEXT("Build tab: building plans; the details list 'Requires:' with each material's have/need, as Craft does."), ECover::BookPage, 2, nullptr, Book(2));
    Add(TEXT("book-build-unaffordable"), TEXT("Build tab, a plan she can't afford yet (a wall if so): its short material in the missing colour, and 'Foundation required'."),
        ECover::Dialog, 11, nullptr, [Later](AHomesteadController& Opened)
        {
            Opened.OpenBook(2);
            Later(Opened, [](AHomesteadController& PC)
            {
                if (!PC.NativeMenu.IsValid()) return;
                int32 Chosen = INDEX_NONE;
                for (int32 Index = 0; Index < static_cast<int32>(Homestead::Piece::Count); ++Index)
                {
                    const auto Piece = static_cast<Homestead::Piece>(Index);
                    if (!Homestead::IsBuildable(Piece)) continue;
                    const Homestead::Inventory Cost = Homestead::PieceCost(Piece);
                    bool bShort = false;
                    for (int32 Material = 0; Material < Homestead::ItemCount; ++Material)
                        bShort = bShort || PC.Sim.Count(static_cast<Item>(Material)) < Cost[Material];
                    if (bShort && (Chosen == INDEX_NONE || Piece == Homestead::Piece::Wall)) Chosen = Index;
                }
                if (Chosen != INDEX_NONE) PC.NativeMenu->FocusSubject(EHomesteadMenuSubject::Legacy, Chosen, 0);
            });
        }, 1.3f);
    Add(TEXT("book-map"), TEXT("Map tab: the estate map with her marker, places and the key."), ECover::BookPage, 7, nullptr, Book(7));
    Add(TEXT("book-appearance"), TEXT("Appearance tab: her full-length view and the look choices."), ECover::BookPage, 6, nullptr, Book(6));
    Add(TEXT("book-credits"), TEXT("Credits page."), ECover::BookPage, 5, nullptr, Book(5));
    Add(TEXT("book-settings"), TEXT("Settings (Start / Esc): Save, Load and Quit over the settings tabs, the clock paused."),
        ECover::BookPage, 4, nullptr, Book(4));
    for (int32 Tab = 0; Tab < 3; ++Tab)
    {
        static const TCHAR* const Ids[] = {TEXT("settings-game"), TEXT("settings-sound"), TEXT("settings-video")};
        static const TCHAR* const Words[] = {
            TEXT("Settings, Game tab: game speed, camera, autosave, minimap and hints."),
            TEXT("Settings, Sound tab: overall, music, ambience and effects sliders."),
            TEXT("Settings, Video tab: 3D resolution scale and vertical sync.")};
        Add(Ids[Tab], Words[Tab], ECover::SettingsTab, Tab, nullptr, [Tab](AHomesteadController& PC)
        {
            PC.OpenBook(4);
            if (PC.NativeMenu.IsValid()) PC.NativeMenu->SetSettingsTab(Tab);
        });
    }
    Add(TEXT("settings-quit-confirm"), TEXT("Quit game confirm: Save & Quit or Quit without Saving."), ECover::Dialog, 1, nullptr,
        [](AHomesteadController& PC) { PC.OpenBook(4); if (PC.NativeMenu.IsValid()) PC.NativeMenu->RequestExit(); });
    Add(TEXT("settings-save-error"), TEXT("A failed save: the error dialog with what to do."), ECover::Dialog, 2, nullptr,
        [](AHomesteadController& PC)
        {
            PC.OpenBook(4);
            if (PC.NativeMenu.IsValid())
                PC.NativeMenu->ShowSaveFailure(TEXT("Could not write the save: the disk is full. Free some space, then save again."));
        });
    Add(TEXT("book-notice"), TEXT("Field book notice: a parchment card, 'Your homestead is saved.'"),
        ECover::Notice, static_cast<int32>(ENotice::BookNotice), nullptr,
        [](AHomesteadController& PC) { PC.OpenBook(0); PC.Notify(TEXT("Your homestead is saved."), false); });
    Add(TEXT("book-notice-error"), TEXT("Field book error notice: a rust-edged parchment card."),
        ECover::Notice, static_cast<int32>(ENotice::BookNoticeError), nullptr,
        [](AHomesteadController& PC) { PC.OpenBook(0); PC.Notify(TEXT("You can't carry any more. Store something in a chest first."), true); });
    Add(TEXT("book-item-menu"), TEXT("Pack, a Cornish pasty's item menu: Eat, Move, Drop and the like."), ECover::Dialog, 3, nullptr,
        [Later, ToPack](AHomesteadController& Opened)
        {
            ToPack(Opened, Item::Pasty);
            Opened.OpenBook(0);
            Later(Opened, [](AHomesteadController& PC)
            {
                const auto Rows = PC.MenuRows();
                for (int32 Index = 0; Index < Rows.Num(); ++Index)
                    if (Rows[Index].Subject == EHomesteadMenuSubject::ItemGroup && Rows[Index].Id == static_cast<int32>(Item::Pasty))
                    { if (PC.NativeMenu.IsValid()) PC.NativeMenu->OpenItemContextMenu(Index, !PC.UsesGamepad()); return; }
            });
        }, 1.3f);
    Add(TEXT("book-quantity"), TEXT("Pack, the how-many popover for a stack of branches."), ECover::Dialog, 4, nullptr,
        [Later, ToPack](AHomesteadController& Opened)
        {
            ToPack(Opened, Item::Branch);
            Opened.OpenBook(0);
            Later(Opened, [](AHomesteadController& PC)
            {
                for (const auto& Row : PC.MenuRows())
                    if (Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.Id == static_cast<int32>(Item::Branch))
                    { if (PC.NativeMenu.IsValid()) PC.NativeMenu->OpenQuantityPrompt(Row); return; }
            });
        }, 1.3f);
    Add(TEXT("book-chest"), TEXT("A storage chest open beside her pack, with its name and Store matching."), ECover::Dialog, 5, Chest,
        [ChestId](AHomesteadController& PC) { PC.OpenChestStorage(ChestId(PC)); });
    Add(TEXT("chest-hotbar-store"), TEXT("A chest open: the hotbar row heads the pack column as one of its rows, the same cells as the pack grid, ready to Shift+click or drag into the chest."),
        ECover::Dialog, 10, Chest,
        [ChestId](AHomesteadController& PC)
        {
            PC.OpenChestStorage(ChestId(PC));
            if (PC.NativeMenu.IsValid()) PC.NativeMenu->ActivateHotbarSlot(0);
        });
    Add(TEXT("book-chest-rename"), TEXT("Naming the chest: the name being typed and the suggestions."), ECover::Dialog, 6, Chest,
        [ChestId](AHomesteadController& PC)
        {
            if (!PC.OpenChestStorage(ChestId(PC)) || !PC.NativeMenu.IsValid()) return;
            PC.NativeMenu->OpenRenameChest();
            PC.NativeMenu->TypeChestName(TEXT("Linen chest"));
        });

    // The general store.
    Add(TEXT("shop-buy"), TEXT("General store, Buy tab: the leather backpack upgrade first, then goods with Energy and whole-coin prices."),
        ECover::Shop, 0, Counter, Shop(1, -1, nullptr));
    Add(TEXT("shop-sell"), TEXT("General store, Sell tab: what she can sell from her pack."), ECover::Shop, 1, Counter, Shop(0, -1, nullptr));
    Add(TEXT("shop-unaffordable"), TEXT("Buy tab with a purse of 30 coins, choosing a pasty: the status says how much more she needs."),
        ECover::Notice, static_cast<int32>(ENotice::ShopStatus), Counter, Shop(1, 30, TEXT("pasty")));
    Add(TEXT("shop-backpack"), TEXT("Buy tab with 1,500 coins, choosing the leather backpack: its confirm."), ECover::Shop, 2, Counter,
        Shop(1, 1500, TEXT("upgrade")));
    Add(TEXT("shop-fishing-pole"), TEXT("Buy tab with 1,500 coins, choosing the fishing pole under Upgrades beside the backpack."),
        ECover::Shop, 5, Counter, Shop(1, 1500, TEXT("Fishing pole")));
    // The leather knapsack on her back (Props' SM_LeatherBackpack), bought through the store's own rule
    // (from the counter: the gallery passes the counter's spot), seen in the open from several sides so
    // the attach can be judged: the pack behind her back, the straps over her shoulders, nothing through
    // her torso. The camera's yaw is relative to her facing (0 = behind her).
    const auto Knapsack = [](float CameraYaw)
    {
        return [CameraYaw](AHomesteadController& PC)
        {
            if (PC.State().shops.empty()) return;
            const auto& Shop = PC.State().shops.front();
            PC.Sim.GrantMoney(Homestead::Backpack::Price - PC.State().money);
            PC.Sim.BuyBackpack(Shop.id, Ahead({Shop.counterX, Shop.counterY}, Shop.counterYaw, 120.0));
            if (const APawn* Pawn = PC.GetPawn())
                PC.SetControlRotation(FRotator(-8.0f, Pawn->GetActorRotation().Yaw + CameraYaw, 0.0f));
        };
    };
    Add(TEXT("hud-backpack"), TEXT("The leather knapsack on her back, from behind: the pack between her shoulder blades, her pack now 240."),
        ECover::Shop, 4, nullptr, Knapsack(0.0f), 1.5f);
    Add(TEXT("hud-backpack-side"), TEXT("The knapsack from her left side (90 degrees): the pack against her back, nothing through her torso."),
        ECover::Hud, 12, nullptr, Knapsack(90.0f), 1.5f);
    Add(TEXT("hud-backpack-three-quarter"), TEXT("The knapsack from behind her left shoulder (three-quarter): the straps over her shoulder."),
        ECover::Hud, 13, nullptr, Knapsack(45.0f), 1.5f);
    Add(TEXT("hud-backpack-front"), TEXT("The knapsack from the front three-quarter: the straps down her chest."),
        ECover::Hud, 14, nullptr, Knapsack(150.0f), 1.5f);
    Add(TEXT("shop-quantity"), TEXT("Buy tab, choosing bread: the how-many dialog."), ECover::Shop, 3, Counter, Shop(1, -1, TEXT("Bread")));

    // Road signs.
    Add(TEXT("sign-confirm"), TEXT("The Gateway road sign: the centred 'Town / Manor' confirm over the Map page."), ECover::Dialog, 7,
        Sign(TEXT("GatewayRoadSign"), 180.0), [SignIndex](AHomesteadController& PC)
        {
            PC.Focus = AHomesteadController::EFocus::RoadSign;
            PC.FocusId = SignIndex(TEXT("GatewayRoadSign"));
            PC.InteractWithRoadSign();
        });
    // At the manor's stop, the town sign's one way (to the manor) is refused: she's already there.
    Add(TEXT("sign-refused"), TEXT("A one-way road sign she can't walk from here: only a rust-edged notice saying why ('You're already at the manor.'), no book."),
        ECover::Dialog, 8, [](AHomesteadController&, FStage& Out, FString& Why)
        {
            const auto* Manor = Homestead::EstatePublicRoad().FindStop("Manor");
            if (!Manor) { Why = TEXT("No manor stop on the public road."); return false; }
            Out.bMove = true;
            Out.Stand = Manor->position;
            Out.Face = {Manor->position.x + 500.0, Manor->position.y};
            return true;
        }, [SignIndex](AHomesteadController& PC)
        {
            PC.Focus = AHomesteadController::EFocus::RoadSign;
            PC.FocusId = SignIndex(TEXT("TownRoadSign"));
            PC.InteractWithRoadSign();
        });
    // World notices.
    Add(TEXT("toast-success"), TEXT("World notice: a parchment slip at the top centre in the book's serif."),
        ECover::Notice, Toast, nullptr, Notify(TEXT("Summer has come"), false));
    Add(TEXT("toast-error"), TEXT("World error notice: the rust-edged slip."),
        ECover::Notice, ToastError, nullptr, Notify(TEXT("Not ready yet"), true));
    Add(TEXT("toast-long"), TEXT("A long world notice wrapping to two or three lines."), ECover::Hud, 11, nullptr,
        Notify(TEXT("Mowed 6 tufts: +4 Hay, +3 Weeds, +1 Seeds. The old orchard meadow is opening up; come back with the scythe tomorrow for the rest."), false));
    Add(TEXT("toast-error-long"), TEXT("A long error notice wrapping to more than one line."), ECover::Hud, 0, nullptr,
        Notify(TEXT("Could not save vertical sync. Your previous preference was restored. Check that the settings file isn't read-only, then try again."), true));
    Add(TEXT("toast-wellfed"), TEXT("Eating a pasty from the hotbar: '+40 Energy · Well fed until …', the Energy bar filling and the Well fed chip."),
        ECover::Hud, 1, nullptr, Eat(55.0), 0.7f);

    // The HUD.
    Add(TEXT("hud-overview"), TEXT("World HUD: calendar and clock, Energy bar, purse, compass, minimap and the hotbar."), ECover::Hud, 2, nullptr, nullptr);
    Add(TEXT("hud-energy-low"), TEXT("Energy below 25: the bar amber and pulsing gently, and the 'Getting tired' notice it gave on crossing."),
        ECover::Hud, 3, nullptr,
        [](AHomesteadController& PC) { PC.Sim.SetEnergy(18.0); PC.Notify(TEXT("Getting tired"), false); });
    Add(TEXT("hud-energy-exhausted"), TEXT("Energy below 10: the bar a muted red and pulsing, and the 'Exhausted' notice."),
        ECover::Hud, 15, nullptr,
        [](AHomesteadController& PC) { PC.Sim.SetEnergy(6.0); PC.Notify(TEXT("Exhausted"), true); });
    Add(TEXT("hud-wellfed"), TEXT("Well fed: the pasty chip 'Well fed until …' under the purse, no toast."), ECover::Hud, 4, nullptr,
        [](AHomesteadController& PC)
        {
            PC.Sim.SetEnergy(55.0);
            if (PC.ChooseOnHotbar(Item::Pasty)) PC.EatFromHotbar(Item::Pasty);
            PC.ToastRemaining = 0;
        }, 2.4f);
    Add(TEXT("hud-season-warning"), TEXT("Late in the season: '3 days left' in gold beside the date."), ECover::Hud, 5, nullptr,
        [](AHomesteadController& PC) { for (int32 Day = 0; Day < 25; ++Day) PC.Sim.SkipToHourOfDay(10.0); });
    Add(TEXT("hud-hotbar-tool"), TEXT("The hotbar with the hoe selected and empty cells."), ECover::Hud, 6, nullptr,
        [](AHomesteadController& PC) { PC.ChooseOnHotbar(Item::DiggingStick); });
    Add(TEXT("hud-pickups"), TEXT("Parchment pickup slip in the common HUD stack: +3 Berries, +2 Branch, +1 Stone."),
        ECover::Notice, static_cast<int32>(ENotice::PickupLine), nullptr, [](AHomesteadController& PC)
        {
            PC.bPickupsPrimed = true;
            PC.Pickups = {{Item::Berries, 3, 0.0f}, {Item::Branch, 2, 0.0f}, {Item::Stone, 1, 0.0f}};
        }, 0.5f);
    Add(TEXT("hud-controls-strip"), TEXT("The first-minute controls strip at the top left (keyboard or controller words)."),
        ECover::Notice, static_cast<int32>(ENotice::ControlsStrip), nullptr, [](AHomesteadController& PC) { PC.ControlsHint.Restart(); });

    // Interaction hints over what's in front of her.
    Add(TEXT("focus-gather"), TEXT("Facing a berry bush: the parchment focus card with the gather key."),
        ECover::Notice, static_cast<int32>(ENotice::FocusCard), NearResource(Homestead::ResourceKind::BerryBush, 140.0), nullptr);
    Add(TEXT("focus-chop"), TEXT("Facing a forest tree without an axe: the card's unkeyed 'Requires an axe'."),
        ECover::Focus, 0, NearResource(Homestead::ResourceKind::ForestTree, 220.0), nullptr);
    Add(TEXT("focus-chop-axe"), TEXT("Facing a forest tree with the axe in hand: the keyed fell hint."),
        ECover::Focus, 1, NearResource(Homestead::ResourceKind::ForestTree, 220.0),
        [](AHomesteadController& PC) { PC.Sim.GrantItems(Item::Hatchet, 1); PC.ChooseOnHotbar(Item::Hatchet); });
    Add(TEXT("focus-plant"), TEXT("A tilled square ahead with seed chosen: the green outline and the keyed 'Sow' hint."),
        ECover::Focus, 2, Garden, [TillAhead](AHomesteadController& PC) { TillAhead(PC); PC.ChooseOnHotbar(Item::Seeds); });
    List.Last().Pending = TEXT("Water's seed outline and 'Plant <seed>' cue (jennifergalley-seed-outline @6408cdd7)");
    // The door only offers a hint while the store is shut (open, it simply opens): after closing time.
    Add(TEXT("focus-door"), TEXT("At the general store's door after closing (7 PM): 'General store | Closed' and the wait hint."), ECover::Focus, 3,
        [](AHomesteadController& PC, FStage& Out, FString& Why)
        {
            const auto* Door = PC.Sim.Layout().FindLandmark(Homestead::Anchor::GeneralStoreDoor);
            if (!Door) { Why = TEXT("No general store door."); return false; }
            PC.Sim.SkipToHourOfDay(19.0);
            Out.bMove = true;
            Out.Stand = Ahead(Door->position, Door->yaw, -60.0);
            Out.Face = Ahead(Door->position, Door->yaw, 300.0);
            return true;
        }, nullptr);
    Add(TEXT("focus-door-sunday"), TEXT("The general store's door at 10:00 on a Sunday: 'Closed today (Sunday) - opens Monday at 8 AM', no wait key, the CLOSED board, no shopkeeper."),
        ECover::Focus, 15, [](AHomesteadController& PC, FStage& Out, FString& Why)
        {
            const auto* Door = PC.Sim.Layout().FindLandmark(Homestead::Anchor::GeneralStoreDoor);
            if (!Door) { Why = TEXT("No general store door."); return false; }
            for (int32 Step = 0; Step < 7 && PC.Sim.Today().weekday != Homestead::Weekday::Sunday; ++Step) PC.Sim.SkipToHourOfDay(10.0);
            Out.bMove = true;
            Out.Stand = Ahead(Door->position, Door->yaw, -60.0);
            Out.Face = Ahead(Door->position, Door->yaw, 300.0);
            return true;
        }, nullptr);
    List.Last().Pending = TEXT("Water's Sunday closing (jennifergalley-sunday-closing @077a7a3c / water-slot-1001)");
    List.Last().bKeepWorld = true;
    Add(TEXT("focus-shopkeeper"), TEXT("At the counter facing the shopkeeper: his name and the keyed talk/trade hint."),
        ECover::Focus, 4, Counter, nullptr);
    Add(TEXT("focus-sign"), TEXT("Facing the Gateway road sign: 'Road sign | Town / Manor' and the keyed 'Choose a way'."),
        ECover::Focus, 5, Sign(TEXT("GatewayRoadSign"), 180.0), nullptr);
    Add(TEXT("focus-chest"), TEXT("A long chest name leads the parchment card; Open stays readable below, with refusal and gains stacked clear."),
        ECover::Focus, 6, Chest, [ChestId](AHomesteadController& PC)
        {
            const auto Rename = PC.Sim.RenameChest(ChestId(PC), "Wide Winter Wool Storage", PC.PlayerPoint(), PC.Sim.GetRevision());
            if (!Rename) { PC.Notify(Rename); return; }
            PC.Notify(TEXT("Too tired"), true);
            PC.bPickupsPrimed = true;
            PC.Pickups = {{Item::Berries, 3, 0.0f}, {Item::Branch, 2, 0.0f}};
        }, 0.5f);
    Add(TEXT("focus-eat"), TEXT("Food chosen on the hotbar with nothing in front of her: the keyed 'Eat' hint."), ECover::Focus, 7, nullptr,
        [](AHomesteadController& PC) { PC.Sim.SetEnergy(55.0); PC.ChooseOnHotbar(Item::Pasty); });
    Add(TEXT("garden-hoe"), TEXT("Hoe in hand over open meadow: the green till outline and the keyed 'Till' hint."),
        ECover::Focus, 8, Garden, [](AHomesteadController& PC) { PC.ChooseOnHotbar(Item::DiggingStick); });
    Add(TEXT("garden-hoe-refused"), TEXT("Hoe in hand on the road: the red outline and why she can't till here."),
        ECover::Focus, 9, Sign(TEXT("GatewayRoadSign"), 420.0), [](AHomesteadController& PC) { PC.ChooseOnHotbar(Item::DiggingStick); });
    Add(TEXT("garden-pail"), TEXT("The pail over a sown square: the watering outline and hint (an empty pail says so)."),
        ECover::Focus, 10, Garden, [TillAhead](AHomesteadController& PC)
        {
            const int32 Plot = TillAhead(PC);
            if (Plot >= 0) PC.Sim.Plant(Plot, PC.PlayerPoint());
            PC.ChooseOnHotbar(Item::WateringCan);
        });
    // Every action on the card (Jenny 2026-09-30): a ripe, weedy plot offers "[E] Harvest" and "[F] Pull
    // weeds"; a dry, weedy growing one "[F] Pull weeds" and, with the pail in hand, its tool-button Water.
    const auto WeedyPlot = [TillAhead](AHomesteadController& PC, double Growth)
    {
        const int32 Plot = TillAhead(PC);
        if (Plot >= 0) PC.Sim.Plant(Plot, PC.PlayerPoint());
        PC.Sim.PassDaysForPlaytest(3.0, false, PC.PlayerPoint());
        PC.Sim.SetCropGrowthForPlaytest(Growth);
        PC.Sim.SkipToHourOfDay(10.0);
    };
    Add(TEXT("focus-plot-ripe-weedy"), TEXT("A ripe, weedy plot: '[E] Harvest' and '[F] Pull weeds' side by side."),
        ECover::Focus, 16, Garden, [WeedyPlot](AHomesteadController& PC) { WeedyPlot(PC, 1.0); });
    Add(TEXT("focus-plot-water-weed"), TEXT("A dry, weedy growing plot with the pail in hand: '[F] Pull weeds' and '[LMB] Water'."),
        ECover::Focus, 17, Garden, [WeedyPlot](AHomesteadController& PC)
        {
            WeedyPlot(PC, 0.4);
            PC.Sim.GrantItems(Item::WateringCan, 1);
            PC.Sim.GrantItems(Item::Water, Homestead::PailPortions);
            PC.ChooseOnHotbar(Item::WateringCan);
        });

    // Seed outlines and the sowing cue (Water's seed-outline, jennifergalley-seed-outline @6408cdd7:
    // Simulation::CheckSow, PreviewGarden(Seed), DescribeSow). Until it is on this line these show the
    // older cue and no seed outline, and the run reports them as pending.
    const TCHAR* const SeedOutline = TEXT("Water's seed outline (jennifergalley-seed-outline @6408cdd7)");
    const auto SowSeed = [](AHomesteadController& PC, bool bSelect)
    {
        if (PC.Sim.Count(Item::CarrotSeed) < 4) PC.Sim.GrantItems(Item::CarrotSeed, 4 - PC.Sim.Count(Item::CarrotSeed));
        PC.ChooseOnHotbar(Item::CarrotSeed);
        if (!bSelect)
        {
            const int32 Empty = PC.FirstEmptyHotbarCell();
            if (Empty != INDEX_NONE) PC.SelectHotbarSlot(Empty);
        }
    };
    Add(TEXT("garden-seed-plant"), TEXT("Carrot seed chosen over a tilled square: the green outline and persistent '[E] Plant Seeds'."),
        ECover::Focus, 11, Garden, [TillAhead, SowSeed](AHomesteadController& PC) { TillAhead(PC); SowSeed(PC, true); });
    List.Last().Pending = SeedOutline;
    Add(TEXT("garden-seed-occupied"), TEXT("Carrot seed over a square already sown: the red outline and 'A crop is already growing here.'"),
        ECover::Focus, 12, Garden, [TillAhead, SowSeed](AHomesteadController& PC)
        {
            const int32 Plot = TillAhead(PC);
            SowSeed(PC, true);
            if (Plot >= 0) PC.Sim.Plant(Plot, PC.PlayerPoint(), Homestead::CropKind::Carrots);
        });
    List.Last().Pending = SeedOutline;
    Add(TEXT("garden-seed-select"), TEXT("A tilled square with carrot seed in the hotbar but not chosen: 'Select Carrot seed (4) to plant'."),
        ECover::Focus, 13, Garden, [TillAhead, SowSeed](AHomesteadController& PC) { TillAhead(PC); SowSeed(PC, false); });
    List.Last().Pending = SeedOutline;
    Add(TEXT("garden-seed-untilled"), TEXT("Carrot seed chosen over untilled meadow: the red outline and 'Till this square before sowing.'"),
        ECover::Focus, 14, Garden, [SowSeed](AHomesteadController& PC) { SowSeed(PC, true); });
    List.Last().Pending = SeedOutline;

    // Night and rain: the same HUD and notices under the night sky and in the rain.
    const auto FishingStage = [](bool Landing)
    {
        return [Landing](AHomesteadController& PC)
        {
            PC.Sim.GrantItems(Item::FishingPole, 1);
            PC.ChooseOnHotbar(Item::FishingPole);
            PC.Sim.SetFishingWaterProbe([](Point) { return Homestead::FishingWater::River; });
            PC.Sim.SetFishingEscapeChance(0.0);
            const auto Cast = PC.Sim.BeginFishing(PC.PlayerPoint());
            PC.PrepareEstateSimulation(PC.Sim);
            if (!Cast) { PC.Notify(Cast); return; }
            PC.Sim.AdvanceFishing(Homestead::Fishing::CastSplashSeconds, PC.PlayerPoint());
            PC.Sim.FishingAnimationContact(Homestead::FishingContact::CastSplash, PC.Sim.FishingCast().token, PC.PlayerPoint());
            PC.Sim.AdvanceFishing(PC.Sim.FishingCast().biteAfter, PC.PlayerPoint());
            if (!Landing) return;
            PC.Sim.FishingPress(PC.PlayerPoint());
            PC.Sim.AdvanceFishing(PC.Sim.FishingCast().strikeAfter, PC.PlayerPoint());
        };
    };
    Add(TEXT("hud-fishing-bite"), TEXT("Staged fishing bite: the float pulled under, with rings closing on it."),
        ECover::Hud, 16, nullptr, FishingStage(false), 0.4f);
    Add(TEXT("hud-fishing-landing"), TEXT("Staged fishing strike: the hooked fish pulls the float under again, rings closing."),
        ECover::Hud, 17, nullptr, FishingStage(true), 0.3f);
    Add(TEXT("hud-night"), TEXT("The world HUD at 10:30 PM: the moon in the calendar, the night-lit world."), ECover::Hud, 7, nullptr, Night, 1.5f);
    Add(TEXT("hud-rain"), TEXT("The world HUD in the rain: the rain cloud in the calendar, rain falling."), ECover::Hud, 8, nullptr, Rain, 1.5f);
    Add(TEXT("toast-night"), TEXT("A world notice at night: the parchment slip over the dark scene."), ECover::Hud, 9, nullptr,
        [Night](AHomesteadController& PC) { Night(PC); PC.Notify(TEXT("Summer has come"), false); }, 1.5f);
    Add(TEXT("toast-rain"), TEXT("A world error notice in the rain."), ECover::Hud, 10, nullptr,
        [Rain](AHomesteadController& PC) { Rain(PC); PC.Notify(TEXT("Not ready yet"), true); }, 1.5f);

    // The new-game setup: the Names step on its own, then the whole flow (it takes over the screen).
    Add(TEXT("setup-names"), TEXT("New-game Names step: her name, family and estate fields with the suggestions."), ECover::Setup, 1, nullptr,
        [](AHomesteadController& PC) { PC.bNewGameSetup = true; PC.ShowNames(); }, 1.2f);
    Add(TEXT("setup-new-game"), TEXT("New-game setup from the start (last in a run): the Appearance step before Names."), ECover::Setup, 0, nullptr,
        [](AHomesteadController& PC)
        {
            // Shown even where agents skip it (-HomesteadSkipNewGameSetup): the gallery is for looking at it.
            IConsoleVariable* Skip = IConsoleManager::Get().FindConsoleVariable(TEXT("homestead.SkipNewGameSetup"));
            // At console priority, so it holds even after `homestead.SkipNewGameSetup 1` was typed.
            const int32 Was = Skip ? Skip->GetInt() : -1;
            if (Skip) Skip->Set(0, ECVF_SetByConsole);
            PC.BeginNewGameSetup();
            if (Skip) Skip->Set(Was, ECVF_SetByConsole);
        }, 1.5f);
    // The garden outlines are world geometry, and night and rain are about the world's look: these keep
    // the world on the plain backdrop.
    for (FEntry& Entry : List)
        Entry.bKeepWorld = Entry.bKeepWorld || Entry.Id.StartsWith(TEXT("garden-")) || Entry.Id == TEXT("focus-plant")
            || Entry.Id.EndsWith(TEXT("-night")) || Entry.Id.EndsWith(TEXT("-rain"));
    return List;
}

const FHomesteadUIGallery::FEntry* FHomesteadUIGallery::Find(const FString& Id)
{
    return Entries().FindByPredicate([&Id](const FEntry& Entry) { return Entry.Id.Equals(Id, ESearchCase::IgnoreCase); });
}

TArray<FString> FHomesteadUIGallery::Resolve(const FString& Spec, FString& Error)
{
    TArray<FString> Ids;
    if (Spec.IsEmpty() || Spec.Equals(TEXT("all"), ESearchCase::IgnoreCase))
    {
        for (const FEntry& Entry : Entries()) Ids.Add(Entry.Id);
        return Ids;
    }
    TArray<FString> Parts;
    Spec.ParseIntoArray(Parts, TEXT(","), true);
    for (FString Part : Parts)
    {
        Part.TrimStartAndEndInline();
        if (const FEntry* Entry = Find(Part)) Ids.Add(Entry->Id);
        else Error += (Error.IsEmpty() ? TEXT("Unknown gallery ids: ") : TEXT(", ")) + Part;
    }
    return Ids;
}

TArray<FString> FHomesteadUIGallery::MissingCoverage(const AHomesteadController&)
{
    TSet<int32> Pages, Tabs, Notices;
    for (const FEntry& Entry : Entries())
    {
        if (Entry.Cover == ECover::BookPage) Pages.Add(Entry.Key);
        else if (Entry.Cover == ECover::SettingsTab) Tabs.Add(Entry.Key);
        else if (Entry.Cover == ECover::Notice) Notices.Add(Entry.Key);
    }
    // Every page the tab bar cycles through, and the two the book opens without a tab: Settings
    // (Start / Esc) and Credits (from Settings).
    constexpr int32 SettingsPage = 4, CreditsPage = 5;
    TArray<int32> Needed(HomesteadMenus::SHomesteadMenu::TabPages());
    Needed.AddUnique(SettingsPage);
    Needed.AddUnique(CreditsPage);
    TArray<FString> Missing;
    for (const int32 Page : Needed)
        if (!Pages.Contains(Page)) Missing.Add(FString::Printf(TEXT("book-page-%d"), Page));
    for (int32 Tab = 0; Tab < HomesteadMenus::SHomesteadMenu::SettingsTabCount; ++Tab)
        if (!Tabs.Contains(Tab)) Missing.Add(FString::Printf(TEXT("settings-tab-%d"), Tab));
    for (int32 Style = 0; Style < static_cast<int32>(ENotice::Count); ++Style)
        if (!Notices.Contains(Style)) Missing.Add(FString::Printf(TEXT("notice-%d"), Style));
    return Missing;
}

FString FHomesteadUIGallery::RefusalFor(const AHomesteadController& PC)
{
    if (PC.SaveRoute.Mode == TEXT("test-sandbox") || PC.SaveRoute.Mode == TEXT("preview")) return FString();
    return FString::Printf(TEXT("The UI gallery changes the game it runs in, and this game saves to %s (%s). Run it with ")
        TEXT("-HomesteadSmokeTest (Capture-UiGallery.ps1), or start the editor with -HomesteadPreviewProfile=<id> ")
        TEXT("(Start-EditorMcp.ps1 -PreviewProfile gallery)."), *PC.SaveRoute.Directory, *PC.SaveRoute.Mode);
}

void FHomesteadUIGallery::Show(AHomesteadController& PC, const FString& Id, bool bPad,
    TFunction<void(bool, const FString&)> OnReady)
{
    using namespace HomesteadUIGalleryFixture;
    if (const FString Refusal = RefusalFor(PC); !Refusal.IsEmpty()) { OnReady(false, Refusal); return; }
    const FEntry* Entry = Find(Id);
    if (!Entry) { OnReady(false, TEXT("No gallery entry ") + Id); return; }
    if (Pending.IsValid()) { FTSTicker::GetCoreTicker().RemoveTicker(Pending); Pending.Reset(); }
    Prepare(PC, bPad);
    FStage Where;
    FString Problem;
    if (Entry->Stage && !Entry->Stage(PC, Where, Problem)) { OnReady(false, Problem); return; }
    if (!Where.bMove) { Where.Stand = {StageX, StageY}; Where.Face = {StageX + 500.0, StageY}; }
    if (Distance(PC.PlayerPoint(), Where.Stand) > SameSpotCm) PC.HomesteadTeleport(Where.Stand.x, Where.Stand.y);
    UE_LOG(LogHomesteadUIGallery, Display, TEXT("UI gallery: %s (%s)"), *Entry->Id, bPad ? TEXT("controller") : TEXT("keyboard and mouse"));
    TWeakObjectPtr<AHomesteadController> Weak(&PC);
    const double Began = FPlatformTime::Seconds();
    double AppliedAt = -1.0;
    Pending = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [Weak, Entry, Where, OnReady, Began, AppliedAt](float) mutable -> bool
        {
            AHomesteadController* Controller = Weak.Get();
            if (!Controller) { Pending.Reset(); OnReady(false, TEXT("The game ended.")); return false; }
            const double Now = FPlatformTime::Seconds();
            if (AppliedAt < 0.0)
            {
                const bool bArrived = !Controller->bPendingGroundSnap && !Controller->bPendingSpawn
                    && Distance(Controller->PlayerPoint(), Where.Stand) < ArrivedWithinCm;
                if (!bArrived)
                {
                    if (Now - Began < ArriveSeconds) return true;
                    Pending.Reset();
                    OnReady(false, TEXT("She did not settle at the gallery spot."));
                    return false;
                }
                Face(*Controller, Where.Face);
                if (Entry->Apply) Entry->Apply(*Controller);
                AppliedAt = Now;
                return true;
            }
            if (Now - AppliedAt < Entry->Settle) return true;
            Pending.Reset();
            OnReady(true, FString());
            return false;
        }));
}

namespace HomesteadUIGalleryCommand
{
void Run(const TArray<FString>& Args, UWorld* World)
{
    using namespace HomesteadUIGalleryFixture;
    auto* PC = World ? Cast<AHomesteadController>(World->GetFirstPlayerController()) : nullptr;
    const FString Verb = Args.IsEmpty() ? FString(TEXT("list")) : Args[0];
    const auto& Entries = FHomesteadUIGallery::Entries();
    if (Verb.Equals(TEXT("backdrop"), ESearchCase::IgnoreCase))
    {
        const bool bPlain = Args.Num() < 2 || !Args[1].Equals(TEXT("world"), ESearchCase::IgnoreCase);
        bLivePlain = bPlain;
        if (PC) FHomesteadUIGallery::SetBackdrop(*PC, bPlain, true);
        UE_LOG(LogHomesteadUIGallery, Display, TEXT("UI gallery backdrop: %s"), bPlain ? TEXT("plain") : TEXT("world"));
        return;
    }
    if (Verb.Equals(TEXT("list"), ESearchCase::IgnoreCase))
    {
        for (const auto& Entry : Entries) UE_LOG(LogHomesteadUIGallery, Display, TEXT("%-24s %s"), *Entry.Id, *Entry.Description);
        return;
    }
    if (!PC) { UE_LOG(LogHomesteadUIGallery, Warning, TEXT("UI gallery: start the game (PIE) first.")); return; }
    if (const FString Refusal = FHomesteadUIGallery::RefusalFor(*PC); !Refusal.IsEmpty())
    {
        UE_LOG(LogHomesteadUIGallery, Warning, TEXT("UI_GALLERY_REFUSED %s"), *Refusal);
        return;
    }
    const bool bPad = Args.Num() > 1 ? Args[1].Equals(TEXT("Pad"), ESearchCase::IgnoreCase) : PC->UsesGamepad();
    TWeakObjectPtr<AHomesteadController> WeakPC(PC);
    const auto Report = [WeakPC](const FString& Id)
    {
        return [Id, WeakPC](bool bOk, const FString& Why)
        {
            const FHomesteadUIGallery::FEntry* Entry = FHomesteadUIGallery::Find(Id);
            if (bOk && WeakPC.IsValid())
                FHomesteadUIGallery::SetBackdrop(*WeakPC.Get(), bLivePlain && !(Entry && Entry->bKeepWorld), true);
            if (bOk) { UE_LOG(LogHomesteadUIGallery, Display, TEXT("UI_GALLERY_READY %s"), *Id); }
            else { UE_LOG(LogHomesteadUIGallery, Warning, TEXT("UI_GALLERY_SKIPPED %s: %s"), *Id, *Why); }
        };
    };
    if (Verb.Equals(TEXT("all"), ESearchCase::IgnoreCase))
    {
        // Walks every entry, holding each on screen a few seconds (for looking, not capturing).
        TWeakObjectPtr<AHomesteadController> Weak(PC);
        TSharedRef<TFunction<void(int32)>> Step = MakeShared<TFunction<void(int32)>>();
        *Step = [Weak, bPad, Step](int32 Index)
        {
            AHomesteadController* Controller = Weak.Get();
            const auto& All = FHomesteadUIGallery::Entries();
            if (!Controller || !All.IsValidIndex(Index)) return;
            Cursor = Index;
            FHomesteadUIGallery::Show(*Controller, All[Index].Id, bPad, [Step, Index](bool, const FString&)
            {
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Step, Index](float) { (*Step)(Index + 1); return false; }), 2.5f);
            });
        };
        (*Step)(0);
        return;
    }
    FString Id = Verb;
    if (Verb.Equals(TEXT("next"), ESearchCase::IgnoreCase) || Verb.Equals(TEXT("prev"), ESearchCase::IgnoreCase))
    {
        const int32 Direction = Verb.Equals(TEXT("next"), ESearchCase::IgnoreCase) ? 1 : -1;
        Cursor = (Cursor == INDEX_NONE ? (Direction > 0 ? 0 : Entries.Num() - 1) : (Cursor + Direction + Entries.Num()) % Entries.Num());
        Id = Entries[Cursor].Id;
    }
    else if (const auto* Entry = FHomesteadUIGallery::Find(Id)) Cursor = static_cast<int32>(Entry - Entries.GetData());
    FHomesteadUIGallery::Show(*PC, Id, bPad, Report(Id));
}

FAutoConsoleCommandWithWorldAndArgs Command(TEXT("homestead.UIGallery"),
    TEXT("UI gallery (Development): list | <id> | next | prev | all, then optional Pad or KBM; backdrop plain|world. Puts one UI surface on screen from an isolated fixture."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Run));
}
#endif
