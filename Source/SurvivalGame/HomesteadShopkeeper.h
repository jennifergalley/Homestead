#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadShopkeeper.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UTextRenderComponent;

// The general-store shopkeeper. Until her MetaHuman is authored (add-dollars-and-general-store
// task 3.2) she is a clearly labelled stand-in: the legacy aproned heroine body playing the
// relaxed idle, turning to face the heroine when she comes within a few metres.
UCLASS()
class SURVIVALGAME_API AHomesteadShopkeeper : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadShopkeeper();
    virtual void Tick(float DeltaSeconds) override;
    // Where she stands and the way she faces when nobody is near (toward the customer side).
    void Place(const FVector& Location, float RestYaw);
    // Hidden outside opening hours.
    void SetOnDuty(bool bOnDuty);
    bool IsOnDuty() const { return bDuty; }
    static const TCHAR* DisplayName() { return TEXT("Mrs. Pascoe"); }
    static const TCHAR* FullName() { return TEXT("Mrs. Martha Pascoe"); }
    bool IsStandIn() const { return true; }

private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    float RestYaw = 0.0f;
    bool bDuty = true;
};
