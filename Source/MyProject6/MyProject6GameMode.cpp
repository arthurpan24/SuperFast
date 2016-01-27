// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MyProject6.h"
#include "MyProject6GameMode.h"
#include "MyProject6Character.h"

AMyProject6GameMode::AMyProject6GameMode()
{
	// set default pawn class to our character
	DefaultPawnClass = AMyProject6Character::StaticClass();	
}
