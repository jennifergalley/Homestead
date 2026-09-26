#include "HomesteadGameMode.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadHUD.h"
#include "HomesteadLab.h"

AHomesteadGameMode::AHomesteadGameMode()
{
    DefaultPawnClass = AHomesteadCharacter::StaticClass();
    PlayerControllerClass = AHomesteadController::StaticClass();
    HUDClass = AHomesteadHUD::StaticClass();
}

void AHomesteadGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    const bool Lab = HomesteadLab::Requested();
    PlayerControllerClass = Lab ? AHomesteadLabController::StaticClass() : AHomesteadController::StaticClass();
    HUDClass = Lab ? AHomesteadLabHUD::StaticClass() : AHomesteadHUD::StaticClass();
}
