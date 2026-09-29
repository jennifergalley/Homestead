#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadShopkeeper.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UTextRenderComponent;

// A town shopkeeper: Mrs. Pascoe at the general store, Mr. Tregear at the seedsman's. Until their
// MetaHumans are authored (add-dollars-and-general-store task 3.2) each is a clearly labelled stand-in:
// a legacy heroine body playing the relaxed idle, turning to face the heroine when she comes within a
// few metres.
UCLASS()
class SURVIVALGAME_API AHomesteadShopkeeper : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadShopkeeper();
    virtual void Tick(float DeltaSeconds) override;
    // Who this is (the label) and the stand-in body; call before Place.
    void SetIdentity(const FString& InFullName, const TCHAR* MeshPath);
    // Where she stands and the way she faces when nobody is near (toward the customer side).
    void Place(const FVector& Location, float RestYaw);
    // Hidden outside opening hours.
    void SetOnDuty(bool bOnDuty);
    bool IsOnDuty() const { return bDuty; }
    static const TCHAR* DisplayName() { return TEXT("Mrs. Pascoe"); }
    static const TCHAR* FullName() { return TEXT("Mrs. Martha Pascoe"); }
    bool IsStandIn() const { return true; }
    const FString& GetFullName() const { return FullNameText; }

private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    float RestYaw = 0.0f;
    bool bDuty = true;
    FString FullNameText = TEXT("Mrs. Martha Pascoe");
    FString StandInMeshPath;
};
