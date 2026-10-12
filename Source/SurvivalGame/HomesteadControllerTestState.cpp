// Agent test tools: the one-call state dump. BuildTestStateJson() describes the clock, weather, vitals,
// purse, pack, hotbar, tools, lamp, clothes, position, focus, nearby interactables and the open screen.
// HomesteadState logs it (LogHomesteadPlaytest); the editor's get_play_state returns it as "world".
// Reads only; it never changes the game. Works in Development builds.
// Recipes: .github\skills\homestead-test-tools\SKILL.md.
#include "HomesteadController.h"
#include "HomesteadPlaytestSupport.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadMine.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadRain.h"
#include "Simulation/HomesteadShops.h"
#include "UI/SHomesteadShop.h"

#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace TestState
{
// The most shop rows listed, so a long Sell tab stays readable.
constexpr int32 MaxShopRows = 14;

using HomesteadPlaytest::ToText;

TSharedRef<FJsonObject> MakeObject() { return MakeShared<FJsonObject>(); }

FString ClockText(double Hour)
{
    const double HourOfDay = FMath::Fmod(Hour, 24.0);
    const int32 Minutes = FMath::FloorToInt(HourOfDay * 60.0 + 0.5) % (24 * 60);
    return FString::Printf(TEXT("%02d:%02d"), Minutes / 60, Minutes % 60);
}

// Degrees to the right of where she faces (0 ahead, 90 right, 180 behind), -180..180.
double BearingFrom(double FromX, double FromY, double ToX, double ToY, double FacingYaw)
{
    const double Direction = FMath::RadiansToDegrees(FMath::Atan2(ToY - FromY, ToX - FromX));
    return FMath::UnwindDegrees(Direction - FacingYaw);
}

struct FNearby
{
    double Distance = 0.0;
    TSharedPtr<FJsonObject> Object;
};
}

FString AHomesteadController::BuildTestStateJson(int32 NearbyCount, float RadiusCm) const
{
    using namespace HomesteadPlaytest;
    const Homestead::State& Now = State();
    const Homestead::Point Here = PlayerPoint();
    const double FacingYaw = GetControlRotation().Yaw;
    TSharedRef<FJsonObject> Root = TestState::MakeObject();

    // Clock and weather.
    const Homestead::Calendar::Date Today = Sim.Today();
    TSharedRef<FJsonObject> Time = TestState::MakeObject();
    Time->SetNumberField(TEXT("hour"), Now.hour);
    Time->SetStringField(TEXT("clock"), TestState::ClockText(Now.hour));
    Time->SetStringField(TEXT("date"), ToText(Homestead::Calendar::LongDate(Today)));
    Time->SetStringField(TEXT("season"), ToText(Homestead::Calendar::SeasonName(Today.season)));
    Time->SetStringField(TEXT("weekday"), ToText(Homestead::Calendar::WeekdayName(Today.weekday)));
    Time->SetNumberField(TEXT("dayOfSeason"), Today.dayOfSeason);
    Time->SetNumberField(TEXT("daysLeftInSeason"), Today.daysLeftInSeason);
    Time->SetBoolField(TEXT("night"), Sim.IsNight());
    Root->SetObjectField(TEXT("time"), Time);

    TSharedRef<FJsonObject> Weather = TestState::MakeObject();
    Weather->SetBoolField(TEXT("raining"), Homestead::IsRainingAt(Now.hour));
    Weather->SetNumberField(TEXT("rainAmount"), Homestead::RainAmount(Now.hour));
    Weather->SetNumberField(TEXT("overcast"), Homestead::Overcast(Now.hour));
    Weather->SetNumberField(TEXT("groundWetness"), Homestead::GroundWetness(Now.hour));
    Homestead::RainSpell Spell;
    if (Homestead::NextRainSpell(Now.hour, Spell))
        Weather->SetNumberField(TEXT("nextRainStartsInHours"), FMath::Max(0.0, Spell.start - Now.hour));
    Root->SetObjectField(TEXT("weather"), Weather);

    // Vitals and money.
    Root->SetNumberField(TEXT("energy"), Now.energy);
    Root->SetBoolField(TEXT("wellFed"), Sim.IsWellFed());
    TSharedRef<FJsonObject> Purse = TestState::MakeObject();
    Purse->SetNumberField(TEXT("coins"), static_cast<double>(Now.money));
    Purse->SetStringField(TEXT("text"), ToText(Homestead::FormatMoney(Now.money)));
    Root->SetObjectField(TEXT("purse"), Purse);

    // Pack by item, the hotbar and the selected cell.
    TSharedRef<FJsonObject> Pack = TestState::MakeObject();
    for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
    {
        const auto Item = static_cast<Homestead::Item>(Index);
        const int Count = Sim.Count(Item);
        if (Count > 0) Pack->SetNumberField(ToText(Homestead::ItemName(Item)), Count);
    }
    Root->SetObjectField(TEXT("pack"), Pack);
    Root->SetStringField(TEXT("packSummary"), MenuInventorySummary());
    TArray<TSharedPtr<FJsonValue>> Hotbar;
    for (const FHomesteadHotbarSlot& Slot : HotbarSnapshot())
    {
        TSharedRef<FJsonObject> Cell = TestState::MakeObject();
        Cell->SetNumberField(TEXT("index"), Slot.Index);
        Cell->SetStringField(TEXT("key"), FString::FromInt((Slot.Index + 1) % 10));
        Cell->SetStringField(TEXT("item"), Slot.Name);
        Cell->SetNumberField(TEXT("count"), Slot.Count);
        Cell->SetBoolField(TEXT("selected"), Slot.Selected);
        if (Slot.Fill >= 0.0f) Cell->SetNumberField(TEXT("fill"), Slot.Fill);
        Hotbar.Add(MakeShared<FJsonValueObject>(Cell));
    }
    Root->SetArrayField(TEXT("hotbar"), Hotbar);
    Root->SetNumberField(TEXT("selectedHotbarIndex"), SelectedHotbarIndex());

    // Tools and lamp.
    TSharedRef<FJsonObject> Tools = TestState::MakeObject();
    for (int32 Index = 0; Index < Homestead::ToolKindCount; ++Index)
    {
        const auto Tool = static_cast<Homestead::ToolKind>(Index);
        Tools->SetStringField(ToText(Homestead::ToolName(Tool)), ToText(Homestead::ToolTierName(Sim.GetToolTier(Tool))));
    }
    Root->SetObjectField(TEXT("toolTiers"), Tools);
    TSharedRef<FJsonObject> Lamp = TestState::MakeObject();
    Lamp->SetBoolField(TEXT("carried"), Sim.Count(Homestead::Item::OilLamp) > 0);
    Lamp->SetBoolField(TEXT("inHand"), Sim.IsLampInHand());
    Lamp->SetBoolField(TEXT("lit"), Sim.IsLampLit());
    Lamp->SetNumberField(TEXT("oilHours"), Sim.LampOil());
    Root->SetObjectField(TEXT("lamp"), Lamp);

    // Clothes she wears, by slot.
    static const TCHAR* const SlotNames[Homestead::EquipmentSlotCount] = {TEXT("torso"), TEXT("legs"), TEXT("apron"), TEXT("feet"), TEXT("outer")};
    TSharedRef<FJsonObject> Worn = TestState::MakeObject();
    for (int32 Slot = 0; Slot < Homestead::EquipmentSlotCount; ++Slot)
    {
        const Homestead::WearableInstance* Garment = Sim.GetWearable(Now.equipment[Slot]);
        Worn->SetStringField(SlotNames[Slot], Garment ? ToText(Homestead::WearableName(Garment->definition)) : FString());
    }
    Root->SetObjectField(TEXT("equipment"), Worn);

    // Where she is.
    TSharedRef<FJsonObject> Position = TestState::MakeObject();
    Position->SetNumberField(TEXT("x"), Here.x);
    Position->SetNumberField(TEXT("y"), Here.y);
    Position->SetNumberField(TEXT("facingYaw"), FMath::UnwindDegrees(FacingYaw));
    const double HereZ = GetPawn() ? GetPawn()->GetActorLocation().Z : 0.0;
    const bool bHereUnderground = IsUnderground();
    Position->SetNumberField(TEXT("z"), FMath::RoundToInt(HereZ));
    Position->SetBoolField(TEXT("underground"), bHereUnderground);
    Position->SetStringField(TEXT("level"), bHereUnderground ? TEXT("underground") : TEXT("surface"));
    Position->SetBoolField(TEXT("sheltered"), Sim.IsSheltered(Here));
    Position->SetBoolField(TEXT("nearFire"), Sim.IsNearFire(Here));
    Position->SetBoolField(TEXT("nearWater"), Sim.NearWater(Here));
    double NearestLandmark = TNumericLimits<double>::Max();
    for (const Homestead::Landmark& Landmark : Sim.Layout().landmarks)
    {
        const double Distance = FVector2D::Distance(FVector2D(Landmark.position.x, Landmark.position.y), FVector2D(Here.x, Here.y));
        if (Distance < NearestLandmark)
        {
            NearestLandmark = Distance;
            Position->SetStringField(TEXT("nearestLandmark"), ToText(Landmark.name));
            Position->SetNumberField(TEXT("nearestLandmarkCm"), Distance);
        }
    }
    Position->SetBoolField(TEXT("worldReady"), IsWorldReady());
    Position->SetBoolField(TEXT("groundSnapPending"), bPendingGroundSnap);
    if (bPendingGroundSnap) Position->SetNumberField(TEXT("groundSnapPendingSeconds"), FMath::RoundToInt(FPlatformTime::Seconds() - GroundSnapStartedAt));
    Root->SetObjectField(TEXT("position"), Position);

    // What she is facing.
    TSharedRef<FJsonObject> Facing = TestState::MakeObject();
    Facing->SetStringField(TEXT("title"), FocusTitle());
    Facing->SetStringField(TEXT("actions"), FocusActions());
    Root->SetObjectField(TEXT("focus"), Facing);

    // Interactables around her, nearest first.
    TArray<TestState::FNearby> Nearby;
    const double Radius = FMath::Max(100.0, static_cast<double>(RadiusCm));
    // Distances are 2D on one level. Across levels (she is on the yard, the node is in the tunnel below) the height
    // counts, so a tunnel face does not read "2 m away" from the surface above it; mine nodes carry their own z.
    const auto Add = [&](const TCHAR* Kind, int32 Id, const FString& Name, double X, double Y, const FString& Note,
        const Homestead::Mine::Node* MineNode = nullptr)
    {
        double Distance = FVector2D::Distance(FVector2D(X, Y), FVector2D(Here.x, Here.y));
        const bool bTargetUnderground = MineNode != nullptr;
        if (bTargetUnderground != bHereUnderground)
        {
            const double TargetZ = MineNode ? MineNode->z : static_cast<double>(GroundHeight(static_cast<float>(X), static_cast<float>(Y)));
            Distance = FMath::Sqrt(Distance * Distance + FMath::Square(TargetZ - HereZ));
        }
        if (Distance > Radius) return;
        TestState::FNearby Entry;
        Entry.Distance = Distance;
        Entry.Object = TestState::MakeObject();
        Entry.Object->SetStringField(TEXT("kind"), Kind);
        Entry.Object->SetNumberField(TEXT("id"), Id);
        Entry.Object->SetStringField(TEXT("name"), Name);
        Entry.Object->SetNumberField(TEXT("distanceCm"), FMath::RoundToInt(Distance));
        Entry.Object->SetStringField(TEXT("level"), bTargetUnderground ? TEXT("underground") : TEXT("surface"));
        Entry.Object->SetNumberField(TEXT("bearingDeg"), FMath::RoundToInt(TestState::BearingFrom(Here.x, Here.y, X, Y, FacingYaw)));
        Entry.Object->SetNumberField(TEXT("x"), FMath::RoundToInt(X));
        Entry.Object->SetNumberField(TEXT("y"), FMath::RoundToInt(Y));
        if (!Note.IsEmpty()) Entry.Object->SetStringField(TEXT("note"), Note);
        Nearby.Add(MoveTemp(Entry));
    };
    for (const Homestead::ResourceNode& Node : Now.resources)
        if (!Node.cleared) Add(TEXT("resource"), Node.id, ToText(Homestead::ResourceName(Node.kind)), Node.position.x, Node.position.y,
            Node.minTier != Homestead::ToolTier::Worn ? FString::Printf(TEXT("needs %s tool"), *ToText(Homestead::ToolTierName(Node.minTier))) : FString(),
            Homestead::Mine::FindNode(Node.id));
    for (const Homestead::Structure& Structure : Now.structures)
    {
        const Homestead::Point Centre = Homestead::StructureCenter(Now, Structure);
        Add(TEXT("structure"), Structure.id, ToText(Homestead::PieceName(Structure.kind)), Centre.x, Centre.y, FString());
    }
    for (const Homestead::Plot& Plot : Now.plots)
    {
        const Homestead::Point Centre = Homestead::PlotCenter(Plot);
        Add(TEXT("plot"), Plot.id, Plot.planted ? FString(TEXT("Planted plot")) : FString(TEXT("Empty plot")), Centre.x, Centre.y, FString());
    }
    for (const Homestead::WorldDrop& Drop : Now.worldDrops)
        Add(TEXT("drop"), Drop.id, ToText(Homestead::ItemName(Drop.item)), Drop.position.x, Drop.position.y, FString::Printf(TEXT("x%d"), Drop.quantity));
    Nearby.Sort([](const TestState::FNearby& A, const TestState::FNearby& B) { return A.Distance < B.Distance; });
    TArray<TSharedPtr<FJsonValue>> NearbyJson;
    for (int32 Index = 0; Index < Nearby.Num() && Index < NearbyCount; ++Index)
        NearbyJson.Add(MakeShared<FJsonValueObject>(Nearby[Index].Object.ToSharedRef()));
    Root->SetArrayField(TEXT("nearby"), NearbyJson);
    Root->SetNumberField(TEXT("nearbyTotalInRadius"), Nearby.Num());
    Root->SetNumberField(TEXT("nearbyRadiusCm"), Radius);

    // The General Store and the screens that may be open.
    TSharedRef<FJsonObject> Store = TestState::MakeObject();
    if (const Homestead::Shop* Shop = Sim.FindShop(Homestead::ShopKind::GeneralStore))
    {
        const bool bOpen = Homestead::IsShopOpen(*Shop, Now.hour);
        Store->SetBoolField(TEXT("open"), bOpen);
        Store->SetNumberField(TEXT("opensAt"), Shop->openHour);
        Store->SetNumberField(TEXT("closesAt"), Shop->closeHour);
        Store->SetNumberField(TEXT("counterX"), Shop->counterX);
        Store->SetNumberField(TEXT("counterY"), Shop->counterY);
        if (!bOpen) Store->SetStringField(TEXT("closedMessage"), ToText(Homestead::ClosedMessage(*Shop, Now.hour)));
    }
    Root->SetObjectField(TEXT("generalStore"), Store);

    TSharedRef<FJsonObject> Screen = TestState::MakeObject();
    FString Open = TEXT("none");
    if (IsShopScreenOpen())
    {
        Open = TEXT("shop");
        const TSharedPtr<HomesteadMenus::SHomesteadShop> Shopping = GetShopScreen();
        Screen->SetBoolField(TEXT("greeting"), Shopping->IsGreeting());
        Screen->SetStringField(TEXT("tab"), Shopping->IsSellTab() ? TEXT("Sell") : TEXT("Buy"));
        Screen->SetNumberField(TEXT("selection"), Shopping->GetSelection());
        Screen->SetBoolField(TEXT("choosingQuantity"), Shopping->IsChoosingQuantity());
        Screen->SetNumberField(TEXT("quantity"), Shopping->GetQuantity());
        Screen->SetStringField(TEXT("status"), Shopping->GetStatus());
        TArray<TSharedPtr<FJsonValue>> ShopRows;
        for (int32 Row = 0; Row < Shopping->RowCount() && Row < TestState::MaxShopRows; ++Row)
            ShopRows.Add(MakeShared<FJsonValueString>(Shopping->RowLabel(Row)));
        Screen->SetArrayField(TEXT("rows"), ShopRows);
        Screen->SetNumberField(TEXT("rowCount"), Shopping->RowCount());
    }
    else if (ActiveStorageChest().IsSet())
    {
        Open = TEXT("chest");
        Screen->SetNumberField(TEXT("chestId"), ActiveStorageChest().GetValue());
    }
    else if (IsBookOpen())
    {
        static const TCHAR* const PageNames[] = {TEXT("inventory"), TEXT("craft"), TEXT("build"), TEXT("page3"), TEXT("settings"), TEXT("page5"), TEXT("appearance"), TEXT("map")};
        const int32 PageIndex = BookPage();
        Open = FString::Printf(TEXT("book:%s"), PageIndex >= 0 && PageIndex < UE_ARRAY_COUNT(PageNames) ? PageNames[PageIndex] : TEXT("other"));
        Screen->SetNumberField(TEXT("page"), PageIndex);
        Screen->SetStringField(TEXT("title"), BookTitle());
        Screen->SetStringField(TEXT("summary"), BookSummary());
        Screen->SetStringField(TEXT("footer"), BookFooter());
        Screen->SetNumberField(TEXT("selectedRow"), SelectedRow());
    }
    else if (IsPlanning())
    {
        Open = TEXT("placement");
        Screen->SetStringField(TEXT("label"), PlacementLabel());
        Screen->SetStringField(TEXT("status"), PlacementStatus());
    }
    Screen->SetStringField(TEXT("open"), Open);
    Root->SetObjectField(TEXT("screen"), Screen);

    Root->SetStringField(TEXT("toast"), Toast());
    Root->SetBoolField(TEXT("gamepadPrompts"), UsesGamepad());

    FString Out;
    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
    FJsonSerializer::Serialize(Root, Writer);
    return Out;
}

void AHomesteadController::HomesteadState(int32 NearbyCount, float RadiusCm)
{
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("STATE: %s"), *BuildTestStateJson(NearbyCount, RadiusCm));
}
