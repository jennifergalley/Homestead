#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadLampLook.h"
#include "Simulation/HomesteadLamp.h"

#include "Engine/World.h"

using HomesteadControllerText::Text;

void AHomesteadController::MenuRefillLamp()
{
    NotifyResourceAction(Sim.RefillLamp(), nullptr);
    LastLampOil = Sim.LampOil();
}

void AHomesteadController::HomesteadLampOil(float Hours)
{
    Sim.SetLampOil(Hours);
    Notify(FString::Printf(TEXT("The lamp has %.1f hours of oil."), Sim.LampOil()));
}

void AHomesteadController::UpdateLamp()
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const bool bInHand = !IsFailed() && LampHandoff == ELampHandoff::None
        && SelectedCarriedTool() == Homestead::Item::OilLamp;
    Sim.SetLampInHand(bInHand);
    if (Avatar) Avatar->SetHeldLampLit(Sim.LampOil() > 0.0);
    // Her hand has reached the ground: the lamp changes hands now.
    if (Avatar && Avatar->ConsumeLampContact())
    {
        if (LampHandoff == ELampHandoff::SetDown) NotifyResourceAction(Sim.SetDownLamp(LampSpot, PlayerPoint()), nullptr);
        else if (LampHandoff == ELampHandoff::PickUp)
        {
            const auto Result = Sim.PickUpDrop(LampDropId, PlayerPoint());
            NotifyResourceAction(Result, nullptr);
        }
        LampHandoff = ELampHandoff::None;
        RefreshRemaining = 0;
    }
    else if (LampHandoff != ELampHandoff::None && (!Avatar || !Avatar->IsLampKneeling()))
        LampHandoff = ELampHandoff::None; // She stood up before her hand reached the ground.
    const double Oil = Sim.LampOil();
    if (bInHand && !bLampWasInHand && Oil <= 0.0)
        Notify(bGamepad ? TEXT("The lamp is empty. Press X to fill it from an oil flask.")
            : TEXT("The lamp is empty. Press F to fill it from an oil flask."));
    else if (LastLampOil > Homestead::Lamp::LowHours && Oil <= Homestead::Lamp::LowHours && Oil > 0.0)
        Notify(TEXT("The lamp is burning low."));
    else if (LastLampOil > 0.0 && Oil <= 0.0)
        Notify(TEXT("The lamp has gone out. Fill it from an oil flask."));
    LastLampOil = Oil;
    bLampWasInHand = bInHand;
}

void AHomesteadController::StartLampSetDown()
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar || LampHandoff != ELampHandoff::None) return;
    const auto Position = PlayerPoint();
    const FVector Forward = Avatar->GetActorForwardVector();
    // Arm's length ahead of her, where the kneel lowers it.
    const Homestead::Point Spot{Position.x + Forward.X * 45.0, Position.y + Forward.Y * 45.0};
    if (Sim.NearWater(Spot))
    {
        Notify(TEXT("Set the lamp on dry ground."), true);
        return;
    }
    if (Avatar->PlayLampKneel(Spot, true))
    {
        LampHandoff = ELampHandoff::SetDown;
        LampSpot = Spot;
        return;
    }
    NotifyResourceAction(Sim.SetDownLamp(Spot, Position), nullptr);
    RefreshRemaining = 0;
}

bool AHomesteadController::StartLampPickUp(int32 DropId)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Lamp = Sim.SetDownLampDrop();
    if (!Avatar || !Lamp || Lamp->id != DropId || LampHandoff != ELampHandoff::None) return false;
    if (Sim.UsedCapacity() >= Sim.PackCapacity())
    {
        Notify(TEXT("Not enough pack space to pick up the lamp."), true);
        return true;
    }
    if (!Avatar->PlayLampKneel(Lamp->position, false)) return false;
    LampHandoff = ELampHandoff::PickUp;
    LampDropId = DropId;
    return true;
}

bool AHomesteadController::ResolveDropPoint(Homestead::Point& Result) const
{
    const Homestead::Point PlayerPosition = PlayerPoint();
    const float Yaw = GetPawn() ? GetPawn()->GetActorRotation().Yaw : GetControlRotation().Yaw;
    static constexpr float Angles[] = {0, -35, 35, -70, 70, 180};
    static constexpr float Distances[] = {150, 185, 210};
    for (const float Distance : Distances)
        for (const float Angle : Angles)
        {
            const FVector Direction = FRotator(0, Yaw + Angle, 0).Vector();
            const Homestead::Point Candidate{PlayerPosition.x + Direction.X * Distance,
                PlayerPosition.y + Direction.Y * Distance};
            if (bEstateMap ? WaterEdgeDistance(Candidate) <= 60.0 : Homestead::IsNearWater(Candidate)) continue;
            bool Clear = true;
            for (const auto& Structure : State().structures)
                if (FVector2D::Distance(FVector2D(Candidate.x, Candidate.y),
                    FVector2D(Homestead::StructureCenter(State(), Structure).x,
                    Homestead::StructureCenter(State(), Structure).y)) < 240)
                { Clear = false; break; }
            if (!Clear) continue;
            for (const auto& Plot : State().plots)
                if (FVector2D::Distance(FVector2D(Candidate.x, Candidate.y),
                    FVector2D(Homestead::PlotCenter(Plot).x, Homestead::PlotCenter(Plot).y)) < 90)
                { Clear = false; break; }
            if (!Clear) continue;
            const FVector Center(Candidate.x, Candidate.y,
                GroundHeight(Candidate.x, Candidate.y) + 45);
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadDropPlacement), false, GetPawn());
            if (GetWorld()->OverlapAnyTestByChannel(Center, FQuat::Identity,
                ECC_WorldStatic, FCollisionShape::MakeSphere(22), Params))
                continue;
            Result = Candidate;
            return true;
        }
    return false;
}
