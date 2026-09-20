#include "HomesteadSmokeTest.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadTestPaths.h"
#include "HomesteadWorld.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool AHomesteadSmokeTest::VerifyPresentationMaterials() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    if (!Avatar) return false;
    const auto* Mesh = Avatar->GetMesh();
    const auto& Look = Controller->GetAppearance();
    const auto* Skin = Mesh->GetMaterial(Mesh->GetMaterialIndex(TEXT("M_Heroine_Skin")));
    const auto* Eyes = Mesh->GetMaterial(Mesh->GetMaterialIndex(TEXT("M_Heroine_LightEyes")));
    if (!Skin || !Eyes || !Skin->GetShadingModels().HasShadingModel(MSM_DefaultLit)
        || !Eyes->GetShadingModels().HasShadingModel(MSM_DefaultLit) || Eyes->GetBlendMode() != BLEND_Masked)
        return false;
    float RejectedParameter = 0, IrisMix = -1;
    if (Skin->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SkinScatter")), RejectedParameter)
        || Eyes->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EyeWetness")), RejectedParameter))
        return false;
    FLinearColor SkinTint, IrisColor;
    return Eyes->GetScalarParameterValue(FMaterialParameterInfo(TEXT("IrisMix")), IrisMix)
        && Skin->GetVectorParameterValue(FMaterialParameterInfo(TEXT("ColorTint")), SkinTint)
        && Eyes->GetVectorParameterValue(FMaterialParameterInfo(TEXT("IrisColor")), IrisColor)
        && FMath::IsNearlyEqual(IrisMix, Look.EyeColor == 0 ? 0.0f : 1.0f)
        && SkinTint.Equals(HomesteadLook::SkinTint(Look.SkinTone), 0.0001f)
        && IrisColor.Equals(HomesteadLook::IrisColor(Look.EyeColor), 0.0001f);
}

void AHomesteadSmokeTest::PreparePresentation()
{
    const bool HairReview = FParse::Param(FCommandLine::Get(), TEXT("HomesteadHairLengthTest"));
    auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    auto* Landscape = Cast<AHomesteadWorld>(UGameplayStatics::GetActorOfClass(this, AHomesteadWorld::StaticClass()));
    auto* Camera = GetWorld()->SpawnActor<ACameraActor>();
    if (!Avatar || !Avatar->HasHeroine() || !Landscape || !Camera)
    {
        Finish(false, TEXT("Presentation fixture requires the real heroine, world and camera."));
        return;
    }
    // A fixed visual fixture, not an ordinary-play route or a simulation acceptance test.
    Controller->SetActorTickEnabled(false);
    if (auto* HUD = Controller->GetHUD()) HUD->bShowHUD = false;
    Avatar->GetCharacterMovement()->StopMovementImmediately();
    Avatar->GetCharacterMovement()->DisableMovement();
    Avatar->SetActorLocation(FVector(-880, 200, AHomesteadWorld::GroundHeight(-880, 200) + 86));
    Avatar->SetActorRotation(FRotator(0, 215, 0));
    Camera->GetCameraComponent()->SetFieldOfView(40);
    Controller->SetViewTarget(Camera);
    FString Description =
        TEXT("Fixed presentation fixture, NOT ordinary gameplay.\n")
        TEXT("Real runtime heroine/materials and unchanged world lighting/exposure.\n")
        TEXT("Controller simulation tick disabled; copied visual state set to hour 12 or 22.\n")
        TEXT("Night uses an ordinary fueled fire at cell -4,0, rotation 2; no extra portrait lights.\n")
        TEXT("Day-to-night transition settles for 18s with unchanged automatic exposure.\n")
        TEXT("Actor=(-880,200,ground+86), yaw=215; camera distance=150cm, FOV=40.\n")
        TEXT("Existing relaxed-idle clip sampled at time zero for reproducible pose.\n")
        TEXT("No player saves, world assets or gameplay settings are modified.\n");
    if (HairReview)
        Description = TEXT("Fixed hair-length fixture, NOT ordinary gameplay.\n")
            TEXT("All six long-wave body/outfit combinations, back and three-quarter, at hour12.\n")
            TEXT("Unchanged world lighting/exposure; controller simulation tick disabled.\n")
            TEXT("Actor=(-880,200,ground+86), yaw=215; camera distance=260cm, FOV=40.\n")
            TEXT("Target=head minus22cm; existing relaxed-idle clip sampled at time zero.\n")
            TEXT("Frame metadata includes shoulder/waist landmarks; no player saves are modified.\n");
    if (!FFileHelper::SaveStringToFile(Description,
        *FPaths::Combine(HomesteadTestOutputDirectory(), HairReview ? TEXT("hair-fixture.txt") : TEXT("presentation-fixture.txt"))))
    {
        Finish(false, TEXT("Could not save presentation fixture metadata."));
        return;
    }
    Results.Add(Description);
    struct FCase { FString Name; double Hour; float Orbit; int32 Skin; int32 Eyes; int32 Body = 0; int32 Outfit = 0; };
    TArray<FCase> Cases = {
        {TEXT("face-day-front"), 12, 0, 0, 0},
        {TEXT("face-day-angle"), 12, 30, 0, 0},
        {TEXT("face-night-front"), 22, 0, 0, 0},
        {TEXT("face-night-angle"), 22, 30, 0, 0},
        {TEXT("face-day-colors"), 12, 0, 2, 2}
    };
    if (HairReview)
    {
        Cases.Reset();
        const TCHAR* Bodies[] = {TEXT("preferred"), TEXT("willow"), TEXT("hazel")};
        for (int32 Body = 0; Body < 3; ++Body)
            for (int32 Outfit = 0; Outfit < 2; ++Outfit)
                for (int32 View = 0; View < 2; ++View)
                    Cases.Add({FString::Printf(TEXT("hair-%s-%s-%s"), Bodies[Body],
                        Outfit ? TEXT("apron") : TEXT("tunic"), View ? TEXT("angle") : TEXT("back")),
                        12, View ? 135.0f : 180.0f, 0, 0, Body, Outfit});
    }
    for (const FCase& Case : Cases)
    {
        const FString Name(Case.Name);
        auto Ready = MakeShared<bool>(false);
        Add(TEXT("Settle fixed presentation: ") + Name,
            [this, Avatar, Landscape, Camera, Case, Ready, HairReview]()
            {
                FHomesteadAppearance Look;
                Look.SkinTone = Case.Skin;
                Look.EyeColor = Case.Eyes;
                Look.BodyPreset = Case.Body;
                Look.Outfit = Case.Outfit;
                *Ready = Avatar->ApplyAppearance(Look);
                if (!*Ready) return;
                auto* Mesh = Avatar->GetMesh();
                Mesh->bPauseAnims = false;
                Mesh->PlayAnimation(Avatar->GetIdleAnimation(), true);
                Mesh->SetPosition(0);
                Mesh->TickAnimation(0, false);
                Mesh->RefreshBoneTransforms();
                Mesh->bPauseAnims = true;
                auto State = Controller->State();
                State.hour = Case.Hour;
                State.structures.clear();
                if (Case.Hour == 22)
                    State.structures.push_back({State.nextId++, Homestead::Piece::Fire, -4, 0, 2, 2, {}});
                Landscape->Refresh(State);
                const FVector Target = Mesh->GetBoneLocation(TEXT("head")) + FVector(0, 0, HairReview ? -22 : 8);
                const FVector Direction = FRotator(0, 215 + Case.Orbit, 0).Vector();
                Camera->SetActorLocation(Target + Direction * (HairReview ? 260 : 150));
                Camera->SetActorRotation((Target - Camera->GetActorLocation()).Rotation());
            },
            [Avatar, Ready, Case]()
            {
                const TCHAR* BodyPrefixes[] = {TEXT(""), TEXT("Willow_"), TEXT("Hazel_")};
                const FString Expected = FString::Printf(TEXT("SK_Heroine_%sLongWave%s"),
                    BodyPrefixes[Case.Body], Case.Outfit ? TEXT("_Apron") : TEXT(""));
                return *Ready && Avatar->GetVelocity().IsNearlyZero()
                    && Avatar->GetMesh()->GetSkeletalMeshAsset()->GetName() == Expected;
            },
            Name == TEXT("face-night-front") ? 18.0f : 6.0f);
        Add(TEXT("Capture fixed presentation: ") + Name,
            [this, Name]() { Screenshot(Name); },
            []() { return true; }, 0.8f);
    }
}
