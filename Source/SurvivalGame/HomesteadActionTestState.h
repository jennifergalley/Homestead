#pragma once

#include "HomesteadController.h"

inline bool MatchesActionState(const AHomesteadController& Controller, const Homestead::Simulation& Transaction, double Hour)
{
    auto Expected = Transaction;
    Expected.AdvanceGameHours(Controller.State().hour - Hour, Controller.PlayerPoint());
    const auto& A = Controller.State();
    const auto& B = Expected.GetState();
    if (A.inventory != B.inventory || A.plots.size() != B.plots.size()
        || A.resources.size() != B.resources.size()) return false;
    for (size_t I = 0; I < A.plots.size(); ++I)
        if (A.plots[I].id != B.plots[I].id || A.plots[I].planted != B.plots[I].planted
            || A.plots[I].kind != B.plots[I].kind
            || FMath::Abs(A.plots[I].moisture - B.plots[I].moisture) > 0.000001
            || FMath::Abs(A.plots[I].weeds - B.plots[I].weeds) > 0.000001
            || FMath::Abs(A.plots[I].growth - B.plots[I].growth) > 0.000001) return false;
    for (size_t I = 0; I < A.resources.size(); ++I)
        if (A.resources[I].id != B.resources[I].id || A.resources[I].cleared != B.resources[I].cleared
            || A.resources[I].readyAtHour != B.resources[I].readyAtHour) return false;
    return true;
}
