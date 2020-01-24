// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MotionMatchingTesterGameMode.h"
#include "MotionMatchingTesterCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMotionMatchingTesterGameMode::AMotionMatchingTesterGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
