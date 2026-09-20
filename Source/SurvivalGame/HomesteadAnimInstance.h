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
    float GaitRate() const;
    float WalkPhase() const;

protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
