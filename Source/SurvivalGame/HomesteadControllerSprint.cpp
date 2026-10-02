// Sprint is a toggle (Jenny, round 2): L3 flips it on press (AHomesteadCharacter::ToggleSprint);
// on the keyboard a tap of Shift flips it on release, so Shift can still be a modifier (Shift+Q in
// the seed pouch). When she's too tired to run the toggle turns itself off, and she
// walks on; nothing forces a collapse.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"

namespace SprintShift
{
bool IsMovementKey(const FKey& Key)
{
    return Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D
        || Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right;
}
}

void AHomesteadController::SprintTooTired()
{
    Notify(TEXT("Too tired to sprint."), true);
}

void AHomesteadController::TrackSprintShift(const FInputKeyEventArgs& Params)
{
    const bool bShift = Params.Key == EKeys::LeftShift || Params.Key == EKeys::RightShift;
    if (bShift && Params.Event == IE_Pressed)
    {
        // Pressed over a menu or the shop, it belongs to them.
        bSprintShiftDown = !bBookOpen && !ShopScreen.IsValid() && !NamesWidget.IsValid() && !IsFailed();
        bSprintShiftModifier = false;
        return;
    }
    if (bShift && Params.Event == IE_Released)
    {
        if (bSprintShiftDown && !bSprintShiftModifier)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->RequestSprintToggle();
        bSprintShiftDown = false;
        return;
    }
    if (bSprintShiftDown && Params.Event == IE_Pressed && !SprintShift::IsMovementKey(Params.Key))
        bSprintShiftModifier = true;
}
