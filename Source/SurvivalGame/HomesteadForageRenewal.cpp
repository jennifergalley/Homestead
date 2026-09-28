#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadSave.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Serialization/JsonSerializer.h"
#include "UI/SHomesteadMenu.h"
#include "UnrealClient.h"

namespace
{
const Homestead::ResourceNode* RenewalNode(const Homestead::State& State, int32 Id)
{
    for (const auto& Node : State.resources) if (Node.id == Id) return &Node;
    return nullptr;
}
bool RenewalObject(const FString& Path, TSharedPtr<FJsonObject>& Object)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text, *Path)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) && Object.IsValid();
}
bool RenewalAtomic(const FString& Path, const FString& Text)
{
    return FFileHelper::SaveStringToFile(Text, *(Path + TEXT(".tmp")))
        && IFileManager::Get().Move(*Path, *(Path + TEXT(".tmp")), true, true, false, true);
}
}

void AHomesteadVisualPlaytest::RenewalEvent(const FString& Message)
{
    Renewal.Events.Add(FString::Printf(TEXT("%.3f hour=%.8f %s"),
        FPlatformTime::Seconds() - Renewal.Started, PC->State().hour, *Message));
    UE_LOG(LogTemp, Display, TEXT("RENEWAL %s"), *Renewal.Events.Last());
}

bool AHomesteadVisualPlaytest::RenewalCheck(bool Condition, const FString& Message)
{
    ++Renewal.Checks;
    if (!Condition) FinishRenewal(TEXT("failed"), Message);
    return Condition;
}

void AHomesteadVisualPlaytest::PrepareRenewal()
{
    auto& R = Renewal;
    R.Started = R.LastTick = FPlatformTime::Seconds();
    R.ReadOnly = FParse::Param(FCommandLine::Get(), TEXT("HomesteadRenewalReadOnly"));
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadRenewalControl="), R.ControlPath);
    const auto* Config = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    if (!RenewalCheck(Config && FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(Config->IniPath),
        FPaths::ConvertRelativePathToFull(FPaths::Combine(OutputDirectory, TEXT("Graphics"), TEXT("GameUserSettings.ini")))),
        TEXT("Graphics configuration must be the explicit synthetic destination."))) return;
    for (const TCHAR* Flag : {TEXT("HomesteadSmokeTest"), TEXT("HomesteadEndurance"), TEXT("HomesteadPresentationDiagnostics"),
        TEXT("HomesteadWateringPlaytest"), TEXT("HomesteadWeedingPlaytest"), TEXT("HomesteadClearingPlaytest")})
        if (!RenewalCheck(!FParse::Param(FCommandLine::Get(), Flag), TEXT("Overlapping automation modes rejected."))) return;
    TSharedPtr<FJsonObject> Control;
    FString State, Deadline, Policy;
    if (!RenewalCheck(RenewalObject(R.ControlPath, Control) && Control->TryGetStringField(TEXT("id"), R.RunId)
        && Control->TryGetStringField(TEXT("state"), State) && State == TEXT("running")
        && Control->TryGetStringField(TEXT("completionPolicy"), Policy)
        && (Policy == TEXT("bounded") || Policy == TEXT("until-complete"))
        && Control->TryGetStringField(TEXT("deadlineUtc"), Deadline) && FDateTime::ParseIso8601(*Deadline, R.Deadline)
        && (Policy == TEXT("until-complete") || (R.Deadline - FDateTime::UtcNow()).GetTotalSeconds() > 960),
        TEXT("Run control cannot admit renewal route."))) return;
    R.CompletionDriven = Policy == TEXT("until-complete");
    const auto* Save = PC->ReadSave(PC->SavePath(TEXT("Homestead_Manual")));
    if (!RenewalCheck(Save != nullptr, TEXT("Copied test-world save missing/invalid."))) return;
    R.WorldId = Save->WorldId; R.SavedState = Save->SimulationData;
    using Kind = FHomesteadRenewalState::Kind;
    auto Add = [&R](Kind K, int32 Id = -1, FVector2D Target = {}, FString Label = {})
    { R.Steps.Add({K, Id, Target, Label}); };
    Add(Kind::Load); Add(Kind::VerifyLoad);
    if (R.ReadOnly) { Add(Kind::Finish); return; }
    Add(Kind::Close);
    Add(Kind::Walk, -1, {-700, 100});
    const auto Approach = [&Add](int32 Id)
    {
        const FVector2D Target = Id == 8 ? FVector2D(-600, -455) : Id == 12 ? FVector2D(-500, -250) : FVector2D(-400, -455);
        Add(Kind::Walk, Id, Target); Add(Kind::Face, Id);
    };
    for (const int32 Id : {8, 12, 10}) { Approach(Id); Add(Kind::Reject, Id, {}, TEXT("early")); }
    Add(Kind::Open); Add(Kind::Save); Add(Kind::Reload); Add(Kind::VerifySave); Add(Kind::Close);
    for (const FVector2D P : {FVector2D(-700, 100), {-1330, 100}, {-1330, -450}, {-1180, -450}, {-1060, -450}})
        Add(Kind::Walk, -1, P);
    for (int32 I = 0; I < 3; ++I) { Add(Kind::Food); Add(Kind::Rest); }
    for (const FVector2D P : {FVector2D(-1180, -450), {-1330, -450}, {-1330, 100}, {-700, 100}})
        Add(Kind::Walk, -1, P);
    for (const int32 Id : {8, 12, 10})
    {
        Approach(Id); Add(Kind::Ready, Id); Add(Kind::Gather, Id); Add(Kind::Recover);
        Add(Kind::Reject, Id, {}, TEXT("depleted-again"));
    }
    Add(Kind::Walk, 14, {-700, -600}); Add(Kind::Face, 14); Add(Kind::Cleared, 14);
    Add(Kind::Open); Add(Kind::Save); Add(Kind::VerifySave); Add(Kind::Finish);
    RecordPresentationSettings(TEXT("renewal-start"));
}

bool AHomesteadVisualPlaytest::RenewalVisuals(int32 Id, bool Ready, bool Focused, const FString& Label, bool Screenshot)
{
    const auto* Node = RenewalNode(PC->State(), Id);
    if (!RenewalCheck(Node && PC->Landscape, TEXT("Actual resource/world missing."))) return false;
    const bool Cleared = Id == 14;
    if (!RenewalCheck(Node->cleared == Cleared && PC->Simulation().CanHarvest(Id) == (Ready && !Cleared),
        TEXT("Resource readiness/cleared state disagrees."))) return false;
    const auto* Base = PC->Landscape->ResourceVisuals.Find(Id);
    const auto* Produce = PC->Landscape->ResourceProduceVisuals.Find(Id);
    const int32 BaseCount = Id == 8 || Cleared ? 0 : Id == 12 ? 2 : 3;
    const int32 ProduceCount = !Ready || Cleared ? 0 : Id == 8 ? 3 : Id == 12 ? 2 : 8;
    if (!RenewalCheck(Base && Produce && Base->Components.Num() == BaseCount && Produce->Components.Num() == ProduceCount,
        FString::Printf(TEXT("Actual resource%d component count did not refresh."), Id))) return false;
    for (const auto* Visual : {Base, Produce})
        for (const auto& Component : Visual->Components)
        {
            const auto* Mesh = Cast<UStaticMeshComponent>(Component);
            if (!RenewalCheck(Mesh && Mesh->IsRegistered() && Mesh->IsVisible() && !Mesh->bHiddenInGame
                && Mesh->GetStaticMesh() && Mesh->IsRenderStateCreated(), TEXT("Resource mesh is not registered/visible/render-state-ready."))) return false;
            const FString Path = Mesh->GetStaticMesh()->GetPathName();
            const bool BerryProduce = Id == 10 && Visual == Produce;
            const bool FlowerBase = Id == 12 && Visual == Base;
            const FString Expected = FlowerBase ? TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tiny_a.")
                : Id == 8 ? TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_DryBranchesMedium01_")
                : Id == 12 ? TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_")
                : TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_");
            if (!RenewalCheck((BerryProduce ? Path == TEXT("/Engine/BasicShapes/Sphere.Sphere")
                    : Mesh->ComponentHasTag(TEXT("AuthoredResource")) && Path.StartsWith(Expected))
                && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision,
                TEXT("Expected authored resource role or separately removable berry produce differs."))) return false;
            for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
            {
                const auto* Interface = Mesh->GetMaterial(Slot);
                auto* Material = Interface ? Interface->GetMaterial() : nullptr;
                auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
                if (!RenewalCheck(Material && Material->GetPathName().StartsWith(TEXT("/Game/"))
                    && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete(),
                    TEXT("Resource material is missing, default or not shader-ready."))) return false;
            }
        }
    if (Cleared)
    {
        FCollisionQueryParams Query(SCENE_QUERY_STAT(RenewalCleared), false, PC->GetPawn());
        const FVector Center(Node->position.x, Node->position.y,
            PC->GroundHeight(Node->position.x, Node->position.y) + 90);
        if (!RenewalCheck(!GetWorld()->OverlapBlockingTestByChannel(Center, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeSphere(20), Query), TEXT("Cleared sapling location regained a blocking collider."))) return false;
        if (!RenewalCheck(!PC->IsResourceFocused(Id), TEXT("Cleared sapling regained focus."))) return false;
    }
    else if (Focused && !RenewalCheck(PC->IsResourceFocused(Id)
        && !PC->FocusTitle().Contains(TEXT("(renewing)"))
        && PC->FocusActions().Contains(TEXT("[A] Gather")) == Ready,
        TEXT("Actual focused HUD prompt and availability did not refresh."))) return false;
    auto Row = MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("phase"), Label); Row->SetNumberField(TEXT("node"), Id);
    Row->SetNumberField(TEXT("hour"), PC->State().hour); Row->SetNumberField(TEXT("deadline"), Node->readyAtHour);
    Row->SetBoolField(TEXT("ready"), Ready); Row->SetBoolField(TEXT("cleared"), Cleared);
    Row->SetNumberField(TEXT("baseComponents"), BaseCount); Row->SetNumberField(TEXT("produceComponents"), ProduceCount);
    Row->SetStringField(TEXT("focusTitle"), PC->FocusTitle()); Row->SetStringField(TEXT("focusActions"), PC->FocusActions());
    Row->SetStringField(TEXT("toast"), PC->Toast()); Row->SetNumberField(TEXT("toastSeconds"), PC->ToastRemaining);
    Row->SetNumberField(TEXT("hunger"), PC->State().hunger);
    FVector2D Screen;
    PC->ProjectWorldLocationToScreen(FVector(Node->position.x, Node->position.y,
        PC->GroundHeight(Node->position.x, Node->position.y) + 35), Screen);
    Row->SetNumberField(TEXT("screenX"), Screen.X); Row->SetNumberField(TEXT("screenY"), Screen.Y);
    FString Text; FJsonSerializer::Serialize(Row, TJsonWriterFactory<>::Create(&Text));
    Text.ReplaceInline(TEXT("\r"), TEXT("")); Text.ReplaceInline(TEXT("\n"), TEXT(""));
    Renewal.Snapshots.Add(Text);
    if (Screenshot)
    {
        const FString File = FString::Printf(TEXT("%02d-%s-node%d.png"), CaptureIndex++, *Label, Id);
        FScreenshotRequest::RequestScreenshot(FPaths::Combine(OutputDirectory, TEXT("Frames"), File), false, false);
    }
    return true;
}

bool AHomesteadVisualPlaytest::WriteRenewal(const FString& Status, const FString& Reason)
{
    const auto& R = Renewal;
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("status"), Status); Object->SetStringField(TEXT("reason"), Reason);
    Object->SetStringField(TEXT("phase"), R.ReadOnly ? TEXT("reload") : TEXT("write"));
    Object->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Object->SetNumberField(TEXT("checks"), R.Checks); Object->SetNumberField(TEXT("step"), R.Index);
    Object->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - R.Started);
    Object->SetNumberField(TEXT("unpausedWallSeconds"), R.Unpaused); Object->SetNumberField(TEXT("pausedWallSeconds"), R.Paused);
    Object->SetNumberField(TEXT("engineUnpausedSeconds"), R.EngineUnpaused);
    Object->SetNumberField(TEXT("initialHour"), R.InitialHour); Object->SetNumberField(TEXT("hour"), PC->State().hour);
    Object->SetNumberField(TEXT("ordinarySleepHours"), R.SleepHours);
    Object->SetNumberField(TEXT("naturalGameHours"), R.Loaded ? PC->State().hour - R.InitialHour - R.SleepHours : 0);
    Object->SetNumberField(TEXT("sleeps"), R.Sleeps); Object->SetNumberField(TEXT("eats"), R.Eats);
    Object->SetNumberField(TEXT("rejectedGathers"), R.Rejects); Object->SetNumberField(TEXT("harvests"), R.Harvests);
    Object->SetNumberField(TEXT("walks"), R.Walks); Object->SetNumberField(TEXT("saves"), R.Saves);
    Object->SetNumberField(TEXT("exactRoundTrips"), R.RoundTrips); Object->SetNumberField(TEXT("frames"), CaptureIndex);
    Object->SetStringField(TEXT("worldId"), R.WorldId);
    const FString Prefix = R.ReadOnly ? TEXT("reload") : TEXT("write");
    FString Text; FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
    return RenewalAtomic(FPaths::Combine(OutputDirectory, Prefix + TEXT("-result.json")), Text)
        && RenewalAtomic(FPaths::Combine(OutputDirectory, Prefix + TEXT("-events.txt")), FString::Join(R.Events, TEXT("\n")))
        && RenewalAtomic(FPaths::Combine(OutputDirectory, Prefix + TEXT("-visuals.jsonl")), FString::Join(R.Snapshots, TEXT("\n")));
}

void AHomesteadVisualPlaytest::FinishRenewal(const FString& Status, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true; ApplyAxes({}, {});
    RenewalEvent(Status + TEXT(": ") + Reason);
    RecordPresentationSettings(TEXT("renewal-end"));
    const bool Saved = WriteRenewal(Status, Reason) && RenewalAtomic(FPaths::Combine(OutputDirectory,
        Renewal.ReadOnly ? TEXT("reload-settings.txt") : TEXT("write-settings.txt")), FString::Join(PresentationSettings, TEXT("\n")));
    if (!Saved) UE_LOG(LogTemp, Error, TEXT("RENEWAL evidence persistence failed"));
    FPlatformMisc::RequestExitWithStatus(false, Saved && Status != TEXT("failed") ? 0 : 1);
}

void AHomesteadVisualPlaytest::TickRenewal(float EngineDelta)
{
    auto& R = Renewal;
    using Kind = FHomesteadRenewalState::Kind;
    const double Now = FPlatformTime::Seconds(), Delta = Now - R.LastTick;
    R.LastTick = Now;
    if (Now >= R.NextControl)
    {
        R.NextControl = Now + 1;
        TSharedPtr<FJsonObject> Control; FString Id, State, Deadline, Policy; FDateTime Limit;
        if (!RenewalCheck(RenewalObject(R.ControlPath, Control) && Control->TryGetStringField(TEXT("id"), Id)
            && Control->TryGetStringField(TEXT("state"), State) && Control->TryGetStringField(TEXT("deadlineUtc"), Deadline)
            && Control->TryGetStringField(TEXT("completionPolicy"), Policy)
            && Policy == (R.CompletionDriven ? TEXT("until-complete") : TEXT("bounded"))
            && FDateTime::ParseIso8601(*Deadline, Limit), TEXT("Run control invalid/unreadable."))) return;
        if (Id != R.RunId || State != TEXT("running")
            || (!R.CompletionDriven && (FDateTime::UtcNow() >= Limit || FDateTime::UtcNow() >= R.Deadline))
            || IFileManager::Get().FileExists(*FPaths::Combine(OutputDirectory, TEXT("stop-renewal.txt"))))
        { FinishRenewal(TEXT("cancelled"), TEXT("Run state/deadline/stop marker; incomplete.")); return; }
    }
    if (PC->IsFailed() || Now - R.Started > 900)
    { FinishRenewal(TEXT("failed"), TEXT("Survival failure or bounded route timeout; no retry.")); return; }
    if (R.Loaded)
    {
        if (PC->IsBookOpen()) R.Paused += Delta;
        else { R.Unpaused += Delta; R.EngineUnpaused += EngineDelta; }
    }
    if (Now >= R.NextReport)
    {
        R.NextReport = Now + 5;
        if (!WriteRenewal(TEXT("running"), TEXT("Mapped targeted renewal route.")))
        { FinishRenewal(TEXT("failed"), TEXT("Could not persist progress.")); return; }
    }
    if (!R.Steps.IsValidIndex(R.Index))
    { FinishRenewal(TEXT("failed"), TEXT("Missing bounded route step.")); return; }
    const auto& Step = R.Steps[R.Index];
    const bool Entered = R.EnteredIndex != R.Index;
    if (Entered) { R.EnteredIndex = R.Index; R.StepStarted = Now; WaterSettle = 0; }
    const double Age = Now - R.StepStarted;
    const auto Next = [&R]() { ++R.Index; };
    const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
    const auto* Anim = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (!Anim) { FinishRenewal(TEXT("failed"), TEXT("Animation instance missing.")); return; }
    if (Step.Command == Kind::Walk)
    {
        const auto P = PC->PlayerPoint();
        const double Distance = FVector2D::Distance(Step.Target, FVector2D(P.x, P.y));
        if (Entered || Distance < R.ProgressDistance - 8) { R.ProgressAt = Now; R.ProgressDistance = Distance; }
        FVector2D Move{}, Look{};
        if (WalkWaterTarget(Step.Target, 16, Delta, Move, Look)) { ++R.Walks; Next(); }
        else if (Now - R.ProgressAt > 12 || Age > 45)
        { FinishRenewal(TEXT("failed"), FString::Printf(TEXT("Actual route blocked at target%s remaining%.1f; no teleport."), *Step.Target.ToString(), Distance)); return; }
        ApplyAxes(Move, Look); return;
    }
    ApplyAxes({}, {});
    if (Step.Command == Kind::Face)
    {
        const float Error = FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, 0);
        if (FMath::Abs(Error) < 2) { if (Age > 0.8) Next(); }
        // A proportional command stalls inside the existing mapped stick deadzones.
        else ApplyAxes({}, {FMath::Sign(Error) * 0.65f, 0});
        if (Age > 8) FinishRenewal(TEXT("failed"), FString::Printf(TEXT("Mapped camera turn hung at yaw%.3f error%.3f."), PC->GetControlRotation().Yaw, Error));
        return;
    }
    const auto* Node = RenewalNode(PC->State(), Step.Node);
    switch (Step.Command)
    {
    // Queue load and pause together, then verify after dispatch; no simulation frame may advance between them.
    case Kind::Load: Tap(EKeys::F9); Tap(EKeys::Gamepad_Special_Right); Next(); break;
    case Kind::VerifyLoad:
        if (Age < 0.8) break;
        if (!RenewalCheck(PC->IsBookOpen() && !PC->ToastIsError() && PC->Toast() == TEXT("Welcome back to your homestead.") && PC->WorldId == R.WorldId
            && FString(UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str())) == R.SavedState, TEXT("Initial/relaunch load changed saved world/state."))) return;
        R.Loaded = true; R.InitialHour = PC->State().hour;
        for (const int32 Id : {8, 12, 10})
        {
            const auto* N = RenewalNode(PC->State(), Id);
            if (!RenewalCheck(N && N->readyAtHour > PC->State().hour && !N->cleared, TEXT("Fixture target is not depleted."))) return;
            R.InitialDeadlines.Add(Id, N->readyAtHour);
            if (!RenewalVisuals(Id, false, false, R.ReadOnly ? TEXT("relaunch") : TEXT("initial"), false)) return;
        }
        if (!RenewalVisuals(14, false, false, TEXT("cleared-control"), false)) return;
        RenewalEvent(TEXT("Exact disclosed fixture loaded; deadlines and actual components verified.")); Next(); break;
    case Kind::Open:
        if (Entered && !PC->IsBookOpen()) Tap(EKeys::Gamepad_Special_Right);
        if (Age < 0.25) break;
        if (!RenewalCheck(PC->IsBookOpen() && PC->BookPage() == 0, TEXT("Mapped pack open did not dispatch."))) return;
        Next(); break;
    case Kind::Close:
        if (Entered) Tap(EKeys::Gamepad_FaceButton_Right);
        if (Age < 0.25) break;
        if (!RenewalCheck(!PC->IsBookOpen(), TEXT("Mapped pack close did not dispatch."))) return;
        Next(); break;
    case Kind::Ready:
        if (Entered && !RenewalVisuals(Step.Node, true, true, TEXT("ready"), true)) return;
        if (Age > 0.8) Next(); break;
    case Kind::Reject:
    case Kind::Gather:
        if (Entered)
        {
            if (!RenewalCheck(Node && PC->IsResourceFocused(Step.Node), TEXT("Mapped gather focus does not match target ID."))) return;
            R.BeforeInventory = PC->State().inventory; R.BeforeStarts = Anim->GatherStarts();
            R.BeforeDeadline = Node->readyAtHour; R.ActionHour = PC->State().hour;
            R.BeforeToastSeconds = PC->ToastRemaining; R.ActionEngine = R.EngineUnpaused;
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        }
        if (Age < 0.7) break;
        if (Step.Command == Kind::Reject)
        {
            if (!RenewalCheck(PC->ToastIsError() && PC->Toast().Contains(TEXT("Nothing to gather"))
                && PC->ToastRemaining > FMath::Max(0.0, R.BeforeToastSeconds - (R.EngineUnpaused - R.ActionEngine)) + 0.1
                && PC->State().inventory == R.BeforeInventory && Node->readyAtHour == R.BeforeDeadline
                && Anim->GatherStarts() == R.BeforeStarts && Anim->ActionWeight() < 0.001f,
                TEXT("Depleted gather granted reward, changed timer or started action."))) return;
            ++R.Rejects;
            if (!RenewalVisuals(Step.Node, false, true, Step.Label, true)) return;
            RenewalEvent(FString::Printf(TEXT("rejected node%d without reward/action; fresh response timer%.3f->%.3f"),
                Step.Node, R.BeforeToastSeconds, PC->ToastRemaining));
        }
        else
        {
            auto Expected = R.BeforeInventory;
            const auto Item = Step.Node == 8 ? Homestead::Item::Branch : Step.Node == 12 ? Homestead::Item::Flowers : Homestead::Item::Berries;
            Expected[static_cast<size_t>(Item)] += Step.Node == 12 ? 3 : 5;
            if (!RenewalCheck(!PC->ToastIsError() && PC->Toast().IsEmpty() && PC->State().inventory == Expected
                && FMath::Abs(Node->readyAtHour - R.ActionHour - (Step.Node == 10 ? 36 : 24)) < 1.e-6
                && Anim->GatherStarts() == R.BeforeStarts + 1 && Anim->GatherWeight() > 0.01f,
                TEXT("Ready gather did not grant exactly one correct reward/cooldown/action."))) return;
            ++R.Harvests; RenewalEvent(FString::Printf(TEXT("harvested node%d once; deadline%.8f"), Step.Node, Node->readyAtHour));
        }
        Next(); break;
    case Kind::Recover:
        if (Age < 2) break;
        if (!RenewalCheck(Anim->ActionWeight() < 0.001f, TEXT("Gather action stuck after recovery."))) return;
        Next(); break;
    case Kind::Save:
        if (Entered)
        {
            if (!RenewalCheck(PC->IsBookOpen(), TEXT("Manual save must start after mapped pause dispatch."))) return;
            R.SavedState = UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str()); Tap(EKeys::F5);
        }
        if (Age < 0.8) break;
        if (!RenewalCheck(!PC->ToastIsError(), TEXT("Mapped manual save failed."))) return;
        ++R.Saves; Next(); break;
    case Kind::Reload: Tap(EKeys::F9); Tap(EKeys::Gamepad_Special_Right); ++R.RoundTrips; Next(); break;
    case Kind::VerifySave:
    {
        if (Age < 0.8) break;
        const auto* Saved = PC->ReadSave(PC->SavePath(TEXT("Homestead_Manual")));
        if (!RenewalCheck(Saved && Saved->WorldId == R.WorldId && Saved->SimulationData == R.SavedState
            && PC->WorldId == R.WorldId && PC->IsBookOpen()
            && FString(UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str())) == R.SavedState,
            TEXT("Exact paused manual/save/load state or CRC integrity differs."))) return;
        if (R.Saves == 1 && !RenewalCheck(!PC->ToastIsError() && PC->Toast() == TEXT("Welcome back to your homestead."),
            TEXT("Mapped roundtrip did not produce a fresh load response."))) return;
        RenewalEvent(TEXT("Exact paused manual state/deadlines preserved.")); Next(); break;
    }
    case Kind::Food:
        if (Entered) { R.FoodOpen = false; R.FoodPending = false; R.FoodClosing = false; }
        if (Now - R.LastButton < 0.25) break;
        R.LastButton = Now;
        if (R.FoodClosing)
        {
            if (!RenewalCheck(!PC->IsBookOpen(), TEXT("Food-menu close did not dispatch."))) return;
            Next(); break;
        }
        if (R.FoodPending)
        {
            if (!RenewalCheck(!PC->ToastIsError() && PC->Simulation().Count(Homestead::Item::Berries) == R.FoodBefore - 1,
                TEXT("Mapped food action failed."))) return;
            ++R.Eats; R.FoodPending = false; RenewalEvent(TEXT("Ate actual carried berry."));
        }
        if ((!R.FoodOpen && PC->State().hunger >= 65) || (R.FoodOpen && PC->State().hunger >= 75))
        {
            if (R.FoodOpen) { Tap(EKeys::Gamepad_FaceButton_Right); R.FoodClosing = true; }
            else Next();
            break;
        }
        if (!R.FoodOpen) { Tap(EKeys::Gamepad_Special_Right); R.FoodOpen = true; break; }
        if (!RenewalCheck(PC->IsBookOpen() && PC->BookPage() == 0, TEXT("Food-menu open did not dispatch."))) return;
        if (PC->HasNativeMenu())
        {
            const auto* Subject = PC->NativeMenu->GetSelectedSubject();
            const FString Region = PC->NativeMenu->GetFocusedRegionName();
            const int32 FoodId = static_cast<int32>(Homestead::Item::Berries);
            if (!RenewalCheck(Subject && (Region == TEXT("Content") || Region == TEXT("Actions")),
                TEXT("Native food subject/focus unavailable."))) return;
            if (Subject->Subject == EHomesteadMenuSubject::ItemGroup && Subject->Id == FoodId)
            {
                if (Region == TEXT("Content")) Tap(EKeys::Gamepad_FaceButton_Bottom);
                else
                {
                    R.FoodBefore = PC->Simulation().Count(Homestead::Item::Berries);
                    Tap(EKeys::Gamepad_FaceButton_Bottom); R.FoodPending = true;
                }
            }
            else
            {
                const auto Rows = PC->MenuRows();
                const int32 Target = Rows.IndexOfByPredicate([FoodId](const FHomesteadRow& Row)
                    { return Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.Id == FoodId; });
                const int32 Current = PC->NativeMenu->GetSelectedContentIndex();
                const int32 Columns = PC->NativeMenu->GetContentColumnCount();
                if (!RenewalCheck(Region == TEXT("Content") && Target != INDEX_NONE && Rows.IsValidIndex(Current) && Columns > 0,
                    TEXT("Native food grid target unavailable."))) return;
                if (Target / Columns != Current / Columns)
                    Tap(Target > Current ? EKeys::Gamepad_DPad_Down : EKeys::Gamepad_DPad_Up);
                else Tap(Target > Current ? EKeys::Gamepad_DPad_Right : EKeys::Gamepad_DPad_Left);
            }
        }
        else if (PC->Rows().IsValidIndex(PC->SelectedRow()) && PC->Rows()[PC->SelectedRow()].Id == static_cast<int32>(Homestead::Item::Berries))
        {
            R.FoodBefore = PC->Simulation().Count(Homestead::Item::Berries);
            Tap(EKeys::Gamepad_FaceButton_Bottom); R.FoodPending = true;
        }
        else Tap(EKeys::Gamepad_DPad_Down);
        if (Age > 12) FinishRenewal(TEXT("failed"), TEXT("Food navigation failed; no refill/reset."));
        break;
    case Kind::Rest:
        if (Entered)
        {
            if (!RenewalCheck(!PC->IsBookOpen() && PC->Focus == AHomesteadController::EFocus::Bed && PC->FocusId == 105
                && (PC->FocusActions().Contains(TEXT("Sleep")) || PC->FocusActions().Contains(TEXT("Nap")))
                && PC->Simulation().IsSheltered(PC->PlayerPoint()),
                TEXT("Mapped route did not reach actual sheltered bed105."))) return;
            R.ActionHour = PC->State().hour; R.ActionEngine = R.EngineUnpaused;
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        }
        if (Age < 0.8) break;
        {
            const double Natural = (R.EngineUnpaused - R.ActionEngine) * 24.0 / (PC->State().dayMinutes * 60.0);
            const double Rest = PC->State().hour - R.ActionHour - Natural;
            if (!RenewalCheck(!PC->ToastIsError() && FMath::Abs(Rest - 8) < 1.e-6, TEXT("Normal mapped8h rest failed."))) return;
            R.SleepHours += Rest; ++R.Sleeps;
            RenewalEvent(FString::Printf(TEXT("normal bed rest%d advanced%.8fh plus%.8fnatural; hunger%.2f"),
                R.Sleeps, Rest, Natural, PC->State().hunger));
        }
        for (const int32 Id : {8, 12, 10})
        {
            const bool Ready = R.InitialDeadlines[Id] <= PC->State().hour;
            const auto* N = RenewalNode(PC->State(), Id);
            if (!RenewalCheck(N && N->readyAtHour == R.InitialDeadlines[Id] && Ready == (Id != 10 || R.Sleeps == 3),
                TEXT("Chosen24h/36h deadlines changed or crossed unexpectedly."))) return;
            if (!RenewalVisuals(Id, Ready, false, FString::Printf(TEXT("rest%d"), R.Sleeps), false)) return;
        }
        if (!RenewalVisuals(14, false, false, TEXT("after-rest-cleared"), false)) return;
        Next(); break;
    case Kind::Cleared:
        if (Entered && !RenewalVisuals(14, false, false, TEXT("cleared-final"), true)) return;
        if (Age > 0.8) Next(); break;
    case Kind::Finish:
        if (!RenewalCheck(R.ReadOnly || (R.Sleeps == 3 && FMath::Abs(R.SleepHours - 24) < 1.e-6 && R.Rejects == 6
            && R.Harvests == 3 && R.Saves == 2 && R.RoundTrips == 1), TEXT("Incomplete fixed renewal route."))) return;
        FinishRenewal(TEXT("passed"), R.ReadOnly ? TEXT("Separate process restored exact final state, cooldowns and actual depleted/cleared components.")
            : TEXT("Mapped24h/36h renewal, visuals, single rewards and saves passed.")); break;
    default: break;
    }
}
