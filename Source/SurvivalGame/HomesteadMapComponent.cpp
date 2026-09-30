#include "HomesteadMapComponent.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "HomesteadController.h"
#include "HomesteadEstateMap.h"
#include "Misc/ConfigCacheIni.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadParcels.h"
#include "UI/SHomesteadCompass.h"
#include "UI/SHomesteadMinimap.h"

namespace
{
constexpr const TCHAR* MapSettingsSection = TEXT("/Script/SurvivalGame.HomesteadMap");
constexpr const TCHAR* RotateKey = TEXT("MinimapRotatesWithCamera");
// A frame-to-frame move longer than this is a load or teleport, not a walk.
constexpr double JumpCm = 3000.0;

HomesteadMap::Vec ToVec(Homestead::Point Point) { return {Point.x, Point.y}; }

struct FLandmarkInfo
{
    const char* Anchor;
    const TCHAR* Name;
    const TCHAR* Description;
    EHomesteadMapGlyph Glyph;
};
}

UHomesteadMapComponent::UHomesteadMapComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

AHomesteadController* UHomesteadMapComponent::Owner() const
{
    return Cast<AHomesteadController>(GetOwner());
}

FString UHomesteadMapComponent::EstateName(const AHomesteadController& Controller)
{
    // The name she chose in the Names step; the placeholder until she has.
    const std::string& Name = Controller.State().estateName;
    return Name.empty() ? FString(TEXT("Trevennor")) : FString(UTF8_TO_TCHAR(Name.c_str()));
}

FBox2D UHomesteadMapComponent::MinimapBox(float ViewWidth, float ViewHeight)
{
    // The bottom-right corner, level with the hotbar's bottom edge; about 220 px at 1080p.
    constexpr float Size = 220, Right = 30, Bottom = 22;
    return FBox2D(FVector2D(ViewWidth - Right - Size, ViewHeight - Bottom - Size), FVector2D(ViewWidth - Right, ViewHeight - Bottom));
}

FBox2D UHomesteadMapComponent::CompassBox(float ViewWidth, float ViewHeight)
{
    // Centred on the top row, level with the calendar (HomesteadHudLayout::CalendarTop), and never
    // closer than Gap to the calendar panel at the top-right (AHomesteadHUD::DrawHUD's CalendarX).
    // The band is 40 units tall; the landmark tokens hang up to 30 below it.
    constexpr float Top = 26, Height = 70, MaxWidth = 460, MinWidth = 280, Gap = 16, CalendarWidth = 460, Margin = 30;
    const float CalendarLeft = FMath::Max(Margin, ViewWidth - Margin - CalendarWidth);
    const float Half = FMath::Min(MaxWidth * 0.5f, FMath::Min(ViewWidth * 0.5f - Margin, CalendarLeft - Gap - ViewWidth * 0.5f));
    if (Half * 2 < MinWidth || ViewHeight < Top + Height) return FBox2D(ForceInit);
    return FBox2D(FVector2D(ViewWidth * 0.5f - Half, Top), FVector2D(ViewWidth * 0.5f + Half, Top + Height));
}

bool UHomesteadMapComponent::IsCompassVisible() const
{
    // It shares the top row with the first-minute controls strip, so it waits for that to retire.
    const AHomesteadController* Controller = Owner();
    return IsMinimapVisible() && Controller && !Controller->IsControlsHintOnScreen();
}

void UHomesteadMapComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GConfig) GConfig->GetBool(MapSettingsSection, RotateKey, bRotateWithCamera, GGameUserSettingsIni);
    MapAsset = LoadObject<UHomesteadEstateMap>(nullptr, UHomesteadEstateMap::AssetPath);
    if (MapAsset && MapAsset->Texture)
    {
        MapBrush.SetResourceObject(MapAsset->Texture);
        MapBrush.ImageSize = FVector2D(MapAsset->Texture->GetSizeX(), MapAsset->Texture->GetSizeY());
        MapBrush.DrawAs = ESlateBrushDrawType::Image;
        bHasMapBrush = true;
    }
    else UE_LOG(LogTemp, Warning, TEXT("Homestead: the estate map (%s) is missing; maps draw without terrain."), UHomesteadEstateMap::AssetPath);
    const AHomesteadController* Controller = Owner();
    if (Controller && Controller->IsLocalController() && GEngine && GEngine->GameViewport)
    {
        Minimap = SNew(HomesteadMenus::SHomesteadMinimap).Map(this);
        GEngine->GameViewport->AddViewportWidgetContent(Minimap.ToSharedRef(), 45);
        Compass = SNew(HomesteadMenus::SHomesteadCompass).Map(this);
        GEngine->GameViewport->AddViewportWidgetContent(Compass.ToSharedRef(), 45);
    }
}

void UHomesteadMapComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Minimap.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Minimap.ToSharedRef());
    if (Compass.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Compass.ToSharedRef());
    Minimap.Reset();
    Compass.Reset();
    Super::EndPlay(Reason);
}

bool UHomesteadMapComponent::IsMinimapVisible() const
{
    const AHomesteadController* Controller = Owner();
    // Hidden with the rest of the HUD, and behind the new-game setup and shop screens.
    return Controller && CurrentFrame.Model.IsValid() && Controller->ShouldShowHotbar() && !Controller->HasNativeMenu()
        && !Controller->IsNewGameSetup() && !Controller->IsNamingSetup() && !Controller->IsShopScreenOpen();
}

void UHomesteadMapComponent::SetRotatesWithCamera(bool bRotate)
{
    bRotateWithCamera = bRotate;
    CurrentFrame.bRotateWithCamera = bRotate;
    if (!GConfig) return;
    GConfig->SetBool(MapSettingsSection, RotateKey, bRotate, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UHomesteadMapComponent::RefreshModel()
{
    const AHomesteadController* Controller = Owner();
    if (!Controller) return;
    const Homestead::Simulation& Sim = Controller->Simulation();
    const Homestead::EstateLayout& Layout = Sim.Layout();
    // Until the fixed estate is running, show the layout's new-game parcels so the map reads.
    const std::vector<Homestead::Parcel> Parcels = Sim.GetState().parcels.empty()
        ? Homestead::ParcelsFromLayout(Layout) : Sim.GetState().parcels;
    FString Key = FString::Printf(TEXT("%p:%d:%s:"), &Layout, Layout.version, *EstateName(*Controller));
    for (const auto& Parcel : Parcels) Key += Parcel.owned ? TEXT("1") : TEXT("0");
    if (Model.IsValid() && Key == ModelKey) return;
    ModelKey = Key;

    TSharedPtr<FHomesteadMapModel> Next = MakeShared<FHomesteadMapModel>();
    if (MapAsset) Next->Transform = MapAsset->Transform();
    Next->EstateName = EstateName(*Controller);
    for (const auto& Parcel : Parcels)
    {
        FHomesteadMapParcel Shape;
        Shape.Id = UTF8_TO_TCHAR(Parcel.id.c_str());
        for (const auto& Point : Parcel.polygon) Shape.Ring.push_back(ToVec(Point));
        Shape.bOwned = Parcel.owned;
        Shape.bForSale = Parcel.forSale && !Parcel.owned;
        Shape.LabelAt = HomesteadMap::Centroid(Shape.Ring);
        FString Name = Shape.Id;
        Name.RemoveFromStart(UTF8_TO_TCHAR(Homestead::Anchor::ForSaleParcelPrefix));
        // "MoorField" -> "Moor Field"
        FString Spaced;
        for (int32 Index = 0; Index < Name.Len(); ++Index)
        {
            if (Index > 0 && FChar::IsUpper(Name[Index]) && FChar::IsLower(Name[Index - 1])) Spaced += TEXT(' ');
            Spaced += Name[Index] == TEXT('_') ? TEXT(' ') : Name[Index];
        }
        Shape.Label = Parcel.id == Homestead::Anchor::EstateBoundary ? Next->EstateName : Spaced;
        Next->Parcels.Add(MoveTemp(Shape));
    }
    const FLandmarkInfo Places[] = {
        {Homestead::Anchor::MineEntrance, TEXT("Mine ruin"), TEXT("The family mine's engine house on the clifftop, long flooded."), EHomesteadMapGlyph::Mine},
        {Homestead::Anchor::CoveBeach, TEXT("The cove"), TEXT("Your own little beach where the river meets the sea."), EHomesteadMapGlyph::Cove},
        {Homestead::Anchor::MillSite, TEXT("Mill site"), TEXT("The old water mill's footings, by the ford."), EHomesteadMapGlyph::Mill},
        {Homestead::Anchor::EstateGateway, TEXT("Estate gateway"), TEXT("Where the estate drive meets the road to town."), EHomesteadMapGlyph::Gateway},
        {Homestead::Anchor::TownSquare, TEXT("Town"), TEXT("The harbour town above the estuary."), EHomesteadMapGlyph::Town},
        {Homestead::Anchor::GeneralStoreDoor, TEXT("General store"), TEXT("Tools, seed and supplies; it buys your goods too."), EHomesteadMapGlyph::Store},
    };
    // The manor first: its footprint's middle, else where she wakes.
    if (const Homestead::LandmarkPolygon* Manor = Layout.FindPolygon(Homestead::Anchor::ManorFootprint))
    {
        std::vector<HomesteadMap::Vec> Ring;
        for (const auto& Point : Manor->points) Ring.push_back(ToVec(Point));
        Next->Landmarks.Add({TEXT("The manor"), TEXT("Your family's ruined house, and its one standing room."), EHomesteadMapGlyph::Manor,
            HomesteadMap::Centroid(Ring)});
    }
    else if (const Homestead::Landmark* Spawn = Layout.FindLandmark(Homestead::Anchor::StandingRoomSpawn))
        Next->Landmarks.Add({TEXT("The manor"), TEXT("Your family's ruined house, and its one standing room."), EHomesteadMapGlyph::Manor, ToVec(Spawn->position)});
    for (const FLandmarkInfo& Place : Places)
        if (const Homestead::Landmark* Found = Layout.FindLandmark(Place.Anchor))
            Next->Landmarks.Add({Place.Name, Place.Description, Place.Glyph, ToVec(Found->position)});
    // The road itself is drawn on the map; it has no label (Jenny, 2026-09-29).
    Model = Next;
}

void UHomesteadMapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AHomesteadController* Controller = Owner();
    if (!Controller) return;
    RefreshModel();
    CurrentFrame.Model = Model;
    CurrentFrame.MapBrush = bHasMapBrush ? &MapBrush : nullptr;
    CurrentFrame.Player = ToVec(Controller->PlayerPoint());
    CurrentFrame.FacingYaw = Controller->GetPawn() ? Controller->GetPawn()->GetActorRotation().Yaw : Controller->GetControlRotation().Yaw;
    CurrentFrame.CameraYaw = Controller->GetControlRotation().Yaw;
    CurrentFrame.bRotateWithCamera = bRotateWithCamera;
    TrackBoundary();
}

void UHomesteadMapComponent::TrackBoundary()
{
    AHomesteadController* Controller = Owner();
    if (!Controller || !Model.IsValid()) return;
    bool bInside = false, bAnyOwned = false;
    double Distance = TNumericLimits<double>::Max();
    for (const FHomesteadMapParcel& Parcel : Model->Parcels)
    {
        if (!Parcel.bOwned) continue;
        bAnyOwned = true;
        bInside |= HomesteadMap::Contains(Parcel.Ring, CurrentFrame.Player);
        Distance = FMath::Min(Distance, HomesteadMap::DistanceToRing(Parcel.Ring, CurrentFrame.Player));
    }
    CurrentFrame.bInsideEstate = bInside || !bAnyOwned;
    // Menus freeze her; nothing is announced while one is open.
    if (!bAnyOwned || !Controller->IsWorldReady() || Controller->IsBookOpen() || Controller->HasNativeMenu() || Controller->IsFailed()
        || Controller->IsNewGameSetup() || Controller->IsNamingSetup() || Controller->IsShopScreenOpen())
        return;
    const double Moved = FMath::Sqrt(FMath::Square(CurrentFrame.Player.x - LastPlayer.x) + FMath::Square(CurrentFrame.Player.y - LastPlayer.y));
    LastPlayer = CurrentFrame.Player;
    if (!bBoundaryKnown || Moved > JumpCm)
    {
        bBoundaryKnown = true;
        bSettledInside = bInside;
        return;
    }
    // Hysteresis: the side only changes once she is well clear of the line.
    if (bInside == bSettledInside || Distance < BoundaryHysteresisCm) return;
    bSettledInside = bInside;
    ++Crossings;
    LastCrossingText = FString::Printf(TEXT("%s the %s estate"), bInside ? TEXT("Entering") : TEXT("Leaving"), *Model->EstateName);
    Controller->Notify(LastCrossingText, false);
}
