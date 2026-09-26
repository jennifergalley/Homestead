#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
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

enum class EHandAction { None, Gather, Water, Clear, KnifeCut, Till, GatherSticks, Machete, Fell };
struct FLocomotionBlend : FAnimNode_TwoWayBlend
{
    FLocomotionBlend() { bAlwaysUpdateChildren = true; }
};

struct FGatherPose : FAnimNode_SequenceEvaluator_Standalone
{
    virtual bool IsLooping() const override { return false; }
};

// Closes the right hand around a held tool handle on top of whatever the clip does: each finger
// joint flexes toward the palm about the across-the-knuckles axis, and the thumb wraps over.
struct FHandGrip : FAnimNode_SkeletalControlBase
{
    static constexpr int32 Chains = 5;
    FBoneReference Joints[Chains][3];
    float Angles[Chains][3] = {{72, 84, 52}, {76, 86, 54}, {80, 86, 54}, {84, 86, 54}, {18, 38, 32}};
    FBoneReference Hand, IndexBase, MiddleBase, PinkyBase;
    // The left hand's bones mirror the right's, so its palm faces the other way.
    bool bLeft = false;
    // Resting carry: ulnar deviation lets a hanging tool's head tip down and forward instead of
    // jutting straight out from the fist. 0 while an authored swing drives the wrist.
    float Carry = 0;
    float CarryDeviation = 46;

    explicit FHandGrip(bool bLeftHand = false) : bLeft(bLeftHand)
    {
        const TCHAR* Side = bLeft ? TEXT("l") : TEXT("r");
        const TCHAR* Names[Chains] = {TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"), TEXT("thumb")};
        for (int32 Chain = 0; Chain < Chains; ++Chain)
            for (int32 Joint = 0; Joint < 3; ++Joint)
                Joints[Chain][Joint].BoneName = *FString::Printf(TEXT("%s_%02d_%s"), Names[Chain], Joint + 1, Side);
        Hand.BoneName = *FString::Printf(TEXT("hand_%s"), Side);
        IndexBase.BoneName = *FString::Printf(TEXT("index_01_%s"), Side);
        MiddleBase.BoneName = *FString::Printf(TEXT("middle_01_%s"), Side);
        PinkyBase.BoneName = *FString::Printf(TEXT("pinky_01_%s"), Side);
        Alpha = 0;
    }
    virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override
    {
        for (auto& Chain : Joints) for (auto& Joint : Chain) Joint.Initialize(RequiredBones);
        for (FBoneReference* Bone : {&Hand, &IndexBase, &MiddleBase, &PinkyBase}) Bone->Initialize(RequiredBones);
    }
    virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override
    {
        for (auto& Chain : Joints) for (auto& Joint : Chain) if (!Joint.IsValidToEvaluate(RequiredBones)) return false;
        return Hand.IsValidToEvaluate(RequiredBones) && IndexBase.IsValidToEvaluate(RequiredBones)
            && MiddleBase.IsValidToEvaluate(RequiredBones) && PinkyBase.IsValidToEvaluate(RequiredBones);
    }
    virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
        TArray<FBoneTransform>& OutBoneTransforms) override
    {
        const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
        auto CS = [&](const FBoneReference& Bone) { return Output.Pose.GetComponentSpaceTransform(Bone.GetCompactPoseIndex(Bones)); };
        const FVector HandAt = CS(Hand).GetLocation();
        const FVector Along = (CS(MiddleBase).GetLocation() - HandAt).GetSafeNormal();
        const FVector Across = (CS(IndexBase).GetLocation() - CS(PinkyBase).GetLocation()).GetSafeNormal();
        const FVector Palm = FVector::CrossProduct(Across, Along).GetSafeNormal() * (bLeft ? -1.0f : 1.0f);
        if (Palm.IsNearlyZero()) return;
        // Palm x Along = -Across, so a positive turn about Palm swings the fingers to the pinky side.
        FTransform Wrist = FTransform::Identity;
        if (Carry > 0.001f)
        {
            const FQuat Turn(Palm, FMath::DegreesToRadians(CarryDeviation * Carry));
            Wrist = FTransform(-HandAt) * FTransform(Turn) * FTransform(HandAt);
            const FCompactPoseBoneIndex HandIndex = Hand.GetCompactPoseIndex(Bones);
            OutBoneTransforms.Add(FBoneTransform(HandIndex, CS(Hand) * Wrist));
        }
        for (int32 Chain = 0; Chain < Chains; ++Chain)
        {
            const bool bThumb = Chain == Chains - 1;
            const FCompactPoseBoneIndex First = Joints[Chain][0].GetCompactPoseIndex(Bones);
            FTransform ParentOld = Output.Pose.GetComponentSpaceTransform(Bones.GetParentBoneIndex(First));
            FTransform ParentNew = ParentOld;
            FVector Direction = Along;
            for (int32 Joint = 0; Joint < 3; ++Joint)
            {
                const FCompactPoseBoneIndex Index = Joints[Chain][Joint].GetCompactPoseIndex(Bones);
                const FTransform Old = Output.Pose.GetComponentSpaceTransform(Index);
                FTransform New = Old.GetRelativeTransform(ParentOld) * ParentNew;
                if (Joint < 2)
                    Direction = (Output.Pose.GetComponentSpaceTransform(Joints[Chain][Joint + 1].GetCompactPoseIndex(Bones)).GetLocation()
                        - Old.GetLocation()).GetSafeNormal();
                // Rotating about Axis moves the segment toward Axis x Direction; pick the sign that
                // curls toward the palm. The thumb bends straight toward the palm.
                FVector Axis = bThumb ? FVector::CrossProduct(Direction, Palm).GetSafeNormal() : Across;
                if (FVector::DotProduct(FVector::CrossProduct(Axis, Direction), Palm) < 0) Axis = -Axis;
                if (!Axis.IsNearlyZero())
                    New.SetRotation(FQuat(Axis, FMath::DegreesToRadians(Angles[Chain][Joint])) * New.GetRotation());
                OutBoneTransforms.Add(FBoneTransform(Index, New * Wrist));
                ParentOld = Old;
                ParentNew = New;
            }
        }
        OutBoneTransforms.Sort(FCompareBoneTransformIndex());
    }
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
        // Eating rides on top of whatever she is doing: the right arm and the head only.
        EatLayer.BasePose.SetLinkNode(&ActionBlend);
        EatLayer.AddPose();
        EatLayer.BlendPoses[0].SetLinkNode(&Eat);
        for (const TCHAR* Branch : {TEXT("clavicle_r"), TEXT("neck_01")})
            EatLayer.LayerSetup[0].BranchFilters.AddDefaulted_GetRef().BoneName = Branch;
        EatLayer.BlendWeights[0] = 0;
        Eat.SetTeleportToExplicitTime(true);
        PlaceToComponent.LocalPose.SetLinkNode(&EatLayer);
        PelvisPlacement.ComponentPose.SetLinkNode(&PlaceToComponent);
        LeftSlope.ComponentPose.SetLinkNode(&PelvisPlacement);
        RightSlope.ComponentPose.SetLinkNode(&LeftSlope);
        LeftPlant.ComponentPose.SetLinkNode(&RightSlope);
        RightPlant.ComponentPose.SetLinkNode(&LeftPlant);
        Grip.ComponentPose.SetLinkNode(&RightPlant);
        LeftGrip.ComponentPose.SetLinkNode(&Grip);
        Pinch.ComponentPose.SetLinkNode(&LeftGrip);
        PlaceToLocal.ComponentPose.SetLinkNode(&Pinch);
        // A pinch for food: index fingertip meets the thumb, the other fingers tuck into the palm.
        const float PinchAngles[FHandGrip::Chains][3] = {{34, 58, 40}, {62, 78, 48}, {82, 88, 52}, {86, 88, 52}, {44, 32, 26}};
        FMemory::Memcpy(Pinch.Angles, PinchAngles, sizeof(PinchAngles));
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
            ToComponent.LocalPose.SetLinkNode(&EatLayer);
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
    FAnimNode_SequenceEvaluator_Standalone Eat;
    FAnimNode_LayeredBoneBlend EatLayer;
    float EatTime = 0;
    float EatAlpha = 0;
    bool bEating = false;
    bool bEatRequested = false;
    uint32 EatStarted = 0;
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
    FHandGrip Grip;
    // Closes the left hand on the axe haft while felling.
    FHandGrip LeftGrip{true};
    float LeftGripAlpha = 0;
    float LeftGripTarget = 0;
    // Closes her right fingers on a bite of food while she eats.
    FHandGrip Pinch;
    float GripAlpha = 0;
    float GripTarget = 0;
    float GripCarry = 46;
    float GripCarryTarget = 46;
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
    uint32 MacheteStarted = 0;
    uint32 FellStarted = 0;
    int32 FellStrokes = 1;
    int32 RequestedStrokes = 1;
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
            Eat.SetSequence(Avatar->GetEatAnimation());
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
                : Active == EHandAction::Machete ? Avatar->GetMacheteAnimation()
                : Active == EHandAction::Fell ? Avatar->GetFellAnimation()
                : Active == EHandAction::KnifeCut ? Avatar->GetKnifeCutAnimation()
                : Active == EHandAction::Clear ? Avatar->GetClearAnimation()
                : Active == EHandAction::Water ? Avatar->GetWaterAnimation()
                : Active == EHandAction::GatherSticks ? Avatar->GetGatherSticksAnimation() : Avatar->GetGatherAnimation());
        }
        const auto* Clip = Gather.GetSequence();
        if (Blocked || bCancelled) bGathering = false;
        if (Requested != EHandAction::None && !Blocked && !bCancelled && Clip && !bGathering && ActionBlend.Alpha <= 0.001f)
        {
            GatherTime = 0;
            bGathering = true;
            if (Active == EHandAction::Clear) ++ClearStarted;
            else if (Active == EHandAction::KnifeCut) ++KnifeStarted;
            else if (Active == EHandAction::Machete) ++MacheteStarted;
            else if (Active == EHandAction::Fell) { ++FellStarted; FellStrokes = RequestedStrokes; }
            else if (Active == EHandAction::Till) ++TillStarted;
            else if (Active == EHandAction::Water) ++WaterStarted;
            else ++Started;
        }
        Requested = EHandAction::None;
        bCancelled = false;
        // Felling repeats the clip's stroke cycle once per stroke the tree needs.
        const bool bFell = Active == EHandAction::Fell && Clip;
        const float PlayLength = !Clip ? 0.0f : bFell
            ? AHomesteadCharacter::FellPlayLength(Clip->GetPlayLength(), FellStrokes) : Clip->GetPlayLength();
        if (bGathering)
        {
            GatherTime = FMath::Min(GatherTime + DeltaSeconds, PlayLength);
            if (GatherTime >= PlayLength) bGathering = false;
        }
        const float ActionTarget = bGathering && GatherTime < PlayLength - 0.16f ? 1.0f : 0.0f;
        ActionBlend.Alpha = FMath::FInterpConstantTo(ActionBlend.Alpha, ActionTarget, DeltaSeconds,
            ActionTarget > ActionBlend.Alpha ? 1.0f / 0.12f : 1.0f / 0.16f);
        // A cancelled pose stays at its current phase while blending out; no restart snap.
        Gather.SetExplicitTime(bFell ? AHomesteadCharacter::FellClipTime(GatherTime, FellStrokes) : GatherTime);
        GripAlpha = FMath::FInterpConstantTo(GripAlpha, GripTarget, DeltaSeconds, 1.0f / 0.15f);
        Grip.Alpha = GripAlpha;
        // Switching tools eases the wrist to the new carry instead of snapping.
        GripCarry = GripAlpha < 0.01f ? GripCarryTarget : FMath::FInterpConstantTo(GripCarry, GripCarryTarget, DeltaSeconds, 180.0f);
        Grip.CarryDeviation = GripCarry;
        Grip.Carry = Active == EHandAction::Machete || Active == EHandAction::Fell ? 1.0f - ActionBlend.Alpha : 1.0f;
        LeftGripAlpha = FMath::FInterpConstantTo(LeftGripAlpha, LeftGripTarget, DeltaSeconds, 1.0f / 0.15f);
        LeftGrip.Alpha = FMath::Max(Active == EHandAction::Fell ? ActionBlend.Alpha : 0.0f, LeftGripAlpha);
        UpdateEating(DeltaSeconds);
    }

    void UpdateEating(float DeltaSeconds)
    {
        const auto* Clip = Eat.GetSequence();
        const float Length = Clip ? Clip->GetPlayLength() : 0.0f;
        if (bEatRequested && Clip && !bEating)
        {
            bEating = true;
            EatTime = 0;
            ++EatStarted;
        }
        bEatRequested = false;
        if (bEating)
        {
            EatTime = FMath::Min(EatTime + DeltaSeconds, Length);
            if (EatTime >= Length) bEating = false;
        }
        const float Target = bEating && EatTime < Length - 0.25f ? 1.0f : 0.0f;
        EatAlpha = FMath::FInterpConstantTo(EatAlpha, Target, DeltaSeconds, Target > EatAlpha ? 1.0f / 0.15f : 1.0f / 0.25f);
        EatLayer.BlendWeights[0] = EatAlpha;
        Eat.SetExplicitTime(EatTime);
        // Fingers close as they find the food in the pouch and open once it is in her mouth.
        const float Close = FMath::SmoothStep(0.45f, 0.63f, EatTime) * (1.0f - FMath::SmoothStep(1.36f, 1.6f, EatTime));
        Pinch.Alpha = (bEating ? Close : 0.0f) * EatAlpha * (1.0f - GripAlpha);
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

void UHomesteadAnimInstance::RequestGatherSticks()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::GatherSticks;
}

bool UHomesteadAnimInstance::IsGatheringSticks() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::GatherSticks && Proxy.bGathering && !Proxy.bCancelled;
}

float UHomesteadAnimInstance::GatherSticksPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::GatherSticks ? Proxy.GatherTime : 0;
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

void UHomesteadAnimInstance::RequestEat()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().bEatRequested = true;
}

bool UHomesteadAnimInstance::IsEating() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().bEating;
}

float UHomesteadAnimInstance::EatPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.bEating ? Proxy.EatTime : 0.0f;
}

float UHomesteadAnimInstance::EatWeight() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().EatAlpha;
}

uint32 UHomesteadAnimInstance::EatStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().EatStarted;
}

void UHomesteadAnimInstance::RequestMacheteHack()
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().Requested = EHandAction::Machete;
}

float UHomesteadAnimInstance::MacheteWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Machete ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::MachetePhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Machete ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::MacheteStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().MacheteStarted;
}

bool UHomesteadAnimInstance::IsHacking() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Machete && Proxy.bGathering && !Proxy.bCancelled;
}

void UHomesteadAnimInstance::RequestFell(int32 Strokes)
{
    auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    Proxy.Requested = EHandAction::Fell;
    Proxy.RequestedStrokes = FMath::Clamp(Strokes, 1, 8);
}

float UHomesteadAnimInstance::FellWeight() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Fell ? Proxy.ActionBlend.Alpha : 0;
}

float UHomesteadAnimInstance::FellPhase() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Fell ? Proxy.GatherTime : 0;
}

uint32 UHomesteadAnimInstance::FellStarts() const
{
    return GetProxyOnGameThread<FHomesteadAnimProxy>().FellStarted;
}

bool UHomesteadAnimInstance::IsFelling() const
{
    const auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    return Proxy.Active == EHandAction::Fell && Proxy.bGathering && !Proxy.bCancelled;
}

void UHomesteadAnimInstance::SetRightHandGrip(float Alpha, float CarryDegrees)
{
    auto& Proxy = GetProxyOnGameThread<FHomesteadAnimProxy>();
    Proxy.GripTarget = FMath::Clamp(Alpha, 0.0f, 1.0f);
    Proxy.GripCarryTarget = FMath::Clamp(CarryDegrees, -30.0f, 75.0f);
}

void UHomesteadAnimInstance::SetLeftHandGrip(float Alpha)
{
    GetProxyOnGameThread<FHomesteadAnimProxy>().LeftGripTarget = FMath::Clamp(Alpha, 0.0f, 1.0f);
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
