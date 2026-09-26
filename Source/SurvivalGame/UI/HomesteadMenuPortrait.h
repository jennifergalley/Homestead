#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadMenuPortrait.generated.h"

class AHomesteadCharacter;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;

// The field book's character preview: a live capture of the heroine herself, exactly as she stands
// in the world (outfit, hair, pouch, the tool in her hand), cut out from the scene.
UCLASS(Transient)
class SURVIVALGAME_API AHomesteadMenuPortrait : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadMenuPortrait();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool Refresh(AHomesteadCharacter& Character);
    void Orbit(float Degrees);
    void ToggleCloseup();
    // What the book draws: the cut-out composite when available, otherwise the raw colour capture.
    UObject* BrushResource() const;
    UTextureRenderTarget2D* Texture() const { return ColorTarget; }
    float OrbitYaw() const { return Yaw; }
    // How far her idle has played since the book opened (the world keeps animating her).
    TOptional<float> IdlePhase() const;

private:
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> CoverageCapture;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    UPROPERTY() TObjectPtr<UPointLightComponent> FillLight;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> ColorTarget;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> CoverageTarget;
    UPROPERTY() TObjectPtr<UMaterialInterface> CompositeMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Composite;
    TWeakObjectPtr<AHomesteadCharacter> Subject;
    // Her primitives lit by the preview lights while the book is open, restored afterwards.
    TArray<TPair<TWeakObjectPtr<UPrimitiveComponent>, FLightingChannels>> LitParts;
    bool bCapturePending = false;
    float Yaw = 0;
    bool bCloseup = false;
    float AnimatedSeconds = 0;
    float StartPhase = 0;
    FVector SubjectCenter = FVector::ZeroVector;
    FVector SubjectExtent = FVector::ZeroVector;
    void FollowSubject();
    void UpdateCaptureFraming();
    void RestoreLighting();
};
