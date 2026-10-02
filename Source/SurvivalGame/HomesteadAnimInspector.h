#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadAnimInspector.generated.h"

class AHomesteadCharacter;
class AHomesteadLabController;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

// Animation Inspector (realistic-animation skill, "Seeing every frame"): plays one Character Lab action
// at a fixed 1/30 s step and records every frame of the heroine for review. Started from the command
// line with the lab (Scripts\Inspect-Animation.ps1):
//   -HomesteadCharacterLab -HomesteadMetaHuman -HomesteadAnimInspector=<LabAction name>
//   -HomesteadAnimInspectorOut=<dir> [-HomesteadAnimInspectorEvery=2] [-HomesteadAnimInspectorViews=front,left,right,top,threequarter]
//   [-HomesteadAnimInspectorHold=<LabHold tool>] [-HomesteadAnimInspectorLit] [-HomesteadAnimInspectorExit]
// It writes, under the output directory:
//   frames.json   - per frame: time, the body's component-space bone transforms (the final pose, after
//                   the MetaHuman post-process), the mesh's world transform, held props and their bounds,
//                   and the ground under each foot;
//   neutral.json  - the skeleton's reference pose in component space (joint_limits' zero);
//   cameras.json  - each view's camera, for projecting points onto its images;
//   <view>/f0000.png - the captured views every Nth frame.
// Scripts/anim_inspector_sheet.py turns them into overlays, contact sheets, a GIF and index.md.
namespace HomesteadAnimInspector
{
bool Requested();
}

UCLASS()
class SURVIVALGAME_API AHomesteadAnimInspector : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadAnimInspector();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    enum class EPhase : uint8 { Settle, Record, Done };

    struct FView
    {
        FString Name;
        bool bOrtho = true;
        float Width = 240.0f;
        float Fov = 35.0f;
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
    };

    void Begin(AHomesteadCharacter& Avatar, AHomesteadLabController& Lab);
    void Record(AHomesteadCharacter& Avatar);
    void Capture(int32 Frame);
    void Finish(const FString& Reason);
    bool IsActionBusy(const AHomesteadCharacter& Avatar) const;
    void WriteNeutral(const AHomesteadCharacter& Avatar);
    void WriteCameras() const;

    UPROPERTY() TArray<TObjectPtr<USceneCaptureComponent2D>> Captures;
    UPROPERTY() TArray<TObjectPtr<UTextureRenderTarget2D>> Targets;
    TArray<FView> Views;
    FString Action, Hold, OutDir;
    int32 Every = 2;
    int32 Resolution = 768;
    bool bLit = false;
    bool bExit = false;
    EPhase Phase = EPhase::Settle;
    float Elapsed = 0;
    int32 Frame = 0;
    int32 IdleFrames = 0;
    bool bSeenBusy = false;
    TArray<FName> Bones;
    // frames.json is streamed: one line per frame.
    FString FramesJson;
};
