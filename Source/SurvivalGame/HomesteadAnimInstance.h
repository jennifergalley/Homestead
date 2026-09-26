#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "HomesteadAnimInstance.generated.h"

UCLASS(Transient)
class SURVIVALGAME_API UHomesteadAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    float WalkWeight() const;
    float SlowWalkWeight() const;
    float SprintWeight() const;
    float SprintPhase() const;
    float GaitRate() const;
    float WalkPhase() const;
    void RequestGather();
    void RequestGatherSticks();
    bool IsGatheringSticks() const;
    // Seconds into the stick-gather clip while it plays.
    float GatherSticksPhase() const;
    void RequestWater();
    void RequestClear();
    void RequestKnifeCut();
    void RequestTill();
    void RequestMacheteHack();
    // Hand-to-mouth eating layered over the right arm and head, so she can eat while walking. It
    // ignores CancelAction and plays to the end.
    void RequestEat();
    bool IsEating() const;
    float EatPhase() const;
    float EatWeight() const;
    uint32 EatStarts() const;
    // Two-handed axe felling: Strokes cuts into the trunk (the clip's stroke cycle repeats).
    void RequestFell(int32 Strokes);
    // Curl the right hand's fingers around a held tool handle (0 open, 1 closed grip). At rest the
    // wrist deviates CarryDegrees toward the pinky so the tool's head hangs down and forward.
    void SetRightHandGrip(float Alpha, float CarryDegrees = 46.0f);
    // Closes the left hand (0-1) outside felling, e.g. on a bunch of reed stems.
    void SetLeftHandGrip(float Alpha);
    void CancelAction(bool Immediate = false);
    float GatherWeight() const;
    float GatherPhase() const;
    uint32 GatherStarts() const;
    float WaterWeight() const;
    float WaterPhase() const;
    uint32 WaterStarts() const;
    bool IsWatering() const;
    float ClearWeight() const;
    float ClearPhase() const;
    uint32 ClearStarts() const;
    bool IsClearing() const;
    float KnifeCutWeight() const;
    float KnifeCutPhase() const;
    uint32 KnifeCutStarts() const;
    float TillWeight() const;
    float TillPhase() const;
    uint32 TillStarts() const;
    bool IsTilling() const;
    float MacheteWeight() const;
    float MachetePhase() const;
    uint32 MacheteStarts() const;
    float FellWeight() const;
    // Seconds since the felling started (play time, including repeated strokes).
    float FellPhase() const;
    uint32 FellStarts() const;
    bool IsFelling() const;
    bool IsHacking() const;
    float ActionWeight() const;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};

// Placed on locomotion clips at each foot's touchdown (homestead_agent.gasp_locomotion), so the
// footstep sound lands with the visible contact at any play rate. It only sounds while its clip
// dominates the blend, so walk and run contacts never double up during gait transitions.
UCLASS(meta = (DisplayName = "Homestead Footstep"))
class SURVIVALGAME_API UHomesteadFootstepNotify : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Footstep")
    bool bLeftFoot = true;
    UPROPERTY(EditAnywhere, Category = "Footstep")
    bool bRun = false;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
};
