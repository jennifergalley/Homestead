#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HomesteadGameMode.generated.h"

UCLASS()
class SURVIVALGAME_API AHomesteadGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHomesteadGameMode();
    // Switches to the character lab controller/HUD when HomesteadLab::Requested().
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
