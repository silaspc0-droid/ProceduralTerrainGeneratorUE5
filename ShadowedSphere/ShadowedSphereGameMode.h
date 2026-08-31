// Copyright Epic Games, Inc. All Rights Reserved.
// Default game mode for ShadowedSphere; selects the player pawn class.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShadowedSphereGameMode.generated.h"

UCLASS(minimalapi)
class AShadowedSphereGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShadowedSphereGameMode();
};
