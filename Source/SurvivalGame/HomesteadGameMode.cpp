#include "HomesteadGameMode.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadHUD.h"

AHomesteadGameMode::AHomesteadGameMode()
{
    DefaultPawnClass = AHomesteadCharacter::StaticClass();
    PlayerControllerClass = AHomesteadController::StaticClass();
    HUDClass = AHomesteadHUD::StaticClass();
}
