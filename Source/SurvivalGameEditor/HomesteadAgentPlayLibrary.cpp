#include "HomesteadAgentPlayLibrary.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HomesteadController.h"
#include "HomesteadLab.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    struct FHeldKey
    {
        FKey Key;
        float Remaining = 0.f;
    };

    struct FAgentInputState
    {
        FVector2D Move = FVector2D::ZeroVector;
        FVector2D Look = FVector2D::ZeroVector;
        float StickRemaining = 0.f;
        TArray<FHeldKey> Held;
        bool bWalking = false;
        FVector2D WalkTarget = FVector2D::ZeroVector;
        float WalkStopDistance = 150.f;
        float WalkRemaining = 0.f;
        float WalkProgressTimer = 0.f;
        double WalkProgressDistance = 0.0;
        FString WalkStatus = TEXT("idle");
        FTSTicker::FDelegateHandle Ticker;
    };

    FAgentInputState& InputState()
    {
        static FAgentInputState State;
        return State;
    }

    APlayerController* PlayController()
    {
        UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
        return World ? World->GetFirstPlayerController() : nullptr;
    }

    void ApplySticks(APlayerController* PC, FVector2D Move, FVector2D Look)
    {
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, Move.X, 1));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY, IE_Axis, Move.Y, 1));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX, IE_Axis, Look.X, 1));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightY, IE_Axis, Look.Y, 1));
    }

    bool Tick(float DeltaSeconds)
    {
        FAgentInputState& State = InputState();
        APlayerController* PC = PlayController();
        if (!PC)
        {
            State.StickRemaining = 0.f;
            State.Held.Reset();
            if (State.bWalking) State.WalkStatus = TEXT("cancelled");
            State.bWalking = false;
            State.Ticker.Reset();
            return false;
        }
        if (State.bWalking)
        {
            const APawn* Pawn = PC->GetPawn();
            const FVector2D Here = Pawn ? FVector2D(Pawn->GetActorLocation()) : FVector2D::ZeroVector;
            const double Distance = FVector2D::Distance(Here, State.WalkTarget);
            State.WalkRemaining -= DeltaSeconds;
            State.WalkProgressTimer += DeltaSeconds;
            FString Finished;
            if (!Pawn) Finished = TEXT("cancelled");
            else if (Distance <= State.WalkStopDistance) Finished = TEXT("arrived");
            else if (State.WalkRemaining <= 0.f) Finished = TEXT("timeout");
            else if (State.WalkProgressTimer >= 2.f)
            {
                if (State.WalkProgressDistance - Distance < 40.0) Finished = TEXT("stuck");
                State.WalkProgressTimer = 0.f;
                State.WalkProgressDistance = Distance;
            }
            if (!Finished.IsEmpty())
            {
                State.bWalking = false;
                State.WalkStatus = Finished;
                ApplySticks(PC, FVector2D::ZeroVector, FVector2D::ZeroVector);
            }
            else
            {
                const double TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(State.WalkTarget.Y - Here.Y, State.WalkTarget.X - Here.X));
                const double Bearing = FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, TargetYaw);
                const double Radians = FMath::DegreesToRadians(Bearing);
                const float Speed = Distance < 400.0 ? 0.55f : 1.f;
                ApplySticks(PC, FVector2D(FMath::Sin(Radians), FMath::Cos(Radians)) * Speed,
                    FVector2D(FMath::Clamp(static_cast<float>(Bearing) / 60.f, -0.6f, 0.6f), 0.f));
            }
        }
        else if (State.StickRemaining > 0.f)
        {
            State.StickRemaining -= DeltaSeconds;
            const bool bDone = State.StickRemaining <= 0.f;
            ApplySticks(PC, bDone ? FVector2D::ZeroVector : State.Move, bDone ? FVector2D::ZeroVector : State.Look);
        }
        for (int32 Index = State.Held.Num() - 1; Index >= 0; --Index)
        {
            FHeldKey& Held = State.Held[Index];
            Held.Remaining -= DeltaSeconds;
            if (Held.Remaining <= 0.f)
            {
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(Held.Key, IE_Released, 0));
                State.Held.RemoveAt(Index);
            }
        }
        if (!State.bWalking && State.StickRemaining <= 0.f && State.Held.IsEmpty())
        {
            State.Ticker.Reset();
            return false;
        }
        return true;
    }

    void EnsureTicker()
    {
        FAgentInputState& State = InputState();
        if (!State.Ticker.IsValid())
        {
            State.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
        }
    }

    FString AgentError(const FString& Message) { return TEXT("error: ") + Message; }

    bool ParseKey(const FString& Name, FKey& Out)
    {
        Out = FKey(FName(*Name));
        return Out.IsValid();
    }

    const TCHAR* ResourceKindName(Homestead::ResourceKind Kind)
    {
        switch (Kind)
        {
        case Homestead::ResourceKind::Branches: return TEXT("Branches");
        case Homestead::ResourceKind::Stones: return TEXT("Stones");
        case Homestead::ResourceKind::BerryBush: return TEXT("BerryBush");
        case Homestead::ResourceKind::Roots: return TEXT("Roots");
        case Homestead::ResourceKind::Flowers: return TEXT("Flowers");
        case Homestead::ResourceKind::Reeds: return TEXT("Reeds");
        case Homestead::ResourceKind::Sapling: return TEXT("Sapling");
        case Homestead::ResourceKind::ForestTree: return TEXT("ForestTree");
        default: return TEXT("Unknown");
        }
    }
}

FString UHomesteadAgentPlayLibrary::TapKey(const FString& Key)
{
    APlayerController* PC = PlayController();
    if (!PC) return AgentError(TEXT("no Play-In-Editor player controller"));
    FKey Parsed;
    if (!ParseKey(Key, Parsed)) return AgentError(FString::Printf(TEXT("unknown key '%s'"), *Key));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Parsed, IE_Pressed, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Parsed, IE_Released, 0));
    return TEXT("ok");
}

FString UHomesteadAgentPlayLibrary::HoldKey(const FString& Key, float Seconds)
{
    APlayerController* PC = PlayController();
    if (!PC) return AgentError(TEXT("no Play-In-Editor player controller"));
    FKey Parsed;
    if (!ParseKey(Key, Parsed)) return AgentError(FString::Printf(TEXT("unknown key '%s'"), *Key));
    FAgentInputState& State = InputState();
    State.Held.RemoveAll([&Parsed](const FHeldKey& Held) { return Held.Key == Parsed; });
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Parsed, IE_Pressed, 1));
    State.Held.Add({Parsed, FMath::Max(Seconds, 0.f)});
    EnsureTicker();
    return TEXT("ok");
}

FString UHomesteadAgentPlayLibrary::SetSticks(float MoveX, float MoveY, float LookX, float LookY, float Seconds)
{
    APlayerController* PC = PlayController();
    if (!PC) return AgentError(TEXT("no Play-In-Editor player controller"));
    FAgentInputState& State = InputState();
    if (State.bWalking) { State.bWalking = false; State.WalkStatus = TEXT("cancelled"); }
    State.Move = FVector2D(FMath::Clamp(MoveX, -1.f, 1.f), FMath::Clamp(MoveY, -1.f, 1.f));
    State.Look = FVector2D(FMath::Clamp(LookX, -1.f, 1.f), FMath::Clamp(LookY, -1.f, 1.f));
    State.StickRemaining = FMath::Max(Seconds, 0.f);
    ApplySticks(PC, State.StickRemaining > 0.f ? State.Move : FVector2D::ZeroVector,
        State.StickRemaining > 0.f ? State.Look : FVector2D::ZeroVector);
    EnsureTicker();
    return TEXT("ok");
}

FString UHomesteadAgentPlayLibrary::ReleaseAll()
{
    FAgentInputState& State = InputState();
    if (APlayerController* PC = PlayController())
    {
        ApplySticks(PC, FVector2D::ZeroVector, FVector2D::ZeroVector);
        for (const FHeldKey& Held : State.Held)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Held.Key, IE_Released, 0));
        }
    }
    State.StickRemaining = 0.f;
    State.Held.Reset();
    if (State.bWalking) { State.bWalking = false; State.WalkStatus = TEXT("cancelled"); }
    return TEXT("ok");
}

FString UHomesteadAgentPlayLibrary::WalkTo(float X, float Y, float StopDistanceCm, float TimeoutSeconds)
{
    APlayerController* PC = PlayController();
    if (!PC || !PC->GetPawn()) return AgentError(TEXT("no Play-In-Editor player pawn"));
    FAgentInputState& State = InputState();
    State.StickRemaining = 0.f;
    State.bWalking = true;
    State.WalkTarget = FVector2D(X, Y);
    State.WalkStopDistance = FMath::Max(StopDistanceCm, 30.f);
    State.WalkRemaining = FMath::Max(TimeoutSeconds, 0.5f);
    State.WalkProgressTimer = 0.f;
    State.WalkProgressDistance = FVector2D::Distance(FVector2D(PC->GetPawn()->GetActorLocation()), State.WalkTarget);
    State.WalkStatus = TEXT("walking");
    EnsureTicker();
    return TEXT("ok");
}

bool UHomesteadAgentPlayLibrary::IsInputActive()
{
    const FAgentInputState& State = InputState();
    return State.bWalking || State.StickRemaining > 0.f || !State.Held.IsEmpty();
}

FString UHomesteadAgentPlayLibrary::GetPlayState(int32 NearbyCount, float RadiusCm)
{
    const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    APlayerController* PC = PlayController();
    Root->SetBoolField(TEXT("pie"), PC != nullptr);
    Root->SetBoolField(TEXT("characterLab"), PC && PC->IsA<AHomesteadLabController>());
    AHomesteadController* HC = Cast<AHomesteadController>(PC);
    if (HC)
    {
        const Homestead::State& Sim = HC->State();
        Root->SetBoolField(TEXT("worldReady"), HC->IsWorldReady());
        Root->SetBoolField(TEXT("bookOpen"), HC->IsBookOpen());
        Root->SetBoolField(TEXT("planning"), HC->IsPlanning());
        Root->SetBoolField(TEXT("failed"), HC->IsFailed());
        Root->SetBoolField(TEXT("gamepadPrompts"), HC->UsesGamepad());
        Root->SetNumberField(TEXT("bookPage"), HC->BookPage());
        Root->SetNumberField(TEXT("selectedRow"), HC->SelectedRow());
        Root->SetNumberField(TEXT("hotbarSlot"), HC->SelectedHotbarIndex());
        Root->SetStringField(TEXT("bookTitle"), HC->BookTitle());
        Root->SetStringField(TEXT("bookSummary"), HC->BookSummary());
        Root->SetStringField(TEXT("bookFooter"), HC->BookFooter());
        Root->SetStringField(TEXT("focusTitle"), HC->FocusTitle());
        Root->SetStringField(TEXT("focusActions"), HC->FocusActions());
        Root->SetStringField(TEXT("toast"), HC->Toast());
        Root->SetBoolField(TEXT("toastIsError"), HC->ToastIsError());
        if (HC->IsPlanning()) Root->SetStringField(TEXT("placement"), HC->PlacementLabel());
        Root->SetStringField(TEXT("inventory"), HC->MenuInventorySummary());
        Root->SetNumberField(TEXT("hour"), Sim.hour);
        Root->SetNumberField(TEXT("hunger"), Sim.hunger);
        Root->SetNumberField(TEXT("energy"), Sim.energy);
        Root->SetNumberField(TEXT("warmth"), Sim.warmth);
    }
    if (PC)
    {
        const float Yaw = PC->GetControlRotation().Yaw;
        Root->SetNumberField(TEXT("controlYaw"), Yaw);
        Root->SetNumberField(TEXT("controlPitch"), PC->GetControlRotation().Pitch);
        if (const APawn* Pawn = PC->GetPawn())
        {
            const FVector Location = Pawn->GetActorLocation();
            const TSharedRef<FJsonObject> Position = MakeShared<FJsonObject>();
            Position->SetNumberField(TEXT("x"), Location.X);
            Position->SetNumberField(TEXT("y"), Location.Y);
            Position->SetNumberField(TEXT("z"), Location.Z);
            Root->SetObjectField(TEXT("location"), Position);
            Root->SetNumberField(TEXT("speedCmPerSec"), Pawn->GetVelocity().Size2D());
            if (HC)
            {
                struct FNear { const Homestead::ResourceNode* Node; double Distance; };
                TArray<FNear> Near;
                for (const Homestead::ResourceNode& Node : HC->State().resources)
                {
                    const double Distance = FVector2D::Distance(FVector2D(Location), FVector2D(Node.position.x, Node.position.y));
                    if (Distance <= RadiusCm) Near.Add({&Node, Distance});
                }
                Near.Sort([](const FNear& A, const FNear& B) { return A.Distance < B.Distance; });
                TArray<TSharedPtr<FJsonValue>> Items;
                for (int32 Index = 0; Index < FMath::Min(Near.Num(), FMath::Max(NearbyCount, 0)); ++Index)
                {
                    const Homestead::ResourceNode& Node = *Near[Index].Node;
                    const double TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(Node.position.y - Location.Y, Node.position.x - Location.X));
                    const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
                    Item->SetNumberField(TEXT("id"), Node.id);
                    Item->SetStringField(TEXT("kind"), ResourceKindName(Node.kind));
                    Item->SetNumberField(TEXT("x"), Node.position.x);
                    Item->SetNumberField(TEXT("y"), Node.position.y);
                    Item->SetNumberField(TEXT("distanceCm"), FMath::RoundToInt(Near[Index].Distance));
                    Item->SetNumberField(TEXT("bearingDeg"), FMath::RoundToInt(FMath::FindDeltaAngleDegrees(Yaw, TargetYaw)));
                    Item->SetBoolField(TEXT("cleared"), Node.cleared);
                    Item->SetBoolField(TEXT("ready"), !Node.cleared && Node.readyAtHour <= HC->State().hour);
                    Item->SetBoolField(TEXT("focused"), HC->IsResourceFocused(Node.id));
                    Items.Add(MakeShared<FJsonValueObject>(Item));
                }
                Root->SetArrayField(TEXT("nearbyResources"), Items);
            }
        }
    }
    Root->SetBoolField(TEXT("agentInputActive"), IsInputActive());
    Root->SetStringField(TEXT("walk"), InputState().WalkStatus);
    FString Output;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root, Writer);
    return Output;
}
