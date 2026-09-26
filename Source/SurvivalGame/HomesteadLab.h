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

    // Test props on the floor, built from the same meshes and layout as the woodland's resources.
    enum class EProp { None, Sticks, Stones, Berries };
    void PlaceProp(EProp Kind, FVector2D At);
    EProp PropKind() const { return Prop; }
    FVector2D PropLocation() const { return PropAt; }
    // Hide one produce component (a lifted stick), or all of it.
    void TakePropPart(int32 Index);
    void TakeAllProp();
    bool PropIntact() const;

private:
    void AddBlock(const FVector& Center, const FVector& SizeCm, const FRotator& Rotation);

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Floor;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY() TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Course;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UMaterialInterface> Grid;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PropBase;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PropProduce;
    EProp Prop = EProp::None;
    FVector2D PropAt = FVector2D::ZeroVector;
    float Hour = 10.0f;
};

UCLASS()
class SURVIVALGAME_API AHomesteadLabController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;

    // Console commands (also reachable through the editor MCP console helpers).
    // Play a work animation in place: Gather, Sticks, Water, Chop, Knife or Till. Sticks and
    // Gather use the placed prop like the game does (Sticks places a stick pile if there is none).
    UFUNCTION(Exec) void LabAction(const FString& Name);
    // Put a resource on the floor 45 cm in front of her (where she stops to gather in the woodland):
    // Sticks, Stones, Berries, or None to clear it.
    UFUNCTION(Exec) void LabProp(const FString& Name);
    // Repeat a LabAction (with a fresh prop and from the same spot) until LabLoop Off.
    UFUNCTION(Exec) void LabLoop(const FString& Name);
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
    bool bHoldingStickPile = false;
    FString LoopAction;
    FTransform LoopStart;
    double LoopNextStart = 0;
    double LoopPlayAt = 0;
    float LoopPeriod = 0;
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
