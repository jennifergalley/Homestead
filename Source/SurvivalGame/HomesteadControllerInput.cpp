#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "UI/HomesteadMenuPortrait.h"
#include "UI/SHomesteadArrival.h"
#include "UI/SHomesteadMenu.h"
#include "UI/SHomesteadNames.h"
#include "UI/SHomesteadShop.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"

namespace ControllerInput
{
class FHomesteadMenuPointerInput final : public IInputProcessor
{
public:
    explicit FHomesteadMenuPointerInput(AHomesteadController* InController) : Controller(InController) {}
    virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
    virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        if (!Controller.IsValid()) return false;
        Controller->MenuPointerIntent(Event.GetCursorDelta().X, Event.GetCursorDelta().Y);
        return !Controller->MenuAcceptsPhysicalInput();
    }
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        return Controller.IsValid() && !Controller->MenuPointerButtonIntent(Event.GetEffectingButton());
    }
    virtual bool HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent& Event) override
    {
        return Controller.IsValid() && !Controller->MenuPointerButtonIntent(Event.GetEffectingButton());
    }
    virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&, const FPointerEvent& Event, const FPointerEvent*) override
    {
        return Controller.IsValid() && !Controller->MenuPhysicalInput(EKeys::MouseWheelAxis, IE_Axis, Event.GetWheelDelta());
    }
private:
    TWeakObjectPtr<AHomesteadController> Controller;
};
}

using ControllerInput::FHomesteadMenuPointerInput;

bool AHomesteadController::InputKey(const FInputKeyEventArgs& Params)
{
    if (bAutomatedInputOnly && !Params.IsSimulatedInput())
    {
        ++IgnoredExternalInputs;
        if (!bLoggedExternalInput && (Params.Event == IE_Pressed || FMath::Abs(Params.AmountDepressed) > 0.15f))
        {
            UE_LOG(LogTemp, Display, TEXT("Automation ignored external input (test-mode isolation; no key contents recorded)."));
            bLoggedExternalInput = true;
        }
        return true;
    }
    FInputAxisProperties AxisProperties;
    const bool HasAxisProperties = Params.Key.IsGamepadKey() && Params.Key.IsAnalog() && PlayerInput
        && PlayerInput->GetAxisProperties(Params.Key, AxisProperties);
    const EHomesteadPromptDevice Intent = PromptIntent.Classify(Params, FPlatformTime::Seconds(), HasAxisProperties ? &AxisProperties : nullptr);
    if (Intent != EHomesteadPromptDevice::None)
    {
        const bool NextGamepad = Intent == EHomesteadPromptDevice::Gamepad;
        if (NextGamepad != bGamepad) ++PromptDeviceChanges;
        bGamepad = NextGamepad;
    }
    if (Params.Key == EKeys::LeftControl || Params.Key == EKeys::RightControl)
    {
        if (Params.Event == IE_Pressed) bControlDown = true;
        else if (Params.Event == IE_Released) bControlDown = false;
    }
    TrackSprintShift(Params);
    if (NamesWidget.IsValid())
    {
        // The Names step owns input; real keys reach it through Slate focus first.
        if (Params.Event == IE_Pressed) NamesWidget->HandleKey(Params.Key);
        return true;
    }
    if (NativeMenu.IsValid()) bShowMouseCursor = !bGamepad;
    if (NativeMenu.IsValid() && (bBookOpen || IsFailed()))
    {
        if (bMenuSaveInProgress) return true;
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F5)
        { if (NativeMenu->PrepareQuickAction()) QuickSave(); return true; }
        if (Params.Event == IE_Pressed && Params.Key == EKeys::F9)
        { if (NativeMenu->PrepareQuickAction()) QuickLoad(); return true; }
        const auto Menu = NativeMenu;
        return Menu->HandleKey(Params.Key, Params.Event, Params.AmountDepressed);
    }
    if (ShopScreen.IsValid())
    {
        const auto Shop = ShopScreen;
        return Shop->HandleKey(Params.Key, Params.Event, Params.AmountDepressed);
    }
    if (bPlanning && !bBookOpen && !IsFailed() && Params.Key == EKeys::MouseWheelAxis && Params.Event == IE_Axis
        && FMath::Abs(Params.AmountDepressed) >= 1.0f
        && !(bControlDown || IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)
            || FSlateApplication::Get().GetModifierKeys().IsControlDown()))
    {
        RotatePlacementBy(Params.AmountDepressed > 0 ? 1 : -1);
        return true;
    }
    if (!bBookOpen && !bPlanning && !IsFailed())
    {
        static const FKey NumberKeys[] = {
            EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
            EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero
        };
        if (Params.Event == IE_Pressed)
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
                if (Params.Key == NumberKeys[Index])
                {
                    SelectHotbarSlot(Index);
                    return true;
                }
        if (Params.Key == EKeys::MouseWheelAxis && Params.Event == IE_Axis
            && FMath::Abs(Params.AmountDepressed) >= 1.0f)
        {
            // Slate's modifier state is the OS's: a Ctrl press that a focused widget took never reaches
            // InputKey, so bControlDown alone missed it and Ctrl+wheel cycled the hotbar instead.
            const bool Control = bControlDown || IsInputKeyDown(EKeys::LeftControl)
                || IsInputKeyDown(EKeys::RightControl) || FSlateApplication::Get().GetModifierKeys().IsControlDown();
            if (Control)
            {
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                    Avatar->Zoom(Params.AmountDepressed);
            }
            else CycleHotbar(Params.AmountDepressed > 0 ? -1 : 1);
            return true;
        }
    }
    return Super::InputKey(Params);
}

bool AHomesteadController::MenuPhysicalInput(FKey Key, EInputEvent Event, float Amount)
{
    if (bAutomatedInputOnly && bSimulatedMenuEvent)
        InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Amount));
    else
        InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, Event, Amount, false, FPlatformTime::Cycles64()));
    return MenuAcceptsPhysicalInput();
}

bool AHomesteadController::MenuPointerButtonIntent(FKey Key)
{
    if (!MenuAcceptsPhysicalInput()) return false;
    if (Key.IsMouseButton())
    {
        if (bGamepad) ++PromptDeviceChanges;
        bGamepad = false;
        bShowMouseCursor = true;
    }
    return true;
}

bool AHomesteadController::MenuPointerIntent(float X, float Y)
{
    MenuPhysicalInput(EKeys::MouseX, IE_Axis, X);
    MenuPhysicalInput(EKeys::MouseY, IE_Axis, Y);
    if (NativeMenu.IsValid() && FSlateApplication::IsInitialized())
        NativeMenu->PointerItemDragMove(FSlateApplication::Get().GetCursorPos());
    return !bAutomatedInputOnly && !bGamepad;
}

void AHomesteadController::ShowNativeMenu()
{
    if (!GEngine || !GEngine->GameViewport) return;
    HoveredHotbarSlot = INDEX_NONE;
    RefreshMenuPortrait();
    if (!NativeMenu.IsValid())
    {
        FlushPressedKeys();
        NativeMenu = SNew(SHomesteadMenu).Controller(this);
        GEngine->GameViewport->AddViewportWidgetContent(NativeMenu.ToSharedRef(), 100);
        MenuPointerInput = MakeShared<FHomesteadMenuPointerInput>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(MenuPointerInput);
    }
    bShowMouseCursor = !bGamepad;
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(NativeMenu);
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    NativeMenu->Refresh();
}

void AHomesteadController::HideNativeMenu()
{
    if (MenuPointerInput.IsValid() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().UnregisterInputPreProcessor(MenuPointerInput);
    MenuPointerInput.Reset();
    // Closing the book keeps a sound level she stepped with the d-pad (saved once, here).
    if (NativeMenu.IsValid()) NativeMenu->CommitAudioStep();
    if (NativeMenu.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(NativeMenu.ToSharedRef());
    NativeMenu.Reset();
    if (MenuPortrait) MenuPortrait->Destroy();
    MenuPortrait = nullptr;
    PortraitBrush.SetResourceObject(nullptr);
    FlushPressedKeys();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}

void AHomesteadController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AHomesteadController::Interact);
    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AHomesteadController::Secondary);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AHomesteadController::Secondary);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AHomesteadController::UseSelectedTool);
    InputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &AHomesteadController::UseSelectedTool);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AHomesteadController::OpenFocusedChestWithMouse);
    InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AHomesteadController::ToggleDeconstruct);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AHomesteadController::Withdraw);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AHomesteadController::Back);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AHomesteadController::OpenSettings);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AHomesteadController::OpenCraft);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AHomesteadController::OpenBuild);
    InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AHomesteadController::OpenMap);
    // View opens the field book at the pack (it opened the retired Guidebook).
    InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AHomesteadController::ToggleBook);
    InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AHomesteadController::PreviousRow);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AHomesteadController::NextRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AHomesteadController::PreviousRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &AHomesteadController::NextRow);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &AHomesteadController::PreviousPage);
    InputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &AHomesteadController::NextPage);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AHomesteadController::RotatePlacement);
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AHomesteadController::NextSeed);
    InputComponent->BindKey(EKeys::Gamepad_RightThumbstick, IE_Pressed, this, &AHomesteadController::CycleZoom);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AHomesteadController::QuickSave);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AHomesteadController::QuickLoad);
}
