// The town shops in the game (Pascoe's general store and Tregear's, the seedsman): their buildings and
// shopkeeper actors, the interaction focus, the shop screen and the wallet readout. Trading itself is
// Simulation::Sell / Simulation::Buy.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "HomesteadGeneralStore.h"
#include "HomesteadShopkeeper.h"
#include "UI/SHomesteadShop.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
FString ShopText(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
}

FString AHomesteadController::GreetingFor(const Homestead::Shop& Shop) const
{
    const FString Estate = EstateName();
    const double Hour = FMath::Fmod(State().hour, 24.0);
    const int32 Pick = Shop.greetings % 2;
    if (Shop.kind == Homestead::ShopKind::Seedsman)
    {
        if (Shop.greetings == 0)
            return FString::Printf(TEXT("Jago Tregear. So you're the one taken on %s. You'll be wanting seed, then. "
                "Whatever the season will grow, I've a drawer of it, and come harvest I'll give you a fair price for "
                "your wheat and barley."), *Estate);
        if (Hour < 12.0)
            return Pick ? TEXT("Early bird. The ground's warming nicely, if you've a mind to sow.")
                : TEXT("Morning. Mind the sacks, I've only just had the corn in.");
        if (Hour < 15.0)
            return Pick ? FString::Printf(TEXT("How's the ground up at %s? Turning over nicely, I hope."), *Estate)
                : TEXT("Afternoon. Seed's weighed out fresh, same as always.");
        return Pick ? TEXT("Nearly shutting, but I'll not send you home empty-handed.")
            : TEXT("Evening. Quick now, the scale's still out.");
    }
    if (Shop.greetings == 0)
        return FString::Printf(TEXT("Well now, you'll be the new lady up at %s! Martha Pascoe. Word travels quick in a town "
            "this size. If it's sold in Cornwall I've likely a shelf of it, and anything you bring down from the estate, "
            "set it on the counter and I'll give you a fair price."), *Estate);
    if (Hour < 12.0)
        return Pick ? TEXT("You're up with the lark. What can I do for you this morning?")
            : TEXT("Morning, my 'andsome. Kettle's only just boiled. What'll it be?");
    if (Hour < 16.0)
        return Pick ? FString::Printf(TEXT("Back again? Let's see what you've brought me from %s."), *Estate)
            : TEXT("Afternoon! Mind the step, it's been loose since Lady Day.");
    return Pick ? TEXT("Evening, dear. The pasties went quick today, but I kept a few back for you.")
        : TEXT("Nearly shutting up, but I've always time for you. Quick now.");
}

void AHomesteadController::OpenShopScreen(int32 ShopId, bool bGreet)
{
    const Homestead::Shop* Shop = Sim.FindShop(ShopId);
    if (!Shop || !GEngine || !GEngine->GameViewport) return;
    if (!Homestead::IsShopOpen(*Shop, State().hour))
    {
        Notify(ShopText(Homestead::ClosedMessage(*Shop, State().hour)), true);
        return;
    }
    if (bBookOpen) CloseBook();
    EndPlacement();
    if (ShopScreen.IsValid()) CloseShopScreen();
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->CancelSprint();
        Avatar->GetCharacterMovement()->StopMovementImmediately();
    }
    const FString Greeting = bGreet ? GreetingFor(*Shop) : FString();
    if (bGreet) Sim.GreetShopkeeper(ShopId);
    FlushPressedKeys();
    ShopScreen = SNew(HomesteadMenus::SHomesteadShop).Controller(this).ShopId(ShopId).Greeting(Greeting);
    GEngine->GameViewport->AddViewportWidgetContent(ShopScreen.ToSharedRef(), 110);
    bShowMouseCursor = !bGamepad;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(ShopScreen);
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    PlayEffect(UIClick, 0.08f);
    UE_LOG(LogTemp, Display, TEXT("SHOP_OPEN shop=%d greeted=%d purse=%lld"), ShopId, bGreet ? 1 : 0,
        static_cast<long long>(State().money));
}

void AHomesteadController::CloseShopScreen()
{
    if (!ShopScreen.IsValid()) return;
    if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(ShopScreen.ToSharedRef());
    ShopScreen.Reset();
    FlushPressedKeys();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
    PlayEffect(UIClick, 0.08f);
}

Homestead::Result AHomesteadController::ShopTrade(int32 ShopId, Homestead::Item Item, int32 Quantity, bool bSell,
    bool bHeroineStock)
{
    const int64 Before = State().money;
    const auto Result = bSell ? Sim.Sell(ShopId, Item, Quantity, PlayerPoint())
        : Sim.Buy(ShopId, Item, Quantity, bHeroineStock, PlayerPoint());
    UE_LOG(LogTemp, Display, TEXT("SHOP_TRADE %s item=%s qty=%d ok=%d purse=%lld message=\"%s\""), bSell ? TEXT("sell") : TEXT("buy"),
        UTF8_TO_TCHAR(Homestead::ItemKey(Item)), Quantity, Result.ok ? 1 : 0, static_cast<long long>(State().money),
        *ShopText(Result.message));
    if (Result.ok)
    {
        LastWalletDelta = State().money - Before;
        WalletDeltaRemaining = 3.0f;
        PlayEffect(bSell ? WoodTapA : WoodTapB, 0.35f);
        if (!bSell) PinNewSeed(Item);
        if (!bSell && Item == Homestead::Item::TinWateringCan && !IsPinnedToHotbar(Item))
        {
            // The can replaces the pail on the hotbar (it's what she fills and pours from now).
            const int32 Pail = HotbarSlots.IndexOfByKey(static_cast<int32>(Homestead::Item::WateringCan));
            if (Pail != INDEX_NONE) HotbarSlots[Pail] = static_cast<int32>(Item);
            else TogglePinnedToHotbar(Item);
        }
    }
    return Result;
}

void AHomesteadController::ShopClick() { PlayEffect(UIClick, 0.05f); }

void AHomesteadController::NoteShopDevice(bool bPad)
{
    if (bPad != bGamepad) ++PromptDeviceChanges;
    bGamepad = bPad;
    bShowMouseCursor = !bPad;
}

void AHomesteadController::SyncStores()
{
    const auto& Shops = State().shops;
    for (int32 Index = Stores.Num() - 1; Index >= 0; --Index)
    {
        AHomesteadGeneralStore* Store = Stores[Index];
        const Homestead::Shop* Shop = Store ? Sim.FindShop(Store->GetShopId()) : nullptr;
        const bool bCurrent = Shop && FVector2D::DistSquared(Store->CounterPoint(), FVector2D(Shop->counterX, Shop->counterY)) < 1.0
            && FMath::IsNearlyEqual(Store->GetCounterYaw(), static_cast<float>(Shop->counterYaw), 0.01f);
        if (bCurrent) continue;
        if (Store) Store->Destroy();
        Stores.RemoveAt(Index);
    }
    for (const auto& Shop : Shops)
    {
        if (Stores.ContainsByPredicate([&](const AHomesteadGeneralStore* Store) { return Store && Store->GetShopId() == Shop.id; }))
            continue;
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Store = GetWorld()->SpawnActor<AHomesteadGeneralStore>(Parameters);
        if (!Store) continue;
        Store->Build(Shop.id, Shop.kind, FVector2D(Shop.counterX, Shop.counterY), static_cast<float>(Shop.counterYaw),
            [this](float X, float Y) { return GroundHeight(X, Y); }, ShopText(Homestead::ClosedSign(Shop, State().hour)));
        Stores.Add(Store);
        UE_LOG(LogTemp, Display, TEXT("STORE_BUILT shop=%d counter=(%.0f, %.0f) yaw=%.0f"), Shop.id, Shop.counterX, Shop.counterY,
            Shop.counterYaw);
    }
}

void AHomesteadController::TickStores(float DeltaSeconds)
{
    WalletDeltaRemaining = FMath::Max(0.0f, WalletDeltaRemaining - DeltaSeconds);
    if (Stores.Num() != static_cast<int32>(State().shops.size())) SyncStores();
    else
        for (const auto& Store : Stores)
        {
            const Homestead::Shop* Shop = Store ? Sim.FindShop(Store->GetShopId()) : nullptr;
            if (!Shop || Store->CounterPoint() != FVector2D(Shop->counterX, Shop->counterY)) { SyncStores(); break; }
        }
    const FVector Heroine = GetPawn() ? GetPawn()->GetActorLocation() : FVector(1e9);
    for (const auto& Store : Stores)
        if (const Homestead::Shop* Shop = Store ? Sim.FindShop(Store->GetShopId()) : nullptr)
        {
            Store->SetOpen(Homestead::IsShopOpen(*Shop, State().hour), Heroine);
            Store->SetClosedText(ShopText(Homestead::ClosedSign(*Shop, State().hour)));
        }
}

void AHomesteadController::ConsiderStoreFocus(TFunctionRef<void(EFocus, int32, Homestead::Point)> Consider) const
{
    for (const auto& Store : Stores)
    {
        if (!Store) continue;
        const Homestead::Shop* Shop = Sim.FindShop(Store->GetShopId());
        if (!Shop) continue;
        if (Homestead::IsShopOpen(*Shop, State().hour))
        {
            const FVector At = Store->ShopkeeperLocation();
            Consider(EFocus::Shopkeeper, Shop->id, {At.X, At.Y});
        }
        else if (!Store->IsDoorOpen())
        {
            const FVector2D Door = Store->DoorPoint();
            Consider(EFocus::StoreDoor, Shop->id, {Door.X, Door.Y});
        }
    }
}

FString AHomesteadController::StoreFocusTitle() const
{
    const Homestead::Shop* Shop = Sim.FindShop(FocusId);
    if (!Shop) return TEXT("General store");
    const FString Name = Shop->kind == Homestead::ShopKind::Seedsman ? TEXT("Tregear's, seedsman") : TEXT("General store");
    if (Focus == EFocus::StoreDoor) return Name + TEXT("  |  Closed");
    return ShopText(Homestead::ShopkeeperName(Shop->kind)) + TEXT("  |  ") + Name;
}

FString AHomesteadController::StoreFocusActions() const
{
    const Homestead::Shop* Shop = Sim.FindShop(FocusId);
    if (!Shop) return FString();
    if (Focus == EFocus::StoreDoor) return ShopText(Homestead::ClosedMessage(*Shop, State().hour));
    return (bGamepad ? TEXT("[A]") : TEXT("[E]")) + FString(TEXT(" Talk to ")) + ShopText(Homestead::ShopkeeperName(Shop->kind));
}

void AHomesteadController::InteractWithStore()
{
    const Homestead::Shop* Shop = Sim.FindShop(FocusId);
    if (!Shop) return;
    if (Focus == EFocus::StoreDoor || !Homestead::IsShopOpen(*Shop, State().hour))
    {
        Notify(ShopText(Homestead::ClosedMessage(*Shop, State().hour)), true);
        return;
    }
    OpenShopScreen(Shop->id);
}

void AHomesteadController::HomesteadOpenStore()
{
    const APawn* Heroine = GetPawn();
    if (!Heroine) return;
    const float Yaw = Heroine->GetActorRotation().Yaw;
    const FVector Forward = FRotator(0, Yaw, 0).Vector();
    const FVector Door = Heroine->GetActorLocation() + Forward * 250.0f;
    const FVector Counter = Door + Forward * AHomesteadGeneralStore::DoorToCounter;
    const bool bFirst = State().shops.empty();
    const auto Result = Sim.PlaceShop(Homestead::ShopKind::GeneralStore, {Counter.X, Counter.Y}, FRotator::NormalizeAxis(Yaw + 180.0f));
    if (Result.ok && bFirst && State().money == 0) Sim.GrantMoney(Homestead::StartingMoney);
    SyncStores();
    Notify(Result.ok ? FString::Printf(TEXT("The general store stands ahead of you. Purse: %s."),
        *ShopText(Homestead::FormatMoney(State().money))) : ShopText(Result.message), !Result.ok);
}

void AHomesteadController::HomesteadMoney(int32 Cents)
{
    const int64 Before = State().money;
    const auto Result = Sim.GrantMoney(Cents);
    if (Result.ok) { LastWalletDelta = State().money - Before; WalletDeltaRemaining = 3.0f; }
    Notify(Result.ok ? FString::Printf(TEXT("Purse: %s"), *ShopText(Homestead::FormatMoney(State().money))) : ShopText(Result.message),
        !Result.ok);
}
