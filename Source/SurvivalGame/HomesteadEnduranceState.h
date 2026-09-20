#pragma once

#include "CoreMinimal.h"

struct FHomesteadEnduranceState
{
    enum class Phase { Load, Settle, Choose, Walk, Gather, EatSelect, EatCheck, Close, Save, Reload, Resume };
    Phase Step = Phase::Load;
    double Started = 0, LastTick = 0, StepStarted = 0, LastDecision = 0;
    double NextSample = 0, NextCapture = 0, NextSave = 30, NextForage = 0, NextControl = 0;
    double Duration = 2700, Paused = 0, Unpaused = 0, EngineUnpaused = 0, Moving = 0, Distance = 0;
    double InitialHour = 0, ActionHours = 0, CaptureExcludeUntil = 0, StuckAction = 0;
    double ProgressAt = 0, ProgressDistance = 0;
    double FrameSum = 0, FrameMaximum = 0;
    uint64 FrameCount = 0;
    TArray<uint64> FrameHistogram;
    TArray<FString> Samples, Events;
    TMap<FString, int64> SaveStamps;
    TSet<int32> PreviouslyUnavailable;
    FString ControlPath, WorldId, SavedState;
    FString RunId;
    FDateTime DeadlineUtc;
    FVector LastPosition = FVector::ZeroVector;
    FVector2D Target = FVector2D::ZeroVector;
    int32 Waypoint = 0, Waypoints = 0, Gathers = 0, Eats = 0, ManualSaves = 0, Loads = 0;
    int32 AutosaveWrites = 0, Refreshes = 0, NavigationFailures = 0, ForageId = -1;
    int32 BeforeCount = 0, FoodId = -1;
    bool Loaded = false, RoundTrip = false, Finalizing = false;
};
