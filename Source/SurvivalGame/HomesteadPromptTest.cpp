#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadPromptIntent.h"
#include "InputKeyEventArgs.h"

void AHomesteadSmokeTest::PreparePromptChecks()
{
    Add(TEXT("Controller field book starts with controller hints"),
        [this]() { Tap(EKeys::Gamepad_DPad_Up); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0 && Controller->UsesGamepad(); });
    Add(TEXT("Capture the controller field book before incidental mouse input"),
        [this]() { Screenshot(TEXT("prompts-book-before")); }, []() { return true; });
    Add(TEXT("Accepted tiny mouse delta while the controller field book is paused"),
        [this]() { Axis(EKeys::MouseX, 0.01f); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Capture the actual field-book labels after the tiny mouse delta"),
        [this]() { Screenshot(TEXT("prompts-book-after")); }, []() { return true; });
    Add(TEXT("Tiny mouse camera input must not steal paused controller hints"),
        []() {}, [this]() { return Controller->UsesGamepad(); });

    using Device = EHomesteadPromptDevice;
    auto Classifier = MakeShared<FHomesteadPromptIntent>();
    const auto Case = [this, Classifier](const FString& Name, FKey Key, EInputEvent Event, float Value, double Time, Device Expected)
    {
        Add(TEXT("Intent policy: ") + Name, []() {}, [Classifier, Key, Event, Value, Time, Expected]()
        {
            return Classifier->Classify(FInputKeyEventArgs::CreateSimulated(Key, Event, Value, 1), Time) == Expected;
        }, 0.02f);
    };
    Case(TEXT("zero mouse"), EKeys::MouseX, IE_Axis, 0, 0, Device::None);
    Case(TEXT("tiny mouse"), EKeys::MouseX, IE_Axis, 0.01f, 0.01, Device::None);
    Case(TEXT("mouse jitter does not accumulate forever"), EKeys::MouseX, IE_Axis, 0.01f, 1, Device::None);
    Case(TEXT("keyboard release"), EKeys::W, IE_Released, 0, 1.01, Device::None);
    Case(TEXT("keyboard repeat"), EKeys::W, IE_Repeat, 1, 1.02, Device::None);
    Case(TEXT("gamepad release"), EKeys::Gamepad_FaceButton_Bottom, IE_Released, 0, 1.03, Device::None);
    Case(TEXT("zero stick"), EKeys::Gamepad_LeftX, IE_Axis, 0, 1.04, Device::None);
    Case(TEXT("stick inside existing deadzone"), EKeys::Gamepad_LeftX, IE_Axis, 0.19f, 1.05, Device::None);
    Case(TEXT("neutral clears cached stick"), EKeys::Gamepad_LeftX, IE_Axis, 0, 1.06, Device::None);
    Case(TEXT("diagonal first component below radial threshold"), EKeys::Gamepad_LeftX, IE_Axis, 0.15f, 1.07, Device::None);
    Case(TEXT("diagonal combined intent matches radial modifier"), EKeys::Gamepad_LeftY, IE_Axis, 0.15f, 1.08, Device::Gamepad);
    Case(TEXT("click switches immediately during held stick"), EKeys::LeftMouseButton, IE_Pressed, 1, 1.09, Device::KeyboardMouse);
    Case(TEXT("held stick cannot immediately undo click"), EKeys::Gamepad_LeftY, IE_Axis, 0.15f, 1.10, Device::None);
    Case(TEXT("held stick resumes after bounded grace"), EKeys::Gamepad_LeftY, IE_Axis, 0.15f, 1.30, Device::Gamepad);
    Case(TEXT("deliberate mouse switches immediately"), EKeys::MouseX, IE_Axis, 1, 1.31, Device::KeyboardMouse);
    Case(TEXT("held stick cannot immediately undo mouse"), EKeys::Gamepad_LeftY, IE_Axis, 0.15f, 1.32, Device::None);
    Case(TEXT("new right-stick gesture bypasses held-stick grace"), EKeys::Gamepad_RightX, IE_Axis, -0.6f, 1.33, Device::Gamepad);
    Case(TEXT("keyboard button bypasses any analog grace"), EKeys::W, IE_Pressed, 1, 1.34, Device::KeyboardMouse);
    Case(TEXT("gamepad button bypasses grace"), EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, 1, 1.35, Device::Gamepad);
    Case(TEXT("fractional mouse travel one"), EKeys::MouseX, IE_Axis, 0.25f, 2, Device::None);
    Case(TEXT("fractional mouse travel two"), EKeys::MouseX, IE_Axis, 0.25f, 2.02, Device::None);
    Case(TEXT("fractional mouse travel three"), EKeys::MouseX, IE_Axis, 0.25f, 2.04, Device::None);
    Case(TEXT("fractional mouse intent accumulates within120ms"), EKeys::MouseX, IE_Axis, 0.25f, 2.06, Device::KeyboardMouse);
    Case(TEXT("wheel intent"), EKeys::MouseWheelAxis, IE_Axis, -1, 2.07, Device::KeyboardMouse);
    Case(TEXT("trigger engagement"), EKeys::Gamepad_LeftTriggerAxis, IE_Axis, 0.8f, 2.08, Device::Gamepad);
    Case(TEXT("keyboard during trigger"), EKeys::I, IE_Pressed, 1, 2.09, Device::KeyboardMouse);
    Case(TEXT("held trigger cannot undo keyboard"), EKeys::Gamepad_LeftTriggerAxis, IE_Axis, 0.8f, 2.10, Device::None);
    Case(TEXT("released trigger"), EKeys::Gamepad_LeftTriggerAxis, IE_Axis, 0, 2.11, Device::None);
    Case(TEXT("new trigger engagement is immediate"), EKeys::Gamepad_LeftTriggerAxis, IE_Axis, 0.8f, 2.12, Device::Gamepad);
    Case(TEXT("partial mouse travel"), EKeys::MouseX, IE_Axis, 0.9f, 3, Device::None);
    Case(TEXT("controller button discards old partial mouse travel"), EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, 1, 3.01, Device::Gamepad);
    Case(TEXT("post-button noise cannot complete old mouse gesture"), EKeys::MouseX, IE_Axis, 0.1f, 3.02, Device::None);

    Add(TEXT("Mapped keyboard closes the paused book and selects keyboard hints"),
        [this]() { Tap(EKeys::I); }, [this]() { return !Controller->IsBookOpen() && !Controller->UsesGamepad(); });
    Add(TEXT("Meaningful mapped gamepad walking survives incidental mouse noise"),
        [this]() { MovementStart = Controller->GetPawn()->GetActorLocation(); },
        [this]() { return Controller->UsesGamepad() && FVector::Dist2D(MovementStart, Controller->GetPawn()->GetActorLocation()) > 15; }, 0.65f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 0.8f); Axis(EKeys::MouseX, 0.01f); };
    Add(TEXT("Stick release and zero mouse leave controller context hints"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); Axis(EKeys::MouseX, 0); },
        [this]() { return Controller->UsesGamepad() && Controller->FocusActions().Contains(TEXT("[X]")); });
    Add(TEXT("Subthreshold mouse still moves the actual camera without stealing hints"),
        [this]() { CameraStart = Controller->GetControlRotation().Yaw; Axis(EKeys::MouseX, 0.25f); },
        [this]()
        {
            const float Delta = FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw));
            Results.Add(FString::Printf(TEXT("OBSERVE synthetic MouseX0.25 yaw_delta=%.8f prompts_gamepad=%d"), Delta, Controller->UsesGamepad()));
            return Controller->UsesGamepad() && Delta > 0.001f;
        });
    Add(TEXT("Capture stable controller context"), [this]() { Screenshot(TEXT("prompts-context-gamepad")); }, []() { return true; });
    Add(TEXT("Deliberate mapped mouse camera movement selects keyboard hints"),
        [this]() { CameraStart = Controller->GetControlRotation().Yaw; Axis(EKeys::MouseX, 4); },
        [this]() { return !Controller->UsesGamepad() && Controller->FocusActions().Contains(TEXT("[F]"))
            && FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw)) > 0.1f; });
    Add(TEXT("Controller drift and releases cannot steal keyboard context"),
        [this]() { Axis(EKeys::Gamepad_LeftX, 0.35f); Axis(EKeys::Gamepad_RightX, 0.35f);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_FaceButton_Bottom, IE_Released, 0)); },
        [this]() { return !Controller->UsesGamepad(); });
    Add(TEXT("Capture intentional keyboard context"), [this]() { Screenshot(TEXT("prompts-context-keyboard")); }, []() { return true; });
    Add(TEXT("Open the keyboard field book; subthreshold stick stays keyboard"),
        [this]() { Axis(EKeys::Gamepad_LeftX, 0); Axis(EKeys::Gamepad_RightX, 0); Tap(EKeys::I); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0 && !Controller->UsesGamepad(); });
    Add(TEXT("Capture intentional keyboard book"), [this]() { Screenshot(TEXT("prompts-book-keyboard")); }, []() { return true; });
    Add(TEXT("Actual diagonal stick intent respects inherited axis shaping plus radial modifier"),
        [this]() { Axis(EKeys::Gamepad_LeftX, 0.36f); Axis(EKeys::Gamepad_LeftY, 0.36f); },
        [this]() { return Controller->UsesGamepad() && Controller->IsBookOpen(); });
    Add(TEXT("Diagonal release does not select keyboard"),
        [this]() { Axis(EKeys::Gamepad_LeftX, 0); Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->UsesGamepad(); });
    Add(TEXT("Controller page navigation selects controller Settings"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->BookPage() == 4 && Controller->UsesGamepad(); });
    Add(TEXT("Capture controller Settings"), [this]() { Screenshot(TEXT("prompts-settings-gamepad")); }, []() { return true; });
    Add(TEXT("Keyboard row navigation selects keyboard Settings"),
        [this]() { Tap(EKeys::Down); }, [this]() { return Controller->BookPage() == 4 && !Controller->UsesGamepad(); });
    Add(TEXT("Capture keyboard Settings"), [this]() { Screenshot(TEXT("prompts-settings-keyboard")); }, []() { return true; });
    Add(TEXT("Navigate to Look through mapped controller pages"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->BookPage() == 5; });
    Add(TEXT("Controller Look page"), [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->BookPage() == 6 && Controller->UsesGamepad(); });
    Add(TEXT("Tiny mouse Look remains responsive without changing controller hints"),
        [this]() { CameraStart = Controller->GetControlRotation().Yaw; PausedHour = Controller->State().hour; Axis(EKeys::MouseX, 0.25f); },
        [this]() { return Controller->UsesGamepad() && Controller->State().hour == PausedHour
            && FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw)) > 0.001f; });
    Add(TEXT("Capture controller Look"), [this]() { Screenshot(TEXT("prompts-look-gamepad")); }, []() { return true; });
    Add(TEXT("Deliberate mouse orbit selects keyboard Look"),
        [this]() { Axis(EKeys::MouseX, 3); },
        [this]() { return !Controller->UsesGamepad() && Controller->BookPage() == 6 && Controller->State().hour == PausedHour; });
    Add(TEXT("Capture keyboard Look"), [this]() { Screenshot(TEXT("prompts-look-keyboard")); }, []() { return true; });
    Add(TEXT("Mapped controller camera selects controller Look"),
        [this]() { CameraStart = Controller->GetControlRotation().Yaw; Axis(EKeys::Gamepad_RightX, 0.8f); },
        [this]() { return Controller->UsesGamepad() && FMath::Abs(FMath::FindDeltaAngleDegrees(CameraStart, Controller->GetControlRotation().Yaw)) > 0.1f; });
    Add(TEXT("Click gets visible priority over continuously held camera stick"),
        [this]() { Tap(EKeys::LeftMouseButton); },
        [this]() { return !Controller->UsesGamepad(); }, 0.08f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_RightX, 0.8f); };
    Add(TEXT("Ongoing real controller camera takes over after200ms"),
        []() {}, [this]() { return Controller->UsesGamepad(); }, 0.3f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_RightX, 0.8f); };
    Add(TEXT("Stop stick and open Build using keyboard mapping"),
        [this]() { Axis(EKeys::Gamepad_RightX, 0); Tap(EKeys::B); },
        [this]() { return Controller->BookPage() == 2 && !Controller->UsesGamepad(); });
    Add(TEXT("Controller enters planning without committing a structure"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsPlanning() && Controller->UsesGamepad(); });
    Add(TEXT("Tiny mouse cannot steal controller planning hints"),
        [this]() { Axis(EKeys::MouseY, 0.01f); },
        [this]() { return Controller->IsPlanning() && Controller->UsesGamepad(); });
    Add(TEXT("Capture controller planning"), [this]() { Screenshot(TEXT("prompts-planning-gamepad")); }, []() { return true; });
    Add(TEXT("Mapped keyboard movement selects keyboard planning"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1)); },
        [this]() { return Controller->IsPlanning() && !Controller->UsesGamepad(); });
    Add(TEXT("Keyboard release retains keyboard planning"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0)); },
        [this]() { return Controller->IsPlanning() && !Controller->UsesGamepad(); });
    Add(TEXT("Capture keyboard planning"), [this]() { Screenshot(TEXT("prompts-planning-keyboard")); }, []() { return true; });
    Add(TEXT("Controller back exits planning and restores controller context"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning() && Controller->UsesGamepad(); });
}
