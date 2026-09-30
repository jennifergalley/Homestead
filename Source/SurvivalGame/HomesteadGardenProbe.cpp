#include "HomesteadGardenProbe.h"

#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadGardenTarget.h"
#include "Simulation/HomesteadPackRow.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#include <algorithm>

namespace GardenProbe
{
FString Plots(const Homestead::State& State)
{
    FString Text;
    for (const auto& Plot : State.plots) Text += FString::Printf(TEXT("%d:%d:%d:%d "), Plot.id, Plot.cellX, Plot.cellY, Plot.planted ? 1 : 0);
    return Text;
}
FString Stock(const Homestead::State& State)
{
    FString Text;
    for (const int32 Count : State.inventory) Text += FString::Printf(TEXT("%d "), Count);
    return Text;
}
}

AHomesteadGardenProbe::AHomesteadGardenProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AHomesteadGardenProbe::Stand(Homestead::Point Position, float Yaw)
{
    if (!Controller->PrepareWorldAt(Position)) { Finish(TEXT("The probe's standing place could not be prepared.")); return; }
    auto* Avatar = Cast<ACharacter>(Controller->GetPawn());
    if (!Avatar) { Finish(TEXT("No heroine to move.")); return; }
    Avatar->GetCharacterMovement()->StopMovementImmediately();
    Avatar->SetActorLocation(FVector(Position.x, Position.y, Controller->GroundHeight(Position.x, Position.y) + 100),
        false, nullptr, ETeleportType::TeleportPhysics);
    Avatar->SetActorRotation(FRotator(0, Yaw, 0));
    Controller->SetControlRotation(FRotator(-35, Yaw, 0));
}

FString AHomesteadGardenProbe::Record(const TCHAR* Label, bool bExpectValid)
{
    const auto& Outline = Controller->Landscape->GardenOutline;
    bool Thin = Outline.Components.Num() == 16;
    for (const auto& Component : Outline.Components)
    {
        const auto* Part = Cast<UPrimitiveComponent>(Component.Get());
        Thin = Thin && Part && Part->IsVisible() && !Part->CastShadow
            && Part->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
    }
    const bool Valid = Outline.Signature.EndsWith(TEXT(":1"));
    const FString Reason = Controller->GardenOutlineReason;
    const FString Focus = Controller->FocusActions();
    const bool ReasonShown = Valid ? Reason.IsEmpty()
        : !Reason.IsEmpty() && (Controller->Focus != AHomesteadController::EFocus::None || Focus.Contains(Reason));
    const bool Passed = !Outline.Signature.IsEmpty() && Thin && Valid == bExpectValid && ReasonShown;
    if (!Passed) ++Failures;
    const FString Line = FString::Printf(TEXT("%s %s outline=%s parts=%d thin=%d reason='%s' focus_kind=%d focus='%s' actor=%s selected=%d"),
        Passed ? TEXT("PASS") : TEXT("FAIL"), Label, *Outline.Signature, Outline.Components.Num(), Thin ? 1 : 0, *Reason,
        static_cast<int32>(Controller->Focus), *Focus, *Controller->GetPawn()->GetActorLocation().ToString(),
        Controller->SelectedHotbarIndex());
    Lines.Add(Line);
    return Line;
}

void AHomesteadGardenProbe::Capture(const FString& Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Output, Name + TEXT(".png")), true, false);
}

void AHomesteadGardenProbe::Finish(const FString& Error)
{
    if (Step < 0) return;
    Step = -1;
    if (!Error.IsEmpty()) Lines.Add(TEXT("ERROR ") + Error);
    Lines.Add(FString::Printf(TEXT("STATUS %s failures=%d"), Error.IsEmpty() && !Failures ? TEXT("passed") : TEXT("failed"), Failures));
    FFileHelper::SaveStringArrayToFile(Lines, *FPaths::Combine(Output, TEXT("garden-probe.txt")));
    FPlatformMisc::RequestExitWithStatus(false, Error.IsEmpty() && !Failures ? 0 : 1);
}

void AHomesteadGardenProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Step < 0) return;
    const double Now = FPlatformTime::Seconds();
    if (Output.IsEmpty())
    {
        FParse::Value(FCommandLine::Get(), TEXT("HomesteadGardenProbe="), Output);
        Output = FPaths::ConvertRelativePathToFull(Output);
        IFileManager::Get().MakeDirectory(*Output, true);
        Controller = Cast<AHomesteadController>(UGameplayStatics::GetPlayerController(this, 0));
        // The probe drains the pail in memory; never let an autosave write that back, even to the copy.
        if (Controller) Controller->bAutosaveEnabled = false;
        Next = Now + 15;
        Deadline = Now + 220;
        return;
    }
    if (Now > Deadline) { Finish(TEXT("Timed out.")); return; }
    if (Now < Next || !Controller || !Controller->GetPawn() || !Controller->Landscape || FScreenshotRequest::IsScreenshotRequested()) return;
    Next = Now + 2.5;
    using Homestead::Item;
    const auto& Sim = Controller->Simulation();
    const auto& State = Sim.GetState();
    switch (Step++)
    {
    case 0:
    {
        if (Controller->IsBookOpen()) Controller->CloseBook();
        const FVector Start = Controller->GetPawn()->GetActorLocation();
        Lines.Add(FString::Printf(TEXT("LOADED actor=%s plots=%d hoe=%d pail=%d water=%d energy=%.1f"), *Start.ToString(),
            static_cast<int32>(State.plots.size()), Sim.Count(Item::DiggingStick), Sim.Count(Item::WateringCan),
            Sim.Count(Item::Water), State.energy));
        // Her own garden: a thirsty plot for the pail, and nearby squares the hoe would and wouldn't till.
        const Homestead::Plot* Thirsty = nullptr;
        for (const auto& Plot : State.plots)
            if (Plot.moisture < 0.9 && (!Thirsty || Plot.moisture < Thirsty->moisture)) Thirsty = &Plot;
        if (!Thirsty) { Finish(TEXT("The loaded save has no thirsty plot.")); return; }
        const auto Centre = Homestead::GardenCellCenter(Thirsty->cellX, Thirsty->cellY);
        PailPlot = {Centre.x - 40, Centre.y};
        bool Good = false, Bad = false;
        for (int32 Ring = 1; Ring <= 30 && !(Good && Bad && Refusals.Num() >= 24); ++Ring)
            for (int32 Y = Thirsty->cellY - Ring; Y <= Thirsty->cellY + Ring; ++Y)
                for (int32 X = Thirsty->cellX - Ring; X <= Thirsty->cellX + Ring; ++X)
                {
                    if (FMath::Max(FMath::Abs(X - Thirsty->cellX), FMath::Abs(Y - Thirsty->cellY)) != Ring) continue;
                    if (std::any_of(State.plots.begin(), State.plots.end(),
                            [X, Y](const auto& Plot) { return Plot.cellX == X && Plot.cellY == Y; })) continue;
                    const auto Square = Homestead::GardenCellCenter(X, Y);
                    const Homestead::Point Position{Square.x - Homestead::GardenCellSize, Square.y};
                    int AheadX = 0, AheadY = 0;
                    Homestead::HoeCellAhead(Position, 1.0, 0.0, AheadX, AheadY);
                    if (AheadX != X || AheadY != Y) continue;
                    const auto Check = Sim.CheckTill(X, Y, Position);
                    const FString Message = UTF8_TO_TCHAR(Check.message.c_str());
                    if (Check.ok && !Good) { Good = true; HoeGood = Position; HoeGoodX = X; HoeGoodY = Y; }
                    else if (!Check.ok && !Bad && !Message.StartsWith(TEXT("Move closer")) && !Message.Contains(TEXT("tired")))
                    { Bad = true; HoeBad = Position; HoeBadX = X; HoeBadY = Y; }
                    // Further out, refused squares with nothing else to focus show the refusal in the focus line.
                    if (!Check.ok && Ring >= 3 && Refusals.Num() < 24 && !Message.StartsWith(TEXT("Move closer"))
                        && !Message.Contains(TEXT("tired")))
                        Refusals.Add(Position);
                }
        if (!Good || !Bad) { Finish(TEXT("No nearby hoe squares for both colours.")); return; }
        Lines.Add(FString::Printf(TEXT("TARGETS pail_plot=%d:%d moisture=%.2f hoe_good=%d:%d hoe_bad=%d:%d"),
            Thirsty->cellX, Thirsty->cellY, Thirsty->moisture, HoeGoodX, HoeGoodY, HoeBadX, HoeBadY));
        if (!Controller->ChooseOnHotbar(Item::DiggingStick)) { Finish(TEXT("Could not choose the hoe.")); return; }
        PlotsBefore = GardenProbe::Plots(State);
        StockBefore = GardenProbe::Stock(State);
        Stand(HoeGood, 0);
        break;
    }
    case 1:
        Record(TEXT("hoe-valid"), true);
        Capture(TEXT("copied-hoe-valid"));
        break;
    case 2: Stand(HoeBad, 0); break;
    case 3:
        Record(TEXT("hoe-invalid"), false);
        Capture(TEXT("copied-hoe-invalid"));
        break;
    case 4:
        if (!Controller->ChooseOnHotbar(Item::WateringCan)) { Finish(TEXT("Could not choose the pail.")); return; }
        Stand(PailPlot, 0);
        break;
    case 5:
        Record(TEXT("pail-valid"), Sim.Count(Item::Water) > 0);
        Capture(TEXT("copied-pail-valid"));
        break;
    case 6:
    {
        const bool Unchanged = GardenProbe::Plots(State) == PlotsBefore && GardenProbe::Stock(State) == StockBefore;
        if (!Unchanged) ++Failures;
        Lines.Add(FString::Printf(TEXT("%s previews left plots and stock unchanged"), Unchanged ? TEXT("PASS") : TEXT("FAIL")));
        // CONTROLLED: empty the pail (in this copy) so the pail's lasting refusal shows.
        Homestead::Simulation Drained = Sim;
        auto& Editable = const_cast<Homestead::State&>(Drained.GetState());
        Editable.inventory[static_cast<int32>(Item::Water)] = 0;
        Editable.inventoryLayout.erase(std::remove_if(Editable.inventoryLayout.begin(), Editable.inventoryLayout.end(),
            [](const auto& Entry) { return Entry.item == Item::Water && Entry.wearableId == 0; }), Editable.inventoryLayout.end());
        Homestead::PackRowRules::Prune(Editable.packRow, Editable.inventoryLayout);
        const auto Result = Controller->Sim.Deserialize(Drained.Serialize());
        if (!Result) { Finish(FString(TEXT("Could not empty the pail: ")) + UTF8_TO_TCHAR(Result.message.c_str())); return; }
        break;
    }
    case 7:
        Record(TEXT("pail-invalid"), false);
        Capture(TEXT("copied-pail-invalid"));
        break;
    case 8:
        if (!Controller->ChooseOnHotbar(Item::DiggingStick)) { Finish(TEXT("Could not choose the hoe again.")); return; }
        if (!Refusals.IsValidIndex(Refusal)) { Finish(TEXT("No refused square with an empty focus line.")); return; }
        Stand(Refusals[Refusal], 0);
        break;
    case 9:
        // Try the next refused square until one has nothing else in focus.
        if (Controller->Focus != AHomesteadController::EFocus::None && Refusals.IsValidIndex(Refusal + 1))
        {
            ++Refusal;
            Stand(Refusals[Refusal], 0);
            Step = 9;
            break;
        }
        Record(TEXT("hoe-refusal-focus-line"), false);
        if (Controller->Focus != AHomesteadController::EFocus::None
            || Controller->FocusActions() != Controller->GardenOutlineReason)
        {
            ++Failures;
            Lines.Add(TEXT("FAIL the hoe's refusal is not the focus line"));
        }
        else Lines.Add(TEXT("PASS the hoe's refusal replaces Till ground in the focus line"));
        Capture(TEXT("copied-hoe-refusal-focus"));
        break;
    default:
        Finish(FString());
        break;
    }
}
