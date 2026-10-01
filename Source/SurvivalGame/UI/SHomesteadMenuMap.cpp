#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{
TSharedRef<SWidget> SHomesteadMenu::BuildMap()
{
    const TWeakObjectPtr<UHomesteadMapComponent> Presenter = Controller.IsValid() ? Controller->MapPresenter() : nullptr;
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuPine).Padding(4)
    [
        FocusAnchor(SAssignNew(MapView, SHomesteadMapView).Map(Presenter)
            .UsesGamepad_Lambda([this]() { return Controller.IsValid() && Controller->UsesGamepad(); })
            .PlaceAction_Lambda([this]() { return MapTravelLine(); })
            .OnPlaceAction_Lambda([this]()
            {
                if (const auto Destination = MapTravelDestination(); Destination && PointerAction()) OpenTravelPrompt(*Destination);
            }), ERegion::Content, 0)
    ];
}

TOptional<Homestead::TravelDestination> SHomesteadMenu::MapTravelDestination() const
{
    const TOptional<EHomesteadMapGlyph> Glyph = MapView ? MapView->SelectedGlyph() : TOptional<EHomesteadMapGlyph>();
    if (!Glyph) return {};
    if (*Glyph == EHomesteadMapGlyph::Manor) return Homestead::TravelDestination::Manor;
    if (*Glyph == EHomesteadMapGlyph::Town || *Glyph == EHomesteadMapGlyph::Store) return Homestead::TravelDestination::Town;
    return {};
}

FString SHomesteadMenu::MapTravelLine() const
{
    const auto Destination = MapTravelDestination();
    if (!Destination || !Controller.IsValid()) return {};
    const Homestead::TravelPlan Plan = Controller->MenuPlanTravel(*Destination);
    if (!Plan.ok) return UTF8_TO_TCHAR(Plan.error.c_str());
    return FString::Printf(TEXT("%s: walk there, about %s"), Controller->UsesGamepad() ? TEXT("X") : TEXT("T or click here"),
        UTF8_TO_TCHAR(Homestead::FormatWalkDuration(Plan.gameHours).c_str()));
}

void SHomesteadMenu::OpenTravelPrompt(Homestead::TravelDestination Destination)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    const Homestead::TravelPlan Plan = Controller->MenuPlanTravel(Destination);
    if (!Plan.ok)
    {
        // The walk refuses with the reason, as a notice, and changes nothing.
        Controller->MenuTravel(Destination, Controller->Simulation().GetRevision());
        return;
    }
    const uint64 Revision = Controller->Simulation().GetRevision();
    PopupOptions.Reset();
    PopupTitle = Destination == Homestead::TravelDestination::Manor ? TEXT("Walk home to the manor?") : TEXT("Walk into town?");
    PopupBody = UTF8_TO_TCHAR(Plan.summary.c_str());
    PopupOptions.Add({[]() { return FString(TEXT("Set off")); },
        [this, Destination, Revision]() { Controller->MenuTravel(Destination, Revision); }, nullptr});
    PopupOptions.Add({[]() { return FString(TEXT("Stay here")); }, nullptr, nullptr});
    // Centred over the map.
    PopupAnchor = MapView ? MapView->GetCachedGeometry().LocalToAbsolute(MapView->GetCachedGeometry().GetLocalSize() * 0.5f
        - FVector2D(200, 110)) : FVector2D::ZeroVector;
    SetDialog(EDialog::Context);
    bTravelPrompt = true;
    DialogSelection = 0;
    bFocusPending = true;
}

void SHomesteadMenu::OpenSignTravelPrompt(const FString& SignWords, const TArray<Homestead::TravelDestination>& Destinations)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || Destinations.IsEmpty()) return;
    // A single way that can't be walked refuses with the reason, as the Map tab does, and changes nothing.
    if (Destinations.Num() == 1 && !Controller->MenuPlanTravel(Destinations[0]).ok)
    {
        Controller->MenuTravel(Destinations[0], Controller->Simulation().GetRevision());
        return;
    }
    const uint64 Revision = Controller->Simulation().GetRevision();
    PopupOptions.Reset();
    PopupTitle = FString::Printf(TEXT("The sign reads \"%s\""), *SignWords);
    TArray<FString> Lines;
    for (const auto Destination : Destinations)
    {
        const Homestead::TravelPlan Plan = Controller->MenuPlanTravel(Destination);
        const FString Where = UTF8_TO_TCHAR(Homestead::TravelDestinationName(Destination));
        if (Plan.ok) Lines.Add(UTF8_TO_TCHAR(Plan.summary.c_str()));
        PopupOptions.Add({[Where, Plan]()
            {
                return Plan.ok ? FString::Printf(TEXT("Walk to %s (%s)"), *Where, UTF8_TO_TCHAR(Homestead::FormatWalkDuration(Plan.gameHours).c_str()))
                    : FString::Printf(TEXT("Walk to %s"), *Where);
            },
            [this, Destination, Revision]() { Controller->MenuTravel(Destination, Revision); },
            [Plan]() { return Plan.ok; }});
    }
    PopupOptions.Add({[]() { return FString(TEXT("Stay here")); }, nullptr, nullptr});
    PopupBody = FString::Join(Lines, TEXT("\n\n"));
    bCenterPopup = true;
    SetDialog(EDialog::Context);
    bTravelPrompt = true;
    // Start on the first way she can walk.
    DialogSelection = 0;
    for (int32 Index = 0; Index < PopupOptions.Num(); ++Index)
        if (!PopupOptions[Index].Enabled || PopupOptions[Index].Enabled()) { DialogSelection = Index; break; }
    bFocusPending = true;
}

bool SHomesteadMenu::HandleMapKey(FKey Key, EInputEvent Event, float InputAmount)
{
    if (Event == IE_Axis)
    {
        if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_RightY
            || Key == EKeys::Gamepad_LeftTriggerAxis || Key == EKeys::Gamepad_RightTriggerAxis)
        {
            MapView->SetAnalog(Key, InputAmount);
            return true;
        }
        return false;
    }
    const bool Arrow = Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Up || Key == EKeys::Down
        || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down;
    if (Event == IE_Repeat && Arrow) Event = IE_Pressed;
    if (Event != IE_Pressed) return false;
    // The triggers zoom here (through their axes) instead of cycling sections.
    if (Key == EKeys::Gamepad_LeftTrigger || Key == EKeys::Gamepad_RightTrigger) return true;
    if (Key == EKeys::M) { Back(); return true; }
    if (Key == EKeys::T || Key == EKeys::Gamepad_FaceButton_Left)
    {
        if (const auto Destination = MapTravelDestination()) OpenTravelPrompt(*Destination);
        return true;
    }
    const int32 Dx = Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right ? 1 : Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left ? -1 : 0;
    const int32 Dy = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down ? 1 : Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up ? -1 : 0;
    if (Region == ERegion::Tabs)
    {
        if (Dy <= 0) return false;
        Region = ERegion::Content;
        bFocusPending = true;
        SynchronizeFocus();
        return true;
    }
    if (Region != ERegion::Content) return false;
    if (Dx || Dy)
    {
        Hover = INDEX_NONE;
        // Past the last place upward, the focus climbs to the tabs as on every other page.
        if (!MapView->Step(Dx, Dy) && Dy < 0)
        {
            Region = ERegion::Tabs;
            FocusedTab = SeenPage;
            bFocusPending = true;
            SynchronizeFocus();
        }
        return true;
    }
    if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom)
    { MapView->ToggleZoomOnSelected(); return true; }
    if (Key == EKeys::Equals || Key == EKeys::Add) { MapView->ZoomBy(1.5, MapView->GetCachedGeometry().GetLocalSize() * 0.5f); return true; }
    if (Key == EKeys::Hyphen || Key == EKeys::Subtract) { MapView->ZoomBy(1 / 1.5, MapView->GetCachedGeometry().GetLocalSize() * 0.5f); return true; }
    // Pointer clicks belong to the map itself (drag, choose a place).
    return Key == EKeys::LeftMouseButton;
}
}
