// Hair investigation aids for the heroine's MetaHuman grooms (Jenny's playtest, 2026-09-29: a rod and a
// fan of strands sticking out of the bob and the updo). Development tools, not gameplay:
//   homestead.HairStyle <0-8>     wear hairstyle N (HomesteadLook::MetaHairStyles order)
//   homestead.HairOverride <0|1>  force the long groom's simulation override on or off (A/B the fix)
//   homestead.HairLOD <n|-1>       force the hair's groom LOD (-1 follows the body again)
//   homestead.HairExtent          how far her hair reaches from her head right now (cm), side and top
//   homestead.HairWatch <seconds> sample HairExtent every 0.2 s and log the worst
// The extent is measured by capturing only the hair groom through a narrow lens from 20 m, so pixels
// map to centimetres in the plane through her head; a strand shoved off the scalp shows as a long reach.
#include "HomesteadCharacter.h"

// Development only: none of this is compiled into Shipping builds.
#if !UE_BUILD_SHIPPING

#include "Components/SceneCaptureComponent2D.h"
#include "Containers/Ticker.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadHairLab, Log, All);

namespace HairLab
{
constexpr int32 CaptureSize = 256;
// The capture covers this width (cm) in the plane through her head.
constexpr float CaptureWidthCm = 400.0f;
constexpr float CaptureDistanceCm = 2000.0f;
constexpr float SampleSeconds = 0.2f;

AHomesteadCharacter* Heroine(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AHomesteadCharacter> It(World); It; ++It) return *It;
    return nullptr;
}

UGroomComponent* Hair(AHomesteadCharacter* Avatar)
{
    TArray<UGroomComponent*> Grooms;
    if (Avatar) Avatar->GetComponents(Grooms);
    for (UGroomComponent* Groom : Grooms)
        if (Groom && Groom->GetName() == TEXT("MetaHumanHair")) return Groom;
    return nullptr;
}

FVector HeadLocation(AHomesteadCharacter* Avatar)
{
    TArray<USkeletalMeshComponent*> Meshes;
    Avatar->GetComponents(Meshes);
    for (USkeletalMeshComponent* Mesh : Meshes)
        if (Mesh && Mesh->GetBoneIndex(TEXT("head")) != INDEX_NONE) return Mesh->GetSocketLocation(TEXT("head"));
    return Avatar->GetActorLocation() + FVector(0, 0, 70);
}

// One capture through the narrow lens toward her head from ViewDir, showing only Shown (or nothing:
// just the sky and background).
bool Capture(AHomesteadCharacter* Avatar, UPrimitiveComponent* Shown, const FVector& ViewDir, TArray<FColor>& Colors)
{
    auto* Scene = NewObject<USceneCaptureComponent2D>(Avatar);
    auto* Target = NewObject<UTextureRenderTarget2D>(Scene);
    Target->ClearColor = FLinearColor::Black;
    Target->InitCustomFormat(CaptureSize, CaptureSize, PF_B8G8R8A8, false);
    Scene->TextureTarget = Target;
    Scene->bCaptureEveryFrame = false;
    Scene->bCaptureOnMovement = false;
    Scene->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Scene->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    if (Shown) Scene->ShowOnlyComponents.Add(Shown);
    Scene->ShowFlags.SetAtmosphere(false);
    Scene->ShowFlags.SetFog(false);
    Scene->ShowFlags.SetVolumetricFog(false);
    Scene->ShowFlags.SetBloom(false);
    Scene->ShowFlags.SetMotionBlur(false);
    Scene->ShowFlags.SetTemporalAA(false);
    Scene->FOVAngle = FMath::RadiansToDegrees(2.0f * FMath::Atan(0.5f * CaptureWidthCm / CaptureDistanceCm));
    const FVector Head = HeadLocation(Avatar);
    const FVector Dir = ViewDir.GetSafeNormal();
    Scene->SetWorldLocationAndRotation(Head - Dir * CaptureDistanceCm, Dir.Rotation());
    Scene->RegisterComponent();
    Scene->CaptureScene();
    FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
    const bool bRead = Resource && Resource->ReadPixels(Colors);
    Scene->DestroyComponent();
    return bRead && Colors.Num() == CaptureSize * CaptureSize;
}

// Farthest hair pixel from her head (cm) seen from ViewDir, or -1 when nothing rendered. The hair is
// whatever differs from the same capture without it.
float Reach(AHomesteadCharacter* Avatar, UGroomComponent* Groom, const FVector& ViewDir, int32& Pixels)
{
    Pixels = 0;
    TArray<FColor> Colors, Background;
    if (!Capture(Avatar, Groom, ViewDir, Colors) || !Capture(Avatar, nullptr, ViewDir, Background)) return -1.0f;
    float Worst = 0.0f;
    const float CmPerPixel = CaptureWidthCm / CaptureSize;
    for (int32 Y = 0; Y < CaptureSize; ++Y)
        for (int32 X = 0; X < CaptureSize; ++X)
        {
            const FColor& C = Colors[Y * CaptureSize + X];
            const FColor& B = Background[Y * CaptureSize + X];
            if (FMath::Max3(FMath::Abs(C.R - B.R), FMath::Abs(C.G - B.G), FMath::Abs(C.B - B.B)) < 12) continue;
            ++Pixels;
            const float Dx = (X + 0.5f - CaptureSize * 0.5f) * CmPerPixel;
            const float Dy = (Y + 0.5f - CaptureSize * 0.5f) * CmPerPixel;
            Worst = FMath::Max(Worst, FMath::Sqrt(Dx * Dx + Dy * Dy));
        }
    return Pixels ? Worst : -1.0f;
}

struct FSample
{
    float Side = -1, Top = -1;
    int32 SidePixels = 0, TopPixels = 0;
};

bool Measure(UWorld* World, FSample& Out)
{
    AHomesteadCharacter* Avatar = Heroine(World);
    UGroomComponent* Groom = Hair(Avatar);
    if (!Avatar || !Groom) return false;
    Out.Side = Reach(Avatar, Groom, Avatar->GetActorRightVector(), Out.SidePixels);
    Out.Top = Reach(Avatar, Groom, FVector(0, 0, -1), Out.TopPixels);
    return true;
}

FString Describe(UWorld* World)
{
    AHomesteadCharacter* Avatar = Heroine(World);
    UGroomComponent* Groom = Hair(Avatar);
    if (!Groom) return TEXT("no heroine hair");
    return FString::Printf(TEXT("groom=%s override=%d lod=%d/%d"),
        Groom->GroomAsset ? *Groom->GroomAsset->GetName() : TEXT("none"),
        Groom->SimulationSettings.bOverrideSettings ? 1 : 0, Groom->GetDesiredSyncLOD(), Groom->GetNumLODs());
}
}

static FAutoConsoleCommandWithWorldAndArgs GHairStyleCommand(
    TEXT("homestead.HairStyle"), TEXT("Wear MetaHuman hairstyle N (0-8)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        AHomesteadCharacter* Avatar = HairLab::Heroine(World);
        if (!Avatar || Args.Num() < 1) return;
        FHomesteadAppearance Look;
        Look.MetaHair = FMath::Clamp(FCString::Atoi(*Args[0]), 0, HomesteadLook::MetaHairCount - 1);
        const bool bApplied = Avatar->ApplyAppearance(Look);
        UE_LOG(LogHomesteadHairLab, Display, TEXT("HairStyle %d applied=%d"), Look.MetaHair, bApplied ? 1 : 0);
    }));

static FAutoConsoleCommandWithWorldAndArgs GHairOverrideCommand(
    TEXT("homestead.HairOverride"), TEXT("Force the long groom's simulation override on (1) or off (0)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGroomComponent* Groom = HairLab::Hair(HairLab::Heroine(World));
        if (!Groom || Args.Num() < 1) return;
        Groom->SimulationSettings.bOverrideSettings = FCString::Atoi(*Args[0]) != 0;
        Groom->ResetSimulation();
        UE_LOG(LogHomesteadHairLab, Display, TEXT("HairOverride %s"), *HairLab::Describe(World));
    }));

static FAutoConsoleCommandWithWorldAndArgs GHairLODCommand(
    TEXT("homestead.HairLOD"), TEXT("Force the heroine's hair groom LOD (-1 to follow the body)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGroomComponent* Groom = HairLab::Hair(HairLab::Heroine(World));
        if (!Groom || Args.Num() < 1) return;
        Groom->SetForcedLOD(FCString::Atoi(*Args[0]));
        UE_LOG(LogHomesteadHairLab, Display, TEXT("HairLOD forced=%d %s"), Groom->GetForcedLOD(), *HairLab::Describe(World));
    }));

static FAutoConsoleCommandWithWorld GHairExtentCommand(
    TEXT("homestead.HairExtent"), TEXT("Log how far the heroine's hair reaches from her head (cm)."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        HairLab::FSample Sample;
        if (!HairLab::Measure(World, Sample)) return;
        UE_LOG(LogHomesteadHairLab, Display, TEXT("HAIR_EXTENT side=%.1f top=%.1f px=%d/%d %s"),
            Sample.Side, Sample.Top, Sample.SidePixels, Sample.TopPixels, *HairLab::Describe(World));
    }));

static FAutoConsoleCommandWithWorldAndArgs GHairWatchCommand(
    TEXT("homestead.HairWatch"), TEXT("Sample the hair extent every 0.2 s for N seconds and log the worst."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        const float Seconds = Args.Num() > 0 ? FMath::Clamp(FCString::Atof(*Args[0]), 0.2f, 120.0f) : 10.0f;
        TWeakObjectPtr<UWorld> WeakWorld(World);
        struct FWatch { float Left, Until, WorstSide = -1, WorstTop = -1; int32 Samples = 0; };
        const TSharedRef<FWatch> Watch = MakeShared<FWatch>(FWatch{Seconds, 0.0f});
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld, Watch](float Delta)
        {
            UWorld* Live = WeakWorld.Get();
            Watch->Left -= HairLab::SampleSeconds;
            HairLab::FSample Sample;
            if (Live && HairLab::Measure(Live, Sample))
            {
                ++Watch->Samples;
                Watch->WorstSide = FMath::Max(Watch->WorstSide, Sample.Side);
                Watch->WorstTop = FMath::Max(Watch->WorstTop, Sample.Top);
            }
            if (Live && Watch->Left > 0) return true;
            UE_LOG(LogHomesteadHairLab, Display, TEXT("HAIR_WATCH samples=%d worst_side=%.1f worst_top=%.1f %s"),
                Watch->Samples, Watch->WorstSide, Watch->WorstTop, Live ? *HairLab::Describe(Live) : TEXT(""));
            return false;
        }), HairLab::SampleSeconds);
    }));

#endif // !UE_BUILD_SHIPPING
