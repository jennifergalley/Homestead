#include "HomesteadController.h"
#include "HomesteadControllerPreferences.h"
#include "HomesteadCharacter.h"
#include "HomesteadEstateGround.h"
#include "HomesteadEstateTerrain.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadFootsteps, Log, All);

using HomesteadControllerPreferences::AudioSettingsSection;
using HomesteadControllerPreferences::LastMusicTrackKey;

void AHomesteadController::UpdateCreekAudio()
{
    if (!Creek->Sound || !bAudioEnabled) return;
    FVector Listener;
    FRotator View;
    GetPlayerViewPoint(Listener, View);
    if (bEstateMap)
    {
        // The burble follows the nearest point of the estate's river splines.
        double Best = TNumericLimits<double>::Max();
        FVector Where = FVector::ZeroVector;
        WaterEdgeDistance({Listener.X, Listener.Y});
        for (const auto& Weak : EstateWaterSplines)
            if (const USplineComponent* Spline = Weak.Get(); Spline && !Spline->IsClosedLoop())  // still lakes don't burble
            {
                const FVector Point = Spline->FindLocationClosestToWorldLocation(Listener, ESplineCoordinateSpace::World);
                const double Distance = FVector::Dist2D(Point, Listener);
                if (Distance < Best) { Best = Distance; Where = Point; }
            }
        if (Best > 6000.0) { if (Creek->IsPlaying()) Creek->FadeOut(2.0f, 0.0f); return; }
        Creek->SetWorldLocation(Where + FVector(0, 0, 20));
        if (!Creek->IsPlaying()) Creek->FadeIn(2.0f, 1.0f);
        return;
    }
    // Nearest point of the meandering centreline: a coarse sweep, then a fine one.
    auto Distance = [&](double Y) { return FMath::Square(Homestead::StreamX(Y) - Listener.X) + FMath::Square(Y - Listener.Y); };
    double BestY = Listener.Y;
    for (double Step : {50.0, 5.0})
    {
        const double From = BestY - (Step > 10 ? 1000.0 : 50.0);
        const double To = BestY + (Step > 10 ? 1000.0 : 50.0);
        for (double Y = From; Y <= To; Y += Step)
            if (Distance(Y) < Distance(BestY)) BestY = Y;
    }
    const APawn* Avatar = GetPawn();
    const double Z = Avatar ? Avatar->GetActorLocation().Z - 60.0 : Listener.Z - 200.0;
    Creek->SetWorldLocation(FVector(Homestead::StreamX(BestY), BestY, Z));
    if (!Creek->IsPlaying()) Creek->FadeIn(2.0f, 1.0f);
}

void AHomesteadController::InitializeAudio()
{
    bAudioEnabled = !FParse::Param(FCommandLine::Get(), TEXT("nosound"));
    GrassStepA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/GrassStepA.GrassStepA"));
    GrassStepB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/GrassStepB.GrassStepB"));
    WoodTapA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/WoodTapA.WoodTapA"));
    WoodTapB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/WoodTapB.WoodTapB"));
    CraftStrikeA = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeA.CraftStrikeA"));
    CraftStrikeB = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeB.CraftStrikeB"));
    CraftStrikeC = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/CraftStrikeC.CraftStrikeC"));
    UIClick = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/UIClick.UIClick"));
    for (const TCHAR* Chop : {TEXT("ChopA"), TEXT("ChopB"), TEXT("ChopC")})
        if (USoundBase* Cue = LoadObject<USoundBase>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Effects/%s.%s"), Chop, Chop), nullptr, LOAD_NoWarn | LOAD_Quiet))
            ChopStrokes.Add(Cue);
    TreeFallThud = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/TreeFall.TreeFall"),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    ScytheSwish = LoadObject<USoundBase>(nullptr, TEXT("/Game/SurvivalGame/Audio/Effects/ScytheSwish.ScytheSwish"),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!ScytheSwish)
        UE_LOG(LogTemp, Error, TEXT("The scythe's mowing cue (ScytheSwish) isn't imported, so mowing is silent. Run Scripts/bootstrap_unreal.py."));
    auto LoadPool = [](TArray<TObjectPtr<USoundBase>>& Pool, const TCHAR* Prefix, int32 Count)
    {
        Pool.Reset();
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FString Name = FString::Printf(TEXT("%s_%02d"), Prefix, Index);
            if (USoundBase* Step = LoadObject<USoundBase>(nullptr,
                    *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Effects/%s.%s"), *Name, *Name)))
                Pool.Add(Step);
        }
    };
    LoadPool(BareWalkSteps, TEXT("BareStepWalk"), 6);
    LoadPool(BareRunSteps, TEXT("BareStepRun"), 4);
    if (!GrassStepA || !GrassStepB || !WoodTapA || !WoodTapB
        || !CraftStrikeA || !CraftStrikeB || !CraftStrikeC || !UIClick || BareWalkSteps.Num() != 6 || BareRunSteps.Num() != 4)
        UE_LOG(LogTemp, Warning, TEXT("Some feedback sounds are missing; rerun the asset/bootstrap pipeline."));
    if (USoundWave* Forest = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/ForestAmbience.ForestAmbience")))
    {
        Forest->bLooping = true;
        Ambience->SetSound(Forest);
        Ambience->SetVolumeMultiplier(AmbienceVolume);
        if (bAudioEnabled) Ambience->FadeIn(3, 1);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Forest ambience is not imported. Run Scripts/bootstrap_unreal.py."));
    if (USoundWave* Brook = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/CreekLoop.CreekLoop")))
    {
        Brook->bLooping = true;
        Creek->SetSound(Brook);
        Creek->SetVolumeMultiplier(AmbienceVolume * CreekGain);
    }
    else UE_LOG(LogTemp, Warning, TEXT("Creek loop is not imported. Run Scripts/bootstrap_unreal.py."));
    // Kevin MacLeod tracks (CC BY 4.0), shuffled with no immediate repeat. Loudness is the gated,
    // K-weighted level measured from each source file (dBFS); every track is matched to the same
    // level, which sits about 14 dB under the old harp-only mix at the default 65% setting, so music
    // stays under the woodland ambience and footsteps instead of dominating them.
    struct FTrack { const TCHAR* Name; float Loudness; };
    static constexpr FTrack Tracks[] = {
        {TEXT("EveningHarp"), -21.0f}, {TEXT("AscendingTheVale"), -21.3f}, {TEXT("TellerOfTheTales"), -23.4f},
        {TEXT("MeditationImpromptu02"), -23.6f}, {TEXT("AtRest"), -28.6f}};
    constexpr float TargetLoudnessAtFullVolume = -35.0f;
    MusicTracks.Reset();
    MusicTrackGains.Reset();
    MusicTrackNames.Reset();
    for (const FTrack& Track : Tracks)
    {
        if (USoundBase* Score = LoadObject<USoundBase>(nullptr,
                *FString::Printf(TEXT("/Game/SurvivalGame/Audio/Music/%s.%s"), Track.Name, Track.Name)))
        {
            MusicTracks.Add(Score);
            MusicTrackNames.Add(Track.Name);
            MusicTrackGains.Add(FMath::Pow(10.0f, (TargetLoudnessAtFullVolume - Track.Loudness) / 20.0f));
        }
        else UE_LOG(LogTemp, Warning, TEXT("Music track %s is not imported and is skipped. Run Scripts/bootstrap_unreal.py."), Track.Name);
    }
    // A fresh launch avoids opening with the track the previous launch last started (a user
    // preference, independent of homestead saves). Order comes from process entropy.
    FString LastTrack;
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (Branch && Disk.Combine(Branch->IniPath)) Disk.GetString(AudioSettingsSection, LastMusicTrackKey, LastTrack);
    MusicBag.Reset(MusicTracks.Num(), MusicTrackNames.IndexOfByKey(LastTrack),
        FPlatformTime::Cycles64() ^ static_cast<uint64>(FDateTime::UtcNow().GetTicks()) ^ FPlatformProcess::GetCurrentProcessId());
    if (MusicTracks.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("No music tracks are available; ambience and effects continue."));
    }
    else
    {
        Music->OnAudioFinished.AddDynamic(this, &AHomesteadController::MusicFinished);
    }
}

float AHomesteadController::MusicLevel() const
{
    return MusicVolume * (MusicTrackGains.IsValidIndex(MusicTrack) ? MusicTrackGains[MusicTrack] : 1.0f);
}

void AHomesteadController::StartNextMusicTrack()
{
    MusicTrack = MusicBag.Next();
    if (!MusicTracks.IsValidIndex(MusicTrack)) return;
    Music->SetSound(MusicTracks[MusicTrack].Get());
    Music->SetVolumeMultiplier(MusicLevel());
    UE_LOG(LogTemp, Display, TEXT("MUSIC_TRACK started=%s catalog=%d"), *MusicTrackNames[MusicTrack], MusicTracks.Num());
    if (const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr)
    {
        FConfigFile Property;
        Property.SetString(AudioSettingsSection, LastMusicTrackKey, *MusicTrackNames[MusicTrack]);
        if (!Property.UpdateSinglePropertyInSection(*Branch->IniPath, LastMusicTrackKey, AudioSettingsSection))
            UE_LOG(LogTemp, Warning, TEXT("Could not record the last music track; the next launch may repeat it."));
    }
}

void AHomesteadController::PlayEffect(USoundBase* Cue, float Gain)
{
    if (Cue && bAudioEnabled && EffectsVolume > 0)
        UGameplayStatics::PlaySound2D(this, Cue, EffectsVolume * Gain, FMath::FRandRange(0.96f, 1.04f));
}

void AHomesteadController::PlayFootstep(bool bLeftFoot, bool bRun)
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Avatar || bBookOpen || bPlanning || IsFailed() || !Avatar->GetCharacterMovement()->IsMovingOnGround()
        || Avatar->GetVelocity().Size2D() < 12 || Now - LastFootstepTime < 0.18)
        return;
    const auto& Pool = bRun ? BareRunSteps : BareWalkSteps;
    if (Pool.IsEmpty()) return;
    LastFootstepTime = Now;
    ++Footsteps;
    int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
    if (Pool.Num() > 1 && Pick == LastBareStep) Pick = (Pick + 1) % Pool.Num();
    LastBareStep = Pick;
    UE_LOG(LogHomesteadFootsteps, Verbose, TEXT("Footstep %s %s t=%.3f"), bLeftFoot ? TEXT("L") : TEXT("R"),
        bRun ? TEXT("run") : TEXT("walk"), Now);
    // Bare feet on soft soil are quiet: about 10 dB under the old shod grass step while walking,
    // a little firmer when running, with a small level variation so repeats don't stand out.
    const float Gain = (bRun ? 0.07f : 0.04f) * FMath::FRandRange(0.85f, 1.15f);
    // On the estate's turf, moor and leaf litter the step is softer still: a few dB down, with a
    // low-pass taking the grit off the top, as bare feet on grass sound. Other ground is unchanged.
    const FVector Feet = Avatar->GetActorLocation();
    if (HomesteadEstateTerrain::IsActive() && HomesteadEstateGround::Activate())
    {
        using ESurface = HomesteadEstateGround::ESurface;
        const ESurface Surface = HomesteadEstateGround::SurfaceAt(Feet.X, Feet.Y);
        if (HomesteadEstateGround::IsSoft(Surface))
        {
            const float Softer = Surface == ESurface::Grass ? 0.55f : Surface == ESurface::Moor ? 0.6f : 0.7f;
            const float Cutoff = Surface == ESurface::Grass ? 2400.0f : Surface == ESurface::Moor ? 3000.0f : 3600.0f;
            if (Pool[Pick] && bAudioEnabled && EffectsVolume > 0)
                if (UAudioComponent* Step = UGameplayStatics::CreateSound2D(this, Pool[Pick].Get(),
                        EffectsVolume * Gain * Softer, FMath::FRandRange(0.94f, 1.02f)))
                {
                    Step->SetLowPassFilterEnabled(true);
                    Step->SetLowPassFilterFrequency(Cutoff);
                    Step->Play();
                }
            return;
        }
    }
    PlayEffect(Pool[Pick].Get(), Gain);
}

void AHomesteadController::MusicFinished()
{
    bMusicFading = false;
    MusicElapsed = 0;
    MusicGapRemaining = FMath::FRandRange(55.0f, 110.0f);
}
