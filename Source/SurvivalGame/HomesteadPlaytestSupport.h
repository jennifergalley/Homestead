#pragma once

// Shared by the agent test tools (HomesteadControllerTest*.cpp): the one LogHomesteadPlaytest category
// and name lookups that resolve items the same way HomesteadGive and the sell helper do. The category
// is defined once, in HomesteadControllerTestShop.cpp. See .github\skills\homestead-test-tools\SKILL.md.
#include "CoreMinimal.h"
#include "Simulation/HomesteadItems.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHomesteadPlaytest, Log, All);

namespace HomesteadPlaytest
{
// Comparison key: spaces, dashes and underscores stripped (compare with IgnoreCase).
inline FString Compact(const FString& Text)
{
    return Text.Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT("")).Replace(TEXT("_"), TEXT(""));
}

inline bool SameName(const FString& A, const FString& B)
{
    return Compact(A).Equals(Compact(B), ESearchCase::IgnoreCase);
}

inline FString ToText(const std::string& Value)
{
    return FString(UTF8_TO_TCHAR(Value.c_str()));
}

inline FString ToText(const char* Value)
{
    return FString(UTF8_TO_TCHAR(Value ? Value : ""));
}

// An item by display name ("Copper ore") or key ("copper-ore"), spaces and case ignored.
inline bool FindItem(const FString& Name, Homestead::Item& Found)
{
    const FString Wanted = Compact(Name);
    if (Wanted.IsEmpty()) return false;
    for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
    {
        const auto Candidate = static_cast<Homestead::Item>(Index);
        const char* Key = Homestead::ItemKey(Candidate);
        if (Compact(ToText(Homestead::ItemName(Candidate))).Equals(Wanted, ESearchCase::IgnoreCase)
            || (Key && Compact(ToText(Key)).Equals(Wanted, ESearchCase::IgnoreCase)))
        {
            Found = Candidate;
            return true;
        }
    }
    return false;
}
}
