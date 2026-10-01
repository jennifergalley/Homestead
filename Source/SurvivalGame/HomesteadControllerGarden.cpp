// The garden outline: with the hoe, the pail or seeds out, a thin border on the ground round the exact garden
// square the next stroke, pour or sowing would act on, green when it would work and red (with the reason in the
// focus line) when it wouldn't. Homestead::PreviewGarden picks the square with the same selector the
// actions use (the hoe's 85 cm, the pail's and the seed's focused plot) and checks it without changing anything.
#include "HomesteadController.h"

#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadGardenTarget.h"

void AHomesteadController::UpdateGardenOutline()
{
    Homestead::GardenTarget Target;
    const APawn* Avatar = GetPawn();
    if (Avatar && Landscape && ShouldShowHotbar() && !HasNativeMenu())
    {
        // The selected cell of her pack's first row (the Coral hotbar); wearables and empty cells show nothing.
        const auto Tool = HotbarItem(SelectedHotbarSlot);
        const FVector Forward = Avatar->GetActorForwardVector().GetSafeNormal2D();
        if (Tool == Homestead::Item::DiggingStick && Sim.Count(Tool) > 0)
            Target = Homestead::PreviewGarden(Sim, Homestead::GardenTool::Hoe, PlayerPoint(), Forward.X, Forward.Y,
                Focus == EFocus::Plot ? FocusId : -1);
        // The pail waters the focused plot; at the water it fills instead, so no square then.
        else if (Tool == Homestead::Item::WateringCan && Sim.Count(Tool) > 0 && Focus == EFocus::Plot)
            Target = Homestead::PreviewGarden(Sim, Homestead::GardenTool::Pail, PlayerPoint(), Forward.X, Forward.Y, FocusId);
        // A seed (or berry) sows the focused plot on [A]/[E]; with nothing in focus, the untilled square ahead
        // is outlined red so she knows to till it first.
        else if (Homestead::CropForSeed(Tool) && (Focus == EFocus::Plot || Focus == EFocus::None))
            Target = Homestead::PreviewGarden(Sim, Homestead::GardenTool::Seed, PlayerPoint(), Forward.X, Forward.Y,
                Focus == EFocus::Plot ? FocusId : -1, Tool);
    }
    GardenOutlineReason = Target.shown && !Target.valid ? FString(UTF8_TO_TCHAR(Target.reason.c_str())) : FString();
    if (Landscape) Landscape->SetGardenOutline(Target.shown, Target.cellX, Target.cellY, Target.valid);
}
