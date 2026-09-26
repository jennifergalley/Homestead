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
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HomesteadWorld.h"
#include "HomesteadLab.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
TAutoConsoleVariable<int32> CVarFootPlacement(TEXT("homestead.FootPlacement"), 1,
    TEXT("Ground-adaptive foot IK for the MetaHuman heroine: feet follow the terrain, the pelvis drops ")
    TEXT("to reach the lower foot and soles stay above ground (0 = off)."));

// Two-bone leg IK whose effector is the animated foot raised onto the ground under it. The ground
// height and pelvis drop come from the game thread (component space); the node adds only as much
// extra lift as keeps the ankle and ball above their standing clearance, so heel strikes and
// toe-offs never dip into the terrain.
struct FGroundedFootIK : FAnimNode_TwoBoneIK
{
    FBoneReference Ball;
    float GroundZ = 0;
    float BallGroundZ = 0;
    float PelvisDrop = 0;
    // MetaHuman standing reference: ankle 8.6 cm, ball 1.1 cm above the floor.
    static constexpr float AnkleClearance = 7.5f;
    static constexpr float BallClearance = 1.0f;

    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override
    {
        FAnimNode_TwoBoneIK::CacheBones_AnyThread(Context);
        Ball.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
    }

    virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override
    {
        return Ball.IsValidToEvaluate(RequiredBones) && FAnimNode_TwoBoneIK::IsValidToEvaluate(Skeleton, RequiredBones);
    }

    virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
        TArray<FBoneTransform>& OutBoneTransforms) override
    {
        const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
        const FCompactPoseBoneIndex FootIndex = IKBone.GetCompactPoseIndex(Bones);
        const FCompactPoseBoneIndex CalfIndex = Bones.GetParentBoneIndex(FootIndex);
        const FCompactPoseBoneIndex ThighIndex = Bones.GetParentBoneIndex(CalfIndex);
        const FVector Foot = Output.Pose.GetComponentSpaceTransform(FootIndex).GetLocation();
        const FVector Calf = Output.Pose.GetComponentSpaceTransform(CalfIndex).GetLocation();
        const FVector Thigh = Output.Pose.GetComponentSpaceTransform(ThighIndex).GetLocation();
        const double BallZ = Output.Pose.GetComponentSpaceTransform(Ball.GetCompactPoseIndex(Bones)).GetLocation().Z;
        // Heights are relative to the ground under the ankle and under the ball respectively.
        const float Clearance = FMath::Max3(0.0f, AnkleClearance - static_cast<float>(Foot.Z - PelvisDrop),
            BallGroundZ - GroundZ + BallClearance - static_cast<float>(BallZ - PelvisDrop));
        EffectorLocation = Foot + FVector(0, 0, GroundZ - PelvisDrop + Clearance);
        // Keep the knee bending in its animated plane; a near-straight leg falls back to the mesh
        // forward axis (+Y for the MetaHuman) so the solver never flips the knee.
        const FVector Axis = (Foot - Thigh).GetSafeNormal();
        FVector Knee = (Calf - Thigh) - Axis * FVector::DotProduct(Calf - Thigh, Axis);
        if (Knee.SizeSquared() < 1.0f)
            Knee = FVector::YAxisVector - Axis * Axis.Y;
        JointTargetLocation = Calf + Knee.GetSafeNormal() * 50.0f;
        FAnimNode_TwoBoneIK::EvaluateSkeletalControl_AnyThread(Output, OutBoneTransforms);
    }
};

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
        PlaceToComponent.LocalPose.SetLinkNode(&ActionBlend);
        PelvisPlacement.ComponentPose.SetLinkNode(&PlaceToComponent);
        LeftSlope.ComponentPose.SetLinkNode(&PelvisPlacement);
        RightSlope.ComponentPose.SetLinkNode(&LeftSlope);
        LeftPlant.ComponentPose.SetLinkNode(&RightSlope);
        RightPlant.ComponentPose.SetLinkNode(&LeftPlant);
        PlaceToLocal.ComponentPose.SetLinkNode(&RightPlant);
        PelvisPlacement.BoneToModify.BoneName = TEXT("pelvis");
        PelvisPlacement.TranslationMode = BMM_Additive;
        PelvisPlacement.TranslationSpace = BCS_ComponentSpace;
        PelvisPlacement.RotationMode = BMM_Ignore;
        PelvisPlacement.ScaleMode = BMM_Ignore;
        LeftPlant.IKBone.BoneName = TEXT("foot_l");
        RightPlant.IKBone.BoneName = TEXT("foot_r");
        LeftPlant.Ball.BoneName = TEXT("ball_l");
        RightPlant.Ball.BoneName = TEXT("ball_r");
        for (FGroundedFootIK* Foot : {&LeftPlant, &RightPlant})
        {
            Foot->EffectorLocationSpace = BCS_ComponentSpace;
            Foot->JointTargetLocationSpace = BCS_ComponentSpace;
            Foot->bMaintainEffectorRelRot = true;
            Foot->bAllowStretching = false;
        }
        LeftSlope.BoneToModify.BoneName = TEXT("foot_l");
        RightSlope.BoneToModify.BoneName = TEXT("foot_r");
        for (FAnimNode_ModifyBone* Foot : {&LeftSlope, &RightSlope})
        {
            Foot->RotationMode = BMM_Additive;
            Foot->RotationSpace = BCS_ComponentSpace;
            Foot->TranslationMode = BMM_Ignore;
            Foot->ScaleMode = BMM_Ignore;
        }
        for (FAnimNode_SkeletalControlBase* Node : std::initializer_list<FAnimNode_SkeletalControlBase*>{
            &PelvisPlacement, &LeftPlant, &RightPlant, &LeftSlope, &RightSlope})
            Node->Alpha = 0;
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
    FAnimNode_ConvertLocalToComponentSpace PlaceToComponent;
    FAnimNode_ModifyBone PelvisPlacement;
    FGroundedFootIK LeftPlant;
    FGroundedFootIK RightPlant;
    FAnimNode_ModifyBone LeftSlope;
    FAnimNode_ModifyBone RightSlope;
    FAnimNode_ConvertComponentToLocalSpace PlaceToLocal;
    float LeftGroundHeight = 0;
    float RightGroundHeight = 0;
    float LeftBallHeight = 0;
    float RightBallHeight = 0;
    float PelvisDrop = 0;
    FQuat LeftTilt = FQuat::Identity;
    FQuat RightTilt = FQuat::Identity;
    float PlacementAlpha = 0;
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
        return bTrialFootLock ? static_cast<FAnimNode_Base*>(&ToLocal) : &PlaceToLocal;
    }

    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        bAnchored = false;
        PreviousSpeed = 0;
        LeftGroundHeight = RightGroundHeight = LeftBallHeight = RightBallHeight = PelvisDrop = PlacementAlpha = 0;
        LeftTilt = RightTilt = FQuat::Identity;
        if (const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner()))
        {
            Idle.SetSequence(Avatar->GetIdleAnimation());
            Walk.SetSequence(Avatar->GetWalkAnimation());
            if (bTrialCMUWalk) SlowWalk.SetSequence(Avatar->GetSlowWalkAnimation());
            Sprint.SetSequence(Avatar->GetSprintAnimation());
            Gather.SetSequence(Avatar->GetGatherAnimation());
        }
    }

    // Game thread: trace the ground under each foot and hand the proxy nodes component-space
    // heights, the pelvis drop and a slope tilt. Values ease in so steps and slope changes never pop.
    void UpdateFootPlacement(const AHomesteadCharacter* Avatar, float DeltaSeconds)
    {
        const USkeletalMeshComponent* Mesh = Avatar ? Avatar->GetMesh() : nullptr;
        const bool Enabled = Mesh && !bTrialFootLock && Avatar->IsMetaHumanActive()
            && CVarFootPlacement.GetValueOnGameThread() != 0;
        const bool Grounded = Enabled && Avatar->GetCharacterMovement()->IsMovingOnGround();
        float LeftTarget = 0, RightTarget = 0, LeftBallTarget = 0, RightBallTarget = 0;
        FQuat LeftTiltTarget = FQuat::Identity, RightTiltTarget = FQuat::Identity;
        if (Grounded)
        {
            const UCapsuleComponent* Capsule = Avatar->GetCapsuleComponent();
            const FTransform& ToWorld = Mesh->GetComponentTransform();
            const float Base = ToWorld.GetLocation().Z;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadFootPlacement), false, Avatar);
            const FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
            auto Ground = [&](const FVector& At, float& Height, FVector* Normal)
            {
                FHitResult Hit;
                if (!Avatar->GetWorld()->LineTraceSingleByChannel(Hit, FVector(At.X, At.Y, Base + 50.0f),
                        FVector(At.X, At.Y, Base - 60.0f), Capsule->GetCollisionObjectType(), Params, Response)
                    || Hit.ImpactNormal.Z < 0.7f)
                    return false;
                Height = FMath::Clamp(static_cast<float>(Hit.ImpactPoint.Z - Base), -40.0f, 45.0f);
                if (Normal) *Normal = Hit.ImpactNormal;
                return true;
            };
            auto Trace = [&](FName Ankle, FName Ball, float& Height, float& BallHeight, FQuat& Tilt)
            {
                FVector Normal;
                if (!Ground(Mesh->GetSocketLocation(Ankle), Height, &Normal)) return;
                if (!Ground(Mesh->GetSocketLocation(Ball), BallHeight, nullptr)) BallHeight = Height;
                const FQuat Full = FQuat::FindBetweenNormals(FVector::UpVector,
                    ToWorld.InverseTransformVectorNoScale(Normal).GetSafeNormal());
                const float Angle = Full.GetAngle();
                const float Limit = FMath::DegreesToRadians(25.0f);
                Tilt = Angle > Limit ? FQuat::Slerp(FQuat::Identity, Full, Limit / Angle) : Full;
            };
            Trace(TEXT("foot_l"), TEXT("ball_l"), LeftTarget, LeftBallTarget, LeftTiltTarget);
            Trace(TEXT("foot_r"), TEXT("ball_r"), RightTarget, RightBallTarget, RightTiltTarget);
        }
        const float Ease = FMath::Clamp(DeltaSeconds * 14.0f, 0.0f, 1.0f);
        LeftGroundHeight = FMath::Lerp(LeftGroundHeight, LeftTarget, Ease);
        RightGroundHeight = FMath::Lerp(RightGroundHeight, RightTarget, Ease);
        LeftBallHeight = FMath::Lerp(LeftBallHeight, LeftBallTarget, Ease);
        RightBallHeight = FMath::Lerp(RightBallHeight, RightBallTarget, Ease);
        LeftTilt = FQuat::Slerp(LeftTilt, LeftTiltTarget, Ease);
        RightTilt = FQuat::Slerp(RightTilt, RightTiltTarget, Ease);
        PelvisDrop = FMath::Lerp(PelvisDrop, FMath::Min3(LeftTarget, RightTarget, 0.0f),
            FMath::Clamp(DeltaSeconds * 10.0f, 0.0f, 1.0f));
        PlacementAlpha = FMath::FInterpConstantTo(PlacementAlpha, Enabled ? 1.0f : 0.0f, DeltaSeconds, 4.0f);
        PelvisPlacement.Translation = FVector(0, 0, PelvisDrop);
        LeftPlant.GroundZ = LeftGroundHeight;
        RightPlant.GroundZ = RightGroundHeight;
        LeftPlant.BallGroundZ = LeftBallHeight;
        RightPlant.BallGroundZ = RightBallHeight;
        LeftPlant.PelvisDrop = RightPlant.PelvisDrop = PelvisDrop;
        LeftSlope.Rotation = LeftTilt.Rotator();
        RightSlope.Rotation = RightTilt.Rotator();
        for (FAnimNode_SkeletalControlBase* Node : std::initializer_list<FAnimNode_SkeletalControlBase*>{
            &PelvisPlacement, &LeftPlant, &RightPlant, &LeftSlope, &RightSlope})
            Node->Alpha = PlacementAlpha;
    }

    virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
        const auto* Avatar = Cast<AHomesteadCharacter>(Instance->TryGetPawnOwner());
        const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
        const bool Lab = Avatar && !PC && Avatar->InCharacterLab();
        const float Speed = Avatar && (Lab || (PC && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()))
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
        UpdateFootPlacement(Avatar, DeltaSeconds);
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
        const bool Blocked = !Avatar || (!Lab && (!PC || PC->IsBookOpen() || PC->IsPlanning() || PC->IsFailed()))
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

void UHomesteadFootstepNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    const auto* Avatar = MeshComp ? Cast<AHomesteadCharacter>(MeshComp->GetOwner()) : nullptr;
    const auto* Anim = MeshComp ? Cast<UHomesteadAnimInstance>(MeshComp->GetAnimInstance()) : nullptr;
    auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
    auto* Lab = Avatar ? Cast<AHomesteadLabController>(Avatar->GetController()) : nullptr;
    if ((!PC && !Lab) || !Anim || (bRun ? Anim->SprintWeight() : Anim->WalkWeight()) < 0.5f) return;
    if (PC) PC->PlayFootstep(bLeftFoot, bRun);
    else Lab->PlayFootstep(bLeftFoot, bRun);
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
