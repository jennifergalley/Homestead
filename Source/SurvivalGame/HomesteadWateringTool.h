#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "HomesteadWateringTool.generated.h"

UCLASS()
class SURVIVALGAME_API UHomesteadWateringTool : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    UHomesteadWateringTool(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    FVector GripPosition() const { return GetComponentLocation(); }
    bool IsPresented() const { return IsVisible() && !bHiddenInGame; }
    static float PourAngle(float Phase);
};
