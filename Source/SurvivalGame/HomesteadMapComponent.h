#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Styling/SlateBrush.h"
#include "UI/HomesteadMapGeometry.h"

#include <vector>

#include "HomesteadMapComponent.generated.h"

class AHomesteadController;
class UHomesteadEstateMap;
class SWidget;

// Glyphs drawn for named places on the minimap and the Map tab (original shapes; see
// UI/HomesteadMapPainter.h).
enum class EHomesteadMapGlyph : uint8 { Manor, Mine, Cove, Mill, Gateway, Road, Town, Store };

struct FHomesteadMapLandmark
{
    FString Name;
    FString Description;
    EHomesteadMapGlyph Glyph = EHomesteadMapGlyph::Manor;
    HomesteadMap::Vec Position;
};

struct FHomesteadMapParcel
{
    FString Id;
    FString Label;
    std::vector<HomesteadMap::Vec> Ring;
    HomesteadMap::Vec LabelAt;
    bool bOwned = false;
    bool bForSale = false;
};

// Neighbouring estates and communal land (Simulation/HomesteadReservedLand.h): a faint dashed outline,
// named at its centroid when there's room; an empty label draws the outline alone.
struct FHomesteadMapOutline
{
    FString Label;
    std::vector<HomesteadMap::Vec> Ring;
    HomesteadMap::Vec LabelAt;
    bool bFaintest = false;
};

// Everything static the map views draw: rebuilt only when the layout or ownership changes.
struct FHomesteadMapModel
{
    HomesteadMap::MapTransform Transform;
    TArray<FHomesteadMapLandmark> Landmarks;
    TArray<FHomesteadMapParcel> Parcels;
    TArray<FHomesteadMapOutline> Outlines;
    FString EstateName;
};

// Where the Map tab was left, so reopening the book keeps the view.
struct FHomesteadMapViewState
{
    bool bValid = false;
    double PixelsPerCm = 0.0;
    HomesteadMap::Vec Center;
    int32 Selected = INDEX_NONE;
};

// One frame of what the views need; they never touch the Simulation.
struct FHomesteadMapFrame
{
    TSharedPtr<const FHomesteadMapModel> Model;
    const FSlateBrush* MapBrush = nullptr; // Null when T_EstateMap is missing.
    HomesteadMap::Vec Player;
    double FacingYaw = 0.0;
    double CameraYaw = 0.0;
    bool bRotateWithCamera = false;
    bool bInsideEstate = true;
};

// The estate map's presentation: the per-frame snapshot, the HUD minimap overlay and the
// boundary-crossing toast. Lives on the Homestead player controller.
UCLASS()
class SURVIVALGAME_API UHomesteadMapComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHomesteadMapComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    const FHomesteadMapFrame& Frame() const { return CurrentFrame; }
    bool IsMinimapVisible() const;
    bool RotatesWithCamera() const { return bRotateWithCamera; }
    void SetRotatesWithCamera(bool bRotate);
    // The estate's display name: a placeholder until add-ruined-manor-and-arrival stores the
    // player's chosen name.
    static FString EstateName(const AHomesteadController& Controller);
    // Where the minimap sits, in the HUD's 1080-line logical units for a view of that logical size.
    static FBox2D MinimapBox(float ViewWidth, float ViewHeight);
    // The compass trial at the top centre (its band and the landmark tokens hanging under it), in the
    // same units; invalid when the view is too narrow to fit it beside the calendar.
    static FBox2D CompassBox(float ViewWidth, float ViewHeight);
    bool IsCompassVisible() const;
    // Crossings shown so far (for tests), and the text of the last one.
    int32 CrossingCount() const { return Crossings; }
    const FString& LastCrossing() const { return LastCrossingText; }
    TSharedPtr<SWidget> MinimapWidget() const { return Minimap; }
    FHomesteadMapViewState& MapViewState() { return ViewState; }

    static constexpr double BoundaryHysteresisCm = 200.0;
    static constexpr double MinimapCropCm = 12000.0;

private:
    AHomesteadController* Owner() const;
    void RefreshModel();
    void TrackBoundary();
    UPROPERTY() TObjectPtr<UHomesteadEstateMap> MapAsset;
    FSlateBrush MapBrush;
    bool bHasMapBrush = false;
    TSharedPtr<FHomesteadMapModel> Model;
    FString ModelKey;
    FHomesteadMapFrame CurrentFrame;
    TSharedPtr<SWidget> Minimap;
    TSharedPtr<SWidget> Compass;
    FHomesteadMapViewState ViewState;
    bool bRotateWithCamera = false;
    // Boundary crossing: the settled side, and where she was last frame (a long jump is a load or
    // teleport, which settles silently).
    bool bBoundaryKnown = false;
    bool bSettledInside = true;
    HomesteadMap::Vec LastPlayer;
    int32 Crossings = 0;
    FString LastCrossingText;
};
