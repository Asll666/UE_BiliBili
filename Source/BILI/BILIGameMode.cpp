// Copyright Epic Games, Inc. All Rights Reserved.

#include "BILIGameMode.h"
#include "BILICharacter.h"
#include "UObject/ConstructorHelpers.h"

ABILIGameMode::ABILIGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
