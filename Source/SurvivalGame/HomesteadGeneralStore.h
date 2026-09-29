#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadItems.h"
#include "HomesteadGeneralStore.generated.h"

class AHomesteadShopkeeper;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

// A town shop: a single-room granite-and-slate building whose door opens onto the street, with a sign
// board and a closed sign on the door outside opening hours. Pascoe's general store has shelves,
// barrels, sacks and crates; Tregear's, the seedsman, has seed drawers, open grain sacks and a scale.
// Built around the shop's counter point: the shopkeeper stands there facing `CounterYaw`, toward the
// door 600 cm away.
UCLASS()
class SURVIVALGAME_API AHomesteadGeneralStore : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadGeneralStore();
    // `Ground` samples terrain height at a world XY point.
    void Build(int32 InShopId, Homestead::ShopKind InKind, const FVector2D& Counter, float CounterYaw,
        TFunctionRef<float(float, float)> Ground, const FString& ClosedText);
    // The notice on the shut door ("on Sundays" or the opening hour); cheap when unchanged.
    void SetClosedText(const FString& Text);
    Homestead::ShopKind GetKind() const { return Kind; }
    // Opens or shuts the door and puts the shopkeeper on or off duty. The door stays open while the
    // heroine is still inside at closing.
    void SetOpen(bool bOpen, const FVector& HeroineLocation);
    int32 GetShopId() const { return ShopId; }
    bool IsBuilt() const { return bBuilt; }
    FVector2D CounterPoint() const { return Counter2D; }
    float GetCounterYaw() const { return Yaw; }
    // Just outside the door, where the "Closed" prompt is offered.
    FVector2D DoorPoint() const;
    FVector ShopkeeperLocation() const;
    bool IsInside(const FVector& Location) const;
    bool IsDoorOpen() const { return bDoorOpen; }
    AHomesteadShopkeeper* GetShopkeeper() const { return Shopkeeper; }
    virtual void Destroyed() override;
    // Layout in the store's own frame (cm): +X from the door into the shop, +Y to the right.
    static constexpr float DoorToCounter = 600.0f;
    static constexpr float RoomDepth = 900.0f;
    static constexpr float RoomHalfWidth = 420.0f;

private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<USceneComponent> DoorHinge;
    UPROPERTY() TObjectPtr<UTextRenderComponent> ClosedSign;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ClosedBoard;
    UPROPERTY() TObjectPtr<AHomesteadShopkeeper> Shopkeeper;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UMaterialInterface> FieldMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> RockMaterial;
    UPROPERTY() TMap<FString, TObjectPtr<UMaterialInterface>> TintCache;
    int32 ShopId = 0;
    Homestead::ShopKind Kind = Homestead::ShopKind::GeneralStore;
    FString ClosedTextShown;
    FVector2D Counter2D = FVector2D::ZeroVector;
    float Yaw = 0.0f;
    float Floor = 0.0f;
    bool bBuilt = false;
    bool bDoorOpen = true;
    int32 PartCount = 0;
    UMaterialInterface* Tint(const FLinearColor& Color, float Roughness = 0.8f);
    UMaterialInterface* Surface(const TCHAR* Name, const FLinearColor& Fallback, float Roughness);
    // A box in store space (centre, full size in cm), yaw about Z in store space.
    UStaticMeshComponent* Box(const FVector& Center, const FVector& Size, UMaterialInterface* Material,
        bool bCollision = true, float LocalYaw = 0.0f, USceneComponent* Parent = nullptr);
    UStaticMeshComponent* Round(const FVector& Center, float Radius, float Height, UMaterialInterface* Material,
        bool bCollision = true);
    // An imported Blender prop if present (under /Game/SurvivalGame/Environment/Store), else null.
    UStaticMeshComponent* Prop(const TCHAR* Name, const FVector& Location, float LocalYaw, bool bCollision = true,
        float Scale = 1.0f);
    UTextRenderComponent* Words(const FString& Text, const FVector& Location, float LocalYaw, float Size,
        const FColor& Color, USceneComponent* Parent = nullptr);
    void BuildShell(TFunctionRef<float(float, float)> Ground);
    void Gable(float X, float HalfWidth, float Rise, UMaterialInterface* Material);
    void BuildInterior();
    void BuildSeedsmanInterior();
    FVector StoreToWorld(const FVector& Local) const;
};
