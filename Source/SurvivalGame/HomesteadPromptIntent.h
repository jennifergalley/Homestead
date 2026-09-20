#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

struct FInputKeyEventArgs;
struct FInputAxisProperties;

enum class EHomesteadPromptDevice : uint8 { None, KeyboardMouse, Gamepad };

class FHomesteadPromptIntent
{
public:
    EHomesteadPromptDevice Classify(const FInputKeyEventArgs& Event, double Now, const FInputAxisProperties* AxisProperties = nullptr);

private:
    FVector2D LeftStick = FVector2D::ZeroVector;
    FVector2D RightStick = FVector2D::ZeroVector;
    FVector2D MouseTravel = FVector2D::ZeroVector;
    TMap<FKey, float> OtherGamepadAxes;
    double MouseWindowStarted = -1;
    double LastDeliberateAt = -1;
    EHomesteadPromptDevice LastDeliberate = EHomesteadPromptDevice::None;
};
