#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadHUD.h"
#include "HomesteadWorld.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadTestPaths.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Misc/App.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

AHomesteadSmokeTest::AHomesteadSmokeTest()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
#endif
}

void AHomesteadSmokeTest::Add(const FString& Name, TFunction<void()> Action, TFunction<bool()> Check, float Wait)
{
    FStep Step;
    Step.Name = Name;
    Step.Action = MoveTemp(Action);
    Step.Check = MoveTemp(Check);
    Step.Wait = Wait;
    Steps.Add(MoveTemp(Step));
}

void AHomesteadSmokeTest::Tap(FKey Key)
{
    TraceState(TEXT("INPUT ") + Key.ToString());
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1));
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0));
}

void AHomesteadSmokeTest::TraceState(const FString& Label)
{
    const auto& Look = Controller->GetAppearance();
    FString Line = FString::Printf(TEXT("step=%d %s actor=%s velocity=%s focus=%s page=%d row=%d look=%d,%d,%d,%d,%d,%d,%d hour=%.6f"),
        StepIndex, *Label, *Controller->GetPawn()->GetActorLocation().ToString(),
        *Controller->GetPawn()->GetVelocity().ToString(), *Controller->FocusTitle(),
        Controller->BookPage(), Controller->SelectedRow(), Look.BodyPreset, Look.HairStyle,
        Look.HairColor, Look.SkinTone, Look.EyeColor, Look.TunicColor, Look.Outfit, Controller->State().hour);
    for (const auto& Plot : Controller->State().plots)
        Line += FString::Printf(TEXT(" plot=%d:kind=%d,growth=%.9f,water=%.6f,weeds=%.6f"),
            Plot.id, static_cast<int32>(Plot.kind), Plot.growth, Plot.moisture, Plot.weeds);
    UE_LOG(LogTemp, Display, TEXT("Smoke trace: %s"), *Line);
}

void AHomesteadSmokeTest::Axis(FKey Key, float Value)
{
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Axis, Value, 1));
}

void AHomesteadSmokeTest::Teleport(Homestead::Point Position)
{
    if (auto* Avatar = Cast<ACharacter>(Controller->GetPawn()))
        Avatar->GetCharacterMovement()->StopMovementImmediately();
    FVector Location(Position.x, Position.y, AHomesteadWorld::GroundHeight(Position.x, Position.y) + 100);
    Controller->GetPawn()->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void AHomesteadSmokeTest::QueueHarvest(int32 ResourceId, Homestead::Item ExpectedItem)
{
    Homestead::Point Position;
    for (const auto& Node : Controller->State().resources)
        if (Node.id == ResourceId) Position = Node.position;
    TSharedRef<int32> Before = MakeShared<int32>(0);
    Add(FString::Printf(TEXT("Approach resource %d"), ResourceId),
        [this, Position, Before, ExpectedItem]()
        {
            *Before = Controller->Simulation().Count(ExpectedItem);
            Teleport(Position);
        },
        [this, ResourceId]() { return Controller->IsResourceFocused(ResourceId); }, 0.65f);
    Add(FString::Printf(TEXT("Gather resource %d through gamepad A"), ResourceId),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before, ExpectedItem]() { return Controller->Simulation().Count(ExpectedItem) > *Before; });
}

void AHomesteadSmokeTest::Screenshot(const FString& Name)
{
    IgnoreProfileUntil = FPlatformTime::Seconds() + 0.35;
    const FString Directory = HomesteadTestOutputDirectory();
    IFileManager::Get().MakeDirectory(*Directory, true);
    FVector CameraLocation;
    FRotator CameraRotation;
    int32 Width = 0, Height = 0;
    Controller->GetViewportSize(Width, Height);
    Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);
    FString Framing = FString::Printf(TEXT("viewport=%d %d\nactor=%s\ncamera=%s\ncamera_rotation=%s\npage=%d\n"),
        Width, Height,
        *Controller->GetPawn()->GetActorLocation().ToString(), *CameraLocation.ToString(),
        *CameraRotation.ToString(), Controller->BookPage());
    Framing += FString::Printf(TEXT("prompts_gamepad=%d\nfocus_actions=%s\n"), Controller->UsesGamepad(), *Controller->FocusActions());
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadBookClarityTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("HomesteadVideoSyncTest")))
    {
        if (const auto* HUD = Controller->GetHUD<AHomesteadHUD>())
            Framing += FString::Printf(TEXT("book_text_fits=%d\n%s"), HUD->BookTextFits(Controller->BookPage()), *HUD->BookTextMeasurements());
    }
    if (const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn()))
    {
        const USkeletalMeshComponent* Visual = Avatar->GetMesh();
        FVector2D CenterPixel, HeadPixel;
        const bool CenterVisible = Controller->ProjectWorldLocationToScreen(Visual->Bounds.Origin, CenterPixel);
        const FVector Head = Visual->GetBoneLocation(TEXT("head"));
        const bool HeadVisible = Controller->ProjectWorldLocationToScreen(Head, HeadPixel);
        Framing += FString::Printf(TEXT("mesh_location=%s\nmesh_rotation=%s\nbounds_center=%s\nbounds_extent=%s\ncenter_projected=%d %.1f %.1f\nhead=%s\nhead_projected=%d %.1f %.1f\n"),
            *Visual->GetComponentLocation().ToString(), *Visual->GetComponentRotation().ToString(),
            *Visual->Bounds.Origin.ToString(), *Visual->Bounds.BoxExtent.ToString(),
            CenterVisible, CenterPixel.X, CenterPixel.Y, *Head.ToString(), HeadVisible, HeadPixel.X, HeadPixel.Y);
        const auto* SkeletalAsset = Visual->GetSkeletalMeshAsset();
        Framing += TEXT("mesh_asset=") + (SkeletalAsset ? SkeletalAsset->GetName() : FString(TEXT("none"))) + TEXT("\n");
        for (const FName Bone : {FName(TEXT("neck_01")), FName(TEXT("spine_01")),
            FName(TEXT("upperarm_l")), FName(TEXT("spine_02"))})
        {
            const FVector Point = Visual->GetBoneLocation(Bone);
            FVector2D Pixel;
            const bool Visible = Controller->ProjectWorldLocationToScreen(Point, Pixel);
            Framing += FString::Printf(TEXT("%s=%s\n%s_projected=%d %.1f %.1f\n"),
                *Bone.ToString(), *Point.ToString(), *Bone.ToString(), Visible, Pixel.X, Pixel.Y);
        }
    }
    if (!FFileHelper::SaveStringToFile(Framing, *FPaths::Combine(Directory, Name + TEXT(".frame.txt"))))
        UE_LOG(LogTemp, Error, TEXT("Could not write screenshot framing evidence."));
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, Name + TEXT(".png")), false, false);
}

void AHomesteadSmokeTest::Prepare()
{
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadVideoSyncTest")))
    {
        PrepareVideoSyncChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadBookClarityTest")))
    {
        for (const TCHAR* Other : {TEXT("HomesteadPromptTest"), TEXT("HomesteadClearingTest"),
            TEXT("HomesteadWeedingTest"), TEXT("HomesteadWateringTest"), TEXT("HomesteadGatheringTest"),
            TEXT("HomesteadPresentationTest"), TEXT("HomesteadFullLoop"), TEXT("HomesteadAudioProof")})
            if (FParse::Param(FCommandLine::Get(), Other))
            { Finish(false, TEXT("Book clarity requires its own isolated run.")); return; }
        PrepareBookClarityChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadPromptTest")))
    {
        PreparePromptChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadClearingTest")))
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadWeedingTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadWateringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadGatheringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof")))
        { Finish(false, TEXT("Clearing requires its own isolated run.")); return; }
        PrepareClearingChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadWeedingTest")))
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadWateringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadGatheringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof")))
        {
            Finish(false, TEXT("Weeding requires its own disclosed fixture run."));
            return;
        }
        Add(TEXT("Load explicitly copied existing test-world save, not a fresh-play setup"),
            [this]() { Tap(EKeys::F9); },
            [this]()
            {
                if (Controller->IsBookOpen() || Controller->ToastIsError() || Controller->IsFailed()) return false;
                for (const auto& Plot : Controller->State().plots)
                    if (Plot.planted && Plot.weeds >= 0.125 && Plot.growth < 1) return true;
                return false;
            }, 1.0f);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadWateringTest")))
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadGatheringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof")))
        {
            Finish(false, TEXT("Watering lifecycle requires its own isolated run."));
            return;
        }
        PrepareWateringChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadGatheringTest")))
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof")))
        {
            Finish(false, TEXT("Gathering lifecycle checks require their own isolated run."));
            return;
        }
        PrepareGatheringChecks();
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest")))
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof")))
        {
            Finish(false, TEXT("Presentation fixtures cannot be combined with full-loop or audio acceptance."));
            return;
        }
        PreparePresentation();
        return;
    }
    bAudioCapture = FParse::Param(FCommandLine::Get(), TEXT("HomesteadAudioProof"));
    if (bAudioCapture)
    {
        FApp::SetUnfocusedVolumeMultiplier(1.0f);
        UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 180);
    }
    Add(TEXT("Clothed heroine and compatible animations are loaded"),
        []() {},
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (!Avatar || !Controller->HasHeroine()) return false;
            const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
            const float Width = FVector::Dist2D(Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l")),
                Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r")));
            return Animation && Animation->WalkWeight() < 0.01f && Width > 12 && Width < 24;
        });
    Add(TEXT("Baseline skin and eyes retain Default Lit shading and saved color controls"),
        []() {}, [this]() { return VerifyPresentationMaterials(); });
    Add(TEXT("Initial notes page is open"),
        []() {},
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 3; });
    Add(TEXT("Automation ignores physical-source menu input"),
        [this]()
        {
            for (const FKey Key : {EKeys::Gamepad_Special_Right, EKeys::Escape})
            {
                Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key,
                    IE_Pressed, 1, false, FPlatformTime::Cycles64()));
                Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key,
                    IE_Released, 0, false, FPlatformTime::Cycles64()));
            }
        },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 3; });
    Add(TEXT("Gamepad closes the field book"),
        [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Gamepad movement reaches the character"),
        [this]() { MovementStart = Controller->GetPawn()->GetActorLocation(); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Animation && Animation->WalkWeight() > 0.99f
                && FMath::Abs(Animation->GaitRate() - Avatar->GetVelocity().Size2D() / 120.0f) < 0.03f
                && FVector::Dist2D(MovementStart, Avatar->GetActorLocation()) > 30;
        }, 0.8f);
    Add(TEXT("Gamepad look rotates the camera"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); CameraStart = Controller->GetControlRotation().Yaw; },
        [this]() { return FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw)) > 3; }, 0.6f);
    Add(TEXT("Capture the actual clearing"),
        [this]() { Axis(EKeys::Gamepad_RightX, 0); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Animation && Animation->WalkWeight() < 0.01f && Animation->GaitRate() < 0.01f;
        }, 2.5f);
    Add(TEXT("Capture the settled clearing exposure"),
        [this]() { Screenshot(TEXT("clearing")); },
        []() { return true; }, 1.0f);
    Add(TEXT("Gamepad opens the pack"),
        [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Menu pauses simulation"),
        [this]() { PausedHour = Controller->State().hour; },
        [this]() { return FMath::Abs(Controller->State().hour - PausedHour) < 0.000001; }, 1.0f);
    Add(TEXT("Gamepad bumper navigates to crafting"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->BookPage() == 1 && Controller->Rows().Num() == static_cast<int>(Homestead::Recipe::Count); });
    Add(TEXT("Capture field-book navigation"),
        [this]() { Screenshot(TEXT("field-book")); },
        []() { return true; }, 0.7f);
    Add(TEXT("Keyboard Escape closes the field book"),
        [this]() { Tap(EKeys::Escape); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Open the pack before appearance changes"),
        [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Open the appearance page with the controller"),
        [this]() { CameraStart = Controller->GetControlRotation().Yaw; Tap(EKeys::Gamepad_LeftShoulder); },
        [this]() { return Controller->BookPage() == 6 && Controller->GetAppearance().HairStyle == 0; }, 3.0f);
    Add(TEXT("Capture the long-haired heroine"),
        [this]() { Screenshot(TEXT("heroine-long")); },
        []() { return true; }, 0.8f);
    Add(TEXT("Heroine is framed clear of appearance controls"),
        []() {},
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (!Avatar) return false;
            int32 Width = 0, Height = 0;
            Controller->GetViewportSize(Width, Height);
            if (Width <= 0 || Height <= 0) return false;
            FVector2D Head;
            if (!Controller->ProjectWorldLocationToScreen(Avatar->GetMesh()->GetBoneLocation(TEXT("head")), Head)) return false;
            const float Scale = FMath::Clamp(Height / 1080.0f, 0.4f, 3.0f);
            const float PanelRight = (32 + FMath::Min(500.0f, Width / Scale * 0.35f)) * Scale;
            return Head.X > PanelRight + 30 * Scale && Head.X < Width - 30 * Scale
                && Head.Y > 30 * Scale && Head.Y < Height * 0.65f;
        });
    Add(TEXT("Switch to the bob preset through the appearance menu"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->HasHeroine() && Controller->GetAppearance().HairStyle == 1; }, 0.8f);
    Add(TEXT("Capture the bob-haired heroine"),
        [this]() { Screenshot(TEXT("heroine-bob")); },
        []() { return true; }, 0.8f);
    for (int32 Row = 1; Row <= 4; ++Row)
    {
        Add(FString::Printf(TEXT("Select appearance color row %d"), Row),
            [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this, Row]() { return Controller->SelectedRow() == Row; });
        Add(FString::Printf(TEXT("Change appearance color row %d"), Row),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this, Row]()
            {
                const auto& Look = Controller->GetAppearance();
                const int Value = Row == 1 ? Look.HairColor : Row == 2 ? Look.SkinTone : Row == 3 ? Look.EyeColor : Look.TunicColor;
                return Value == 1 && Controller->HasHeroine();
            });
    }
    Add(TEXT("Capture editable colors"),
        [this]() { Screenshot(TEXT("heroine-colors")); },
        []() { return true; }, 0.8f);
    for (int32 Row = 3; Row >= 0; --Row)
        Add(TEXT("Return to the hairstyle selector"), [this]() { Tap(EKeys::Gamepad_DPad_Up); },
            [this, Row]() { return Controller->SelectedRow() == Row; });
    Add(TEXT("Select the third hairstyle without changing outfit"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->GetAppearance().HairStyle == 2 && Controller->GetAppearance().Outfit == 0
                && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == TEXT("SK_Heroine_Ponytail");
        }, 0.8f);
    Add(TEXT("Capture the ponytail"), [this]() { Screenshot(TEXT("heroine-ponytail")); },
        []() { return true; }, 0.8f);
    Add(TEXT("Cycle back to long hair"), [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().HairStyle == 0; });
    Add(TEXT("Return to the bob"), [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().HairStyle == 1; });
    for (int32 Row = 1; Row <= 5; ++Row)
        Add(TEXT("Navigate to the outfit selector"), [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this, Row]() { return Controller->SelectedRow() == Row; });
    Add(TEXT("Choose the apron without changing the hair selection"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->GetAppearance().Outfit == 1 && Controller->GetAppearance().HairStyle == 1
                && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == TEXT("SK_Heroine_Bob_Apron");
        }, 0.8f);
    Add(TEXT("Capture the apron outfit"), [this]() { Screenshot(TEXT("heroine-apron")); },
        []() { return true; }, 0.8f);
    for (int32 Row = 4; Row >= 0; --Row)
        Add(TEXT("Return to hair while wearing the apron"), [this]() { Tap(EKeys::Gamepad_DPad_Up); },
            [this, Row]() { return Controller->SelectedRow() == Row; });
    const TCHAR* ApronNames[] = {TEXT("SK_Heroine_Ponytail_Apron"), TEXT("SK_Heroine_LongWave_Apron"), TEXT("SK_Heroine_Bob_Apron")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const FString Expected = ApronNames[Index];
        const int32 Style = (Index + 2) % 3;
        Add(TEXT("Each hairstyle keeps its matching apron mesh"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this, Expected, Style]()
            {
                const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                return Avatar && Controller->GetAppearance().Outfit == 1 && Controller->GetAppearance().HairStyle == Style
                    && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == Expected;
            });
    }
    for (int32 Row = 1; Row <= 4; ++Row)
        Add(TEXT("Return to the dye selector"), [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this, Row]() { return Controller->SelectedRow() == Row; });
    int32 CurrentHair = 1;
    int32 CurrentOutfit = 1;
    const TCHAR* BodyNames[] = {TEXT("Preferred"), TEXT("Willow"), TEXT("Hazel")};
    const TCHAR* HairNames[] = {TEXT("LongWave"), TEXT("Bob"), TEXT("Ponytail")};
    for (int32 Body = 1; Body <= 2; ++Body)
    {
        QueueSelectRow(6);
        Add(TEXT("Choose an independently authored face/body preset"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this, Body]() { return Controller->GetAppearance().BodyPreset == Body && Controller->HasHeroine(); });
        for (int32 Outfit = 0; Outfit < 2; ++Outfit)
        {
            if (CurrentOutfit != Outfit)
            {
                QueueSelectRow(5);
                Add(TEXT("Keep the chosen outfit on each body preset"),
                    [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
                    [this, Outfit]() { return Controller->GetAppearance().Outfit == Outfit; });
                CurrentOutfit = Outfit;
            }
            for (int32 Hair = 0; Hair < 3; ++Hair)
            {
                QueueSelectRow(0);
                while (CurrentHair != Hair)
                {
                    CurrentHair = (CurrentHair + 1) % 3;
                    const int32 ExpectedHair = CurrentHair;
                    Add(TEXT("Cycle a hairstyle on the selected body"),
                        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
                        [this, ExpectedHair]() { return Controller->GetAppearance().HairStyle == ExpectedHair; });
                }
                const FString Expected = FString::Printf(TEXT("SK_Heroine_%s_%s%s"), BodyNames[Body], HairNames[Hair], Outfit ? TEXT("_Apron") : TEXT(""));
                Add(TEXT("The selected body, hair and outfit resolve to the correct mesh"),
                    []() {},
                    [this, Expected, Body, Hair, Outfit]()
                    {
                        const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                        const auto& Look = Controller->GetAppearance();
                        return Avatar && Look.BodyPreset == Body && Look.HairStyle == Hair && Look.Outfit == Outfit
                            && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == Expected;
                    });
                if (Hair == 0 && Outfit == 0)
                {
                    const FString Name = Body == 1 ? TEXT("heroine-willow") : TEXT("heroine-hazel");
                    Add(TEXT("Capture the alternate face/body preset"), [this, Name]() { Screenshot(Name); },
                        []() { return true; }, 0.8f);
                }
            }
        }
    }
    QueueSelectRow(6);
    Add(TEXT("Cycle body selection back through the preferred preset"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().BodyPreset == 0; });
    Add(TEXT("Choose Willow for appearance persistence verification"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().BodyPreset == 1; });
    QueueSelectRow(0);
    Add(TEXT("Cycle hair back through long waves"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().HairStyle == 0; });
    Add(TEXT("Restore the bob with the selected body and apron"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().HairStyle == 1 && Controller->GetAppearance().Outfit == 1; });
    QueueSelectRow(4);
    Add(TEXT("Save appearance without saving the temporary portrait camera"),
        [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError(); });
    Add(TEXT("Change the tunic again before restoring"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->GetAppearance().TunicColor == 2; });
    Add(TEXT("Restore appearance and gameplay camera"),
        [this]() { Tap(EKeys::F9); },
        [this]()
        {
            const auto& Look = Controller->GetAppearance();
            return !Controller->IsBookOpen() && Controller->HasHeroine()
                && Look.HairStyle == 1 && Look.HairColor == 1 && Look.SkinTone == 1 && Look.EyeColor == 1 && Look.TunicColor == 1 && Look.Outfit == 1 && Look.BodyPreset == 1
                && FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw)) < 0.1f;
        }, 0.6f);

    int BerryId = -1;
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::BerryBush) { BerryId = Node.id; break; }
    if (BerryId < 0) { Finish(false, TEXT("No berry node exists in the default world.")); return; }
    QueueHarvest(BerryId, Homestead::Item::Berries);
    Add(TEXT("Keyboard opens the pack"),
        [this]() { Tap(EKeys::I); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Select the harvested food"),
        [this]()
        {
            BerriesBeforeFood = Controller->Simulation().Count(Homestead::Item::Berries);
            HungerBeforeFood = Controller->State().hunger;
            Tap(EKeys::Gamepad_DPad_Down);
        },
        [this]()
        {
            const auto Rows = Controller->Rows();
            return Rows.IsValidIndex(Controller->SelectedRow())
                && Rows[Controller->SelectedRow()].Id == static_cast<int>(Homestead::Item::Berries);
        });
    Add(TEXT("Eat forage through the actual inventory control"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]()
        {
            return Controller->Simulation().Count(Homestead::Item::Berries) == BerriesBeforeFood - 1
                && Controller->State().hunger > HungerBeforeFood;
        });
    Add(TEXT("Close pack before saving"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Write a checksummed sandbox save"),
        [this]() { SavedBranches = Controller->Simulation().Count(Homestead::Item::Branch); Tap(EKeys::F5); },
        [this]()
        {
            return IFileManager::Get().FileExists(*FPaths::Combine(HomesteadTestOutputDirectory(),
                TEXT("SmokeSave/Homestead_Manual.sav"))) && !Controller->ToastIsError();
        });
    int BranchId = -1;
    for (const auto& Node : Controller->State().resources)
        if (Node.kind == Homestead::ResourceKind::Branches) { BranchId = Node.id; break; }
    if (BranchId < 0) { Finish(false, TEXT("No branch node exists in the default world.")); return; }
    QueueHarvest(BranchId, Homestead::Item::Branch);
    Add(TEXT("Restore inventory through the Unreal save wrapper"),
        [this]() { Tap(EKeys::F9); },
        [this]()
        {
            return Controller->Simulation().Count(Homestead::Item::Branch) == SavedBranches
                && !Controller->ToastIsError();
        }, 0.6f);

    int Branches = 0, Stones = 0, Reeds = 0;
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind == Homestead::ResourceKind::Branches && Branches < 3)
        {
            QueueHarvest(Node.id, Homestead::Item::Branch);
            ++Branches;
        }
        if (Node.kind == Homestead::ResourceKind::Stones && Stones < 2)
        {
            QueueHarvest(Node.id, Homestead::Item::Stone);
            ++Stones;
        }
        if (Node.kind == Homestead::ResourceKind::Reeds && Reeds < 2)
        {
            QueueHarvest(Node.id, Homestead::Item::Fiber);
            ++Reeds;
        }
    }
    Add(TEXT("Open crafting with the keyboard"),
        [this]() { Tap(EKeys::C); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 1; });
    Add(TEXT("Craft a hatchet using the same gamepad action"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1; });
    Add(TEXT("Close crafting before clearing land"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    for (const auto& Node : Controller->State().resources)
    {
        if (Node.kind != Homestead::ResourceKind::Sapling
            || FMath::FloorToInt(Node.position.x / Homestead::CellSize) != -4
            || FMath::FloorToInt(Node.position.y / Homestead::CellSize) != -2) continue;
        const auto Position = Node.position;
        const int ResourceId = Node.id;
        Add(TEXT("Approach the sapling on the building site"),
            [this, Position]() { Teleport(Position); },
            []() { return true; }, 0.6f);
        Add(TEXT("Clear the building site through gamepad X"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
            [this, ResourceId]()
            {
                for (const auto& Resource : Controller->State().resources)
                    if (Resource.id == ResourceId) return Resource.cleared;
                return false;
            });
    }
    Add(TEXT("Prepare a clear building location"),
        [this]()
        {
            Teleport({-1400, -450});
            Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-15, 0, 0));
            StructuresBeforeBuild = Controller->State().structures.size();
        },
        [this]() { return !Controller->IsBookOpen(); }, 0.6f);
    Add(TEXT("Open the building page"),
        [this]() { Tap(EKeys::B); },
        [this]() { return Controller->BookPage() == 2 && Controller->SelectedRow() == 0; });
    Add(TEXT("Enter construction preview"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsPlanning() && !Controller->IsBookOpen(); });
    Add(TEXT("Commit a foundation from the preview"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->State().structures.size() == static_cast<size_t>(StructuresBeforeBuild + 1); });
    Add(TEXT("Leave construction preview"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning(); });
    Add(TEXT("Capture the first placed foundation"),
        [this]() { Screenshot(TEXT("first-foundation")); },
        []() { return true; }, 1.0f);
}

void AHomesteadSmokeTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    Elapsed += DeltaSeconds;
    if (bCompletionPending)
    {
        Finish(bPendingSuccess, PendingReason);
        return;
    }
    if (Elapsed > 600) { Finish(false, TEXT("Smoke-test timeout.")); return; }
    if (!bStarted)
    {
        Controller = Cast<AHomesteadController>(UGameplayStatics::GetPlayerController(this, 0));
        if (Elapsed < 8 || !Controller || !Controller->GetPawn()) return;
#if WITH_EDITOR
        if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return;
#endif
        bStarted = true;
        LastFrameWallTime = FPlatformTime::Seconds();
        Prepare();
    }
    if (bFinished) return;
    const double Now = FPlatformTime::Seconds();
    if (LastFrameWallTime >= IgnoreProfileUntil && Now > LastFrameWallTime)
        FrameMilliseconds.Add((Now - LastFrameWallTime) * 1000.0);
    LastFrameWallTime = Now;
    if (!Steps.IsValidIndex(StepIndex))
    {
        if (!bBookStoragePrepared && FParse::Param(FCommandLine::Get(), TEXT("HomesteadBookClarityTest")))
        {
            bBookStoragePrepared = true;
            PrepareBookStorageChecks();
        }
        else if (!bWeedingPrepared && FParse::Param(FCommandLine::Get(), TEXT("HomesteadWeedingTest")))
        {
            bWeedingPrepared = true;
            PrepareWeedingChecks();
            if (bFinished) return;
        }
        else if (!bFullLoopPrepared && FParse::Param(FCommandLine::Get(), TEXT("HomesteadFullLoop")))
        {
            bFullLoopPrepared = true;
            PrepareFullLoop();
        }
        else
        {
            Finish(true, FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
                ? TEXT("Fixed presentation fixture completed; not ordinary-play acceptance.")
                : TEXT("All engine integration steps passed."));
            return;
        }
    }
    if (!Steps.IsValidIndex(StepIndex)) { Finish(false, TEXT("Full-loop scenario did not queue any steps.")); return; }
    FStep& Step = Steps[StepIndex];
    if (!bActed)
    {
        if (Step.Skip && Step.Skip()) { ++StepIndex; return; }
        Step.Action();
        TraceState(TEXT("BEGIN ") + Step.Name);
        bActed = true;
        StepElapsed = 0;
        LastNavigationAt = -1;
    }
    if (Step.Name == TEXT("Gamepad movement reaches the character")) Axis(EKeys::Gamepad_LeftY, 1);
    if (Step.Repeat) Step.Repeat();
    if (Step.Name == TEXT("Gamepad look rotates the camera")) Axis(EKeys::Gamepad_RightX, 0.65f);
    if (Step.Name == TEXT("Walk through the cabin doorway")) Axis(EKeys::Gamepad_LeftY, 1);
    StepElapsed += DeltaSeconds;
    bool NavigationComplete = false;
    if (Step.NavigateToId >= 0)
    {
        const auto Rows = Controller->Rows();
        NavigationComplete = Rows.IsValidIndex(Controller->SelectedRow())
            && Rows[Controller->SelectedRow()].Id == Step.NavigateToId;
        if (!NavigationComplete && StepElapsed - LastNavigationAt >= 0.18f)
        {
            Tap(EKeys::Gamepad_DPad_Down);
            LastNavigationAt = StepElapsed;
        }
    }
    if (StepElapsed < Step.Wait && !(NavigationComplete && StepElapsed >= 0.25f)) return;
    if (!Step.Check())
    {
        TraceState(TEXT("FAIL ") + Step.Name);
        Finish(false, Step.Name + TEXT(" | ") + Controller->Toast());
        return;
    }
    const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (StepElapsed >= 0.3f && Animation
        && (Controller->IsBookOpen() || Controller->IsPlanning() || Controller->IsFailed())
        && Animation->ActionWeight() > 0.001f)
    {
        TraceState(TEXT("ACTION FAIL ") + Step.Name);
        Finish(false, Step.Name + TEXT(" | Hand-action pose remained active during a menu, planning or failure."));
        return;
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest")) && !VerifyPresentationMaterials())
    {
        TraceState(TEXT("MATERIAL FAIL ") + Step.Name);
        Finish(false, Step.Name + TEXT(" | Skin/eye material or saved color contract failed."));
        return;
    }
    if (Avatar && Avatar->GetWateringTool()->IsPresented()
        && (!Animation || !Animation->IsWatering() || Controller->IsBookOpen() || Controller->IsPlanning()
            || Controller->IsFailed() || Controller->Simulation().Count(Homestead::Item::WateringCan) == 0))
    {
        Finish(false, Step.Name + TEXT(" | Contextual watering tool remained visible without its valid action."));
        return;
    }
    if (Avatar && Avatar->GetHatchet()->IsPresented()
        && (!Animation || !Animation->IsClearing() || Avatar->GetWateringTool()->IsPresented()
            || Controller->IsBookOpen() || Controller->IsPlanning() || Controller->IsFailed()
            || Controller->Simulation().Count(Homestead::Item::Hatchet) == 0))
    {
        Finish(false, Step.Name + TEXT(" | Contextual hatchet was orphaned or competed with another tool."));
        return;
    }
    Results.Add(TEXT("PASS ") + Step.Name);
    TraceState(TEXT("PASS ") + Step.Name);
    UE_LOG(LogTemp, Display, TEXT("Homestead smoke PASS: %s"), *Step.Name);
    ++StepIndex;
    bActed = false;
}

void AHomesteadSmokeTest::Finish(bool Success, const FString& Reason)
{
    if (bFinished) return;
    if (!bCompletionPending)
    {
        bCompletionPending = true;
        bPendingSuccess = Success;
        PendingReason = Reason;
        CompletionStarted = Elapsed;
        if (bAudioCapture)
        {
            UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile,
                TEXT("game-audio"), HomesteadTestOutputDirectory());
            return;
        }
    }
    if (bAudioCapture && Elapsed - CompletionStarted < 1.5f) return;
    bFinished = true;
    if (Controller)
    {
        Axis(EKeys::Gamepad_LeftY, 0);
        Axis(EKeys::Gamepad_RightX, 0);
        const int32 Probes = FParse::Param(FCommandLine::Get(), TEXT("HomesteadPresentationTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadGatheringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadWateringTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadWeedingTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadClearingTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadPromptTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("HomesteadBookClarityTest")) ? 0 : 4;
        Results.Add(FString::Printf(TEXT("INPUT_ISOLATION ignored_external_events=%u (includes %d deliberate rejection probes)"),
            Controller->IgnoredExternalInputCount(), Probes));
    }
    if (FrameMilliseconds.Num() >= 60)
    {
        double Total = 0;
        for (double Value : FrameMilliseconds) Total += Value;
        FrameMilliseconds.Sort();
        const int32 Last = FrameMilliseconds.Num() - 1;
        Results.Add(FString::Printf(TEXT("PERFORMANCE mean_fps=%.2f p95_frame_ms=%.2f p99_frame_ms=%.2f samples=%d exclusions=startup_and_screenshot_readback"),
            1000.0 / (Total / FrameMilliseconds.Num()),
            FrameMilliseconds[FMath::FloorToInt(Last * 0.95)],
            FrameMilliseconds[FMath::FloorToInt(Last * 0.99)], FrameMilliseconds.Num()));
    }
    Results.Add((Success ? TEXT("SUCCESS ") : TEXT("FAILURE ")) + Reason);
    const FString Directory = HomesteadTestOutputDirectory();
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Report = FString::Join(Results, TEXT("\n")) + TEXT("\n");
    if (!FFileHelper::SaveStringToFile(Report, *FPaths::Combine(Directory, TEXT("smoke-result.txt"))))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not write the smoke-test report."));
        Success = false;
    }
    UE_LOG(LogTemp, Display, TEXT("Homestead smoke result: %s"), *Reason);
    FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}
