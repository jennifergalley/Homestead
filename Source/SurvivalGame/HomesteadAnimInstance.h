#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "HomesteadFishingPresentation.h"
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
    // A hand action is playing or still blending out (a new request would be refused), or one is
    // already requested for the next update.
    bool IsHandActionBusy() const;
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
    // Crafting by hand (AN_HeroineMH_CraftHands) layered from spine_02 up while a recipe is held.
    float CraftWeight() const;
    // The oil lamp held up ahead of her (AN_HeroineMH_LampRaised), over the right arm and head.
    void SetLampRaised(bool bRaised);
    float LampRaisedWeight() const;
    // Fishing (AN_HeroineMH_Fishing; segments and beats in HomesteadFishingPresentation.h). The
    // controller sets the pose for the simulation's phase every tick; the counters advance only when
    // the loaded clip crosses its authored contact beat, never on a timer.
    void SetFishingPose(EHomesteadFishingPose Pose);
    // The quick hook-set, played once over the current pose (presentation only).
    void PlayFishingStrike();
    // The float lands on the water: commit the cast and start the bite clock.
    uint32 FishCastSplashes() const;
    // The fish clears the water: grant the catch.
    uint32 FishCatchLifts() const;
    // The segment playing: a landed Catch keeps playing to its end after SetFishingPose(None).
    EHomesteadFishingPose FishingPose() const;
    // Seconds on the fishing clip's own timeline while a segment plays, else -1.
    float FishingClipTime() const;
    bool IsFishingStrike() const;
    float FishingWeight() const;
    // False when the fishing clip failed to load: no contact beats will ever arrive.
    bool HasFishingClip() const;
    // Kneeling to set the lamp down or take it up (AN_HeroineMH_LampSetDown).
    void RequestLampKneel();
    bool IsLampKneeling() const;
    // Seconds into the kneel while it plays, else -1.
    float LampKneelPhase() const;
    // Two-handed axe felling: Strokes cuts into the trunk (the clip's stroke cycle repeats).
    void RequestFell(int32 Strokes);
    // A held tool button asks the running strike for more strokes (Homestead::ToolRepeat). True if the
    // clip took them: it loops its stroke cycle again instead of recovering. False once it has left
    // its last cycle, or when no strike is playing.
    bool ExtendFell(int32 Strokes);
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
