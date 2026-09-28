#include "HomesteadTownBuilding.h"

#include "HomesteadEstateTerrain.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor GraniteGrey(0.29f, 0.28f, 0.26f);
const FLinearColor Lime(0.80f, 0.77f, 0.68f);
const FLinearColor SlateGrey(0.11f, 0.12f, 0.14f);
const FLinearColor RidgeTile(0.20f, 0.19f, 0.18f);
const FLinearColor Cream(0.86f, 0.80f, 0.64f);
const FLinearColor Glass(0.10f, 0.14f, 0.16f);
const FLinearColor Terracotta(0.42f, 0.20f, 0.11f);
const FString Surfaces = TEXT("/Game/SurvivalGame/Environment/Props/StoreSurfaces/");
constexpr float Overhang = 35.0f;
constexpr float SlabThickness = 12.0f;
constexpr float DoorWidth = 110.0f;
constexpr float DoorHeight = 220.0f;
}

AHomesteadTownBuilding::AHomesteadTownBuilding()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("BuildingRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Cube = CubeAsset.Object;
    Cylinder = CylinderAsset.Object;
}

void AHomesteadTownBuilding::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Rebuild();
}

void AHomesteadTownBuilding::BeginPlay()
{
    Super::BeginPlay();
    // Transient parts aren't saved or duplicated into PIE, so the game builds its own.
    if (Parts.Num() == 0) Rebuild();
}

UMaterialInterface* AHomesteadTownBuilding::Tint(const FLinearColor& Color, float Roughness)
{
    const FString Key = FString::Printf(TEXT("Tint_%.3f_%.3f_%.3f_%.2f"), Color.R, Color.G, Color.B, Roughness);
    if (TObjectPtr<UMaterialInterface>* Existing = Materials.Find(Key)) return Existing->Get();
    UMaterialInterface* Field = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    if (!Field) return nullptr;
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Field, this);
    Instance->SetVectorParameterValue(TEXT("Tint"), Color);
    Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Instance->SetScalarParameterValue(TEXT("Glow"), 0.0f);
    Materials.Add(Key, Instance);
    return Instance;
}

UMaterialInterface* AHomesteadTownBuilding::Surface(const TCHAR* Name, const FLinearColor& Multiply)
{
    const FString Key = FString::Printf(TEXT("%s_%.3f_%.3f_%.3f"), Name, Multiply.R, Multiply.G, Multiply.B);
    if (TObjectPtr<UMaterialInterface>* Existing = Materials.Find(Key)) return Existing->Get();
    const FString Path = FString::Printf(TEXT("%sMI_Store_%s.MI_Store_%s"), *Surfaces, Name, Name);
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Base) return nullptr;
    UMaterialInterface* Result = Base;
    if (!Multiply.Equals(FLinearColor::White))
    {
        UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Base, this);
        Instance->SetVectorParameterValue(TEXT("Tint"), Multiply);
        Result = Instance;
    }
    Materials.Add(Key, Result);
    return Result;
}

UMaterialInterface* AHomesteadTownBuilding::Slate(float AlongRidge, float UpSlope)
{
    const FString Key = FString::Printf(TEXT("Slate_%.0f_%.0f"), AlongRidge, UpSlope);
    if (TObjectPtr<UMaterialInterface>* Existing = Materials.Find(Key)) return Existing->Get();
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        *(Surfaces + TEXT("MI_Store_SlateRoof.MI_Store_SlateRoof")), nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Base) return Tint(SlateGrey, 0.55f);
    // The slate texture covers 2 m; the roof UV material scales the cube's 0-1 face UVs.
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Base, this);
    Instance->SetVectorParameterValue(TEXT("UVScale"), FLinearColor(AlongRidge / 200.0f, UpSlope / 200.0f, 0, 0));
    Materials.Add(Key, Instance);
    return Instance;
}

UStaticMeshComponent* AHomesteadTownBuilding::Box(const FVector& Center, const FVector& Size, UMaterialInterface* Material,
    bool bCollision, const FRotator& Rotation)
{
    auto* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
    Part->SetStaticMesh(Cube);
    Part->SetupAttachment(GetRootComponent());
    Part->SetRelativeLocationAndRotation(Center + FVector(0, 0, Lift), Rotation);
    Part->SetRelativeScale3D(Size / 100.0f);
    if (Material) Part->SetMaterial(0, Material);
    Part->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Part->SetCollisionResponseToAllChannels(bCollision ? ECR_Block : ECR_Ignore);
    Part->RegisterComponent();
    Parts.Add(Part);
    return Part;
}

void AHomesteadTownBuilding::Prism(const FVector& A, const FVector& B, const FVector& C, const FVector& Extrude,
    UMaterialInterface* Material)
{
    auto* Mesh = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
    Mesh->SetupAttachment(GetRootComponent());
    const FVector Centroid = (A + B + C) / 3.0f + Extrude * 0.5f;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    const auto Face = [&](const TArray<FVector>& Corners)
    {
        FVector Normal = FVector::CrossProduct(Corners[1] - Corners[0], Corners[2] - Corners[0]).GetSafeNormal();
        if (FVector::DotProduct(Normal, Corners[0] - Centroid) < 0) Normal = -Normal;
        const int32 First = Vertices.Num();
        for (const FVector& Corner : Corners)
        {
            Vertices.Add(Corner);
            Normals.Add(Normal);
            UVs.Add(FVector2D(Corner.X + Corner.Y, Corner.Z) / 200.0f);
        }
        for (int32 I = 1; I + 1 < Corners.Num(); ++I)
        {
            // Both windings, so the face shows whichever way the engine culls.
            Triangles.Append({First, First + I, First + I + 1});
            Triangles.Append({First, First + I + 1, First + I});
        }
    };
    const FVector E = Extrude;
    Face({A, B, C});
    Face({A + E, B + E, C + E});
    Face({A, C, C + E, A + E});
    Face({B, C, C + E, B + E});
    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
    if (Material) Mesh->SetMaterial(0, Material);
    Mesh->SetRelativeLocation(FVector(0, 0, Lift));
    Mesh->RegisterComponent();
    Parts.Add(Mesh);
}

void AHomesteadTownBuilding::Window(const FVector& Origin, const FVector& Out, const FVector& Across, float W, float H,
    UMaterialInterface* Stone)
{
    const FRotator Facing = FRotationMatrix::MakeFromXZ(Out, FVector::UpVector).Rotator();
    UMaterialInterface* Paint = Tint(Cream, 0.6f);
    Box(Origin + Out * 2.0f, FVector(4, W + 14, H + 14), Paint, false, Facing);
    Box(Origin + Out * 3.0f, FVector(3, W, H), Tint(Glass, 0.15f), false, Facing);
    // Two-over-two sashes: a meeting rail and a centre bar.
    Box(Origin + Out * 4.5f, FVector(3, W, 5), Paint, false, Facing);
    Box(Origin + Out * 4.5f, FVector(3, 4, H), Paint, false, Facing);
    Box(Origin + Out * 7.0f - FVector::UpVector * (H * 0.5f + 12.0f), FVector(14, W + 26, 9), Stone, false, Facing);
    Box(Origin + Out * 3.0f + FVector::UpVector * (H * 0.5f + 17.0f), FVector(6, W + 34, 20), Stone, false, Facing);
}

void AHomesteadTownBuilding::Chimney(const FVector& Base, const FVector2D& Size, float Top, UMaterialInterface* Stone)
{
    const float Height = Top - Base.Z;
    Box(FVector(Base.X, Base.Y, Base.Z + Height * 0.5f), FVector(Size.X, Size.Y, Height), Stone);
    Box(FVector(Base.X, Base.Y, Top + 6), FVector(Size.X + 14, Size.Y + 14, 12), Stone);
    const bool bAlongY = Size.Y > Size.X;
    for (float Side : {-1.0f, 1.0f})
    {
        const FVector Pot = FVector(Base.X, Base.Y, Top + 30)
            + (bAlongY ? FVector(0, Side * Size.Y * 0.22f, 0) : FVector(Side * Size.X * 0.22f, 0, 0));
        UStaticMeshComponent* Part = Box(Pot, FVector(26, 26, 36), Tint(Terracotta, 0.8f));
        Part->SetStaticMesh(Cylinder);
    }
}

void AHomesteadTownBuilding::Rebuild()
{
    for (UActorComponent* Part : Parts)
        if (Part) Part->DestroyComponent();
    Parts.Reset();

    const float W = FMath::Max(Width, 300.0f), D = FMath::Max(Depth, 300.0f);
    const float Half = W * 0.5f;
    const float H = FMath::Clamp(Storeys, 1, 3) * StoreyHeight + 30.0f;
    const float DoorY = FMath::Clamp(DoorAt, -1.0f, 1.0f) * FMath::Max(0.0f, Half - DoorWidth * 0.5f - 60.0f);
    // Snap: the floor sits 12 cm above the highest ground under the footprint, the plinth runs down to
    // the lowest, and steps drop from the door to the street.
    Lift = 0.0f;
    float P = FMath::Max(PlinthDepth, 20.0f);
    float StepDrop = 0.0f;
    if (bSnapToGround && HomesteadEstateTerrain::Activate())
    {
        const FTransform& Frame = GetActorTransform();
        const auto GroundAt = [&](float X, float Y)
        {
            const FVector World = Frame.TransformPosition(FVector(X, Y, 0));
            return HomesteadEstateTerrain::Height(World.X, World.Y);
        };
        float Highest = -1e9f, Lowest = 1e9f;
        for (float X : {0.0f, D * 0.5f, D})
            for (float Y : {-Half, 0.0f, Half})
            {
                const float Ground = GroundAt(X, Y);
                if (!FMath::IsFinite(Ground)) continue;
                Highest = FMath::Max(Highest, Ground);
                Lowest = FMath::Min(Lowest, Ground);
            }
        if (Highest > -1e8f)
        {
            Lift = Highest + 12.0f - GetActorLocation().Z;
            P += Highest - Lowest + 12.0f;
            const float Street = GroundAt(-90.0f, DoorY);
            if (FMath::IsFinite(Street)) StepDrop = FMath::Max(0.0f, Highest + 12.0f - Street);
        }
    }
    UMaterialInterface* Granite = Surface(TEXT("WallGranite"), FLinearColor::White);
    if (!Granite) Granite = Tint(GraniteGrey, 0.9f);
    UMaterialInterface* Stone = nullptr;
    switch (Wall)
    {
    case ETownWall::Rubble: Stone = Surface(TEXT("WallGranite"), WallTint); break;
    case ETownWall::Ashlar:
        Stone = Surface(TEXT("WallAshlar"), WallTint);
        if (!Stone) Stone = Surface(TEXT("WallGranite"), WallTint * 1.15f);
        break;
    case ETownWall::Render:
        Stone = Surface(TEXT("Render"), WallTint);
        if (!Stone) Stone = Surface(TEXT("Limewash"), WallTint);
        if (!Stone) Stone = Tint(Lime * WallTint, 0.9f);
        break;
    }
    if (!Stone) Stone = Tint(GraniteGrey * WallTint, 0.9f);
    // Trim (sills, lintels, quoins, plinth) is dressed granite whatever the walling.
    UMaterialInterface* Trim = Wall == ETownWall::Rubble ? Granite : Surface(TEXT("WallAshlar"), FLinearColor::White);
    if (!Trim) Trim = Granite;

    // The solid body, sunk into the slope below the floor.
    Box(FVector(D * 0.5f, 0, (H - P) * 0.5f), FVector(D, W, H + P), Stone, true);
    if (Wall != ETownWall::Rubble)
    {
        Box(FVector(D * 0.5f, 0, (40.0f - P) * 0.5f), FVector(D + 8, W + 8, P + 40), Trim);
        if (Wall == ETownWall::Render)
            for (float Side : {-1.0f, 1.0f})
                for (float Z = 40.0f; Z + 28.0f <= H; Z += 58.0f)
                {
                    // Long and short quoins, alternating up the front corners.
                    const bool bLong = FMath::RoundToInt(Z / 58.0f) % 2 == 0;
                    const FVector Size(bLong ? 26 : 50, bLong ? 50 : 26, 28);
                    Box(FVector(Size.X * 0.5f - 3, Side * (Half - Size.Y * 0.5f + 3), Z + 14), Size, Trim);
                }
    }

    // Roof: slate slabs on a solid gable prism, a ridge capping and the chimney stacks.
    const float Pitch = FMath::DegreesToRadians(FMath::Clamp(RoofPitch, 20.0f, 60.0f));
    const bool bAlongStreet = Roof == ETownRoof::RidgeAlongStreet;
    const float Span = bAlongStreet ? D * 0.5f : Half;
    const float Length = (bAlongStreet ? W : D) + Overhang * 2;
    const float Rise = Span * FMath::Tan(Pitch);
    const float Slope = (Span + Overhang) / FMath::Cos(Pitch);
    const float SlabZ = H + Rise - (Span + Overhang) * 0.5f * FMath::Tan(Pitch) + SlabThickness * 0.5f / FMath::Cos(Pitch);
    const float RidgeZ = H + Rise + SlabThickness / FMath::Cos(Pitch);
    UMaterialInterface* Slates = Slate(Length, Slope);
    if (bAlongStreet)
    {
        Prism(FVector(0, -Half, H), FVector(D, -Half, H), FVector(D * 0.5f, -Half, H + Rise), FVector(0, W, 0), Stone);
        for (float Side : {-1.0f, 1.0f})
            Box(FVector(D * 0.5f + Side * (Span + Overhang) * 0.5f, 0, SlabZ), FVector(Length, Slope, SlabThickness), Slates,
                false, FRotator(0, 90, -Side * FMath::RadiansToDegrees(Pitch)));
        Box(FVector(D * 0.5f, 0, RidgeZ), FVector(24, Length, 12), Tint(RidgeTile, 0.6f));
        for (float Side : {-1.0f, 1.0f})
            if (Chimneys == 2 || Chimneys == static_cast<int32>(Side))
                Chimney(FVector(D * 0.5f, Side * (Half - 38), H - 20), FVector2D(110, 70), H + Rise + 110, Trim);
    }
    else
    {
        Prism(FVector(0, -Half, H), FVector(0, Half, H), FVector(0, 0, H + Rise), FVector(D, 0, 0), Stone);
        for (float Side : {-1.0f, 1.0f})
            Box(FVector(D * 0.5f, Side * (Half + Overhang) * 0.5f, SlabZ), FVector(Length, Slope, SlabThickness), Slates,
                false, FRotator(0, 0, Side * FMath::RadiansToDegrees(Pitch)));
        Box(FVector(D * 0.5f, 0, RidgeZ), FVector(Length, 24, 12), Tint(RidgeTile, 0.6f));
        // -1 stacks on the back gable, 1 on the front.
        for (float End : {-1.0f, 1.0f})
            if (Chimneys == 2 || Chimneys == static_cast<int32>(End))
                Chimney(FVector(End > 0 ? 38 : D - 38, 0, H - 20), FVector2D(70, 110), H + Rise + 110, Trim);
    }

    // The street front: an inert door with a granite step and lintel, windows on every storey, and
    // either a shopfront with an empty fascia board or a plain sign board over the door.
    const FVector Out(-1, 0, 0), Across(0, 1, 0);
    Box(FVector(-4, DoorY, DoorHeight * 0.5f), FVector(4, DoorWidth, DoorHeight), Tint(DoorColor, 0.5f));
    Box(FVector(-1.5f, DoorY, DoorHeight * 0.5f + 4), FVector(3, DoorWidth + 16, DoorHeight + 8), Tint(Cream, 0.6f));
    Box(FVector(-5, DoorY, DoorHeight + 16), FVector(8, DoorWidth + 40, 24), Trim);
    Box(FVector(-24, DoorY, -(StepDrop + 30) * 0.5f), FVector(48, DoorWidth + 50, StepDrop + 30), Granite, true);
    const int32 Steps = FMath::CeilToInt(StepDrop / 20.0f);
    for (int32 Step = 1; Step < Steps; ++Step)
    {
        const float Top = -StepDrop * Step / Steps;
        Box(FVector(-48 - Step * 30.0f + 15, DoorY, (Top - StepDrop - 30) * 0.5f),
            FVector(30, DoorWidth + 50, Top + StepDrop + 30), Granite, true);
    }
    float ShopMin = DoorY, ShopMax = DoorY;
    if (bShopfront)
    {
        UMaterialInterface* Timber = Tint(SignColor, 0.55f);
        for (float Side : {-1.0f, 1.0f})
        {
            const float Near = DoorY + Side * (DoorWidth * 0.5f + 25.0f);
            const float Far = Side * (Half - 55.0f);
            const float Room = (Far - Near) * Side;
            if (Room < 170.0f) continue;
            const float Glaze = FMath::Min(Room, 320.0f);
            const float Y = Near + Side * Glaze * 0.5f;
            ShopMin = FMath::Min(ShopMin, Y - Glaze * 0.5f);
            ShopMax = FMath::Max(ShopMax, Y + Glaze * 0.5f);
            Box(FVector(-4, Y, 30), FVector(8, Glaze + 20, 60), Timber);
            Box(FVector(-3, Y, 145), FVector(6, Glaze + 20, 180), Timber);
            Box(FVector(-5, Y, 145), FVector(4, Glaze, 160), Tint(Glass, 0.12f));
            for (int32 Bar = 1; Bar < FMath::Max(2, FMath::RoundToInt(Glaze / 90.0f)); ++Bar)
                Box(FVector(-7, Y - Glaze * 0.5f + Bar * Glaze / FMath::Max(2, FMath::RoundToInt(Glaze / 90.0f)), 145),
                    FVector(4, 5, 160), Timber);
        }
        // The fascia over the whole front is the (empty) sign board.
        Box(FVector(-8, (ShopMin + ShopMax) * 0.5f, 262), FVector(12, ShopMax - ShopMin + 60, 46), Timber);
        Box(FVector(-12, (ShopMin + ShopMax) * 0.5f, 287), FVector(20, ShopMax - ShopMin + 76, 8), Timber);
    }
    else
    {
        Box(FVector(-5, DoorY, DoorHeight + 58), FVector(6, FMath::Min(W - 120, 240.0f), 46), Tint(SignColor, 0.6f));
    }
    const int32 Bays = FMath::Max(1, FMath::RoundToInt(W / 260.0f));
    for (int32 Storey = 0; Storey < FMath::Clamp(Storeys, 1, 3); ++Storey)
        for (int32 Bay = 0; Bay < Bays; ++Bay)
        {
            const float Y = -Half + (Bay + 0.5f) * W / Bays;
            if (Storey == 0)
            {
                if (FMath::Abs(Y - DoorY) < DoorWidth * 0.5f + 80.0f) continue;
                if (bShopfront && Y > ShopMin - 60.0f && Y < ShopMax + 60.0f) continue;
            }
            Window(FVector(0, Y, Storey * StoreyHeight + 150.0f), Out, Across, 96.0f, 140.0f, Trim);
        }
    if (bSideWindows && D >= 450.0f)
        for (float Side : {-1.0f, 1.0f})
            for (int32 Storey = 0; Storey < FMath::Clamp(Storeys, 1, 3); ++Storey)
                Window(FVector(D * 0.5f, Side * Half, Storey * StoreyHeight + 150.0f), FVector(0, Side, 0),
                    FVector(1, 0, 0), 90.0f, 130.0f, Trim);
}
