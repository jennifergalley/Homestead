#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadMenuPortrait.generated.h"

class AHomesteadCharacter;
class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;

UCLASS(Transient)
class SURVIVALGAME_API AHomesteadMenuPortrait : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadMenuPortrait();
    virtual void Tick(float DeltaSeconds) override;
    bool Refresh(AHomesteadCharacter& Character);
    void Orbit(float Degrees);
    UTextureRenderTarget2D* Texture() const { return Target; }

private:
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Garments;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
    bool bCapturePending = false;
    float Yaw = 0;
    FRotator MeshRotation = FRotator::ZeroRotator;
};
