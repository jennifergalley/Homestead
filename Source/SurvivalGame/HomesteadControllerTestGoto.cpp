// Agent test tools: HomesteadGoto and HomesteadUse. Stand her facing a landmark, a placement or the
// nearest thing of a kind, then (Use) select a hotbar item and use it on what she faces, or press
// Interact. Teleport is the existing HomesteadTeleport; this adds only the wait for the ground snap, the
// facing and the log. Works in Development builds. Recipes: .github\skills\homestead-test-tools\SKILL.md.
#include "HomesteadController.h"
#include "HomesteadPlaytestSupport.h"
#include "HomesteadCharacter.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadMine.h"
#include "Simulation/HomesteadShops.h"

#include "Engine/World.h"
#include "TimerManager.h"

namespace TestGoto
{
// How often the teleport's ground snap is polled, and when to give up on it, in real seconds.
constexpr float PollIntervalSeconds = 0.25f;
constexpr double TimeoutSeconds = 90.0;
// HomesteadTeleport's "no height given" value: stand on the terrain.
constexpr double NoZ = -1000000.0;
// She counts as arrived when she settled this close to where the teleport aimed, in cm (an abandoned ground snap
// puts her back where she started).
constexpr double ArrivalToleranceCm = 300.0;
// Underground the teleport starts over the floor: HomesteadTeleport adds 100, which makes the ground snap's
// 150 start height over the spot that MoveWithinMine uses, in cm.
constexpr double UndergroundLiftCm = 50.0;
// How far out from the mine house door she stands in the yard, and how far from a rock face underground, in cm.
constexpr double MineYardOutCm = 120.0;
constexpr double MineNodeStandCm = 130.0;
// Where she stands from a placement, back toward where she came from, in cm; focus reaches about 280.
constexpr double StandBackCm = 150.0;
// Her stand point from the shop counter, along the counter's facing, in cm (as HomesteadSellAtStore).
constexpr double CounterStandCm = 250.0;
// Real seconds between arriving and acting, so focus and the camera have ticked once at the new spot.
constexpr float SettleSeconds = 0.5f;
// Real seconds after the action before the outcome (toast, pack) is logged; swings land late.
constexpr float OutcomeSeconds = 3.5f;

struct FAlias { const TCHAR* Word; const char* Anchor; };
const FAlias Aliases[] = {
    {TEXT("store"), Homestead::Anchor::GeneralStoreDoor}, {TEXT("shop"), Homestead::Anchor::GeneralStoreDoor},
    {TEXT("storedoor"), Homestead::Anchor::GeneralStoreDoor},
    {TEXT("cove"), Homestead::Anchor::CoveBeach}, {TEXT("beach"), Homestead::Anchor::CoveBeach},
    {TEXT("square"), Homestead::Anchor::TownSquare}, {TEXT("town"), Homestead::Anchor::TownSquare},
    {TEXT("gate"), Homestead::Anchor::EstateGateway}, {TEXT("gateway"), Homestead::Anchor::EstateGateway},
    {TEXT("mill"), Homestead::Anchor::MillSite}, {TEXT("farm"), Homestead::Anchor::DerelictFarmGate},
    {TEXT("home"), Homestead::Anchor::StandingRoomSpawn}, {TEXT("room"), Homestead::Anchor::StandingRoomSpawn},
    {TEXT("spawn"), Homestead::Anchor::StandingRoomSpawn},
};

double YawToward(double FromX, double FromY, double ToX, double ToY)
{
    return FMath::RadiansToDegrees(FMath::Atan2(ToY - FromY, ToX - FromX));
}

double Distance(const Homestead::Point& A, double X, double Y)
{
    return FMath::Sqrt(FMath::Square(A.x - X) + FMath::Square(A.y - Y));
}

// The tunnel floor height at X,Y: the nearest floor segment's z (the mine node's own z when there are none).
double MineFloorZNear(double X, double Y, double Fallback)
{
    double BestGap = TNumericLimits<double>::Max();
    double Z = Fallback;
    for (const Homestead::Mine::FloorSegment& Floor : Homestead::Mine::GetLayout().floors)
    {
        const double Dx = Floor.b.x - Floor.a.x;
        const double Dy = Floor.b.y - Floor.a.y;
        const double LengthSquared = Dx * Dx + Dy * Dy;
        const double T = LengthSquared > 0.0 ? FMath::Clamp(((X - Floor.a.x) * Dx + (Y - Floor.a.y) * Dy) / LengthSquared, 0.0, 1.0) : 0.0;
        const double Gap = FMath::Sqrt(FMath::Square(X - (Floor.a.x + Dx * T)) + FMath::Square(Y - (Floor.a.y + Dy * T))) - Floor.halfWidth;
        if (Gap < BestGap) { BestGap = Gap; Z = Floor.z; }
    }
    return Z;
}
}

bool AHomesteadController::RejectTestWhileSnapping(const TCHAR* Tag)
{
    if (!bPendingGroundSnap) return false;
    UE_LOG(LogHomesteadPlaytest, Error, TEXT("%s: refused: she is still being placed from the last teleport (pending for %.0f s). Wait for \"arrived\" or the ground snap's HOMESTEAD_GROUND_TIMEOUT, then retry."),
        Tag, FPlatformTime::Seconds() - GroundSnapStartedAt);
    RejectPendingGroundSnapAction();
    return true;
}

bool AHomesteadController::BeginTestTeleport(double X, double Y, const FRotator& Facing, const TCHAR* Tag, TFunction<void()> Then,
    TFunction<void()> Failed, double Z)
{
    FTimerManager& Timers = GetWorldTimerManager();
    Timers.ClearTimer(TestTeleportTimer);
    const bool bUnderground = Z > TestGoto::NoZ * 0.5 && Homestead::Mine::IsUnderground({X, Y}, Z);
    HomesteadTeleport(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
    if (!bPendingGroundSnap)
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("%s: the teleport to (%.0f, %.0f) did not start (no pawn, or one is already pending)."), Tag, X, Y);
        return false;
    }
    const double StartedAt = FPlatformTime::Seconds();
    const FString TagText(Tag);
    Timers.SetTimer(TestTeleportTimer, FTimerDelegate::CreateWeakLambda(this, [this, StartedAt, X, Y, bUnderground, Facing, TagText, Then, Failed]()
    {
        if (bPendingGroundSnap || GroundSnapStreamingActor)
        {
            if (FPlatformTime::Seconds() - StartedAt > TestGoto::TimeoutSeconds)
            {
                UE_LOG(LogHomesteadPlaytest, Error, TEXT("%s: FAILED: gave up after %.0f s waiting for the teleport to settle."), *TagText, TestGoto::TimeoutSeconds);
                const TFunction<void()> OnFailed = Failed;
                GetWorldTimerManager().ClearTimer(TestTeleportTimer);
                if (OnFailed) OnFailed();
            }
            return;
        }
        // Clearing the timer destroys this lambda and its captures, so keep what the rest needs on the stack.
        const FRotator Face = Facing;
        const TFunction<void()> Next = Then;
        const TFunction<void()> OnFailed = Failed;
        const FString Label = TagText;
        const double TargetX = X;
        const double TargetY = Y;
        const bool bNeedsUnderground = bUnderground;
        GetWorldTimerManager().ClearTimer(TestTeleportTimer);
        // An abandoned ground snap puts her back where she started: that is a failed trip, not an arrival.
        const Homestead::Point Now = PlayerPoint();
        const APawn* Avatar = GetPawn();
        const bool bPlaced = FMath::Sqrt(FMath::Square(Now.x - TargetX) + FMath::Square(Now.y - TargetY)) <= TestGoto::ArrivalToleranceCm
            && (!bNeedsUnderground || (Avatar && Homestead::Mine::IsUnderground(Now, Avatar->GetActorLocation().Z)));
        if (!bPlaced)
        {
            UE_LOG(LogHomesteadPlaytest, Error, TEXT("%s: FAILED: ended at (%.0f, %.0f, z %.0f), not the target (%.0f, %.0f)%s. The ground snap was abandoned (look for HOMESTEAD_GROUND_TIMEOUT in the log), so she is back where she started."),
                *Label, Now.x, Now.y, Avatar ? Avatar->GetActorLocation().Z : 0.0, TargetX, TargetY, bNeedsUnderground ? TEXT(" underground") : TEXT(""));
            if (OnFailed) OnFailed();
            return;
        }
        if (AHomesteadCharacter* Heroine = Cast<AHomesteadCharacter>(GetPawn()))
        {
            Heroine->SetActorRotation(FRotator(0.0, Face.Yaw, 0.0));
            Heroine->SetRestingViewRotation(Face);
            Heroine->SnapCamera();
        }
        SetControlRotation(Face);
        if (Next) Next();
    }), TestGoto::PollIntervalSeconds, true);
    return true;
}

bool AHomesteadController::ResolveTestTarget(const FString& Target, double& StandX, double& StandY, double& StandZ, FRotator& Facing, FString& Described) const
{
    using namespace HomesteadPlaytest;
    StandZ = TestGoto::NoZ;
    const FString Wanted = Compact(Target);
    if (Wanted.IsEmpty()) return false;
    const Homestead::Point Here = PlayerPoint();
    const APawn* Avatar = GetPawn();
    const double HereZ = Avatar ? Avatar->GetActorLocation().Z : 0.0;
    const bool bHereUnderground = bEstateMap && Homestead::Mine::IsUnderground(Here, HereZ);

    // Underground: stand on the tunnel floor in front of a rock face, looking at it (the node's normal points out
    // of the rock into the gallery). The height is the tunnel floor's, not the surface's above it.
    const auto AtMineNode = [&](const Homestead::Mine::Node& Node)
    {
        double NormalX = Node.normalX;
        double NormalY = Node.normalY;
        const double Length = FMath::Sqrt(NormalX * NormalX + NormalY * NormalY);
        if (Length < 0.01) { NormalX = 1.0; NormalY = 0.0; }
        else { NormalX /= Length; NormalY /= Length; }
        StandX = Node.x + NormalX * TestGoto::MineNodeStandCm;
        StandY = Node.y + NormalY * TestGoto::MineNodeStandCm;
        const double FloorZ = TestGoto::MineFloorZNear(StandX, StandY, Node.z);
        StandZ = FloorZ + TestGoto::UndergroundLiftCm;
        Facing = FRotator(0.0, TestGoto::YawToward(StandX, StandY, Node.x, Node.y), 0.0);
        Described = FString::Printf(TEXT("mine node %d (%s) underground at (%.0f, %.0f), floor z %.0f"), Node.id,
            *ToText(Homestead::ResourceName(Homestead::Mine::ResourceFor(Node.kind))), Node.x, Node.y, FloorZ);
        return true;
    };
    const Homestead::Mine::Layout& MineLayout = Homestead::Mine::GetLayout();
    {
        const bool bYard = Wanted.Equals(TEXT("mine"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("minehouse"), ESearchCase::IgnoreCase)
            || Wanted.Equals(TEXT("minedoor"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("mineyard"), ESearchCase::IgnoreCase)
            || Wanted.Equals(TEXT("mineentrance"), ESearchCase::IgnoreCase);
        // The yard in front of the mine house door, looking at the door (outward = from the house origin to its door).
        if (bYard)
        {
            double OutX = MineLayout.door.x - MineLayout.house.x;
            double OutY = MineLayout.door.y - MineLayout.house.y;
            const double Length = FMath::Sqrt(OutX * OutX + OutY * OutY);
            if (Length < 1.0) { OutX = 1.0; OutY = 0.0; }
            else { OutX /= Length; OutY /= Length; }
            StandX = MineLayout.door.x + OutX * TestGoto::MineYardOutCm;
            StandY = MineLayout.door.y + OutY * TestGoto::MineYardOutCm;
            Facing = FRotator(0.0, TestGoto::YawToward(StandX, StandY, MineLayout.door.x, MineLayout.door.y), 0.0);
            Described = FString::Printf(TEXT("the mine house door yard (door at (%.0f, %.0f))"), MineLayout.door.x, MineLayout.door.y);
            return true;
        }
        // The foot of the mine ladder, on the tunnel floor.
        if (Wanted.Equals(TEXT("mineladder"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("underground"), ESearchCase::IgnoreCase)
            || Wanted.Equals(TEXT("tunnel"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("tunnels"), ESearchCase::IgnoreCase)
            || Wanted.Equals(TEXT("ladderfoot"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("minefloor"), ESearchCase::IgnoreCase))
        {
            StandX = MineLayout.ladderFoot.x;
            StandY = MineLayout.ladderFoot.y;
            StandZ = MineLayout.ladderFoot.z + TestGoto::UndergroundLiftCm;
            Facing = FRotator(0.0, FMath::UnwindDegrees(MineLayout.ladderFoot.yaw), 0.0);
            Described = FString::Printf(TEXT("the mine ladder foot underground at (%.0f, %.0f), floor z %.0f"), StandX, StandY, MineLayout.ladderFoot.z);
            return true;
        }
    }

    // A point she stands at looking at a thing: back off toward where she is now.
    const auto FaceThing = [&](double X, double Y, const FString& Label)
    {
        double BackX = Here.x - X;
        double BackY = Here.y - Y;
        const double Length = FMath::Sqrt(BackX * BackX + BackY * BackY);
        if (Length < 1.0) { BackX = -1.0; BackY = 0.0; }
        else { BackX /= Length; BackY /= Length; }
        StandX = X + BackX * FMath::Min(TestGoto::StandBackCm, FMath::Max(Length, 1.0));
        StandY = Y + BackY * FMath::Min(TestGoto::StandBackCm, FMath::Max(Length, 1.0));
        Facing = FRotator(0.0, TestGoto::YawToward(StandX, StandY, X, Y), 0.0);
        Described = FString::Printf(TEXT("%s at (%.0f, %.0f)"), *Label, X, Y);
        return true;
    };
    const auto AtLandmark = [&](const Homestead::Landmark& Landmark)
    {
        StandX = Landmark.position.x;
        StandY = Landmark.position.y;
        Facing = FRotator(0.0, FMath::UnwindDegrees(Landmark.yaw), 0.0);
        Described = FString::Printf(TEXT("landmark %s at (%.0f, %.0f)"), *ToText(Landmark.name), StandX, StandY);
        return true;
    };

    // "x,y": any point on the map; she faces north (+X).
    if (Target.Contains(TEXT(",")))
    {
        TArray<FString> Coordinates;
        Target.ParseIntoArray(Coordinates, TEXT(","));
        if (Coordinates.Num() == 2 && Coordinates[0].TrimStartAndEnd().IsNumeric() && Coordinates[1].TrimStartAndEnd().IsNumeric())
        {
            StandX = FCString::Atod(*Coordinates[0]);
            StandY = FCString::Atod(*Coordinates[1]);
            Facing = FRotator::ZeroRotator;
            Described = FString::Printf(TEXT("point (%.0f, %.0f)"), StandX, StandY);
            return true;
        }
    }

    // The shop counter, from the customer's side.
    if (Wanted.Equals(TEXT("counter"), ESearchCase::IgnoreCase) || Wanted.Equals(TEXT("storecounter"), ESearchCase::IgnoreCase))
    {
        if (const Homestead::Shop* Shop = Sim.FindShop(Homestead::ShopKind::GeneralStore))
        {
            const double Radians = FMath::DegreesToRadians(Shop->counterYaw);
            StandX = Shop->counterX + FMath::Cos(Radians) * TestGoto::CounterStandCm;
            StandY = Shop->counterY + FMath::Sin(Radians) * TestGoto::CounterStandCm;
            Facing = FRotator(0.0, FMath::UnwindDegrees(Shop->counterYaw + 180.0), 0.0);
            Described = FString::Printf(TEXT("General Store counter at (%.0f, %.0f)"), Shop->counterX, Shop->counterY);
            return true;
        }
    }

    // A landmark by alias, exact name, then partial name.
    const Homestead::EstateLayout& Layout = Sim.Layout();
    for (const TestGoto::FAlias& Alias : TestGoto::Aliases)
        if (Wanted.Equals(Alias.Word, ESearchCase::IgnoreCase))
            if (const Homestead::Landmark* Landmark = Layout.FindLandmark(Alias.Anchor)) return AtLandmark(*Landmark);
    for (const Homestead::Landmark& Landmark : Layout.landmarks)
        if (SameName(ToText(Landmark.name), Wanted)) return AtLandmark(Landmark);
    if (Wanted.Len() >= 4)
        for (const Homestead::Landmark& Landmark : Layout.landmarks)
            if (Compact(ToText(Landmark.name)).Contains(Wanted, ESearchCase::IgnoreCase)) return AtLandmark(Landmark);

    // A placement by id: "plot:3", "structure:2", "drop:7"; a bare number is a resource, then the others.
    FString Prefix;
    FString Number = Wanted;
    int32 Colon = INDEX_NONE;
    if (Target.FindChar(TEXT(':'), Colon))
    {
        Prefix = Compact(Target.Left(Colon));
        Number = Compact(Target.Mid(Colon + 1));
    }
    if (!Number.IsEmpty() && Number.IsNumeric())
    {
        const int32 Id = FCString::Atoi(*Number);
        const bool bAny = Prefix.IsEmpty();
        if (bAny || SameName(Prefix, TEXT("resource")) || SameName(Prefix, TEXT("node")))
        {
            if (const Homestead::Mine::Node* MineNode = Homestead::Mine::FindNode(Id)) return AtMineNode(*MineNode);
            for (const Homestead::ResourceNode& Node : State().resources)
                if (Node.id == Id)
                    return FaceThing(Node.position.x, Node.position.y, FString::Printf(TEXT("resource %d (%s%s)"), Id,
                        *ToText(Homestead::ResourceName(Node.kind)), Node.cleared ? TEXT(", cleared") : TEXT("")));
        }
        if (bAny || SameName(Prefix, TEXT("structure")) || SameName(Prefix, TEXT("piece")))
            for (const Homestead::Structure& Structure : State().structures)
                if (Structure.id == Id)
                {
                    const Homestead::Point Centre = Homestead::StructureCenter(State(), Structure);
                    return FaceThing(Centre.x, Centre.y, FString::Printf(TEXT("structure %d (%s)"), Id, *ToText(Homestead::PieceName(Structure.kind))));
                }
        if (bAny || SameName(Prefix, TEXT("plot")))
            for (const Homestead::Plot& Plot : State().plots)
                if (Plot.id == Id)
                {
                    const Homestead::Point Centre = Homestead::PlotCenter(Plot);
                    return FaceThing(Centre.x, Centre.y, FString::Printf(TEXT("plot %d"), Id));
                }
        if (bAny || SameName(Prefix, TEXT("drop")))
            for (const Homestead::WorldDrop& Drop : State().worldDrops)
                if (Drop.id == Id)
                    return FaceThing(Drop.position.x, Drop.position.y, FString::Printf(TEXT("drop %d (%s x%d)"), Id,
                        *ToText(Homestead::ItemName(Drop.item)), Drop.quantity));
        return false;
    }

    // The nearest of a kind: a resource kind or structure name (partial is fine), "plot" or "drop".
    double BestDistance = TNumericLimits<double>::Max();
    double BestX = 0.0;
    double BestY = 0.0;
    FString BestLabel;
    const Homestead::Mine::Node* BestMineNode = nullptr;
    // Underground nodes count their height too, and (when she is below) surface things count as far away: a
    // 2D distance would call a tunnel face "2 m away" from the yard above it.
    const auto Consider = [&](double X, double Y, const FString& Label, const Homestead::Mine::Node* MineNode = nullptr)
    {
        double Distance = TestGoto::Distance(Here, X, Y);
        if (MineNode) Distance = FMath::Sqrt(FMath::Square(Distance) + FMath::Square(MineNode->z - HereZ));
        else if (bHereUnderground) Distance += 1000000.0;
        if (Distance < BestDistance) { BestDistance = Distance; BestX = X; BestY = Y; BestLabel = Label; BestMineNode = MineNode; }
    };
    if (Wanted.Equals(TEXT("plot"), ESearchCase::IgnoreCase))
        for (const Homestead::Plot& Plot : State().plots)
        {
            const Homestead::Point Centre = Homestead::PlotCenter(Plot);
            Consider(Centre.x, Centre.y, FString::Printf(TEXT("plot %d"), Plot.id));
        }
    if (Wanted.Equals(TEXT("drop"), ESearchCase::IgnoreCase))
        for (const Homestead::WorldDrop& Drop : State().worldDrops)
            Consider(Drop.position.x, Drop.position.y, FString::Printf(TEXT("drop %d (%s x%d)"), Drop.id, *ToText(Homestead::ItemName(Drop.item)), Drop.quantity));
    for (const Homestead::Structure& Structure : State().structures)
    {
        const FString Name = Compact(ToText(Homestead::PieceName(Structure.kind)));
        if (Name.Equals(Wanted, ESearchCase::IgnoreCase) || (Wanted.Len() >= 3 && Name.Contains(Wanted, ESearchCase::IgnoreCase)))
        {
            const Homestead::Point Centre = Homestead::StructureCenter(State(), Structure);
            Consider(Centre.x, Centre.y, FString::Printf(TEXT("structure %d (%s)"), Structure.id, *ToText(Homestead::PieceName(Structure.kind))));
        }
    }
    for (const Homestead::ResourceNode& Node : State().resources)
    {
        if (Node.cleared) continue;
        const FString Name = Compact(ToText(Homestead::ResourceName(Node.kind)));
        if (Name.Equals(Wanted, ESearchCase::IgnoreCase) || (Wanted.Len() >= 3 && Name.Contains(Wanted, ESearchCase::IgnoreCase)))
            Consider(Node.position.x, Node.position.y, FString::Printf(TEXT("resource %d (%s)"), Node.id, *ToText(Homestead::ResourceName(Node.kind))),
                Homestead::Mine::FindNode(Node.id));
    }
    if (BestLabel.IsEmpty()) return false;
    if (BestMineNode)
    {
        AtMineNode(*BestMineNode);
        Described += FString::Printf(TEXT(", nearest of \"%s\", %.0f cm away"), *Target, BestDistance);
        return true;
    }
    return FaceThing(BestX, BestY, BestLabel + FString::Printf(TEXT(", nearest of \"%s\", %.0f cm away"), *Target, BestDistance));
}

void AHomesteadController::HomesteadGoto(const FString& Target)
{
    if (RejectTestWhileSnapping(TEXT("GOTO"))) return;
    GetWorldTimerManager().ClearTimer(TestUseTimer);
    GetWorldTimerManager().ClearTimer(TestOutcomeTimer);
    double StandX = 0.0;
    double StandY = 0.0;
    double StandZ = TestGoto::NoZ;
    FRotator Facing = FRotator::ZeroRotator;
    FString Described;
    if (!GetPawn() || !ResolveTestTarget(Target, StandX, StandY, StandZ, Facing, Described))
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("GOTO: no target matches \"%s\". Try a landmark (store counter mine cove square gate mill farm home), mine-ladder / underground, an id (plot:3 structure:2 drop:7 node:588011 or a resource id), a kind (chest plot drop or a resource name) or \"x,y\"."), *Target);
        Notify(FString::Printf(TEXT("GOTO: nothing called %s."), *Target), true);
        return;
    }
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("GOTO: %s; standing at (%.0f, %.0f) facing yaw %.0f."), *Described, StandX, StandY, Facing.Yaw);
    BeginTestTeleport(StandX, StandY, Facing, TEXT("GOTO"), [this, Described]()
    {
        UpdateFocus();
        const APawn* Avatar = GetPawn();
        UE_LOG(LogHomesteadPlaytest, Display, TEXT("GOTO: arrived at (%.0f, %.0f, z %.0f%s) for %s. Focus: \"%s\" [%s]."), PlayerPoint().x, PlayerPoint().y,
            Avatar ? Avatar->GetActorLocation().Z : 0.0, IsUnderground() ? TEXT(", underground") : TEXT(""), *Described, *FocusTitle(), *FocusActions());
    }, nullptr, StandZ);
}

void AHomesteadController::HomesteadUse(const FString& TargetAndItem)
{
    if (RejectTestWhileSnapping(TEXT("USE"))) return;
    GetWorldTimerManager().ClearTimer(TestUseTimer);
    GetWorldTimerManager().ClearTimer(TestOutcomeTimer);
    using namespace HomesteadPlaytest;
    // "<target> [item words]": the longest leading run of words that names a target wins, so "plot:3 Seeds"
    // and "oak Hatchet" both work. A leading quoted target may hold spaces.
    TArray<FString> Words;
    TargetAndItem.ParseIntoArrayWS(Words);
    if (Words.IsEmpty())
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("USE: format is \"<target> [item]\", e.g. \"plot:3 Seeds\" or \"chest\"."));
        return;
    }
    double StandX = 0.0;
    double StandY = 0.0;
    double StandZ = TestGoto::NoZ;
    FRotator Facing = FRotator::ZeroRotator;
    FString Described;
    FString ItemName;
    int32 TargetWords = 0;
    for (int32 Count = Words.Num(); Count >= 1 && TargetWords == 0; --Count)
    {
        const FString Candidate = FString::Join(TArrayView<const FString>(Words).Left(Count), TEXT(" "));
        if (ResolveTestTarget(Candidate, StandX, StandY, StandZ, Facing, Described))
        {
            TargetWords = Count;
            ItemName = FString::Join(TArrayView<const FString>(Words).RightChop(Count), TEXT(" "));
        }
    }
    if (TargetWords == 0 || !GetPawn())
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("USE: no target matches the start of \"%s\"."), *TargetAndItem);
        Notify(TEXT("USE: no such target."), true);
        return;
    }
    Homestead::Item Item = Homestead::Item::Count;
    const bool bHasItem = !ItemName.IsEmpty();
    if (bHasItem && !FindItem(ItemName, Item))
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("USE: no item called \"%s\"."), *ItemName);
        Notify(FString::Printf(TEXT("USE: no item called %s."), *ItemName), true);
        return;
    }
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("USE: %s; %s."), *Described,
        bHasItem ? *FString::Printf(TEXT("will use %s"), *ToText(Homestead::ItemName(Item))) : TEXT("will press Interact (E)"));

    BeginTestTeleport(StandX, StandY, Facing, TEXT("USE"), [this, bHasItem, Item]()
    {
        GetWorldTimerManager().SetTimer(TestUseTimer, FTimerDelegate::CreateWeakLambda(this, [this, bHasItem, Item]()
        {
            UpdateFocus();
            UE_LOG(LogHomesteadPlaytest, Display, TEXT("USE: facing \"%s\" [%s]."), *FocusTitle(), *FocusActions());
            if (bHasItem)
            {
                if (!ChooseOnHotbar(Item))
                {
                    UE_LOG(LogHomesteadPlaytest, Error, TEXT("USE: could not put %s on the hotbar (do you carry it?)."), *ToText(Homestead::ItemName(Item)));
                    return;
                }
                UseSelectedTool();
            }
            else
            {
                Interact();
            }
            GetWorldTimerManager().SetTimer(TestOutcomeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                UE_LOG(LogHomesteadPlaytest, Display, TEXT("USE: outcome. Toast: \"%s\". Pack: %s. Energy %.0f. Focus now: \"%s\"."), *Toast(),
                    *MenuInventorySummary(), State().energy, *FocusTitle());
            }), TestGoto::OutcomeSeconds, false);
        }), TestGoto::SettleSeconds, false);
    }, nullptr, StandZ);
}
