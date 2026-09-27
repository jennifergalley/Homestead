#include "HomesteadGeneralStore.h"

#include "HomesteadShopkeeper.h"
#include "Components/PointLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor Oak(0.30f, 0.17f, 0.08f);
const FLinearColor DarkOak(0.16f, 0.09f, 0.045f);
const FLinearColor Planks(0.36f, 0.23f, 0.12f);
const FLinearColor Slate(0.11f, 0.12f, 0.14f);
const FLinearColor Lime(0.80f, 0.77f, 0.68f);
const FLinearColor Glass(0.10f, 0.14f, 0.16f);
const FLinearColor SignGreen(0.035f, 0.10f, 0.07f);
const FLinearColor Cream(0.86f, 0.80f, 0.64f);
const FLinearColor Iron(0.08f, 0.08f, 0.08f);
const FLinearColor Hessian(0.55f, 0.43f, 0.27f);
const FLinearColor Tin(0.42f, 0.44f, 0.45f);
const FLinearColor Jar(0.52f, 0.38f, 0.22f);
const FLinearColor Cloth(0.46f, 0.12f, 0.10f);
const FLinearColor GraniteGrey(0.29f, 0.28f, 0.26f);
constexpr float WallHeight = 340.0f;
constexpr float WallThickness = 50.0f;
constexpr float DoorHalfWidth = 70.0f;
constexpr float DoorHeight = 240.0f;
constexpr float OpenDoorYaw = -100.0f;
}

AHomesteadGeneralStore::AHomesteadGeneralStore()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("StoreRoot"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Cube = CubeAsset.Object;
    Cylinder = CylinderAsset.Object;
}

UMaterialInterface* AHomesteadGeneralStore::Tint(const FLinearColor& Color, float Roughness)
{
    if (!FieldMaterial) return nullptr;
    const FString Key = FString::Printf(TEXT("%.3f_%.3f_%.3f_%.2f"), Color.R, Color.G, Color.B, Roughness);
    if (TObjectPtr<UMaterialInterface>* Existing = TintCache.Find(Key)) return Existing->Get();
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(FieldMaterial, this);
    Instance->SetVectorParameterValue(TEXT("Tint"), Color);
    Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Instance->SetScalarParameterValue(TEXT("Glow"), 0.0f);
    TintCache.Add(Key, Instance);
    return Instance;
}

UStaticMeshComponent* AHomesteadGeneralStore::Box(const FVector& Center, const FVector& Size, UMaterialInterface* Material,
    bool bCollision, float LocalYaw, USceneComponent* Parent)
{
    auto* Part = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("StorePart%d"), PartCount++));
    Part->SetStaticMesh(Cube);
    Part->SetupAttachment(Parent ? Parent : Root.Get());
    Part->SetRelativeLocationAndRotation(Center, FRotator(0, LocalYaw, 0));
    Part->SetRelativeScale3D(Size / 100.0f);
    if (Material) Part->SetMaterial(0, Material);
    Part->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Part->SetCollisionResponseToAllChannels(bCollision ? ECR_Block : ECR_Ignore);
    Part->RegisterComponent();
    return Part;
}

UStaticMeshComponent* AHomesteadGeneralStore::Round(const FVector& Center, float Radius, float Height,
    UMaterialInterface* Material, bool bCollision)
{
    UStaticMeshComponent* Part = Box(Center, FVector(Radius * 2, Radius * 2, Height), Material, bCollision);
    Part->SetStaticMesh(Cylinder);
    return Part;
}

UStaticMeshComponent* AHomesteadGeneralStore::Prop(const TCHAR* Name, const FVector& Location, float LocalYaw,
    bool bCollision, float Scale)
{
    // SM_Store_Counter lives in Props/StoreCounter, as import_props.py lays it out.
    const FString Folder = FString(Name).RightChop(3).Replace(TEXT("_"), TEXT(""));
    const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), *Folder, Name, Name);
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Mesh) return nullptr;
    auto* Part = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("StoreProp%d"), PartCount++));
    Part->SetStaticMesh(Mesh);
    Part->SetupAttachment(Root);
    // Blender recipes face -Y, which imports as +Y (yaw 90); turn that front to face LocalYaw.
    Part->SetRelativeLocationAndRotation(Location, FRotator(0, LocalYaw - 90.0f, 0));
    Part->SetRelativeScale3D(FVector(Scale));
    Part->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Part->RegisterComponent();
    return Part;
}

UTextRenderComponent* AHomesteadGeneralStore::Words(const FString& Text, const FVector& Location, float LocalYaw,
    float Size, const FColor& Color, USceneComponent* Parent)
{
    auto* Label = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("StoreText%d"), PartCount++));
    Label->SetupAttachment(Parent ? Parent : Root.Get());
    Label->SetRelativeLocationAndRotation(Location, FRotator(0, LocalYaw, 0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(Size);
    Label->SetTextRenderColor(Color);
    Label->SetText(FText::FromString(Text));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCastShadow(false);
    Label->RegisterComponent();
    return Label;
}

FVector AHomesteadGeneralStore::StoreToWorld(const FVector& Local) const
{
    return GetActorTransform().TransformPosition(Local);
}

FVector2D AHomesteadGeneralStore::DoorPoint() const
{
    const FVector World = StoreToWorld(FVector(-90, 0, 0));
    return FVector2D(World.X, World.Y);
}

FVector AHomesteadGeneralStore::ShopkeeperLocation() const
{
    return StoreToWorld(FVector(DoorToCounter, 0, 0));
}

bool AHomesteadGeneralStore::IsInside(const FVector& Location) const
{
    const FVector Local = GetActorTransform().InverseTransformPosition(Location);
    return Local.X > -10 && Local.X < RoomDepth && FMath::Abs(Local.Y) < RoomHalfWidth + 10;
}

void AHomesteadGeneralStore::Build(int32 InShopId, const FVector2D& Counter, float CounterYaw,
    TFunctionRef<float(float, float)> Ground, const FString& ClosedText)
{
    ShopId = InShopId;
    Counter2D = Counter;
    Yaw = CounterYaw;
    FieldMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    RockMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
    // The door lies DoorToCounter in front of the counter, and the shop runs back from it.
    const FVector2D Facing(FMath::Cos(FMath::DegreesToRadians(CounterYaw)), FMath::Sin(FMath::DegreesToRadians(CounterYaw)));
    const FVector2D Door = Counter + Facing * DoorToCounter;
    const float StoreYaw = CounterYaw + 180.0f;
    SetActorLocationAndRotation(FVector(Door.X, Door.Y, 0), FRotator(0, StoreYaw, 0));
    BuildShell(Ground);
    BuildInterior();
    ClosedBoard = Box(FVector(-10, DoorHalfWidth - 8, 150), FVector(3, 70, 36), Tint(Cream), false, 0, DoorHinge);
    ClosedSign = Words(ClosedText, FVector(-12, DoorHalfWidth - 8, 150), 180.0f, 7.0f, FColor(40, 24, 12), DoorHinge);
    if (UWorld* World = GetWorld())
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Shopkeeper = World->SpawnActor<AHomesteadShopkeeper>(Parameters);
        if (Shopkeeper) Shopkeeper->Place(ShopkeeperLocation(), CounterYaw);
    }
    bBuilt = true;
    bDoorOpen = false;
    SetOpen(true, FVector(1e9));
}

void AHomesteadGeneralStore::BuildShell(TFunctionRef<float(float, float)> Ground)
{
    // Sit the floor just above the highest ground under the footprint; a granite plinth runs down into
    // the slope so no gap shows.
    float Highest = -1e9f;
    for (float X : {-120.0f, 0.0f, RoomDepth * 0.5f, RoomDepth + WallThickness})
        for (float Y : {-RoomHalfWidth - WallThickness, 0.0f, RoomHalfWidth + WallThickness})
        {
            const FVector World = GetActorTransform().TransformPosition(FVector(X, Y, 0));
            Highest = FMath::Max(Highest, Ground(World.X, World.Y));
        }
    Floor = Highest + 12.0f;
    SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, Floor));

    // Flat dressed-granite tint until the store has its own tiling masonry material.
    UMaterialInterface* Granite = Tint(GraniteGrey, 0.92f);
    UMaterialInterface* Wash = Tint(Lime, 0.9f);
    const float OuterHalf = RoomHalfWidth + WallThickness;
    const float Back = RoomDepth + WallThickness * 0.5f;
    // Plinth, floorboards and a granite doorstep.
    Box(FVector(RoomDepth * 0.5f, 0, -160), FVector(RoomDepth + WallThickness * 2, OuterHalf * 2, 320), Granite);
    Box(FVector(RoomDepth * 0.5f, 0, 3), FVector(RoomDepth, RoomHalfWidth * 2, 6), Tint(Planks, 0.75f));
    for (int32 Board = 1; Board < 14; ++Board)
        Box(FVector(RoomDepth * 0.5f, -RoomHalfWidth + Board * 60.0f, 6.2f), FVector(RoomDepth, 1.5f, 0.5f), Tint(DarkOak), false);
    // A granite threshold and as many steps down to the street as the ground needs (risers under 20 cm).
    const FVector Street = GetActorTransform().TransformPosition(FVector(-WallThickness - 200, 0, 0));
    const float Drop = FMath::Max(0.0f, Floor - Ground(Street.X, Street.Y));
    Box(FVector(-WallThickness - 30, 0, -Drop * 0.5f - 25), FVector(60, 240, Drop + 50), Granite);
    const int32 Steps = FMath::CeilToInt(Drop / 20.0f);
    for (int32 Step = 1; Step < Steps; ++Step)
    {
        const float Top = -Drop * Step / Steps;
        const float Out = WallThickness + 60 + Step * 32.0f;
        Box(FVector(-Out + 16, 0, (Top - Drop - 50) * 0.5f), FVector(32, 240, Top + Drop + 50), Granite);
    }
    // Walls: back, both sides, and the street front either side of the door.
    Box(FVector(Back, 0, WallHeight * 0.5f), FVector(WallThickness, OuterHalf * 2, WallHeight), Granite);
    for (float Side : {-1.0f, 1.0f})
    {
        Box(FVector(RoomDepth * 0.5f, Side * (RoomHalfWidth + WallThickness * 0.5f), WallHeight * 0.5f),
            FVector(RoomDepth + WallThickness * 2, WallThickness, WallHeight), Granite);
        const float Inner = DoorHalfWidth, Outer = OuterHalf;
        Box(FVector(-WallThickness * 0.5f, Side * (Inner + Outer) * 0.5f, WallHeight * 0.5f),
            FVector(WallThickness, Outer - Inner, WallHeight), Granite);
        // A sash window each side of the door, with a lime-washed reveal inside.
        const float WindowY = Side * 250.0f;
        Box(FVector(-WallThickness - 1, WindowY, 170), FVector(4, 150, 150), Tint(Cream, 0.6f), false);
        Box(FVector(-WallThickness - 3, WindowY, 170), FVector(4, 136, 136), Tint(Glass, 0.15f), false);
        Box(FVector(-WallThickness - 4, WindowY, 170), FVector(3, 136, 5), Tint(Cream, 0.6f), false);
        Box(FVector(-WallThickness - 4, WindowY, 170), FVector(3, 5, 136), Tint(Cream, 0.6f), false);
        Box(FVector(-WallThickness - 4, WindowY, 98), FVector(16, 170, 10), Granite, false);
        Box(FVector(1, WindowY, 170), FVector(3, 150, 150), Tint(Glass, 0.2f), false);
    }
    // Lintel over the door, and the lime-washed inside faces.
    Box(FVector(-WallThickness * 0.5f, 0, (DoorHeight + WallHeight) * 0.5f),
        FVector(WallThickness, DoorHalfWidth * 2, WallHeight - DoorHeight), Granite);
    Box(FVector(-WallThickness - 2, 0, DoorHeight + 12), FVector(8, DoorHalfWidth * 2 + 40, 24), Granite, false);
    Box(FVector(RoomDepth - 1, 0, WallHeight * 0.5f), FVector(2, RoomHalfWidth * 2, WallHeight), Wash, false);
    for (float Side : {-1.0f, 1.0f})
    {
        Box(FVector(RoomDepth * 0.5f, Side * (RoomHalfWidth - 1), WallHeight * 0.5f), FVector(RoomDepth, 2, WallHeight), Wash, false);
        Box(FVector(1, Side * (DoorHalfWidth + RoomHalfWidth) * 0.5f, WallHeight * 0.5f),
            FVector(2, RoomHalfWidth - DoorHalfWidth, WallHeight), Wash, false);
    }
    // Ceiling beams and boards.
    Box(FVector(RoomDepth * 0.5f, 0, WallHeight - 2), FVector(RoomDepth, RoomHalfWidth * 2, 4), Tint(Planks, 0.8f), false);
    for (int32 Beam = 1; Beam < 5; ++Beam)
        Box(FVector(Beam * RoomDepth / 5.0f, 0, WallHeight - 16), FVector(22, RoomHalfWidth * 2, 26), Tint(DarkOak), false);
    // A 45-degree slate roof on a diamond gable at each end, and a chimney on the back gable.
    const float Rise = OuterHalf;
    const float Overhang = 45.0f;
    const float SlabWidth = (OuterHalf + Overhang) * UE_SQRT_2;
    for (float Side : {-1.0f, 1.0f})
    {
        UStaticMeshComponent* Slab = Box(FVector::ZeroVector, FVector(RoomDepth + WallThickness * 2 + 80, SlabWidth, 14), Tint(Slate, 0.55f));
        Slab->SetRelativeLocationAndRotation(
            FVector(RoomDepth * 0.5f, Side * (OuterHalf + Overhang) * 0.5f, WallHeight + Rise - (OuterHalf + Overhang) * 0.5f + 12),
            FRotator(0, 0, Side * 45.0f));
    }
    for (float X : {-WallThickness * 0.5f, Back})
        Gable(X, OuterHalf, Rise, Granite);
    Box(FVector(Back - 20, 180, WallHeight + Rise - 30), FVector(90, 90, 420), Granite);
    Box(FVector(Back - 20, 180, WallHeight + Rise + 185), FVector(104, 104, 14), Granite);
    Round(FVector(Back - 20, 180, WallHeight + Rise + 210), 16, 40, Tint(Jar));
    // The door, hinged on its left post, and the shop's sign board above it.
    DoorHinge = NewObject<USceneComponent>(this, TEXT("DoorHinge"));
    DoorHinge->SetupAttachment(Root);
    DoorHinge->SetRelativeLocation(FVector(-WallThickness * 0.5f, -DoorHalfWidth, 0));
    DoorHinge->RegisterComponent();
    Box(FVector(0, DoorHalfWidth, DoorHeight * 0.5f), FVector(7, DoorHalfWidth * 2 - 4, DoorHeight - 4), Tint(SignGreen, 0.6f), true, 0, DoorHinge);
    Box(FVector(-4, DoorHalfWidth, DoorHeight * 0.66f), FVector(2, DoorHalfWidth * 2 - 36, 70), Tint(Glass, 0.15f), false, 0, DoorHinge);
    Box(FVector(-5, DoorHalfWidth * 2 - 18, 110), FVector(6, 6, 6), Tint(Tin, 0.3f), false, 0, DoorHinge);
    Box(FVector(-WallThickness - 6, 0, DoorHeight + 62), FVector(8, 360, 58), Tint(SignGreen, 0.7f), false);
    Words(TEXT("GENERAL STORE"), FVector(-WallThickness - 11, 0, DoorHeight + 70), 180.0f, 30.0f, FColor(222, 178, 96));
    Words(TEXT("M. PASCOE  -  PROVISIONS & SUNDRIES"), FVector(-WallThickness - 11, 0, DoorHeight + 45), 180.0f, 11.0f,
        FColor(222, 206, 170));
    // Warm lamplight inside.
    for (float X : {260.0f, 640.0f})
    {
        auto* Light = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("StoreLamp%d"), PartCount++));
        Light->SetupAttachment(Root);
        Light->SetRelativeLocation(FVector(X, 0, WallHeight - 60));
        Light->SetIntensityUnits(ELightUnits::Candelas);
        Light->SetIntensity(220.0f);
        Light->SetAttenuationRadius(900.0f);
        Light->SetLightColor(FLinearColor(1.0f, 0.74f, 0.45f));
        Light->SetCastShadows(X > 500.0f);
        Light->RegisterComponent();
    }
}

void AHomesteadGeneralStore::Gable(float X, float HalfWidth, float Rise, UMaterialInterface* Material)
{
    // A triangular prism of wall above the eaves, WallThickness deep, apex at the ridge.
    auto* Mesh = NewObject<UProceduralMeshComponent>(this, *FString::Printf(TEXT("StoreGable%d"), PartCount++));
    Mesh->SetupAttachment(Root);
    const float H = WallThickness * 0.5f;
    const FVector A(0, -HalfWidth, WallHeight), B(0, HalfWidth, WallHeight), C(0, 0, WallHeight + Rise);
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    const auto Face = [&](const TArray<FVector>& Corners, const FVector& Normal)
    {
        const int32 Base = Vertices.Num();
        for (const FVector& Corner : Corners)
        {
            Vertices.Add(Corner);
            Normals.Add(Normal);
            UVs.Add(FVector2D(Corner.Y + Corner.X, Corner.Z) / 200.0f);
        }
        for (int32 I = 1; I + 1 < Corners.Num(); ++I)
        {
            // Both windings, so the face shows whichever way the engine culls.
            Triangles.Append({Base, Base + I, Base + I + 1});
            Triangles.Append({Base, Base + I + 1, Base + I});
        }
    };
    const FVector Front(-H, 0, 0), BackOffset(H, 0, 0);
    Face({A + Front, B + Front, C + Front}, FVector(-1, 0, 0));
    Face({A + BackOffset, B + BackOffset, C + BackOffset}, FVector(1, 0, 0));
    Face({A + Front, C + Front, C + BackOffset, A + BackOffset}, FVector(0, -1, 1).GetSafeNormal());
    Face({B + Front, C + Front, C + BackOffset, B + BackOffset}, FVector(0, 1, 1).GetSafeNormal());
    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, true);
    if (Material) Mesh->SetMaterial(0, Material);
    Mesh->SetRelativeLocation(FVector(X, 0, 0));
    Mesh->RegisterComponent();
}

void AHomesteadGeneralStore::BuildInterior()
{
    const float CounterX = DoorToCounter - 70.0f;
    // Counter, with a lifting flap gap at the left so the shopkeeper can come round.
    if (!Prop(TEXT("SM_Store_Counter"), FVector(CounterX, 0, 0), 180.0f))
    {
        Box(FVector(CounterX, 20, 48), FVector(66, 520, 96), Tint(Oak, 0.7f));
        Box(FVector(CounterX - 2, 20, 99), FVector(78, 536, 6), Tint(DarkOak, 0.45f));
        for (int32 Panel = 0; Panel < 5; ++Panel)
            Box(FVector(CounterX - 34, -200 + Panel * 110.0f, 48), FVector(2, 90, 70), Tint(DarkOak, 0.7f), false);
    }
    // Scales, a till and a jar of humbugs on the counter.
    Box(FVector(CounterX, -150, 110), FVector(40, 30, 16), Tint(Tin, 0.3f), false);
    Round(FVector(CounterX, -150, 124), 14, 3, Tint(Tin, 0.3f), false);
    Box(FVector(CounterX + 6, 150, 116), FVector(40, 44, 28), Tint(DarkOak, 0.5f), false);
    Round(FVector(CounterX - 6, 60, 118), 9, 32, Tint(Glass, 0.1f), false);
    // Wall shelving behind the counter and down the right side, stocked with tins, jars and bolts of cloth.
    const auto Shelving = [this](const FVector& Base, float LocalYaw, int32 Seed)
    {
        // LocalYaw turns the fallback unit; the prop's open front faces the room the same way.
        if (Prop(TEXT("SM_Store_Shelves"), Base + FRotator(0, LocalYaw, 0).RotateVector(FVector(-4, 0, 0)), LocalYaw)) return;
        const FRotator Turn(0, LocalYaw, 0);
        const auto At = [&](float Along, float Out, float Up) { return Base + Turn.RotateVector(FVector(Out, Along, Up)); };
        Box(At(0, 0, 110), FVector(36, 240, 220), Tint(DarkOak), true, LocalYaw);
        for (int32 Shelf = 0; Shelf < 4; ++Shelf)
        {
            const float Up = 30 + Shelf * 52.0f;
            Box(At(0, -4, Up), FVector(40, 240, 4), Tint(Oak, 0.6f), false, LocalYaw);
            for (int32 Good = 0; Good < 7; ++Good)
            {
                const int32 Kind = (Good * 3 + Shelf + Seed) % 4;
                const FLinearColor Color = Kind == 0 ? Tin : Kind == 1 ? Jar : Kind == 2 ? Cloth : Cream;
                const float Height = Kind == 2 ? 24.0f : 30.0f - ((Good + Shelf) % 3) * 5.0f;
                Box(At(-100 + Good * 33.0f, -8, Up + 2 + Height * 0.5f), FVector(18, 22, Height),
                    Tint(Color, Kind == 0 ? 0.35f : 0.7f), false, LocalYaw);
            }
        }
    };
    for (int32 Unit = 0; Unit < 3; ++Unit)
        Shelving(FVector(RoomDepth - 24, -270 + Unit * 270.0f, 0), 180.0f, Unit);
    Shelving(FVector(730, RoomHalfWidth - 24, 0), -90.0f, 5);
    // Barrels, sacks and crates by the door.
    const FVector Barrels[] = {{150, -345, 0}, {235, -360, 0}, {170, -262, 0}};
    for (const FVector& At : Barrels)
        if (!Prop(TEXT("SM_Store_Barrel"), At, At.X))
        {
            Round(At + FVector(0, 0, 48), 38, 96, Tint(Oak, 0.7f));
            Round(At + FVector(0, 0, 18), 39.5f, 5, Tint(Iron, 0.4f), false);
            Round(At + FVector(0, 0, 78), 39.5f, 5, Tint(Iron, 0.4f), false);
        }
    const FVector Sacks[] = {{150, 335, 0}, {225, 355, 0}, {205, 275, 0}, {160, 330, 55}};
    for (const FVector& At : Sacks)
        if (!Prop(TEXT("SM_Store_Sack"), At, At.Y))
        {
            Round(At + FVector(0, 0, 32), 30, 64, Tint(Hessian, 0.95f));
            Round(At + FVector(0, 0, 66), 12, 10, Tint(Hessian, 0.95f), false);
        }
    const FVector Crates[] = {{385, -360, 0}, {385, -360, 62}, {330, -300, 0}};
    for (const FVector& At : Crates)
        if (!Prop(TEXT("SM_Store_Crate"), At, 0.0f))
        {
            Box(At + FVector(0, 0, 30), FVector(60, 60, 60), Tint(Planks, 0.8f));
            Box(At + FVector(0, 0, 30), FVector(62, 8, 62), Tint(DarkOak), false);
        }
    // A hanging lantern over the counter and the shop bell over the door.
    Round(FVector(CounterX + 10, 0, WallHeight - 50), 1.5f, 60, Tint(Iron), false);
    Box(FVector(CounterX + 10, 0, WallHeight - 92), FVector(22, 22, 30), Tint(Glass, 0.1f), false);
    Round(FVector(20, 40, DoorHeight + 10), 7, 10, Tint(Jar, 0.3f), false);
}

void AHomesteadGeneralStore::SetOpen(bool bOpen, const FVector& HeroineLocation)
{
    if (!bBuilt) return;
    if (Shopkeeper) Shopkeeper->SetOnDuty(bOpen);
    // She's never shut in: the door waits for her to step out.
    const bool bWantOpen = bOpen || IsInside(HeroineLocation);
    if (bWantOpen != bDoorOpen)
    {
        bDoorOpen = bWantOpen;
        DoorHinge->SetRelativeRotation(FRotator(0, bDoorOpen ? OpenDoorYaw : 0.0f, 0));
    }
    if (ClosedBoard) ClosedBoard->SetVisibility(!bOpen);
    if (ClosedSign) ClosedSign->SetVisibility(!bOpen);
}

void AHomesteadGeneralStore::Destroyed()
{
    if (Shopkeeper) Shopkeeper->Destroy();
    Super::Destroyed();
}
