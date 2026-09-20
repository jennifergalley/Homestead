#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"

namespace
{
struct FLocomotionBlend : FAnimNode_TwoWayBlend
{
    FLocomotionBlend() { bAlwaysUpdateChildren = true; }
};

struct FHomesteadAnimProxy : FAnimInstanceProxy
{
    explicit FHomesteadAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        Blend.A.SetLinkNode(&Idle);
        Blend.B.SetLinkNode(&Walk);
    }

    FAnimNode_SequencePlayer_Standalone Idle;
    FAnimNode_SequencePlayer_Standalone Walk;
    FLocomotionBlend Blend;
    float Rate = 0;

    virtual FAnimNode_Base* GetCustomRootNode() override { return &Blend; }

    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        if (const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner()))
        {
            Idle.SetSequence(Avatar->GetIdleAnimation());
            Walk.SetSequence(Avatar->GetWalkAnimation());
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
