#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HomesteadAppearance.h"
#include "HomesteadWardrobePresentation.h"
#include "HomesteadCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UHomesteadWateringTool;
class UHomesteadHatchet;
class UHomesteadDiggingStick;
class UHomesteadKnife;
class UGroomComponent;
class ULODSyncComponent;
class AHomesteadWorld;
namespace Homestead { struct Point; }

UCLASS()
class SURVIVALGAME_API AHomesteadCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AHomesteadCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void SetPlanning(bool Enabled);
    void Zoom(float Amount);
    void CycleZoom();
    bool ApplyAppearance(const FHomesteadAppearance& Appearance);
    bool PrepareEquipment(const Homestead::State& CandidateState,
        const FHomesteadAppearance& Look, FString& Error);
    bool ApplyPreparedEquipment(FString& Error);
    void ClearPreparedEquipment();
    bool IsEquipmentPresentationReady() const { return ActiveEquipment.Ready; }
    const FHomesteadEquipmentPresentation* GetEquipmentPresentation() const
    {
        return ActiveEquipment.Ready ? &ActiveEquipment : nullptr;
    }
    bool HasHeroine() const { return bHeroineReady; }
    // The MetaHuman heroine is the default. The legacy heroine is the rollback: homestead.MetaHumanHeroine 0,
    // -HomesteadLegacyHeroine, or smoke/automation tests (their wardrobe contracts cover the legacy stack).
    static bool UsesMetaHumanHeroine();
    bool IsMetaHumanActive() const { return bMetaHumanActive; }
    float WalkSpeed() const { return bMetaHumanActive ? 210.0f : 180.0f; }
    float SprintSpeed() const { return bMetaHumanActive ? 480.0f : 300.0f; }
    // Ground speed (cm/s) each locomotion clip covers at play rate 1, so play rate follows speed.
    // Legacy clips were authored for 120/300 cm/s. The MetaHuman uses the Game Animation Sample
    // walk/run loops, measured with homestead_agent.gasp_locomotion after retargeting.
    float WalkClipSpeed() const { return bMetaHumanActive ? 208.7f : 120.0f; }
    float SprintClipSpeed() const { return bMetaHumanActive ? 521.7f : 300.0f; }
    void SetAppearancePreview(bool Enabled);
    FRotator GameplayViewRotation() const;
    float CameraDistance() const;
    UAnimSequence* GetIdleAnimation() const { return IdleAnimation; }
    UAnimSequence* GetWalkAnimation() const { return WalkAnimation; }
    UAnimSequence* GetSlowWalkAnimation() const { return SlowWalkAnimation; }
    UAnimSequence* GetSprintAnimation() const { return SprintAnimation; }
    bool IsSprinting() const { return bSprintActive; }
    void CancelSprint();
    UAnimSequence* GetGatherAnimation() const { return GatherAnimation; }
    UAnimSequence* GetWaterAnimation() const { return WaterAnimation; }
    UHomesteadWateringTool* GetWateringTool() const { return WateringTool; }
    UAnimSequence* GetClearAnimation() const { return ClearAnimation; }
    UAnimSequence* GetKnifeCutAnimation() const { return KnifeCutAnimation; }
    UAnimSequence* GetTillAnimation() const { return TillAnimation; }
    UHomesteadHatchet* GetHatchet() const { return Hatchet; }
    UHomesteadDiggingStick* GetDiggingStick() const { return DiggingStick; }
    UHomesteadKnife* GetKnife() const { return Knife; }
    void PlayGather();
    void PlayWater();
    void PlayWater(Homestead::Point Target);
    void PlayClear();
    void PlayClear(Homestead::Point Target);
    void PlayKnifeCut(Homestead::Point Target);
    void PlayTill(Homestead::Point Target);
    float ClearTargetYaw() const { return ClearYaw.Get(GetActorRotation().Yaw); }
    float TillTargetYaw() const { return TillYaw.Get(GetActorRotation().Yaw); }
    float WaterTargetYaw() const { return WaterYaw.Get(GetActorRotation().Yaw); }
    void CancelAction(bool Immediate = false);
    FRotator ChooseStartingView(const AHomesteadWorld& Landscape, FRotator Preferred);
    const FString& StartingViewEvidence() const { return InitialViewEvidence; }

private:
    friend class AHomesteadSmokeTest;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StandIn;
    UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
    UPROPERTY() TObjectPtr<UInputAction> MoveAction;
    UPROPERTY() TObjectPtr<UInputAction> SprintAction;
    UPROPERTY() TObjectPtr<UInputAction> MouseLookAction;
    UPROPERTY() TObjectPtr<UInputAction> StickLookAction;
    UPROPERTY() TObjectPtr<UInputAction> ZoomAction;
    UPROPERTY() TObjectPtr<USkeletalMesh> LongHairMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> BobHairMesh;
    UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> WardrobeMeshes;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SlowWalkAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SprintAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> GatherAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> WaterAnimation;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHomesteadWateringTool> WateringTool;
    UPROPERTY() TObjectPtr<UAnimSequence> ClearAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> KnifeCutAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TillAnimation;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHomesteadHatchet> Hatchet;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHomesteadDiggingStick> DiggingStick;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHomesteadKnife> Knife;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> AppearanceMaterials;
    UPROPERTY() TObjectPtr<UMaterialParameterCollection> CameraFoliageParameters;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<USkeletalMeshComponent>> GarmentComponents;
    UPROPERTY() FHomesteadEquipmentPresentation PreparedEquipment;
    UPROPERTY() FHomesteadEquipmentPresentation ActiveEquipment;
    UPROPERTY() TObjectPtr<USkeletalMesh> MetaHumanBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> MetaHumanFace;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> MetaHumanOutfit;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UGroomComponent>> MetaHumanGrooms;
    UPROPERTY(VisibleAnywhere) TObjectPtr<ULODSyncComponent> MetaHumanLODSync;
    bool bMetaHumanActive = false;
    bool bAttemptedMetaHumanLoad = false;
    bool bMetaHumanAssetsValid = false;
    bool bPlanning = false;
    bool bHeroineReady = false;
    bool bAttemptedAssetLoad = false;
    bool bHeroineAssetsValid = false;
    bool bAppearancePreview = false;
    bool bSprintHeld = false;
    bool bSprintActive = false;
    float SavedCameraDistance = 470;
    FRotator SavedViewRotation = FRotator::ZeroRotator;
    FString InitialViewEvidence = TEXT("Saved/manual view; no fresh-start selection recorded.");
    TOptional<float> ClearYaw;
    TOptional<float> TillYaw;
    TOptional<float> WaterYaw;

    bool LoadHeroineAssets();
    bool LoadMetaHumanStack();
    bool ApplyMetaHumanStack();
    float InferMeshYaw(const USkeletalMesh& Asset) const;
    void UpdateAppearanceFraming();
    void Move(const FInputActionValue& Value);
    void BeginSprint(const FInputActionValue& Value);
    void EndSprint(const FInputActionValue& Value);
    void MouseLook(const FInputActionValue& Value);
    void StickLook(const FInputActionValue& Value);
    void ZoomInput(const FInputActionValue& Value);
    void ApplyLook(FVector2D Value, float Scale);
    void CreateMappings();
};
