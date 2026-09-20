#pragma once

#include "Commandlets/Commandlet.h"
#include "HomesteadAuthoringProbeCommandlet.generated.h"

UCLASS()
class UHomesteadAuthoringProbeCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UHomesteadAuthoringProbeCommandlet();
    virtual int32 Main(const FString& Params) override;
};
