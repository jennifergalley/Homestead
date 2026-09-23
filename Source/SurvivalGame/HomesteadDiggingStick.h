#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "HomesteadDiggingStick.generated.h"

UCLASS(ClassGroup=(Homestead), meta=(BlueprintSpawnableComponent))
class SURVIVALGAME_API UHomesteadDiggingStick : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    UHomesteadDiggingStick(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* TickFunction) override;
    bool IsPresented() const { return !bHiddenInGame; }
};
