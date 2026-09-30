#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "HomesteadLab.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialParameterCollectionInstance.h"

static_assert(AHomesteadCharacter::SprintEnergyFloor == Homestead::Exertion::SprintFloor, "The sprint floor lives in the simulation.");

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
    SprintAction = MakeAction(EInputActionValueType::Boolean);
    ShiftSprintAction = MakeAction(EInputActionValueType::Boolean);
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
    Mapping->MapKey(ShiftSprintAction, EKeys::LeftShift);
    Mapping->MapKey(ShiftSprintAction, EKeys::RightShift);
    Mapping->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
    auto& LookStick = Mapping->MapKey(StickLookAction, EKeys::Gamepad_Right2D);
    LookStick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Mapping));
}

void AHomesteadCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    CreateMappings();
    if (auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHomesteadCharacter::Move);
        Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AHomesteadCharacter::ToggleSprint);
        // In the game the controller reads Shift taps itself (so Shift+Q and Shift+click don't flip
        // sprint); the character lab has no Homestead controller, so Shift's release toggles here.
        Input->BindAction(ShiftSprintAction, ETriggerEvent::Completed, this, &AHomesteadCharacter::LabShiftSprint);
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
    if (!PC && !InCharacterLab()) return;
    if (PC && (!PC->IsWorldReady() || PC->IsBookOpen() || PC->IsShopScreenOpen() || PC->IsFailed())) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    if (!Axis.IsNearlyZero()) CancelAction();
    const FRotator Facing(0, Controller->GetControlRotation().Yaw, 0);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHomesteadCharacter::RequestSprintToggle()
{
    auto* PC = Cast<AHomesteadController>(Controller);
    // Menus, planning, the shop and a failed run own the button; it doesn't flip the toggle there.
    const bool Allowed = PC ? PC->IsWorldReady() && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        && !PC->IsShopScreenOpen() && !bAppearancePreview : InCharacterLab();
    if (!Allowed) return;
    if (bSprintOn) { ResetSprint(); return; }
    if (PC && !PC->Simulation().CanSprint()) { PC->SprintTooTired(); return; }
    bSprintOn = true;
}

void AHomesteadCharacter::LabShiftSprint(const FInputActionValue&)
{
    if (!Cast<AHomesteadController>(Controller)) RequestSprintToggle();
}

void AHomesteadCharacter::CancelSprint()
{
    bSprintActive = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed();
}

void AHomesteadCharacter::ResetSprint()
{
    bSprintOn = false;
    CancelSprint();
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
        CancelSprint();
        CancelAction(true);
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

void AHomesteadCharacter::UpdateRoomCamera(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !Camera || bAppearancePreview) return;
    // Under a roof the full chase arm can't fit: it slams into the ceiling or pins the lens to
    // the wall behind her. Bring it in and down while she's indoors, and give the outdoor
    // distance back when she steps out. The short delay stops a lintel toggling it in the door.
    constexpr float RoomArm = 300.0f;
    const FVector Head = GetActorLocation() + FVector(0, 0, 60);
    FHitResult Hit;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadRoomCamera), false, this);
    const bool Roofed = World->LineTraceSingleByChannel(Hit, Head, Head + FVector(0, 0, 400),
        ECC_Camera, Query);
    RoomCameraSwitchTime = Roofed != bRoomCamera ? RoomCameraSwitchTime + DeltaSeconds : 0.0f;
    if (RoomCameraSwitchTime > 0.3f)
    {
        RoomCameraSwitchTime = 0.0f;
        bRoomCamera = Roofed;
        if (bRoomCamera)
        {
            RoomOpenArm = CameraArm->TargetArmLength;
            RoomSetArm = FMath::Min(RoomOpenArm, RoomArm);
            CameraArm->TargetArmLength = RoomSetArm;
        }
        else if (FMath::IsNearlyEqual(CameraArm->TargetArmLength, RoomSetArm, 1.0f))
            CameraArm->TargetArmLength = RoomOpenArm;
    }
    const FVector Socket = bRoomCamera ? FVector(0, 28, 30) : FVector(0, 45, 55);
    CameraArm->SocketOffset = FMath::VInterpTo(CameraArm->SocketOffset, Socket, DeltaSeconds, 5.0f);
    // When a wall pins the arm short, lift the pivot to head height so the lens looks over her
    // shoulder into the room instead of filling the frame with her back.
    const float Reach = FVector::Dist(Camera->GetComponentLocation(),
        CameraArm->GetComponentLocation() + CameraArm->TargetOffset);
    const float Lift = bRoomCamera ? 75.0f * FMath::Clamp((260.0f - Reach) / 160.0f, 0.0f, 1.0f) : 0.0f;
    CameraArm->TargetOffset.Z = FMath::FInterpTo(CameraArm->TargetOffset.Z, Lift, DeltaSeconds, 4.0f);
    // If a wall still pushes the lens right up against her, don't render her from inside.
    const bool Hide = Reach < (bHiddenFromCamera ? 70.0f : 55.0f);
    if (Hide != bHiddenFromCamera)
    {
        bHiddenFromCamera = Hide;
        TInlineComponentArray<UPrimitiveComponent*> Parts(this);
        for (UPrimitiveComponent* Part : Parts) Part->SetOwnerNoSee(Hide);
    }
}

void AHomesteadCharacter::RestoreNearClip()
{
    if (!SavedNearClip.IsSet()) return;
    SetNearClipPlaneGlobals(SavedNearClip.GetValue());
    SavedNearClip.Reset();
}

bool AHomesteadCharacter::InCharacterLab() const
{
    return Controller && Controller->IsA<AHomesteadLabController>();
}

FRotator AHomesteadCharacter::GameplayViewRotation() const
{
    return bAppearancePreview ? SavedViewRotation : (Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator);
}

float AHomesteadCharacter::CameraDistance() const
{
    return CameraArm->TargetArmLength;
}

void AHomesteadCharacter::SnapCamera()
{
    if (!CameraArm || bAppearancePreview) return;
    // The lagged arm still aims at where she was: its sweep from her new spot back toward the old
    // one hits the ground at her feet and pins the lens against her. Skip the lag for a couple of
    // ticks so the arm re-seats behind her, and start the indoor check afresh.
    if (bRoomCamera && FMath::IsNearlyEqual(CameraArm->TargetArmLength, RoomSetArm, 1.0f))
        CameraArm->TargetArmLength = RoomOpenArm;
    bRoomCamera = false;
    RoomCameraSwitchTime = 0.0f;
    CameraArm->SocketOffset = FVector(0, 45, 55);
    CameraArm->TargetOffset.Z = 0.0f;
    CameraArm->bEnableCameraLag = false;
    CameraSnapFrames = 3;
}
