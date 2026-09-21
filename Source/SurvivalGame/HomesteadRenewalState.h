#pragma once

#include "CoreMinimal.h"
#include "Simulation/HomesteadSimulation.h"

struct FHomesteadRenewalState
{
    enum class Kind { Load, VerifyLoad, Walk, Face, Reject, Open, Save, Reload, VerifySave, Close,
        Food, Rest, Ready, Gather, Recover, Cleared, Finish };
    struct Step
    {
        Kind Command;
        int32 Node = -1;
        FVector2D Target = FVector2D::ZeroVector;
        FString Label;
    };
    TArray<Step> Steps;
    TArray<FString> Events, Snapshots;
    TMap<int32, double> InitialDeadlines;
    FString ControlPath, RunId, WorldId, SavedState;
    FDateTime Deadline;
    double Started = 0, LastTick = 0, StepStarted = 0, NextControl = 0, NextReport = 0;
    double InitialHour = 0, SleepHours = 0, Unpaused = 0, Paused = 0, EngineUnpaused = 0;
    double ActionHour = 0, ActionEngine = 0, BeforeDeadline = 0, ProgressAt = 0, ProgressDistance = 0, LastButton = 0;
    double BeforeToastSeconds = 0;
    Homestead::Inventory BeforeInventory{};
    uint32 BeforeStarts = 0;
    int32 Index = 0, EnteredIndex = -1, Checks = 0, Sleeps = 0, Eats = 0, Rejects = 0, Harvests = 0;
    int32 Walks = 0, Saves = 0, RoundTrips = 0, FoodBefore = 0;
    bool Loaded = false, ReadOnly = false, FoodOpen = false, FoodPending = false, FoodClosing = false;
    bool CompletionDriven = false;
};
