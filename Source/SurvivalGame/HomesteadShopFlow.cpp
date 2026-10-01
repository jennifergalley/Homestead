// The general store in the game: its building and shopkeeper actors, the interaction focus, the
// shop screen and the wallet readout. Trading itself is Simulation::Sell / Simulation::Buy.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "HomesteadGeneralStore.h"
#include "HomesteadShopkeeper.h"
#include "UI/SHomesteadShop.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadShop, Log, All);

namespace
{
FString ShopText(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
}

FString AHomesteadController::GreetingFor(const Homestead::Shop& Shop) const
{
    const FString Estate = EstateName();
    if (Shop.greetings == 0)
        return FString::Printf(TEXT("Well now, you'll be the new lady up at %s! Martha Pascoe. Word travels quick in a town "
            "this size. If it's sold in Cornwall I've likely a shelf of it, and anything you bring down from the estate, "
            "set it on the counter and I'll give you a fair price."), *Estate);
    const double Hour = FMath::Fmod(State().hour, 24.0);
    const int32 Pick = Shop.greetings % 2;
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
    if (RejectPendingGroundSnapAction())
        return {false, "Still finding your footing. Wait a moment.", Homestead::ResultCode::Unavailable, Sim.GetRevision()};
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
        // Bought goods land in the first empty hotbar cell, else below it (HomesteadPackRow.h).
    }
    return Result;
}

Homestead::Result AHomesteadController::ShopBuyBackpack(int32 ShopId)
{
    if (RejectPendingGroundSnapAction())
        return {false, "Still finding your footing. Wait a moment.", Homestead::ResultCode::Unavailable, Sim.GetRevision()};
    const int64 Before = State().money;
    const auto Result = Sim.BuyBackpack(ShopId, PlayerPoint());
    UE_LOG(LogTemp, Display, TEXT("SHOP_TRADE upgrade item=leather-backpack ok=%d purse=%lld capacity=%d message=\"%s\""),
        Result.ok ? 1 : 0, static_cast<long long>(State().money), Sim.PackCapacity(), *ShopText(Result.message));
    if (Result.ok)
    {
        LastWalletDelta = State().money - Before;
        WalletDeltaRemaining = 3.0f;
        PlayEffect(WoodTapB, 0.35f);
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
        if (Shop.kind != Homestead::ShopKind::GeneralStore) continue;
        if (Stores.ContainsByPredicate([&](const AHomesteadGeneralStore* Store) { return Store && Store->GetShopId() == Shop.id; }))
            continue;
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Store = GetWorld()->SpawnActor<AHomesteadGeneralStore>(Parameters);
        if (!Store) continue;
        Store->Build(Shop.id, FVector2D(Shop.counterX, Shop.counterY), static_cast<float>(Shop.counterYaw),
            [this](float X, float Y) { return GroundHeight(X, Y); }, ShopText(Homestead::ClosedSignText(Shop, State().hour)));
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
            const bool bOpen = Homestead::IsShopOpen(*Shop, State().hour);
            Store->SetOpen(bOpen, Heroine);
            // The board names the day it reopens (a Sunday, or a Saturday evening); SetText only on a change.
            if (!bOpen) Store->SetClosedText(ShopText(Homestead::ClosedSignText(*Shop, State().hour)));
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
    if (Focus == EFocus::StoreDoor) return TEXT("General store  |  Closed");
    return FString(AHomesteadShopkeeper::DisplayName()) + TEXT("  |  General store");
}

namespace ShopWait
{
// How long the "Wait until ...?" question stays up, and how soon a second press may answer it (so a
// bounced or doubled press can't confirm by accident), in real seconds.
constexpr double AskSeconds = 8.0;
constexpr double MinAnswerSeconds = 0.35;
// "13 h", "1 h 30 min", "45 min".
FString Duration(double Hours)
{
    const int32 Minutes = FMath::Max(1, FMath::RoundToInt(Hours * 60.0));
    if (Minutes < 60) return FString::Printf(TEXT("%d min"), Minutes);
    const int32 Whole = Minutes / 60, Rest = Minutes % 60;
    return Rest ? FString::Printf(TEXT("%d h %d min"), Whole, Rest) : FString::Printf(TEXT("%d h"), Whole);
}
}

bool AHomesteadController::IsShopWaitArmed() const
{
    return WaitShopId != INDEX_NONE && Focus == EFocus::StoreDoor && FocusId == WaitShopId
        && FPlatformTime::Seconds() - WaitAskedAt < ShopWait::AskSeconds;
}

bool AHomesteadController::CancelShopWait()
{
    const bool bArmed = IsShopWaitArmed();
    WaitShopId = INDEX_NONE;
    return bArmed;
}

FString AHomesteadController::StoreFocusActions() const
{
    const Homestead::Shop* Shop = Sim.FindShop(FocusId);
    if (!Shop) return FString();
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    if (Focus == EFocus::StoreDoor)
    {
        // Across a closed day there's no waiting at the door: just when it opens.
        if (!Homestead::CanWaitForShop(*Shop, State().hour)) return ShopText(Homestead::ClosedMessage(*Shop, State().hour));
        const FString Opens = ShopText(Homestead::FormatHour(Shop->openHour));
        const double Wait = Homestead::HoursUntilOpen(*Shop, State().hour);
        // Past midnight is a night out in the street; say so before she agrees.
        const bool bOvernight = FMath::Fmod(State().hour, 24.0) + Wait >= 24.0;
        const FString Hours = ShopWait::Duration(Wait) + (bOvernight ? TEXT(", overnight,") : TEXT(""));
        if (IsShopWaitArmed())
            return FString::Printf(TEXT("Wait %s until %s?   %s Wait   %s Cancel"), *Hours, *Opens, *A,
                bGamepad ? TEXT("[B]") : TEXT("[Esc]"));
        return ShopText(Homestead::ClosedMessage(*Shop, State().hour)) + TEXT("   ") + A + TEXT(" Wait until ") + Opens
            + TEXT(" (") + ShopWait::Duration(Wait) + TEXT(")");
    }
    return A + FString(TEXT(" Talk to ")) + AHomesteadShopkeeper::DisplayName();
}

void AHomesteadController::InteractWithStore()
{
    const Homestead::Shop* Shop = Sim.FindShop(FocusId);
    if (!Shop) return;
    if (Focus == EFocus::StoreDoor || !Homestead::IsShopOpen(*Shop, State().hour))
    {
        if (Focus != EFocus::StoreDoor) { Notify(ShopText(Homestead::ClosedMessage(*Shop, State().hour)), true); return; }
        // Nothing to wait for across a closed day: E says when it opens (WaitForShop's refusal).
        if (!Homestead::CanWaitForShop(*Shop, State().hour))
        {
            WaitShopId = INDEX_NONE;
            Notify(ShopText(Sim.WaitForShop(FocusId, PlayerPoint()).message), true);
            return;
        }
        if (!IsShopWaitArmed())
        {
            // First press asks; the prompt shows the question and how to answer it.
            WaitShopId = FocusId;
            WaitAskedAt = FPlatformTime::Seconds();
            PlayEffect(UIClick, 0.05f);
            return;
        }
        if (FPlatformTime::Seconds() - WaitAskedAt < ShopWait::MinAnswerSeconds) return;
        const int32 ShopId = WaitShopId;
        WaitShopId = INDEX_NONE;
        const auto Result = Sim.WaitForShop(ShopId, PlayerPoint());
        Notify(ShopText(Result.message), !Result.ok);
        if (Result.ok) UE_LOG(LogHomesteadShop, Log, TEXT("Waited for shop %d; now hour %.2f."), ShopId, State().hour);
        return;
    }
    OpenShopScreen(Shop->id);
}

void AHomesteadController::HomesteadOpenStore()
{
    if (RejectPendingGroundSnapAction()) return;
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

void AHomesteadController::HomesteadMoney(int32 Coins)
{
    if (RejectPendingGroundSnapAction()) return;
    const int64 Before = State().money;
    const auto Result = Sim.GrantMoney(Coins);
    if (Result.ok) { LastWalletDelta = State().money - Before; WalletDeltaRemaining = 3.0f; }
    Notify(Result.ok ? FString::Printf(TEXT("Purse: %s"), *ShopText(Homestead::FormatMoney(State().money))) : ShopText(Result.message),
        !Result.ok);
}
