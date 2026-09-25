#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
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
    void RequestWater();
    void RequestClear();
    void RequestKnifeCut();
    void RequestTill();
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
    float ActionWeight() const;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
