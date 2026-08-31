// Copyright Epic Games, Inc. All Rights Reserved.
// Implementation of AShadowedSphereGameMode: sets the default pawn to the Blueprinted
// third-person character.

#include "ShadowedSphereGameMode.h"
#include "ShadowedSphereCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShadowedSphereGameMode::AShadowedSphereGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
