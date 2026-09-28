#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadTownBuilding.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UENUM()
enum class ETownWall : uint8
{
    Rubble,  // coursed granite rubble
    Ashlar,  // dressed granite ashlar
    Render,  // lime-washed render with granite quoins
};

UENUM()
enum class ETownRoof : uint8
{
    RidgeAlongStreet, // eaves to the street, gable ends to the sides
    GableToStreet,    // a gable end faces the street
};

// Blockout massing for the town square: a solid granite-and-slate building placed in the Estate
// level, with sash windows, an inert door, chimneys and an empty sign board. The parts are
// transient and rebuilt from the properties, in the editor on construction and in the game on
// BeginPlay, so the level only saves the parameters. Local frame: the front wall is at X = 0
// facing -X (the street), the building runs back along +X, and +Y is to the right when facing it.
UCLASS()
class SURVIVALGAME_API AHomesteadTownBuilding : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadTownBuilding();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Town") float Width = 800.0f;          // along the street
    UPROPERTY(EditAnywhere, Category = "Town") float Depth = 700.0f;          // back from the street
    UPROPERTY(EditAnywhere, Category = "Town", meta = (ClampMin = 1, ClampMax = 3)) int32 Storeys = 2;
    UPROPERTY(EditAnywhere, Category = "Town") float StoreyHeight = 290.0f;
    UPROPERTY(EditAnywhere, Category = "Town", meta = (ClampMin = 20, ClampMax = 60)) float RoofPitch = 40.0f;
    UPROPERTY(EditAnywhere, Category = "Town") ETownRoof Roof = ETownRoof::RidgeAlongStreet;
    UPROPERTY(EditAnywhere, Category = "Town") ETownWall Wall = ETownWall::Rubble;
    UPROPERTY(EditAnywhere, Category = "Town") FLinearColor WallTint = FLinearColor::White;
    // Chimney stacks at the left end, the right end, or both (-1, 1, 2); 0 for none.
    UPROPERTY(EditAnywhere, Category = "Town") int32 Chimneys = 1;
    // Door position across the front, -1 (left) to 1 (right).
    UPROPERTY(EditAnywhere, Category = "Town") float DoorAt = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Town") FLinearColor DoorColor = FLinearColor(0.05f, 0.09f, 0.14f);
    // A shopfront: a wide display window by the door with a fascia and an empty sign board.
    UPROPERTY(EditAnywhere, Category = "Town") bool bShopfront = false;
    UPROPERTY(EditAnywhere, Category = "Town") FLinearColor SignColor = FLinearColor(0.04f, 0.10f, 0.07f);
    UPROPERTY(EditAnywhere, Category = "Town") bool bSideWindows = true;
    // How far the plinth runs below the floor to meet falling ground (added to the slope when snapping).
    UPROPERTY(EditAnywhere, Category = "Town") float PlinthDepth = 60.0f;
    // Sit the floor just above the highest estate ground under the footprint, whatever the actor's Z.
    UPROPERTY(EditAnywhere, Category = "Town") bool bSnapToGround = true;

private:
    UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> Parts;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UMaterialInterface>> Materials;
    // Height of the floor above the actor origin, from snapping to the ground.
    float Lift = 0.0f;
    void Rebuild();
    UMaterialInterface* Tint(const FLinearColor& Color, float Roughness = 0.8f);
    // A store tiling surface (MI_Store_<Name>) multiplied by Multiply, or null if it isn't imported.
    UMaterialInterface* Surface(const TCHAR* Name, const FLinearColor& Multiply);
    UMaterialInterface* Slate(float AlongRidge, float UpSlope);
    UStaticMeshComponent* Box(const FVector& Center, const FVector& Size, UMaterialInterface* Material,
        bool bCollision = false, const FRotator& Rotation = FRotator::ZeroRotator);
    // A triangular prism: triangle A-B-C swept by Extrude.
    void Prism(const FVector& A, const FVector& B, const FVector& C, const FVector& Extrude, UMaterialInterface* Material);
    // A sash window on a wall face. Origin is the wall surface point at the window's centre, Out
    // the wall's outward normal, Across the wall's horizontal direction.
    void Window(const FVector& Origin, const FVector& Out, const FVector& Across, float W, float H,
        UMaterialInterface* Stone);
    void Chimney(const FVector& Base, const FVector2D& Size, float Top, UMaterialInterface* Stone);
};
