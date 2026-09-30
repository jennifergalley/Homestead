#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadGardenProbe.generated.h"

class AHomesteadController;

// Development-only acceptance for the garden outline on a real, already-loaded save
// (-HomesteadGardenProbe=<output directory>, with -UserDir pointing at a copy of the player's saves):
// the hoe's green and red squares and the pail's green and red plot beside her own garden, each
// recorded and captured with the HUD, then the game exits. It pauses autosave in memory and never saves.
// Never spawned in Shipping.
UCLASS()
class SURVIVALGAME_API AHomesteadGardenProbe : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadGardenProbe();
    virtual void Tick(float DeltaSeconds) override;

private:
    void Stand(Homestead::Point Position, float Yaw);
    FString Record(const TCHAR* Label, bool bExpectValid);
    void Capture(const FString& Name);
    void Finish(const FString& Error);

    UPROPERTY() TObjectPtr<AHomesteadController> Controller;
    FString Output;
    TArray<FString> Lines;
    int32 Step = 0;
    int32 Failures = 0;
    double Next = 0;
    double Deadline = 0;
    Homestead::Point HoeGood, HoeBad, PailPlot;
    int32 HoeGoodX = 0, HoeGoodY = 0, HoeBadX = 0, HoeBadY = 0;
    TArray<Homestead::Point> Refusals;
    int32 Refusal = 0;
    FString PlotsBefore, StockBefore;
};
