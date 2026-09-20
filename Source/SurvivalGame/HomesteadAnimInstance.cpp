#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
enum class EHandAction { None, Gather, Water, Clear };
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
        Blend.B.SetLinkNode(&Walk);
        ActionBlend.A.SetLinkNode(&Blend);
        ActionBlend.B.SetLinkNode(&Gather);
        Gather.SetTeleportToExplicitTime(true);
    }

    FAnimNode_SequencePlayer_Standalone Idle;
    FAnimNode_SequencePlayer_Standalone Walk;
    FLocomotionBlend Blend;
    FLocomotionBlend ActionBlend;
    FGatherPose Gather;
    float Rate = 0;
    float GatherTime = 0;
    uint32 Started = 0;
    uint32 WaterStarted = 0;
    uint32 ClearStarted = 0;
    EHandAction Requested = EHandAction::None;
    EHandAction Active = EHandAction::Gather;
    bool bCancelled = false;
    bool bGathering = false;

    virtual FAnimNode_Base* GetCustomRootNode() override { return &ActionBlend; }

    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        if (const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner()))
        {
            Idle.SetSequence(Avatar->GetIdleAnimation());
            Walk.SetSequence(Avatar->GetWalkAnimation());
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
        // The authored stance travels 60 cm in half a one-second cycle.
        Rate = Speed / 120.0f;
        Walk.SetPlayRate(Rate);
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
            Gather.SetSequence(Active == EHandAction::Clear ? Avatar->GetClearAnimation()
                : Active == EHandAction::Water ? Avatar->GetWaterAnimation() : Avatar->GetGatherAnimation());
        }
        const auto* Clip = Gather.GetSequence();
        if (Blocked || bCancelled) bGathering = false;
        if (Requested != EHandAction::None && !Blocked && !bCancelled && Clip && !bGathering && ActionBlend.Alpha <= 0.001f)
        {
            GatherTime = 0;
            bGathering = true;
            if (Active == EHandAction::Clear) ++ClearStarted;
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
    return GetProxyOnGameThread<FHomesteadAnimProxy>().Blend.Alpha;
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

void UHomesteadAnimInstance::CancelAction()
{
    auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    Proxy.Requested = EHandAction::None;
    Proxy.bCancelled = true;
}

void UHomesteadAnimInstance::RequestClear()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Clear;
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
