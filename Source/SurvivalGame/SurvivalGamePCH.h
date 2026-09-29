// Private precompiled header for the SurvivalGame module.
// The engine's shared PCHs are 2.0 GB (Engine) and 2.4 GB (UnrealEd, used for the editor target). Each
// compiler process loads the whole PCH, so on this shared machine a smaller PCH means less memory per
// compile and less paging. Keep this to headers most of the module already includes; files still
// include what they use.
#pragma once

#include "CoreMinimal.h"
#include <algorithm>
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
