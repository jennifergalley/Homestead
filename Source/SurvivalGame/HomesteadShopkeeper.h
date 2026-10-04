#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadShopkeeper.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHomesteadShopkeeper, Log, All);

class UGroomComponent;
class ULODSyncComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

// The general-store shopkeeper: a MetaHuman man in his fifties in shirt, waistcoat and apron
// (metahuman-store-clerk), leaning on the counter in his idle and turning a little toward the
// heroine when she comes near. If his MetaHuman assets are missing he falls back to the legacy
// aproned heroine body (IsStandIn).
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
    static const TCHAR* DisplayName() { return TEXT("Mr. Trethewey"); }
    static const TCHAR* FullName() { return TEXT("Mr. Josiah Trethewey"); }
    bool IsStandIn() const { return !bMetaHuman; }

private:
    // HomesteadShopkeeperMetaHuman.cpp: body, face, grooms, garments, pencil and LOD sync.
    bool BuildMetaHuman();
    void BuildStandIn();

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Face;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Garments;
    UPROPERTY() TArray<TObjectPtr<UGroomComponent>> Grooms;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Pencil;
    UPROPERTY() TObjectPtr<ULODSyncComponent> LODSync;
    float RestYaw = 0.0f;
    bool bDuty = true;
    bool bBuilt = false;
    bool bMetaHuman = false;
};
