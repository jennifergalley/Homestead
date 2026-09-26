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
class UStaticMesh;
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

enum class EHomesteadKneelGather : uint8 { Sticks, Stones, Pouch };

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
    // Possessed by the character lab controller: no simulation, so movement is never gated by it.
    bool InCharacterLab() const;
    float WalkSpeed() const { return bMetaHumanActive ? 210.0f : 180.0f; }
    float SprintSpeed() const { return bMetaHumanActive ? 480.0f : 300.0f; }
    // Ground speed (cm/s) each locomotion clip covers at play rate 1, so play rate follows speed.
    // Legacy clips were authored for 120/300 cm/s. The MetaHuman uses the Game Animation Sample
    // walk/run loops, measured with homestead_agent.gasp_locomotion after retargeting.
    float WalkClipSpeed() const { return bMetaHumanActive ? 209.9f : 120.0f; }
    float SprintClipSpeed() const { return bMetaHumanActive ? 524.8f : 300.0f; }
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
    // MetaHuman only: the kneeling gather clip for the current kind (sticks and stones share the
    // arm-cradle clip; roots and berries use the hip-pouch clip), or null.
    UAnimSequence* GetGatherSticksAnimation() const
    {
        if (!bMetaHumanActive) return nullptr;
        return KneelKind == EHomesteadKneelGather::Pouch ? GatherPouchAnimation.Get() : GatherSticksAnimation.Get();
    }
    UAnimSequence* GetWaterAnimation() const { return WaterAnimation; }
    UHomesteadWateringTool* GetWateringTool() const { return WateringTool; }
    UAnimSequence* GetClearAnimation() const { return ClearAnimation; }
    UAnimSequence* GetKnifeCutAnimation() const { return KnifeCutAnimation; }
    UAnimSequence* GetTillAnimation() const { return TillAnimation; }
    UHomesteadHatchet* GetHatchet() const { return Hatchet; }
    UHomesteadDiggingStick* GetDiggingStick() const { return DiggingStick; }
    UHomesteadKnife* GetKnife() const { return Knife; }
    void PlayGather();
    // Gathering sticks: the kneeling pickup with carried stick props when available, else PlayGather.
    // Returns true when the kneeling pickup plays. With Pile (world X/Y of the gathered pile) she
    // turns and settles so her hand lands on it.
    bool PlayGatherSticks(TOptional<FVector2D> Pile = {});
    // Kneeling gathers for other forage: stones are cradled in the left arm like sticks, roots and
    // berries are slipped into the hip pouch. Returns true when the kneeling clip plays.
    bool PlayKneelGather(EHomesteadKneelGather Kind, TOptional<FVector2D> Pile = {}, bool bBerries = false);
    EHomesteadKneelGather GetKneelKind() const { return KneelKind; }
    // True from a kneeling stick gather's start until she lifts the last stick off the ground, so the
    // world keeps the gathered pile visible until then.
    bool IsStickPileOnGround() const { return bStickPileOnGround; }
    // Sticks lifted off the pile so far in the current kneeling gather (0-2).
    int32 SticksLiftedFromPile() const { return bStickPileOnGround ? SticksLifted : 2; }
    // World scale of the carried stick props; the woodland's Branches pile uses the same meshes at this scale.
    static constexpr float CarriedStickScale = 0.6f;
    // Loose single stones (Blender prop set HandStones, A-C), or null until they are imported. The
    // woodland's Stones pile and the heroine's carried stones share them.
    static UStaticMesh* LoadHandStone(int32 Index);
    // Width in cm of stone pile part Index (the heroine lifts parts 1 and 2). The MossRocks cluster
    // fallback reads as a scatter of pebbles, so it stays larger.
    static float StonePileSize(int32 Index, bool bHandStone) { return bHandStone ? 13.0f + Index * 3.0f : 24.0f + Index * 4.0f; }
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
    UPROPERTY() TObjectPtr<UAnimSequence> GatherSticksAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> GatherPouchAnimation;
    // Two branch props that appear in her hand and stack on her left forearm during the stick gather.
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> CarriedSticks;
    // Two stones for the stone gather (same meshes and sizes as the woodland's stone pile).
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> CarriedStones;
    // The root or berry cluster in her right hand during a pouch gather.
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CarriedForage;
    UPROPERTY() TObjectPtr<UStaticMesh> ForageBerryMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> ForageRootMesh;
    // The forage pouch on her right hip (shown on the MetaHuman heroine).
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ForagePouch;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CordBelt;
    EHomesteadKneelGather KneelKind = EHomesteadKneelGather::Sticks;
    bool bForageBerries = false;
    int32 StickStage = 0;
    bool bStickPileOnGround = false;
    bool bStickGatherStarted = false;
    int32 SticksLifted = 0;
    void UpdateCarriedSticks();
    static constexpr float StickAlignSeconds = 0.5f;
    FTransform StickAlignFrom, StickAlignTo;
    float StickAlignRemaining = 0;
    void UpdateStickAlignment(float DeltaSeconds);
    UPROPERTY() TObjectPtr<UGroomComponent> MetaHumanHair;
    float HairSprintBlend = 0;
    void UpdateHairMotion(float DeltaSeconds);
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
