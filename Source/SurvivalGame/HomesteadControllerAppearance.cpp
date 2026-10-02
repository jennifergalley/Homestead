// The Appearance page's camera controls (UI/SHomesteadMenu): drag on her, WASD or the right stick to
// orbit, and the wheel to zoom. They act only while the book is open on Appearance.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"

void AHomesteadController::MenuOrbitAppearance(float Yaw, float Pitch)
{
    if (!bBookOpen || Page != 6) return;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->OrbitAppearance(Yaw, Pitch);
}

void AHomesteadController::MenuZoomAppearance(float Steps)
{
    if (!bBookOpen || Page != 6) return;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->ZoomAppearance(Steps);
}
