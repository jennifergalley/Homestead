#include "HomesteadPromptIntent.h"
#include "InputKeyEventArgs.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerInput.h"

EHomesteadPromptDevice FHomesteadPromptIntent::Classify(const FInputKeyEventArgs& Event, double Now, const FInputAxisProperties* AxisProperties)
{
    const auto Deliberate = [this, Now](EHomesteadPromptDevice Device)
    {
        LastDeliberate = Device;
        LastDeliberateAt = Now;
        if (Device == EHomesteadPromptDevice::Gamepad)
        { MouseTravel = FVector2D::ZeroVector; MouseWindowStarted = -1; }
        return Device;
    };
    if (!Event.Key.IsValid()) return EHomesteadPromptDevice::None;
    if (!Event.Key.IsAnalog())
        return Event.Event == IE_Pressed
            ? Deliberate(Event.Key.IsGamepadKey() ? EHomesteadPromptDevice::Gamepad : EHomesteadPromptDevice::KeyboardMouse)
            : EHomesteadPromptDevice::None;
    if (Event.Event != IE_Axis || !FMath::IsFinite(Event.AmountDepressed)) return EHomesteadPromptDevice::None;
    float Value = Event.AmountDepressed;
    if (Event.Key.IsGamepadKey())
    {
        // Hint-space only: honor the inherited per-axis shaping before the mapped stick modifier.
        if (AxisProperties)
        {
            const float Range = 1.0f - AxisProperties->DeadZone;
            const float Magnitude = Range > 0 ? FMath::Max(0.0f, FMath::Abs(Value) - AxisProperties->DeadZone) / Range : 0;
            Value = FMath::Sign(Value) * FMath::Pow(Magnitude, AxisProperties->Exponent) * AxisProperties->Sensitivity;
        }
        const auto* Deadzone = GetDefault<UInputModifierDeadZone>();
        const auto Active = [Deadzone](FVector2D Axis)
        {
            const double Magnitude = Deadzone->Type == EDeadZoneType::Radial
                ? Axis.Size() : FMath::Max(FMath::Abs(Axis.X), FMath::Abs(Axis.Y));
            return Magnitude > Deadzone->LowerThreshold;
        };
        FVector2D* Stick = nullptr;
        bool Vertical = false;
        if (Event.Key == EKeys::Gamepad_LeftX || Event.Key == EKeys::Gamepad_LeftY)
        { Stick = &LeftStick; Vertical = Event.Key == EKeys::Gamepad_LeftY; }
        if (Event.Key == EKeys::Gamepad_RightX || Event.Key == EKeys::Gamepad_RightY)
        { Stick = &RightStick; Vertical = Event.Key == EKeys::Gamepad_RightY; }
        const bool WasActive = Stick ? Active(*Stick)
            : FMath::Abs(OtherGamepadAxes.FindRef(Event.Key)) > Deadzone->LowerThreshold;
        if (Stick) { if (Vertical) Stick->Y = Value; else Stick->X = Value; }
        else OtherGamepadAxes.Add(Event.Key, Value);
        if (FMath::IsNearlyZero(Value) || !(Stick ? Active(*Stick) : FMath::Abs(Value) > Deadzone->LowerThreshold))
            return EHomesteadPromptDevice::None;
        if (!WasActive) return Deliberate(EHomesteadPromptDevice::Gamepad);
        // A continuously polled held stick must not immediately undo a deliberate key/click/mouse gesture.
        if (LastDeliberate == EHomesteadPromptDevice::KeyboardMouse && Now - LastDeliberateAt < 0.2)
            return EHomesteadPromptDevice::None;
        return EHomesteadPromptDevice::Gamepad;
    }
    if (Event.Key == EKeys::MouseX || Event.Key == EKeys::MouseY)
    {
        // Short signed accumulation keeps fine mouse intent, without accumulating idle jitter indefinitely.
        if (MouseWindowStarted < 0 || Now - MouseWindowStarted > 0.12)
        { MouseTravel = FVector2D::ZeroVector; MouseWindowStarted = Now; }
        if (Event.Key == EKeys::MouseX) MouseTravel.X += Value;
        else MouseTravel.Y += Value;
        if (MouseTravel.SizeSquared() < 1.0) return EHomesteadPromptDevice::None;
        MouseTravel = FVector2D::ZeroVector;
        MouseWindowStarted = Now;
        return Deliberate(EHomesteadPromptDevice::KeyboardMouse);
    }
    if (Event.Key == EKeys::MouseWheelAxis && FMath::Abs(Value) >= 1.0f)
        return Deliberate(EHomesteadPromptDevice::KeyboardMouse);
    return EHomesteadPromptDevice::None;
}
