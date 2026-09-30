// The garden outline: with the hoe or the pail out, a thin border on the ground round the exact garden
// square the next stroke or pour would act on, green when it would work and red (with the reason in the
// focus line) when it wouldn't. Homestead::PreviewGarden picks the square with the same selector the
// actions use (the hoe's 85 cm, the pail's focused plot) and checks it without changing anything.
#include "HomesteadController.h"

#include "HomesteadWorld.h"
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
            Target = Homestead::PreviewGarden(Sim, Homestead::GardenTool::Hoe, PlayerPoint(), Forward.X, Forward.Y);
        // The pail waters the focused plot; at the water it fills instead, so no square then.
        else if (Tool == Homestead::Item::WateringCan && Sim.Count(Tool) > 0 && Focus == EFocus::Plot)
            Target = Homestead::PreviewGarden(Sim, Homestead::GardenTool::Pail, PlayerPoint(), Forward.X, Forward.Y, FocusId);
    }
    GardenOutlineReason = Target.shown && !Target.valid ? FString(UTF8_TO_TCHAR(Target.reason.c_str())) : FString();
    if (Landscape) Landscape->SetGardenOutline(Target.shown, Target.cellX, Target.cellY, Target.valid);
}
