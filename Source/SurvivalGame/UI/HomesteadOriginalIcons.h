#pragma once

#include "CoreMinimal.h"

struct FSlateDynamicImageBrush;

namespace HomesteadOriginalIcons
{
TSharedPtr<FSlateDynamicImageBrush> Load(FName Kind);
}
