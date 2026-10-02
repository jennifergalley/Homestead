#include "HomesteadAnimInspector.h"

#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadLab.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ReferenceSkeleton.h"
#include "Serialization/Archive.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadAnimInspector, Log, All);

namespace AnimInspectorTuning
{
// The step every recorded frame advances (the clips are authored at 30 fps).
constexpr float StepSeconds = 1.0f / 30.0f;
// Real seconds the lab gets to settle (spawn, ground, first shaders) before the action starts.
constexpr float SettleSeconds = 4.0f;
// Frames recorded after the action ends, so the settle into idle is reviewed too.
constexpr int32 TailFrames = 8;
// A clip that never ends (a looping layer, a failed request) stops here: 30 s at 30 fps.
constexpr int32 MaxFrames = 900;
// Orthographic views frame a 240 cm square around her middle; the top view is a little wider for
// a swing's arc. Cameras sit far enough out that perspective views don't clip her reach.
constexpr float OrthoWidthCm = 240.0f;
constexpr float TopWidthCm = 300.0f;
constexpr float CameraDistanceCm = 600.0f;
constexpr float ThreeQuarterDistanceCm = 380.0f;
constexpr float ThreeQuarterFov = 40.0f;
// The views' centre height above her capsule's base (cm), about her navel.
constexpr float CentreHeightCm = 95.0f;
// How far down to look for the ground under each foot (cm).
constexpr float GroundTraceCm = 200.0f;

FString Vec(const FVector& V) { return FString::Printf(TEXT("[%.3f,%.3f,%.3f]"), V.X, V.Y, V.Z); }
FString Quat(const FQuat& Q) { return FString::Printf(TEXT("[%.6f,%.6f,%.6f,%.6f]"), Q.X, Q.Y, Q.Z, Q.W); }
FString Xform(const FTransform& T)
{
    const FVector L = T.GetLocation();
    const FQuat Q = T.GetRotation();
    return FString::Printf(TEXT("[%.3f,%.3f,%.3f,%.6f,%.6f,%.6f,%.6f]"), L.X, L.Y, L.Z, Q.X, Q.Y, Q.Z, Q.W);
}
}

bool HomesteadAnimInspector::Requested()
{
    FString Action;
    return FParse::Value(FCommandLine::Get(), TEXT("HomesteadAnimInspector="), Action) && !Action.IsEmpty();
}

AHomesteadAnimInspector::AHomesteadAnimInspector()
{
    PrimaryActorTick.bCanEverTick = true;
    // After the character and its animation, so each frame records the pose this step produced.
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("InspectorRoot"));
    const TCHAR* Command = FCommandLine::Get();
    FParse::Value(Command, TEXT("HomesteadAnimInspector="), Action);
    FParse::Value(Command, TEXT("HomesteadAnimInspectorHold="), Hold);
    FParse::Value(Command, TEXT("HomesteadAnimInspectorOut="), OutDir);
    FParse::Value(Command, TEXT("HomesteadAnimInspectorEvery="), Every);
    FParse::Value(Command, TEXT("HomesteadAnimInspectorResolution="), Resolution);
    Every = FMath::Clamp(Every, 1, 30);
    Resolution = FMath::Clamp(Resolution, 256, 2048);
    bLit = FParse::Param(Command, TEXT("HomesteadAnimInspectorLit"));
    bExit = FParse::Param(Command, TEXT("HomesteadAnimInspectorExit"));
    if (OutDir.IsEmpty()) OutDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AnimInspector"), Action);
    FString ViewList = TEXT("front,left,right,top,threequarter");
    FParse::Value(Command, TEXT("HomesteadAnimInspectorViews="), ViewList, /*bShouldStopOnSeparator=*/false);
    TArray<FString> Names;
    ViewList.ParseIntoArray(Names, TEXT(","));
    for (const FString& Name : Names)
    {
        FView View;
        View.Name = Name.TrimStartAndEnd().ToLower();
        if (View.Name == TEXT("front") || View.Name == TEXT("left") || View.Name == TEXT("right")
            || View.Name == TEXT("top") || View.Name == TEXT("threequarter"))
            Views.Add(View);
    }
}

bool AHomesteadAnimInspector::IsActionBusy(const AHomesteadCharacter& Avatar) const
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar.GetMesh()->GetAnimInstance());
    if (!Animation) return false;
    return Animation->IsHandActionBusy() || Animation->IsEating() || Animation->CraftWeight() > 0.01f
        || Animation->IsLampKneeling() || Avatar.IsApproachingFell();
}

void AHomesteadAnimInspector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Phase == EPhase::Done) return;
    Elapsed += DeltaSeconds;
    auto* Lab = Cast<AHomesteadLabController>(UGameplayStatics::GetPlayerController(this, 0));
    auto* Avatar = Lab ? Cast<AHomesteadCharacter>(Lab->GetPawn()) : nullptr;
    if (Phase == EPhase::Settle)
    {
        if (!Lab || !Avatar || Elapsed < AnimInspectorTuning::SettleSeconds) return;
#if WITH_EDITOR
        if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return;
#endif
        if (!Avatar->IsMetaHumanActive())
        {
            Finish(TEXT("The MetaHuman heroine isn't active; start with -HomesteadMetaHuman."));
            return;
        }
        Begin(*Avatar, *Lab);
        return;
    }
    if (!Avatar)
    {
        Finish(TEXT("The heroine went away mid-recording."));
        return;
    }
    Record(*Avatar);
    const bool bBusy = IsActionBusy(*Avatar);
    bSeenBusy |= bBusy;
    IdleFrames = bBusy ? 0 : IdleFrames + 1;
    if (bSeenBusy && IdleFrames > AnimInspectorTuning::TailFrames) Finish(TEXT("The action finished."));
    else if (!bSeenBusy && Frame > 60) Finish(TEXT("The action never started (unknown LabAction name, or it can't play)."));
    else if (Frame >= AnimInspectorTuning::MaxFrames) Finish(TEXT("Stopped at the frame limit."));
}

void AHomesteadAnimInspector::Begin(AHomesteadCharacter& Avatar, AHomesteadLabController& Lab)
{
    IFileManager::Get().MakeDirectory(*OutDir, true);
    const FVector Base = Avatar.GetActorLocation() - FVector(0, 0, Avatar.GetSimpleCollisionHalfHeight());
    const FVector Centre = Base + FVector(0, 0, AnimInspectorTuning::CentreHeightCm);
    const FVector Forward = Avatar.GetActorForwardVector().GetSafeNormal2D();
    const FVector Left = -Avatar.GetActorRightVector().GetSafeNormal2D();
    const float Yaw = Avatar.GetActorRotation().Yaw;
    for (FView& View : Views)
    {
        FVector From = Centre;
        if (View.Name == TEXT("front")) From = Centre + Forward * AnimInspectorTuning::CameraDistanceCm;
        else if (View.Name == TEXT("left")) From = Centre + Left * AnimInspectorTuning::CameraDistanceCm;
        else if (View.Name == TEXT("right")) From = Centre - Left * AnimInspectorTuning::CameraDistanceCm;
        else if (View.Name == TEXT("top")) From = Centre + FVector(0, 0, AnimInspectorTuning::CameraDistanceCm);
        else From = Centre + (Forward + Left).GetSafeNormal() * AnimInspectorTuning::ThreeQuarterDistanceCm + FVector(0, 0, 60);
        View.Location = From;
        View.Rotation = (Centre - From).Rotation();
        // Looking straight down, keep her forward pointing up the image.
        if (View.Name == TEXT("top")) View.Rotation = FRotator(-90.0f, Yaw, 0.0f);
        View.bOrtho = View.Name != TEXT("threequarter");
        View.Width = View.Name == TEXT("top") ? AnimInspectorTuning::TopWidthCm : AnimInspectorTuning::OrthoWidthCm;
        View.Fov = AnimInspectorTuning::ThreeQuarterFov;

        auto* Target = NewObject<UTextureRenderTarget2D>(this);
        Target->RenderTargetFormat = RTF_RGBA8;
        Target->ClearColor = FLinearColor(0.42f, 0.42f, 0.4f);
        Target->InitAutoFormat(Resolution, Resolution);
        Target->UpdateResourceImmediate(true);
        auto* Capture = NewObject<USceneCaptureComponent2D>(this);
        Capture->SetupAttachment(RootComponent);
        Capture->SetWorldLocationAndRotation(View.Location, View.Rotation);
        Capture->ProjectionType = View.bOrtho ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
        Capture->OrthoWidth = View.Width;
        Capture->FOVAngle = View.Fov;
        Capture->TextureTarget = Target;
        Capture->bCaptureEveryFrame = false;
        Capture->bCaptureOnMovement = false;
        Capture->bAlwaysPersistRenderingState = true;
        // Base colour reads the same at any exposure; -HomesteadAnimInspectorLit captures the lit scene
        // at a fixed daylight exposure instead.
        Capture->CaptureSource = bLit ? ESceneCaptureSource::SCS_FinalColorLDR : ESceneCaptureSource::SCS_BaseColor;
        if (bLit)
        {
            Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
            Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
            Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
            Capture->PostProcessSettings.AutoExposureBias = 14.0f;
        }
        Capture->RegisterComponent();
        Captures.Add(Capture);
        Targets.Add(Target);
        IFileManager::Get().MakeDirectory(*FPaths::Combine(OutDir, View.Name), true);
    }
    WriteCameras();
    WriteNeutral(Avatar);

    // Every frame from here is exactly one 30 fps step, however long capturing it takes.
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(AnimInspectorTuning::StepSeconds);
    if (!Hold.IsEmpty()) Lab.LabHold(Hold);
    Lab.LabAction(Action);
    Phase = EPhase::Record;
    FramesJson = FString::Printf(TEXT("{\"action\":\"%s\",\"fps\":30,\"every\":%d,\"bones\":["), *Action, Every);
    for (int32 Index = 0; Index < Bones.Num(); ++Index)
        FramesJson += FString::Printf(TEXT("%s\"%s\""), Index ? TEXT(",") : TEXT(""), *Bones[Index].ToString());
    FramesJson += TEXT("],\"frames\":[\n");
    UE_LOG(LogHomesteadAnimInspector, Display, TEXT("ANIM_INSPECTOR recording %s (%d views, every %d frames) to %s"),
        *Action, Views.Num(), Every, *OutDir);
}

void AHomesteadAnimInspector::WriteNeutral(const AHomesteadCharacter& Avatar)
{
    const USkeletalMeshComponent* Mesh = Avatar.GetMesh();
    const USkinnedAsset* Asset = Mesh ? Mesh->GetSkinnedAsset() : nullptr;
    if (!Asset) return;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    const TArray<FTransform>& Local = Ref.GetRefBonePose();
    TArray<FTransform> Component;
    Component.SetNum(Local.Num());
    Bones.Reset();
    FString Json = TEXT("{\"bones\":{");
    for (int32 Index = 0; Index < Local.Num(); ++Index)
    {
        const int32 Parent = Ref.GetParentIndex(Index);
        Component[Index] = Parent == INDEX_NONE ? Local[Index] : Local[Index] * Component[Parent];
        Bones.Add(Ref.GetBoneName(Index));
        Json += FString::Printf(TEXT("%s\"%s\":%s"), Index ? TEXT(",") : TEXT(""), *Ref.GetBoneName(Index).ToString(),
            *AnimInspectorTuning::Xform(Component[Index]));
    }
    Json += TEXT("},\"parents\":{");
    for (int32 Index = 0; Index < Local.Num(); ++Index)
    {
        const int32 Parent = Ref.GetParentIndex(Index);
        Json += FString::Printf(TEXT("%s\"%s\":\"%s\""), Index ? TEXT(",") : TEXT(""), *Ref.GetBoneName(Index).ToString(),
            Parent == INDEX_NONE ? TEXT("") : *Ref.GetBoneName(Parent).ToString());
    }
    Json += TEXT("}}\n");
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(OutDir, TEXT("neutral.json")));
}

void AHomesteadAnimInspector::WriteCameras() const
{
    FString Json = FString::Printf(TEXT("{\"resolution\":%d,\"views\":["), Resolution);
    for (int32 Index = 0; Index < Views.Num(); ++Index)
    {
        const FView& View = Views[Index];
        const FRotationMatrix Basis(View.Rotation);
        Json += FString::Printf(TEXT("%s{\"name\":\"%s\",\"ortho\":%s,\"width\":%.3f,\"fov\":%.3f,\"location\":%s,")
            TEXT("\"forward\":%s,\"right\":%s,\"up\":%s}"),
            Index ? TEXT(",") : TEXT(""), *View.Name, View.bOrtho ? TEXT("true") : TEXT("false"), View.Width, View.Fov,
            *AnimInspectorTuning::Vec(View.Location), *AnimInspectorTuning::Vec(Basis.GetScaledAxis(EAxis::X)),
            *AnimInspectorTuning::Vec(Basis.GetScaledAxis(EAxis::Y)), *AnimInspectorTuning::Vec(Basis.GetScaledAxis(EAxis::Z)));
    }
    Json += TEXT("]}\n");
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(OutDir, TEXT("cameras.json")));
}

void AHomesteadAnimInspector::Record(AHomesteadCharacter& Avatar)
{
    USkeletalMeshComponent* Mesh = Avatar.GetMesh();
    const TArray<FTransform>& Pose = Mesh->GetComponentSpaceTransforms();
    const auto* Animation = Cast<UHomesteadAnimInstance>(Mesh->GetAnimInstance());
    FString Line = FString::Printf(TEXT("%s{\"i\":%d,\"t\":%.4f,\"busy\":%s,\"weight\":%.3f,\"mesh\":%s,\"pose\":["),
        Frame ? TEXT(",\n") : TEXT(""), Frame, Frame * AnimInspectorTuning::StepSeconds,
        IsActionBusy(Avatar) ? TEXT("true") : TEXT("false"), Animation ? Animation->ActionWeight() : 0.0f,
        *AnimInspectorTuning::Xform(Mesh->GetComponentTransform()));
    for (int32 Index = 0; Index < Bones.Num(); ++Index)
        Line += (Index ? TEXT(",") : TEXT("")) + (Pose.IsValidIndex(Index) ? AnimInspectorTuning::Xform(Pose[Index]) : FString(TEXT("null")));
    // Everything she holds: visible static meshes attached under her body (tools, pail, produce).
    Line += TEXT("],\"props\":[");
    TArray<UStaticMeshComponent*> Parts;
    Avatar.GetComponents<UStaticMeshComponent>(Parts);
    bool bFirst = true;
    for (const UStaticMeshComponent* Part : Parts)
    {
        if (!Part || !Part->IsVisible() || !Part->GetStaticMesh() || !Part->IsAttachedTo(Mesh)) continue;
        Line += FString::Printf(TEXT("%s{\"name\":\"%s\",\"mesh\":\"%s\",\"xform\":%s,\"centre\":%s,\"extent\":%s}"),
            bFirst ? TEXT("") : TEXT(","), *Part->GetName(), *Part->GetStaticMesh()->GetName(),
            *AnimInspectorTuning::Xform(Part->GetComponentTransform()), *AnimInspectorTuning::Vec(Part->Bounds.Origin),
            *AnimInspectorTuning::Vec(Part->Bounds.BoxExtent));
        bFirst = false;
    }
    // The floor under each foot, so penetration and slides are judged against the real ground.
    Line += TEXT("],\"ground\":{");
    bFirst = true;
    for (const TCHAR* Foot : {TEXT("foot_l"), TEXT("foot_r"), TEXT("ball_l"), TEXT("ball_r")})
    {
        const FVector At = Mesh->GetSocketLocation(Foot);
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadAnimInspectorGround), false, &Avatar);
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 50),
            At - FVector(0, 0, AnimInspectorTuning::GroundTraceCm), ECC_Visibility, Query);
        Line += FString::Printf(TEXT("%s\"%s\":%s"), bFirst ? TEXT("") : TEXT(","), Foot,
            bHit ? *FString::Printf(TEXT("%.3f"), Hit.ImpactPoint.Z) : TEXT("null"));
        bFirst = false;
    }
    Line += TEXT("}}");
    FramesJson += Line;
    if (Frame % Every == 0) Capture(Frame);
    ++Frame;
}

void AHomesteadAnimInspector::Capture(int32 Index)
{
    for (int32 View = 0; View < Captures.Num(); ++View)
    {
        Captures[View]->CaptureScene();
        const FString Path = FPaths::Combine(OutDir, Views[View].Name, FString::Printf(TEXT("f%04d.png"), Index));
        TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*Path));
        if (!File || !FImageUtils::ExportRenderTarget2DAsPNG(Targets[View], *File))
            UE_LOG(LogHomesteadAnimInspector, Warning, TEXT("Couldn't write %s"), *Path);
    }
}

void AHomesteadAnimInspector::Finish(const FString& Reason)
{
    if (Phase == EPhase::Done) return;
    const bool bRecorded = Phase == EPhase::Record;
    Phase = EPhase::Done;
    FApp::SetUseFixedTimeStep(false);
    if (bRecorded)
    {
        FramesJson += TEXT("\n]}\n");
        FFileHelper::SaveStringToFile(FramesJson, *FPaths::Combine(OutDir, TEXT("frames.json")));
    }
    UE_LOG(LogHomesteadAnimInspector, Display, TEXT("ANIM_INSPECTOR done: %s %d frames to %s"), *Reason, Frame, *OutDir);
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("%s\nframes=%d\n"), *Reason, Frame),
        *FPaths::Combine(OutDir, TEXT("result.txt")));
    if (bExit) FPlatformMisc::RequestExit(false, TEXT("HomesteadAnimInspector"));
}

void AHomesteadAnimInspector::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Phase != EPhase::Done) FApp::SetUseFixedTimeStep(false);
    Super::EndPlay(Reason);
}
