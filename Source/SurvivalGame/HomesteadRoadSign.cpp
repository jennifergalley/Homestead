#include "HomesteadRoadSign.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

// The stand-in's proportions, cm: a weathered post with a board across its top.
namespace RoadSignStyle
{
constexpr float PostHeight = 190.0f, PostWidth = 12.0f;
constexpr float BoardWidth = 110.0f, BoardHeight = 30.0f, BoardDepth = 5.0f;
constexpr float TextSize = 11.0f;
// Props' SM_RoadSign (Content/Python road_sign.py): same board height and width, pivot at the post
// foot, a 4 cm board whose painted face is 2 cm off the centreline with a bead frame 0.6 cm proud,
// and a painted field of 98 x 20 cm inside the bead that the words must stay within.
constexpr float AuthoredBoardDepth = 4.0f, AuthoredFaceProud = 0.6f;
constexpr float PaintedWidth = 98.0f, PaintedHeight = 20.0f;
// Words sit just proud of the board's face (or the authored bead) so they never z-fight it.
constexpr float StandInFaceProud = 0.6f;
const FColor Paint(238, 226, 196);
const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
const TCHAR* Material = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
}

AHomesteadRoadSign::AHomesteadRoadSign()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    const auto Part = [this](const TCHAR* Name)
    {
        auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(Root);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return Mesh;
    };
    Post = Part(TEXT("StandInPost"));
    Board = Part(TEXT("StandInBoard"));
    Authored = Part(TEXT("SignMesh"));
    Authored->SetVisibility(false);
    const auto Words = [this](const TCHAR* Name, float Yaw)
    {
        auto* Text = CreateDefaultSubobject<UTextRenderComponent>(Name);
        Text->SetupAttachment(Root);
        Text->SetRelativeRotation(FRotator(0, Yaw, 0));
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetVerticalAlignment(EVRTA_TextCenter);
        Text->SetWorldSize(RoadSignStyle::TextSize);
        Text->SetTextRenderColor(RoadSignStyle::Paint);
        Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Text->SetCastShadow(false);
        return Text;
    };
    Front = Words(TEXT("FrontWords"), 0.0f);
    Back = Words(TEXT("BackWords"), 180.0f);
}

void AHomesteadRoadSign::Place(const FString& Name, const FVector& Location, float Yaw, const FString& Words)
{
    SignName = Name;
    SetActorLocationAndRotation(Location, FRotator(0, Yaw, 0));
    if (!Authored->GetStaticMesh() && FPackageName::DoesPackageExist(MeshPackage))
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshObject)) Authored->SetStaticMesh(Mesh);
    bStandIn = Authored->GetStaticMesh() == nullptr;
    Authored->SetVisibility(!bStandIn);
    Post->SetVisibility(bStandIn);
    Board->SetVisibility(bStandIn);
    if (bStandIn && !Post->GetStaticMesh())
    {
        UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, RoadSignStyle::Cylinder);
        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, RoadSignStyle::Cube);
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, RoadSignStyle::Material);
        // Basic shapes are 100 cm across with their pivot at the middle.
        Post->SetStaticMesh(Cylinder);
        Post->SetRelativeScale3D(FVector(RoadSignStyle::PostWidth, RoadSignStyle::PostWidth, RoadSignStyle::PostHeight) / 100.0f);
        Post->SetRelativeLocation(FVector(0, 0, RoadSignStyle::PostHeight * 0.5f));
        Board->SetStaticMesh(Cube);
        Board->SetRelativeScale3D(FVector(RoadSignStyle::BoardDepth, RoadSignStyle::BoardWidth, RoadSignStyle::BoardHeight) / 100.0f);
        Board->SetRelativeLocation(FVector(0, 0, RoadSignStyle::PostHeight - RoadSignStyle::BoardHeight * 0.5f));
        if (Material) { Post->SetMaterial(0, Material); Board->SetMaterial(0, Material); }
        if (!Cylinder || !Cube) UE_LOG(LogTemp, Warning, TEXT("Road sign stand-in shapes are missing."));
    }
    const float Standoff = bStandIn ? RoadSignStyle::BoardDepth * 0.5f + RoadSignStyle::StandInFaceProud
        : RoadSignStyle::AuthoredBoardDepth * 0.5f + RoadSignStyle::AuthoredFaceProud;
    const FText Text = FText::FromString(bStandIn ? Words + TEXT("\n(stand-in sign)") : Words);
    for (UTextRenderComponent* Face : {Front.Get(), Back.Get()})
    {
        const float Yaw = Face == Front ? 0.0f : 180.0f;
        Face->SetRelativeLocation(FVector(0, 0, RoadSignStyle::PostHeight - RoadSignStyle::BoardHeight * 0.5f)
            + FRotator(0, Yaw, 0).Vector() * Standoff);
        Face->SetWorldSize(RoadSignStyle::TextSize);
        Face->SetText(Text);
        // On the authored board, a longer label shrinks to stay inside the painted field.
        if (!bStandIn)
        {
            const FVector Size = Face->GetTextLocalSize();
            const float Fit = FMath::Min(Size.Y > 0 ? RoadSignStyle::PaintedWidth / Size.Y : 1.0f,
                Size.Z > 0 ? RoadSignStyle::PaintedHeight / Size.Z : 1.0f);
            if (Fit < 1.0f) Face->SetWorldSize(RoadSignStyle::TextSize * Fit);
        }
    }
}
