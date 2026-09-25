#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HomesteadAgentPlayLibrary.generated.h"

/**
 * Editor-only input and state bridge for agents playing Homestead in Play-In-Editor.
 * Input goes through APlayerController::InputKey as simulated events, the same path the
 * game's own playtest harnesses use, so it reaches both gameplay and the native menu.
 * Exposed to Python and, through Content/Python/homestead_agent, to the editor MCP server.
 */
UCLASS()
class UHomesteadAgentPlayLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Presses and releases a key (e.g. "E", "Tab", "Escape", "Gamepad_FaceButton_Bottom"). */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString TapKey(const FString& Key);

    /** Holds a key for Seconds, then releases it. */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString HoldKey(const FString& Key, float Seconds);

    /**
     * Applies gamepad sticks every frame for Seconds, then centers them.
     * Move: X right, Y forward. Look: X turn right, Y look up. Values -1..1.
     */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString SetSticks(float MoveX, float MoveY, float LookX, float LookY, float Seconds);

    /** Centers sticks and releases held keys. */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString ReleaseAll();

    /**
     * Walks toward a world XY point using the left stick (turning the camera toward it with the
     * right stick) until within StopDistanceCm or TimeoutSeconds elapse. Non-blocking; poll
     * GetPlayState().walk for "walking", "arrived", "timeout", "stuck" or "cancelled".
     */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString WalkTo(float X, float Y, float StopDistanceCm = 150.f, float TimeoutSeconds = 20.f);

    /** True while stick or held-key input from this library is still being applied. */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static bool IsInputActive();

    /** JSON snapshot of the PIE player, menu, needs, and the nearest resource nodes. */
    UFUNCTION(BlueprintCallable, Category = "Homestead Agent")
    static FString GetPlayState(int32 NearbyCount = 8, float RadiusCm = 4000.f);
};
