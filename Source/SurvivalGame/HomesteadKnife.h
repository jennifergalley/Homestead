#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "HomesteadKnife.generated.h"

UCLASS()
class SURVIVALGAME_API UHomesteadKnife : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    explicit UHomesteadKnife(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* TickFunction) override;
    bool IsPresented() const { return !bHiddenInGame; }
};
