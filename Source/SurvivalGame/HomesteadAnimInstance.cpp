#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HomesteadWorld.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
enum class EHandAction { None, Gather, Water, Clear, KnifeCut, Till };
struct FLocomotionBlend : FAnimNode_TwoWayBlend
{
    FLocomotionBlend() { bAlwaysUpdateChildren = true; }
};

struct FGatherPose : FAnimNode_SequenceEvaluator_Standalone
{
    virtual bool IsLooping() const override { return false; }
};

struct FHomesteadAnimProxy : FAnimInstanceProxy
{
    explicit FHomesteadAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        Blend.A.SetLinkNode(&Idle);
        Blend.B.SetLinkNode(&SprintBlend);
        if (bTrialCMUWalk)
        {
            GaitBlend.A.SetLinkNode(&Walk);
            GaitBlend.B.SetLinkNode(&SlowWalk);
            SlowWalk.SetTeleportToExplicitTime(true);
            SlowWalk.SetShouldLoop(true);
        }
        SprintBlend.A.SetLinkNode(bTrialCMUWalk
            ? static_cast<FAnimNode_Base*>(&GaitBlend) : static_cast<FAnimNode_Base*>(&Walk));
        SprintBlend.B.SetLinkNode(&Sprint);
        ActionBlend.A.SetLinkNode(&Blend);
        ActionBlend.B.SetLinkNode(&Gather);
        Gather.SetTeleportToExplicitTime(true);
        if (bTrialFootLock)
        {
            ToComponent.LocalPose.SetLinkNode(&ActionBlend);
            LeftFoot.ComponentPose.SetLinkNode(&ToComponent);
            RightFoot.ComponentPose.SetLinkNode(&LeftFoot);
            LeftRotation.ComponentPose.SetLinkNode(&RightFoot);
            RightRotation.ComponentPose.SetLinkNode(&LeftRotation);
            ToLocal.ComponentPose.SetLinkNode(&RightRotation);
            LeftFoot.IKBone.BoneName = TEXT("foot_l");
            RightFoot.IKBone.BoneName = TEXT("foot_r");
            for (FAnimNode_TwoBoneIK* Foot : {&LeftFoot, &RightFoot})
            {
                Foot->EffectorLocationSpace = BCS_ComponentSpace;
                Foot->JointTargetLocationSpace = BCS_ComponentSpace;
                Foot->bMaintainEffectorRelRot = true;
                Foot->bAllowStretching = false;
                Foot->Alpha = 0;
            }
            LeftRotation.BoneToModify.BoneName = TEXT("foot_l");
            RightRotation.BoneToModify.BoneName = TEXT("foot_r");
            for (FAnimNode_ModifyBone* Foot : {&LeftRotation, &RightRotation})
            {
                Foot->RotationMode = BMM_Replace;
                Foot->RotationSpace = BCS_ComponentSpace;
                Foot->TranslationMode = BMM_Ignore;
                Foot->ScaleMode = BMM_Ignore;
                Foot->Alpha = 0;
            }
        }
    }

    FAnimNode_SequencePlayer_Standalone Idle;
    FAnimNode_SequencePlayer_Standalone Walk;
    FAnimNode_SequenceEvaluator_Standalone SlowWalk;
    FAnimNode_SequencePlayer_Standalone Sprint;
    FLocomotionBlend GaitBlend;
    FLocomotionBlend SprintBlend;
    FLocomotionBlend Blend;
    FLocomotionBlend ActionBlend;
    FGatherPose Gather;
    const bool bTrialFootLock = FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialFootLock"))
        || FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineTrialVitruvian01"))
        || FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialCMULevelHead"));
    const bool bTrialCMUWalk = FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialCMUWalk01"));
    FAnimNode_ConvertLocalToComponentSpace ToComponent;
    FAnimNode_TwoBoneIK LeftFoot;
    FAnimNode_TwoBoneIK RightFoot;
    FAnimNode_ModifyBone LeftRotation;
    FAnimNode_ModifyBone RightRotation;
    FAnimNode_ConvertComponentToLocalSpace ToLocal;
    FVector LeftAnchor = FVector::ZeroVector;
    FVector RightAnchor = FVector::ZeroVector;
    FQuat LeftAnchorRotation = FQuat::Identity;
    FQuat RightAnchorRotation = FQuat::Identity;
    float LeftGroundedZ = 0;
    float RightGroundedZ = 0;
    bool bLockLeft = false;
    bool bLockRight = false;
    FVector PreviousActorLocation = FVector::ZeroVector;
    bool bAnchored = false;
    float PreviousSpeed = 0;
    float Rate = 0;
    float GatherTime = 0;
    uint32 Started = 0;
    uint32 WaterStarted = 0;
    uint32 ClearStarted = 0;
    uint32 KnifeStarted = 0;
    uint32 TillStarted = 0;
    EHandAction Requested = EHandAction::None;
    EHandAction Active = EHandAction::Gather;
    bool bCancelled = false;
    bool bGathering = false;

    virtual FAnimNode_Base* GetCustomRootNode() override
    {
        return bTrialFootLock ? static_cast<FAnimNode_Base*>(&ToLocal) : &ActionBlend;
    }

    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        bAnchored = false;
        PreviousSpeed = 0;
        if (const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner()))
        {
            Idle.SetSequence(Avatar->GetIdleAnimation());
            Walk.SetSequence(Avatar->GetWalkAnimation());
            if (bTrialCMUWalk) SlowWalk.SetSequence(Avatar->GetSlowWalkAnimation());
            Sprint.SetSequence(Avatar->GetSprintAnimation());
            Gather.SetSequence(Avatar->GetGatherAnimation());
        }
    }

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner());
        const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
        const float Speed = Avatar && PC && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
            ? Avatar->GetVelocity().Size2D() : 0.0f;
        if (bTrialFootLock)
        {
            const bool Grounded = Avatar && Avatar->GetCharacterMovement()->IsMovingOnGround();
            const FVector ActorLocation = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
            const bool Teleported = bAnchored && FVector::DistSquared(ActorLocation, PreviousActorLocation) > FMath::Square(100.0f);
            if (!Grounded || Teleported || Speed > 12.0f)
                bAnchored = false;
            else if (!bAnchored && PreviousSpeed > 12.0f && PC
                && Avatar->GetMesh()->GetSkeletalMeshAsset())
            {
                const USkeletalMeshComponent* Mesh = Avatar->GetMesh();
                const FVector LeftToe = Mesh->GetSocketLocation(TEXT("ball_l"));
                const FVector RightToe = Mesh->GetSocketLocation(TEXT("ball_r"));
                auto GroundAt = [Avatar, PC](const FVector& Toe, double& Ground)
                {
                    Ground = AHomesteadWorld::GroundHeight(Toe.X, Toe.Y, PC->State().world);
                    if (!FMath::IsFinite(Ground)) return false;
                    const auto& Floor = Avatar->GetCharacterMovement()->CurrentFloor;
                    if (Floor.IsWalkableFloor()
                        && Floor.HitResult.ImpactPoint.Z > Ground + 20.0)
                        Ground = Floor.HitResult.ImpactPoint.Z;
                    return true;
                };
                double LeftGround = 0, RightGround = 0;
                if (GroundAt(LeftToe, LeftGround) && GroundAt(RightToe, RightGround))
                {
                    LeftAnchor = Mesh->GetSocketLocation(TEXT("foot_l"));
                    RightAnchor = Mesh->GetSocketLocation(TEXT("foot_r"));
                    LeftAnchorRotation = Mesh->GetSocketTransform(TEXT("foot_l")).GetRotation();
                    RightAnchorRotation = Mesh->GetSocketTransform(TEXT("foot_r")).GetRotation();
                    const float LeftClearance = LeftToe.Z - LeftGround;
                    const float RightClearance = RightToe.Z - RightGround;
                    // Keep two planted soles still; a raised swing foot must finish its step.
                    const bool BothPlanted = FMath::Max(LeftClearance, RightClearance) <= 4.8f
                        && FMath::Abs(LeftClearance - RightClearance) <= 2.4f;
                    bLockLeft = BothPlanted || LeftClearance <= RightClearance;
                    bLockRight = BothPlanted || RightClearance < LeftClearance;
                    LeftGroundedZ = LeftGround + 1.0f + LeftAnchor.Z - LeftToe.Z;
                    RightGroundedZ = RightGround + 1.0f + RightAnchor.Z - RightToe.Z;
                    bAnchored = true;
                }
                else
                    UE_LOG(LogTemp, Warning, TEXT("Planted-stop trial could not resolve finite terrain below both feet."));
            }
            const USkeletalMeshComponent* Mesh = Avatar ? Avatar->GetMesh() : nullptr;
            LeftFoot.Alpha = bAnchored && bLockLeft && Mesh ? 1.0f : 0.0f;
            RightFoot.Alpha = bAnchored && bLockRight && Mesh ? 1.0f : 0.0f;
            LeftRotation.Alpha = LeftFoot.Alpha;
            RightRotation.Alpha = RightFoot.Alpha;
            if (bAnchored && Mesh)
            {
                if (bLockLeft)
                    LeftAnchor.Z = FMath::FInterpConstantTo(LeftAnchor.Z, LeftGroundedZ, DeltaSeconds, 35.0f);
                if (bLockRight)
                    RightAnchor.Z = FMath::FInterpConstantTo(RightAnchor.Z, RightGroundedZ, DeltaSeconds, 35.0f);
                const FTransform ComponentToWorld = Mesh->GetComponentTransform();
                LeftFoot.EffectorLocation = ComponentToWorld.InverseTransformPosition(LeftAnchor);
                RightFoot.EffectorLocation = ComponentToWorld.InverseTransformPosition(RightAnchor);
                LeftFoot.JointTargetLocation = ComponentToWorld.InverseTransformPosition(
                    Mesh->GetSocketLocation(TEXT("calf_l")));
                RightFoot.JointTargetLocation = ComponentToWorld.InverseTransformPosition(
                    Mesh->GetSocketLocation(TEXT("calf_r")));
                LeftRotation.Rotation = (ComponentToWorld.GetRotation().Inverse() * LeftAnchorRotation).Rotator();
                RightRotation.Rotation = (ComponentToWorld.GetRotation().Inverse() * RightAnchorRotation).Rotator();
            }
            PreviousActorLocation = ActorLocation;
            PreviousSpeed = Speed;
        }
        if (bTrialCMUWalk && Walk.GetSequence() && SlowWalk.GetSequence())
        {
            const float TargetSlow = FMath::Clamp((140.0f - Speed) / 60.0f, 0.0f, 1.0f);
            GaitBlend.Alpha = FMath::FInterpConstantTo(GaitBlend.Alpha, TargetSlow,
                DeltaSeconds, 1.0f / 0.18f);
            const float NormalLength = Walk.GetSequence()->GetPlayLength();
            const float SlowLength = SlowWalk.GetSequence()->GetPlayLength();
            const float DistancePerCycle = FMath::Lerp(
                129.0f * NormalLength, 88.8f * SlowLength, GaitBlend.Alpha);
            Rate = Speed * NormalLength / DistancePerCycle;
            const float Phase = FMath::Fmod(
                (Walk.GetCurrentAssetTime() + DeltaSeconds * Rate) / NormalLength
                + 12.0f / 172.0f, 1.0f);
            SlowWalk.SetExplicitTime(Phase * SlowLength);
        }
        else
            Rate = Speed / (Avatar ? Avatar->WalkClipSpeed() : 120.0f);
        Walk.SetPlayRate(Rate);
        Sprint.SetPlayRate(FMath::Clamp(Speed / (Avatar ? Avatar->SprintClipSpeed() : 300.0f), 0.5f, 1.2f));
        const float SprintTarget = Avatar && Avatar->IsSprinting() && Speed > 12.0f ? 1.0f : 0.0f;
        SprintBlend.Alpha = FMath::FInterpConstantTo(SprintBlend.Alpha, SprintTarget,
            DeltaSeconds, 1.0f / 0.16f);
        const float Target = FMath::Clamp(Speed / 35.0f, 0.0f, 1.0f);
        Blend.Alpha = FMath::FInterpConstantTo(Blend.Alpha, Target, DeltaSeconds, 1.0f / 0.20f);
        const bool Blocked = !Avatar || !PC || PC->IsBookOpen() || PC->IsPlanning() || PC->IsFailed()
            || Avatar->GetVelocity().Size2D() > 5
            || !Avatar->GetCharacterMovement()->IsMovingOnGround()
            || !Avatar->GetPendingMovementInputVector().IsNearlyZero()
            || Avatar->GetCharacterMovement()->GetCurrentAcceleration().Size2D() > 1;
        if (Requested != EHandAction::None && !Blocked && !bCancelled && !bGathering && ActionBlend.Alpha <= 0.001f)
        {
            Active = Requested;
            Gather.SetSequence(Active == EHandAction::Till ? Avatar->GetTillAnimation()
                : Active == EHandAction::KnifeCut ? Avatar->GetKnifeCutAnimation()
                : Active == EHandAction::Clear ? Avatar->GetClearAnimation()
                : Active == EHandAction::Water ? Avatar->GetWaterAnimation() : Avatar->GetGatherAnimation());
        }
        const auto* Clip = Gather.GetSequence();
        if (Blocked || bCancelled) bGathering = false;
        if (Requested != EHandAction::None && !Blocked && !bCancelled && Clip && !bGathering && ActionBlend.Alpha <= 0.001f)
        {
            GatherTime = 0;
            bGathering = true;
            if (Active == EHandAction::Clear) ++ClearStarted;
            else if (Active == EHandAction::KnifeCut) ++KnifeStarted;
            else if (Active == EHandAction::Till) ++TillStarted;
            else if (Active == EHandAction::Water) ++WaterStarted;
            else ++Started;
        }
        Requested = EHandAction::None;
        bCancelled = false;
        if (bGathering)
        {
            GatherTime = FMath::Min(GatherTime + DeltaSeconds, Clip->GetPlayLength());
            if (GatherTime >= Clip->GetPlayLength()) bGathering = false;
        }
        const float ActionTarget = bGathering && GatherTime < Clip->GetPlayLength() - 0.16f ? 1.0f : 0.0f;
        ActionBlend.Alpha = FMath::FInterpConstantTo(ActionBlend.Alpha, ActionTarget, DeltaSeconds,
            ActionTarget > ActionBlend.Alpha ? 1.0f / 0.12f : 1.0f / 0.16f);
        // A cancelled pose stays at its current phase while blending out; no restart snap.
        Gather.SetExplicitTime(GatherTime);
    }
};
}

FAnimInstanceProxy* UHomesteadAnimInstance::CreateAnimInstanceProxy()
{
    return new FHomesteadAnimProxy(this);
}

void UHomesteadAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{
    delete static_cast<FHomesteadAnimProxy*>(Proxy);
}

float UHomesteadAnimInstance::WalkWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Blend.Alpha * (1.0f - Proxy.SprintBlend.Alpha);
}

float UHomesteadAnimInstance::SlowWalkWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.bTrialCMUWalk ? Proxy.Blend.Alpha
        * (1.0f - Proxy.SprintBlend.Alpha) * Proxy.GaitBlend.Alpha : 0.0f;
}

float UHomesteadAnimInstance::SprintWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Blend.Alpha * Proxy.SprintBlend.Alpha;
}

float UHomesteadAnimInstance::SprintPhase() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().Sprint.GetCurrentAssetTime();
}

float UHomesteadAnimInstance::GaitRate() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().Rate;
}

float UHomesteadAnimInstance::WalkPhase() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().Walk.GetCurrentAssetTime();
}

void UHomesteadAnimInstance::RequestGather()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Gather;
}

void UHomesteadAnimInstance::RequestWater()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Water;
}

void UHomesteadAnimInstance::CancelAction(bool Immediate)
{
    auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    Proxy.Requested = EHandAction::None;
    Proxy.bCancelled = true;
    if (Immediate)
    {
        Proxy.bGathering = false;
        Proxy.ActionBlend.Alpha = 0;
    }
}

void UHomesteadAnimInstance::RequestClear()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Clear;
}

void UHomesteadAnimInstance::RequestKnifeCut()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::KnifeCut;
}

void UHomesteadAnimInstance::RequestTill()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Till;
}

float UHomesteadAnimInstance::ClearWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Clear ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::ClearPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Clear ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::ClearStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().ClearStarted;
}

bool UHomesteadAnimInstance::IsClearing() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Clear && Proxy.bGathering && !Proxy.bCancelled;
}

float UHomesteadAnimInstance::KnifeCutWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::KnifeCut ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::KnifeCutPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::KnifeCut ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::KnifeCutStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().KnifeStarted;
}

float UHomesteadAnimInstance::TillWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Till ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::TillPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Till ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::TillStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().TillStarted;
}

bool UHomesteadAnimInstance::IsTilling() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Till && Proxy.bGathering && !Proxy.bCancelled;
}

float UHomesteadAnimInstance::GatherWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Gather ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::GatherPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Gather ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::GatherStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().Started;
}

float UHomesteadAnimInstance::WaterWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Water ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::WaterPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Water ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::WaterStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().WaterStarted;
}

bool UHomesteadAnimInstance::IsWatering() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Water && Proxy.bGathering && !Proxy.bCancelled;
}

float UHomesteadAnimInstance::ActionWeight() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().ActionBlend.Alpha;
}
