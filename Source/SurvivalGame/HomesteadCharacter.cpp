#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

AHomesteadCharacter::AHomesteadCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(32.0f, 86.0f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 300, 0);
    GetCharacterMovement()->MaxWalkSpeed = 180.0f;
    GetCharacterMovement()->MaxAcceleration = 700.0f;
    GetCharacterMovement()->MaxStepHeight = 42.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 900.0f;

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 470;
    CameraArm->SocketOffset = FVector(0, 45, 55);
    CameraArm->bUsePawnControlRotation = true;
    CameraArm->bEnableCameraLag = true;
    CameraArm->CameraLagSpeed = 12;
    CameraArm->ProbeSize = 12;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->FieldOfView = 75;

    // An explicit fallback, never a silent substitute for a missing character asset.
    StandIn = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TechnicalStandIn"));
    StandIn->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> StandInMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (StandInMesh.Succeeded())
    {
        StandIn->SetStaticMesh(StandInMesh.Object);
    }
    StandIn->SetRelativeScale3D(FVector(0.42, 0.42, 1.55));
    StandIn->SetRelativeLocation(FVector(0, 0, -8));
    StandIn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WateringTool = CreateDefaultSubobject<UHomesteadWateringTool>(TEXT("ContextualWateringCan"));
    WateringTool->SetupAttachment(GetMesh(), TEXT("hand_r"));
    Hatchet = CreateDefaultSubobject<UHomesteadHatchet>(TEXT("ContextualHatchet"));
    Hatchet->SetupAttachment(GetMesh(), TEXT("hand_r"));
    DiggingStick = CreateDefaultSubobject<UHomesteadDiggingStick>(TEXT("ContextualDiggingStick"));
    DiggingStick->SetupAttachment(GetMesh(), TEXT("hand_r"));
    const FName GarmentNames[] = {TEXT("EquippedTunic"), TEXT("EquippedApron"), TEXT("EquippedFootwear")};
    for (FName Name : GarmentNames)
    {
        auto* Garment = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
        Garment->SetupAttachment(GetMesh());
        Garment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Garment->SetGenerateOverlapEvents(false);
        Garment->bUseAttachParentBound = true;
        Garment->SetVisibility(false);
        GarmentComponents.Add(Garment);
    }
}

void AHomesteadCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (!ActiveEquipment.Ready)
    {
        if (auto* PC = Cast<AHomesteadController>(Controller))
            ApplyAppearance(PC->GetAppearance());
    }
    CreateMappings();
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->AddMappingContext(Mapping, 0);
            }
        }
    }
}

bool AHomesteadCharacter::LoadHeroineAssets()
{
    if (bAttemptedAssetLoad) return bHeroineAssetsValid;
    bAttemptedAssetLoad = true;
    LongHairMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes/SK_Heroine_LongWave.SK_Heroine_LongWave"));
    BobHairMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob/SK_Heroine_Bob.SK_Heroine_Bob"));
    IdleAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_RelaxedIdle.AN_Heroine_RelaxedIdle"));
    WalkAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_GroundedWalk.AN_Heroine_GroundedWalk"));
    GatherAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/SurvivalGame/Characters/Heroine/Animations/AN_Heroine_Gather.AN_Heroine_Gather"));
    WaterAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_WaterRefined.AN_Heroine_WaterRefined"));
    ClearAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_Chop.AN_Heroine_Chop"));
    TillAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_Till.AN_Heroine_Till"));
    if (!LongHairMesh || !BobHairMesh || !IdleAnimation || !WalkAnimation || !GatherAnimation || !WaterAnimation || !ClearAnimation || !TillAnimation)
    {
        UE_LOG(LogTemp, Error, TEXT("Heroine mesh or motion assets are missing. Run Scripts/Build-Game.ps1; the labeled stand-in remains visible."));
        return false;
    }
    if (!LongHairMesh->GetSkeleton() || LongHairMesh->GetSkeleton() != BobHairMesh->GetSkeleton()
        || IdleAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || WalkAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || GatherAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || WaterAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || ClearAnimation->GetSkeleton() != LongHairMesh->GetSkeleton()
        || TillAnimation->GetSkeleton() != LongHairMesh->GetSkeleton())
    {
        UE_LOG(LogTemp, Error, TEXT("Heroine meshes and clips do not share a skeleton."));
        return false;
    }
    WardrobeMeshes = {LongHairMesh, BobHairMesh};
    const TCHAR* MoreMeshes[] = {
        TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail.SK_Heroine_Ponytail"),
        TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes/SK_Heroine_LongWave_Apron.SK_Heroine_LongWave_Apron"),
        TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob/SK_Heroine_Bob_Apron.SK_Heroine_Bob_Apron"),
        TEXT("/Game/SurvivalGame/Characters/Heroine/SK_Heroine_Ponytail_Apron.SK_Heroine_Ponytail_Apron")
    };
    for (const TCHAR* Path : MoreMeshes)
    {
        USkeletalMesh* Variant = LoadObject<USkeletalMesh>(nullptr, Path);
        if (!Variant || Variant->GetSkeleton() != LongHairMesh->GetSkeleton())
        {
            UE_LOG(LogTemp, Error, TEXT("A wardrobe variant is missing or incompatible: %s"), Path);
            return false;
        }
        WardrobeMeshes.Add(Variant);
    }
    const TCHAR* Bodies[] = {TEXT("Willow"), TEXT("Hazel")};
    const TCHAR* Styles[] = {TEXT("LongWave"), TEXT("Bob"), TEXT("Ponytail")};
    for (const TCHAR* Body : Bodies)
    {
        for (int32 Outfit = 0; Outfit < 2; ++Outfit)
        {
            for (const TCHAR* Style : Styles)
            {
                const FString Name = FString::Printf(TEXT("SK_Heroine_%s_%s%s"), Body, Style, Outfit ? TEXT("_Apron") : TEXT(""));
                const TCHAR* Directory = FCString::Strcmp(Style, TEXT("LongWave")) == 0
                    ? TEXT("/Game/Trials/HeroineWave_20260921_01/Meshes")
                    : FCString::Strcmp(Style, TEXT("Bob")) == 0
                        ? TEXT("/Game/SurvivalGame/Characters/ModularClothing/JoinedBob")
                        : TEXT("/Game/SurvivalGame/Characters/Heroine");
                const FString Path = FString::Printf(TEXT("%s/%s.%s"), Directory, *Name, *Name);
                USkeletalMesh* Preset = LoadObject<USkeletalMesh>(nullptr, *Path);
                if (!Preset || Preset->GetSkeleton() != LongHairMesh->GetSkeleton())
                {
                    UE_LOG(LogTemp, Error, TEXT("A complete character preset is missing or incompatible: %s"), *Path);
                    return false;
                }
                WardrobeMeshes.Add(Preset);
            }
        }
    }
    bHeroineAssetsValid = true;
    return true;
}

float AHomesteadCharacter::InferMeshYaw(const USkeletalMesh& Asset) const
{
    const FReferenceSkeleton& Skeleton = Asset.GetRefSkeleton();
    const FName FootNames[] = {TEXT("foot_l"), TEXT("foot.L"), TEXT("LeftFoot")};
    const FName ToeNames[] = {TEXT("ball_l"), TEXT("toe_l"), TEXT("toe.L"), TEXT("LeftToeBase")};
    int32 Foot = INDEX_NONE, Toe = INDEX_NONE;
    for (const FName Name : FootNames) if ((Foot = Skeleton.FindBoneIndex(Name)) != INDEX_NONE) break;
    for (const FName Name : ToeNames) if ((Toe = Skeleton.FindBoneIndex(Name)) != INDEX_NONE) break;
    if (Foot == INDEX_NONE || Toe == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot infer heroine facing from foot/toe bones; verify the imported mesh orientation."));
        return 0;
    }
    auto Position = [&Skeleton](int32 Bone)
    {
        FTransform Transform = Skeleton.GetRefBonePose()[Bone];
        for (int32 Parent = Skeleton.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Skeleton.GetParentIndex(Parent))
            Transform = Transform * Skeleton.GetRefBonePose()[Parent];
        return Transform.GetLocation();
    };
    FVector Forward = Position(Toe) - Position(Foot);
    Forward.Z = 0;
    if (Forward.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Heroine foot reference does not define a horizontal forward axis."));
        return 0;
    }
    return -Forward.Rotation().Yaw;
}

bool AHomesteadCharacter::ApplyAppearance(const FHomesteadAppearance& Appearance)
{
    if (ActiveEquipment.Ready)
    {
        UE_LOG(LogTemp, Error, TEXT("Owned wardrobe appearance requires PrepareEquipment with authoritative state."));
        return false;
    }
    if (!Appearance.IsValid() || !LoadHeroineAssets())
    {
        UE_LOG(LogTemp, Error, TEXT("Appearance change rejected: invalid selection or unavailable assets."));
        return false;
    }
    USkeletalMesh* Desired = WardrobeMeshes[Appearance.BodyPreset * 6 + Appearance.Outfit * 3 + Appearance.HairStyle].Get();
    for (const auto& Slot : Desired->GetMaterials())
    {
        const FString Name = Slot.MaterialSlotName.ToString();
        const UMaterialInterface* Surface = Slot.MaterialInterface;
        if (!Surface) { UE_LOG(LogTemp, Error, TEXT("Character material %s is missing."), *Name); return false; }
        TArray<FMaterialParameterInfo> VectorParameters;
        TArray<FGuid> VectorIds;
        Surface->GetAllVectorParameterInfo(VectorParameters, VectorIds);
        auto HasVector = [&VectorParameters](FName Parameter)
        {
            return VectorParameters.ContainsByPredicate([Parameter](const FMaterialParameterInfo& Info) { return Info.Name == Parameter; });
        };
        const bool NeedsTint = Name == TEXT("M_Heroine_Skin") || Name == TEXT("M_Heroine_Eyebrows")
            || Name == TEXT("M_Heroine_MossLinen") || Name.StartsWith(TEXT("M_Heroine_Hair_"));
        if (NeedsTint && !HasVector(TEXT("ColorTint")))
        {
            UE_LOG(LogTemp, Error, TEXT("Character material %s has no ColorTint parameter. Reimport its material contract."), *Name);
            return false;
        }
        if (Name == TEXT("M_Heroine_LightEyes"))
        {
            TArray<FMaterialParameterInfo> ScalarParameters;
            TArray<FGuid> ScalarIds;
            Surface->GetAllScalarParameterInfo(ScalarParameters, ScalarIds);
            if (!HasVector(TEXT("IrisColor"))
                || !ScalarParameters.ContainsByPredicate([](const FMaterialParameterInfo& Info) { return Info.Name == TEXT("IrisMix"); }))
            {
                UE_LOG(LogTemp, Error, TEXT("Character eye material is missing its iris-only color controls."));
                return false;
            }
        }
    }
    CancelAction();
    USkeletalMeshComponent* VisualMesh = GetMesh();
    if (VisualMesh->GetSkeletalMeshAsset() != Desired)
    {
        VisualMesh->EmptyOverrideMaterials();
        VisualMesh->SetSkeletalMesh(Desired);
        VisualMesh->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
        VisualMesh->SetRelativeRotation(FRotator(0, InferMeshYaw(*Desired), 0));
        VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        VisualMesh->SetAnimInstanceClass(UHomesteadAnimInstance::StaticClass());
        AppearanceMaterials.Reset();
        for (int32 Index = 0; Index < VisualMesh->GetNumMaterials(); ++Index)
            AppearanceMaterials.Add(VisualMesh->CreateDynamicMaterialInstance(Index));
    }
    const TArray<FName> Names = VisualMesh->GetMaterialSlotNames();
    for (int32 Index = 0; Index < Names.Num() && Index < AppearanceMaterials.Num(); ++Index)
    {
        auto* Material = AppearanceMaterials[Index].Get();
        if (!Material) { UE_LOG(LogTemp, Error, TEXT("Heroine material slot %d is unavailable."), Index); return false; }
        const FString Name = Names[Index].ToString();
        if (Name == TEXT("M_Heroine_Skin"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::SkinTint(Appearance.SkinTone));
        else if (Name == TEXT("M_Heroine_Hair_long01_Neutral") || Name == TEXT("M_Heroine_Hair_bob01_Neutral"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::NeutralHairTint(Appearance.HairColor));
        else if (Name.StartsWith(TEXT("M_Heroine_Hair_")) || Name == TEXT("M_Heroine_Eyebrows"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::HairTint(Appearance.HairColor));
        else if (Name == TEXT("M_Heroine_MossLinen"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::TunicTint(Appearance.TunicColor));
        else if (Name == TEXT("M_Heroine_LightEyes"))
        {
            Material->SetVectorParameterValue(TEXT("IrisColor"), HomesteadLook::IrisColor(Appearance.EyeColor));
            Material->SetScalarParameterValue(TEXT("IrisMix"), Appearance.EyeColor == 0 ? 0.0f : 1.0f);
        }
    }
    StandIn->SetVisibility(false);
    VisualMesh->SetVisibility(true);
    bHeroineReady = true;
    return true;
}

bool AHomesteadCharacter::PrepareEquipment(const Homestead::State& CandidateState,
    const FHomesteadAppearance& Look, FString& Error)
{
    ClearPreparedEquipment();
    if (!LoadHeroineAssets())
    {
        Error = TEXT("Original heroine skeleton or animations are unavailable.");
        UE_LOG(LogTemp, Error, TEXT("Wardrobe preparation failed: %s"), *Error);
        return false;
    }
    return HomesteadWardrobePresentation::Prepare(this, CandidateState, Look,
        *LongHairMesh, PreparedEquipment, Error);
}

void AHomesteadCharacter::ClearPreparedEquipment()
{
    PreparedEquipment = {};
}

bool AHomesteadCharacter::ApplyPreparedEquipment(FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !PreparedEquipment.Ready)
    {
        Error = TEXT("No retained wardrobe presentation is ready to apply.");
        UE_LOG(LogTemp, Error, TEXT("Wardrobe apply rejected: %s"), *Error);
        return false;
    }
    CancelAction();
    USkeletalMeshComponent* VisualMesh = GetMesh();
    HomesteadWardrobePresentation::ApplySurface(PreparedEquipment.Base, *VisualMesh);
    VisualMesh->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    VisualMesh->SetRelativeRotation(FRotator(0, InferMeshYaw(*PreparedEquipment.Base.Mesh), 0));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (!VisualMesh->GetAnimInstance() || VisualMesh->GetAnimInstance()->GetClass() != UHomesteadAnimInstance::StaticClass())
        VisualMesh->SetAnimInstanceClass(UHomesteadAnimInstance::StaticClass());
    const int32 Slots[] = {0, 2, 3};
    for (int32 Index = 0; Index < GarmentComponents.Num(); ++Index)
    {
        USkeletalMeshComponent* Component = GarmentComponents[Index];
        const auto* Surface = PreparedEquipment.Garments.FindByPredicate(
            [Slot = Slots[Index]](const FHomesteadEquipmentSurface& Item) { return Item.Slot == Slot; });
        if (Surface)
        {
            HomesteadWardrobePresentation::ApplySurface(*Surface, *Component);
            Component->SetLeaderPoseComponent(VisualMesh, true, false);
            Component->SetVisibility(true);
        }
        else
        {
            Component->SetVisibility(false);
            Component->SetLeaderPoseComponent(nullptr);
            Component->EmptyOverrideMaterials();
            Component->SetSkeletalMesh(nullptr);
        }
    }
    AppearanceMaterials.Reset();
    ActiveEquipment = MoveTemp(PreparedEquipment);
    ClearPreparedEquipment();
    StandIn->SetVisibility(false);
    VisualMesh->SetVisibility(true);
    bHeroineReady = true;
    return true;
}

void AHomesteadCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bAppearancePreview) UpdateAppearanceFraming();
}

FRotator AHomesteadCharacter::ChooseStartingView(const AHomesteadWorld& Landscape, FRotator Preferred)
{
    const FVector Origin = GetActorLocation() + CameraArm->TargetOffset;
    const FVector Focus = GetActorLocation() + FVector(0, 0, 45);
    int32 BestScore = MAX_int32, InitialObstructions = -1, BestObstructions = -1;
    FRotator Best = Preferred;
    for (int32 Index = 0; Index < 24; ++Index)
    {
        const int32 Offset = Index == 0 ? 0 : (Index % 2 ? (Index + 1) / 2 : -Index / 2);
        const FRotator Rotation(Preferred.Pitch, Preferred.Yaw + Offset * 15, 0);
        const FVector Desired = Origin - Rotation.Vector() * CameraArm->TargetArmLength
            + FRotationMatrix(Rotation).TransformVector(CameraArm->SocketOffset);
        FHitResult Hit;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadStartingView), false, this);
        const bool Blocked = GetWorld()->SweepSingleByChannel(Hit, Origin, Desired, FQuat::Identity,
            CameraArm->ProbeChannel, FCollisionShape::MakeSphere(CameraArm->ProbeSize), Query);
        const FVector CameraPosition = Blocked ? Hit.Location : Desired;
        const int32 Obstructions = Landscape.StartingViewObstructions(Focus, CameraPosition);
        if (Index == 0) InitialObstructions = Obstructions;
        const int32 Score = Obstructions * 1000
            + FMath::RoundToInt(FMath::Max(0.0, 250.0 - FVector::Dist(Origin, CameraPosition)));
        if (Score < BestScore)
        {
            BestScore = Score;
            BestObstructions = Obstructions;
            Best = Rotation;
        }
    }
    InitialViewEvidence = FString::Printf(
        TEXT("fresh_start=1 preferred_yaw=%.1f chosen_yaw=%.1f bounds_hits_before=%d bounds_hits_after=%d; bounds heuristic, not pixel visibility"),
        Preferred.Yaw, Best.Yaw, InitialObstructions, BestObstructions);
    return Best;
}

void AHomesteadCharacter::PlayGather()
{
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestGather();
    else
        UE_LOG(LogTemp, Error, TEXT("Gather succeeded but the heroine gathering animation instance is unavailable."));
}

void AHomesteadCharacter::PlayWater()
{
    WaterYaw.Reset();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayWater(Homestead::Point Target)
{
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Water committed without a valid presentation target; using heroine facing."));
        PlayWater();
        return;
    }
    WaterYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::CancelAction()
{
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->CancelAction();
    WateringTool->SetHiddenInGame(true, true);
    Hatchet->SetHiddenInGame(true, true);
    DiggingStick->SetHiddenInGame(true, true);
    ClearYaw.Reset();
    TillYaw.Reset();
    WaterYaw.Reset();
}

void AHomesteadCharacter::PlayClear()
{
    ClearYaw.Reset();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Sapling clearing succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayClear(Homestead::Point Target)
{
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Chop committed without a valid presentation target; using heroine facing."));
        PlayClear();
        return;
    }

    ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Chopping succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayTill(Homestead::Point Target)
{
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Till committed without a valid presentation target; using heroine facing."));
        TillYaw.Reset();
    }
    else
        TillYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestTill();
    else
        UE_LOG(LogTemp, Error, TEXT("Tilling succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::CreateMappings()
{
    if (Mapping) return;
    Mapping = NewObject<UInputMappingContext>(this);
    auto MakeAction = [this](EInputActionValueType Type)
    {
        UInputAction* Action = NewObject<UInputAction>(this);
        Action->ValueType = Type;
        return Action;
    };
    MoveAction = MakeAction(EInputActionValueType::Axis2D);
    MouseLookAction = MakeAction(EInputActionValueType::Axis2D);
    StickLookAction = MakeAction(EInputActionValueType::Axis2D);
    ZoomAction = MakeAction(EInputActionValueType::Axis1D);

    auto AddMove = [this](FKey Key, bool Vertical, bool Negative)
    {
        FEnhancedActionKeyMapping& Entry = Mapping->MapKey(MoveAction, Key);
        if (Negative) Entry.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
        if (Vertical)
        {
            auto* Swizzle = NewObject<UInputModifierSwizzleAxis>(Mapping);
            Swizzle->Order = EInputAxisSwizzle::YXZ;
            Entry.Modifiers.Add(Swizzle);
        }
    };
    AddMove(EKeys::W, true, false);
    AddMove(EKeys::S, true, true);
    AddMove(EKeys::A, false, true);
    AddMove(EKeys::D, false, false);
    auto& MoveStick = Mapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
    MoveStick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
    Mapping->MapKey(MouseLookAction, EKeys::Mouse2D);
    auto& LookStick = Mapping->MapKey(StickLookAction, EKeys::Gamepad_Right2D);
    LookStick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
    Mapping->MapKey(ZoomAction, EKeys::MouseScrollUp);
    Mapping->MapKey(ZoomAction, EKeys::MouseScrollDown).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
}

void AHomesteadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    CreateMappings();
    if (auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::Move);
        Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::MouseLook);
        Input->BindAction(StickLookAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::StickLook);
        Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::ZoomInput);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Homestead requires EnhancedInputComponent; check DefaultInput.ini."));
    }
}

void AHomesteadCharacter::Move(const FInputActionValue& Value)
{
    AHomesteadController* PC = Cast<AHomesteadController>(Controller);
    if (!PC || !PC->IsWorldReady() || PC->IsBookOpen() || PC->IsFailed()) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    if (!Axis.IsNearlyZero()) CancelAction();
    if (bPlanning)
    {
        PC->NudgePlacement(Axis);
        return;
    }
    const FRotator Facing(0, Controller->GetControlRotation().Yaw, 0);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHomesteadCharacter::ApplyLook(FVector2D Value, float Scale)
{
    AHomesteadController* PC = Cast<AHomesteadController>(Controller);
    if (!PC || (PC->IsBookOpen() && PC->BookPage() != 6) || PC->IsFailed()) return;
    AddControllerYawInput(Value.X * Scale * PC->Sensitivity);
    AddControllerPitchInput(Value.Y * Scale * PC->Sensitivity * (PC->bInvertY ? -1.0f : 1.0f));
}

void AHomesteadCharacter::MouseLook(const FInputActionValue& Value)
{
    ApplyLook(Value.Get<FVector2D>(), 0.6f);
}

void AHomesteadCharacter::StickLook(const FInputActionValue& Value)
{
    ApplyLook(Value.Get<FVector2D>(), GetWorld()->GetDeltaSeconds() * 95.0f);
}

void AHomesteadCharacter::SetPlanning(bool Enabled)
{
    bPlanning = Enabled;
    if (Enabled)
    {
        CancelAction();
        GetCharacterMovement()->StopMovementImmediately();
    }
}

void AHomesteadCharacter::Zoom(float Amount)
{
    CameraArm->TargetArmLength = FMath::Clamp(CameraArm->TargetArmLength - Amount * 80.0f, 250.0f, 1000.0f);
}

void AHomesteadCharacter::CycleZoom()
{
    CameraArm->TargetArmLength = bAppearancePreview
        ? (CameraArm->TargetArmLength < 240 ? 320 : 190)
        : (CameraArm->TargetArmLength < 500 ? 740 : 380);
}

void AHomesteadCharacter::ZoomInput(const FInputActionValue& Value)
{
    Zoom(Value.Get<float>());
}

void AHomesteadCharacter::SetAppearancePreview(bool Enabled)
{
    if (Enabled == bAppearancePreview || !Controller) return;
    if (Enabled)
    {
        SavedViewRotation = Controller->GetControlRotation();
        SavedCameraDistance = CameraArm->TargetArmLength;
        CameraArm->TargetArmLength = 280;
        UpdateAppearanceFraming();
        Controller->SetControlRotation(FRotator(-6, GetActorRotation().Yaw + 180, 0));
    }
    else
    {
        CameraArm->TargetArmLength = SavedCameraDistance;
        CameraArm->SocketOffset = FVector(0, 45, 55);
        Camera->PostProcessSettings.bOverride_AutoExposureBias = false;
        Controller->SetControlRotation(SavedViewRotation);
    }
    bAppearancePreview = Enabled;
}

void AHomesteadCharacter::UpdateAppearanceFraming()
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!PC) return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;
    const float Scale = FMath::Clamp(Height / 1080.0f, 0.4f, 3.0f);
    const float VirtualWidth = Width / Scale;
    const float PanelRight = (32.0f + FMath::Min(500.0f, VirtualWidth * 0.35f)) * Scale;
    const float Offset = -(PanelRight / Width) * CameraArm->TargetArmLength
        * FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f));
    CameraArm->SocketOffset = FVector(0, Offset, 10);
    const auto* GameController = Cast<AHomesteadController>(Controller);
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias =
        GameController && GameController->Simulation().IsNight() ? 0.5f : 0.0f;
}

FRotator AHomesteadCharacter::GameplayViewRotation() const
{
    return bAppearancePreview ? SavedViewRotation : (Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator);
}
