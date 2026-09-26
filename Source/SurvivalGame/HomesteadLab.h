#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HomesteadLab.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UStaticMeshComponent;
class USoundBase;

// Character lab: an endless flat test ground for iterating on the heroine's model, animation,
// foot placement and shading, with the real character, camera, input and animation graph but none
// of the simulation, generated woodland, menus or saves. Start it with -HomesteadCharacterLab
// (packaged or -game) or set homestead.CharacterLab 1 before Play-In-Editor.
namespace HomesteadLab
{
bool Requested();
}

// Flat, world-aligned grid floor that follows the player so it never ends, plus a small test
// course (ramps and steps) away from the spawn for foot placement.
UCLASS()
class SURVIVALGAME_API AHomesteadLabWorld : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadLabWorld();
    virtual void Tick(float DeltaSeconds) override;
    void SetSunHour(float Hour);
    float SunHour() const { return Hour; }
    // Ground height under X/Y on the floor or course (for teleports).
    float SurfaceHeight(float X, float Y) const;

    // The course sits along +X from this point: ramps of 10, 20 and 30 degrees, then steps.
    static constexpr float CourseX = 2500.0f;

private:
    void AddBlock(const FVector& Center, const FVector& SizeCm, const FRotator& Rotation);

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Floor;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY() TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Course;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UMaterialInterface> Grid;
    float Hour = 10.0f;
};

UCLASS()
class SURVIVALGAME_API AHomesteadLabController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;

    // Console commands (also reachable through the editor MCP console helpers).
    // Play a work animation in place: Gather, Sticks, Water, Chop, Knife or Till.
    UFUNCTION(Exec) void LabAction(const FString& Name);
    // Move the sun to a time of day (0-24); shadows and sky follow.
    UFUNCTION(Exec) void LabSun(float Hour);
    // Put the heroine at X/Y (cm) on the floor or course, facing +X.
    UFUNCTION(Exec) void LabTeleport(float X, float Y);
    // Jump to the start of the ramp/step course.
    UFUNCTION(Exec) void LabCourse();

    void PlayFootstep(bool bLeftFoot, bool bRun);
    const AHomesteadLabWorld* LabWorld() const { return World; }

private:
    UPROPERTY() TObjectPtr<AHomesteadLabWorld> World;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> WalkSteps;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> RunSteps;
    double LastFootstepTime = -1;
    int32 LastStep = INDEX_NONE;
};

UCLASS()
class SURVIVALGAME_API AHomesteadLabHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    float SmoothedFrameMs = 16.7f;
};
