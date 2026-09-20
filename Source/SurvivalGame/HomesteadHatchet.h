#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "HomesteadHatchet.generated.h"

UCLASS()
class SURVIVALGAME_API UHomesteadHatchet : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    UHomesteadHatchet(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    FVector GripPosition() const { return GetComponentLocation(); }
    bool IsPresented() const { return IsVisible() && !bHiddenInGame; }
    static float SwingAngle(float Phase);
};
