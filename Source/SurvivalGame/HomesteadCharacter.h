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
namespace Homestead { struct Point; enum class Item : int; }

enum class EHomesteadKneelGather : uint8 { Sticks, Stones, Pouch, Reeds, Plant };

UCLASS()
class SURVIVALGAME_API AHomesteadCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AHomesteadCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
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
    // The garment mesh the MetaHuman heroine shows in a slot (0 top, 1 legs, 2 coat, 3 feet), if any.
    const USkeletalMesh* MetaHumanGarmentMesh(int32 Slot) const;
    // Sole thickness of what she wears on her feet: the mesh stands this much higher (cm).
    float GetFootwearLift() const { return FootwearLift; }
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
    // The view the appearance preview returns to (a spawn placed while it was open).
    void SetRestingViewRotation(const FRotator& Rotation) { SavedViewRotation = Rotation; }
    // While previewing appearance, bring the camera in close on her face (eye choices).
    void SetAppearanceFaceFocus(bool bFace);
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
    // arm-cradle clip; roots and berries use the hip-pouch clip; reeds are sawn free with the
    // knife), or null.
    UAnimSequence* GetGatherSticksAnimation() const
    {
        if (!bMetaHumanActive) return nullptr;
        switch (KneelKind)
        {
        case EHomesteadKneelGather::Pouch: return GatherPouchAnimation.Get();
        case EHomesteadKneelGather::Reeds: return GatherReedsAnimation.Get();
        case EHomesteadKneelGather::Plant: return GatherPlantAnimation.Get();
        default: return GatherSticksAnimation.Get();
        }
    }
    UAnimSequence* GetWaterAnimation() const
    {
        // The carved pail's own clips (homestead_agent.pail_fill / pail_pour) when present.
        if (bFillingPail && PailFillAnimation) return PailFillAnimation;
        if (!bFillingPail && PailPourAnimation) return PailPourAnimation;
        return WaterAnimation;
    }
    // homestead_agent.pail_pour EVENTS (seconds): her left hand takes the pail by its side, the
    // water starts and stops running, and her right fist takes the bail again. SPOT: where the
    // stream lands, ahead of her.
    static constexpr float PailTakeStart = 16.0f / 30.0f;
    static constexpr float PailTakeEnd = 22.0f / 30.0f;
    static constexpr float PailPourStart = 36.0f / 30.0f;
    static constexpr float PailPourStop = 64.0f / 30.0f;
    static constexpr float PailGiveStart = 80.0f / 30.0f;
    static constexpr float PailGiveEnd = 86.0f / 30.0f;
    static constexpr float PourForward = 43.0f;
    static constexpr float PourRight = 0.0f;
    // homestead_agent.pail_fill: the pail goes into the water this far ahead and to her right.
    static constexpr float FillForward = 59.0f;
    static constexpr float FillRight = 12.0f;
    bool IsFillingPail() const { return bFillingPail; }
    UHomesteadWateringTool* GetWateringTool() const { return WateringTool; }
    UAnimSequence* GetClearAnimation() const { return ClearAnimation; }
    UAnimSequence* GetKnifeCutAnimation() const { return KnifeCutAnimation; }
    UAnimSequence* GetTillAnimation() const { return TillAnimation; }
    // Tilling is the two-handed stone hoe clip (MetaHuman with AN_HeroineMH_HoeTill).
    bool UsesHoeTill() const { return bHoeTill; }
    // homestead_agent.hoe_till EVENTS: the first chop turns the soil.
    static constexpr float HoeFirstChop = 31.0f / 30.0f;
    UHomesteadHatchet* GetHatchet() const { return Hatchet; }
    UHomesteadDiggingStick* GetDiggingStick() const { return DiggingStick; }
    UHomesteadKnife* GetKnife() const { return Knife; }
    void PlayGather();
    // Gathering sticks: the kneeling pickup with carried stick props when available, else PlayGather.
    // Returns true when the kneeling pickup plays. With Pile (world X/Y of the gathered pile) she
    // turns and settles so her hand lands on it.
    bool PlayGatherSticks(TOptional<FVector2D> Pile = {});
    // Kneeling gathers for other forage: stones are cradled in the left arm like sticks, roots and
    // berries are slipped into the hip pouch, reeds are gathered in the left fist and sawn through
    // with the knife. Returns true when the kneeling clip plays; false (with no animation for
    // reeds, PlayGather for the rest) when its clip or props are unavailable.
    bool PlayKneelGather(EHomesteadKneelGather Kind, TOptional<FVector2D> Pile = {}, bool bBerries = false);
    EHomesteadKneelGather GetKneelKind() const { return KneelKind; }
    // Kneeling over reeds with the knife (the reed gather is playing).
    bool IsCuttingReeds() const;
    // Kneel and press one seed into the tilled square at Target, then cover it (MetaHuman only;
    // false when the clip is unavailable). IsStickPileOnGround stays true until it is covered.
    bool PlayPlant(Homestead::Point Target);
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
    // Kneel at the bank and fill the pail in the stream at Stream (a point on the water).
    void PlayFillPail(Homestead::Point Stream);
    void PlayClear();
    void PlayClear(Homestead::Point Target);
    void PlayKnifeCut(Homestead::Point Target);
    void PlayTill(Homestead::Point Target);
    // Machete hack through underbrush at Target: two diagonal forehand cuts. False when the
    // clip is unavailable (legacy heroine), so the caller clears at once instead.
    // The billhook (and the scythe, if its mowing clip is missing) reuses the hack with its own
    // held prop; false when that prop is missing.
    bool PlayMacheteHack(Homestead::Point Target);
    bool PlayMacheteHack(Homestead::Point Target, Homestead::Item Tool);
    // Seconds into the hack when the second cut lands and the plant is cleared.
    static constexpr float MacheteClearSeconds = 1.25f;
    UAnimSequence* GetMacheteAnimation() const { return MacheteAnimation; }
    // Two-handed felling with the hatchet (MetaHuman only): Strokes cuts, then she recovers to the
    // carry. False when the clip or the held hatchet is unavailable, so the caller uses PlayClear.
    // TrunkRadius (cm) places her so the bit lands on the trunk's surface: she steps and turns
    // into the felling stance during the address. 0 keeps her where she stands, facing Target.
    bool PlayFell(Homestead::Point Target, int32 Strokes, float TrunkRadius = 0);
    bool CanFell() const;
    // Two-handed work at the ground that shares the felling clip's timing and stroke loop: the
    // pickaxe (rubble, rock) or the axe (stumps, fallen timber) struck down in front of her
    // (AN_HeroineMH_GroundStrike), or the scythe's mowing sweep (AN_HeroineMH_ScytheMow). Radius
    // (cm) places her so the point or bit lands on the target's near side; below 0 (and always for
    // the scythe) she only turns to face Target. False when the clip or the held prop is unavailable.
    bool PlayStrike(Homestead::Point Target, Homestead::Item Tool, int32 Strokes, float Radius);
    bool CanStrike(Homestead::Item Tool) const;
    // The tool the felling-timed clip is swinging (the hatchet unless a strike or mow is playing).
    Homestead::Item FellingTool() const { return FellTool; }
    // At impact in AN_HeroineMH_GroundStrike (ground_strike.bits()): cm to her left and forward.
    static constexpr float StrikePickLeft = 3.3f;
    static constexpr float StrikePickForward = 75.1f;
    static constexpr float StrikeAxeLeft = 0.8f;
    static constexpr float StrikeAxeForward = 58.6f;
    // She is still walking up to the felling stance; the swing has not been requested yet.
    bool IsApproachingFell() const { return bFellApproach; }
    // At impact in AN_HeroineMH_AxeFell (axe_fell.bit_at_strike): the bit's centre relative to her
    // root (cm to her left, cm forward) and its horizontal travel into the trunk (left, forward).
    static constexpr float FellBitLeft = -27.7f;
    static constexpr float FellBitForward = 66.2f;
    static constexpr float FellCutLeft = 0.863f;
    static constexpr float FellCutForward = 0.505f;
    UAnimSequence* GetFellAnimation() const;
    // A berry (or piece of root) from the hip pouch to her mouth (MetaHuman only; false otherwise).
    bool PlayEat(bool bBerry);
    UAnimSequence* GetEatAnimation() const { return EatAnimation; }
    // AN_HeroineMH_CraftHands (homestead_agent.craft_hands): one loop is one craft cycle
    // (the same length as SHomesteadMenu::CraftCycleSeconds).
    UAnimSequence* GetCraftAnimation() const { return CraftAnimation; }
    static constexpr float CraftCycleSeconds = 1.2f;
    // Where she is in the current craft cycle (0-1) while a recipe is held in the field book, or
    // while the lab's `LabAction Craft` runs; -1 otherwise.
    float CraftingPhase() const;
    void PlayLabCraft(int32 Cycles = 3);
    static constexpr float CraftPieceScale = 0.42f;
    // AN_HeroineMH_Eat (homestead_agent.eat_berry EVENTS): food in her fingers, then in her mouth.
    static constexpr float EatPick = 19.0f / 30.0f;
    static constexpr float EatBite = 40.0f / 30.0f;
    // AN_HeroineMH_AxeFell timing (axe_fell.py FRAMES): the clip holds two identical strokes and
    // the cycle from the first rock-free to the second repeats for longer fells.
    static constexpr float FellLoopStart = 44.0f / 30.0f;
    static constexpr float FellLoop = 36.0f / 30.0f;
    static constexpr float FellFirstStrike = 34.0f / 30.0f;
    static float FellPlayLength(float ClipLength, int32 Strokes) { return ClipLength + (Strokes - 2) * FellLoop; }
    static float FellClipTime(float PlayTime, int32 Strokes)
    {
        if (PlayTime < FellLoopStart) return PlayTime;
        if (PlayTime < FellLoopStart + (Strokes - 1) * FellLoop)
            return FellLoopStart + FMath::Fmod(PlayTime - FellLoopStart, FellLoop);
        return PlayTime - (Strokes - 2) * FellLoop;
    }
    // Play time at which stroke Index (0-based) bites into the trunk.
    static float FellStrikeSeconds(int32 Index) { return FellFirstStrike + Index * FellLoop; }
    UStaticMeshComponent* GetHeldMachete() const { return HeldMachete; }
    // The Blender prop shown in her hand while Tool is selected on the hotbar (MetaHuman only), or
    // null when that tool has no authored held prop.
    UStaticMeshComponent* GetHeldProp(Homestead::Item Tool) const;
    // Character lab only (no hotbar there): the tool she carries at rest. Item::Count = none.
    void SetLabHeldTool(Homestead::Item Tool);
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
    UPROPERTY() TObjectPtr<UAnimSequence> GatherReedsAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> GatherPlantAnimation;
    // The pinch of seed in her fingers while she plants.
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CarriedSeed;
    // The cut bundle of reed stems in her left fist after the reed gather's cut.
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CarriedReeds;
    // Two branch props that appear in her hand and stack on her left forearm during the stick gather.
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> CarriedSticks;
    // Two stones for the stone gather (same meshes and sizes as the woodland's stone pile).
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> CarriedStones;
    // The root or berry cluster in her right hand during a pouch gather.
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CarriedForage;
    UPROPERTY() TObjectPtr<UAnimSequence> EatAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> CraftAnimation;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CraftPiece;
    void UpdateCraftPiece(float Weight);
    double LabCraftStart = 0;
    double LabCraftUntil = -1;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EatenFood;
    bool bEatBerry = true;
    bool bEatFoodInHand = false;
    void UpdateEating();
    UPROPERTY() TObjectPtr<UStaticMesh> ForageBerryMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> ForageRootMesh;
    // The forage pouch on her right hip (shown on the MetaHuman heroine).
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ForagePouch;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CordBelt;
    // The pouch hangs from the belt and lies on the outside of her right thigh, so it swings with
    // the thigh (forward and back about the belt, out and in about the hip) once her pose is final.
    FTransform PelvisRefPose;
    FVector PouchPivotRef = FVector::ZeroVector, HipRef = FVector::ZeroVector, ThighDirRef = -FVector::UpVector;
    FDelegateHandle PouchSwingHandle;
    void UpdatePouchSwing();
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
    // The look the MetaHuman stack presents: groom style and hair pigment (ApplyMetaHumanLook).
    FHomesteadAppearance MetaHumanLook, PendingMetaHumanLook;
    int32 AppliedMetaHair = 0;
    void ApplyMetaHumanLook();
    void ApplyMetaHumanSkinAndEyes();
    float HairSprintBlend = 0;
    FTransform HairLastHead;
    double HairLastRealTime = 0;
    bool bHairHasLastHead = false;
    void UpdateHairMotion(float DeltaSeconds);
    UPROPERTY() TObjectPtr<UAnimSequence> WaterAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> PailPourAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> PailFillAnimation;
    bool bFillingPail = false;
    // The water running from the pail's lip while she pours (a thin translucent column).
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PourStream;
    // Two-handed pouring: the pail laid between her fists instead of hanging from the bail.
    void UpdateWaterPail(UStaticMeshComponent& Pail, float DeltaSeconds);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UHomesteadWateringTool> WateringTool;
    UPROPERTY() TObjectPtr<UAnimSequence> ClearAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> KnifeCutAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TillAnimation;
    bool bHoeTill = false;
    UPROPERTY() TObjectPtr<UAnimSequence> MacheteAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> FellAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> StrikeAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> MowAnimation;
    // Which tool (and for the axe, which clip) the felling-timed action swings; set by PlayFell and
    // PlayStrike before any request.
    Homestead::Item FellTool{};
    bool bStrikeHatchet = false;
    bool BeginTwoHanded(Homestead::Point Target, int32 Strokes, float Radius, float BitLeft, float BitForward,
        float CutLeft, float CutForward);
    // The mowing scythe, laid from both fists on its nibs.
    void UpdateMowingScythe(UStaticMeshComponent& Prop, float Weight);
    // Eases her into a work stance (felling, hacking) instead of snapping: a snapped turn flings
    // the simulated hair.
    FVector FellStepFrom = FVector::ZeroVector, FellStepTo = FVector::ZeroVector;
    float FellStepFromYaw = 0, FellStepToYaw = 0;
    float FellStepRemaining = 0;
    void BeginStanceStep(const FVector& To, float Yaw);
    static constexpr float FellStepSeconds = 0.4f;
    // Walking up to a trunk beyond a stance step before the swing starts.
    bool bFellApproach = false;
    FVector2D FellApproachTo = FVector2D::ZeroVector;
    float FellApproachYaw = 0, FellApproachTime = 0;
    int32 FellApproachStrokes = 0;
    void UpdateFellApproach(float DeltaSeconds);
    // The Blender machete, held in the right hand's closed grip (pivot at the grip centre).
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeldMachete;
    // Selected-tool carry props, parallel to HeldToolSpecs.
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> HeldProps;
    struct FHeldToolSpec
    {
        Homestead::Item Tool;
        // Wrist ulnar deviation at rest (FHandGrip carry).
        float CarryDegrees;
        // Hangs plumb from the hand by a bail (the water pail) rather than turning with the wrist.
        bool bHangs;
        // Attachment to hand_r with the handle square across the fingers (as the actions use it).
        FTransform Rest;
    };
    FTransform MacheteGrip;
    // The tool swinging the hack clip.
    Homestead::Item HackTool{};
    // Resting carry: the wrist deviates only this far; the rest of CarryDegrees tips the tool in
    // the fist instead, so the hand stays in line with the forearm.
    static constexpr float RestWristDegrees = 12.0f;
    static constexpr float MacheteCarryDegrees = 22.0f;
    float HeldToolTilt = 0;
    void UpdateFellingHatchet();
    TArray<FHeldToolSpec> HeldToolSpecs;
    TOptional<Homestead::Item> LabHeldTool;
    // The pail's pendulum: tilt (pitch, roll in degrees) and its rate, driven by the hand's motion.
    FVector2D PailSwing = FVector2D::ZeroVector, PailSwingRate = FVector2D::ZeroVector;
    FVector PailHandLast = FVector::ZeroVector, PailHandVelocity = FVector::ZeroVector;
    bool bPailHandValid = false;
    void UpdateHeldTools(float DeltaSeconds);
    void UpdateHangingPail(UStaticMeshComponent& Pail, float DeltaSeconds);
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
    // Worn over the homespun base layer: top, legs, coat and feet (MetaHumanGarmentSlots order).
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<USkeletalMeshComponent>> MetaHumanGarments;
    // Wearable definition per MetaHuman garment slot (-1 when nothing is worn there).
    TArray<int32> MetaHumanWorn, PendingMetaHumanWorn;
    // How far soled footwear lifts her off the ground, in cm.
    float FootwearLift = 0;
    void ApplyMetaHumanGarments();
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
    bool bAppearanceFaceFocus = false;
    float FaceFocusBlend = 0.0f;
    float FaceFocusBodyArm = 280.0f;
    TOptional<float> SavedNearClip;
    bool bSprintHeld = false;
    bool bSprintActive = false;
    float SavedCameraDistance = 470;
    bool bRoomCamera = false;
    float RoomCameraSwitchTime = 0.0f;
    float RoomOpenArm = 470.0f;
    float RoomSetArm = 0.0f;
    bool bHiddenFromCamera = false;
    void UpdateRoomCamera(float DeltaSeconds);
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
    void RestoreNearClip();
    void Move(const FInputActionValue& Value);
    void BeginSprint(const FInputActionValue& Value);
    void EndSprint(const FInputActionValue& Value);
    void MouseLook(const FInputActionValue& Value);
    void StickLook(const FInputActionValue& Value);
    void ZoomInput(const FInputActionValue& Value);
    void ApplyLook(FVector2D Value, float Scale);
    void CreateMappings();
};
