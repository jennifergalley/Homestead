#pragma once

#include "CoreMinimal.h"

namespace HomesteadControllerText
{
inline FString Text(const char* Value) { return UTF8_TO_TCHAR(Value); }

inline FString SleepClockText(double Hour)
{
    const int32 Minutes = FMath::RoundToInt32(FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0) * 60.0) % (24 * 60);
    return FString::Printf(TEXT("%02d:%02d"), Minutes / 60, Minutes % 60);
}
}
